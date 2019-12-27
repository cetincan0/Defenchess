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

#ifndef CONST_H
#define CONST_H

#include <atomic>
#include <cassert>
#include <cstdint>

typedef int8_t Square;
typedef uint64_t Bitboard;
typedef uint8_t Piece;
typedef uint16_t Move;

enum Color {
    white, black
};

inline Color& operator++(Color& c) { return c = Color(int(c) + 1); }
inline Color& operator--(Color& c) { return c = Color(int(c) - 1); }
inline Color operator~(Color c) { return Color(c ^ 1); }

typedef struct Score {
    int midgame;
    int endgame;

    Score(int x=0, int y=0) : midgame(x), endgame(y)
    {}

    inline Score operator+(const Score& a) const
    {
        return Score(midgame + a.midgame, endgame + a.endgame);
    }
    inline Score operator-(const Score& a) const
    {
        return Score(midgame - a.midgame, endgame - a.endgame);
    }
    inline Score operator+=(const Score& a) 
    {
        this->midgame += a.midgame;
        this->endgame += a.endgame;
        return *this;
    }
    inline Score operator-=(const Score& a) 
    {        
        this->midgame -= a.midgame;
        this->endgame -= a.endgame;
        return *this;
    }
    inline Score operator/(const Score& a) const
    {
        return Score(midgame / a.midgame, endgame / a.endgame);
    }
    inline Score operator*(const Score& a) const
    {
        return Score(midgame * a.midgame, endgame * a.endgame);
    }
    inline Score operator+=(const int& a) 
    {
        this->midgame += a;
        this->endgame += a;
        return *this;
    }
    inline Score operator-=(const int& a)
    {
        this->midgame -= a;
        this->endgame -= a;
        return *this;
    }
    inline Score operator*=(const int& a)
    {
        this->midgame *= a;
        this->endgame *= a;
        return *this;
    }
    inline Score operator/=(const int& a)
    {
        this->midgame /= a;
        this->endgame /= a;
        return *this;
    }
    inline Score operator+(const int& a) const
    {
        return Score(midgame + a, endgame + a);
    }
    inline Score operator-(const int& a) const
    {
        return Score(midgame - a, endgame - a);
    }
    inline Score operator/(const int& a) const
    {
        return Score(midgame / a, endgame / a);
    }
    inline Score operator*(const int& a) const
    {
        return Score(midgame * a, endgame * a);
    }
} Score;

typedef struct Position Position;
typedef struct MoveGen MoveGen;
typedef struct SearchThread SearchThread;

const uint8_t can_king_castle_mask[2] = {1, 4};
const uint8_t can_queen_castle_mask[2] = {2, 8};
const uint8_t can_castle_mask[2] = {3, 12};

const Bitboard MASK_ISOLATED[8] = {
    0x0202020202020202, 0x0505050505050505, 0x0A0A0A0A0A0A0A0A, 0x1414141414141414,
    0x2828282828282828, 0x5050505050505050, 0xA0A0A0A0A0A0A0A0, 0x4040404040404040
};

const Bitboard FILE_ABB = 0x0101010101010101ULL;
const Bitboard FILE_BBB = FILE_ABB << 1;
const Bitboard FILE_CBB = FILE_ABB << 2;
const Bitboard FILE_DBB = FILE_ABB << 3;
const Bitboard FILE_EBB = FILE_ABB << 4;
const Bitboard FILE_FBB = FILE_ABB << 5;
const Bitboard FILE_GBB = FILE_ABB << 6;
const Bitboard FILE_HBB = FILE_ABB << 7;

const Bitboard FILE_MASK[8] = {
    FILE_ABB, FILE_BBB, FILE_CBB, FILE_DBB, FILE_EBB, FILE_FBB, FILE_GBB, FILE_HBB
};

const Bitboard COLOR_MASKS[2] = {0xAA55AA55AA55AA55, 0x55AA55AA55AA55AA};
const int TILE_COLOR[64] = {
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0,
    0, 1, 0, 1, 0, 1, 0, 1,
    1, 0, 1, 0, 1, 0, 1, 0
};

const uint8_t FLAG_ALPHA = 1;
const uint8_t FLAG_BETA = 2;
const uint8_t FLAG_EXACT = 3; // FLAG_ALPHA | FLAG_BETA

