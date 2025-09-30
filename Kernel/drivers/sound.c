#include <sound.h>
#include <interrupts.h>
#include <time.h>
#include <fonts.h>

/*

https://wiki.osdev.org/Programmable_Interval_Timer

"""
For the "lobyte/hibyte" mode, 16 bits are always transferred as a pair, with the lowest 8 bits followed by the highest 8 bits (both 8 bit transfers are to the same IO port, sequentially – a word transfer will not work).
"""

0xb6 -> 0b010110110
0 (unused?)
0 1 = Channel 1
1 1 = Access mode: lobyte/hibyte
0 1 1 = Mode 3 (square wave generator)

*/

void play_sound(uint32_t nFrequence) {
    uint32_t div = 1193180 / nFrequence;

    setPITMode(0xb6);
    setPITFrequency(div);

    setSpeaker(SPEAKER_ON);
}
