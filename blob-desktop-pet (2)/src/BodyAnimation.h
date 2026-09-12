#pragma once
#include "Noise.h"
#include "EmotionSystem.h"

// Procedural squash/stretch/wiggle/jiggle for the blob's body. No fixed
// animation loop — everything here is a function of current speed, a hop
// phase, ambient noise, and short-lived reaction triggers (flinch / happy
// bounce), so the exact silhouette never repeats identically.
class BodyAnimation {
public:
    explicit BodyAnimation(uint32_t seed);

    void Update(float dt, float speedFraction, bool isMoving, Emotion emotion);

    // Triggers a single discrete hop (used occasionally by BehaviorController).
    void TriggerHop();

    // A quick, exaggerated compress-and-overshoot — the "bonk" reaction to
    // being clicked/hit.
    void TriggerFlinch();

    // Schedules a short burst of cheerful little hops.
    void TriggerHappyBounce();

    float ScaleX() const { return scaleX_; }
    float ScaleY() const { return scaleY_; }
    float BounceOffsetY() const { return bounceY_; }
    float WiggleOffsetX() const { return wiggleX_; }

    // Per-row jelly displacement: call with a row index (0..gridSize-1) to
    // get how far that row should shift horizontally this frame, for a
    // wobbling-goo look. Intensity rises briefly after hops/flinches/happy
    // bounces and settles back to a gentle idle jiggle.
    float GooRowOffset(int row, int gridSize) const;

private:
    SmoothRandom idleWiggle_;
    SmoothRandom idleBob_;

    float scaleX_ = 1.0f, scaleY_ = 1.0f;
    float bounceY_ = 0.0f;
    float wiggleX_ = 0.0f;

    float hopPhase_ = 0.0f; // 0 = not hopping, ramps 0..1..0
    bool hopping_ = false;

    float flinchPhase_ = 0.0f;
    bool flinching_ = false;

    int happyHopsRemaining_ = 0;
    float happyHopCooldown_ = 0.0f;

    float bobPhase_ = 0.0f;
    float wobbleTime_ = 0.0f;
    float wobbleIntensity_ = 0.25f; // idle baseline jiggle
};
