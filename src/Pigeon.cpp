#include "Pigeon.h"

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>

Pigeon::Pigeon(const std::string& n, PigeonStats s, PigeonRarity r,
               const std::string& desc)
    : ownerName(""),
      name(n),
      stats(s),
      rarity(r),
      isAvailable(true),
      messagesSent(0),
      description(desc) {}

void Pigeon::changeOwner(const std::string& newOwnerName) {
    ownerName = newOwnerName;
}

std::string Pigeon::rarityName() const {
    switch (rarity) {
        case PigeonRarity::Common:
            return "Common";
        case PigeonRarity::Uncommon:
            return "Uncommon";
        case PigeonRarity::Rare:
            return "Rare";
        case PigeonRarity::Epic:
            return "Epic";
        case PigeonRarity::Legendary:
            return "Legendary";
    }
    return "Unknown";
}

std::string Pigeon::rarityColor() const {
    switch (rarity) {
        case PigeonRarity::Common:
            return "\033[37m";
        case PigeonRarity::Uncommon:
            return "\033[32m";
        case PigeonRarity::Rare:
            return "\033[34m";
        case PigeonRarity::Epic:
            return "\033[35m";
        case PigeonRarity::Legendary:
            return "\033[33m";
    }
    return "\033[0m";
}

static std::string statBar(int value, int width = 20) {
    int filled = (value * width) / 99;
    std::string bar = "[";
    for (int i = 0; i < width; ++i) bar += (i < filled) ? "#" : ".";
    bar += "]";
    return bar;
}

void Pigeon::printCard() const {
    const std::string reset = "\033[0m";
    const std::string bold = "\033[1m";
    const std::string col = rarityColor();

    std::cout << "  +----------------------------------------+\n";
    std::cout << "  | " << bold << col << std::left << std::setw(34) << name
              << reset << "  |\n";
    std::cout << "  | " << col << std::setw(36) << ("[" + rarityName() + "]")
              << reset << "|\n";
    std::cout << "  | " << "\033[90m" << std::setw(36) << description << reset
              << "|\n";
    std::cout << "  |----------------------------------------|\n";
    std::cout << "  | SPD " << statBar(stats.speed, 16) << " " << std::setw(2)
              << stats.speed << "  |\n";
    std::cout << "  | ACC " << statBar(stats.accuracy, 16) << " "
              << std::setw(2) << stats.accuracy << "  |\n";
    std::cout << "  | STA " << statBar(stats.stamina, 16) << " " << std::setw(2)
              << stats.stamina << "  |\n";
    std::cout << "  | STL " << statBar(stats.stealth, 16) << " " << std::setw(2)
              << stats.stealth << "  |\n";
    std::cout << "  | LOY " << statBar(stats.loyalty, 16) << " " << std::setw(2)
              << stats.loyalty << "  |\n";
    std::cout << "  |----------------------------------------|\n";
    std::cout << "  | OVR: " << bold << col << stats.overall() << reset
              << "   Flights: " << messagesSent << "   "
              << (isAvailable ? "\033[32m[Available]\033[0m"
                              : "\033[31m[In transit]\033[0m")
              << "     |\n";
    std::cout << "  +----------------------------------------+\n";
}

double Pigeon::estimateFlightTime(double distanceKm) const {
    double speedKmh = 40.0 + (stats.speed / 99.0) * 70.0;
    double fatigueFactor = 1.0;
    if (distanceKm > 200.0) {
        double staminaBonus = stats.stamina / 99.0;
        fatigueFactor =
            1.0 + (distanceKm - 200.0) / 1000.0 * (1.0 - staminaBonus * 0.5);
    }
    return (distanceKm / speedKmh) * 60.0 * fatigueFactor;
}

bool Pigeon::deliverMessage(double distanceKm, std::mt19937& rng) const {
    std::uniform_real_distribution<double> dist(0.0, 100.0);
    double lostChance = 5.0 * (1.0 - stats.loyalty / 99.0);
    if (distanceKm > 500.0) lostChance += (distanceKm - 500.0) / 200.0;
    if (dist(rng) < lostChance) return false;
    return true;
}

