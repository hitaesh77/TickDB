#include "wal.h"

WALReader::WALReader(const std::filesystem::path& path) {
    wal_file.open(path, std::ios_base::binary); // file openend in binary append mode

    if (!wal_file.is_open()) {
        throw std::runtime_error("Failed to open WAL file");
    }
}