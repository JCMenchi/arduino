#ifndef _SPRITES_H
#define _SPRITES_H

// Sprite dimensions
#define SPRITE_WIDTH 9
#define SPRITE_HEIGHT 8
#define SMALL_SPRITE_WIDTH 5

#include <avr/pgmspace.h>

// Empty sprite (all pixels off)
// Each byte represents a column of 8 pixels, with the least significant bit on the top
//
//    012345678
// D0 .........  1
// D1 .........  2
// D2 .........  4
// D3 .........  8
// D4 .........  1
// D5 .........  2
// D6 .........  4
// D7 .........  8
const PROGMEM uint8_t empty[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Alien sprite (type 1)
// Each byte represents a column of 8 pixels, with the least significant bit on the top
//
//    012345678
// D0 .........  1
// D1 ..O...O..  2
// D2 ...O.O...  4
// D3 ..OOOOO..  8
// D4 .OO.O.OO.  1
// D5 .O.OOO.O.  2
// D6 ..O...O..  4
// D7 ...O.O...  8
const PROGMEM uint8_t alien[] = {
    0x00, 0x30, 0x5A, 0xAC, 0x38, 0xAC, 0x5A, 0x30, 0x00
};

// Alien sprite (type 2)
// Each byte represents a column of 8 pixels, with the least significant bit on the top
//
//    012345678
// D0 .........  1
// D1 .O.O.O.O.  2
// D2 .O.O.O.O.  4
// D3 ..OOOOO..  8
// D4 ..O.O.O..  1
// D5 ..OOOOO..  2
// D6 ..O...O..  4
// D7 ...OOO...  8
const PROGMEM uint8_t alien2[] = {
    0x00, 0x06, 0x78, 0xAE, 0xB8, 0xAE, 0x78, 0x06, 0x00
};

// Spaceship sprite
// Spaceship is 9x8 pixels, represented as a byte array
// Each byte represents a column of 8 pixels, with the least significant bit on the top
//
//    012345678
// D0 ....O....  1
// D1 ....O....  2
// D2 ..O.O.O..  4
// D3 ..O.O.O..  8
// D4 ...OOO...  1
// D5 ..OOOOO..  2
// D6 .OOOOOOO.  4
// D7 .........  8
const PROGMEM uint8_t spaceship[] = {
    0x00, 0x40, 0x6C, 0x70, 0x7F, 0x70, 0x6C, 0x40, 0x00
};

// Small Spaceship sprite
// Spaceship is 5x8 pixels, represented as a byte array
// Each byte represents a column of 8 pixels, with the least significant bit on the top
//
//    012345
// D0 ......  1
// D1 ......  2
// D2 ......  4
// D3 ......  8
// D4 ......  1
// D5 ..O...  2
// D6 .OOO..  4
// D7 OO.OO.  8
const PROGMEM uint8_t small_spaceship[] = {
    0x80, 0xC0, 0x60, 0xC0, 0x80, 0x00
};

// Explosion animation frames (5 frames, 9 bytes each)
// Each byte represents a column of 8 pixels, with the least significant bit on the top
//
//      FRAME1
//    012345678
// D0 .........  1
// D1 .........  2
// D2 ..O......  4
// D3 ...OO....  8
// D4 ..OO.....  1
// D5 ....O....  2
// D6 .........  4
// D7 .........  8
//
//     FRAME2
//    012345678
// D0 .........  1
// D1 .........  2
// D2 .....O...  4
// D3 ...O..O..  8
// D4 .....O...  1
// D5 ...OO....  2
// D6 .....O...  4
// D7 .........  8
//
//     FRAME3
//    012345678
// D0 .........  1
// D1 ..O.O..O.  2
// D2 .....OO..  4
// D3 ...OO..O.  8
// D4 ....O.O..  1
// D5 ..O..OO..  2
// D6 ....O..O.  4
// D7 .........  8
//
//     FRAME4
//    012345678
// D0 .........  1
// D1 ..OO...O.  2
// D2 ......O..  4
// D3 ...OOO...  8
// D4 ....OOO..  1
// D5 ...O..O..  2
// D6 ..O.O..O.  4
// D7 .........  8
//
//     FRAME5
//    012345678
// D0 .........  1
// D1 .........  2
// D2 .........  4
// D3 .........  8
// D4 .........  1
// D5 .........  2
// D6 .........  4
// D7 .........  8
const PROGMEM uint8_t explosion_frames[] = {
    // Frame 1
    0x00, 0x00, 0x00, 0x14, 0x18, 0x28, 0x00, 0x00, 0x00, 
    // Frame 2
    0x00, 0x00, 0x00, 0x28, 0x20, 0x54, 0x08, 0x00, 0x00, 
    // Frame 3
    0x00, 0x00, 0x22, 0x08, 0x5A, 0x24, 0x34, 0x4A, 0x00,
    // Frame 4
    0x00, 0x00, 0x42, 0x2A, 0x58, 0x18, 0x34, 0x42, 0x00,
    // Frame 5
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Shelter sprite width in pixels
#define SHELTER_WIDTH 14

// Shelter sprite (used as player cover)
// Each byte represents a column of 8 pixels, with the least significant bit on the top
//
//    01234567890123
// D0 ..............  1
// D1 ...OOOOOOOO...  2
// D2 ..OOOOOOOOOO..  4
// D3 .OOOOOOOOOOOO.  8
// D4 OOOOOOOOOOOOOO  1
// D5 OOOOOOOOOOOOOO  2
// D6 ..............  4
// D7 ..............  8
const PROGMEM uint8_t shelter[] = {
    0x30, 0x38, 0x3C, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3C, 0x38, 0x30
};

#endif