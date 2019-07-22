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

#include <cstring>

#include "tt.h"
#include "const.h"

Table table;

void init_tt() {
    table.tt_size = one_mb * 16ULL; // 16 MB
    table.tt = (Bucket*) malloc(table.tt_size);
    table.bucket_mask = (uint64_t)(table.tt_size / sizeof(Bucket) - 1);
    table.generation = 0;
    std::memset(table.tt, 0, table.tt_size);
    // std::cout << sizeof(Bucket) << std::endl;
}

void clear_tt() {
    std::memset(table.tt, 0, table.tt_size);
    table.generation = 0;
}

void reset_tt(int megabytes) {
    table.tt_size = one_mb * (uint64_t) (megabytes);
    table.tt = (Bucket*) realloc(table.tt, table.tt_size);
    table.bucket_mask = (uint64_t)(table.tt_size / sizeof(Bucket) - 1);
    clear_tt();
}

void start_search() {
    table.generation = (table.generation + 1) % 64;
}

int score_to_tt(int score, uint16_t ply) {
    if (score >= MATE_IN_MAX_PLY) {
        return score + ply;
    } else if (score <= MATED_IN_MAX_PLY) {
        return score - ply;
    } else {
        return score;
    }
}

int tt_to_score(int score, uint16_t ply) {
    if (score >= MATE_IN_MAX_PLY) {
        return score - ply;
    } else if (score <= MATED_IN_MAX_PLY) {
        return score + ply;
    } else {
        return score;
    }
}

int hashfull() {
    int count = 0;
    for (uint64_t i = 0; i < table.bucket_mask; i += uint64_t(table.bucket_mask / 1000)) {
        Bucket *bucket = &table.tt[i];
        for (int j = 0; j < bucket_size; ++j) {
            if (tte_age(&bucket->ttes[j]) == table.generation) {
                ++count;
            }
        }
    }
    return count / bucket_size;
}

int age_diff(TTEntry *tte) {
    return (table.generation - tte_age(tte)) & 0x3F;
}

void set_tte(uint64_t hash, TTEntry *tte, Move move, int depth, int score, int static_eval, uint8_t flag) {
#ifndef __TUNE__
    uint16_t h = (uint16_t)(hash >> 48);

    if (move || h != tte->hash) {
        tte->move = move;
    }

    if (h != tte->hash || depth > tte->depth - 4) {
        assert(depth < 256 && depth > -256);
        tte->hash = h;
        tte->depth = (int8_t)depth;
        tte->score = (int16_t)score;
        tte->static_eval = (int16_t)static_eval;
        tte->ageflag = (table.generation << 2) | flag;
    }
#endif
}

TTEntry *get_tte(uint64_t hash, bool &tt_hit) {
#ifndef __TUNE__
    uint64_t index = hash & table.bucket_mask;
    Bucket *bucket = &table.tt[index];

    uint16_t h = (uint16_t)(hash >> 48);
    for (int i = 0; i < bucket_size; ++i) {
        if (!bucket->ttes[i].hash) {
            tt_hit = false;
            return &bucket->ttes[i];
        }
        if (bucket->ttes[i].hash == h) {
            tt_hit = true;
            return &bucket->ttes[i];
        }
    }

    TTEntry *replacement = &bucket->ttes[0];
    for (int i = 1; i < bucket_size; ++i) {
        if (bucket->ttes[i].depth - age_diff(&bucket->ttes[i]) * 16 < replacement->depth - age_diff(replacement) * 16) {
            replacement = &bucket->ttes[i];
        }
    }

    tt_hit = false;
    return replacement;
#else
    Bucket *bucket = &table.tt[0];
    tt_hit = false;
    return &bucket->ttes[0];
#endif
}

