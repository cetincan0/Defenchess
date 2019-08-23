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

#include "params.h"

int
    PAWN_MID = 100, PAWN_END = 135,
    KNIGHT_MID = 486, KNIGHT_END = 432,
    BISHOP_MID = 520, BISHOP_END = 465,
    ROOK_MID = 721, ROOK_END = 734,
    QUEEN_MID = 1435, QUEEN_END = 1423;

int piece_values[NUM_PIECE] = {0, 0, PAWN_MID, PAWN_MID, KNIGHT_MID, KNIGHT_MID, BISHOP_MID, BISHOP_MID,
                                     ROOK_MID, ROOK_MID, QUEEN_MID, QUEEN_MID, 100 * QUEEN_MID, 100 * QUEEN_MID};

Score
    double_pawn_penalty = {-3, 23},
    bishop_pawn_penalty = {4, 10};

int king_danger_shelter_bonus = 22,
    king_danger_queen_penalty = 853,
    king_danger_pinned_penalty = 53,
    king_danger_init = 39,
    king_zone_attack_penalty = 73,
    king_danger_weak_penalty = 86,
    king_danger_weak_zone_penalty = 69,
    king_flank_penalty = 3,
    queen_check_penalty = 460,
    knight_check_penalty = 521,
    rook_check_penalty = 547,
    bishop_check_penalty = 153,
    pawn_distance_penalty = 9;

int pawn_shelter[4][8] = {
    { 12, 52, 42,  19,  11,  41, -22},
    {-34, 36, 25, -19, -21, -11, -59},
    {  1, 64, 26,  11,   7,  55,  44},
    {  6, 30,  1,   1,   2,  12,  92}
};
int pawn_storm[2][4][8] = {
    {
        {13, -74, -44, 16,   1,  -3, -10, -4},
        {15, -61,  42, 25,  11,   7,   3, 48},
        { 7, -28,  87, 33,  18,   1,   0, 38},
        { 7,  70, 101, 37,  29,  -5,   0, 11}
    }, {
        {0, 0,  57,  -9, -23,  -6,  37,   7},
        {0, 0, 111,  12,   6,  35,   9,  95},
        {0, 0, 130,  21,  -2,  10,  99,  56},
        {0, 0, 138,  28,   9,   0,  -2,  24}
    }
};
int tempo = 12;
int ATTACK_VALUES[6] = {0, 0, 53, 40, 40, 22};
int bishop_pair = 53;

int SCALE_PURE_OCB = 16;
int SCALE_OCB_WITH_PIECES = 27;
int SCALE_NO_PAWNS = 0;

// connected_bonus[opposed][adjacent][rank]
// cannot be opposed on rank 7
// cannot be not adjacent on rank 2
Score connected_bonus[2][2][8] = {
    // unopposed
    {
        // not adjacent
        {{0, 0}, {0, 0}, {29, -1}, {41, 1}, {52, 20}, {90, 54}, {420, 36}, {0, 0}},
        // adjacent
        {{0, 0}, {18, -12}, {16, -3}, {21, 18}, {59, 41}, {156, 104}, {93, 176}, {0, 0}}
    },
    // opposed
    {
        // not adjacent
        {{0, 0}, {0, 0}, {13, 7}, {12, 1}, {24, 16}, {44, 50}, {0, 0}, {0, 0}},
        // adjacent
        {{0, 0}, {5, -1}, {13, -4}, {16, 2}, {30, 19}, {85, 252}, {0, 0}, {0, 0}}
    }
};

Score mobility_bonus[6][32] = {
    {}, // NO PIECE
    {}, // PAWN
    {   // KNIGHT
        {-41, -65}, {-38, -120}, {-35, -47}, {-30, -23},
        {-14, -15}, { -9,   -1}, { -1,   9}, {  5,  13},
        { 14,  -2}
    },
    {   // BISHOP
        {-40, -37}, {-40, -47}, {-10, -40}, {-6, -14},
        {  9,   5}, { 18,  19}, { 25,  28}, {26,  34},
        { 27,  42}, { 28,  41}, { 24,  43}, {62,  17},
        { 23,  31}, {221, -98}
    },
    {   // ROOK
        {-88, 19}, {-14, -22}, {  1,  1}, { 0, 22},
        {  4, 35}, {  1,  55}, {  4, 56}, {10, 63},
        { 17, 74}, { 23,  78}, { 19, 87}, {15, 96},
        { 31, 90}, { 49,  82}, {133, 53}
    },
    {   // QUEEN
        {-662, -587}, {-18, 125}, {-14, 38}, { -9, -20},
        {   6,  -29}, {  5,   4}, { 11, 20}, { 18,  29},
        {  24,   53}, { 32,  52}, { 41, 53}, { 46,  66},
        {  51,   73}, { 49,  75}, { 47, 83}, { 46,  89},
        {  46,   86}, { 36,  92}, { 28, 88}, { 50,  79},
        {  55,   69}, { 67,  53}, { 78, 42}, {126,   6},
        { 127,   -2}, {115,   6}, {166, 36}, { 84, -12},
        { 189,  298}
    }
};

Score passed_pawn_bonus[8] = {
    {0, 0}, {-10, 1}, {0, 2}, {-11, 2}, {2, 3}, {16, 60}, {36, 120} // Pawn is never on RANK_8
};

