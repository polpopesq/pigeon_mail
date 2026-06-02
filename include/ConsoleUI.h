#pragma once
#include <string>

#include "Events.h"
#include "UI.h"

// ============================================================
//  ConsoleUI
//  Handles all terminal input/output.
//  Registers handlers for App* events (rendering).
//  Emits Ui* events in response to user input.
//  Never touches GameState or App directly.
// ============================================================
class ConsoleUI : public UI {
   public:
    ConsoleUI() = default;
    ~ConsoleUI() = default;

    void registerHandlers() override;
    void run() override;

   private:
    // ----------------------------------------------------------
    //  App* event handlers — these render data onto the terminal
    // ----------------------------------------------------------
    void onShowMainMenu(const AppShowMainMenuEvent& e);
    void onShowRoster(const AppShowPigeonRosterEvent& e);
    void onShowPigeonDetail(const AppShowPigeonDetailEvent& e);
    void onShowInbox(const AppShowInboxEvent& e);
    void onShowOutbox(const AppShowOutboxEvent& e);
    void onShowMessage(const AppShowMessageEvent& e);
    void onShowMarket(const AppShowMarketEvent& e);
    void onShowLocations(const AppShowLocationsEvent& e);
    void onMessageSent(const AppMessageSentEvent& e);
    void onLoginSuccess(const AppLoginSuccessEvent& e);
    void onLoginFailed(const AppLoginFailedEvent& e);
    void onSignupSuccess(const AppSignupSuccessEvent& e);
    void onSignupFailed(const AppSignupFailedEvent& e);
    void onError(const AppErrorEvent& e);
    void onSuccess(const AppSuccessEvent& e);

    // ----------------------------------------------------------
    //  Input helpers — only ConsoleUI uses these
    // ----------------------------------------------------------
    void printHeader(const std::string& title) const;
    void printSeparator() const;
    void waitEnter() const;
    std::string statBar(int value, int width = 18) const;
    std::string rarityColor(const std::string& rarity) const;

    int getInt(const std::string& prompt, int min, int max) const;
    std::string getString(const std::string& prompt) const;

    // ----------------------------------------------------------
    //  State the UI needs to track between renders
    //  (only display-level state — nothing game-related)
    // ----------------------------------------------------------

    // Last roster shown — needed so "pick pigeon [1-N]" works
    // after AppShowPigeonRosterEvent arrives
    std::vector<PigeonData> _lastRoster;
    std::vector<MessageData> _lastInbox;
    std::vector<MessageData> _lastOutbox;
    std::vector<PigeonData> _lastMarket;
    std::vector<LocationData> _lastLocations;

    // Tracks which screen we're on so run() knows what input to collect
    Screen _currentScreen = Screen::Login;

    bool _running = true;
};