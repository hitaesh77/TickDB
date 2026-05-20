#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include "tick.h"
#include "wal/wal.h"

int main(int argc, char** argv) {

    // TEST 1: round trip (encode to decode)
    {
        Tick original = {1716135000, 150.25, 500};
        uint8_t buffer[20];

        encode_tick_20(original, buffer);
        Tick decoded = decode_tick_20(buffer);

        assert(decoded.time == original.time && "Test 1.1: FAILED - time mismatch");
        assert(decoded.price == original.price && "Test 1.2: FAILED - price mismatch");
        assert(decoded.volume == original.volume && "Test 1.3: FAILED - volume mismatch");
        std::cout << "Test 1: PASSED - round trip matches" << std::endl;
    }

    // TEST 2: buffer layout and boundary verification
    {
        Tick original = {0x0102030405060708, 0.0, 0xAABBCCDD};
        uint8_t buffer[20] = {0};

        encode_tick_20(original, buffer);

        // little endian verification of time bits
        assert(buffer[0] == 0x08 && "Test 2.1: FAILED - byte 0 layout incorrect");
        assert(buffer[7] == 0x01 && "Test 2.2: FAILED - byte 7 layout incorrect");

        // little endian verification of volume bits
        assert(buffer[16] == 0xDD && "Test 2.3: FAILED - volume byte 16 incorrect");
        assert(buffer[19] == 0xAA && "Test 2.4: FAILED - volume byte 19 incorrect");
        
        std::cout << "Test 2: PASSED - buffer offsets match 20 byte spec." << std::endl;
    }

    // TEST 3: continuous stream processing
    {
        Tick stream_in[3] = {
            {1, 10.5, 100},
            {2, 11.0, 200},
            {3, 11.5, 300}
        };

        // 60 byte stream buffer
        uint8_t stream_buffer[60];

        // pack stream into buffer
        for (int i = 0; i < 3; ++i) {
            encode_tick_20(stream_in[i], stream_buffer + (i * 20));
        }

        // unpack and verify stream form buffer
        for (int i = 0; i < 3; ++i) {
            Tick unpacked = decode_tick_20(stream_buffer + (i * 20));

            assert(unpacked.time == stream_in[i].time && "Test 3: FAILED - stream time mismatch");
            assert(unpacked.price == stream_in[i].price && "Test 3: FAILED - stream price mismatch");
            assert(unpacked.volume == stream_in[i].volume && "Test 3: FAILED - stream volume mismatch");
        }

        std::cout << "Test 3: PASSED - stream array handling packs and unpacks contiguously." << std::endl;
    }

    // Define a temporary path for WAL testing
    std::filesystem::path test_wal_path = "test_suite_exec.wal";
    
    // Ensure clean state before WAL tests
    if (std::filesystem::exists(test_wal_path)) {
        std::filesystem::remove(test_wal_path);
    }

    // TEST 4: WAL Single Batch Persistence & Binary Integrity Validation
    {
        std::vector<Tick> batch_in = {
            {1716135001, 150.50, 1000},
            {1716135002, 150.55, 1200}
        };

        // Notice: Your wal_writer.h specifies std::vector<const Tick> 
        // while your wal_writer.cpp specifies std::vector<const Tick>&.
        // Assuming you match them to std::vector<Tick> or pass properly:
        {
            WALWriter writer(test_wal_path);
            writer.append_batch(batch_in);
            writer.flush();
        } // Close writer scope to force drop and close the file handle safely

        // Read back the file manually to verify structure
        std::ifstream test_file(test_wal_path, std::ios::binary);
        assert(test_file.is_open() && "Test 4: FAILED - Could not open written WAL file for reading");

        // Read and check header
        WALRecordHeader header;
        test_file.read(reinterpret_cast<char*>(&header), sizeof(WALRecordHeader));
        
        assert(header.tick_count == 2 && "Test 4.1: FAILED - Header tick count mismatch");
        assert(header.payload_size == 40 && "Test 4.2: FAILED - Header payload size mismatch");

        // Read and check payload
        std::vector<uint8_t> actual_payload(header.payload_size);
        test_file.read(reinterpret_cast<char*>(actual_payload.data()), header.payload_size);

        // Verify CRC32 match
        uint32_t expected_crc = static_cast<uint32_t>(crc32(0L, reinterpret_cast<const Bytef*>(actual_payload.data()), actual_payload.size()));
        assert(header.crc32 == expected_crc && "Test 4.3: FAILED - Saved CRC32 does not match actual payload data CRC32");

        // Decode ticks and verify contents
        for (size_t i = 0; i < header.tick_count; ++i) {
            Tick decoded = decode_tick_20(actual_payload.data() + (i * 20));
            assert(decoded.time == batch_in[i].time && "Test 4.4: FAILED - Deserialized time mismatch");
            assert(decoded.price == batch_in[i].price && "Test 4.5: FAILED - Deserialized price mismatch");
            assert(decoded.volume == batch_in[i].volume && "Test 4.6: FAILED - Deserialized volume mismatch");
        }

        test_file.close();
        std::cout << "Test 4: PASSED - Single batch WAL write, header layout, CRC, and payload match." << std::endl;
    }

    // TEST 5: WAL Appending & Multiple Batch Management
    {
        // Reopen existing WAL file to test append mode longevity
        std::vector<Tick> batch_2 = {
            {1716135003, 150.60, 500}
        };

        {
            WALWriter writer(test_wal_path);
            writer.append_batch(batch_2);
            writer.flush();
        }

        std::ifstream test_file(test_wal_path, std::ios::binary);
        
        // Skip first batch completely (Header + Payload = 12 bytes + 40 bytes)
        test_file.seekg(sizeof(WALRecordHeader) + 40, std::ios::beg);

        // Read second batch header
        WALRecordHeader header_2;
        test_file.read(reinterpret_cast<char*>(&header_2), sizeof(WALRecordHeader));

        assert(header_2.tick_count == 1 && "Test 5.1: FAILED - Batch 2 header tick count mismatch");
        assert(header_2.payload_size == 20 && "Test 5.2: FAILED - Batch 2 header payload size mismatch");

        // Read second batch payload
        std::vector<uint8_t> payload_2(header_2.payload_size);
        test_file.read(reinterpret_cast<char*>(payload_2.data()), header_2.payload_size);

        Tick decoded = decode_tick_20(payload_2.data());
        assert(decoded.time == batch_2[0].time && "Test 5.3: FAILED - Batch 2 data time mismatch");
        assert(decoded.volume == batch_2[0].volume && "Test 5.4: FAILED - Batch 2 data volume mismatch");

        test_file.close();
        std::cout << "Test 5: PASSED - Multi-batch append configuration validated." << std::endl;
    }

    // TEST 6: WAL Empty Batch Edge Case Handling
    {
        // Get file size before empty append operation
        size_t size_before = std::filesystem::file_size(test_wal_path);

        std::vector<Tick> empty_batch;
        {
            WALWriter writer(test_wal_path);
            writer.append_batch(empty_batch);
            writer.flush();
        }

        size_t size_after = std::filesystem::file_size(test_wal_path);
        assert(size_before == size_after && "Test 6: FAILED - Empty batch should not write any bytes to disk");

        std::cout << "Test 6: PASSED - Empty batch ignored correctly." << std::endl;
    }

    // Clean up test file artifacts
    std::filesystem::remove(test_wal_path);

    return 0;    
}