const Bitboard RANK_1BB = 0x00000000000000FF;
const Bitboard RANK_2BB = 0x000000000000FF00;
const Bitboard RANK_3BB = 0x0000000000FF0000;
const Bitboard RANK_4BB = 0x00000000FF000000;
const Bitboard RANK_5BB = 0x000000FF00000000;
const Bitboard RANK_6BB = 0x0000FF0000000000;
const Bitboard RANK_7BB = 0x00FF000000000000;
const Bitboard RANK_8BB = 0xFF00000000000000;

const Bitboard RANK_MASK[8] = {
    RANK_1BB, RANK_2BB, RANK_3BB, RANK_4BB, RANK_5BB, RANK_6BB, RANK_7BB, RANK_8BB
};

const uint64_t cross_lt[15] = {
    0x0000000000000001, 0x0000000000000102, 0x0000000000010204, 0x0000000001020408,
    0x0000000102040810, 0x0000010204081020, 0x0810204080000000, 0x0001020408102040,
    0x0102040810204080, 0x0204081020408000, 0x0408102040800000, 0x1020408000000000,
    0x2040800000000000, 0x4080000000000000, 0x8000000000000000
};

const uint64_t cross_rt[15] = {
    0x0100000000000000, 0x0201000000000000, 0x0402010000000000, 0x0804020100000000,
    0x1008040201000000, 0x2010080402010000, 0x4020100804020100, 0x8040201008040201,
    0x0080402010080402, 0x0000804020100804, 0x0000008040201008, 0x0000000080402010,
    0x0000000000804020, 0x0000000000008040, 0x0000000000000080
};

const uint16_t MAX_PLY = 128;

const uint64_t _knight_targets = 0x0000005088008850;
const uint64_t _king_targets  = 0x0000000070507000;

const int
    MATE = 32000,
    INFINITE = 32001,
    UNDEFINED = 32002,
    TIMEOUT = 32003,

    MATE_IN_MAX_PLY = MATE - MAX_PLY,
    MATED_IN_MAX_PLY = -MATE + MAX_PLY,

    TB_WIN = MATE_IN_MAX_PLY - MAX_PLY - 1;

const int RANK_1 = 0,
          RANK_2 = 1,
          RANK_3 = 2,
          RANK_4 = 3,
          RANK_5 = 4,
          RANK_6 = 5,
          RANK_7 = 6,
          RANK_8 = 7;

const int FILE_A = 0,
          FILE_B = 1,
          FILE_C = 2,
          FILE_D = 3,
          FILE_E = 4,
          FILE_F = 5,
          FILE_G = 6,
          FILE_H = 7;

enum MoveType{
    NORMAL      = 0,
    PROMOTION_N = 0,
    PROMOTION_B = 4,
    PROMOTION_Q = 12,
    PROMOTION_R = 8,
    CASTLING    = 3,
    ENPASSANT   = 1, // Enpassant capture
    PROMOTION   = 2
};

const Move null_move = Move(~0);
const Move no_move = Move(0);

enum MoveGenType {
    SILENT = 0,
    CAPTURE = 1,
    ALL = 2
};

const int NUM_PIECE = 15;

enum _Piece {
    white_occupy = 0,
    black_occupy = 1,
    white_pawn = 2,
    black_pawn = 3,
    white_knight = 4,
    black_knight = 5,
    white_bishop = 6,
    black_bishop = 7,
    white_rook = 8,
    black_rook = 9,
    white_queen = 10,
    black_queen = 11,
    white_king = 12,
    black_king = 13,
    no_piece = 14
};

enum _PieceType {
    NO_PIECE_TYPE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};

enum _Square : Square {
    A1 = 0,
        B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    no_sq
};

const char piece_chars[NUM_PIECE] = {'\0', '\0', '\0', '\0', 'N', 'n', 'B', 'b', 'R', 'r', 'Q', 'q', 'K', 'k'};

typedef struct Info {
    // COPIED 
    uint64_t pawn_hash;
    int      non_pawn_material[2];
    int      material_index;
    uint8_t  castling; // black_queenside | black_kingside | white_queenside | white_kingside
    uint8_t  last_irreversible;
    // NOT TO COPY
    Square   enpassant; 
    uint64_t hash;
    Bitboard pinned[2];
    Piece    captured;
    Info     *previous;
} Info;

typedef struct CopiedInfo {
    // COPIED 
    uint64_t pawn_hash;
    int      non_pawn_material[2];
    int      material_index;
    uint8_t  castling; // black_queenside | black_kingside | white_queenside | white_kingside
    uint8_t  last_irreversible;
} CopiedInfo;

const int info_size = sizeof(CopiedInfo);

typedef struct Metadata {
    int  ply;
    Move current_move;
    int  static_eval;
    Move killers[2];
    Move pv[MAX_PLY + 1];
    Move excluded_move;
    int  **counter_move_history;
} Metadata;

