#pragma once

// ============================================================
//  UI — abstract base
//  Subclasses register App* event handlers and own the
//  run loop. They never touch GameState or App directly —
//  everything goes through the bus.
// ============================================================
class UI {
   public:
    virtual ~UI() = default;

    // Register all App* event handlers on the bus.
    // Call once before run(), after App::registerHandlers().
    virtual void registerHandlers() = 0;

    // Start the UI loop. Returns when the user exits.
    virtual void run() = 0;
};