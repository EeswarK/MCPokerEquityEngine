#ifndef ENGINE_SIMD_HELPER_H
#define ENGINE_SIMD_HELPER_H

#include <cstdint>
#include <vector>

#include "core/card.h"

// Check for AVX2 support
#if defined(__x86_64__) || defined(_M_X64)
#if defined(__AVX2__)
#define USE_AVX2 1
#include <immintrin.h>
#endif
#endif

namespace poker_engine {

/**
 * @brief SIMD Batch configuration and helper types.
 */
struct SIMDConfig {
  static constexpr int kBatchSize = 8;  // Logic assumes 8-lane processing
};

/**
 * @brief Structure of Arrays (SoA) for a batch of poker hands.
 * Each hand consists of 7 cards.
 */
struct HandBatch {
  // Aligned to 32 bytes for AVX2 load/store efficiency
  alignas(32) uint32_t ranks[7][SIMDConfig::kBatchSize];
  alignas(32) uint32_t suits[7][SIMDConfig::kBatchSize];
};

/**
 * @brief Helper class for SIMD operations.
 * Provides abstraction for AVX2 and potentially other architectures.
 */
class SIMDHelper {
 public:
  SIMDHelper() = delete;

  /**
   * @brief Checks if AVX2 is supported and enabled in this build.
   */
  static constexpr bool IsAvx2Supported() {
#ifdef USE_AVX2
    return true;
#else
    return false;
#endif
  }

#ifdef USE_AVX2
  /**
   * @brief Loads 8 integers into a 256-bit register.
   */
  static __m256i Load(const uint32_t* data) {
    return _mm256_load_si256(reinterpret_cast<const __m256i*>(data));
  }

  /**
   * @brief Stores a 256-bit register into an array of 8 integers.
   */
  static void Store(uint32_t* dest, __m256i vec) {
    _mm256_store_si256(reinterpret_cast<__m256i*>(dest), vec);
  }
#endif

  /**
   * @brief Aligned allocation helper for SIMD data.
   */
  static void* AlignedAlloc(size_t size) {
#if defined(_MSC_VER)
    return _aligned_malloc(size, 32);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 32, size) != 0) return nullptr;
    return ptr;
#endif
  }

  static void AlignedFree(void* ptr) {
#if defined(_MSC_VER)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
  }
};

}  // namespace poker_engine

#endif  // ENGINE_SIMD_HELPER_H
