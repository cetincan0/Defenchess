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

#include <random>

#include "bitboard.h"
#include "data.h"
#include "magic.h"
#include "pst.h"
#include "test.h"
#include "thread.h"
#include "tt.h"

Bitboard BISHOP_MASKS_1[64];
Bitboard BISHOP_MASKS_2[64];
Bitboard BISHOP_MASKS_COMBINED[64];
Bitboard ROOK_MASKS_HORIZONTAL[64];
Bitboard ROOK_MASKS_VERTICAL[64];
Bitboard ROOK_MASKS_COMBINED[64];
Bitboard KNIGHT_MASKS[64];
Bitboard KING_MASKS[64];
Bitboard KING_EXTENDED_MASKS[2][64];
Bitboard bfi[65];
Bitboard bfi_queen_castle[2];
Bitboard bfi_king_castle[2];
Bitboard PAWN_ADVANCE_MASK_1[64][2];
Bitboard PAWN_ADVANCE_MASK_2[64][2];
Bitboard PAWN_CAPTURE_MASK[64][2];
Bitboard BETWEEN_MASK[64][64];
Bitboard BETWEEN_MASK_INCLUSIVE[64][64];
Bitboard FROMTO_MASK[64][64];
Bitboard PASSED_PAWN_MASK[64][2];
Bitboard FRONT_MASK[64][2];
Bitboard ADJACENT_MASK[64];
Bitboard DISTANCE_RING[64][8];
Bitboard FRONT_RANK_MASKS[64][8];

Material material_base[9*3*3*3*2*9*3*3*3*2];

int CASTLE_TYPE[64];
Square ROOK_MOVES_CASTLE_TO[64];

Square ENPASSANT_INDEX[64];
uint64_t castling_hash[16];
uint64_t enpassant_hash[8];
uint64_t white_hash;
uint64_t hash_combined[NUM_PIECE][64];

int num_threads = 1;
int move_overhead = 100;
bool chess960 = false;

SearchThread main_thread;
SearchThread *search_threads;

int mvvlva_values[12][NUM_PIECE];

int reductions[2][64][64];

Bitboard knight_king_possibles(Square sq) {
    switch(file_of(sq)){
        case FILE_A:
            return FILE_ABB | FILE_BBB | FILE_CBB;
        case FILE_B:
            return FILE_ABB | FILE_BBB | FILE_CBB | FILE_DBB;
        case FILE_G:
            return FILE_EBB | FILE_FBB | FILE_GBB | FILE_HBB;
        case FILE_H:
            return FILE_FBB | FILE_GBB | FILE_HBB;
        default:
            return ~0ULL;
    }
}

void init_fromto() {
    for (Square i = A1; i <= H8; ++i) {
        for (Square j = A1; j <= H8; ++j) {
            if (i == j) {
                FROMTO_MASK[i][j] = 0;
                continue;
            }
            if (file_of(i) == file_of(j)) {
                FROMTO_MASK[i][j] = FILE_MASK[file_of(i)];
            } else if (rank_of(i) == rank_of(j)) {
                FROMTO_MASK[i][j] = RANK_MASK[rank_of(i)];
            } else if (rank_of(i) - rank_of(j) == file_of(i) - file_of(j) || rank_of(i) - rank_of(j) == file_of(j) - file_of(i)) {
                if (i > j) {
                    FROMTO_MASK[i][j] = file_of(i) < file_of(j) ? BISHOP_MASKS_1[i] : BISHOP_MASKS_2[i];
                } else {
                    FROMTO_MASK[i][j] = file_of(i) > file_of(j) ? BISHOP_MASKS_1[i] : BISHOP_MASKS_2[i];
                }
            } else {    
                FROMTO_MASK[i][j] = 0;
            }
        }
    }
}

