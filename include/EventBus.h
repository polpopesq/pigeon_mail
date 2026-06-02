#pragma once
#include <any>
#include <functional>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

// ============================================================
//  HandlerList
//  Owns all handlers registered for one specific event type T.
//  Type-erased so the bus can store them in a single map.
// ============================================================

// Base class — the bus holds pointers to these
struct HandlerListBase {
    virtual ~HandlerListBase() = default;

    // Replay one queued event (stored as std::any holding a T)
    virtual void replayOne(const std::any& payload) = 0;
};

template <typename T>
struct HandlerList : HandlerListBase {
    // Each handler is a function that takes a const ref to the event struct
    std::vector<std::function<void(const T&)>> handlers;

    void replayOne(const std::any& payload) override {
        // std::any_cast is safe here: we only store T in T's bucket
        const T& event = std::any_cast<const T&>(payload);
        for (auto& h : handlers) h(event);
    }

    void invoke(const T& event) {
        for (auto& h : handlers) h(event);
    }
};

// ============================================================
//  EventBus
// ============================================================

class EventBus {
   public:
    // Singleton access
    static EventBus& instance() {
        static EventBus inst;
        return inst;
    }

    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    // ----------------------------------------------------------
    //  on<T>(handler)
    //  Register a handler for event type T.
    //  If there are queued events of type T, replay them now.
    // ----------------------------------------------------------
    template <typename T>
    void on(std::function<void(const T&)> handler) {
        auto& list = getOrCreateList<T>();
        list.handlers.push_back(handler);

        // Replay any events that fired before this handler existed
        auto key = std::type_index(typeid(T));
        auto it = _pending.find(key);
        if (it != _pending.end()) {
            for (auto& payload : it->second) list.replayOne(payload);
            _pending.erase(it);
        }
    }

    // ----------------------------------------------------------
    //  emit<T>(event)
    //  Fire an event. If no handler is registered yet, queue it.
    //  dispatch() is virtual so subclasses can defer execution
    //  (e.g. an SFML bus that drains the queue each frame).
    // ----------------------------------------------------------
    template <typename T>
    void emit(T event) {
        auto key = std::type_index(typeid(T));
        auto it = _handlers.find(key);

        if (it == _handlers.end()) {
            // No handler registered yet — queue for replay
            _pending[key].push_back(std::any(std::move(event)));
            return;
        }

        auto* list = static_cast<HandlerList<T>*>(it->second.get());

        // Wrap the actual dispatch in a lambda.
        // Synchronous: dispatch() just calls fn() immediately.
        // Async subclass: dispatch() pushes fn into a frame queue.
        dispatch([list, event = std::move(event)]() { list->invoke(event); });
    }

    // ----------------------------------------------------------
    //  off<T>()
    //  Remove all handlers for event type T.
    //  Useful when tearing down a screen or switching UI mode.
    // ----------------------------------------------------------
    template <typename T>
    void off() {
        auto key = std::type_index(typeid(T));
        _handlers.erase(key);
    }

    // ----------------------------------------------------------
    //  clear()
    //  Remove everything — handlers and pending queue.
    //  Call between major state transitions (e.g. logout).
    // ----------------------------------------------------------
    void clear() {
        _handlers.clear();
        _pending.clear();
    }

   protected:
    // ----------------------------------------------------------
    //  dispatch(fn)
    //  The single seam for sync vs async execution.
    //  Synchronous (default): call fn() immediately.
    //  Override in a subclass to push fn into a frame queue.
    // ----------------------------------------------------------
    virtual void dispatch(std::function<void()> fn) { fn(); }

   private:
    EventBus() = default;

    // Get or create the HandlerList<T> for type T
    template <typename T>
    HandlerList<T>& getOrCreateList() {
        auto key = std::type_index(typeid(T));
        auto it = _handlers.find(key);
        if (it == _handlers.end()) {
            _handlers[key] = std::make_unique<HandlerList<T>>();
            it = _handlers.find(key);
        }
        return *static_cast<HandlerList<T>*>(it->second.get());
    }

    // Registered handlers, keyed by event type
    std::unordered_map<std::type_index, std::unique_ptr<HandlerListBase>>
        _handlers;

    // Events that fired before any handler was registered, keyed by event type.
    // Stored as std::any so we can hold any T without knowing T here.
    std::unordered_map<std::type_index, std::vector<std::any>> _pending;
};

// ============================================================
//  Async-ready subclass stub — ready for SFML later
//  Uncomment and use in main.cpp when you have a game loop.
// ============================================================
//
// #include <queue>
// #include <mutex>
//
// class AsyncEventBus : public EventBus {
// public:
//     // Call once per frame from your SFML game loop
//     void drainQueue() {
//         std::lock_guard<std::mutex> lock(_mutex);
//         while (!_frameQueue.empty()) {
//             _frameQueue.front()();
//             _frameQueue.pop();
//         }
//     }
//
// protected:
//     void dispatch(std::function<void()> fn) override {
//         std::lock_guard<std::mutex> lock(_mutex);
//         _frameQueue.push(std::move(fn));
//     }
//
// private:
//     std::queue<std::function<void()>> _frameQueue;
//     std::mutex                         _mutex;
// };

// ============================================================
//  Convenience free function
// ============================================================
inline EventBus& Bus() { return EventBus::instance(); }