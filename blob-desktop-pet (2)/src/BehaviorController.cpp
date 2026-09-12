#include "BehaviorController.h"
#include "Noise.h"
#include <cmath>

using mathutil::RandRange;
using mathutil::RandInt;
using mathutil::WeightedPick;
using mathutil::Clamp;

BehaviorController::BehaviorController(uint32_t seed, MovementController& movement,
                                        Blob& blob, EmotionSystem& emotion,
                                        ScreenInfo& screen)
    : rng_(seed), movement_(movement), blob_(blob), emotion_(emotion), screen_(screen) {
    EnterState(BehaviorState::Idle);
}

MonitorRect BehaviorController::CurrentBounds() const {
    // Margin keeps the blob's ~14*pixelScale sprite comfortably inside the
    // visible area regardless of resolution/DPI.
    int margin = blob_.GridSize() * blob_.PixelScale() * 1 + 20;
    const MonitorRect& m = screen_.MonitorAt(
        static_cast<int>(movement_.X()), static_cast<int>(movement_.Y()));
    return ScreenInfo::Inset(m, margin);
}

void BehaviorController::EnterState(BehaviorState s) {
    state_ = s;
    stateTimer_ = 0.0f;
    lookedBeforeMoving_ = false;

    switch (s) {
        case BehaviorState::Idle:
            stateDuration_ = RandRange(rng_, 2.0f, 6.0f);
            idleSince_ = 0.0f;
            break;
        case BehaviorState::LookingAround:
            stateDuration_ = RandRange(rng_, 0.6f, 1.6f);
            break;
        case BehaviorState::Traveling:
            stateDuration_ = 999.0f; // governed by arrival, not a timer
            break;
        case BehaviorState::Resting:
            stateDuration_ = RandRange(rng_, 5.0f, 12.0f);
            idleSince_ = 0.0f;
            break;
        case BehaviorState::Flinching:
            stateDuration_ = 1.1f; // matches the fixed check in Update()
            break;
    }
}

float BehaviorController::ChooseTravelSpeed() const {
    // Base speed influenced by energy/restlessness/sleepiness — a tired
    // blob ambles, a restless energetic one moves briskly. Occasional
    // "confident" bursts vs "hesitant" slow crawls come from the random
    // multiplier.
    float base = 26.0f + emotion_.energy * 30.0f + emotion_.restlessness * 18.0f;
    base -= emotion_.sleepiness * 20.0f;
    base = Clamp(base, 10.0f, 90.0f);
    float variance = RandRange(rng_, 0.75f, 1.2f);
    return base * variance;
}

void BehaviorController::PickNewDestination() {
    MonitorRect bounds = CurrentBounds();

    // Rare cross-monitor decision: only consider another monitor a small
    // fraction of the time, and only if one exists, so switching screens
    // feels like a deliberate, uncommon choice rather than constant hopping.
    const auto& monitors = screen_.Monitors();
    MonitorRect targetBounds = bounds;
    if (monitors.size() > 1 && RandRange(rng_, 0.0f, 1.0f) < 0.06f) {
        int idx = RandInt(rng_, 0, static_cast<int>(monitors.size()) - 1);
        targetBounds = ScreenInfo::Inset(monitors[static_cast<size_t>(idx)],
                                          blob_.GridSize() * blob_.PixelScale() + 20);
    }

    // Bias away from dead-center to keep compositions visually interesting;
    // pick within an inner region but skew slightly using two samples
    // averaged toward one side chosen at random (cheap way to avoid a
    // uniform-looking distribution clustering in the middle).
    float rx = RandRange(rng_, 0.0f, 1.0f);
    float ry = RandRange(rng_, 0.0f, 1.0f);
    float destX = targetBounds.x + rx * targetBounds.w;
    float destY = targetBounds.y + ry * targetBounds.h;

    float speed = ChooseTravelSpeed();
    movement_.SetDestination(destX, destY, speed);

    // Small chance to glance toward the destination briefly before
    // committing to the journey — a tiny "thinking about it" beat.
    if (RandRange(rng_, 0.0f, 1.0f) < 0.35f) {
        float dx = destX - movement_.X();
        float dy = destY - movement_.Y();
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 1.0f) {
            blob_.EyeRef().LookAt(dx / len, dy / len, RandRange(rng_, 0.3f, 0.7f));
        }
    }
}

