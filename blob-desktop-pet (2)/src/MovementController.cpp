#include "MovementController.h"
#include "Noise.h"
#include <cmath>

using mathutil::ApproachExp;

MovementController::MovementController() = default;

void MovementController::SetPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void MovementController::SetDestination(float x, float y, float maxSpeed) {
    destX_ = x;
    destY_ = y;
    maxSpeed_ = maxSpeed;
    hasDestination_ = true;
    arrived_ = false;
}

void MovementController::ClearDestination() {
    hasDestination_ = false;
    arrived_ = true;
}

void MovementController::Recoil(float dirX, float dirY, float strength) {
    vx_ = dirX * strength;
    vy_ = dirY * strength;
    hasDestination_ = false;
    arrived_ = true;
}

void MovementController::Update(float dt) {
    if (dt <= 0.0f) return;

    float targetVx = 0.0f;
    float targetVy = 0.0f;

    if (hasDestination_ && !arrived_) {
        float dx = destX_ - x_;
        float dy = destY_ - y_;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= kArriveRadius) {
            arrived_ = true;
        } else {
            // Ease speed down as we approach so the stop is gentle rather
            // than an abrupt cut. The easing radius scales with max speed
            // so fast legs still get a graceful deceleration.
            float easeRadius = std::max(24.0f, maxSpeed_ * 1.5f);
            float speedScale = dist < easeRadius ? (dist / easeRadius) : 1.0f;
            speedScale = 0.15f + 0.85f * speedScale; // never fully stall
            float desiredSpeed = maxSpeed_ * speedScale;

            float invDist = 1.0f / dist;
            targetVx = dx * invDist * desiredSpeed;
            targetVy = dy * invDist * desiredSpeed;
        }
    }

    // Critically-damped-feeling approach: accelerating is a bit slower than
    // decelerating, so stops feel more decisive than starts.
    bool speedingUp = (targetVx * targetVx + targetVy * targetVy) >
                       (vx_ * vx_ + vy_ * vy_);
    float smoothing = speedingUp ? kAccelSmoothing : kDecelSmoothing;

    vx_ = ApproachExp(vx_, targetVx, dt, smoothing);
    vy_ = ApproachExp(vy_, targetVy, dt, smoothing);

    x_ += vx_ * dt;
    y_ += vy_ * dt;
}

float MovementController::Speed() const {
    return std::sqrt(vx_ * vx_ + vy_ * vy_);
}

float MovementController::SpeedFraction() const {
    if (maxSpeed_ <= 0.001f) return 0.0f;
    return std::min(1.0f, Speed() / maxSpeed_);
}
