# Product Guidelines: Poker Equity Engine

## Tone and Voice
- **Pragmatic and Professional:** Documentation and UI copy should be direct and clear. Focus on the "why" behind architectural decisions and the measurable impact of optimizations.
- **Academic Rigor:** When discussing algorithms or statistical convergence, use precise terminology (e.g., "Monte Carlo error bounds," "SIMD lane utilization") to demonstrate technical depth.
- **Engaging Clarity:** Complex concepts should be explained simply enough for an enthusiast to follow, without sacrificing technical accuracy.

## Visual Identity (UI/UX)
- **Modern & Gamified:** The interface should feel alive and responsive. Use smooth animations for progress bars and "pop" effects when simulations complete.
- **Clean Aesthetic:** Maintain a minimalist, card-based layout with generous whitespace to ensure that data visualizations remain the focal point.
- **Interactive Dashboards:** Prioritize real-time feedback. Users should see "live" updates as the engine processes simulations, creating a sense of power and speed.
- **Data Visualization:** Use high-contrast charts (e.g., Recharts) to visualize probability distributions and convergence rates, favoring dark mode for a professional "engineering terminal" feel.

## Engineering Principles
- **Evidence-Based Optimization:** No optimization is accepted without benchmark data. Every performance-related change must demonstrate a measurable improvement in the hot path.
- **Performance First (Calculated):** In the C++ core, prioritize execution speed and memory efficiency. Use hardware-specific optimizations (SIMD, cache-locality) where they provide significant gains.
- **Maintainable Hot Paths:** While performance is critical, code should remain readable. Use modern C++ abstractions (RAII, smart pointers, templates) that offer zero-cost performance while ensuring safety and maintainability.
- **Strict Separation of Concerns:** Keep the calculation logic (C++), orchestration (Python), and presentation (React) decoupled to allow for independent scaling and testing.
- **Test-Driven Rigor:** Every feature or optimization must include unit tests and, where applicable, integration tests to ensure the engine's mathematical correctness.
