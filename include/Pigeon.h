#pragma once
#include <string>
#include <random>

struct PigeonStats {
    int speed;       // 1-99: affects flight time
    int accuracy;    // 1-99: chance message arrives intact
    int stamina;     // 1-99: reduces fatigue penalty on long routes
    int stealth;     // 1-99: chance of avoiding interception
    int loyalty;     // 1-99: reduces chance of getting lost

    int overall() const {
        return (speed + accuracy + stamina + stealth + loyalty) / 5;
    }
};

enum class PigeonRarity {
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary
};

class Pigeon {
public:
    std::string name;
    PigeonStats stats;
    PigeonRarity rarity;
    bool isAvailable;
    int messagesSent;
    std::string description;

    Pigeon(const std::string& name, PigeonStats stats, PigeonRarity rarity, const std::string& desc);

    std::string rarityName() const;
    std::string rarityColor() const;  // ANSI color code
    void printCard() const;

    // Returns flight time in minutes, given distance in km
    double estimateFlightTime(double distanceKm) const;

    // Returns true if message arrives intact
    bool deliverMessage(double distanceKm, std::mt19937& rng) const;
};

// Factory: generate a random pigeon
Pigeon generateRandomPigeon(std::mt19937& rng);

// Factory: generate a starter pigeon (decent stats, Common)
Pigeon generateStarterPigeon(const std::string& name);
