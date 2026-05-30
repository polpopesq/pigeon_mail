#include "PigeonMailApp.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <random>
#include <algorithm>

PigeonMailApp::PigeonMailApp() {
    std::random_device rd;
    rng = std::mt19937(rd());
}

void PigeonMailApp::printHeader(const std::string& title) const {
    std::cout << "\033[2J\033[H";
    std::cout << "\033[33m";
    std::cout << " +==================================================+\n";
    std::cout << " |        >>  P I G E O N   M A I L  <<            |\n";
    std::cout << " +==================================================+\033[0m\n";
    if (!title.empty())
        std::cout << "\n \033[1m" << title << "\033[0m\n";
    printSeparator();
}

void PigeonMailApp::printSeparator() const {
    std::cout << " \033[90m" << std::string(52, '-') << "\033[0m\n";
}

void PigeonMailApp::waitEnter() const {
    std::cout << "\n \033[90mPress Enter to continue...\033[0m ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int PigeonMailApp::getIntInput(const std::string& prompt, int min, int max) const {
    int val;
    while (true) {
        std::cout << " " << prompt << " (" << min << "-" << max << "): ";
        if (std::cin >> val && val >= min && val <= max) {
            std::cin.ignore();
            return val;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << " \033[31mInvalid input, try again.\033[0m\n";
    }
}

std::string PigeonMailApp::getStringInput(const std::string& prompt) const {
    std::string s;
    std::cout << " " << prompt << ": ";
    std::getline(std::cin, s);
    return s;
}

void PigeonMailApp::run() {
    printHeader("");
    std::cout << "\n \033[1mWelcome to Pigeon Mail!\033[0m\n";
    std::cout << " The most romantic and unreliable messaging service.\n\n";

    std::string yourName = getStringInput("Your name");
    std::string partnerName = getStringInput("Your partner's name");

    std::vector<std::pair<std::string, Location>> cities = {
        {"Bucharest, Romania",  {"Bucharest",  44.4268, 26.1025}},
        {"London, UK",          {"London",     51.5074, -0.1278}},
        {"Paris, France",       {"Paris",      48.8566,  2.3522}},
        {"Rome, Italy",         {"Rome",       41.9028, 12.4964}},
        {"Barcelona, Spain",    {"Barcelona",  41.3851,  2.1734}},
        {"Vienna, Austria",     {"Vienna",     48.2082, 16.3738}},
        {"Prague, Czech Rep.",  {"Prague",     50.0755, 14.4378}},
        {"Istanbul, Turkey",    {"Istanbul",   41.0082, 28.9784}},
        {"Cairo, Egypt",        {"Cairo",      30.0444, 31.2357}},
        {"New York, USA",       {"New York",   40.7128,-74.0060}},
    };

    std::cout << "\n Your location:\n";
    for (int i = 0; i < (int)cities.size(); ++i)
        std::cout << "  [" << (i+1) << "] " << cities[i].first << "\n";
    std::cout << "  [" << cities.size()+1 << "] Custom coordinates\n";
    int yourCity = getIntInput("Choose", 1, (int)cities.size()+1) - 1;

    Location yourLoc;
    if (yourCity == (int)cities.size()) {
        yourLoc.name = yourName + "'s location";
        std::cout << " Latitude: "; std::cin >> yourLoc.lat;
        std::cout << " Longitude: "; std::cin >> yourLoc.lon;
        std::cin.ignore();
    } else {
        yourLoc = cities[yourCity].second;
    }

    std::cout << "\n " << partnerName << "'s location:\n";
    for (int i = 0; i < (int)cities.size(); ++i)
        std::cout << "  [" << (i+1) << "] " << cities[i].first << "\n";
    std::cout << "  [" << cities.size()+1 << "] Custom coordinates\n";
    int partnerCity = getIntInput("Choose", 1, (int)cities.size()+1) - 1;

    Location partnerLoc;
    if (partnerCity == (int)cities.size()) {
        partnerLoc.name = partnerName + "'s location";
        std::cout << " Latitude: "; std::cin >> partnerLoc.lat;
        std::cout << " Longitude: "; std::cin >> partnerLoc.lon;
        std::cin.ignore();
    } else {
        partnerLoc = cities[partnerCity].second;
    }

    you     = std::make_unique<Player>(yourName,    yourLoc);
    partner = std::make_unique<Player>(partnerName, partnerLoc);

    double distance = haversineDistance(yourLoc, partnerLoc);

    you->addPigeon(generateStarterPigeon("Pip"));
    you->addPigeon(generateRandomPigeon(rng));
    you->addPigeon(generateRandomPigeon(rng));

    {
        Message welcome;
        welcome.id = 999;
        welcome.sender = partnerName;
        welcome.recipient = yourName;
        welcome.content = "My dearest, I've just set up my pigeon loft. Write to me soon! ~" + partnerName;
        welcome.pigeonName = "Feather";
        welcome.distanceKm = distance;
        welcome.status = MessageStatus::Delivered;
        welcome.sentAt = std::time(nullptr) - 3600;
        welcome.arrivesAt = std::time(nullptr) - 10;
        you->inbox.push_back(welcome);
    }

    std::cout << "\n \033[32m[+] Setup complete!\033[0m Distance between you two: \033[1m"
              << std::fixed << std::setprecision(0) << distance << " km\033[0m\n";
    waitEnter();

    mainMenu();
}

void PigeonMailApp::mainMenu() {
    while (true) {
        you->checkInbox();
        printHeader("Main Menu");

        long unread = std::count_if(you->inbox.begin(), you->inbox.end(),
            [](const Message& m){ return m.status == MessageStatus::Delivered; });
        long transit = std::count_if(you->outbox.begin(), you->outbox.end(),
            [](const Message& m){ return m.status == MessageStatus::InTransit; });

        std::cout << "\n  Playing as: \033[1m" << you->name << "\033[0m"
                  << "  |  Location: " << you->location.name
                  << "  |  Pigeons: " << you->pigeons.size() << "\n\n";

        std::cout << "  [1]  \033[33m(~) My Pigeon Roster\033[0m\n";
        std::cout << "  [2]  \033[36m(>) Send a Message\033[0m\n";
        std::cout << "  [3]  \033[32m(v) Inbox\033[0m";
        if (unread > 0) std::cout << "  \033[32m(" << unread << " delivered)\033[0m";
        std::cout << "\n";
        std::cout << "  [4]  \033[34m(^) Outbox\033[0m";
        if (transit > 0) std::cout << "  \033[33m(" << transit << " in transit)\033[0m";
        std::cout << "\n";
        std::cout << "  [5]  \033[35m($) Pigeon Market\033[0m\n";
        std::cout << "  [0]  Exit\n\n";

        int choice = getIntInput("Choose", 0, 5);
        switch (choice) {
            case 1: pigeonRosterMenu(); break;
            case 2: sendMessageMenu();  break;
            case 3: inboxMenu();        break;
            case 4: outboxMenu();       break;
            case 5: buyPigeonMenu();    break;
            case 0:
                std::cout << "\n \033[33mFly free, little pigeons.\033[0m\n\n";
                return;
        }
    }
}

void PigeonMailApp::pigeonRosterMenu() {
    while (true) {
        printHeader("My Pigeon Roster");
        you->printPigeons();
        std::cout << "\n  [1-" << you->pigeons.size() << "]  View pigeon details\n";
        std::cout << "  [0]  Back\n\n";
        int choice = getIntInput("Choose", 0, (int)you->pigeons.size());
        if (choice == 0) return;
        pigeonDetailMenu(choice - 1);
    }
}

void PigeonMailApp::pigeonDetailMenu(int index) {
    printHeader("Pigeon Profile");
    you->pigeons[index].printCard();
    std::cout << "\n  [1]  Rename this pigeon\n";
    std::cout << "  [0]  Back\n\n";
    int choice = getIntInput("Choose", 0, 1);
    if (choice == 1) renamePigeonMenu(index);
}

void PigeonMailApp::renamePigeonMenu(int index) {
    printHeader("Rename Pigeon");
    std::cout << "  Current name: \033[1m" << you->pigeons[index].name << "\033[0m\n\n";
    std::string newName = getStringInput("New name");
    if (!newName.empty()) {
        you->pigeons[index].name = newName;
        std::cout << "  \033[32mPigeon renamed to '" << newName << "'!\033[0m\n";
        waitEnter();
    }
}

void PigeonMailApp::sendMessageMenu() {
    printHeader("Send a Message");

    double distance = haversineDistance(you->location, partner->location);
    std::cout << "  Sending to: \033[1m" << partner->name << "\033[0m"
              << "  (" << partner->location.name << ")\n";
    std::cout << "  Distance: \033[1m" << std::fixed << std::setprecision(0) << distance << " km\033[0m\n\n";

    std::vector<int> available;
    for (int i = 0; i < (int)you->pigeons.size(); ++i)
        if (you->pigeons[i].isAvailable) available.push_back(i);

    if (available.empty()) {
        std::cout << "  \033[31mAll your pigeons are currently in transit!\033[0m\n";
        waitEnter();
        return;
    }

    std::cout << "  Choose your pigeon:\n";
    for (int idx : available) {
        const auto& p = you->pigeons[idx];
        double eta = p.estimateFlightTime(distance);
        int hours = (int)(eta / 60);
        int mins  = (int)(eta) % 60;
        std::cout << "  [" << (idx+1) << "] " << p.rarityColor() << p.name << "\033[0m"
                  << "  OVR: " << p.stats.overall()
                  << "  ETA: ~" << hours << "h " << mins << "m"
                  << "  ACC: " << p.stats.accuracy
                  << "\n";
    }
    std::cout << "  [0]  Cancel\n\n";

    int pigeonChoice = getIntInput("Choose pigeon", 0, (int)you->pigeons.size());
    if (pigeonChoice == 0) return;
    pigeonChoice--;

    if (!you->pigeons[pigeonChoice].isAvailable) {
        std::cout << "  \033[31mThat pigeon is already in transit!\033[0m\n";
        waitEnter();
        return;
    }

    std::cout << "\n  Write your message:\n  > ";
    std::string content;
    std::getline(std::cin, content);
    if (content.empty()) return;

    Message sent = you->sendMessage(partner->name, content, pigeonChoice, distance, rng);

    Message received = sent;
    bool arrived = you->pigeons[pigeonChoice].deliverMessage(distance, rng);
    std::uniform_int_distribution<int> accRoll(1, 99);
    bool readable = accRoll(rng) <= you->pigeons[pigeonChoice].stats.accuracy;

    if (!arrived) {
        received.status = MessageStatus::Lost;
    } else if (!readable) {
        received.status = MessageStatus::Corrupted;
        received.content = "~~ message garbled in transit ~~";
    }
    partner->inbox.push_back(received);

    double eta = you->pigeons[pigeonChoice].estimateFlightTime(distance);
    int hours = (int)(eta / 60);
    int mins  = (int)eta % 60;

    std::cout << "\n  \033[32m[+] Message sent with \033[1m" << you->pigeons[pigeonChoice].name
              << "\033[0m\033[32m!\033[0m\n";
    std::cout << "  Estimated arrival: " << hours << " hours " << mins << " minutes\n";
    waitEnter();
}

void PigeonMailApp::inboxMenu() {
    you->checkInbox();
    printHeader("Inbox");
    you->printInbox();
    if (!you->inbox.empty()) {
        std::cout << "\n  [1-" << you->inbox.size() << "]  Read message  [0] Back\n\n";
        int choice = getIntInput("Choose", 0, (int)you->inbox.size());
        if (choice > 0) {
            const auto& m = you->inbox[choice - 1];
            printHeader("Message #" + std::to_string(m.id));
            std::cout << "  From:    \033[1m" << m.sender    << "\033[0m\n";
            std::cout << "  Via:     " << m.pigeonName << " (" << (int)m.distanceKm << " km)\n";
            std::cout << "  Status:  " << m.statusString() << "\n";
            std::cout << "\n  \033[33m" << std::string(50, '-') << "\033[0m\n";
            std::cout << "  " << m.content << "\n";
            std::cout << "  \033[33m" << std::string(50, '-') << "\033[0m\n";
            waitEnter();
        }
    } else {
        waitEnter();
    }
}

void PigeonMailApp::outboxMenu() {
    std::time_t now = std::time(nullptr);
    for (auto& msg : you->outbox) {
        if (msg.status == MessageStatus::InTransit && now >= msg.arrivesAt) {
            msg.status = MessageStatus::Delivered;
            for (auto& p : you->pigeons)
                if (p.name == msg.pigeonName && !p.isAvailable)
                    p.isAvailable = true;
        }
    }
    printHeader("Outbox");
    you->printOutbox();
    waitEnter();
}

void PigeonMailApp::buyPigeonMenu() {
    printHeader("Pigeon Market");
    std::cout << "  Three pigeons are available today. Choose wisely!\n\n";

    Pigeon options[3] = {
        generateRandomPigeon(rng),
        generateRandomPigeon(rng),
        generateRandomPigeon(rng)
    };

    for (int i = 0; i < 3; ++i) {
        std::cout << "\n  Option " << (i+1) << ":\n";
        options[i].printCard();
    }

    std::cout << "\n  [1-3]  Adopt pigeon  [0] Back\n\n";
    int choice = getIntInput("Choose", 0, 3);
    if (choice > 0) {
        you->addPigeon(options[choice - 1]);
        std::cout << "  \033[32m[+] " << options[choice-1].name << " has joined your loft!\033[0m\n";
        waitEnter();
    }
}
