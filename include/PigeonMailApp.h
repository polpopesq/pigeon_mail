#pragma once
#include <memory>
#include <random>
#include "Player.h"

class PigeonMailApp {
public:
    PigeonMailApp();
    void run();

private:
    std::unique_ptr<Player> you;
    std::unique_ptr<Player> partner;
    std::mt19937 rng;

    // Menu screens
    void mainMenu();
    void pigeonRosterMenu();
    void sendMessageMenu();
    void inboxMenu();
    void outboxMenu();
    void pigeonDetailMenu(int index);
    void renamePigeonMenu(int index);
    void buyPigeonMenu();

    // UI helpers
    void printHeader(const std::string& title) const;
    void waitEnter() const;
    int getIntInput(const std::string& prompt, int min, int max) const;
    std::string getStringInput(const std::string& prompt) const;
    void printSeparator() const;
};
