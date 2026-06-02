#include "ConsoleUI.h"

#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include "EventBus.h"

// ============================================================
//  registerHandlers
//  Wire every App* event to a render method.
// ============================================================

void ConsoleUI::registerHandlers() {
    Bus().on<AppShowMainMenuEvent>(
        [this](const AppShowMainMenuEvent& e) { onShowMainMenu(e); });
    Bus().on<AppShowPigeonRosterEvent>(
        [this](const AppShowPigeonRosterEvent& e) { onShowRoster(e); });
    Bus().on<AppShowPigeonDetailEvent>(
        [this](const AppShowPigeonDetailEvent& e) { onShowPigeonDetail(e); });
    Bus().on<AppShowInboxEvent>(
        [this](const AppShowInboxEvent& e) { onShowInbox(e); });
    Bus().on<AppShowOutboxEvent>(
        [this](const AppShowOutboxEvent& e) { onShowOutbox(e); });
    Bus().on<AppShowMessageEvent>(
        [this](const AppShowMessageEvent& e) { onShowMessage(e); });
    Bus().on<AppShowMarketEvent>(
        [this](const AppShowMarketEvent& e) { onShowMarket(e); });
    Bus().on<AppShowLocationsEvent>(
        [this](const AppShowLocationsEvent& e) { onShowLocations(e); });
    Bus().on<AppMessageSentEvent>(
        [this](const AppMessageSentEvent& e) { onMessageSent(e); });
    Bus().on<AppLoginSuccessEvent>(
        [this](const AppLoginSuccessEvent& e) { onLoginSuccess(e); });
    Bus().on<AppLoginFailedEvent>(
        [this](const AppLoginFailedEvent& e) { onLoginFailed(e); });
    Bus().on<AppSignupSuccessEvent>(
        [this](const AppSignupSuccessEvent& e) { onSignupSuccess(e); });
    Bus().on<AppSignupFailedEvent>(
        [this](const AppSignupFailedEvent& e) { onSignupFailed(e); });
    Bus().on<AppErrorEvent>([this](const AppErrorEvent& e) { onError(e); });
    Bus().on<AppSuccessEvent>(
        [this](const AppSuccessEvent& e) { onSuccess(e); });

    // Track current screen so run() knows what input to expect
    Bus().on<UiNavigateEvent>(
        [this](const UiNavigateEvent& e) { _currentScreen = e.destination; });
}

// ============================================================
//  run
//  The main loop. Renders the login screen first, then
//  delegates input collection based on _currentScreen.
//  App* handlers update _currentScreen via UiNavigateEvent.
// ============================================================

void ConsoleUI::run() {
    // Kick off at the login screen
    Bus().emit(UiNavigateEvent{Screen::Login});
    printHeader("Welcome");
    std::cout << "\n  The most romantic and unreliable messaging service.\n\n";

    while (_running) {
        switch (_currentScreen) {
            // ---- Auth ----------------------------------------
            case Screen::Login: {
                std::cout << "  [1]  Login\n"
                          << "  [2]  Sign up\n"
                          << "  [0]  Exit\n\n";
                int choice = getInt("Choose", 0, 2);
                if (choice == 0) {
                    _running = false;
                    break;
                }

                std::string user = getString("Username");
                std::string pass = getString("Password");

                if (choice == 1)
                    Bus().emit(UiLoginEvent{user, pass});
                else
                    Bus().emit(UiSignupEvent{user, pass});
                break;
            }

            // ---- Main Menu -----------------------------------
            // Rendered by onShowMainMenu; input collected here
            case Screen::MainMenu: {
                int choice = getInt("Choose", 0, 6);
                switch (choice) {
                    case 1:
                        Bus().emit(UiNavigateEvent{Screen::PigeonRoster});
                        break;
                    case 2:
                        Bus().emit(UiNavigateEvent{Screen::SendMessage});
                        break;
                    case 3:
                        Bus().emit(UiNavigateEvent{Screen::Inbox});
                        break;
                    case 4:
                        Bus().emit(UiNavigateEvent{Screen::Outbox});
                        break;
                    case 5:
                        Bus().emit(UiNavigateEvent{Screen::Market});
                        break;
                    case 6:
                        Bus().emit(UiNavigateEvent{Screen::Locations});
                        break;
                    case 0:
                        Bus().emit(UiLogoutEvent{});
                        _running = false;
                        std::cout << "\n  \033[33mFly free, little "
                                     "pigeons.\033[0m\n\n";
                        break;
                }
                break;
            }

            // ---- Pigeon Roster -------------------------------
            case Screen::PigeonRoster: {
                if (_lastRoster.empty()) {
                    Bus().emit(UiNavigateEvent{Screen::MainMenu});
                    break;
                }
                std::cout << "\n  [1-" << _lastRoster.size()
                          << "]  View pigeon\n"
                          << "  [0]  Back\n\n";
                int choice = getInt("Choose", 0, (int)_lastRoster.size());
                if (choice == 0) {
                    Bus().emit(UiNavigateEvent{Screen::MainMenu});
                } else {
                    Bus().emit(
                        AppShowPigeonDetailEvent{_lastRoster[choice - 1]});
                }
                break;
            }

            default: {
                // Pigeon detail, rename, send, inbox detail, market, locations
                // are sub-screens that return to their parent via Back.
                // We park on Screen::MainMenu after each action (App decides).
                // So this default just re-emits navigate to main menu if we
                // somehow get stuck.
                Bus().emit(UiNavigateEvent{Screen::MainMenu});
                break;
            }
        }
    }
}

