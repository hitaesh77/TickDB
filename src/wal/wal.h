#ifndef WAL_H
#define WAL_H

#include <filesystem>
#include <span>
#include <iostream>
#include <fstream>
#include <vector>
#include <zlib.h>

#include "../tick.h"

// Goal: append batches durably and replay them after restart.

struct WALRecordHeader {
    uint32_t tick_count;
    uint32_t payload_size;
    uint32_t crc32;
};

class WALWriter {
    public:
        explicit WALWriter(const std::filesystem::path& path);
        void append_batch(const std::vector<Tick>& ticks);
        void flush();
    
        private:
            std::ofstream wal_file; // ofstream for writing 
};


class WALReader {
    public:
        explicit WALReader(const std::filesystem::path& path);
        std::vector<Tick> replay();
    
        private:
            std::ifstream wal_file; // ifstream for reading 
};

#endif