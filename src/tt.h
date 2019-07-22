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

#ifndef TT_H
#define TT_H

#include "data.h"

void init_tt();
void clear_tt();
void reset_tt(int megabytes);

const int bucket_size = 3;
const uint64_t one_mb = 1024ULL * 1024ULL;

typedef struct TTEntry {
    uint16_t hash;
    Move     move;
    int16_t  score;
    int16_t  static_eval;
    uint8_t  ageflag;
    int8_t   depth;
} TTEntry;

typedef struct Bucket {
    TTEntry ttes[bucket_size];
    char padding[2]; // Totaling 32 bytes
} Bucket;

typedef struct Table {
    Bucket *tt;
    uint8_t generation;
    uint64_t tt_size;
    uint64_t bucket_mask;
} Table;

inline uint8_t tte_flag(TTEntry *tte) {
    return (uint8_t) (tte->ageflag & 0x3);
}

inline uint8_t tte_age(TTEntry *tte) {
    return (uint8_t) (tte->ageflag >> 2);
}

int hashfull();
void start_search();
void set_tte(uint64_t hash, TTEntry *tte, Move m, int depth, int score, int static_eval, uint8_t flag);
TTEntry *get_tte(uint64_t hash, bool &tt_hit);

int score_to_tt(int score, uint16_t ply);
int tt_to_score(int score, uint16_t ply);

#endif
