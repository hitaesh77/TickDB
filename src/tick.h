#ifndef TICK_H
#define TICK_H

#include <cstdint>
#include <cstring>

// pragma pack(push, n) sets packing alignment to n bytes. forces compiler to align members 
// on n-byte bnoundaries or their natural alignment (whichever smaller)
// push save the current alignment setting on compiler stack then sets a new alignment
// pop restores the packing alignment to state it was in before alst push operation

// #pragma pack(push, 1)
struct Tick {
    uint64_t time;
    double price;
    uint32_t volume;
};
// #pragma pop()

// use formal encode and decode instead of pragma 
// lightweight inline functions instead of a separate for cpp file, for funsies!
inline void encode_tick_20(const Tick& tick, uint8_t* data_out) {
    // bits 0 to 7
    std::memcpy(data_out, &(tick.time), sizeof(tick.time));

    // bits 8 to 15
    std::memcpy(data_out + 8, &(tick.price), sizeof(tick.price));

    // bits 16 to 19
    std::memcpy(data_out + 16, &(tick.volume), sizeof(tick.volume));
}

inline Tick decode_tick_20(uint8_t* data_in) {
    Tick result;

    std::memcpy(&(result.time), data_in, sizeof(result.time));
    std::memcpy(&(result.price), data_in + 8, sizeof(result.price));
    std::memcpy(&(result.volume), data_in + 16, sizeof(result.volume));

    return result;
}

#endif