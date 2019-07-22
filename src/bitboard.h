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

#ifndef BITBOARD_H
#define BITBOARD_H

#include <cmath>

#include "data.h"

inline int count(Bitboard b) { return __builtin_popcountll(b); }

inline Square lsb(Bitboard b) { assert(b != 0); return (Square)(__builtin_ctzll(b)); }
inline Square msb(Bitboard b) { assert(b != 0); return (Square)(63 ^ __builtin_clzll(b)); }

inline Square pop(Bitboard *b) {
    const Square sq = lsb(*b);
    *b &= *b - 1;
    return sq;
}

inline bool on(Bitboard b, Square index) {
    return b & bfi[index];
}

char *bitstring(Bitboard b);

inline Bitboard on_(Bitboard a, Bitboard b){
    return a ^ b;
}

inline bool more_than_one(Bitboard b) {
  return b & (b - 1);
}

inline Bitboard shift(Bitboard b, int offset) {
    if (offset > 0) {
        return b << offset;
    }
    return b >> -offset;
}

Bitboard reverse(Bitboard a);
Bitboard trim(Bitboard b, int r, int f);

#endif
