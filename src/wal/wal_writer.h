#include <filesystem>
#include <span>
#include <iostream>
#include <fstream>
#include <vector>

#include "tick.h"

// Goal: append batches durably and replay them after restart.

class WALWriter {
    public:
        explicit WALWriter(const std::filesystem::path& path);
        void append_batch(std::vector<const Tick> ticks);
    
        private:
            std::ofstream wal_file; // ofstream for writing 
};