#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include "tick.h"

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

    return 0;    
}