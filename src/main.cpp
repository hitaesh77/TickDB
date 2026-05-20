#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include "tick.h"
#include "wal/wal.h"

// Test Suite helped written by ChatGPT

void assert_tick_equal(const Tick& actual, const Tick& expected, const char* message) {
    assert(actual.time == expected.time && message);
    assert(actual.price == expected.price && message);
    assert(actual.volume == expected.volume && message);
}

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

        // TEST 7: WALReader Full Replay
    {
        WALReader reader(test_wal_path);
        std::vector<Tick> recovered = reader.replay();

        assert(recovered.size() == 3 && "Test 7.1: FAILED - WALReader should recover 3 ticks");

        Tick expected_1 = {1716135001, 150.50, 1000};
        Tick expected_2 = {1716135002, 150.55, 1200};
        Tick expected_3 = {1716135003, 150.60, 500};

        assert_tick_equal(recovered[0], expected_1, "Test 7.2: FAILED - Recovered tick 1 mismatch");
        assert_tick_equal(recovered[1], expected_2, "Test 7.3: FAILED - Recovered tick 2 mismatch");
        assert_tick_equal(recovered[2], expected_3, "Test 7.4: FAILED - Recovered tick 3 mismatch");

        std::cout << "Test 7: PASSED - WALReader replays all valid records correctly." << std::endl;
    }

    // TEST 8: WALReader Handles Empty WAL
    {
        std::filesystem::path empty_wal_path = "test_empty.wal";

        if (std::filesystem::exists(empty_wal_path)) {
            std::filesystem::remove(empty_wal_path);
        }

        {
            std::ofstream empty_file(empty_wal_path, std::ios::binary);
        }

        {
            WALReader reader(empty_wal_path);
            std::vector<Tick> recovered = reader.replay();

            assert(recovered.empty() && "Test 8: FAILED - Empty WAL should recover zero ticks");
        } // reader destructor runs here, file closes here

        std::filesystem::remove(empty_wal_path);

        std::cout << "Test 8: PASSED - WALReader handles empty WAL correctly." << std::endl;
    }

    // TEST 9: WALReader Stops Cleanly On Partial Final Payload
    {
        std::filesystem::path partial_wal_path = "test_partial_payload.wal";

        if (std::filesystem::exists(partial_wal_path)) {
            std::filesystem::remove(partial_wal_path);
        }

        std::vector<Tick> valid_batch = {
            {1, 100.0, 10},
            {2, 101.0, 20}
        };

        std::vector<Tick> damaged_batch = {
            {3, 102.0, 30},
            {4, 103.0, 40}
        };

        {
            WALWriter writer(partial_wal_path);
            writer.append_batch(valid_batch);
            writer.append_batch(damaged_batch);
            writer.flush();
        }

        std::uintmax_t original_size = std::filesystem::file_size(partial_wal_path);
        std::filesystem::resize_file(partial_wal_path, original_size - 5);

        {
            WALReader reader(partial_wal_path);
            std::vector<Tick> recovered = reader.replay();

            assert(recovered.size() == 2 && "Test 9.1: FAILED - Reader should recover only the first complete batch");

            assert_tick_equal(recovered[0], valid_batch[0], "Test 9.2: FAILED - First recovered tick mismatch");
            assert_tick_equal(recovered[1], valid_batch[1], "Test 9.3: FAILED - Second recovered tick mismatch");
        }
        std::filesystem::remove(partial_wal_path);

        std::cout << "Test 9: PASSED - WALReader ignores partial final payload correctly." << std::endl;
    }

    // TEST 10: WALReader Stops On Corrupted Payload CRC
    {
        std::filesystem::path corrupt_wal_path = "test_corrupt_payload.wal";

        if (std::filesystem::exists(corrupt_wal_path)) {
            std::filesystem::remove(corrupt_wal_path);
        }

        std::vector<Tick> valid_batch = {
            {10, 200.0, 100},
            {11, 201.0, 110}
        };

        std::vector<Tick> corrupt_batch = {
            {12, 202.0, 120},
            {13, 203.0, 130}
        };

        {
            WALWriter writer(corrupt_wal_path);
            writer.append_batch(valid_batch);
            writer.append_batch(corrupt_batch);
            writer.flush();
        }

        {
            std::fstream file(corrupt_wal_path, std::ios::in | std::ios::out | std::ios::binary);
            assert(file.is_open() && "Test 10: FAILED - Could not open WAL file for corruption");

            // First record size:
            // header = 12 bytes
            // payload = 2 ticks * 20 bytes = 40 bytes
            // total = 52 bytes
            //
            // Second payload starts at:
            // first record 52 bytes + second header 12 bytes = byte 64
            file.seekg(64, std::ios::beg);

            char byte;
            file.read(&byte, 1);

            byte ^= 0xFF;

            file.seekp(64, std::ios::beg);
            file.write(&byte, 1);
        }

        {
            WALReader reader(corrupt_wal_path);
            std::vector<Tick> recovered = reader.replay();

            assert(recovered.size() == 2 && "Test 10.1: FAILED - Reader should recover only records before corrupted payload");

            assert_tick_equal(recovered[0], valid_batch[0], "Test 10.2: FAILED - First recovered tick mismatch");
            assert_tick_equal(recovered[1], valid_batch[1], "Test 10.3: FAILED - Second recovered tick mismatch");
        }
        std::filesystem::remove(corrupt_wal_path);

        std::cout << "Test 10: PASSED - WALReader detects CRC corruption and stops safely." << std::endl;
    }

    // TEST 11: WALReader Stops On Partial Header
    {
        std::filesystem::path partial_header_path = "test_partial_header.wal";

        if (std::filesystem::exists(partial_header_path)) {
            std::filesystem::remove(partial_header_path);
        }

        std::vector<Tick> valid_batch = {
            {20, 300.0, 200}
        };

        {
            WALWriter writer(partial_header_path);
            writer.append_batch(valid_batch);
            writer.flush();
        }

        {
            std::ofstream file(partial_header_path, std::ios::app | std::ios::binary);
            uint32_t fake_partial_header = 12345;
            file.write(reinterpret_cast<const char*>(&fake_partial_header), sizeof(fake_partial_header));
        }

        {
            WALReader reader(partial_header_path);
            std::vector<Tick> recovered = reader.replay();

            assert(recovered.size() == 1 && "Test 11.1: FAILED - Reader should ignore partial trailing header");
            assert_tick_equal(recovered[0], valid_batch[0], "Test 11.2: FAILED - Recovered tick mismatch");
        }
        std::filesystem::remove(partial_header_path);

        std::cout << "Test 11: PASSED - WALReader ignores partial trailing header correctly." << std::endl;
    }

    // Clean up test file artifacts
    std::filesystem::remove(test_wal_path);

    return 0;    
}