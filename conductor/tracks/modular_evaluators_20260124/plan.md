# Implementation Plan: Modular Hand Evaluators & Multipliers

## Phase 1: Foundation & Data Models [checkpoint: 70175b8]
- [x] Task: Update Shared C++ Types f1f7a43
    - [x] Add `EvaluatorType` enum to `src/cpp/poker_engine/evaluators/hand_types.h`.
    - [x] Add `OptimizationFlags` bitmask or struct to support SIMD, Perfect Hash, etc.
- [x] Task: Update Python Data Models f1f7a43
    - [x] Update `AlgorithmType` in `src/python/api/models.py`.
    - [x] Add `optimizations` field to the `JobRequest` model.
- [x] Task: Update FlatBuffers Schema 03f4778
    - [x] Modify `telemetry.fbs` (if used for job config) or relevant JSON utils to handle new types.
- [x] Task: Conductor - User Manual Verification 'Phase 1: Foundation & Data Models' (Protocol in workflow.md)

## Phase 2: C++ Engine - Core Algorithms
- [x] Task: Implement Cactus Kev Evaluator (7-card) bff3550
    - [x] Port/implement prime product logic.
    - [x] Implement Perfect Hash multiplier.
- [x] Task: Implement PHEvaluator (7-card) 6d08e88
    - [x] Implement rank-suit decomposition logic.
    - [x] Implement prefetching multiplier.
- [x] Task: Implement Two Plus Two Evaluator (7-card) 95f04a7
    - [x] Implement state machine lookup logic.
    - [x] Implement prefetching multiplier.
- [x] Task: Implement OMPEval (7-card) 12fbede
    - [x] Implement bit manipulation logic.
    - [x] Ensure SIMD-ready structure.
- [ ] Task: Conductor - User Manual Verification 'Phase 2: C++ Engine - Core Algorithms' (Protocol in workflow.md)

## Phase 3: C++ Engine - Global Optimizations
- [ ] Task: Implement SIMD Framework
    - [ ] Add AVX2/SIMD helper abstractions for batch evaluation.
- [ ] Task: Implement Multithreading Orchestration
    - [ ] Update `EquityEngine` to dispatch work based on the selected algorithm and thread count.
- [ ] Task: Engine Validation & Benchmarking
    - [ ] Write GTest unit tests to ensure all evaluators match `NaiveEvaluator` results.
    - [ ] Add benchmark cases to measure multiplier efficiency.
- [ ] Task: Conductor - User Manual Verification 'Phase 3: C++ Engine - Global Optimizations' (Protocol in workflow.md)

## Phase 4: Frontend Implementation
- [ ] Task: Refactor Algorithm Selection UI
    - [ ] Replace simple selection with a Dropdown/Radio group for Core Algorithms.
- [ ] Task: Implement Multiplier Checkboxes
    - [ ] Use Shadcn checkboxes for optimizations.
    - [ ] Implement mutual exclusivity logic (e.g., SIMD vs Perfect Hash).
- [ ] Task: Add Tooltips and Badges
    - [ ] Integrate Shadcn Tooltips for algorithm/multiplier explanations.
    - [ ] Add "Estimated Gain" badges next to multipliers.
- [ ] Task: Conductor - User Manual Verification 'Phase 4: Frontend Implementation' (Protocol in workflow.md)

## Phase 5: Integration & Verification
- [ ] Task: End-to-End Testing
    - [ ] Verify full flow: UI Selection -> FastAPI -> C++ Engine -> Results.
- [ ] Task: Final Performance Audit
    - [ ] Run "Race Mode" benchmarks and verify UI telemetry accurately reflects engine throughput.
- [ ] Task: Conductor - User Manual Verification 'Phase 5: Integration & Verification' (Protocol in workflow.md)
