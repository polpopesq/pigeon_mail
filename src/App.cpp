#include "App.h"

#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "EventBus.h"
#include "Pigeon.h"
#include "Player.h"

// ============================================================
//  Constructor
// ============================================================

App::App() {
    std::random_device rd;
    _rng = std::mt19937(rd());
}

// ============================================================
//  registerHandlers
//  Wire every Ui* event to its handler method.
//  'this' capture is safe: App outlives the bus in main().
// ============================================================

void App::registerHandlers() {
    Bus().on<UiNavigateEvent>(
        [this](const UiNavigateEvent& e) { onNavigate(e); });
    Bus().on<UiLoginEvent>([this](const UiLoginEvent& e) { onLogin(e); });
    Bus().on<UiSignupEvent>([this](const UiSignupEvent& e) { onSignup(e); });
    Bus().on<UiLogoutEvent>([this](const UiLogoutEvent& e) { onLogout(e); });
    Bus().on<UiSendMessageEvent>(
        [this](const UiSendMessageEvent& e) { onSendMessage(e); });
    Bus().on<UiBuyPigeonEvent>(
        [this](const UiBuyPigeonEvent& e) { onBuyPigeon(e); });
    Bus().on<UiRenamePigeonEvent>(
        [this](const UiRenamePigeonEvent& e) { onRenamePigeon(e); });
    Bus().on<UiAddLocationEvent>(
        [this](const UiAddLocationEvent& e) { onAddLocation(e); });
    Bus().on<UiSelectLocationEvent>(
        [this](const UiSelectLocationEvent& e) { onSelectLocation(e); });
    Bus().on<UiRefreshInboxEvent>(
        [this](const UiRefreshInboxEvent& e) { onRefreshInbox(e); });
    Bus().on<UiReadMessageEvent>(
        [this](const UiReadMessageEvent& e) { onReadMessage(e); });
}

// ============================================================
//  Navigation
//  Every screen transition goes through here.
//  We tick flights/market on every navigation so the game
//  state is always fresh when a screen renders.
// ============================================================

void App::onNavigate(const UiNavigateEvent& e) {
    if (_state.isReady()) {
        tickFlights();
        tickMarket();
    }

    switch (e.destination) {
        case Screen::MainMenu:
            emitMainMenu();
            break;

        case Screen::PigeonRoster: {
            std::vector<PigeonData> pigeons;
            for (const auto& p : _state.you->pigeons)
                pigeons.push_back(toPigeonData(p));
            Bus().emit(AppShowPigeonRosterEvent{pigeons});
            break;
        }

        case Screen::Inbox: {
            _state.you->checkInbox();
            std::vector<MessageData> messages;
            for (const auto& m : _state.you->inbox)
                messages.push_back(toMessageData(m));
            Bus().emit(AppShowInboxEvent{messages});
            break;
        }

        case Screen::Outbox: {
            std::vector<MessageData> messages;
            for (const auto& m : _state.you->outbox)
                messages.push_back(toMessageData(m));
            Bus().emit(AppShowOutboxEvent{messages});
            break;
        }

        case Screen::Market: {
            std::vector<PigeonData> offers;
            for (const auto& p : _state.marketPigeons)
                offers.push_back(toPigeonData(p));
            Bus().emit(AppShowMarketEvent{offers, _state.lastMarketRefresh});
            break;
        }

        case Screen::Locations:
            Bus().emit(AppShowLocationsEvent{_state.locations});
            break;

        case Screen::SendMessage: {
            // SendMessage screen needs the available pigeons to show ETA
            // options. We reuse the roster event — UI decides how to present
            // it.
            std::vector<PigeonData> available;
            for (const auto& p : _state.you->pigeons)
                if (p.isAvailable) available.push_back(toPigeonData(p));

            if (available.empty()) {
                Bus().emit(AppErrorEvent{
                    "All your pigeons are currently in transit!"});
                Bus().emit(UiNavigateEvent{Screen::MainMenu});
                return;
            }
            Bus().emit(AppShowPigeonRosterEvent{available});
            break;
        }

        case Screen::Login:
            // Nothing to prepare — UI just shows the login form
            break;
    }
}

