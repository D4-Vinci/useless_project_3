#pragma once
#include "Noise.h"
#include "EmotionSystem.h"

// Procedural squash/stretch/wiggle for the blob's body. No fixed animation
// loop — everything here is a function of current speed, a hop phase, and
// slow ambient noise, so the exact silhouette never repeats identically.
class BodyAnimation {
public:
    explicit BodyAnimation(uint32_t seed);

    void Update(float dt, float speedFraction, bool isMoving, Emotion emotion);

    // Triggers a discrete hop (used occasionally by BehaviorController).
    void TriggerHop();

    float ScaleX() const { return scaleX_; }
    float ScaleY() const { return scaleY_; }
    float BounceOffsetY() const { return bounceY_; }
    float WiggleOffsetX() const { return wiggleX_; }

private:
    SmoothRandom idleWiggle_;
    SmoothRandom idleBob_;

    float scaleX_ = 1.0f, scaleY_ = 1.0f;
    float bounceY_ = 0.0f;
    float wiggleX_ = 0.0f;

    float hopPhase_ = 0.0f; // 0 = not hopping, ramps 0..1..0
    bool hopping_ = false;

    float bobPhase_ = 0.0f;
};
