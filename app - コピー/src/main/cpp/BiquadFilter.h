#pragma once

class BiquadFilter {
public:
    void reset() noexcept {
        z1_ = 0.0f;
        z2_ = 0.0f;
    }

    void setCoefficients(
            float b0,
            float b1,
            float b2,
            float a1,
            float a2
    ) noexcept {
        b0_ = b0;
        b1_ = b1;
        b2_ = b2;
        a1_ = a1;
        a2_ = a2;
    }

    float process(float input) noexcept {
        const float output = b0_ * input + z1_;
        z1_ = b1_ * input - a1_ * output + z2_;
        z2_ = b2_ * input - a2_ * output;
        return output;
    }

private:
    float b0_ = 1.0f;
    float b1_ = 0.0f;
    float b2_ = 0.0f;
    float a1_ = 0.0f;
    float a2_ = 0.0f;
    float z1_ = 0.0f;
    float z2_ = 0.0f;
};