void init_between() {
    for (Square i = A1; i <= H8; ++i) {
        for (Square j = A1; j <= H8; ++j) {
            if (i == j) {
                BETWEEN_MASK[i][j] = 0;
                BETWEEN_MASK_INCLUSIVE[i][j] = bfi[i];
                continue;
            }
            if (file_of(i) == file_of(j)) {
                if (i > j) {
                    BETWEEN_MASK[i][j] = (bfi[i] - 2 * bfi[j]) & FILE_MASK[file_of(i)];
                } else {
                    BETWEEN_MASK[i][j] = (bfi[j] - 2 * bfi[i]) & FILE_MASK[file_of(i)];
                }
            } else if (rank_of(i) == rank_of(j)) {
                if (i > j) {
                    BETWEEN_MASK[i][j] = bfi[i] - 2 * bfi[j];
                } else {
                    BETWEEN_MASK[i][j] = bfi[j] - 2 * bfi[i];
                }
            } else if (rank_of(i) - rank_of(j) == file_of(i) - file_of(j) || rank_of(i) - rank_of(j) == file_of(j) - file_of(i)) {
                if (i > j) {
                    BETWEEN_MASK[i][j] = (bfi[i] - 2 * bfi[j]) & (file_of(i) < file_of(j) ? BISHOP_MASKS_1[i] : BISHOP_MASKS_2[i]);
                } else {
                    BETWEEN_MASK[i][j] = (bfi[j] - 2 * bfi[i]) & (file_of(i) > file_of(j) ? BISHOP_MASKS_1[i] : BISHOP_MASKS_2[i]);
                }
            } else {    
                BETWEEN_MASK[i][j] = 0;
            }
            BETWEEN_MASK_INCLUSIVE[i][j] = BETWEEN_MASK[i][j] | bfi[i] | bfi[j];
        }
    }
}

void init_pawns(){
    for (Square sq = A1; sq <= H8; ++sq) {
        PAWN_ADVANCE_MASK_1[sq][white] = 0;
        PAWN_ADVANCE_MASK_2[sq][white] = 0;
        PAWN_CAPTURE_MASK[sq][white] = 0;
        PAWN_ADVANCE_MASK_1[sq][black] = 0;
        PAWN_ADVANCE_MASK_2[sq][black] = 0;
        PAWN_CAPTURE_MASK[sq][black] = 0;
    }
    for (Square sq = A2; sq <= H7; ++sq) {
        PAWN_ADVANCE_MASK_1[sq][white] = bfi[sq + 8];
        PAWN_ADVANCE_MASK_1[sq][black] = bfi[sq - 8];
        if (rank_of(sq) == RANK_2) {
            PAWN_ADVANCE_MASK_2[sq][white] |= bfi[sq + 16];
        }
        if (rank_of(sq) == RANK_7) {
            PAWN_ADVANCE_MASK_2[sq][black] |= bfi[sq - 16];
        }
    }
    for (Square sq = A1; sq <= H8; ++sq) {
        PAWN_CAPTURE_MASK[sq][white] = 0;
        PAWN_CAPTURE_MASK[sq][black] = 0;
        int file = file_of(sq);
        if (file != FILE_A) {
            if (sq + 7 <= H8) {
                PAWN_CAPTURE_MASK[sq][white] |= bfi[sq + 7];
            }
            if (sq - 9 >= A1) {
                PAWN_CAPTURE_MASK[sq][black] |= bfi[sq - 9];
            }
        }
        if (file != FILE_H) {
            if (sq + 9 <= H8) {
                PAWN_CAPTURE_MASK[sq][white] |= bfi[sq + 9];
            }
            if (sq - 7 >= A1) {
                PAWN_CAPTURE_MASK[sq][black] |= bfi[sq - 7];
            }
        }
    }
}

void init_enpassants(){
    for (Square sq = A1; sq <= H8; ++sq) {
        if (sq >= A2 && sq <= H3) {
            ENPASSANT_INDEX[sq] = sq + 8;
        }
        if (sq >= A6 && sq <= H7) {
            ENPASSANT_INDEX[sq] = sq - 8;
        }
    }
}

void init_knight_masks() {
    for (Square sq = A1; sq <= H8; ++sq) {
        KNIGHT_MASKS[sq] = shift(_knight_targets, sq - 21) & knight_king_possibles(sq);
    }
}

void init_king_masks(){
    for (Square sq = A1; sq <= H8; ++sq) {
        KING_MASKS[sq] = shift(_king_targets, sq - 21) & knight_king_possibles(sq);
    }
}

