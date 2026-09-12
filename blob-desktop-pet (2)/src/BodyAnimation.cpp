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

void BodyAnimation::TriggerFlinch() {
    flinching_ = true;
    flinchPhase_ = 0.0f;
    wobbleIntensity_ = 1.6f; // big jelly jiggle right after a bonk
}

void BodyAnimation::TriggerHappyBounce() {
    happyHopsRemaining_ = 3;
    happyHopCooldown_ = 0.0f;
    wobbleIntensity_ = std::max(wobbleIntensity_, 0.9f);
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

    // Happy-bounce scheduling: fire off a few quick hops in a row.
    happyHopCooldown_ -= dt;
    if (happyHopsRemaining_ > 0 && !hopping_ && happyHopCooldown_ <= 0.0f) {
        TriggerHop();
        happyHopsRemaining_--;
        happyHopCooldown_ = 0.16f;
    }

    // Flinch: a sharp compress followed by a springy overshoot recovery —
    // reads as "ouch" rather than the gentler arrival hop.
    float flinchScaleX = 1.0f, flinchScaleY = 1.0f;
    if (flinching_) {
        flinchPhase_ += dt / 0.45f;
        if (flinchPhase_ >= 1.0f) {
            flinchPhase_ = 1.0f;
            flinching_ = false;
        }
        // Damped oscillation: quick squash then a couple of decaying
        // overshoots.
        float decay = std::exp(-flinchPhase_ * 5.0f);
        float osc = std::cos(flinchPhase_ * 3.14159265f * 3.2f);
        float squash = decay * osc;
        flinchScaleY = 1.0f - squash * 0.30f;
        flinchScaleX = 1.0f + squash * 0.22f;
    }

    scaleX_ = ApproachExp(scaleX_, targetScaleX * flinchScaleX, dt, 0.14f);
    scaleY_ = ApproachExp(scaleY_, targetScaleY * flinchScaleY, dt, 0.14f);

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

    // Goo jiggle intensity decays back down to a gentle idle baseline.
    wobbleTime_ += dt;
    wobbleIntensity_ = ApproachExp(wobbleIntensity_, 0.25f, dt, 0.9f);
}

float BodyAnimation::GooRowOffset(int row, int gridSize) const {
    if (gridSize <= 1) return 0.0f;
    float t = static_cast<float>(row) / static_cast<float>(gridSize - 1); // 0 (top) .. 1 (bottom)
    // Bottom rows jiggle more than the top, like a soft-bodied creature
    // whose base wobbles more freely than its "head".
    float weight = 0.35f + t * 0.9f;
    float phase = wobbleTime_ * 5.2f + t * 6.0f;
    return std::sin(phase) * wobbleIntensity_ * weight;
}
