# 🐦 Pigeon Mail

A terminal-based C++ messaging app for you and your partner, using carrier pigeons.
Messages travel in real time — arrival depends on your pigeon's stats and the actual
distance between your locations.

---

## Features

- **Real distance calculation** using the Haversine formula (actual GPS coordinates)
- **Pigeon stat system** — Speed, Accuracy, Stamina, Stealth, Loyalty — just like FIFA cards
- **Rarity tiers** — Common → Uncommon → Rare → Epic → Legendary
- **Real flight time** — messages arrive after hours/days depending on distance and pigeon speed
- **Risk mechanics** — pigeons can get lost (low Loyalty) or deliver corrupted messages (low Accuracy)
- **Pigeon Market** — adopt new pigeons with randomly generated stats
- **Rename your pigeons** and track their flight history
- **Inbox / Outbox** with live ETA countdown

---

## Build

### Requirements
- C++17 compiler (g++ or clang++)
- Terminal with ANSI color support

### With g++ directly
```bash
g++ -std=c++17 -O2 -Iinclude \
    src/main.cpp src/Pigeon.cpp src/Message.cpp \
    src/Player.cpp src/PigeonMailApp.cpp \
    -o pigeon_mail
./pigeon_mail
```

### With CMake
```bash
mkdir build && cd build
cmake ..
make
./pigeon_mail
```

---

## How to Play

1. **Setup** — enter both your names and pick your cities from the list (or enter custom GPS coordinates).
2. **Send messages** — choose a pigeon and type your message. The pigeon flies in real time.
3. **Check inbox** — messages from your partner appear after the pigeon's flight time elapses.
4. **Grow your loft** — visit the Pigeon Market to adopt new birds with better stats.

---

## Pigeon Stats

| Stat     | Effect |
|----------|--------|
| SPD      | Faster flight → shorter arrival time |
| ACC      | Higher chance of message arriving readable |
| STA      | Reduces fatigue penalty on very long routes |
| STL      | Chance of not being intercepted (future feature) |
| LOY      | Reduces chance of the pigeon getting lost |
| **OVR**  | Average of all five stats |

---

## Architecture

```
include/
  Pigeon.h          - PigeonStats, PigeonRarity, Pigeon class
  Message.h         - Message struct, MessageStatus enum
  Player.h          - Player class, Location struct, Haversine distance
  PigeonMailApp.h   - Main application / UI

src/
  Pigeon.cpp        - Stat bars, flight time, delivery logic, pigeon factories
  Message.cpp       - Status strings, ETA countdown
  Player.cpp        - Send/receive, inbox/outbox printing
  PigeonMailApp.cpp - All UI menus and game loop
  main.cpp          - Entry point
```

---

## Extending the App

Some ideas for future versions:

- **Network mode** — use sockets so you and your partner run separate instances
- **Pigeon fatigue** — pigeons need rest between flights
- **Weather system** — wind/rain affects flight time randomly
- **Pigeon breeding** — combine two pigeons to get offspring with mixed stats
- **Rival interceptors** — low Stealth pigeons can have messages stolen
- **Save/load** — serialize player state to a JSON or binary file
