# Poker Equity Engine

A high-performance poker equity calculation engine with multiple implementation strategies (base Python, NumPy, multiprocessing, and future C++ optimizations).

## Project Structure

```
PokerEquityEngine/
├── src/
│   ├── python/          # Python equity engine implementation
│   │   ├── models/      # Data models (Card, JobRequest, etc.)
│   │   ├── engine/      # EquityEngine and factory
│   │   └── utils/       # Utilities (metrics collection)
│   └── web/             # React frontend
├── tests/
│   └── python/          # Python unit tests
├── docs/                 # API documentation
└── requirements.txt      # Python dependencies
```

## Python Implementation

### Requirements

- **Python**: 3.10+ (tested with Python 3.13.3)
- **Dependencies**: See `requirements.txt`

### Setup

1. **Create virtual environment**:

   ```bash
   python3 -m venv .venv
   source .venv/bin/activate  # On Windows: .venv\Scripts\activate
   ```

2. **Install dependencies**:

   ```bash
   pip install -r requirements.txt
   ```

3. **Install package in editable mode**:

   ```bash
   pip install -e .
   ```

### Development

The Python engine uses a dependency injection pattern, allowing different strategy implementations to be plugged in:

- **Base Python**: Pure Python implementation (current)
- **NumPy**: Vectorized implementation (future)
- **Multiprocessing**: Parallel processing (future)

### Testing

#### Run All Tests

```bash
source .venv/bin/activate  # Activate venv first
pytest tests/python/ -v
```

#### Run Tests with Coverage

```bash
pytest tests/python/ --cov=src/python --cov-report=term-missing
```

### Performance

## Frontend

The React frontend is located in `src/web/`. See the frontend documentation for setup and usage.

## API Contract

See `docs/api-contract.md` for the complete API specification that all backend implementations must follow.

## License

[Add license information here]
