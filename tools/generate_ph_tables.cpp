/**
 * @file generate_ph_tables.cpp
 * @brief Offline generator for PHEvaluator lookup tables
 *
 * This tool generates the three lookup tables used by PHEvaluator:
 * - Flush table: 8,192 entries (32 KB)
 * - Rank table: 50,388 entries (196 KB)
 * - Hash table: 91 entries (364 bytes)
 *
 * Total output: PHRanks.dat (228 KB)
 *
 * Usage: ./generate_ph_tables
 * Output: PHRanks.dat in current directory
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <ctime>

// Include the table generation functions
// We'll compile this with the table generation source
#include "../src/cpp/poker_engine/evaluators/ph_evaluator_tables.h"

using namespace poker_engine;

int main() {
    printf("PHEvaluator Table Generator\n");
    printf("============================\n\n");

    // Allocate tables
    int32_t flush_table[8192];
    int32_t rank_table[50388];
    uint32_t hash_table[7][13];

    printf("Allocating tables...\n");
    printf("  Flush table: %zu bytes\n", sizeof(flush_table));
    printf("  Rank table: %zu bytes\n", sizeof(rank_table));
    printf("  Hash table: %zu bytes\n", sizeof(hash_table));
    printf("  Total: %zu bytes (%.1f KB)\n\n",
           sizeof(flush_table) + sizeof(rank_table) + sizeof(hash_table),
           (sizeof(flush_table) + sizeof(rank_table) + sizeof(hash_table)) / 1024.0);

    // Generate tables
    clock_t start = clock();

    printf("Generating hash table (binomial coefficients)...\n");
    populate_hash_table(hash_table);
    printf("  Done. Hash[0][0]=%u, Hash[6][12]=%u\n", hash_table[0][0], hash_table[6][12]);

    printf("Generating flush table (8,192 entries)...\n");
    populate_flush_table(flush_table);
    printf("  Done. First=%d, Last=%d\n", flush_table[0], flush_table[8191]);

    printf("Generating rank table (50,388 entries)...\n");
    populate_rank_table(rank_table, hash_table);
    printf("  Done. First=%d, Last=%d\n", rank_table[0], rank_table[50387]);

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\nTable generation completed in %.3f seconds\n\n", elapsed);

    // Save to binary file
    const char* filename = "PHRanks.dat";
    printf("Writing tables to %s...\n", filename);

    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "ERROR: Could not open %s for writing\n", filename);
        return 1;
    }

    // Write tables in order: hash, flush, rank
    size_t written = 0;
    written += fwrite(hash_table, sizeof(hash_table), 1, f);
    written += fwrite(flush_table, sizeof(flush_table), 1, f);
    written += fwrite(rank_table, sizeof(rank_table), 1, f);

    fclose(f);

    if (written != 3) {
        fprintf(stderr, "ERROR: Failed to write all tables\n");
        return 1;
    }

    printf("Successfully wrote %s (%.1f KB)\n", filename,
           (sizeof(hash_table) + sizeof(flush_table) + sizeof(rank_table)) / 1024.0);

    // Verification: Print some sample entries
    printf("\nVerification samples:\n");
    printf("  Hash table:\n");
    printf("    C(0,1) = hash[0][0] = %u (expected 1)\n", hash_table[0][0]);
    printf("    C(1,1) = hash[0][1] = %u (expected 1)\n", hash_table[0][1]);
    printf("    C(6,2) = hash[1][5] = %u (expected 15)\n", hash_table[1][5]);
    printf("    C(18,7) = hash[6][12] = %u (expected 31824)\n", hash_table[6][12]);

    printf("  Flush table:\n");
    printf("    mask=0x1F00 (Royal Flush: A-K-Q-J-T) = %d\n", flush_table[0x1F00]);
    printf("    mask=0x1F (Straight Flush: 6-5-4-3-2) = %d\n", flush_table[0x1F]);
    printf("    mask=0x100F (Wheel SF: A-5-4-3-2) = %d\n", flush_table[0x100F]);

    printf("  Rank table:\n");
    printf("    index=0 (seven deuces) = %d\n", rank_table[0]);
    printf("    index=50387 (seven aces) = %d\n", rank_table[50387]);

    printf("\nDone!\n");
    return 0;
}
