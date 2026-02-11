#pragma once
#include <array>
#include <complex>
#include <random>

#include "AudioFFT.h"
#include "analyze_synthsis_online.hpp"
#include "hann.hpp"

namespace phaser {

class SpectralPhaserLayer {
public:
    void ProcessFft(float* re, float* im, size_t num_bins) noexcept {
        if (!enable) return;

        float dry = 1 - drywet;
        float wet = drywet;
        for (size_t i = 0; i < num_bins; ++i) {
            float g = GetGain(i, phase, space_);
            re[i] = dry * re[i] + wet * g * re[i];
            im[i] = dry * im[i] + wet * g * im[i];
        }
    }

    void Update(float fs, float fft_size, float hop_size) noexcept {
        float freq = 440.0f * std::exp2((pitch - 69.0f) / 12.0f);
        float bins = freq / fs * fft_size;
        space_ = Warp(bins);

        barber_phase_ += barber_freq * hop_size / fs;
        barber_phase_ -= std::floor(barber_phase_);
    }

    float GetLfoPhase() const noexcept {
        return barber_phase_;
    }

    void SetLfoPhase(float p) noexcept {
        barber_phase_ = p;
    }

    bool enable{};
    float pitch{};
    float morph{};
    float phase{};
    float drywet{};
    float barber_freq{};
private:
    float Warp(float x) noexcept {
        float lin = x;
        float log = std::log(x + 1);
        return std::lerp(lin, log, morph);
    }

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

    float GetGain(size_t i, float phi, float cycle) noexcept {
        float flange_phase = Warp(static_cast<float>(i)) / cycle;
        flange_phase += phi;
        flange_phase += barber_phase_;
        flange_phase -= std::floor(flange_phase);
        return SinReaktor(flange_phase) * 0.5f + 0.5f;
    }

    float space_{};
    float barber_phase_{};
};

class SpectralPhaser {
public:
    static constexpr size_t kFftSize = 1024;
    static constexpr size_t kNumBins = kFftSize / 2 + 1;
    static constexpr size_t kHopSize = 256;
    static constexpr size_t kNumLayers = 4;

    SpectralPhaser() {
        qwqdsp_window::Hann::Window(hann_window_, true);

        std::random_device rd{};
        std::mt19937 rng{rd()};
        std::uniform_real_distribution<float> dist(0.0f, std::numbers::pi_v<float>);
        for (size_t i = 0; i < kNumBins; ++i) {
            random_phase_[i] = std::polar(1.0f, dist(rng));
        }
    }

    void Init(float fs) {
        fs_ = fs;

        segement_.SetSize(kFftSize);
        segement_.SetHop(kHopSize);
        fft_.init(kFftSize);
    }

    void Process(float* left, float* right, size_t num_samples) noexcept {
        segement_.Process({left, num_samples}, {right, num_samples}, *this);
    }

    void Update() noexcept {
        for (auto& layer : layers_) {
            layer.Update(fs_, static_cast<float>(kFftSize), kHopSize);
        }
    }

    void operator()(std::span<float const> in_left, std::span<float const> in_right, std::span<float> out_left,
                    std::span<float> out_right) noexcept {
        for (size_t i = 0; i < kFftSize; ++i) {
            out_left[i] = in_left[i] * hann_window_[i];
        }
        for (size_t i = 0; i < kFftSize; ++i) {
            out_right[i] = in_right[i] * hann_window_[i];
        }

        fft_.fft(out_left.data(), re_.data(), im_.data());
        SpectralProcess();
        fft_.ifft(out_left.data(), re_.data(), im_.data());

        fft_.fft(out_right.data(), re_.data(), im_.data());
        SpectralProcess();
        fft_.ifft(out_right.data(), re_.data(), im_.data());

        for (size_t i = 0; i < kFftSize; ++i) {
            out_left[i] *= hann_window_[i];
        }
        for (size_t i = 0; i < kFftSize; ++i) {
            out_right[i] *= hann_window_[i];
        }
    }

    SpectralPhaserLayer& GetLayer(size_t i) noexcept {
        return layers_[i];
    }

    bool phasy{};
private:
    void SpectralProcess() noexcept {
        for (auto& layer : layers_) {
            layer.ProcessFft(re_.data(), im_.data(), kNumBins);
        }

        if (phasy) {
            for (size_t i = 0; i < kNumBins; ++i) {
                std::complex a{re_[i], im_[i]};
                a *= random_phase_[i];
                re_[i] = a.real();
                im_[i] = a.imag();
            }
        }
    }

    float fs_{};
    std::array<SpectralPhaserLayer, kNumLayers> layers_;

    qwqdsp_segement::AnalyzeSynthsisOnline segement_;
    audiofft::AudioFFT fft_;

    std::array<float, kNumBins> re_;
    std::array<float, kNumBins> im_;
    std::array<float, kFftSize> hann_window_;
    std::array<std::complex<float>, kNumBins> random_phase_;
};

} // namespace phaser