void BehaviorController::MaybeChangeMindMidJourney(float dt) {
    if (state_ != BehaviorState::Traveling) return;
    // Low, per-second probability of aborting and choosing a new
    // destination — represents the blob "changing its mind".
    float pChangeMind = 0.02f * emotion_.restlessness * dt;
    if (RandRange(rng_, 0.0f, 1.0f) < pChangeMind) {
        PickNewDestination();
    }
}

void BehaviorController::UpdateIdleBehaviors(float dt) {
    idleSince_ += dt;
    emotion_.OnIdleFor(idleSince_);

    microShuffleCooldown_ -= dt;
    if (microShuffleCooldown_ <= 0.0f && RandRange(rng_, 0.0f, 1.0f) < 0.5f) {
        // A tiny in-place shuffle: a very short hop or a brief glance, not
        // a real journey. Roughly half the time we just reset the cooldown
        // without doing anything visible, so shuffles feel occasional.
        int choice = RandInt(rng_, 0, 2);
        if (choice == 0) {
            blob_.Body().TriggerHop();
        } else if (choice == 1) {
            float dx = RandRange(rng_, -1.0f, 1.0f);
            float dy = RandRange(rng_, -1.0f, 1.0f);
            blob_.EyeRef().LookAt(dx, dy, RandRange(rng_, 0.4f, 1.0f));
        } else if (choice == 2) {
            blob_.EyeRef().RequestBlink();
        }
        microShuffleCooldown_ = RandRange(rng_, 3.0f, 8.0f);
    }
}

bool BehaviorController::IsRestful() const {
    return (state_ == BehaviorState::Idle || state_ == BehaviorState::Resting) &&
           movement_.Speed() < 1.0f;
}

void BehaviorController::SetCursorInfo(float x, float y, bool valid) {
    cursorX_ = x;
    cursorY_ = y;
    cursorValid_ = valid;

    if (!valid || state_ == BehaviorState::Flinching) return;

    // Notice a nearby cursor and occasionally glance toward it — makes the
    // blob feel aware of the mouse without being glued to it. Only glances
    // when not already mid-look-override, and only within a modest radius
    // so it doesn't react to the cursor from across the whole screen.
    float dx = cursorX_ - movement_.X();
    float dy = cursorY_ - movement_.Y();
    float dist = std::sqrt(dx * dx + dy * dy);
    const float noticeRadius = 180.0f;

    if (dist < noticeRadius) {
        emotion_.curiosity = Clamp(emotion_.curiosity + 0.15f * (1.0f - dist / noticeRadius) *
                                        0.02f,
                                    0.0f, 1.0f);
        if (RandRange(rng_, 0.0f, 1.0f) < 0.6f) {
            float len = std::max(1.0f, dist);
            blob_.EyeRef().LookAt(dx / len, dy / len, RandRange(rng_, 0.3f, 0.6f));
        }
    }
}

void BehaviorController::NotifyClick(float clickX, float clickY) {
    if (state_ == BehaviorState::Flinching) return; // already reacting

    float dx = clickX - movement_.X();
    float dy = clickY - movement_.Y();
    float dist = std::sqrt(dx * dx + dy * dy);

    // Hit radius roughly matches the visible body, with a little extra
    // forgiveness so clicking just next to the blob still registers as a
    // "bonk on the head" rather than requiring pixel-perfect precision.
    float hitRadius = blob_.GridSize() * blob_.PixelScale() * 0.65f;
    if (dist > hitRadius) return;

    preFlinchState_ = state_;
    // A small knockback — downward and slightly to a random side, like the
    // blob actually got bonked — which then naturally decelerates back to
    // rest (MovementController::Recoil drops any destination so nothing
    // fights the impulse).
    float sideDir = RandRange(rng_, -1.0f, 1.0f);
    movement_.Recoil(sideDir * 0.4f, 1.0f, RandRange(rng_, 45.0f, 75.0f));
    blob_.TriggerHit();
    emotion_.OnHit();
    EnterState(BehaviorState::Flinching);
}