// ============================================================
//  Auth handlers
// ============================================================

void App::onLogin(const UiLoginEvent& e) {
    // TODO: replace with AuthService::instance().login() in Step 4
    // For now: accept any non-empty credentials and set up dummy state

    if (e.username.empty() || e.password.empty()) {
        Bus().emit(
            AppLoginFailedEvent{"Username and password cannot be empty."});
        return;
    }

    // Dummy setup — will be replaced when Supabase auth is wired in
    Location yourLoc{e.username + "'s city", 44.4268, 26.1025};
    Location partnerLoc{"Partner's city", 51.5074, -0.1278};

    _state.you = std::make_unique<Player>(e.username, yourLoc);
    _state.partner = std::make_unique<Player>("Partner", partnerLoc);

    _state.you->addPigeon(generateStarterPigeon("Pip"));
    _state.you->addPigeon(generateRandomPigeon(_rng));
    _state.you->addPigeon(generateRandomPigeon(_rng));

    // Seed welcome message
    double distance = haversineDistance(yourLoc, partnerLoc);
    Message welcome;
    welcome.id = 999;
    welcome.sender = "Partner";
    welcome.recipient = e.username;
    welcome.content =
        "My dearest, I've just set up my pigeon loft. Write to me soon!";
    welcome.pigeonName = "Feather";
    welcome.distanceKm = distance;
    welcome.status = MessageStatus::Delivered;
    welcome.sentAt = std::time(nullptr) - 3600;
    welcome.arrivesAt = std::time(nullptr) - 10;
    _state.you->inbox.push_back(welcome);

    tickMarket();  // generate initial market offers

    Bus().emit(AppLoginSuccessEvent{e.username});
    Bus().emit(UiNavigateEvent{Screen::MainMenu});
}

void App::onSignup(const UiSignupEvent& e) {
    // TODO: replace with AuthService::instance().signup() in Step 4
    if (e.username.empty() || e.password.empty()) {
        Bus().emit(
            AppSignupFailedEvent{"Username and password cannot be empty."});
        return;
    }
    // For now, signup just logs in
    onLogin(UiLoginEvent{e.username, e.password});
    Bus().emit(AppSignupSuccessEvent{e.username});
}

void App::onLogout(const UiLogoutEvent&) {
    _state.you.reset();
    _state.partner.reset();
    _state.marketPigeons.clear();
    _state.lastMarketRefresh = 0;
    Bus().emit(UiNavigateEvent{Screen::Login});
}

// ============================================================
//  Messaging
// ============================================================

void App::onSendMessage(const UiSendMessageEvent& e) {
    if (!_state.isReady()) {
        Bus().emit(AppErrorEvent{"Not logged in."});
        return;
    }

    if (e.content.empty()) {
        Bus().emit(AppErrorEvent{"Message cannot be empty."});
        return;
    }

    // Find the pigeon by id
    int pigeonIndex = -1;
    for (int i = 0; i < (int)_state.you->pigeons.size(); ++i) {
        if (_state.you->pigeons[i].name ==
            e.pigeonId) {  // using name as id until DB
            pigeonIndex = i;
            break;
        }
    }

    if (pigeonIndex == -1) {
        Bus().emit(AppErrorEvent{"Pigeon not found."});
        return;
    }

    Pigeon& pigeon = _state.you->pigeons[pigeonIndex];

    if (!pigeon.isAvailable) {
        Bus().emit(AppErrorEvent{pigeon.name + " is already in transit!"});
        return;
    }

    double distance =
        haversineDistance(_state.you->location, _state.partner->location);

    // Send — updates pigeon.isAvailable and appends to outbox
    Message sent = _state.you->sendMessage(_state.partner->name, e.content,
                                           pigeonIndex, distance, _rng);

    // Determine outcome (lost / corrupted) using pigeon stats
    bool arrived = pigeon.deliverMessage(distance, _rng);
    std::uniform_int_distribution<int> accRoll(1, 99);
    bool readable = accRoll(_rng) <= pigeon.stats.accuracy;

    // Mirror into partner's inbox — status resolved at delivery time
    Message received = sent;
    if (!arrived) {
        received.status = MessageStatus::Lost;
    } else if (!readable) {
        received.status = MessageStatus::Corrupted;
        received.content = "~~ message garbled in transit ~~";
    }
    _state.partner->inbox.push_back(received);

    // Build ETA string for the confirmation event
    double etaMinutes = pigeon.estimateFlightTime(distance);
    std::string eta = formatEta(etaMinutes);

    Bus().emit(AppMessageSentEvent{pigeon.name, _state.partner->name, eta});

    // Return to main menu after sending
    Bus().emit(UiNavigateEvent{Screen::MainMenu});
}

