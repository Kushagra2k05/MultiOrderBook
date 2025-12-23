# Multi-Order Book Engine (C++)

A **high-performance, deterministic multi-order book engine** implemented in modern C++, designed to model real-world exchange matching behavior with strict correctness guarantees.

This project focuses on **systems design, data-structure efficiency, and correctness-critical logic**, rather than being a toy DSA implementation.

---

## Features

- Price–time priority matching (industry standard)
- Support for real exchange order types:
  - Good-Till-Cancel (GTC)
  - Good-For-Day (GFD)
  - Fill-And-Kill (FAK)
  - Fill-Or-Kill (FOK)
  - Market orders
- Partial and full fills with deterministic outcomes
- Clean separation of concerns:
  - Order lifecycle
  - Matching logic
  - Order book state
- Designed for extensibility toward low-latency trading systems

---

## Order Matching Logic

The engine enforces **strict price–time priority**:

1. Best price is matched first  
   - Highest bid vs lowest ask
2. FIFO ordering within the same price level
3. Partial fills supported
4. Remaining quantity handled according to order type semantics

This guarantees deterministic and reproducible results, a critical requirement for trading systems.

---

## High-Level Architecture

Order
├─ Immutable metadata (ID, side, type)
└─ Mutable state (remaining quantity)

Price Level
├─ FIFO queue of orders
└─ Maintains time priority

Order Book
├─ Buy side (sorted descending by price)
├─ Sell side (sorted ascending by price)
└─ Matching engine
Performance Considerations

O(log N) price-level lookup

O(1) FIFO operations within a price level

Minimal copying and controlled memory ownership

Easily extensible to:

Cache-aware data layouts

Lock-free or concurrent matching

Latency benchmarking

The implementation prioritizes correctness and clarity first, reflecting real production development practices.

Correctness Guarantees

No over-execution of orders

No invalid partial states

Strict enforcement of order semantics

Consistent order book state after every operation

Defensive validation against invalid inputs

Why This Project

This project demonstrates:

Practical understanding of exchange mechanics

Strong systems programming fundamentals

C++ proficiency beyond syntax

Ability to design and reason about stateful, correctness-critical software

Relevant to roles involving:

Backend infrastructure

Low-latency systems

Distributed systems

Financial and trading platforms

Future Enhancements

Order cancellation and modification

Multi-instrument support

Persistence and recovery

Thread-safe matching

Market data feed simulation

FIX protocol integration

Tech Stack

Language: C++

Paradigm: Object-oriented, performance-aware design

Focus: Determinism, correctness, extensibility

Author

Kushagra Srivastava
Engineering student interested in systems programming, low-latency software, and algorithmic correctness.
The design mirrors real exchange implementations where **orders are grouped by price level and sequenced by arrival time**.

---

## Example Usage

```cpp
Order order(
    OrderType::FillOrKill,
    orderId,
    Side::Buy,
    price,
    quantity
);