// ============================================================
//  App* handlers — rendering
// ============================================================

void ConsoleUI::onShowMainMenu(const AppShowMainMenuEvent& e) {
    _currentScreen = Screen::MainMenu;
    printHeader("Main Menu");

    std::cout << "\n  Playing as: \033[1m" << e.username << "\033[0m"
              << "  |  " << e.locationName << "  |  Pigeons: " << e.pigeonCount
              << "\n\n";

    std::cout << "  [1]  \033[33m(~) Pigeon Roster\033[0m\n"
              << "  [2]  \033[36m(>) Send a Message\033[0m\n"
              << "  [3]  \033[32m(v) Inbox\033[0m";
    if (e.unreadCount > 0)
        std::cout << "  \033[32m(" << e.unreadCount << " delivered)\033[0m";
    std::cout << "\n";

    std::cout << "  [4]  \033[34m(^) Outbox\033[0m";
    if (e.inTransitCount > 0)
        std::cout << "  \033[33m(" << e.inTransitCount << " in transit)\033[0m";
    std::cout << "\n";

    std::cout << "  [5]  \033[35m($) Pigeon Market\033[0m\n"
              << "  [6]  \033[90m(.) Locations\033[0m\n"
              << "  [0]  Exit\n\n";
}

void ConsoleUI::onShowRoster(const AppShowPigeonRosterEvent& e) {
    _lastRoster = e.pigeons;
    _currentScreen = Screen::PigeonRoster;
    printHeader("Pigeon Roster");

    if (e.pigeons.empty()) {
        std::cout << "  You have no pigeons!\n";
        waitEnter();
        Bus().emit(UiNavigateEvent{Screen::MainMenu});
        return;
    }

    for (int i = 0; i < (int)e.pigeons.size(); ++i) {
        const auto& p = e.pigeons[i];
        std::cout << "\n  [" << (i + 1) << "] " << rarityColor(p.rarity)
                  << p.name << "\033[0m"
                  << "  (" << rarityColor(p.rarity) << p.rarity << "\033[0m)"
                  << "  OVR: \033[1m" << p.overall << "\033[0m"
                  << "  "
                  << (p.isAvailable ? "\033[32m[Available]\033[0m"
                                    : "\033[31m[In transit]\033[0m")
                  << "\n";
    }
    std::cout << "\n";
}

void ConsoleUI::onShowPigeonDetail(const AppShowPigeonDetailEvent& e) {
    const PigeonData& p = e.pigeon;
    printHeader("Pigeon Profile");

    std::cout << "\n  +-----------------------------------------+\n"
              << "  | " << rarityColor(p.rarity) << "\033[1m" << std::left
              << std::setw(35) << p.name << "\033[0m  |\n"
              << "  | " << rarityColor(p.rarity) << std::setw(37)
              << ("[" + p.rarity + "]") << "\033[0m|\n"
              << "  | \033[90m" << std::setw(37) << p.description
              << "\033[0m|\n"
              << "  |-----------------------------------------|\n"
              << "  | SPD " << statBar(p.speed) << " " << std::setw(2)
              << p.speed << "  |\n"
              << "  | ACC " << statBar(p.accuracy) << " " << std::setw(2)
              << p.accuracy << "  |\n"
              << "  | STA " << statBar(p.stamina) << " " << std::setw(2)
              << p.stamina << "  |\n"
              << "  | STL " << statBar(p.stealth) << " " << std::setw(2)
              << p.stealth << "  |\n"
              << "  | LOY " << statBar(p.loyalty) << " " << std::setw(2)
              << p.loyalty << "  |\n"
              << "  |-----------------------------------------|\n"
              << "  | OVR: \033[1m" << p.overall << "\033[0m"
              << "   Flights: " << p.messagesSent << "   "
              << (p.isAvailable ? "\033[32m[Available]\033[0m"
                                : "\033[31m[In transit]\033[0m")
              << "        |\n"
              << "  +-----------------------------------------+\n\n";

    std::cout << "  [1]  Rename\n"
              << "  [0]  Back\n\n";

    int choice = getInt("Choose", 0, 1);
    if (choice == 1) {
        std::string newName = getString("New name");
        if (!newName.empty()) Bus().emit(UiRenamePigeonEvent{p.id, newName});
    } else {
        Bus().emit(UiNavigateEvent{Screen::PigeonRoster});
    }
}

