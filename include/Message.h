#pragma once
#include <string>
#include <chrono>
#include <ctime>

enum class MessageStatus {
    InTransit,
    Delivered,
    Lost,
    Corrupted  // arrived but unreadable (low accuracy)
};

struct Message {
    int id;
    std::string sender;
    std::string recipient;
    std::string content;
    std::string pigeonName;
    double distanceKm;
    MessageStatus status;

    std::time_t sentAt;
    std::time_t arrivesAt;   // estimated arrival

    std::string statusString() const;
    std::string timeRemainingString() const;
    bool hasArrived() const;
};