void init_bishop_masks() {
    for (Square sq = A1; sq <= H8; ++sq) {
        for (int j = 0; j < 15; ++j) {
            if (bfi[sq] & cross_lt[j]) {
                BISHOP_MASKS_1[sq] = cross_lt[j];
            }
            if (bfi[sq] & cross_rt[j]) {
                BISHOP_MASKS_2[sq] = cross_rt[j];
            }
            
        }
    }
    for (Square sq = A1; sq <= H8; ++sq){
        BISHOP_MASKS_COMBINED[sq] = BISHOP_MASKS_1[sq] | BISHOP_MASKS_2[sq];
    }
}

void init_rook_masks() {
    for (Square sq = A1; sq <= H8; ++sq) {
       ROOK_MASKS_HORIZONTAL[sq] = RANK_MASK[rank_of(sq)];
       ROOK_MASKS_VERTICAL[sq] = FILE_MASK[file_of(sq)];
       ROOK_MASKS_COMBINED[sq] = ROOK_MASKS_VERTICAL[sq] | ROOK_MASKS_HORIZONTAL[sq];
    }
}

void init_bfi() {
    for (Square sq = A1; sq <= H8; ++sq) {
        bfi[sq] = 1ULL << sq;
    }

    bfi[no_sq] = 0;
}

void init_castles() {
    bfi_queen_castle[white] = bfi[C1];
    bfi_queen_castle[black] = bfi[C8];
    bfi_king_castle[white] = bfi[G1];
    bfi_king_castle[black] = bfi[G8];

    CASTLE_TYPE[G1] = KINGSIDE;
    CASTLE_TYPE[C1] = QUEENSIDE;
    CASTLE_TYPE[G8] = KINGSIDE;
    CASTLE_TYPE[C8] = QUEENSIDE;

    ROOK_MOVES_CASTLE_TO[G1] = F1;
    ROOK_MOVES_CASTLE_TO[C1] = D1;
    ROOK_MOVES_CASTLE_TO[G8] = F8;
    ROOK_MOVES_CASTLE_TO[C8] = D8;
}

void init_hash() {
    std::mt19937_64 r(0);

    uint64_t castling_hash_array[4];

    for (int i = 0; i < NUM_PIECE; i++) {
        for (int j = 0; j < 64; j++) {
            hash_combined[i][j] = r();
        }
    }

    for (int i = 0; i < 4; ++i) {
        castling_hash_array[i] = r();
    }

    for (int i = 0; i < 8; ++i) {
        enpassant_hash[i] = r();
    }

    white_hash = r();

    for (int castle_mask = 0; castle_mask < 16; castle_mask++) {
        if (castle_mask & can_king_castle_mask[white]) { // White king-side
            castling_hash[castle_mask] ^= castling_hash_array[0];
        }
        if (castle_mask & can_queen_castle_mask[white]) { // White queen-side
            castling_hash[castle_mask] ^= castling_hash_array[1];
        }
        if (castle_mask & can_king_castle_mask[black]) { // Black king-side
            castling_hash[castle_mask] ^= castling_hash_array[2];
        }
        if (castle_mask & can_queen_castle_mask[black]) { // Black queen-side
            castling_hash[castle_mask] ^= castling_hash_array[3];
        }
    }
}

