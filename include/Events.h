#pragma once
#include <ctime>
#include <string>
#include <vector>

// ============================================================
//  Shared data structs
//  These are dumb data carriers — no methods, no logic.
//  Both App and UI see these; neither owns them.
// ============================================================

// A flat, display-ready snapshot of a pigeon.
// App fills this from its internal Pigeon object.
// UI reads this to render — it never touches Pigeon directly.

struct PigeonData {
    std::string id;
    std::string ownerName;
    std::string name;
    std::string rarity;
    std::string description;
    int speed;
    int accuracy;
    int stamina;
    int stealth;
    int loyalty;
    int overall;
    bool isAvailable;
    int messagesSent;
};

struct MessageData {
    std::string id;
    std::string senderId;
    std::string senderName;
    std::string recipientId;
    std::string recipientName;
    std::string pigeonName;
    std::string content;
    std::string status;  // "in_transit", "delivered", "lost", "corrupted"
    double distanceKm;
    std::time_t sentAt;
    std::time_t arrivesAt;
    std::string etaString;  // pre-formatted: "3h 20m", "arriving now", etc.
};

struct LocationData {
    std::string id;
    std::string name;
    double lat;
    double lon;
    std::time_t lastUsedAt;
};

// ============================================================
//  UI --> App events
//  Naming: anything the user initiated.
//  Prefix: Ui prefix, e.g. UiLoginEvent
// ============================================================

struct UiLoginEvent {
    std::string username;
    std::string password;
};

struct UiSignupEvent {
    std::string username;
    std::string password;
};

struct UiLogoutEvent {};

struct UiSendMessageEvent {
    std::string content;
    std::string pigeonId;
};

struct UiBuyPigeonEvent {
    std::string marketPigeonId;
};

struct UiRenamePigeonEvent {
    std::string pigeonId;
    std::string newName;
};

struct UiAddLocationEvent {
    std::string name;
    double lat;
    double lon;
};

struct UiSelectLocationEvent {
    std::string locationId;  // user picked this as active location
};

// User explicitly asked to refresh inbox display
struct UiRefreshInboxEvent {};

// User asked to read a specific message
struct UiReadMessageEvent {
    std::string messageId;
};

// User navigated to a top-level screen
// Rather than one event per screen, we use a destination tag.
// This keeps navigation centralised in one handler in App.
enum class Screen {
    MainMenu,
    PigeonRoster,
    SendMessage,
    Inbox,
    Outbox,
    Market,
    Locations,
    Login,
};

struct UiNavigateEvent {
    Screen destination;
};

// ============================================================
//  App --> UI events
//  Naming: what the app wants displayed.
//  Prefix: App prefix, e.g. AppShowMainMenuEvent
// ============================================================

struct AppShowMainMenuEvent {
    std::string username;
    std::string locationName;
    int pigeonCount;
    int unreadCount;     // messages delivered but not yet opened
    int inTransitCount;  // outbound messages still flying
};

struct AppShowPigeonRosterEvent {
    std::vector<PigeonData> pigeons;
};

struct AppShowPigeonDetailEvent {
    PigeonData pigeon;
};

struct AppShowInboxEvent {
    std::vector<MessageData> messages;
};

struct AppShowOutboxEvent {
    std::vector<MessageData> messages;
};

struct AppShowMessageEvent {
    MessageData message;
};

struct AppShowMarketEvent {
    std::vector<PigeonData> pigeons;  // exactly 3 offers
    std::time_t lastMarketRefresh;
};

struct AppShowLocationsEvent {
    std::vector<LocationData> locations;
};

// Sent after UiSendMessageEvent is processed successfully
struct AppMessageSentEvent {
    std::string pigeonName;
    std::string recipientName;
    std::string etaString;
};

// Auth results
struct AppLoginSuccessEvent {
    std::string username;
};

struct AppLoginFailedEvent {
    std::string reason;
};

struct AppSignupSuccessEvent {
    std::string username;
};

struct AppSignupFailedEvent {
    std::string reason;
};

// Generic feedback — errors and confirmations
struct AppErrorEvent {
    std::string message;
};

struct AppSuccessEvent {
    std::string message;
};