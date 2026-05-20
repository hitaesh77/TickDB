#include "wal_writer.h"

WALWriter::WALWriter(const std::filesystem::path& path) {
    WALWriter::wal_file.open(path, std::ios_base::app | std::ios_base::binary); // file openend in binary append mode

    if (!wal_file.is_open()) {
        throw std::runtime_error("Failed to open WAL file");
    }
}

void WALWriter::append_batch(const std::vector<Tick>& ticks) {
    if (ticks.empty()) {
        return;
    }

    if (ticks.size() > std::numeric_limits<uint32_t>::max()) {
        throw std::runtime_error("Too many ticks in WAL batch");
    }

    const uint32_t tick_count = static_cast<uint32_t>(ticks.size());
    const uint32_t payload_size = tick_count * 20;
    
    std::vector<uint8_t> payload(payload_size);

    for (size_t i = 0; i < ticks.size(); i++) {
        encode_tick_20(ticks[i], payload.data() + (i * 20));
    }

    // Bytef is a data type from zlib, so have to cast our payload type to that
    const uint32_t checksum = static_cast<uint32_t> (crc32(
        0L,
        reinterpret_cast<const Bytef*>(payload.data()),
        payload.size()
    ));

    WALRecordHeader header {
        tick_count,
        payload_size,
        checksum
    };

    // write header
    wal_file.write(
        reinterpret_cast<const char*>(&header),
        sizeof(WALRecordHeader)
    );

    // write payload
    wal_file.write(
        reinterpret_cast<const char*>(payload.data()),
        payload.size()
    );

    if (!wal_file) {
        throw std::runtime_error("Failed to write WAL record");
    }
}

void WALWriter::flush() {
    wal_file.flush();

    if (!wal_file) {
        throw std::runtime_error("Failed to flush WAL file");
    }
}