#include <iostream>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include "tick.h"

int main(int argc, char** argv) {

    // TESTING PACKED STRUCT SIZE

    // Test 1: size of struct
    tick test_tick = {123, 123, 123};
    assert(sizeof(test_tick) == 20 && "Test 1: FAILED - tick struct should be 20 bytes");
    std::cout << "Test 1: PASSED - tick struct is 20 bytes" << std::endl;

    // Test 2: Field Offsets
    assert(offsetof(tick, time) == 0 && "Test 2.1: FAILED - offset of 'time' should be 0");
    assert(offsetof(tick, price) == 8 && "Test 2.2: FAILED - offset of 'price' should be 8");
    assert(offsetof(tick, volume) == 16 && "Test 2.3: FAILED - offset of 'volume' should be 16");
    std::cout << "Test 2: PASSED - struct is aligned to 1 bit" << std::endl;

    // Test 3: contiguous structs
    tick test_buffer[5];

    tick* test_addr0 = &(test_buffer[0]);
    tick* test_addr1 = &(test_buffer[1]);
    tick* test_addr2 = &(test_buffer[2]);
    tick* test_addr3 = &(test_buffer[3]);
    tick* test_addr4 = &(test_buffer[4]);

    // casting the following to char* to get byte offset, not index offset
    assert(((char*)test_addr1 - (char*)test_addr0) == 20 && "Test 3.1: FAILED - back to back not contiguous");
    assert(((char*)test_addr2 - (char*)test_addr0) == 40 && "Test 3.2: FAILED - 3 back to back not contiguous");
    assert(((char*)test_addr4 - (char*)test_addr0) == 80 && "Test 3.3: FAILED - full buffer back to back not contiguous");
    assert(sizeof(test_buffer) == 100 && "Test 3.4: FAILED - full array size not 100 bytes");
    std::cout << "Test 3: PASSED - contiguous slot of structs is aligned to 1 bit" << std::endl;

    return 0;    
}