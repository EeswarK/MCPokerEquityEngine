#include <gtest/gtest.h>
#include "../engine/simd_helper.h"

TEST(SIMDHelperTest, DetectsSupport) {
    // This test just ensures it compiles and runs without crashing
    bool supported = SIMDHelper::is_avx2_supported();
    // We can't assert true or false universally, but we can print it
    if (supported) {
        SUCCEED() << "AVX2 is supported";
    } else {
        SUCCEED() << "AVX2 is not supported (expected on ARM/non-AVX CPUs)";
    }
}
