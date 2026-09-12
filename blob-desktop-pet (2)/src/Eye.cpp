#include "Eye.h"

using mathutil::Clamp;
using mathutil::ApproachExp;
using mathutil::RandRange;

Eye::Eye(uint32_t seed)
    : rng_(seed), wanderX_(seed + 101, 3.2f), wanderY_(seed + 202, 2.7f) {
    ScheduleNextBlink();
}

void Eye::ScheduleNextBlink() {
    // Mostly-spontaneous blinking: exponential-ish spacing via a wide
    // uniform range plus occasional short "double blink" via RequestBlink.
    nextBlinkAt_ = RandRange(rng_, 2.0f, 7.0f);
    blinkTimer_ = 0.0f;
}

void Eye::LookAt(float dirX, float dirY, float holdSeconds) {
    targetX_ = Clamp(dirX, -1.0f, 1.0f);
    targetY_ = Clamp(dirY, -1.0f, 1.0f);
    overrideHold_ = holdSeconds;
}

void Eye::RequestBlink() {
    blinkPhase_ = BlinkPhase::Closing;
    blinkSpeed_ = 1.6f;
}

void Eye::RequestLongSleepyBlink() {
    blinkPhase_ = BlinkPhase::Closing;
    blinkSpeed_ = 0.35f;
}

void Eye::RequestFlinch() {
    flinchTimer_ = 0.5f;
    happySquintTimer_ = 0.0f; // reactions don't overlap
    // Look straight down/inward for a beat — a startled "brace" pose.
    LookAt(0.0f, 0.5f, 0.5f);
}

void Eye::RequestHappySquint(float holdSeconds) {
    if (flinchTimer_ > 0.0f) return; // getting bonked takes priority
    happySquintTimer_ = holdSeconds;
}

void Eye::Update(float dt, Emotion emotion, float movementDirX, float movementDirY,
                  bool isMoving) {
    // --- Reaction overrides drive the same generic parameters normal
    // blinking uses, so no special-case rendering is needed. ---
    if (flinchTimer_ > 0.0f) {
        flinchTimer_ -= dt;
        // Squeeze nearly shut and hold, rather than a quick blink-and-open.
        eyelidOpen_ = ApproachExp(eyelidOpen_, 0.12f, dt, 0.06f);
        pupilScale_ = ApproachExp(pupilScale_, 0.85f, dt, 0.15f);
        eyeWidth_ = ApproachExp(eyeWidth_, 0.9f, dt, 0.15f);
        pupilX_ = ApproachExp(pupilX_, targetX_, dt, 0.2f);
        pupilY_ = ApproachExp(pupilY_, targetY_, dt, 0.2f);
        return;
    }

    if (happySquintTimer_ > 0.0f) {
        happySquintTimer_ -= dt;
        // A gentle, content half-closed squint rather than fully shut.
        eyelidOpen_ = ApproachExp(eyelidOpen_, 0.3f, dt, 0.15f);
        eyeWidth_ = ApproachExp(eyeWidth_, 1.08f, dt, 0.2f);
        pupilScale_ = ApproachExp(pupilScale_, 1.05f, dt, 0.2f);
        pupilY_ = ApproachExp(pupilY_, -0.15f, dt, 0.25f); // slight cheerful upward glance
        pupilX_ = ApproachExp(pupilX_, 0.0f, dt, 0.25f);
        return;
    }

    // --- Pupil target selection ---
    if (overrideHold_ > 0.0f) {
        overrideHold_ -= dt;
    } else if (isMoving) {
        // Look mostly toward the direction of travel, with a little
        // organic wander layered on top so it doesn't feel locked.
        targetX_ = Clamp(movementDirX * 0.8f + wanderX_.Update(dt) * 0.2f, -1.0f, 1.0f);
        targetY_ = Clamp(movementDirY * 0.8f + wanderY_.Update(dt) * 0.2f, -1.0f, 1.0f);
    } else {
        // Idle: slow independent wandering look-around, occasionally
        // glancing at extremes (handled by the smooth-random amplitude).
        targetX_ = wanderX_.Update(dt);
        targetY_ = wanderY_.Update(dt) * 0.6f; // less vertical range looks natural
    }

    float lookSmoothing = isMoving ? 0.18f : 0.45f;
    pupilX_ = ApproachExp(pupilX_, targetX_, dt, lookSmoothing);
    pupilY_ = ApproachExp(pupilY_, targetY_, dt, lookSmoothing);

    // --- Emotion-driven eye shape ---
    float targetScale = 1.0f;
    float targetWidth = 1.0f;
    switch (emotion) {
        case Emotion::Surprised: targetScale = 1.35f; targetWidth = 1.25f; break;
        case Emotion::Curious:   targetScale = 1.15f; targetWidth = 1.05f; break;
        case Emotion::Sleepy:    targetScale = 0.85f; targetWidth = 0.9f;  break;
        case Emotion::Excited:   targetScale = 1.1f;  targetWidth = 1.0f;  break;
        case Emotion::Confused:  targetScale = 1.0f;  targetWidth = 0.95f; break;
        default: break;
    }
    pupilScale_ = ApproachExp(pupilScale_, targetScale, dt, 0.5f);
    eyeWidth_ = ApproachExp(eyeWidth_, targetWidth, dt, 0.6f);

    // --- Blink state machine ---
    const float closeTime = 0.06f / blinkSpeed_;
    const float closedHold = 0.05f / blinkSpeed_;
    const float openTime = 0.09f / blinkSpeed_;

    switch (blinkPhase_) {
        case BlinkPhase::Open:
            blinkTimer_ += dt;
            eyelidOpen_ = ApproachExp(eyelidOpen_, 1.0f, dt, 0.1f);
            if (blinkTimer_ >= nextBlinkAt_) {
                blinkPhase_ = BlinkPhase::Closing;
                blinkSpeed_ = 1.0f;
                blinkTimer_ = 0.0f;
            }
            break;
        case BlinkPhase::Closing:
            blinkTimer_ += dt;
            eyelidOpen_ = 1.0f - Clamp(blinkTimer_ / closeTime, 0.0f, 1.0f);
            if (blinkTimer_ >= closeTime) {
                blinkPhase_ = BlinkPhase::Closed;
                blinkTimer_ = 0.0f;
            }
            break;
        case BlinkPhase::Closed:
            blinkTimer_ += dt;
            eyelidOpen_ = 0.0f;
            if (blinkTimer_ >= closedHold) {
                blinkPhase_ = BlinkPhase::Opening;
                blinkTimer_ = 0.0f;
            }
            break;
        case BlinkPhase::Opening:
            blinkTimer_ += dt;
            eyelidOpen_ = Clamp(blinkTimer_ / openTime, 0.0f, 1.0f);
            if (blinkTimer_ >= openTime) {
                blinkPhase_ = BlinkPhase::Open;
                blinkTimer_ = 0.0f;
                ScheduleNextBlink();
            }
            break;
    }

    // Sleepiness gradually lowers the resting eyelid position even when
    // "open", giving a droopy look without a dedicated animation.
    if (emotion == Emotion::Sleepy && blinkPhase_ == BlinkPhase::Open) {
        eyelidOpen_ = ApproachExp(eyelidOpen_, 0.55f, dt, 0.8f);
    }
}
