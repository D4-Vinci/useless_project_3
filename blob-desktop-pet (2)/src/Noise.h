#pragma once
#include <cstdint>
#include <cmath>
#include <random>

// Lightweight smooth-random-walk generator.
//
// Rather than a full Perlin/Simplex implementation, this produces a
// continuously varying value in [-1, 1] by interpolating between randomly
// chosen "control points" spaced out in time. It costs almost nothing per
// call (one lerp + smoothstep) and never repeats predictably because a new
// random target is chosen whenever the previous one is reached.
class SmoothRandom {
public:
    explicit SmoothRandom(uint32_t seed, float periodSeconds = 2.5f)
        : rng_(seed), period_(periodSeconds) {
        prev_ = dist_(rng_);
        next_ = dist_(rng_);
    }

    // Advance time and return the current smoothed value in [-1, 1].
    float Update(float dt) {
        t_ += dt;
        if (t_ >= period_) {
            t_ -= period_;
            prev_ = next_;
            next_ = dist_(rng_);
        }
        float f = t_ / period_;
        // smoothstep easing so direction changes are gentle, not linear.
        float eased = f * f * (3.0f - 2.0f * f);
        return prev_ + (next_ - prev_) * eased;
    }

    float Value() const { return prev_; }

    void SetPeriod(float p) { period_ = p > 0.05f ? p : 0.05f; }

private:
    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_{-1.0f, 1.0f};
    float prev_ = 0.0f;
    float next_ = 0.0f;
    float t_ = 0.0f;
    float period_;
};

// Small helpers shared across the behavior/movement/eye systems.
namespace mathutil {

inline float Clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

// Exponential smoothing towards a target — frame-rate independent.
// `smoothing` is a time constant in seconds (smaller = snappier).
inline float ApproachExp(float current, float target, float dt, float smoothing) {
    if (smoothing <= 0.0001f) return target;
    float alpha = 1.0f - std::exp(-dt / smoothing);
    return current + (target - current) * alpha;
}

inline float RandRange(std::mt19937& rng, float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(rng);
}

inline int RandInt(std::mt19937& rng, int lo, int hi) {
    std::uniform_int_distribution<int> d(lo, hi);
    return d(rng);
}

// Weighted pick among N items given a weight array. Returns index.
inline int WeightedPick(std::mt19937& rng, const float* weights, int count) {
    float total = 0.0f;
    for (int i = 0; i < count; ++i) total += weights[i];
    if (total <= 0.0f) return 0;
    float r = RandRange(rng, 0.0f, total);
    float acc = 0.0f;
    for (int i = 0; i < count; ++i) {
        acc += weights[i];
        if (r <= acc) return i;
    }
    return count - 1;
}

} // namespace mathutil