static const std::array<std::string, 90> pigeonNames = {
    "Comet",   "Nimbus",   "Hermes",   "Zephyr",  "Atlas",     "Echo",
    "Bolt",    "Gale",     "Dusk",     "Flint",   "Cirrus",    "Talon",
    "Sage",    "Frost",    "Ember",    "Pebble",  "Cinder",    "Slate",
    "Storm",   "Quill",    "Arrow",    "Streak",  "Glide",     "Wisp",
    "Pip",     "Feather",  "Cobalt",   "Drifter", "Patch",     "Gravel",

    "Shadow",  "Blaze",    "Raven",    "Sky",     "Mistral",   "Rocket",
    "Dusty",   "Copper",   "Silver",   "Goldie",  "Marble",    "Jasper",
    "Onyx",    "Topaz",    "Ruby",     "Pearl",   "Scout",     "Ranger",
    "Hunter",  "Falcon",   "Breeze",   "Thunder", "Lightning", "Cloud",
    "Rain",    "Mist",     "Snow",     "Aurora",  "Nova",      "Orbit",
    "Astro",   "Meteor",   "Cosmo",    "Luna",    "Sol",       "Phoenix",
    "Bandit",  "Maverick", "Vortex",   "Summit",  "Harbor",    "Canyon",
    "River",   "Forest",   "Pine",     "Oak",     "Birch",     "Maple",
    "Willow",  "Aspen",    "Traveler", "Nomad",   "Courier",   "Messenger",
    "Wingtip", "Skipper",  "Swoop",    "Jet",     "Horizon",   "Trail"};

static const std::array<std::string, 40> pigeonDescs = {
    "Swift and reliable",
    "A tireless traveler",
    "Knows every shortcut",
    "Eyes like a hawk",
    "Never loses the trail",
    "Old but wise",
    "Born in a thunderstorm",
    "Fastest of the flock",
    "Quiet and precise",
    "Legend of the loft",

    "Can cross mountains without rest",
    "Always arrives ahead of schedule",
    "A veteran of countless journeys",
    "Known across distant rooftops",
    "Fearless in rough weather",
    "Carries messages with pride",
    "Remarkably calm under pressure",
    "Masters even the longest routes",
    "Quick to learn new paths",
    "Trusted by every handler",

    "Rarely seen resting",
    "Famous for impossible deliveries",
    "Navigates by instinct alone",
    "Small but exceptionally brave",
    "A favorite among the flock",
    "Moves like the wind",
    "Never misses a landmark",
    "Has crossed seas and deserts",
    "Surprisingly strong for its size",
    "Can spot home from miles away",

    "Elegant in flight",
    "The first to leave and last to land",
    "Unafraid of storms",
    "Always curious about new routes",
    "A born explorer",
    "Flies with unmatched grace",
    "Renowned for endurance",
    "Keeps perfect bearings",
    "The pride of the loft",
    "A true sky legend"};

Pigeon generateRandomPigeon(std::mt19937& rng) {
    std::uniform_int_distribution<int> statRoll(30, 85);
    std::uniform_int_distribution<int> namePick(0, (int)pigeonNames.size() - 1);
    std::uniform_int_distribution<int> descPick(0, (int)pigeonDescs.size() - 1);
    std::uniform_int_distribution<int> rarityRoll(0, 99);

    PigeonRarity rarity;
    int r = rarityRoll(rng);
    if (r < 50)
        rarity = PigeonRarity::Common;
    else if (r < 75)
        rarity = PigeonRarity::Uncommon;
    else if (r < 90)
        rarity = PigeonRarity::Rare;
    else if (r < 97)
        rarity = PigeonRarity::Epic;
    else
        rarity = PigeonRarity::Legendary;

    int boost = (int)rarity * 5;
    auto clamp = [](int v) { return std::min(99, v); };

    PigeonStats s{clamp(statRoll(rng) + boost), clamp(statRoll(rng) + boost),
                  clamp(statRoll(rng) + boost), clamp(statRoll(rng) + boost),
                  clamp(statRoll(rng) + boost)};

    return Pigeon(pigeonNames[namePick(rng)], s, rarity,
                  pigeonDescs[descPick(rng)]);
}

Pigeon generateStarterPigeon(const std::string& name) {
    PigeonStats s{55, 60, 50, 45, 65};
    return Pigeon(name, s, PigeonRarity::Common,
                  "Your first and faithful pigeon");
}
