#pragma once

#define N_INTERVALS 5

uint8_t intervals[] = {
    5, 0,2,5,7,9, // Pentatonic
    8, 0,1,3,4,6,7,9,10, // Octatonic
    7, 0,2,4,5,7,9,11, // Diatonic
    7, 0,3,4,5,8,9,11, // Mid-Eastern
    12, 0,1,2,3,4,5,6,7,8,9,10,11, // Chromatic
};

uint8_t chords[] = {
    0,0,3,7,
    0,0,4,6,
    0,0,4,7,
    0,0,4,13,
    0,0,4,9
};

uint16_t interval_look_up[N_INTERVALS];

void intervals_init() {
    uint8_t i = 0;
    uint16_t j = 0;
    uint16_t offset = 0;
    while (i < N_INTERVALS) {
        interval_look_up[i] = offset;
        offset += (intervals[offset] + 1);
        i++;
    }    
}

inline uint8_t getNote(uint8_t scale, uint8_t root, uint8_t val) {
    uint8_t ofs = interval_look_up[scale];
    uint8_t len = intervals[ofs];
    uint8_t oct = val / len;
    return (root + (oct * 12) + intervals[ofs + 1 + (val % len)]);
}

inline uint8_t getIntervalSize(uint8_t scale) {
    return (intervals[interval_look_up[scale]] * 3);
}