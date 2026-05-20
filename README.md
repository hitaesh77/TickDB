# TickDB

A high-performance, log-structured time-series storage engine built in C++ for financial market data. Optimized for write throughput, durable ingestion, and memory-efficient binary storage.

## Overview

TickDB is my attempt at designing a time-series store for high-volume market tick data. The project uses a write-ahead log, fixed-width binary encoding, in-memory buffering, and immutable segment files to support fast sequential writes and efficient range queries.

The current focus is building the storage engine from the ground up, starting with a durable write path and recovery system.

## Current Status

Implemented:

- Fixed-width 20-byte binary tick encoding and decoding
- Append-only Write-Ahead Log writer
- WAL batch records with CRC32 checksums using zlib
- WAL replay through `WALReader`
- Recovery behavior for:
  - valid WAL records
  - empty WAL files
  - partial final payloads
  - partial trailing headers
  - corrupted payloads detected through CRC mismatch

Next:

- MemTable implementation
- Immutable segment writer
- Segment reader with sparse indexing
- StorageEngine integration
- Query engine
- HTTP API
- Benchmarks against CSV append and scan

## Core Architecture

- **Binary Tick Encoding:** Each tick is encoded into a fixed 20-byte on-disk representation.
- **Write-Ahead Log:** Append-only WAL stores batches before they enter higher-level storage structures.
- **Crash Recovery:** WALReader scans valid records and stops safely at the first incomplete or corrupted record.
- **In-Memory Buffering:** MemTable will hold recent writes before flushing sorted data to disk.
- **Immutable Segments:** Flushed data will be stored in sorted binary segment files.
- **Sparse Indexing:** Segment indexes will map timestamps to file offsets for efficient range queries.
- **API Layer:** A minimal C++ HTTP server will expose ingestion and query endpoints.

## Technical Specifications

- **Language:** C++20
- **Checksum:** zlib CRC32
- **Data Model:** Fixed-width binary records, 20 bytes per tick
- **WAL Header:** Compact 12-byte header per batch record
- **Disk Strategy:** Sequential append for WAL, large-block flushes for segments
- **Query Strategy:** Sparse index lookup followed by sequential scan
- **Build System:** Makefile

## Tick Data Model

```cpp
struct Tick {
    uint64_t time;
    double price;
    uint32_t volume;
};
```

The in-memory `Tick` structure is encoded explicitly into a 20-byte disk format:

```txt
bytes 0..7    time
bytes 8..15   price
bytes 16..19  volume
```

This avoids relying on compiler struct padding and keeps the disk format stable.

## WAL Record Format

Each call to `append_batch()` writes one WAL record.

```txt
[WALRecordHeader]
[payload bytes]
```

The current WAL header is intentionally lean:

```cpp
struct WALRecordHeader {
    uint32_t tick_count;
    uint32_t payload_size;
    uint32_t crc32;
};
```

The payload contains `tick_count` consecutive 20-byte encoded ticks.

```txt
payload_size = tick_count * 20
```

The CRC32 checksum is computed over the full payload.

## WAL Recovery Behavior

`WALReader::replay()` reads the WAL sequentially:

1. Read the 12-byte header.
2. Validate `tick_count` and `payload_size`.
3. Read the payload.
4. Recompute CRC32 over the payload.
5. Decode ticks into memory.
6. Stop safely if a partial header, partial payload, or CRC mismatch is found.

This allows TickDB to recover all valid records before a crash-damaged WAL tail.

## Example WAL Layout

```txt
Record 1:
  Header:
    tick_count = 2
    payload_size = 40
    crc32 = checksum(payload)
  Payload:
    tick 1
    tick 2

Record 2:
  Header:
    tick_count = 1
    payload_size = 20
    crc32 = checksum(payload)
  Payload:
    tick 3
```

## Build and Run

```bash
make run
```

Current test coverage includes:

- Tick encode/decode round trip
- Binary layout verification
- Continuous tick stream encoding
- Single-batch WAL write
- Multi-batch WAL append
- Empty batch handling
- Full WAL replay
- Empty WAL replay
- Partial payload recovery
- CRC corruption detection
- Partial trailing header recovery

## Project Roadmap

### Stage 1: Binary Tick Encoding

Completed.

- Define `Tick`
- Encode to 20-byte binary format
- Decode from 20-byte binary format
- Verify byte layout

### Stage 2: Write-Ahead Log

Completed.

- Write batches to append-only WAL
- Store tick count, payload size, and CRC32
- Replay WAL records
- Detect corrupted or incomplete records

### Stage 3: MemTable

Next.

- Preallocate in-memory tick buffer
- Append batches
- Track flush threshold
- Support sorted snapshots
- Support simple range queries over active memory

### Stage 4: Segment Files

Planned.

- Write sorted immutable binary segment files
- Store metadata and sparse index
- Validate file structure
- Query segment ranges efficiently

### Stage 5: Storage Engine

Planned.

- Combine WAL, MemTable, and segments
- Support write, flush, recover, and query
- Reset WAL safely after successful segment flush

### Stage 6: API and Benchmarks

Planned.

- HTTP write and query endpoints
- CSV baseline implementation
- Ingestion throughput benchmarks
- Query latency benchmarks
- Disk footprint comparison

## Long-Term Ideas

- Segment compaction
- Multi-symbol support
- Timestamp delta encoding
- Compression
- Memory-mapped segment reads
- Configurable fsync policy
- More detailed benchmarking harness

## Why This Project Exists

TickDB is a systems project focused on understanding how storage engines work under the hood. The goal is not to build a full database, but to implement the core mechanics behind durable, high-throughput time-series ingestion:

- binary serialization
- write-ahead logging
- crash recovery
- immutable files
- sparse indexing
- range queries
- benchmarking against naive storage approaches
