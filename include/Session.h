#pragma once
#include <ctime>
#include <string>

struct Session {
    std::string userId;
    std::string partnerId;
    std::string username;
    std::string accessToken;
    std::string refreshToken;
    std::time_t expiresAt = 0;

    bool isValid() const {
        return !accessToken.empty() && std::time(nullptr) < expiresAt;
    }
};