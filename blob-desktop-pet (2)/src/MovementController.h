#pragma once

// Handles position/velocity integration with smooth acceleration and
// deceleration (critically-damped spring towards a target speed, not
// constant-velocity linear motion). BehaviorController decides *where* the
// blob wants to go; this decides *how* it gets there physically.
class MovementController {
public:
    MovementController();

    void SetPosition(float x, float y);
    void SetDestination(float x, float y, float maxSpeed);
    void ClearDestination();

    // Immediately kicks velocity in a direction (e.g. recoiling from a
    // click) and drops any current destination so the impulse isn't
    // fought — the blob will coast on this velocity and naturally
    // decelerate back towards rest once ClearDestination-like behavior
    // (no target) takes over.
    void Recoil(float dirX, float dirY, float strength);

    // Advances physics by dt seconds.
    void Update(float dt);

    float X() const { return x_; }
    float Y() const { return y_; }
    float VelX() const { return vx_; }
    float VelY() const { return vy_; }
    float Speed() const;

    // 0..1, how close current speed is to this leg's max speed — useful for
    // driving body squash/stretch intensity.
    float SpeedFraction() const;

    bool HasDestination() const { return hasDestination_; }
    bool HasArrived() const { return hasDestination_ && arrived_; }

    float DestX() const { return destX_; }
    float DestY() const { return destY_; }

private:
    float x_ = 0.0f, y_ = 0.0f;
    float vx_ = 0.0f, vy_ = 0.0f;
    float destX_ = 0.0f, destY_ = 0.0f;
    float maxSpeed_ = 40.0f;
    bool hasDestination_ = false;
    bool arrived_ = true;

    // Tunable feel: how quickly velocity approaches its target (seconds).
    static constexpr float kAccelSmoothing = 0.55f;
    static constexpr float kDecelSmoothing = 0.35f;
    static constexpr float kArriveRadius = 3.0f;
};
