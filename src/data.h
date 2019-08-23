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

#ifndef DATA_H
#define DATA_H

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cstdlib>

#include "const.h"
#include "params.h"

extern Bitboard BISHOP_MASKS_1[64];
extern Bitboard BISHOP_MASKS_2[64];
extern Bitboard BISHOP_MASKS_COMBINED[64];
extern Bitboard ROOK_MASKS_HORIZONTAL[64];
extern Bitboard ROOK_MASKS_VERTICAL[64];
extern Bitboard ROOK_MASKS_COMBINED[64];
extern Bitboard KNIGHT_MASKS[64];
extern Bitboard KING_MASKS[64];
extern Bitboard KING_EXTENDED_MASKS[2][64];
extern Bitboard bfi[65];
extern Bitboard bfi_queen_castle[2];
extern Bitboard bfi_king_castle[2];
extern Bitboard KING_CASTLE_MASK[2];
extern Bitboard KING_CASTLE_MASK_THREAT[2];
extern Bitboard QUEEN_CASTLE_MASK[2];
extern Bitboard QUEEN_CASTLE_MASK_THREAT[2];
extern Bitboard PAWN_ADVANCE_MASK_1[64][2];
extern Bitboard PAWN_ADVANCE_MASK_2[64][2];
extern Bitboard PAWN_CAPTURE_MASK[64][2];
extern Bitboard BETWEEN_MASK[64][64];
extern Bitboard FROMTO_MASK[64][64];
extern Bitboard PASSED_PAWN_MASK[64][2];
extern Bitboard FRONT_MASK[64][2];
extern Bitboard ADJACENT_MASK[64];
extern Bitboard DISTANCE_RING[64][8];
extern Bitboard FRONT_RANK_MASKS[64][8];

extern Square ENPASSANT_INDEX[64];
extern uint64_t castlingHash[16];

extern uint8_t CASTLING_RIGHTS[64];
extern uint8_t ROOK_MOVES_CASTLE_FROM[64];
extern uint8_t ROOK_MOVES_CASTLE_TO[64];
extern uint8_t ROOK_MOVES_CASTLE_PIECE[64];

void init();
void init_imbalance();
extern int ours[5][5];
extern int theirs[5][5];
extern int pawn_set[9];

extern int mvvlva_values[12][NUM_PIECE];

extern Material material_base[9*3*3*3*2*9*3*3*3*2];

inline Material *get_material(Position *p) { return &material_base[p->info->material_index]; }
inline PawnTTEntry *get_pawntte(Position *p) { return &p->my_thread->pawntt[p->info->pawn_hash & (pawntt_size - 1)]; }

bool scored_move_compare(ScoredMove lhs, ScoredMove rhs);
bool scored_move_compare_greater(ScoredMove lhs, ScoredMove rhs);

inline bool is_main_thread(Position *p) {return p->my_thread->thread_id == 0;}

extern SearchThread main_thread;
extern SearchThread *search_threads;

inline SearchThread *get_thread(int thread_id) { return thread_id == 0 ? &main_thread : &search_threads[thread_id - 1]; }

extern int num_threads;
extern int move_overhead;

inline void initialize_nodes() {
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = get_thread(i);
        t->nodes = 0;
        t->tb_hits = 0;
    }
}

inline uint64_t sum_nodes() {
    uint64_t s = 0;
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = get_thread(i);
        s += t->nodes;
    }
    return s;
}

inline uint64_t sum_tb_hits() {
    uint64_t s = 0;
    for (int i = 0; i < num_threads; ++i) {
        SearchThread *t = get_thread(i);
        s += t->tb_hits;
    }
    return s;
}

extern int reductions[2][64][64];

inline Move _movecast(Square from, Square to, Square type) {
    // | from (6) | to (6) | type (4) |
    return (from << 10) | (to << 4) | type;
}

inline Color piece_color(Piece p) {return Color(p & 1);}

inline bool is_white(Piece p) {return piece_color(p) == white;}
inline bool is_black(Piece p) {return piece_color(p) == black;}

inline int piece_type(Piece p) {return int(p / 2);}

inline Piece king(Color color) {return white_king | color;}
inline Piece queen(Color color) {return white_queen | color;}
inline Piece rook(Color color) {return white_rook | color;}
inline Piece bishop(Color color) {return white_bishop | color;}
inline Piece knight(Color color) {return white_knight | color;}
inline Piece pawn(Color color) {return white_pawn | color;}

inline Piece occupy(Color color) {return white_occupy | color;}

inline bool is_king(Piece p) {return piece_type(p) == KING;}
inline bool is_queen(Piece p) {return piece_type(p) == QUEEN;}
inline bool is_rook(Piece p) {return piece_type(p) == ROOK;}
inline bool is_bishop(Piece p) {return piece_type(p) == BISHOP;}
inline bool is_knight(Piece p) {return piece_type(p) == KNIGHT;}
inline bool is_pawn(Piece p) {return piece_type(p) == PAWN;}

inline Square move_from(Move m) {return m >> 10;}
inline Square move_to(Move m) {return (m >> 4) & 63;}

// Both no_move and null_move have the same from/to values
inline bool is_move_valid(Move m) {return move_from(m) != move_to(m);}

inline int rank_of(Square s) {return s >> 3;}
inline int file_of(Square s) {return s & 7;}

inline int relative_rank(Square s, Color color) {return rank_of(s) ^ (color * 7);}

inline int distance(Square s1, Square s2) {
    int col_distance = std::abs(file_of(s1) - file_of(s2));
    int row_distance = std::abs(rank_of(s1) - rank_of(s2));
    return std::max(col_distance, row_distance);
}

inline Square mirror_square(Square s, Color color) {return s ^ (color * H8);}
inline Square relative_square(Square s, Color color) {return s ^ (color * A8);}

inline Square pawn_forward(Square s, Color color) {return color == white ? s + 8 : s - 8;}
inline Square pawn_backward(Square s, Color color) {return color == white ? s - 8 : s + 8;}

inline int move_type(Move m) {return m & 3;}

inline bool is_promotion(Move m) {return (move_type(m) == PROMOTION);}
inline bool is_queen_promotion(Move m){return (m & 12) == PROMOTION_Q;}
inline bool is_rook_promotion(Move m){return (m & 12) == PROMOTION_R;}
inline bool is_bishop_promotion(Move m){return (m & 12) == PROMOTION_B;}
inline bool is_knight_promotion(Move m){return (m & 12) == PROMOTION_N;}

inline bool opposite_colors(Square s1, Square s2) {
    int s = s1 ^ s2;
    return ((s >> 3) ^ s) & 1;
}

inline int promotion_type(Move m) {return m & 12;}
inline Piece get_promotion_piece(Move m, Color c){
    int promotion = promotion_type(m);
    if (promotion == 12) {
        return white_queen + c;
    } else if (promotion == 0) {
        return white_knight + c;
    } else if (promotion == 4) {
        return white_bishop + c;
    } else if (promotion == 8) {
        return white_rook + c;
    }
    assert(false);
    return white_queen + c;
}

extern uint64_t polyglotCombined[NUM_PIECE][64];

#endif
