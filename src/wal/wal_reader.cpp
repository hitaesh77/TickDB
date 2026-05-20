#include "wal.h"

WALReader::WALReader(const std::filesystem::path& path) {
    wal_file.open(path, std::ios_base::binary); // file openend in binary append mode

    if (!wal_file.is_open()) {
        throw std::runtime_error("Failed to open WAL file");
    }
}

std::vector<Tick> WALReader::replay() {
    std::vector<Tick> recovered;

    while (true) {
        // read header from wal file
        WALRecordHeader header;
        wal_file.read(reinterpret_cast<char*>(&header), sizeof(WALRecordHeader)); // treat header struct as an array of bytes

        // validate bytes read
        std::streamsize bytes_read = wal_file.gcount();
        if (bytes_read == 0) break;
        if (bytes_read != static_cast<std::streamsize>(sizeof(WALRecordHeader))) break;
        if (header.tick_count == 0) break;
        if (header.payload_size == 0) break;
        if (header.payload_size % 20 != 0) break;
        if (header.tick_count != header.payload_size / 20) break;

        // read payload
        std::vector<uint8_t> payload(header.payload_size);
        wal_file.read(reinterpret_cast<char*>(payload.data()), header.payload_size);

        // validate payload
        if (wal_file.gcount() != static_cast<std::streamsize>(header.payload_size)) break;
        
        uint32_t computed_crc = static_cast<uint32_t>(
            crc32(
                0L,
                reinterpret_cast<const Bytef*>(payload.data()),
                static_cast<uInt>(payload.size())
            )
        );

        if (computed_crc != header.crc32) break;

        for (uint32_t i = 0; i < header.tick_count; i++) {
            const uint8_t* tick_ptr = payload.data() + (i * 20);
            recovered.push_back(decode_tick_20(tick_ptr));
        }
    }

    return recovered;
}