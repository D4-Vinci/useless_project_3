#include "Blob.h"
#include <cmath>
#include <algorithm>

Blob::Blob(uint32_t seed, int pixelScale)
    : body_(seed), eye_(seed + 999), pixelScale_(pixelScale) {
    BuildShape();
}

// Builds an irregular, rounded goo silhouette by perturbing a circle's
// radius with a few fixed sine harmonics (phase/amplitude chosen by hand
// for a pleasing asymmetric shape — slightly flatter bottom so it looks
// "settled", small bump top-right). Runs once at startup, not per frame.
void Blob::BuildShape() {
    const float cx = (kGridSize - 1) / 2.0f;
    const float cy = (kGridSize - 1) / 2.0f + 0.6f; // slightly low center
    const float baseR = kGridSize * 0.46f;          // big, round, goo-like

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy);
            float angle = std::atan2(dy, dx);

            float r = baseR
                + std::sin(angle * 2.0f + 0.6f) * 1.3f
                + std::sin(angle * 3.0f + 2.1f) * 0.7f
                + std::cos(angle * 1.0f + 1.0f) * 0.9f;

            // Flatten the bottom slightly so it doesn't look like it's
            // floating — subtle "settled" weight.
            if (dy > 0) r -= dy * 0.12f;

            if (dist <= r - 1.3f) {
                shape_[y][x] = 1; // fill
            } else if (dist <= r) {
                shape_[y][x] = 2; // outline
            } else {
                shape_[y][x] = 0;
            }
        }
    }

    // Small highlight patch, upper-left, for a glossy, cute bit of
    // dimensionality.
    int hx = static_cast<int>(cx - baseR * 0.4f);
    int hy = static_cast<int>(cy - baseR * 0.5f);
    for (int y = hy; y < hy + 3 && y < kGridSize; ++y) {
        for (int x = hx; x < hx + 4 && x < kGridSize; ++x) {
            if (y >= 0 && x >= 0 && shape_[y][x] == 1) shape_[y][x] = 3;
        }
    }
}

void Blob::Update(float dt, float speedFraction, bool isMoving, float dirX, float dirY,
                   Emotion emotion) {
    body_.Update(dt, speedFraction, isMoving, emotion);
    eye_.Update(dt, emotion, dirX, dirY, isMoving);
    effects_.Update(dt);
}

void Blob::TriggerHit() {
    body_.TriggerFlinch();
    eye_.RequestFlinch();
    effects_.Trigger(EffectType::ImpactStars, 0.3f);
    effects_.Trigger(EffectType::Dizzy, 1.1f);
}

void Blob::TriggerHappy() {
    body_.TriggerHappyBounce();
    eye_.RequestHappySquint(1.1f);
    effects_.Trigger(EffectType::Hearts, 1.3f);
}

