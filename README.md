# TickDB

A high performance, log structured time series storage engine built in C++ for financial market data. Optimized for write throughput and memory efficiency.

## Overview
TickDB is my attempt at designing a time series store that can handle the speed of high frequency trading data. By utilizing a Write Ahead Log (WAL) and immutable disk segments, it ensures sequential disk I/O and predictable query performance.

## Core Architecture
- **In-Memory:** Memtable for fast ingestion with preallocated buffers to minimize heap fragmentation.
- **Persistence:** Append-only WAL for crash recovery and periodic flushing to binary segment files.
- **Indexing:** Sparse indexing (timestamp → file offset) for efficient $O(\log n)$ range lookups.
- **API:** Minimalist C++ HTTP server for remote ingestion and querying.

## Technical Specifications
- **Data Model:** Fixed-width binary records (16 bytes per tick).
- **Disk Strategy:** Large-block sequential flushes to eliminate random seek overhead.
- **Memory Management:** Cache-friendly struct layouts and minimal object overhead.
- **Query Engine:** Identifies relevant segments via sparse index, performs binary search, and executes sequential scans.

## Data Structure
```cpp
struct Tick {
    int64_t timestamp;  // Nanoseconds
    double price;       // Market price
    int32_t volume;     // Trade size
};
