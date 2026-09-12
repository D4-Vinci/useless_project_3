#pragma once
#include "Noise.h"
#include "EmotionSystem.h"
#include <random>

// The eye is animated independently from the body. It owns its own pupil
// target, blink scheduling, and eyelid/shape state so it can react (look
// towards travel direction, glance around, widen in surprise) without the
// body needing to know anything about it.
class Eye {
public:
    explicit Eye(uint32_t seed);

    void Update(float dt, Emotion emotion, float movementDirX, float movementDirY,
                bool isMoving);

    // Occasionally called by BehaviorController to make the eye glance at a
    // specific point before the body commits to moving there, or to look
    // behind itself briefly, etc.
    void LookAt(float dirX, float dirY, float holdSeconds);
    void RequestBlink();          // force a blink soon (e.g. on surprise)
    void RequestLongSleepyBlink();

    float PupilX() const { return pupilX_; }
    float PupilY() const { return pupilY_; }
    float PupilScale() const { return pupilScale_; }
    float EyelidOpen() const { return eyelidOpen_; } // 0 closed .. 1 open
    float EyeWidth() const { return eyeWidth_; }      // shape morph (surprise widens)

private:
    void ScheduleNextBlink();

    std::mt19937 rng_;
    SmoothRandom wanderX_;
    SmoothRandom wanderY_;

    float pupilX_ = 0.0f, pupilY_ = 0.0f;   // current, [-1,1]
    float targetX_ = 0.0f, targetY_ = 0.0f; // desired
    float overrideHold_ = 0.0f;             // seconds remaining on LookAt()
    float pupilScale_ = 1.0f;

    float eyelidOpen_ = 1.0f;
    float eyeWidth_ = 1.0f;

    // Blink state machine.
    enum class BlinkPhase { Open, Closing, Closed, Opening };
    BlinkPhase blinkPhase_ = BlinkPhase::Open;
    float blinkTimer_ = 0.0f;
    float nextBlinkAt_ = 3.0f;
    float blinkSpeed_ = 1.0f; // multiplier; long sleepy blinks are slower
};
