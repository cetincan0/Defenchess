/*
    Defenchess, a chess engine
    Copyright 2017-2019 Can Cetin, Dogac Eldenk

    Defenchess is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Defenchess is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Defenchess.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "magic.h"
#include "bitboard.h"

Bitboard rook_magic_moves[64][4096];
Bitboard bishop_magic_moves[64][512];

void init_magic(){
    generate_magic_rook_moves();
    generate_magic_bishop_moves();
}

Bitboard magicify(uint64_t square, Bitboard b) {
    int total = count(b);
    Bitboard x = b, bitmask = 0;

    for (int i = 0; i < total; ++i) {
        Bitboard y = ((x - 1) & x) ^ x;
        x &= x - 1;
        if (bfi[i] & square) {
            bitmask |= y;
        }
    }
    return bitmask;
}

Bitboard create_rook_attacks(int sq, Bitboard b) {
    Bitboard m1 = ROOK_MASKS_HORIZONTAL[sq];
    Bitboard line_attacks = (((b & m1) - 2 * bfi[sq]) ^ 
        (reverse(reverse(b & m1) - 2 * reverse(bfi[sq])))) & m1;
     
    Bitboard m2 = ROOK_MASKS_VERTICAL[sq];
    Bitboard vertical_attacks = (((b & m2) - 2 * bfi[sq]) ^ 
        (reverse(reverse(b & m2) - 2 * reverse(bfi[sq])))) & m2;

    return vertical_attacks | line_attacks;
}

Bitboard create_bishop_attacks(int sq, Bitboard b) {
    Bitboard m1 = BISHOP_MASKS_1[sq];
    Bitboard line_attacks = (((b & m1) - 2 * bfi[sq]) ^ 
        (reverse(reverse(b & m1) - 2 * reverse(bfi[sq])))) & m1;
     
    Bitboard m2 = BISHOP_MASKS_2[sq];
    Bitboard vertical_attacks = (((b & m2) - 2 * bfi[sq]) ^ 
        (reverse(reverse(b & m2) - 2 * reverse(bfi[sq])))) & m2;

    return vertical_attacks | line_attacks;
}

void generate_magic_rook_moves() {
    for (int sq = A1; sq <= H8; ++sq){
        Bitboard mask = trim(ROOK_MASKS_COMBINED[sq] ^ bfi[sq], rank_of(sq), file_of(sq));
        for (uint64_t i = 0; i < bfi[count(mask)]; ++i) {
            Bitboard bitmask = magicify(i, mask);
            int index = (bitmask * rookMagic[sq].magic) >> 52;
            rook_magic_moves[sq][index] = create_rook_attacks(sq, bitmask);
        }
    } 
}

void generate_magic_bishop_moves() {
    for (int sq = A1; sq <= H8; ++sq){
        Bitboard mask = trim(BISHOP_MASKS_COMBINED[sq] ^ bfi[sq], rank_of(sq), file_of(sq));;
        for (uint64_t i = 0; i < bfi[count(mask)]; ++i) {
            Bitboard bitmask = magicify(i, mask);
            int index = (bitmask * bishopMagic[sq].magic) >> 55;
            bishop_magic_moves[sq][index] = create_bishop_attacks(sq, bitmask);
        }
    }
}
