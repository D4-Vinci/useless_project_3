#pragma once
#include <SDL2/SDL.h>

enum class EffectType {
    Hearts,      // random happiness bursts
    ImpactStars, // instant "bonk" burst on click
    Dizzy,       // lingering orbiting stars after a hit
};

// Small, cheap, fixed-slot set of short-lived decorative effects drawn
// above/around the blob. Not part of the creature's core state — purely
// visual flair triggered by BehaviorController/Blob in response to events.
class EffectsOverlay {
public:
    void Trigger(EffectType type, float duration);
    void Update(float dt);
    // topY: the y coordinate of the top of the blob's body (effects anchor
    // relative to this). unit: base pixel-block size, matched to the
    // blob's own pixel scale so effects look consistent with its art.
    void Render(SDL_Renderer* renderer, float centerX, float centerY, float topY,
                float unit) const;

private:
    static constexpr int kMaxSlots = 4;
    struct Slot {
        EffectType type = EffectType::Hearts;
        float timer = 0.0f;
        float duration = 0.0f;
        bool active = false;
    };
    Slot slots_[kMaxSlots];
};
