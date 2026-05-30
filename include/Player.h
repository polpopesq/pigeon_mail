#pragma once
#include <string>
#include <vector>
#include "Pigeon.h"
#include "Message.h"

struct Location {
    std::string name;
    double lat;
    double lon;
};

double haversineDistance(const Location& a, const Location& b);

class Player {
public:
    std::string name;
    Location location;
    std::vector<Pigeon> pigeons;
    std::vector<Message> inbox;
    std::vector<Message> outbox;
    int messageIdCounter;

    Player(const std::string& name, const Location& loc);

    void addPigeon(Pigeon p);
    void removePigeon(int index);

    // Send a message; returns the created Message
    Message sendMessage(const std::string& recipient, const std::string& content,
                        int pigeonIndex, double distanceKm, std::mt19937& rng);

    void checkInbox();   // resolves InTransit messages that have arrived
    void printPigeons() const;
    void printInbox() const;
    void printOutbox() const;
};
