#pragma once
#include "MovementController.h"
#include "EmotionSystem.h"
#include "Blob.h"
#include "ScreenInfo.h"
#include <random>

enum class BehaviorState {
    Idle,          // standing still, blinking, occasionally glancing
    LookingAround, // idle but actively scanning for a new destination
    Traveling,     // moving toward a chosen destination
    Resting,       // longer, sleepier stillness
    Flinching,     // brief "bonk" reaction after being clicked, overrides everything else
};

// The autonomous "brain". Continuously decides what the blob wants to do
// next based on internal state (emotion, time-in-state, speed) plus
// weighted randomness — never a fixed repeating script. Owns the
// high-level state machine; delegates physical motion to
// MovementController and visible reactions to Blob (body + eye).
class BehaviorController {
public:
    BehaviorController(uint32_t seed, MovementController& movement, Blob& blob,
                        EmotionSystem& emotion, ScreenInfo& screen);

    void Update(float dt);

    BehaviorState State() const { return state_; }

    // True while the creature is essentially motionless and idle — used by
    // the main loop to throttle update/render rate for CPU savings.
    bool IsRestful() const;

    // Called once per frame by the main loop with the current global mouse
    // position (screen coordinates), so the blob can glance toward a
    // nearby cursor. `valid` is false if the pointer query failed.
    void SetCursorInfo(float x, float y, bool valid);

    // Called when the mouse button transitions down; internally checks
    // whether the click landed on/near the blob and triggers the "bonk"
    // reaction if so. Safe to call every frame — it no-ops when the click
    // isn't close enough.
    void NotifyClick(float clickX, float clickY);

private:
    void EnterState(BehaviorState s);
    void PickNewDestination();
    void MaybeChangeMindMidJourney(float dt);
    void UpdateIdleBehaviors(float dt);
    float ChooseTravelSpeed() const;
    MonitorRect CurrentBounds() const;

    mutable std::mt19937 rng_;
    MovementController& movement_;
    Blob& blob_;
    EmotionSystem& emotion_;
    ScreenInfo& screen_;

    BehaviorState state_ = BehaviorState::Idle;
    float stateTimer_ = 0.0f;
    float stateDuration_ = 3.0f; // how long to stay in current state before reconsidering

    float idleSince_ = 0.0f;
    float lastMonitorRefresh_ = 0.0f;

    // Occasional small idle-in-place shuffles without a real destination.
    float microShuffleCooldown_ = 4.0f;

    bool lookedBeforeMoving_ = false;

    // Cursor awareness.
    float cursorX_ = 0.0f, cursorY_ = 0.0f;
    bool cursorValid_ = false;
    BehaviorState preFlinchState_ = BehaviorState::Idle;

    // Random happiness bursts — independent of the main state machine so
    // they can interrupt idling or traveling alike.
    float happyBurstCooldown_ = 20.0f;
};
