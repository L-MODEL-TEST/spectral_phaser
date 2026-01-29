#pragma once
#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

namespace qwqdsp_segement {
class AnalyzeSynthsisOnline {
public:
    /**
     * @tparam Func void(std::span<float const> left, std::span<float const> right, std::span<float> left_out,
     *                   std::span<float> left_right)
     */
    template <class Func>
    void Process(std::span<float> left_block, std::span<float> right_block, Func&& func) noexcept(
        noexcept(func(std::declval<std::span<float const>>(), std::declval<std::span<float const>>(),
                      std::declval<std::span<float>>(), std::declval<std::span<float>>()))) {
        size_t in_wrpos = 0;
        size_t const in_size = left_block.size();

        while (in_wrpos != in_size) {
            size_t need = size_ - input_wpos_;
            size_t can_read = std::min(need, in_size - in_wrpos);
            std::copy_n(left_block.begin() + static_cast<int>(in_wrpos), can_read,
                        input_buffer_left_.begin() + static_cast<int>(input_wpos_));
            std::copy_n(right_block.begin() + static_cast<int>(in_wrpos), can_read,
                        input_buffer_right_.begin() + static_cast<int>(input_wpos_));
            input_wpos_ += can_read;

            if (input_wpos_ >= size_) {
                func(std::span<float const>{input_buffer_left_.data(), size_},
                     std::span<float const>{input_buffer_right_.data(), size_},
                     std::span<float>{process_buffer_left_.data(), size_},
                     std::span<float>{process_buffer_right_.data(), size_});

                input_wpos_ -= hop_;
                for (size_t i = 0; i < input_wpos_; i++) {
                    input_buffer_left_[i] = input_buffer_left_[i + hop_];
                }
                for (size_t i = 0; i < input_wpos_; i++) {
                    input_buffer_right_[i] = input_buffer_right_[i + hop_];
                }

                for (size_t i = 0; i < size_; i++) {
                    output_buffer_left_[i + write_add_end_] += process_buffer_left_[i];
                }
                for (size_t i = 0; i < size_; i++) {
                    output_buffer_right_[i + write_add_end_] += process_buffer_right_[i];
                }
                write_add_end_ += hop_;
                write_end_ = write_add_end_ + size_;
            }

            if (write_add_end_ >= can_read) {
                // extract output
                size_t extractSize = can_read;
                for (size_t i = 0; i < extractSize; ++i) {
                    left_block[i + in_wrpos] = output_buffer_left_[i];
                    output_buffer_left_[i] = 0;
                }
                for (size_t i = 0; i < extractSize; ++i) {
                    right_block[i + in_wrpos] = output_buffer_right_[i];
                    output_buffer_right_[i] = 0;
                }

                // shift output buffer
                size_t shiftSize = write_end_ - extractSize;
                for (size_t i = 0; i < shiftSize; i++) {
                    output_buffer_left_[i] = output_buffer_left_[i + extractSize];
                }
                for (size_t i = 0; i < shiftSize; i++) {
                    output_buffer_right_[i] = output_buffer_right_[i + extractSize];
                }
                write_add_end_ -= extractSize;
                write_end_ = write_add_end_ + size_;
            }
            else {
                // zero buffer
                std::fill_n(left_block.begin() + static_cast<int>(in_wrpos), can_read, 0.0f);
                std::fill_n(right_block.begin() + static_cast<int>(in_wrpos), can_read, 0.0f);
            }

            in_wrpos += can_read;
        }
    }

    void SetSize(size_t size) noexcept {
        size_ = size;
        if (input_buffer_left_.size() < size) {
            input_buffer_left_.resize(size);
            input_buffer_right_.resize(size);
        }
        if (output_buffer_left_.size() < (size + hop_) * 2) {
            output_buffer_left_.resize((size + hop_) * 2);
            output_buffer_right_.resize((size + hop_) * 2);
        }
        if (process_buffer_left_.size() < size) {
            process_buffer_left_.resize(size);
            process_buffer_right_.resize(size);
        }
    }

    void SetHop(size_t hop) noexcept {
        hop_ = hop;
    }

    void Reset() noexcept {
        std::fill_n(output_buffer_left_.begin(), write_end_, 0.0f);
        std::fill_n(output_buffer_right_.begin(), write_end_, 0.0f);
        input_wpos_ = 0;
        write_end_ = 0;
        write_add_end_ = 0;
    }
private:
    std::vector<float> input_buffer_left_;
    std::vector<float> input_buffer_right_;
    std::vector<float> process_buffer_left_;
    std::vector<float> process_buffer_right_;
    std::vector<float> output_buffer_left_;
    std::vector<float> output_buffer_right_;
    size_t size_{};
    size_t hop_{};
    size_t input_wpos_{};
    size_t write_end_{};
    size_t write_add_end_{};
};
} // namespace qwqdsp_segement
