#include "BodyAnimation.h"
#include <cmath>

using mathutil::ApproachExp;
using mathutil::Clamp;

BodyAnimation::BodyAnimation(uint32_t seed)
    : idleWiggle_(seed + 11, 4.0f), idleBob_(seed + 22, 3.3f) {}

void BodyAnimation::TriggerHop() {
    hopping_ = true;
    hopPhase_ = 0.0f;
}

void BodyAnimation::Update(float dt, float speedFraction, bool isMoving, Emotion emotion) {
    // Idle life: slow bob + subtle horizontal wiggle so the blob is never
    // perfectly frozen even at rest.
    bobPhase_ += dt;
    float idleBobY = idleBob_.Update(dt) * 1.2f;
    wiggleX_ = ApproachExp(wiggleX_, idleWiggle_.Update(dt) * 0.6f, dt, 0.4f);

    // Movement squash/stretch: stretched along travel, slightly squashed
    // perpendicular, intensity scaling with speed.
    float targetScaleX = 1.0f;
    float targetScaleY = 1.0f;
    if (isMoving) {
        float stretch = 0.12f * speedFraction;
        targetScaleX = 1.0f + stretch;
        targetScaleY = 1.0f - stretch * 0.6f;
    }

    // Emotion flavor: excited creatures are a touch bouncier/rounder,
    // sleepy ones sag slightly.
    if (emotion == Emotion::Sleepy) {
        targetScaleY *= 0.94f;
        targetScaleX *= 1.03f;
    } else if (emotion == Emotion::Excited) {
        targetScaleY *= 1.03f;
    }

    scaleX_ = ApproachExp(scaleX_, targetScaleX, dt, 0.18f);
    scaleY_ = ApproachExp(scaleY_, targetScaleY, dt, 0.18f);

    // Hop: a quick vertical arc, independent of walking, used sparingly.
    float hopOffset = 0.0f;
    if (hopping_) {
        hopPhase_ += dt / 0.35f; // ~0.35s hop duration
        if (hopPhase_ >= 1.0f) {
            hopPhase_ = 1.0f;
            hopping_ = false;
        }
        hopOffset = -std::sin(hopPhase_ * 3.14159265f) * 6.0f;
    }

    bounceY_ = idleBobY * (isMoving ? 0.3f : 1.0f) + hopOffset;
}
