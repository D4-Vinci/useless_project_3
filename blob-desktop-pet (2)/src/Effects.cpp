#include "Effects.h"
#include <cmath>
#include <algorithm>

void EffectsOverlay::Trigger(EffectType type, float duration) {
    // Reuse a slot already playing this effect type, otherwise take any
    // free slot, otherwise steal the most-finished one.
    int bestIdx = -1;
    float bestProgress = -1.0f;
    for (int i = 0; i < kMaxSlots; ++i) {
        if (slots_[i].active && slots_[i].type == type) {
            slots_[i].timer = 0.0f;
            slots_[i].duration = duration;
            return;
        }
        if (!slots_[i].active) {
            bestIdx = i;
            break;
        }
        float progress = slots_[i].duration > 0.0f ? slots_[i].timer / slots_[i].duration : 1.0f;
        if (progress > bestProgress) {
            bestProgress = progress;
            bestIdx = i;
        }
    }
    if (bestIdx >= 0) {
        slots_[bestIdx].active = true;
        slots_[bestIdx].type = type;
        slots_[bestIdx].timer = 0.0f;
        slots_[bestIdx].duration = duration;
    }
}

void EffectsOverlay::Update(float dt) {
    for (auto& s : slots_) {
        if (!s.active) continue;
        s.timer += dt;
        if (s.timer >= s.duration) s.active = false;
    }
}

namespace {

void FillBlock(SDL_Renderer* r, float x, float y, float w, float h, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect rect{static_cast<int>(x), static_cast<int>(y), static_cast<int>(std::ceil(w)) + 1,
                  static_cast<int>(std::ceil(h)) + 1};
    SDL_RenderFillRect(r, &rect);
}

// 5x4 pixel heart, 'F' = filled.
const char* kHeartRows[4] = {
    ".F.F.",
    "FFFFF",
    ".FFF.",
    "..F..",
};

void DrawHeart(SDL_Renderer* renderer, float cx, float cy, float unit, float alpha) {
    SDL_Color c{255, 110, 150, static_cast<Uint8>(alpha * 255)};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 5; ++col) {
            if (kHeartRows[row][col] != 'F') continue;
            float x = cx + (col - 2) * unit;
            float y = cy + (row - 1.5f) * unit;
            FillBlock(renderer, x, y, unit, unit, c);
        }
    }
}

void DrawImpactBurst(SDL_Renderer* renderer, float cx, float cy, float unit, float progress) {
    // Radiating short segments, shrinking and fading as progress -> 1.
    float length = unit * (3.0f - progress * 1.5f);
    Uint8 alpha = static_cast<Uint8>(255.0f * (1.0f - progress));
    SDL_Color c{255, 235, 130, alpha};
    const int spokes = 8;
    for (int i = 0; i < spokes; ++i) {
        float angle = (2.0f * 3.14159265f * i) / spokes;
        float innerR = unit * 1.2f;
        float outerR = innerR + length;
        float x0 = cx + std::cos(angle) * innerR;
        float y0 = cy + std::sin(angle) * innerR;
        float x1 = cx + std::cos(angle) * outerR;
        float y1 = cy + std::sin(angle) * outerR;
        FillBlock(renderer, std::min(x0, x1), std::min(y0, y1), std::max(2.0f, std::abs(x1 - x0)),
                   std::max(2.0f, std::abs(y1 - y0)), c);
    }
}

void DrawDizzyStars(SDL_Renderer* renderer, float cx, float cy, float unit, float t) {
    Uint8 alpha = 235;
    SDL_Color c{255, 224, 90, alpha};
    const int count = 3;
    float radius = unit * 2.4f;
    for (int i = 0; i < count; ++i) {
        float angle = t * 5.5f + (2.0f * 3.14159265f * i) / count;
        float x = cx + std::cos(angle) * radius;
        // Flattened orbit (ellipse) so it reads as circling above the head.
        float y = cy + std::sin(angle) * radius * 0.45f;
        FillBlock(renderer, x - unit * 0.4f, y - unit * 0.4f, unit * 0.8f, unit * 0.8f, c);
    }
}

} // namespace

void EffectsOverlay::Render(SDL_Renderer* renderer, float centerX, float centerY, float topY,
                             float unit) const {
    for (const auto& s : slots_) {
        if (!s.active) continue;
        float progress = s.duration > 0.0f ? s.timer / s.duration : 1.0f;

        switch (s.type) {
            case EffectType::Hearts: {
                // Two hearts drifting up and fading, gently offset so they
                // don't perfectly overlap.
                float alpha = 1.0f - progress;
                float rise = progress * unit * 6.0f;
                DrawHeart(renderer, centerX - unit * 2.5f, topY - unit * 1.5f - rise, unit, alpha);
                DrawHeart(renderer, centerX + unit * 2.2f, topY - unit * 2.5f - rise * 0.8f, unit,
                          alpha * 0.85f);
                break;
            }
            case EffectType::ImpactStars:
                DrawImpactBurst(renderer, centerX, topY, unit, progress);
                break;
            case EffectType::Dizzy:
                DrawDizzyStars(renderer, centerX, topY - unit * 1.0f, unit, s.timer);
                break;
        }
    }
}
