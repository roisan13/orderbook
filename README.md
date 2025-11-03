# 📈 C++ Orderbook

A high-performance limit order book implementation in modern C++20.

<div align="center">

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C.svg)
![Tests](https://img.shields.io/badge/tests-27%20passing-success.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

</div>

---

## ✨ Features

- **Multiple Order Types**: Market, Limit (GTC), IOC, FOK, Post-Only, Stop Orders
- **Price-Time Priority**: FIFO matching at each price level
- **Smart Matching**: Orders execute at maker's price
- **Comprehensive Tests**: 27+ unit tests with Google Test
- **Modern C++20**: Smart pointers, structured bindings, concepts

---

## 🚀 Quick Start

### Build & Run
```bash
# Configure and build
cmake -B build && cmake --build build

# Run tests
ctest --test-dir build

# Run demo
./build/orderbook_app
```

### Example Usage
```cpp
#include "Orderbook.h"

Orderbook book;

// Add a sell order
auto ask = std::make_shared<Order>(
    OrderType::GoodTillCancel, 1, Side::Sell, 100, 10
);
book.AddOrder(ask);

// Add a matching buy order
auto bid = std::make_shared<Order>(
    OrderType::GoodTillCancel, 2, Side::Buy, 100, 10
);
auto trades = book.AddOrder(bid);

std::cout << "Trades executed: " << trades.size() << "\n";
```

---

## 📦 Order Types

| Type | Description | Use Case |
|------|-------------|----------|
| **Market** | Execute at any available price | Immediate execution |
| **GTC** | Good Till Cancel - rests in book | Standard limit order |
| **IOC** | Immediate or Cancel - fill & cancel rest | Partial fills OK |
| **FOK** | Fill or Kill - all or nothing | Must fill completely |
| **Post-Only** | Never takes liquidity | Market making |
| **Stop** | Triggers at price level | Stop-loss, breakouts |

---

## 🏗️ Architecture
```
┌─────────────────────────────────────┐
│          Orderbook                  │
├─────────────────────────────────────┤
│  Bids (Price ↓)    Asks (Price ↑)   │
│  100: [O1, O2]     101: [O3]        │
│   99: [O4]         102: [O5, O6]    │
└─────────────────────────────────────┘
         ↓                    ↓
    Best Bid            Best Ask
```

**Key Design Choices:**
- `std::map` for price-sorted books (O(log n) access)
- `std::unordered_map` for O(1) order lookup by ID
- `std::shared_ptr` for safe multi-ownership
- `std::list` for O(1) FIFO queue operations

---

## 🧪 Testing
```bash
# Run all tests
./build/orderbook_test

# Run specific test category
./build/orderbook_test --gtest_filter=*FIFO*

# Verbose output
ctest --test-dir build --verbose
```

**Test Coverage:**
- ✅ Order matching & partial fills
- ✅ FIFO and price-time priority
- ✅ All 6 order types
- ✅ Multi-level book walking
- ✅ Stop order triggering
- ✅ Error handling & edge cases

---

## 📊 Performance

| Operation | Time Complexity |
|-----------|----------------|
| Add order | O(log n) |
| Cancel order | O(log n) |
| Match order | O(k × log n)* |
| Get best price | O(1) |
| Lookup by ID | O(1) |

*k = number of trades generated

---

## 📁 Project Structure
```
orderbook/
├── include/              # Public API
│   ├── Order.h
│   ├── Orderbook.h
│   ├── Trade.h
│   └── Types.h
├── src/                  # Implementation
├── tests/                # Unit tests
├── CMakeLists.txt
└── main.cpp
```

---

## 🛠️ Requirements

- **Compiler**: GCC 10+, Clang 11+, or MSVC 2019+
- **CMake**: 3.20 or later
- **C++ Standard**: C++20

---

## 🎯 Build Options
```bash
# Debug build (default)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Optimized release build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build without tests
cmake -B build -DBUILD_TESTS=OFF
```

---

## 💡 Key Concepts

### Price-Time Priority
Orders are matched by best price first, then by time (FIFO) at each level.

### Maker-Taker Model
Trades execute at the **resting order's price** (the maker), not the aggressive order's price.

### Stop Orders
Trigger when a **trade** occurs at or through the stop price:
- **Stop-Loss Sell**: Triggers when price ≤ stop price
- **Stop-Buy**: Triggers when price ≥ stop price

---

## 📝 License

MIT License