int passer_my_distance[7] = {0, -3, 1, 7, 14, 19, 16};
int passer_enemy_distance[7] = {0, -2, -1, 10, 26, 38, 44};
Score passer_blocked[2][7] = {
    {{0, 0}, {8, 10}, {-6, 0}, {-4, 12}, {12, 19}, {2, 31}, {86, 93}},
    {{0, 0}, {-8, -2}, {-12, -3}, {6, -3}, {21, -17}, {6, -83}, {95, -96}}
};
Score passer_unsafe[2][7] = {
    {{0, 0}, {15, 7}, {14, 20}, {-14,20}, {-35, 40}, {40, 68}, {66, 58}},
    {{0, 0}, {-2, 9}, {4, 12}, {6, 3}, {17, -8}, {87, -58}, {127, -112}}
};

Score rook_file_bonus[2] = {{22, 0}, {56, 6}};

Score isolated_pawn_penalty[2] = {{21, 21}, {18, 9}},
      backward_pawn_penalty[2] = {{19, 21}, {5, 8}};

Score pawn_push_threat_bonus = {29, 16};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 6, 15}, // Pawn
    {24, 40}, // Knight
    {34, 29}, // Bishop
    {66,  5}, // Rook
    {46,  1}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    {  0,  0}, // Empty
    {  1, 21}, // Pawn
    { 44, 25}, // Knight
    { 47, 36}, // Bishop
    {  0,  0}, // Rook
    {103, -3}  // Queen
    // { 0, 0}  // King should never be called
};

Score pawn_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 0,  0}, // Pawn
    {77, 24}, // Knight
    {71, 67}, // Bishop
    {91, 13}, // Rook
    {72, 11}  // Queen
    // { 0, 0}  // King should never be called
};

Score king_threat_bonus[2] = {{-8, 27}, {53, 64}};

Score pst[NUM_PIECE][64];

int bonusPawn[2][32] = {
    {
        0,    0,    0,    0,
      -16,  -15,  -71,  -14,
       -3,  -47,   -9,    8,
        7,  -13,   -8,   21,
      -10,  -19,   -2,   17,
      -10,  -16,   -3,   10,
      -13,   -2,   -8,    8,
        0,    0,    0,    0
    }, {
        0,    0,    0,    0,
       35,   40,   26,   27,
       29,   26,   13,    5,
        3,   -8,  -15,  -25,
       -4,  -13,  -18,  -23,
      -11,  -14,  -11,   -7,
      -11,  -15,   -3,   -7,
        0,    0,    0,    0
    }
};

int bonusKnight[2][32] = {
    {
     -309, -176, -173,  -23,
     -130,  -79,   -4,  -15,
      -46,   36,   29,   37,
        1,   17,   27,   39,
       -3,   28,   31,   28,
      -21,    4,   17,   21,
      -19,  -25,   -1,    4,
      -54,  -17,  -39,  -13
    }, {
      -74,  -55,  -32,  -37,
      -41,  -18,  -32,   -6,
      -43,  -20,    0,   -1,
      -17,    3,   12,   23,
      -11,   -8,   10,   20,
      -24,  -18,  -10,    6,
      -43,  -25,  -22,  -10,
      -46,  -41,  -22,  -16
    }
};

int bonusBishop[2][32] = {
    {
      -40,  -29, -144,  -83,
      -89,  -77,  -51,  -51,
       -4,   17,    9,    3,
       -8,   -9,    9,   24,
       -2,    9,    4,   29,
      -11,   14,   16,    7,
        7,   28,   17,    2,
      -30,   -6,   -7,   -4
    }, {
      -28,  -22,  -24,  -27,
      -15,   -8,   -9,  -17,
       -8,   -9,    3,   -6,
       -9,    1,    6,   11,
      -18,   -7,    5,    9,
      -14,  -11,   -1,    6,
      -30,  -17,  -15,   -3,
      -32,  -18,  -10,  -10
    }
};

int bonusRook[2][32] = {
    {
      -23,   -6,  -60,   17,
      -21,  -30,    5,   30,
      -36,    2,  -15,   -8,
      -44,  -26,   12,   22,
      -47,   -6,  -24,    5,
      -45,  -15,   -9,  -10,
      -56,  -13,   -9,    4,
      -25,  -19,   -2,   11
    }, {
       21,   18,   22,   20,
       10,   11,    8,    0,
        1,    6,    0,    0,
        9,    3,    9,    1,
        0,    3,    4,   -3,
       -9,   -1,   -8,   -6,
       -8,   -8,   -7,   -8,
      -10,   -2,   -1,   -7
    }
};

int bonusQueen[2][32] = {
    {
      -46,   -3,    3,   30,
      -41,  -91,  -27,  -49,
       10,  -12,  -22,  -27,
      -28,  -28,  -17,  -55,
      -23,  -12,   -9,  -20,
      -16,    8,  -12,   -6,
      -17,   -5,   21,    3,
        7,  -13,   -9,   16
    }, {
        5,    7,   17,   17,
       -8,   21,   30,   56,
      -13,    9,   36,   58,
       29,   40,   38,   69,
       16,   33,   34,   53,
        9,   -9,   23,   13,
      -30,  -24,  -23,    4,
      -39,  -29,  -25,  -27
    }
};

int bonusKing[2][32] = {
    {
      124,  206,  149,  197,
      220,  222,  183,  307,
      116,  366,  380,  243,
      106,  256,  249,  155,
       40,  143,  174,  107,
       82,  160,  190,  113,
      145,  150,  138,  113,
      149,  174,  155,  122
    }, {
       39,   72,   89,   58,
       89,  104,  112,   93,
      100,  116,  112,   89,
       79,  102,  103,   97,
       65,   78,   95,   96,
       64,   76,   83,   89,
       51,   66,   77,   78,
       13,   41,   58,   53
    }
};

