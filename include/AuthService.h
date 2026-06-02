#pragma once
#include <string>

#include "Session.h"

struct AuthResult {
    bool success;
    std::string errorMessage;
};

class AuthService {
   public:
    static AuthService& instance() {
        static AuthService inst;
        return inst;
    }

    AuthService(const AuthService&) = delete;
    AuthService& operator=(const AuthService&) = delete;

    // Show login/signup prompt and loop until success or user quits.
    // Call once at app startup if tryRestoreSession() returns false.
    void runLoginFlow();

    // Authenticate with username + password.
    // Constructs fake email internally; populates and persists session on
    // success.
    AuthResult login(const std::string& username, const std::string& password);

    // Register new account, then automatically log in.
    AuthResult signup(const std::string& username, const std::string& password);

    // Read session.json from disk. If the token is expired, refresh it
    // silently. Returns true if the session is now valid (user can skip login
    // screen).
    bool tryRestoreSession();

    // Clear session from memory and delete session.json.
    void logout();

    const Session& session() const { return _session; }
    bool isLoggedIn() const { return _session.isValid(); }

   private:
    AuthService() = default;

    // Called internally after a successful login/signup Supabase response.
    // Fetches the matching row from public.users to get username + partner_id.
    bool fetchUserProfile();

    // Silently exchange the refresh token for a new access token.
    bool doRefreshToken();

    // Serialize _session to session.json next to the executable.
    void persistSession() const;

    // Deserialize session.json into _session. Returns false if file
    // missing/corrupt.
    bool loadPersistedSession();

    // Delete session.json from disk.
    void deletePersistedSession() const;

    Session _session;
};

// Convenience free function so callers can write Auth().login(...) etc.
inline AuthService& Auth() { return AuthService::instance(); }