#include "AuthService.h"

#include <curl/curl.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Constants — swap SUPABASE_URL and SUPABASE_ANON_KEY for your project values
// ---------------------------------------------------------------------------
static const std::string SUPABASE_URL = "https://YOUR_PROJECT.supabase.co";
static const std::string SUPABASE_ANON_KEY = "YOUR_ANON_KEY";
static const std::string SESSION_FILE = "session.json";
static const std::string FAKE_EMAIL_DOMAIN = "@pigeonmail.local";

// ---------------------------------------------------------------------------
// Internal HTTP helper — lives only in this translation unit
// ---------------------------------------------------------------------------
struct HttpResponse {
    int statusCode = 0;
    std::string body;
};

static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb,
                                void* userdata) {
    auto* out = reinterpret_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

// Perform a POST request. Optionally attach a Bearer token.
static HttpResponse httpPost(const std::string& url,
                             const std::string& jsonBody,
                             const std::string& bearerToken = "") {
    HttpResponse result;
    CURL* curl = curl_easy_init();
    if (!curl) return result;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers =
        curl_slist_append(headers, ("apikey: " + SUPABASE_ANON_KEY).c_str());
    if (!bearerToken.empty())
        headers = curl_slist_append(
            headers, ("Authorization: Bearer " + bearerToken).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.statusCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

// Perform a GET request. bearerToken is required for protected endpoints.
static HttpResponse httpGet(const std::string& url,
                            const std::string& bearerToken) {
    HttpResponse result;
    CURL* curl = curl_easy_init();
    if (!curl) return result;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers =
        curl_slist_append(headers, ("apikey: " + SUPABASE_ANON_KEY).c_str());
    headers = curl_slist_append(
        headers, ("Authorization: Bearer " + bearerToken).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.statusCode);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static std::string fakeEmail(const std::string& username) {
    return username + FAKE_EMAIL_DOMAIN;
}

// Parse a Supabase auth error body into a human-readable string.
// Supabase returns: { "error": "...", "error_description": "..." }
// or for newer versions: { "msg": "...", "code": ... }
static std::string parseAuthError(const std::string& body) {
    try {
        auto j = json::parse(body);
        if (j.contains("error_description") &&
            j["error_description"].is_string())
            return j["error_description"].get<std::string>();
        if (j.contains("msg") && j["msg"].is_string())
            return j["msg"].get<std::string>();
        if (j.contains("error") && j["error"].is_string())
            return j["error"].get<std::string>();
    } catch (...) {
    }
    return "Unknown error (status check failed)";
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

AuthResult AuthService::login(const std::string& username,
                              const std::string& password) {
    json body = {{"email", fakeEmail(username)}, {"password", password}};

    HttpResponse resp = httpPost(
        SUPABASE_URL + "/auth/v1/token?grant_type=password", body.dump());

    if (resp.statusCode != 200) {
        return {false, parseAuthError(resp.body)};
    }

    try {
        auto j = json::parse(resp.body);
        _session.accessToken = j.at("access_token").get<std::string>();
        _session.refreshToken = j.at("refresh_token").get<std::string>();
        _session.userId = j.at("user").at("id").get<std::string>();
        _session.expiresAt = std::time(nullptr) + j.value("expires_in", 3600);
        _session.username = username;
    } catch (const std::exception& e) {
        return {false,
                std::string("Failed to parse login response: ") + e.what()};
    }

    if (!fetchUserProfile()) {
        // Non-fatal: we're logged in but couldn't load partner_id yet.
        // DataService::fetchAll() will retry this.
    }

    persistSession();
    return {true, ""};
}

AuthResult AuthService::signup(const std::string& username,
                               const std::string& password) {
    // Step 1: create the Supabase Auth account
    json body = {{"email", fakeEmail(username)}, {"password", password}};

    HttpResponse resp = httpPost(SUPABASE_URL + "/auth/v1/signup", body.dump());

    // 200 = success; 400 = email already taken or weak password
    if (resp.statusCode != 200) {
        return {false, parseAuthError(resp.body)};
    }

    std::string userId;
    std::string accessToken;
    try {
        auto j = json::parse(resp.body);
        userId = j.at("user").at("id").get<std::string>();
        // Supabase may return a session immediately if email confirmation is
        // off
        if (j.contains("access_token"))
            accessToken = j["access_token"].get<std::string>();
    } catch (const std::exception& e) {
        return {false,
                std::string("Failed to parse signup response: ") + e.what()};
    }

    // Step 2: insert a row in public.users
    // We need a token to do this — if Supabase didn't return one, log in first.
    if (accessToken.empty()) {
        AuthResult loginResult = login(username, password);
        if (!loginResult.success)
            return {false, "Account created but auto-login failed: " +
                               loginResult.errorMessage};
        // login() already called persistSession(), so we're done.
        return {true, ""};
    }

    // We got an immediate session — store it temporarily to insert the profile
    // row.
    _session.accessToken = accessToken;
    _session.userId = userId;
    _session.username = username;

    json profileBody = {{"id", userId}, {"username", username}};

    HttpResponse profileResp = httpPost(SUPABASE_URL + "/rest/v1/users",
                                        profileBody.dump(), accessToken);

    if (profileResp.statusCode != 201) {
        // Row might already exist (race); not fatal — proceed.
    }

    // Now do a clean login to get the full session with refresh token.
    return login(username, password);
}

bool AuthService::tryRestoreSession() {
    if (!loadPersistedSession()) return false;

    if (_session.isValid()) return true;

    // Token expired — try a silent refresh
    if (!_session.refreshToken.empty() && doRefreshToken()) return true;

    // Refresh failed — clear stale data and force re-login
    _session = Session{};
    deletePersistedSession();
    return false;
}

void AuthService::logout() {
    _session = Session{};
    deletePersistedSession();
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

bool AuthService::fetchUserProfile() {
    // GET /rest/v1/users?id=eq.{userId}&select=username,partner_id
    std::string url = SUPABASE_URL + "/rest/v1/users" + "?id=eq." +
                      _session.userId + "&select=username,partner_id";

    HttpResponse resp = httpGet(url, _session.accessToken);
    if (resp.statusCode != 200) return false;

    try {
        auto arr = json::parse(resp.body);
        if (arr.empty()) return false;
        auto row = arr[0];

        if (row.contains("username") && row["username"].is_string())
            _session.username = row["username"].get<std::string>();

        if (row.contains("partner_id") && row["partner_id"].is_string())
            _session.partnerId = row["partner_id"].get<std::string>();

    } catch (...) {
        return false;
    }

    persistSession();  // update disk copy with partner_id
    return true;
}

bool AuthService::doRefreshToken() {
    json body = {{"refresh_token", _session.refreshToken}};

    HttpResponse resp = httpPost(
        SUPABASE_URL + "/auth/v1/token?grant_type=refresh_token", body.dump());

    if (resp.statusCode != 200) return false;

    try {
        auto j = json::parse(resp.body);
        _session.accessToken = j.at("access_token").get<std::string>();
        _session.refreshToken = j.at("refresh_token").get<std::string>();
        _session.expiresAt = std::time(nullptr) + j.value("expires_in", 3600);
    } catch (...) {
        return false;
    }

    persistSession();
    return true;
}

void AuthService::persistSession() const {
    json j = {{"userId", _session.userId},
              {"partnerId", _session.partnerId},
              {"username", _session.username},
              {"accessToken", _session.accessToken},
              {"refreshToken", _session.refreshToken},
              {"expiresAt", (long long)_session.expiresAt}};

    std::ofstream f(SESSION_FILE);
    if (f.is_open()) f << j.dump(2);
}

bool AuthService::loadPersistedSession() {
    std::ifstream f(SESSION_FILE);
    if (!f.is_open()) return false;

    try {
        json j = json::parse(f);
        _session.userId = j.value("userId", "");
        _session.partnerId = j.value("partnerId", "");
        _session.username = j.value("username", "");
        _session.accessToken = j.value("accessToken", "");
        _session.refreshToken = j.value("refreshToken", "");
        _session.expiresAt = (std::time_t)j.value("expiresAt", (long long)0);
    } catch (...) {
        return false;
    }

    return !_session.userId.empty();
}

void AuthService::deletePersistedSession() const {
    std::remove(SESSION_FILE.c_str());
}

// ---------------------------------------------------------------------------
// Login UI flow
// ---------------------------------------------------------------------------

void AuthService::runLoginFlow() {
    auto getInput = [](const std::string& prompt) {
        std::string s;
        std::cout << " " << prompt << ": ";
        std::getline(std::cin, s);
        return s;
    };

    while (true) {
        std::cout << "\033[2J\033[H";
        std::cout << "\033[33m "
                     "+==================================================+\n";
        std::cout << " |        >>  P I G E O N   M A I L  <<            |\n";
        std::cout << " +==================================================+"
                     "\033[0m\n\n";
        std::cout << "  [1]  Login\n";
        std::cout << "  [2]  Sign up\n";
        std::cout << "  [0]  Exit\n\n";

        std::string choice;
        std::cout << " Choose (0-2): ";
        std::getline(std::cin, choice);

        if (choice == "0") {
            std::cout << "\nGoodbye.\n";
            std::exit(0);
        }

        std::string username = getInput("Username");
        std::string password = getInput("Password");

        if (username.empty() || password.empty()) {
            std::cout
                << "\n \033[31mUsername and password cannot be empty.\033[0m\n";
            std::cout << " Press Enter to try again...";
            std::cin.ignore();
            continue;
        }

        AuthResult result = (choice == "2") ? signup(username, password)
                                            : login(username, password);

        if (result.success) {
            std::cout << "\n \033[32m[+] Welcome, " << _session.username
                      << "!\033[0m\n";
            std::cout << " Press Enter to continue...";
            std::cin.ignore();
            return;
        }

        std::cout << "\n \033[31m[x] " << result.errorMessage << "\033[0m\n";
        std::cout << " Press Enter to try again...";
        std::cin.ignore();
    }
}