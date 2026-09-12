#pragma once
#include "BodyAnimation.h"
#include "Eye.h"
#include "EmotionSystem.h"
#include "Effects.h"
#include <SDL2/SDL.h>

// The visible creature: owns body animation + eye + transient effects, and
// knows how to draw itself as crisp, cute pixel-art goo (integer-scaled
// blocks, no smoothing) centered on a given screen position.
class Blob {
public:
    explicit Blob(uint32_t seed, int pixelScale = 8);

    void Update(float dt, float speedFraction, bool isMoving, float dirX, float dirY,
                Emotion emotion);

    void Render(SDL_Renderer* renderer, float centerX, float centerY);

    BodyAnimation& Body() { return body_; }
    Eye& EyeRef() { return eye_; }

    // Reaction triggers — combine body + eye + effect for a single
    // readable beat, so callers (BehaviorController) don't have to
    // choreograph each sub-system by hand.
    void TriggerHit();   // "bonk on the head" — click/mouse reaction
    void TriggerHappy(); // random happiness burst

    int PixelScale() const { return pixelScale_; }
    int GridSize() const { return kGridSize; }

private:
    static constexpr int kGridSize = 20;
    // 0 = empty, 1 = body fill, 2 = outline, 3 = highlight. Built once in
    // the constructor via a perturbed-radius formula so the silhouette is
    // an irregular, organic blob rather than a perfect circle.
    uint8_t shape_[kGridSize][kGridSize] = {};
    void BuildShape();

    BodyAnimation body_;
    Eye eye_;
    EffectsOverlay effects_;
    int pixelScale_;
};
