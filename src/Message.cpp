#include "Message.h"
#include <sstream>
#include <ctime>

std::string Message::statusString() const {
    switch(status) {
        case MessageStatus::InTransit:  return "\033[33m[~] In transit\033[0m";
        case MessageStatus::Delivered:  return "\033[32m[+] Delivered\033[0m";
        case MessageStatus::Lost:       return "\033[31m[x] Lost\033[0m";
        case MessageStatus::Corrupted:  return "\033[35m[?] Corrupted\033[0m";
    }
    return "Unknown";
}

bool Message::hasArrived() const {
    return std::time(nullptr) >= arrivesAt;
}

std::string Message::timeRemainingString() const {
    if (status != MessageStatus::InTransit) return "";
    std::time_t now = std::time(nullptr);
    if (now >= arrivesAt) return "arriving now...";

    long secs = (long)(arrivesAt - now);
    long mins  = secs / 60;
    long hours = mins / 60;
    long days  = hours / 24;

    std::ostringstream oss;
    if (days > 0)       oss << days  << "d " << (hours % 24) << "h";
    else if (hours > 0) oss << hours << "h " << (mins  % 60) << "m";
    else if (mins > 0)  oss << mins  << "m " << (secs  % 60) << "s";
    else                oss << secs  << "s";
    return oss.str();
}
