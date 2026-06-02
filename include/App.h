#pragma once
#include <memory>
#include <random>
#include <string>

#include "Events.h"
#include "Player.h"

// ============================================================
//  GameState
//  Everything the app needs to run, in one place.
//  No printing, no cin — pure data + logic.
// ============================================================
struct GameState {
    std::unique_ptr<Player> you;
    std::unique_ptr<Player> partner;

    // Market offers (3 pigeons, refreshed every 24h)
    std::vector<Pigeon> marketPigeons;
    std::time_t lastMarketRefresh = 0;

    // Locations loaded from DB, sorted by last use
    std::vector<LocationData> locations;

    bool isReady() const { return you != nullptr && partner != nullptr; }
};

// ============================================================
//  App
//  Owns GameState and the RNG.
//  Registers handlers for Ui* events.
//  Emits App* events back to UI.
//  Never touches std::cout or std::cin.
// ============================================================
class App {
   public:
    App();

    // Register all Ui* event handlers on the bus.
    // Call once before UI::run().
    void registerHandlers();

   private:
    GameState _state;
    std::mt19937 _rng;

    // ----------------------------------------------------------
    //  Handlers — one per Ui* event
    //  Each one does work then emits an App* result event.
    // ----------------------------------------------------------

    void onNavigate(const UiNavigateEvent& e);

    void onLogin(const UiLoginEvent& e);
    void onSignup(const UiSignupEvent& e);
    void onLogout(const UiLogoutEvent& e);

    void onSendMessage(const UiSendMessageEvent& e);
    void onBuyPigeon(const UiBuyPigeonEvent& e);
    void onRenamePigeon(const UiRenamePigeonEvent& e);

    void onAddLocation(const UiAddLocationEvent& e);
    void onSelectLocation(const UiSelectLocationEvent& e);

    void onRefreshInbox(const UiRefreshInboxEvent& e);
    void onReadMessage(const UiReadMessageEvent& e);

    // ----------------------------------------------------------
    //  Internal helpers
    // ----------------------------------------------------------

    // Convert internal Pigeon -> flat PigeonData for events
    PigeonData toPigeonData(const Pigeon& p) const;

    // Convert internal Message -> flat MessageData for events
    MessageData toMessageData(const Message& m) const;

    // Check all in-transit outbox messages and free pigeons
    // whose flight time has elapsed. Called on every navigation.
    void tickFlights();

    // Refresh market if >24h since last refresh
    void tickMarket();

    // Build and emit AppShowMainMenuEvent from current state
    void emitMainMenu();

    // Format a flight ETA as a human-readable string
    std::string formatEta(double minutes) const;
    std::string formatTimeRemaining(std::time_t arrivesAt) const;
};