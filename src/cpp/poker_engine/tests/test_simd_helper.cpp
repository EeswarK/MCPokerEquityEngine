#include <gtest/gtest.h>
#include "../engine/simd_helper.h"
#include <cstdint>

namespace poker_engine {

TEST(SIMDHelperTest, HandBatchAlignment) {
  HandBatch batch;
  // Check if the arrays are actually 32-byte aligned as requested
  uintptr_t ranks_addr = reinterpret_cast<uintptr_t>(&batch.ranks[0][0]);
  uintptr_t suits_addr = reinterpret_cast<uintptr_t>(&batch.suits[0][0]);
  
  EXPECT_EQ(ranks_addr % 32, 0);
  EXPECT_EQ(suits_addr % 32, 0);
}

TEST(SIMDHelperTest, AlignedAlloc) {
  void* ptr = SIMDHelper::AlignedAlloc(1024);
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 32, 0);
  
  // Fill with data to ensure it's writable
  uint32_t* u32_ptr = static_cast<uint32_t*>(ptr);
  for (int i = 0; i < 256; ++i) {
    u32_ptr[i] = i;
  }
  
  SIMDHelper::AlignedFree(ptr);
}

TEST(SIMDHelperTest, ArchitectureDetection) {
#if defined(__aarch64__) || defined(__arm64__)
  EXPECT_FALSE(SIMDHelper::IsAvx2Supported());
  std::cout << "[ INFO ] Running on ARM64 - SIMD paths disabled as expected." << std::endl;
#elif defined(USE_AVX2)
  EXPECT_TRUE(SIMDHelper::IsAvx2Supported());
  std::cout << "[ INFO ] AVX2 Support detected and enabled." << std::endl;
#else
  std::cout << "[ INFO ] No SIMD architecture detected or enabled." << std::endl;
#endif
}

} // namespace poker_engine