// ============================================================
//  Pigeon management
// ============================================================

void App::onBuyPigeon(const UiBuyPigeonEvent& e) {
    // Find the market pigeon by name (will be uuid from DB later)
    auto it = std::find_if(
        _state.marketPigeons.begin(), _state.marketPigeons.end(),
        [&](const Pigeon& p) { return p.name == e.marketPigeonId; });

    if (it == _state.marketPigeons.end()) {
        Bus().emit(AppErrorEvent{"That pigeon is no longer available."});
        return;
    }

    Pigeon adopted = *it;
    _state.marketPigeons.erase(it);
    _state.you->addPigeon(adopted);

    Bus().emit(AppSuccessEvent{adopted.name + " has joined your loft!"});
    Bus().emit(UiNavigateEvent{Screen::PigeonRoster});
}

void App::onRenamePigeon(const UiRenamePigeonEvent& e) {
    if (e.newName.empty()) {
        Bus().emit(AppErrorEvent{"Name cannot be empty."});
        return;
    }

    for (auto& p : _state.you->pigeons) {
        if (p.name == e.pigeonId) {  // will be uuid from DB later
            std::string old = p.name;
            p.name = e.newName;
            Bus().emit(AppSuccessEvent{old + " renamed to " + e.newName + "."});
            Bus().emit(UiNavigateEvent{Screen::PigeonRoster});
            return;
        }
    }

    Bus().emit(AppErrorEvent{"Pigeon not found."});
}

// ============================================================
//  Locations
// ============================================================

void App::onAddLocation(const UiAddLocationEvent& e) {
    if (e.name.empty()) {
        Bus().emit(AppErrorEvent{"Location name cannot be empty."});
        return;
    }
    if (e.lat < -90.0 || e.lat > 90.0) {
        Bus().emit(AppErrorEvent{"Latitude must be between -90 and 90."});
        return;
    }
    if (e.lon < -180.0 || e.lon > 180.0) {
        Bus().emit(AppErrorEvent{"Longitude must be between -180 and 180."});
        return;
    }

    LocationData loc;
    loc.id = e.name;  // will be uuid from DB later
    loc.name = e.name;
    loc.lat = e.lat;
    loc.lon = e.lon;
    loc.lastUsedAt = std::time(nullptr);

    _state.locations.insert(_state.locations.begin(),
                            loc);  // front = most recent

    Bus().emit(AppSuccessEvent{"Location '" + e.name + "' added."});
    Bus().emit(AppShowLocationsEvent{_state.locations});
}

void App::onSelectLocation(const UiSelectLocationEvent& e) {
    for (auto& loc : _state.locations) {
        if (loc.id == e.locationId) {
            // Update player location
            _state.you->location = Location{loc.name, loc.lat, loc.lon};
            loc.lastUsedAt = std::time(nullptr);

            // Re-sort by last use
            std::sort(_state.locations.begin(), _state.locations.end(),
                      [](const LocationData& a, const LocationData& b) {
                          return a.lastUsedAt > b.lastUsedAt;
                      });

            Bus().emit(AppSuccessEvent{"Location set to " + loc.name + "."});
            emitMainMenu();
            return;
        }
    }
    Bus().emit(AppErrorEvent{"Location not found."});
}

// ============================================================
//  Inbox
// ============================================================

