#include "Blob.h"
#include <cmath>
#include <algorithm>

Blob::Blob(uint32_t seed, int pixelScale)
    : body_(seed), eye_(seed + 999), pixelScale_(pixelScale) {
    BuildShape();
}

// Builds an irregular blob silhouette by perturbing a circle's radius with
// a few fixed sine harmonics (phase/amplitude chosen by hand for a pleasing
// asymmetric shape — slightly flatter bottom, small bump top-right). This
// runs once at startup, not per frame.
void Blob::BuildShape() {
    const float cx = (kGridSize - 1) / 2.0f;
    const float cy = (kGridSize - 1) / 2.0f + 0.4f; // slightly low center
    const float baseR = kGridSize * 0.40f;

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            float dx = x - cx;
            float dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy);
            float angle = std::atan2(dy, dx);

            float r = baseR
                + std::sin(angle * 2.0f + 0.6f) * 1.1f
                + std::sin(angle * 3.0f + 2.1f) * 0.6f
                + std::cos(angle * 1.0f + 1.0f) * 0.8f;

            // Flatten the bottom slightly so it doesn't look like it's
            // floating — subtle "settled" weight.
            if (dy > 0) r -= dy * 0.15f;

            if (dist <= r - 1.1f) {
                shape_[y][x] = 1; // fill
            } else if (dist <= r) {
                shape_[y][x] = 2; // outline
            } else {
                shape_[y][x] = 0;
            }
        }
    }

    // Small highlight patch, upper-left, for a bit of dimensionality.
    int hx = static_cast<int>(cx - baseR * 0.35f);
    int hy = static_cast<int>(cy - baseR * 0.45f);
    for (int y = hy; y < hy + 2 && y < kGridSize; ++y) {
        for (int x = hx; x < hx + 3 && x < kGridSize; ++x) {
            if (y >= 0 && x >= 0 && shape_[y][x] == 1) shape_[y][x] = 3;
        }
    }
}

void Blob::Update(float dt, float speedFraction, bool isMoving, float dirX, float dirY,
                   Emotion emotion) {
    body_.Update(dt, speedFraction, isMoving, emotion);
    eye_.Update(dt, emotion, dirX, dirY, isMoving);

    // Emotion-driven eye behaviors the Blob itself is responsible for
    // triggering opportunistically (BehaviorController handles the
    // higher-level "look somewhere before moving" choreography).
    if (emotion == Emotion::Surprised) {
        // handled via RequestBlink from BehaviorController on the event
    }
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
            case 1: out = {96, 209, 148, 255}; break;   // body fill (soft green)
            case 2: out = {46, 128, 92, 255}; break;    // outline (darker edge)
            case 3: out = {158, 236, 191, 255}; break;  // highlight
            default: out = {0, 0, 0, 0}; break;
        }
    };

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            uint8_t cell = shape_[y][x];
            if (cell == 0) continue;
            SDL_Color c;
            colorFor(cell, c);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);

            SDL_Rect rect;
            rect.x = static_cast<int>(originX + x * px * scaleX);
            rect.y = static_cast<int>(originY + y * px * scaleY);
            rect.w = static_cast<int>(std::ceil(px * scaleX)) + 1;
            rect.h = static_cast<int>(std::ceil(px * scaleY)) + 1;
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // --- Eye ---
    // Placed slightly above body center, size relative to grid.
    float eyeCenterX = centerX + wiggleX * 0.3f;
    float eyeCenterY = centerY + bounceY - gridPxH * 0.06f;
    float eyeOuterR = gridPxH * 0.30f * eye_.EyeWidth();
    float eyeOuterRy = eyeOuterR * std::max(0.15f, eye_.EyelidOpen());

    // Eye white (approximate an ellipse with stacked rects for crisp pixel
    // look rather than SDL_RenderDrawEllipse, which doesn't exist in SDL2).
    SDL_SetRenderDrawColor(renderer, 245, 250, 248, 255);
    int steps = 10;
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
    float pupilR = eyeOuterR * 0.42f * eye_.PupilScale();
    float pupilCx = eyeCenterX + eye_.PupilX() * (eyeOuterR - pupilR) * 0.9f;
    float pupilCy = eyeCenterY + eye_.PupilY() * (eyeOuterRy - pupilR) * 0.7f;

    SDL_SetRenderDrawColor(renderer, 28, 30, 36, 255);
    int pSteps = 6;
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

    // Eyelid: a body-colored rect that closes over the top of the eye based
    // on (1 - EyelidOpen), drawn last so it occludes white+pupil smoothly.
    float closedAmount = 1.0f - std::max(0.0f, std::min(1.0f, eye_.EyelidOpen()));
    if (closedAmount > 0.01f) {
        SDL_SetRenderDrawColor(renderer, 96, 209, 148, 255);
        SDL_Rect lid;
        lid.x = static_cast<int>(eyeCenterX - eyeOuterR - 1);
        lid.y = static_cast<int>(eyeCenterY - eyeOuterRy - 1);
        lid.w = static_cast<int>(eyeOuterR * 2.0f + 2);
        lid.h = static_cast<int>(eyeOuterRy * 2.0f * closedAmount + 1);
        SDL_RenderFillRect(renderer, &lid);
    }
}
