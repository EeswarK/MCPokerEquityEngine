# Technology Stack: Poker Equity Engine

## Core Architecture
A multi-tier architecture designed for high-performance calculation, real-time telemetry, and a responsive user experience.

## Performance Layer (C++)
- **Language:** C++17 (for modern features and zero-cost abstractions).
- **Build System:** CMake.
- **Optimization:** SIMD (target-specific), multi-threading (std::thread/pthread), and cache-aware data structures.
- **JSON Handling:** RapidJSON (high-performance parsing).
- **Serialization:** FlatBuffers (for efficient binary telemetry).

## Orchestration Layer (Python)
- **Framework:** FastAPI (asynchronous REST API).
- **Server:** Uvicorn.
- **Data Validation:** Pydantic v2.
- **Concurrency:** Asyncio for WebSocket management and non-blocking job orchestration.

## Presentation Layer (React)
- **Framework:** React 18+ (TypeScript).
- **Build Tool:** Vite.
- **Styling:** Tailwind CSS (utility-first CSS) and Shadcn/UI (accessible components).
- **State Management:** Zustand.
- **Visualizations:** Recharts (for convergence and performance charts).

## Infrastructure & IPC
- **IPC:** POSIX Shared Memory for ultra-low latency metrics transfer between C++ and Python.
- **Real-time Updates:** WebSockets for bidirectional communication with the frontend.
- **Deployment:** AWS EC2 with Nginx as a reverse proxy and Systemd for process management.

## Quality Assurance
- **Python Testing:** Pytest with coverage reporting.
- **C++ Testing:** Google Test (GTest).
- **Linting/Formatting:** Ruff (Python), ESLint/Prettier (TypeScript/React).
