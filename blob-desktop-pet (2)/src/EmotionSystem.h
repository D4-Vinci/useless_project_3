#pragma once
#include <random>

enum class Emotion {
    Calm,
    Curious,
    Happy,
    Sleepy,
    Excited,
    Confused,
    Surprised,
    Bored,
};

const char* EmotionName(Emotion e);

// A tiny internal emotional model. Values drift continuously (exponential
// smoothing towards slowly-wandering baselines) rather than snapping, so the
// creature never feels like it flips a mood switch. Other systems (behavior,
// eye, body) read these floats to bias their own decisions.
class EmotionSystem {
public:
    explicit EmotionSystem(uint32_t seed);

    void Update(float dt);

    // Nudges — called by BehaviorController/MovementController when
    // something notable happens (arrived somewhere, sat idle a long time,
    // started moving suddenly, etc). These are gentle pushes, not resets.
    void OnMoved(float speedFraction);
    void OnArrived();
    void OnIdleFor(float seconds);
    void OnSuddenEvent();
    void OnHit(); // clicked/bonked on the head

    Emotion Dominant() const;

    float curiosity = 0.4f;
    float happiness = 0.5f;
    float sleepiness = 0.2f;
    float energy = 0.6f;
    float restlessness = 0.2f;
    float surprise = 0.0f;
    float boredom = 0.1f;
    float confusion = 0.0f;

private:
    std::mt19937 rng_;
    float t_ = 0.0f;
};
