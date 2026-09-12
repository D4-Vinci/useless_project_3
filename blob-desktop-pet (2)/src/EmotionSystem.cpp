#include "EmotionSystem.h"
#include "Noise.h"
#include <algorithm>

using mathutil::Clamp;
using mathutil::ApproachExp;

const char* EmotionName(Emotion e) {
    switch (e) {
        case Emotion::Calm: return "calm";
        case Emotion::Curious: return "curious";
        case Emotion::Happy: return "happy";
        case Emotion::Sleepy: return "sleepy";
        case Emotion::Excited: return "excited";
        case Emotion::Confused: return "confused";
        case Emotion::Surprised: return "surprised";
        case Emotion::Bored: return "bored";
    }
    return "calm";
}

EmotionSystem::EmotionSystem(uint32_t seed) : rng_(seed) {}

void EmotionSystem::Update(float dt) {
    t_ += dt;

    // Surprise and confusion decay quickly back to zero — they're spikes,
    // not persistent moods.
    surprise = ApproachExp(surprise, 0.0f, dt, 1.2f);
    confusion = ApproachExp(confusion, 0.0f, dt, 3.0f);

    // Energy slowly bleeds into sleepiness over time, and vice versa when
    // the creature has been resting. This is what eventually makes a long
    // idle period turn into a yawn/sleepy phase, and a long sleepy phase
    // eventually recover.
    sleepiness = Clamp(sleepiness + dt * 0.004f * (1.0f - energy), 0.0f, 1.0f);
    energy = Clamp(energy - dt * 0.0015f, 0.15f, 1.0f);

    // Boredom rises slowly while idle-ish (approximated via low restlessness
    // decay) and is relieved externally by OnMoved/OnArrived.
    boredom = Clamp(boredom + dt * 0.003f, 0.0f, 1.0f);

    // Restlessness drifts back towards a low baseline; OnIdleFor pushes it
    // up when the creature has been still too long.
    restlessness = ApproachExp(restlessness, 0.15f, dt, 6.0f);

    // Happiness gently wanders around a comfortable baseline.
    happiness = ApproachExp(happiness, 0.55f, dt, 8.0f);

    curiosity = Clamp(curiosity, 0.0f, 1.0f);
    happiness = Clamp(happiness, 0.0f, 1.0f);
}

void EmotionSystem::OnMoved(float speedFraction) {
    boredom = Clamp(boredom - 0.08f * speedFraction, 0.0f, 1.0f);
    restlessness = Clamp(restlessness - 0.05f, 0.0f, 1.0f);
    if (speedFraction > 0.7f) {
        energy = Clamp(energy - 0.01f, 0.15f, 1.0f);
    }
}

void EmotionSystem::OnArrived() {
    curiosity = Clamp(curiosity - 0.1f, 0.0f, 1.0f);
    happiness = Clamp(happiness + 0.05f, 0.0f, 1.0f);
}

void EmotionSystem::OnIdleFor(float seconds) {
    if (seconds > 8.0f) {
        restlessness = Clamp(restlessness + 0.002f * (seconds - 8.0f), 0.0f, 1.0f);
        boredom = Clamp(boredom + 0.0015f * (seconds - 8.0f), 0.0f, 1.0f);
    }
    if (seconds > 20.0f) {
        sleepiness = Clamp(sleepiness + 0.002f * (seconds - 20.0f), 0.0f, 1.0f);
    }
}

void EmotionSystem::OnSuddenEvent() {
    surprise = Clamp(surprise + 0.8f, 0.0f, 1.0f);
    sleepiness = Clamp(sleepiness - 0.2f, 0.0f, 1.0f);
}

void EmotionSystem::OnHit() {
    surprise = Clamp(surprise + 0.9f, 0.0f, 1.0f);
    confusion = Clamp(confusion + 0.7f, 0.0f, 1.0f);
    sleepiness = Clamp(sleepiness - 0.35f, 0.0f, 1.0f);
    happiness = Clamp(happiness - 0.15f, 0.0f, 1.0f); // brief dip, recovers on its own
}

Emotion EmotionSystem::Dominant() const {
    if (surprise > 0.55f) return Emotion::Surprised;
    if (confusion > 0.5f) return Emotion::Confused;
    if (sleepiness > 0.65f) return Emotion::Sleepy;
    if (restlessness > 0.7f && energy > 0.5f) return Emotion::Excited;
    if (boredom > 0.7f) return Emotion::Bored;
    if (curiosity > 0.55f) return Emotion::Curious;
    if (happiness > 0.65f) return Emotion::Happy;
    return Emotion::Calm;
}
