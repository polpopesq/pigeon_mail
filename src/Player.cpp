#include "Player.h"

#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

static const double PI = 3.14159265358979323846;

double haversineDistance(const Location& a, const Location& b) {
    double R = 6371.0;
    double dLat = (b.lat - a.lat) * PI / 180.0;
    double dLon = (b.lon - a.lon) * PI / 180.0;
    double lat1 = a.lat * PI / 180.0;
    double lat2 = b.lat * PI / 180.0;
    double x = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(lat1) * std::cos(lat2) * std::sin(dLon / 2) *
                   std::sin(dLon / 2);
    double c = 2 * std::atan2(std::sqrt(x), std::sqrt(1 - x));
    return R * c;
}

Player::Player(const std::string& n, const Location& loc)
    : name(n), location(loc), messageIdCounter(1) {}

void Player::addPigeon(Pigeon p) {
    p.changeOwner(name);
    pigeons.push_back(std::move(p));
}

void Player::removePigeon(int index) {
    if (index >= 0 && index < (int)pigeons.size())
        pigeons.erase(pigeons.begin() + index);
}

Message Player::sendMessage(const std::string& recipient,
                            const std::string& content, int pigeonIndex,
                            double distanceKm, std::mt19937& rng) {
    Pigeon& p = pigeons[pigeonIndex];
    p.isAvailable = false;
    p.messagesSent++;

    double flightMinutes = p.estimateFlightTime(distanceKm);

    std::uniform_real_distribution<double> jitter(0.8, 1.2);
    flightMinutes *= jitter(rng);

    std::time_t now = std::time(nullptr);
    Message msg;
    msg.id = messageIdCounter++;
    msg.sender = name;
    msg.recipient = recipient;
    msg.content = content;
    msg.pigeonName = p.name;
    msg.distanceKm = distanceKm;
    msg.status = MessageStatus::InTransit;
    msg.sentAt = now;
    msg.arrivesAt = now + (std::time_t)(flightMinutes * 60);

    outbox.push_back(msg);
    return msg;
}

void Player::checkInbox() {
    std::time_t now = std::time(nullptr);
    for (auto& msg : inbox) {
        if (msg.status == MessageStatus::InTransit && now >= msg.arrivesAt) {
            msg.status = MessageStatus::Delivered;
        }
    }
}

void Player::printPigeons() const {
    if (pigeons.empty()) {
        std::cout << "  You have no pigeons!\n";
        return;
    }
    for (int i = 0; i < (int)pigeons.size(); ++i) {
        std::cout << "\n  [" << (i + 1) << "] " << pigeons[i].name << " ("
                  << pigeons[i].rarityColor() << pigeons[i].rarityName()
                  << "\033[0m"
                  << ")  OVR: \033[1m" << pigeons[i].stats.overall()
                  << "\033[0m"
                  << "  "
                  << (pigeons[i].isAvailable ? "\033[32m[Available]\033[0m"
                                             : "\033[31m[In transit]\033[0m")
                  << "\n";
    }
}

void Player::printInbox() const {
    if (inbox.empty()) {
        std::cout << "  No messages in inbox.\n";
        return;
    }
    std::cout << "  " << std::left << std::setw(4) << "#" << std::setw(14)
              << "From" << std::setw(14) << "Pigeon" << std::setw(20)
              << "Status"
              << "Preview\n";
    std::cout << "  " << std::string(70, '-') << "\n";
    for (const auto& m : inbox) {
        std::string preview =
            m.content.substr(0, 25) + (m.content.size() > 25 ? "..." : "");
        std::cout << "  " << std::setw(4) << m.id << std::setw(14) << m.sender
                  << std::setw(14) << m.pigeonName << std::setw(20)
                  << m.statusString() << preview << "\n";
    }
}

void Player::printOutbox() const {
    if (outbox.empty()) {
        std::cout << "  No messages sent yet.\n";
        return;
    }
    std::cout << "  " << std::left << std::setw(4) << "#" << std::setw(14)
              << "To" << std::setw(14) << "Pigeon" << std::setw(20) << "Status"
              << std::setw(16) << "ETA"
              << "Preview\n";
    std::cout << "  " << std::string(80, '-') << "\n";
    for (const auto& m : outbox) {
        std::string preview =
            m.content.substr(0, 20) + (m.content.size() > 20 ? "..." : "");
        std::cout << "  " << std::setw(4) << m.id << std::setw(14)
                  << m.recipient << std::setw(14) << m.pigeonName
                  << std::setw(20) << m.statusString() << std::setw(16)
                  << (m.status == MessageStatus::InTransit
                          ? m.timeRemainingString()
                          : "-")
                  << preview << "\n";
    }
}
