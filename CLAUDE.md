# Claude Memory

This file contains important context and decisions for the project.

## Build System Notes

### HandRanks.dat Preservation
**IMPORTANT**: When cleaning the build directory with `rm -rf build/*`, the TwoPlusTwoEvaluator's `HandRanks.dat` file is deleted (it's generated into build/). Solutions:
- Place HandRanks.dat in parent directory or resources folder
- Use `make clean` instead of `rm -rf build/*`
- Regenerate with `tools/generate_table` after cleaning

### PHEvaluator Implementation (2026-01-26)
- Replaced pattern-matching implementation with true PHEvaluator algorithm
- Uses pre-computed lookup tables (228 KB total):
  - Flush table: 8,192 entries (32 KB)
  - Rank table: 50,388 entries (196 KB)
  - Hash table: 91 entries (364 bytes)
- Tables generated at runtime on first use (~80ms one-time cost)
- Static tables shared across all PHEvaluator instances
- Expected performance: 5-8M hands/sec (3-5x faster than pattern matching)

## Scoring System

All evaluators must use the **unified scoring format** defined in `hand_types.h`:
```
score = HandType * 1,000,000 + base15(ranks)
```

Where base15(ranks) encodes the key ranks as: `r0*15^4 + r1*15^3 + r2*15^2 + r3*15 + r4`

Example: Pair of Aces with K-Q-J kickers = 1,000,000 + (14*50625 + 14*3375 + 13*225 + 12*15 + 11) = 1,759,686

## Project Structure

- `src/cpp/poker_engine/` - Core C++ poker engine
- `src/frontend/` - React/TypeScript frontend
- `src/python/` - Python bindings and utilities
- `tools/` - Build utilities (table generators, etc.)