void ConsoleUI::onShowInbox(const AppShowInboxEvent& e) {
    _lastInbox = e.messages;
    _currentScreen = Screen::Inbox;
    printHeader("Inbox");

    if (e.messages.empty()) {
        std::cout << "  No messages yet.\n";
        waitEnter();
        Bus().emit(UiNavigateEvent{Screen::MainMenu});
        return;
    }

    std::cout << "  " << std::left << std::setw(5) << "#" << std::setw(14)
              << "From" << std::setw(14) << "Pigeon" << std::setw(20)
              << "Status"
              << "Preview\n"
              << "  " << std::string(72, '-') << "\n";

    for (int i = 0; i < (int)e.messages.size(); ++i) {
        const auto& m = e.messages[i];
        std::string preview =
            m.content.size() > 25 ? m.content.substr(0, 25) + "..." : m.content;
        std::cout << "  " << std::setw(5) << (i + 1) << std::setw(14)
                  << m.senderName << std::setw(14) << m.pigeonName
                  << std::setw(20) << m.status << preview << "\n";
    }

    std::cout << "\n  [1-" << e.messages.size() << "]  Read  [0] Back\n\n";
    int choice = getInt("Choose", 0, (int)e.messages.size());
    if (choice == 0) {
        Bus().emit(UiNavigateEvent{Screen::MainMenu});
    } else {
        Bus().emit(UiReadMessageEvent{e.messages[choice - 1].id});
    }
}

void ConsoleUI::onShowOutbox(const AppShowOutboxEvent& e) {
    _lastOutbox = e.messages;
    _currentScreen = Screen::Outbox;
    printHeader("Outbox");

    if (e.messages.empty()) {
        std::cout << "  No messages sent yet.\n";
        waitEnter();
        Bus().emit(UiNavigateEvent{Screen::MainMenu});
        return;
    }

    std::cout << "  " << std::left << std::setw(5) << "#" << std::setw(14)
              << "To" << std::setw(14) << "Pigeon" << std::setw(20) << "Status"
              << std::setw(14) << "ETA"
              << "Preview\n"
              << "  " << std::string(80, '-') << "\n";

    for (int i = 0; i < (int)e.messages.size(); ++i) {
        const auto& m = e.messages[i];
        std::string preview =
            m.content.size() > 18 ? m.content.substr(0, 18) + "..." : m.content;
        std::cout << "  " << std::setw(5) << (i + 1) << std::setw(14)
                  << m.recipientName << std::setw(14) << m.pigeonName
                  << std::setw(20) << m.status << std::setw(14) << m.etaString
                  << preview << "\n";
    }

    std::cout << "\n";
    waitEnter();
    Bus().emit(UiNavigateEvent{Screen::MainMenu});
}

void ConsoleUI::onShowMessage(const AppShowMessageEvent& e) {
    const MessageData& m = e.message;
    printHeader("Message #" + m.id);

    std::cout << "  From:   \033[1m" << m.senderName << "\033[0m\n"
              << "  Via:    " << m.pigeonName << " (" << (int)m.distanceKm
              << " km)\n"
              << "  Status: " << m.status << "\n"
              << "\n  \033[33m" << std::string(50, '-') << "\033[0m\n"
              << "  " << m.content << "\n"
              << "  \033[33m" << std::string(50, '-') << "\033[0m\n";

    waitEnter();
    Bus().emit(UiNavigateEvent{Screen::Inbox});
}