void init_passed_pawns() {
    Bitboard PASSED_PAWN_HORIZONTAL[64][2];
    for (Square sq = A1; sq <= H8; ++sq) {
        PASSED_PAWN_HORIZONTAL[sq][white] = 0;
        PASSED_PAWN_HORIZONTAL[sq][black] = 0;
        for (Square j = sq + 8; j <= H8; j += 8) {
            PASSED_PAWN_HORIZONTAL[sq][white] |= ROOK_MASKS_HORIZONTAL[j];
        }
        for (Square j = sq - 8; j >= A1; j -= 8) {
            PASSED_PAWN_HORIZONTAL[sq][black] |= ROOK_MASKS_HORIZONTAL[j];
        }
    }
    for (Square sq = A1; sq <= H8; ++sq) {
        PASSED_PAWN_MASK[sq][white] = ROOK_MASKS_VERTICAL[sq];
        PASSED_PAWN_MASK[sq][black] = ROOK_MASKS_VERTICAL[sq];
        if (file_of(sq) != FILE_A) {
            PASSED_PAWN_MASK[sq][white] |= ROOK_MASKS_VERTICAL[sq - 1];
            PASSED_PAWN_MASK[sq][black] |= ROOK_MASKS_VERTICAL[sq - 1];
        }
        if (file_of(sq) != FILE_H) {
            PASSED_PAWN_MASK[sq][white] |= ROOK_MASKS_VERTICAL[sq + 1];
            PASSED_PAWN_MASK[sq][black] |= ROOK_MASKS_VERTICAL[sq + 1];
        }
        PASSED_PAWN_MASK[sq][white] &= PASSED_PAWN_HORIZONTAL[sq][white];
        PASSED_PAWN_MASK[sq][black] &= PASSED_PAWN_HORIZONTAL[sq][black];
    }
}

void init_pawn_masks() {
    for (Square sq = A1; sq <= H8; ++sq) {
        FRONT_MASK[sq][white] = 0;
        FRONT_MASK[sq][black] = 0;

        for (Square j = sq + 8; j <= H8; j += 8) {
            FRONT_MASK[sq][white] |= bfi[j];
        }
        for (Square j = sq - 8; j >= A1; j -= 8) {
            FRONT_MASK[sq][black] |= bfi[j];
        }
    }
}

void init_adj(){
    for (Square sq = A1; sq <= H8; ++sq) {
        if (file_of(sq) == FILE_A) {
            ADJACENT_MASK[sq] = bfi[sq+1];
        } else if (file_of(sq) == FILE_H) {
            ADJACENT_MASK[sq] = bfi[sq-1];
        } else {
            ADJACENT_MASK[sq] = bfi[sq-1] | bfi[sq+1];
        }
    }
}

void init_king_extended(){
    for (Square sq = A1; sq <= H8; ++sq) {
        KING_EXTENDED_MASKS[white][sq] = (KING_MASKS[sq] | (KING_MASKS[sq] << 8)) & ~bfi[sq];
        KING_EXTENDED_MASKS[black][sq] = (KING_MASKS[sq] | (KING_MASKS[sq] >> 8)) & ~bfi[sq];
    }
}

int my_pieces[5][5] = {
    // pawn knight bishop rook queen
    {    16                          }, // Pawn
    {   146,   -40                   }, // Knight
    {    55,    12,     7            }, // Bishop
    {    32,    31,    61, -81       }, // Rook
    {    12,    80,    75, -35,   12 }  // Queen
};

int opponent_pieces[5][5] = {
    // pawn knight bishop rook queen
    {     0                          }, // Pawn
    {    42,     0                   }, // Knight
    {    40,    12,     0            }, // Bishop
    {    50,     9,   -11,   0       }, // Rook
    {    88,   -28,    60,  96,    0 }  // Queen
};

int imbalance(const int piece_count[5][5], Color color) {
    int bonus = 0;

    // Second-degree polynomial material imbalance, by Tord Romstad
    for (int pt1 = 0; pt1 < 5; ++pt1) {
        if (!piece_count[color][pt1]) {
            continue;
        }

        for (int pt2 = 0; pt2 <= pt1; ++pt2) {
            bonus += my_pieces[pt1][pt2] * piece_count[color][pt1] * piece_count[color][pt2] +
                     opponent_pieces[pt1][pt2] * piece_count[color][pt1] * piece_count[~color][pt2];
        }
    }

    return bonus;
}

