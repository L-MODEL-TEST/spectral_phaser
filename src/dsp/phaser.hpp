#pragma once
#include <array>

#include "AudioFFT.h"
#include "analyze_synthsis_online.hpp"
#include "hann.hpp"


namespace phaser {

class SpectralPhaser {
public:
    static constexpr size_t kFftSize = 1024;
    static constexpr size_t kNumBins = kFftSize / 2 + 1;
    static constexpr size_t kHopSize = 256;

    SpectralPhaser() {
        qwqdsp_window::Hann::Window(hann_window_, true);
    }

    void Init(float fs) {
        fs_ = fs;

        segement_.SetSize(kFftSize);
        segement_.SetHop(kHopSize);
        fft_.init(kFftSize);
    }

    void Process(float* left, float* right, size_t num_samples) noexcept {
        float freq = 440.0f * std::exp2((pitch_ - 69.0f) / 12.0f);
        float bins = freq / fs_ * kFftSize;
        lin_space_ = Warp(bins);

        segement_.Process({left, num_samples}, *this);

        std::copy_n(left, num_samples, right);
    }

    void operator()(std::span<const float> input, std::span<float> output) noexcept {
        for (size_t i = 0; i < kFftSize; ++i) {
            output[i] = input[i] * hann_window_[i];
        }

        fft_.fft(output.data(), re_.data(), im_.data());

        for (size_t i = 0; i < kNumBins; ++i) {
            float g = GetGain(i);
            re_[i] *= g;
            im_[i] *= g;
        }

        fft_.ifft(output.data(), re_.data(), im_.data());
        for (size_t i = 0; i < kFftSize; ++i) {
            output[i] *= hann_window_[i];
        }
    }

    float pitch_{};
    float phase_{};
    float morph_{};
private:
    /**
     * @brief poly sin approximate from reaktor, -110dB 3rd harmonic
     * @note x from 0.5 is sin, 0 is cos
     * @param x [0.0, 1.0]
     */
    static inline constexpr float SinReaktor(float x) noexcept {
        x = 2 * std::abs(x - 0.5f) - 0.5f;
        float const x2 = x * x;
        float u = -0.540347434104161f * x2 + 2.535656174488765f;
        u = u * x2 - 5.166512943349853f;
        u = u * x2 + 3.141592653589793f;
        return u * x;
    }

    float GetGain(size_t i) noexcept {
        float flange_phase = Warp(static_cast<float>(i)) / lin_space_;
        flange_phase += phase_;
        flange_phase -= std::floor(flange_phase);
        return SinReaktor(flange_phase) * 0.5f + 0.5f;
    }

    float Warp(float x) noexcept {
        float lin = x;
        float log = std::log(x + 1);
        return std::lerp(lin, log, morph_);
    }

    // float log_space_{};
    float lin_space_{};
    float fs_{};

    qwqdsp_segement::AnalyzeSynthsisOnline segement_;
    audiofft::AudioFFT fft_;

    std::array<float, kNumBins> re_;
    std::array<float, kNumBins> im_;
    std::array<float, kFftSize> hann_window_;
};

} // namespace phaser
