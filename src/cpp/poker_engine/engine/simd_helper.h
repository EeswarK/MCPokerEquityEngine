#ifndef ENGINE_SIMD_HELPER_H
#define ENGINE_SIMD_HELPER_H

#include <vector>
#include <cstdint>
#include "core/card.h"

// Check for AVX2 support
#if defined(__x86_64__) || defined(_M_X64)
    #if defined(__AVX2__)
        #define USE_AVX2 1
        #include <immintrin.h>
    #endif
#endif

// Helper class for SIMD operations (currently a wrapper)
// In Phase 3, this will be expanded to support batch evaluation logic
// shared across evaluators (like OMPEval).

class SIMDHelper {
public:
    static bool is_avx2_supported() {
#ifdef USE_AVX2
        return true;
#else
        return false;
#endif
    }

    // Placeholder for batch processing
    // Takes a flat vector of cards (N * 7 cards) and output scores vector
    static void evaluate_batch_placeholder(const std::vector<uint8_t>& ranks, 
                                           const std::vector<uint8_t>& suits, 
                                           std::vector<int32_t>& results) {
        // Fallback scalar loop
        // This is where we would use _mm256_load_si256 etc.
    }
};

#endif // ENGINE_SIMD_HELPER_H