enum RookSquares {
    QUEENSIDE = 0,
    KINGSIDE = 1
};

struct Position {
    SearchThread *my_thread;
    Bitboard     bbs[NUM_PIECE];
    Bitboard     board;
    Piece        pieces[64];
    Score        score;
    Square       king_index[2];
    Info         *info;
    Color        color;
    Square       initial_rooks[2][2]; // [color][queenside = 0, kingside = 1]
    uint8_t      CASTLING_RIGHTS[64];
};

const uint64_t pawntt_size = 16384ULL;

typedef struct PawnTTEntry {
    uint64_t pawn_hash;
    Score    score[2];
    Bitboard pawn_passers[2];
    int      semi_open_files[2];
    int      pawn_shelter_value[2];
    Square   king_index[2];
    uint8_t  castling;
} PawnTTEntry;

typedef struct Evaluation {
    Bitboard    targets[NUM_PIECE];
    Bitboard    double_targets[2];
    Bitboard    mobility_area[2];
    Score       score;
    int         num_king_attackers[2];
    int         num_king_zone_attacks[2];
    int         king_zone_score[2];
    Bitboard    king_zone[2];
    int         num_queens[2];
    PawnTTEntry *pawntte;
} Evaluation;

enum EndgameType {
    NORMAL_ENDGAME,
    DRAW_ENDGAME
};

const int SCALE_NORMAL = 32;

typedef struct Material {
    int               phase;
    int               score;
    EndgameType       endgame_type;
} Material;

const int material_balance[NUM_PIECE] ={
    0, 0,
    2*2*3*3*3*3*9,   // White Pawn
    2*2*3*3*3*3*9*9, // Black Pawn
    2*2*3*3*3*3,     // White Knight
    2*2*3*3*3*3*3,   // Black Knight
    2*2*3*3,         // White Bishop
    2*2*3*3*3,       // Black Bishop
    2*2,             // White Rook
    2*2*3,           // Black Rook
    1,               // White Queen
    1*2,             // Black Queen
    0, 0,            // Kings
};

enum SearchType {
    NORMAL_SEARCH = 0,
    QUIESCENCE_SEARCH = 1,
    PROBCUT_SEARCH = 2
};

typedef struct ScoredMove {
    Move move;
    int  score;
} ScoredMove;

bool scored_move_compare(ScoredMove lhs, ScoredMove rhs);
bool scored_move_compare_greater(ScoredMove lhs, ScoredMove rhs);

enum Stages {
    // Regular stages
    NORMAL_TTE_MOVE = 0,

    GOOD_CAPTURES_SORT,
    GOOD_CAPTURES,

    KILLER_MOVE_2,
    COUNTER_MOVES,

    QUIETS_SORT,
    QUIETS,

    BAD_CAPTURES,

    // Evasion stages
    EVASIONS_SORT,
    EVASIONS,

    // Quiescence stages
    QUIESCENCE_TTE_MOVE,
    QUIESCENCE_CAPTURES_SORT,
    QUIESCENCE_CAPTURES,
    QUIESCENCE_CHECKS,

    // Probcut stages
    PROBCUT_TTE_MOVE,
    PROBCUT_CAPTURES_SORT,
    PROBCUT_CAPTURES
};

enum ScoreType {
    SCORE_CAPTURE = 0,
    SCORE_QUIET = 1,
    SCORE_EVASION = 2
};

struct MoveGen {
    ScoredMove moves[256];
    Position   *position;
    Move       tte_move;
    Move       counter_move;
    int        stage;
    uint8_t    head;
    uint8_t    tail;
    int        end_bad_captures;
    int        threshold;
};

const MoveGen blank_movegen = {
    {}, // Moves
    nullptr, // Position
    no_move, // tte_move
    no_move, // counter move
    0, // stage
    0, // head
    0, // tail
    0, // end bad captures
    0 // threshold
};

struct SearchThread {
    Position    position;
    Info        infos[1024];
    int         thread_id;
    int         root_ply;
    int         search_ply;
    Metadata    metadatas[MAX_PLY + 2];
    Move        counter_moves[NUM_PIECE][64];
    int         history[2][64][64];
    int         **counter_move_history[NUM_PIECE][64];
    int         selply;
    PawnTTEntry pawntt[pawntt_size];
    uint64_t    nodes;
    uint64_t    tb_hits;
    bool        nmp_enabled;
};

const int MAX_THREADS = 256;

#endif
