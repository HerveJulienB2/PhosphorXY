#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

// Tampon circulaire mono, ecrit par le thread audio et lu par l'interface.
// Les echantillons sont des atomiques "relaxed" : aucun verrou, aucune course de donnees.
class ScopeBuffer
{
public:
    static constexpr int64_t size = (int64_t) 1 << 20;   // ~5,4 s a 192 kHz, ~21 s a 48 kHz
    static constexpr uint64_t mask = (uint64_t) size - 1;

    ScopeBuffer() : data (new std::atomic<float>[(size_t) size])
    {
        for (int64_t i = 0; i < size; ++i)
            data[(size_t) i].store (0.0f, std::memory_order_relaxed);
    }

    void push (const float* src, int n) noexcept
    {
        const auto w = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
            data[(size_t) ((uint64_t) (w + i) & mask)].store (src[i], std::memory_order_relaxed);
        writePos.store (w + n, std::memory_order_release);
    }

    int64_t getWritePos() const noexcept { return writePos.load (std::memory_order_acquire); }

    float at (int64_t i) const noexcept
    {
        return data[(size_t) ((uint64_t) i & mask)].load (std::memory_order_relaxed);
    }

private:
    std::unique_ptr<std::atomic<float>[]> data;
    std::atomic<int64_t> writePos { 0 };
};
