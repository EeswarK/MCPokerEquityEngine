# Track Specification: Modular Hand Evaluators & Multipliers

## Overview
This track introduces a suite of advanced poker hand evaluation algorithms to the C++ engine, each with modular "multipliers" (performance optimizations). The goal is to provide users with a variety of evaluation strategies and the ability to tune performance while demonstrating engineering rigor through benchmarking and technical clarity in the UI.

## Functional Requirements

### C++ Engine (Simulation Core)
- **Implement New Core Algorithms (7-Card Texas Hold'em):**
    - **Cactus Kev (Prime Product):** Classic evaluation using products of prime numbers (iterating 5-card combinations).
    - **PHEvaluator (Rank-Suit Decomp):** Modern, fast evaluation using rank/suit decomposition lookups (7-card native).
    - **Two Plus Two (State Machine):** Extremely fast evaluation using a massive state-transition lookup table (7-card native).
    - **OMPEval (Pure Bit Math):** High-performance bitwise evaluation with inherent SIMD friendliness (7-card native).
- **Implement Modular Multipliers (Optimizations):**
    - **Multithreading:** Parallelize simulations across available CPU cores (supported by all).
    - **SIMD (Single Instruction, Multiple Data):** Process multiple hand evaluations in a single clock cycle (e.g., AVX2).
    - **Perfect Hash:** Optimized lookup for Cactus Kev to replace prime product search.
    - **Prefetching:** Proactively load lookup table data into CPU cache to reduce latency.
- **Validation Logic:** Enforce hardware/algorithmic compatibility (e.g., SIMD and Perfect Hash are mutually exclusive for Cactus Kev).

### Orchestration & API (Python)
- **Typed Schema:** Update `AlgorithmType` enums and `Job` models to support the new algorithms and their specific optimization flags.
- **Validation:** Implement backend validation to ensure that optimization combinations sent from the UI are valid for the selected core algorithm.

### Frontend UI (React)
- **Algorithm Selector:** Use a dropdown or radio group to select the primary Core Algorithm.
- **Multiplier Selection:** Use **Shadcn Checkboxes** for selecting optimizations.
- **Visual Logic:**
    - Disable incompatible checkboxes (e.g., if SIMD is checked for Cactus Kev, the Perfect Hash checkbox becomes disabled).
    - **Tooltips:** Provide concise explanations of implementation strategy and performance gains for each algorithm/multiplier.
    - **Gain Badges:** Display "Estimated Gain" badges (e.g., "[+400%]") next to multipliers.

## Non-Functional Requirements
- **Performance:** Benchmarking suite must be updated to compare these new strategies.
- **Precision:** All evaluators must produce identical results to the `NaiveEvaluator` (gold standard).
- **Type Safety:** Strict enum usage across C++, Python, and TypeScript.

## Acceptance Criteria
1. Users can select from 4 new core algorithms in the UI.
2. Multipliers can be toggled via checkboxes, with mutually exclusive options correctly handled (disabled).
3. Tooltips correctly describe each selection.
4. Simulations run successfully using the selected algorithm/multiplier combination.
5. Benchmark data confirms the relative performance gains of different multipliers.

## Out of Scope
- Supporting legacy CPU instructions (e.g., SSE2); focus on modern AVX2+.