void init_imbalance(){
    for (int wp = 0 ; wp < 9 ; wp++) {
        for (int wn = 0 ; wn < 3 ; wn++) {
            for (int wb = 0 ; wb < 3 ; wb++) {
                for (int wr = 0 ; wr < 3 ; wr++) {
                    for (int wq = 0 ; wq < 2 ; wq++){
                        for (int bp = 0 ; bp < 9 ; bp++) {
                            for (int bn = 0 ; bn < 3 ; bn++) {
                                for (int bb = 0 ; bb < 3 ; bb++) {
                                    for (int br = 0 ; br < 3 ; br++) {
                                        for (int bq = 0 ; bq < 2 ; bq++){

        int index = wq * material_balance[white_queen]  +
                    bq * material_balance[black_queen]  +
                    wr * material_balance[white_rook]   +
                    br * material_balance[black_rook]   +
                    wb * material_balance[white_bishop] +
                    bb * material_balance[black_bishop] +
                    wn * material_balance[white_knight] +
                    bn * material_balance[black_knight] +
                    wp * material_balance[white_pawn]   +
                    bp * material_balance[black_pawn];
        Material *material = &material_base[index];
        material->phase = std::max(0, (11 * (wn + wb + bn + bb) + 22 * (wr + br) + 40 * (wq + bq) - 48)) * 16 / 13;

        const int piece_count[2][5] = {
            { wp, wn, wb, wr, wq },
            { bp, bn, bb, br, bq }
        };

        material->score = (imbalance(piece_count, white) - imbalance(piece_count, black)) / 16;

        // Bishop pair
        if (wb > 1) {
            material->score += bishop_pair;
        }
        if (bb > 1) {
            material->score -= bishop_pair;
        }

        // Endgames
        int white_minor = wn + wb;
        int white_major = wr + wq;
        int black_minor = bn + bb;
        int black_major = br + bq;
        int all_minor = white_minor + black_minor;
        int all_major = white_major + black_major;
        bool no_pawns = wp == 0 && bp == 0;

        material->endgame_type = NORMAL_ENDGAME;

        if (wp + bp + all_minor + all_major == 0) {
            material->endgame_type = DRAW_ENDGAME;
        }
        else if (no_pawns && all_major == 0 && white_minor < 2 && black_minor < 2) {
            material->endgame_type = DRAW_ENDGAME;
        }
        else if (no_pawns && all_major == 0 && all_minor == 2 && (wn == 2 || bn == 2)) {
            material->endgame_type = DRAW_ENDGAME;
        }
                                        }
                                    }
                                }
                            }
                        }  
                    }
                }
            }
        }
    }
}

void init_distance() {
    for (Square s1 = A1; s1 <= H8; ++s1) {
        for (Square s2 = A1; s2 <= H8; ++s2) {
            if (s1 != s2)
            {
                int col_distance = std::abs(file_of(s1) - file_of(s2));
                int row_distance = std::abs(rank_of(s1) - rank_of(s2));
                int distance = std::max(col_distance, row_distance);
                DISTANCE_RING[s1][distance - 1] |= bfi[s2];
            }
        }
    }
}

void init_lmr() {
    for (int depth = 1; depth < 64; ++depth) {
        for (int num_moves = 1; num_moves < 64; ++num_moves) {
            reductions[0][depth][num_moves] = int(std::round(log(depth) * log(num_moves) / 2.0));
            reductions[1][depth][num_moves] = std::max(reductions[0][depth][num_moves] - 1, 0);
        }
    }
}

void init_front_rank_masks() {
    for (Square sq = A1; sq <= H8; ++sq) {
        int my_rank = relative_rank(sq, white);

        for (int r = my_rank + 1; r <= RANK_8; ++r) {
            FRONT_RANK_MASKS[sq][white] |= RANK_MASK[r];
        }

        for (int r = my_rank - 1; r >= RANK_1; --r) {
            FRONT_RANK_MASKS[sq][black] |= RANK_MASK[r];
        }
    }
}

void init_masks() {
    init_bfi();
    init_rook_masks();
    init_king_masks();
    init_knight_masks();
    init_bishop_masks();
    init_rook_masks();
    init_castles();
    init_enpassants();
    init_between();
    init_fromto();
    init_pawns();
    init_values();
    init_hash();
    init_passed_pawns();
    init_pawn_masks();
    init_adj();
    init_king_extended();
    init_distance();
    init_lmr();
    init_front_rank_masks();
}

void init() {
    init_threads();
    init_masks();
    init_tt();
    init_magic();
    init_imbalance();
}