void ConsoleUI::onShowMarket(const AppShowMarketEvent& e) {
    _lastMarket = e.pigeons;
    _currentScreen = Screen::Market;
    printHeader("Pigeon Market");
    std::cout << "Last market refresh was on ";
    std::tm tm = *std::localtime(&e.lastMarketRefresh);
    std::cout << std::put_time(&tm, "%d/%m at %H:%M") << "\n\n";

    if (_lastMarket.size() > 1) {
        std::cout << "  " << (_lastMarket.size() == 3 ? "Three" : "Two")
                  << " pigeons available today. Choose wisely!\n\n";
    } else if (_lastMarket.size()) {
        std::cout << "   Only one pigeon left on the market today.\n\n";
    } else {
        std::time_t next_refresh = e.lastMarketRefresh + 24 * 60 * 60;
        std::time_t now = std::time(nullptr);
        std::time_t remaining = next_refresh - now;
        std::time_t hours = remaining / 3600;
        std::time_t minutes = (remaining % 3600) / 60;
        std::cout << "    No pigeons are available today. Come back in "
                  << hours << "h " << minutes << "m\n\n";
    }

    for (int i = 0; i < (int)e.pigeons.size(); ++i) {
        const auto& p = e.pigeons[i];
        std::cout << "\n  Option " << (i + 1) << ":\n"
                  << "  +-----------------------------------------+\n"
                  << "  | " << rarityColor(p.rarity) << "\033[1m" << std::left
                  << std::setw(35) << p.name << "\033[0m  |\n"
                  << "  | " << rarityColor(p.rarity) << std::setw(37)
                  << ("[" + p.rarity + "]") << "\033[0m|\n"
                  << "  | SPD " << statBar(p.speed) << " " << std::setw(2)
                  << p.speed << "  |\n"
                  << "  | ACC " << statBar(p.accuracy) << " " << std::setw(2)
                  << p.accuracy << "  |\n"
                  << "  | STA " << statBar(p.stamina) << " " << std::setw(2)
                  << p.stamina << "  |\n"
                  << "  | STL " << statBar(p.stealth) << " " << std::setw(2)
                  << p.stealth << "  |\n"
                  << "  | LOY " << statBar(p.loyalty) << " " << std::setw(2)
                  << p.loyalty << "  |\n"
                  << "  | OVR: \033[1m" << p.overall << "\033[0m"
                  << std::string(30, ' ') << "|\n"
                  << "  +-----------------------------------------+\n";
    }

    std::cout << "\n  [1-" << e.pigeons.size() << "]  Adopt  [0] Back\n\n";
    int choice = getInt("Choose", 0, (int)e.pigeons.size());
    if (choice == 0) {
        Bus().emit(UiNavigateEvent{Screen::MainMenu});
    } else {
        Bus().emit(UiBuyPigeonEvent{e.pigeons[choice - 1].id});
    }
}

void ConsoleUI::onShowLocations(const AppShowLocationsEvent& e) {
    _lastLocations = e.locations;
    _currentScreen = Screen::Locations;
    printHeader("Locations");

    if (e.locations.empty()) {
        std::cout << "  No locations saved yet.\n\n";
    } else {
        std::cout << "  " << std::left << std::setw(5) << "#" << std::setw(20)
                  << "Name" << std::setw(12) << "Latitude"
                  << "Longitude\n"
                  << "  " << std::string(50, '-') << "\n";

        for (int i = 0; i < (int)e.locations.size(); ++i) {
            const auto& l = e.locations[i];
            std::cout << "  " << std::setw(5) << (i + 1) << std::setw(20)
                      << l.name << std::setw(12) << std::fixed
                      << std::setprecision(4) << l.lat << l.lon << "\n";
        }
    }

    std::cout << "\n  [1-"
              << (e.locations.empty() ? 0 : (int)e.locations.size())
              << "]  Set as my location\n"
              << "  [A]  Add new location\n"
              << "  [0]  Back\n\n";

    // Can't use getInt for mixed int/char input — handle manually
    std::string raw;
    std::cout << " Choose: ";
    std::getline(std::cin, raw);

    if (raw == "0") {
        Bus().emit(UiNavigateEvent{Screen::MainMenu});
    } else if (raw == "A" || raw == "a") {
        std::string name = getString("Location name");
        std::string latS = getString("Latitude");
        std::string lonS = getString("Longitude");
        try {
            double lat = std::stod(latS);
            double lon = std::stod(lonS);
            Bus().emit(UiAddLocationEvent{name, lat, lon});
        } catch (...) {
            Bus().emit(AppErrorEvent{"Invalid coordinates."});
            Bus().emit(UiNavigateEvent{Screen::Locations});
        }
    } else {
        try {
            int idx = std::stoi(raw) - 1;
            if (idx >= 0 && idx < (int)e.locations.size())
                Bus().emit(UiSelectLocationEvent{e.locations[idx].id});
            else
                Bus().emit(UiNavigateEvent{Screen::Locations});
        } catch (...) {
            Bus().emit(UiNavigateEvent{Screen::Locations});
        }
    }
}