void Blob::Render(SDL_Renderer* renderer, float centerX, float centerY) {
    const float scaleX = body_.ScaleX();
    const float scaleY = body_.ScaleY();
    const float bounceY = body_.BounceOffsetY();
    const float wiggleX = body_.WiggleOffsetX();

    const float px = static_cast<float>(pixelScale_);
    const float gridPxW = kGridSize * px * scaleX;
    const float gridPxH = kGridSize * px * scaleY;
    const float originX = centerX - gridPxW / 2.0f + wiggleX;
    const float originY = centerY - gridPxH / 2.0f + bounceY;

    auto colorFor = [](uint8_t cell, SDL_Color& out) {
        switch (cell) {
            // Soft pastel mint-aqua goo, cuter/rounder palette.
            case 1: out = {132, 224, 184, 255}; break;  // body fill
            case 2: out = {70, 158, 128, 255}; break;   // outline
            case 3: out = {210, 250, 226, 255}; break;  // glossy highlight
            default: out = {0, 0, 0, 0}; break;
        }
    };

    for (int y = 0; y < kGridSize; ++y) {
        // Per-row goo jiggle: each row shifts horizontally by a slightly
        // different phase/amount so the whole silhouette wobbles like
        // soft jelly rather than moving as one rigid block.
        float rowWobble = body_.GooRowOffset(y, kGridSize) * px * 0.35f;

        for (int x = 0; x < kGridSize; ++x) {
            uint8_t cell = shape_[y][x];
            if (cell == 0) continue;
            SDL_Color c;
            colorFor(cell, c);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

            SDL_Rect rect;
            rect.x = static_cast<int>(originX + x * px * scaleX + rowWobble);
            rect.y = static_cast<int>(originY + y * px * scaleY);
            rect.w = static_cast<int>(std::ceil(px * scaleX)) + 1;
            rect.h = static_cast<int>(std::ceil(px * scaleY)) + 1;
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // --- Eye (big, round, cute — sized generously relative to the body) ---
    float eyeCenterX = centerX + wiggleX * 0.3f;
    float eyeCenterY = centerY + bounceY - gridPxH * 0.08f;
    float eyeOuterR = gridPxH * 0.36f * eye_.EyeWidth();
    float eyeOuterRy = eyeOuterR * std::max(0.15f, eye_.EyelidOpen());

    // Eye white (approximate an ellipse with stacked rects for a crisp
    // pixel look rather than SDL_RenderDrawEllipse, which doesn't exist in
    // SDL2).
    SDL_SetRenderDrawColor(renderer, 250, 253, 251, 255);
    int steps = 12;
    for (int i = -steps; i <= steps; ++i) {
        float t = static_cast<float>(i) / steps;
        float rowHalfW = eyeOuterR * std::sqrt(std::max(0.0f, 1.0f - t * t));
        SDL_Rect rowRect;
        rowRect.x = static_cast<int>(eyeCenterX - rowHalfW);
        rowRect.y = static_cast<int>(eyeCenterY + t * eyeOuterRy - 1);
        rowRect.w = static_cast<int>(rowHalfW * 2.0f);
        rowRect.h = 2;
        if (rowRect.w > 0) SDL_RenderFillRect(renderer, &rowRect);
    }

    // Pupil, offset within the eye white by Eye's normalized pupil coords.
    // Slightly oversized (relative to eye) for extra cuteness.
    float pupilR = eyeOuterR * 0.48f * eye_.PupilScale();
    float pupilCx = eyeCenterX + eye_.PupilX() * (eyeOuterR - pupilR) * 0.85f;
    float pupilCy = eyeCenterY + eye_.PupilY() * (eyeOuterRy - pupilR) * 0.65f;

    SDL_SetRenderDrawColor(renderer, 30, 32, 40, 255);
    int pSteps = 7;
    for (int i = -pSteps; i <= pSteps; ++i) {
        float t = static_cast<float>(i) / pSteps;
        float rowHalfW = pupilR * std::sqrt(std::max(0.0f, 1.0f - t * t));
        SDL_Rect rowRect;
        rowRect.x = static_cast<int>(pupilCx - rowHalfW);
        rowRect.y = static_cast<int>(pupilCy + t * pupilR - 1);
        rowRect.w = static_cast<int>(rowHalfW * 2.0f);
        rowRect.h = 2;
        if (rowRect.w > 0) SDL_RenderFillRect(renderer, &rowRect);
    }

    // Tiny catchlight in the pupil — a classic "cute" cue.
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 230);
    SDL_Rect catchlight{static_cast<int>(pupilCx - pupilR * 0.35f - px * 0.15f),
                         static_cast<int>(pupilCy - pupilR * 0.45f - px * 0.15f),
                         static_cast<int>(std::max(2.0f, px * 0.3f)),
                         static_cast<int>(std::max(2.0f, px * 0.3f))};
    SDL_RenderFillRect(renderer, &catchlight);

    // Eyelid: a body-colored rect that closes over the top of the eye based
    // on (1 - EyelidOpen), drawn after the pupil so it occludes cleanly.
    float closedAmount = 1.0f - std::max(0.0f, std::min(1.0f, eye_.EyelidOpen()));
    if (closedAmount > 0.01f) {
        SDL_SetRenderDrawColor(renderer, 132, 224, 184, 255);
        SDL_Rect lid;
        lid.x = static_cast<int>(eyeCenterX - eyeOuterR - 1);
        lid.y = static_cast<int>(eyeCenterY - eyeOuterRy - 1);
        lid.w = static_cast<int>(eyeOuterR * 2.0f + 2);
        lid.h = static_cast<int>(eyeOuterRy * 2.0f * closedAmount + 1);
        SDL_RenderFillRect(renderer, &lid);
    }

    // --- Blush cheeks: a small, soft cuteness cue below/either side of the
    // eye. Always present but faint; effects/emotion could brighten these
    // further later without changing the render structure. ---
    SDL_SetRenderDrawColor(renderer, 255, 158, 176, 90);
    float blushR = eyeOuterR * 0.5f;
    float blushY = eyeCenterY + eyeOuterRy * 1.3f;
    SDL_Rect blushL{static_cast<int>(eyeCenterX - eyeOuterR * 2.1f - blushR),
                     static_cast<int>(blushY - blushR * 0.5f), static_cast<int>(blushR * 1.6f),
                     static_cast<int>(blushR)};
    SDL_Rect blushR2{static_cast<int>(eyeCenterX + eyeOuterR * 1.3f),
                      static_cast<int>(blushY - blushR * 0.5f), static_cast<int>(blushR * 1.6f),
                      static_cast<int>(blushR)};
    SDL_RenderFillRect(renderer, &blushL);
    SDL_RenderFillRect(renderer, &blushR2);

    // --- Reaction effects (hearts / impact burst / dizzy stars) drawn
    // above the head, anchored to the top of the current (wobbly) body. ---
    float topY = originY - px * 0.5f;
    effects_.Render(renderer, centerX + wiggleX * 0.3f, centerY, topY, px * 0.7f);
}
