#include "wal_writer.h"

WALWriter::WALWriter(const std::filesystem::path& path) {
    WALWriter::wal_file.open(path, std::ios_base::app | std::ios_base::binary);
    if (!wal_file.is_open()) {
        throw std::runtime_error("Failed to open WAL file");
    }
}

void WALWriter::append_batch(std::vector<const Tick> ticks) {
    
}