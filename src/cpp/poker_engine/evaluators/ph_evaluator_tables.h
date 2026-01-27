#ifndef EVALUATORS_PH_EVALUATOR_TABLES_H
#define EVALUATORS_PH_EVALUATOR_TABLES_H

#include <cstdint>

namespace poker_engine {

// Binomial coefficient calculation: C(n, k)
uint32_t binomial_coefficient(uint32_t n, uint32_t k);

// Populate the hash table for combinatorial indexing
// hash[i][j] = C(j + i, i + 1)
void populate_hash_table(uint32_t hash_table[7][13]);

// Populate flush table: 8192 entries for all 13-bit rank masks
// Maps flush patterns to hand scores (includes straight flushes and royal flush)
void populate_flush_table(int32_t flush_table[8192]);

// Populate rank table: 50,388 entries for all 7-card non-flush combinations
// Uses colexicographical enumeration of multisets
void populate_rank_table(int32_t rank_table[50388], const uint32_t hash_table[7][13]);

}  // namespace poker_engine

#endif  // EVALUATORS_PH_EVALUATOR_TABLES_H