void App::onRefreshInbox(const UiRefreshInboxEvent&) {
    _state.you->checkInbox();
    std::vector<MessageData> messages;
    for (const auto& m : _state.you->inbox)
        messages.push_back(toMessageData(m));
    Bus().emit(AppShowInboxEvent{messages});
}

void App::onReadMessage(const UiReadMessageEvent& e) {
    for (const auto& m : _state.you->inbox) {
        if (std::to_string(m.id) == e.messageId) {
            Bus().emit(AppShowMessageEvent{toMessageData(m)});
            return;
        }
    }
    Bus().emit(AppErrorEvent{"Message not found."});
}

// ============================================================
//  Internal helpers
// ============================================================

void App::tickFlights() {
    std::time_t now = std::time(nullptr);
    for (auto& msg : _state.you->outbox) {
        if (msg.status == MessageStatus::InTransit && now >= msg.arrivesAt) {
            msg.status = MessageStatus::Delivered;
            // Free the pigeon that carried this message
            for (auto& p : _state.you->pigeons)
                if (p.name == msg.pigeonName && !p.isAvailable)
                    p.isAvailable = true;
        }
    }
}

void App::tickMarket() {
    constexpr int MARKET_SIZE = 3;
    constexpr int MARKET_INTERVAL = 24 * 60 * 60;  // 24 hours in seconds

    std::time_t now = std::time(nullptr);
    if (now - _state.lastMarketRefresh < MARKET_INTERVAL) return;

    _state.marketPigeons.clear();
    for (int i = 0; i < MARKET_SIZE; ++i)
        _state.marketPigeons.push_back(generateRandomPigeon(_rng));

    _state.lastMarketRefresh = now;
}

void App::emitMainMenu() {
    if (!_state.isReady()) return;

    long unread = std::count_if(
        _state.you->inbox.begin(), _state.you->inbox.end(),
        [](const Message& m) { return m.status == MessageStatus::Delivered; });
    long inTransit = std::count_if(
        _state.you->outbox.begin(), _state.you->outbox.end(),
        [](const Message& m) { return m.status == MessageStatus::InTransit; });

    Bus().emit(AppShowMainMenuEvent{_state.you->name, _state.you->location.name,
                                    (int)_state.you->pigeons.size(),
                                    (int)unread, (int)inTransit});
}

PigeonData App::toPigeonData(const Pigeon& p) const {
    return PigeonData{p.name,  // id: using name until DB gives us uuid
                      p.ownerName,     p.name,
                      p.rarityName(),  p.description,
                      p.stats.speed,   p.stats.accuracy,
                      p.stats.stamina, p.stats.stealth,
                      p.stats.loyalty, p.stats.overall(),
                      p.isAvailable,   p.messagesSent};
}

MessageData App::toMessageData(const Message& m) const {
    return MessageData{std::to_string(m.id),
                       "",  // senderId — will come from DB
                       m.sender,
                       "",  // recipientId — will come from DB
                       m.recipient,
                       m.pigeonName,
                       m.content,
                       m.statusString(),
                       m.distanceKm,
                       m.sentAt,
                       m.arrivesAt,
                       formatTimeRemaining(m.arrivesAt)};
}

std::string App::formatEta(double minutes) const {
    int h = (int)(minutes / 60);
    int m = (int)minutes % 60;
    std::ostringstream oss;
    if (h > 0) oss << h << "h ";
    oss << m << "m";
    return oss.str();
}

std::string App::formatTimeRemaining(std::time_t arrivesAt) const {
    std::time_t now = std::time(nullptr);
    if (now >= arrivesAt) return "arrived";

    long secs = (long)(arrivesAt - now);
    long mins = secs / 60;
    long hours = mins / 60;
    long days = hours / 24;

    std::ostringstream oss;
    if (days > 0)
        oss << days << "d " << (hours % 24) << "h";
    else if (hours > 0)
        oss << hours << "h " << (mins % 60) << "m";
    else if (mins > 0)
        oss << mins << "m " << (secs % 60) << "s";
    else
        oss << secs << "s";
    return oss.str();
}