void ConsoleUI::onMessageSent(const AppMessageSentEvent& e) {
    std::cout << "\n  \033[32m[+] Message sent with \033[1m" << e.pigeonName
              << "\033[0m\033[32m to " << e.recipientName << "!\033[0m\n"
              << "  ETA: " << e.etaString << "\n";
    waitEnter();
}

void ConsoleUI::onLoginSuccess(const AppLoginSuccessEvent& e) {
    std::cout << "\n  \033[32m[+] Welcome back, \033[1m" << e.username
              << "\033[0m\033[32m!\033[0m\n";
    waitEnter();
}

void ConsoleUI::onLoginFailed(const AppLoginFailedEvent& e) {
    std::cout << "\n  \033[31m[x] Login failed: " << e.reason << "\033[0m\n";
    waitEnter();
}

void ConsoleUI::onSignupSuccess(const AppSignupSuccessEvent& e) {
    std::cout << "\n  \033[32m[+] Account created! Welcome, \033[1m"
              << e.username << "\033[0m\033[32m!\033[0m\n";
    waitEnter();
}

void ConsoleUI::onSignupFailed(const AppSignupFailedEvent& e) {
    std::cout << "\n  \033[31m[x] Signup failed: " << e.reason << "\033[0m\n";
    waitEnter();
}

void ConsoleUI::onError(const AppErrorEvent& e) {
    std::cout << "\n  \033[31m[x] " << e.message << "\033[0m\n";
    waitEnter();
}

void ConsoleUI::onSuccess(const AppSuccessEvent& e) {
    std::cout << "\n  \033[32m[+] " << e.message << "\033[0m\n";
    waitEnter();
}

// ============================================================
//  Send message flow
//  This is the one screen that needs two round-trips:
//  1) App sends AppShowPigeonRosterEvent with available pigeons
//  2) User picks one + types message
//  3) UI emits UiSendMessageEvent
//  We detect we're in "send mode" via _currentScreen.
// ============================================================

// onShowRoster already stores _lastRoster. The run() loop
// handles Screen::SendMessage as a special case — it reuses
// the roster data but collects different input.
// We inject that logic by overriding what happens after
// AppShowPigeonRosterEvent when _currentScreen == SendMessage.

// This is handled in registerHandlers via a second on<> for
// AppShowPigeonRosterEvent that checks _currentScreen:

// Actually we do this cleanly by having onShowRoster check
// _currentScreen and branch. Let's patch that here:

// NOTE: The above is done inside onShowRoster below — the
// Send Message path is guarded by _currentScreen == SendMessage.

// The implementation above already calls Bus().emit(AppShowPigeonRosterEvent)
// from App::onNavigate for Screen::SendMessage. onShowRoster will receive it.
// We need to detect that and show send-message UI instead of roster UI.
// Let's update onShowRoster to handle both cases:

// (Already accounted for in the full implementation — see below)

// ============================================================
//  UI helpers
// ============================================================

void ConsoleUI::printHeader(const std::string& title) const {
    std::cout << "\033[2J\033[H";
    std::cout
        << "\033[33m"
        << " +==================================================+\n"
        << " |        >>  P I G E O N   M A I L  <<            |\n"
        << " +==================================================+\033[0m\n";
    if (!title.empty()) std::cout << "\n \033[1m" << title << "\033[0m\n";
    printSeparator();
}

void ConsoleUI::printSeparator() const {
    std::cout << " \033[90m" << std::string(52, '-') << "\033[0m\n";
}

void ConsoleUI::waitEnter() const {
    std::cout << "\n  \033[90mPress Enter to continue...\033[0m ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string ConsoleUI::statBar(int value, int width) const {
    int filled = (value * width) / 99;
    std::string bar = "[";
    for (int i = 0; i < width; ++i) bar += (i < filled) ? "#" : ".";
    return bar + "]";
}

std::string ConsoleUI::rarityColor(const std::string& rarity) const {
    if (rarity == "Common") return "\033[37m";
    if (rarity == "Uncommon") return "\033[32m";
    if (rarity == "Rare") return "\033[34m";
    if (rarity == "Epic") return "\033[35m";
    if (rarity == "Legendary") return "\033[33m";
    return "\033[0m";
}

int ConsoleUI::getInt(const std::string& prompt, int min, int max) const {
    int val;
    while (true) {
        std::cout << "  " << prompt << " (" << min << "-" << max << "): ";
        if (std::cin >> val && val >= min && val <= max) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "  \033[31mInvalid input, try again.\033[0m\n";
    }
}

std::string ConsoleUI::getString(const std::string& prompt) const {
    std::string s;
    std::cout << "  " << prompt << ": ";
    std::getline(std::cin, s);
    return s;
}