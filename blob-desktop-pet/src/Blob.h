#pragma once
#include "BodyAnimation.h"
#include "Eye.h"
#include "EmotionSystem.h"
#include <SDL2/SDL.h>

// The visible creature: owns body animation + eye, and knows how to draw
// itself as crisp pixel art (integer-scaled blocks, no smoothing) centered
// on a given screen position.
class Blob {
public:
    explicit Blob(uint32_t seed, int pixelScale = 6);

    void Update(float dt, float speedFraction, bool isMoving, float dirX, float dirY,
                Emotion emotion);

    void Render(SDL_Renderer* renderer, float centerX, float centerY);

    BodyAnimation& Body() { return body_; }
    Eye& EyeRef() { return eye_; }

    int PixelScale() const { return pixelScale_; }
    int GridSize() const { return kGridSize; }

private:
    static constexpr int kGridSize = 14;
    // 0 = empty, 1 = body fill, 2 = outline, 3 = highlight. Built once in
    // the constructor via a perturbed-radius formula so the silhouette is
    // an irregular, organic blob rather than a perfect circle.
    uint8_t shape_[kGridSize][kGridSize] = {};
    void BuildShape();

    BodyAnimation body_;
    Eye eye_;
    int pixelScale_;
};