void BehaviorController::Update(float dt) {
    stateTimer_ += dt;

    switch (state_) {
        case BehaviorState::Idle: {
            UpdateIdleBehaviors(dt);
            if (stateTimer_ >= stateDuration_) {
                // Decide what's next using emotion-weighted probabilities:
                // sleepy -> more likely to Rest; curious/restless -> more
                // likely to start LookingAround for a destination.
                float wLookAround = 0.5f + emotion_.curiosity * 0.8f +
                                     emotion_.restlessness * 0.6f + emotion_.boredom * 0.5f;
                float wRest = 0.25f + emotion_.sleepiness * 1.2f - emotion_.energy * 0.3f;
                float wStayIdle = 0.6f;
                wRest = std::max(0.02f, wRest);
                wStayIdle = std::max(0.02f, wStayIdle);

                float weights[3] = {wLookAround, wRest, wStayIdle};
                int pick = WeightedPick(rng_, weights, 3);
                if (pick == 0) EnterState(BehaviorState::LookingAround);
                else if (pick == 1) EnterState(BehaviorState::Resting);
                else EnterState(BehaviorState::Idle); // re-roll a fresh idle beat
            }
            break;
        }

        case BehaviorState::Resting: {
            UpdateIdleBehaviors(dt);
            if (RandRange(rng_, 0.0f, 1.0f) < 0.15f * dt) {
                blob_.EyeRef().RequestLongSleepyBlink();
            }
            if (stateTimer_ >= stateDuration_) {
                emotion_.sleepiness = Clamp(emotion_.sleepiness - 0.3f, 0.0f, 1.0f);
                EnterState(BehaviorState::Idle);
            }
            break;
        }

        case BehaviorState::LookingAround: {
            // Glance around before deciding — visually communicates
            // "thinking" before the journey starts.
            if (!lookedBeforeMoving_) {
                float dx = RandRange(rng_, -1.0f, 1.0f);
                float dy = RandRange(rng_, -0.6f, 0.6f);
                blob_.EyeRef().LookAt(dx, dy, stateDuration_);
                lookedBeforeMoving_ = true;
            }
            if (stateTimer_ >= stateDuration_) {
                PickNewDestination();
                EnterState(BehaviorState::Traveling);
            }
            break;
        }

        case BehaviorState::Traveling: {
            MaybeChangeMindMidJourney(dt);
            if (movement_.HasArrived()) {
                emotion_.OnArrived();
                // Small chance of a happy little hop on arrival.
                if (RandRange(rng_, 0.0f, 1.0f) < 0.3f) {
                    blob_.Body().TriggerHop();
                }
                movement_.ClearDestination();
                EnterState(BehaviorState::Idle);
            }
            break;
        }

        case BehaviorState::Flinching: {
            // Movement stays cleared/still for the whole reaction — being
            // bonked interrupts whatever it was doing. Once it passes,
            // resume from a fresh Idle beat rather than snapping back into
            // whatever it was mid-way through.
            if (stateTimer_ >= 1.1f) {
                EnterState(BehaviorState::Idle);
            }
            break;
        }
    }

    if (movement_.HasDestination() && !movement_.HasArrived()) {
        emotion_.OnMoved(movement_.SpeedFraction());
    }

    // Random happiness bursts: independent of the main state machine so a
    // little wave of joy can interrupt idling or traveling alike. More
    // likely, and more frequent, the happier the blob currently is.
    happyBurstCooldown_ -= dt;
    if (state_ != BehaviorState::Flinching && happyBurstCooldown_ <= 0.0f) {
        float chance = 0.15f + emotion_.happiness * 0.5f;
        if (RandRange(rng_, 0.0f, 1.0f) < chance) {
            blob_.TriggerHappy();
            emotion_.happiness = Clamp(emotion_.happiness + 0.1f, 0.0f, 1.0f);
        }
        happyBurstCooldown_ = RandRange(rng_, 8.0f, 22.0f) - emotion_.happiness * 6.0f;
        happyBurstCooldown_ = std::max(4.0f, happyBurstCooldown_);
    }

    emotion_.Update(dt);
}
