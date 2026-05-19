#include <cstdint>

// pragma pack(push, n) sets packing alignment to n bytes. forces compiler to align members 
// on n-byte bnoundaries or their natural alignment (whichever smaller)
// push save the current alignment setting on compiler stack then sets a new alignment
// pop restores the packing alignment to state it was in before alst push operation

#pragma pack(push, 1)
struct Tick {
    uint64_t time;
    double price;
    uint32_t volume;
};
#pragma pop()