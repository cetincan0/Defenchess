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
    PAWN_MID = 100, PAWN_END = 152,
    KNIGHT_MID = 484, KNIGHT_END = 517,
    BISHOP_MID = 527, BISHOP_END = 546,
    ROOK_MID = 743, ROOK_END = 838,
    QUEEN_MID = 1486, QUEEN_END = 1648;

int piece_values[NUM_PIECE] = {0, 0, PAWN_MID, PAWN_MID, KNIGHT_MID, KNIGHT_MID, BISHOP_MID, BISHOP_MID,
                                     ROOK_MID, ROOK_MID, QUEEN_MID, QUEEN_MID, 100 * QUEEN_MID, 100 * QUEEN_MID};

Score
    double_pawn_penalty = {-5, 28},
    bishop_pawn_penalty = {6, 7};

int king_danger_shelter_bonus = 24,
    king_danger_queen_penalty = 738,
    king_danger_pinned_penalty = 120,
    king_danger_init = 67,
    king_zone_attack_penalty = 65,
    king_danger_weak_penalty = 116,
    king_danger_weak_zone_penalty = 107,
    king_flank_penalty = 3,
    queen_check_penalty = 328,
    knight_check_penalty = 396,
    rook_check_penalty = 477,
    bishop_check_penalty = 237,
    pawn_distance_penalty = 11;

int pawn_shelter[4][8] = {
    {-15, 34, 27,   3,   7, 29, 84},
    {-36, 41, 27, -18, -20,  9, -6},
    {  6, 51, 22,  14,  26, 44, 51},
    {  3, 17,  9,  -4,  -2,  0, 61}
};
int pawn_storm[2][4][8] = {
    {
        {36, -143, -37, 34, 20, 4,  3, 15},
        {26,  -84,  50, 34, 20, 3, -4, 46},
        {10,   51,  78, 36, 16, 3,  7, 66},
        {19,    6,  59, 37, 18, 0, -2, 33}
    }, {
        {0, 0,  37, -12, -19, -7,   5,  33},
        {0, 0,  60,   5,  -3, -8, -27, 138},
        {0, 0, 103,  26,  11, 10,  51,  96},
        {0, 0,  72,  31,   2,  8, -57,  62}
    }
};
int tempo = 16;
int ATTACK_VALUES[6] = {0, 0, 50, 23, 17, 7};
int bishop_pair = 49;

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
        {{0, 0}, {0, 0}, {32, 6}, {29, 12}, {38, 35}, {96, 71}, {220, 86}, {0, 0}},
        // adjacent
        {{0, 0}, {13, -8}, {23, 4}, {29, 19}, {54, 38}, {129, 99}, {502, 350}, {0, 0}}
    },
    // opposed
    {
        // not adjacent
        {{0, 0}, {0, 0}, {18, 12}, {8, 5}, {7, 19}, {36, 45}, {0, 0}, {0, 0}},
        // adjacent
        {{0, 0}, {6, 1}, {17, 4}, {14, 4}, {30, 10}, {165, 69}, {0, 0}, {0, 0}}
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
    {0, 0}, {-10, 0}, {0, 1}, {-10, 4}, {2, 5}, {13, 60}, {22, 120} // Pawn is never on RANK_8
};
int passer_my_distance[7] = {0, -2, 2, 8, 16, 25, 21};
int passer_enemy_distance[7] = {0, -2, 0, 11, 30, 48, 46};
Score passer_blocked[2][7] = {
    {{0, 0}, {-7, 10}, {-18, -1}, {1, 12}, {7, 22}, {-5, 33}, {90, 103}},
    {{0, 0}, {-6, -7}, {-30, -2}, {-4, -8}, {3, -14}, {-10, -71}, {70, -49}}
};
Score passer_unsafe[2][7] = {
    {{0, 0}, {12, 6}, {6, 17}, {-6, 21}, {-15, 41}, {58, 62}, {137, 28}},
    {{0, 0}, {4, 4}, {2, 13}, {-1, 0}, {7, -7}, {73, -45}, {91, -65}}
};

Score rook_file_bonus[2] = {{10, -7}, {24, 13}};

Score isolated_pawn_penalty[2] = {{24, 15}, {9, 10}},
      backward_pawn_penalty[2] = {{21, 22}, {-2, 12}};

Score pawn_push_threat_bonus = {25, 22};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 7, 20}, // Pawn
    {36, 35}, // Knight
    {48, 49}, // Bishop
    {82, 37}, // Rook
    {78, 57}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    {  0,  0}, // Empty
    {  6, 33}, // Pawn
    { 49, 42}, // Knight
    { 60, 46}, // Bishop
    {  0,  0}, // Rook
    {126, 67}  // Queen
    // { 0, 0}  // King should never be called
};

Score pawn_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 0,  0}, // Pawn
    {91, 49}, // Knight
    {97, 76}, // Bishop
    {78, 10}, // Rook
    {83,  4}  // Queen
    // { 0, 0}  // King should never be called
};

Score king_threat_bonus[2] = {{50, 6}, {112, 81}};

Score pst[NUM_PIECE][64];

int bonusPawn[2][32] = {
    {
        0,   0,   0,   0,
        6, -77, -59,   2,
       -1, -41, -13,   8,
       -2, -12,  -9,  13,
      -12, -16,  -3,  10,
      -14, -15,  -3,   8,
      -11,  -6,  -6,   9,
        0,   0,   0,   0
    }, {
        0,   0,   0,   0,
       24,  43,  30,  15,
       29,  32,  11,  -4,
        6,  -1, -15, -26,
       -8,  -8, -18, -22,
      -11, -12, -14, -11,
       -9,  -9,  -4,  -1,
        0,   0,   0,   0
    }
};

int bonusKnight[2][32] = {
    {
     -281, -151, -181, -53,
      -93,  -70,   -4, -30,
       -9,   -5,    7,  23,
       10,   11,   42,  38,
       -2,   24,   27,  33,
      -21,    3,   12,  20,
      -10,   -7,   -5,   8,
      -57,  -30,  -12,  -7
    }, {
      -73,  -48,  -40, -34,
      -40,  -13,  -19, -14,
      -25,  -19,    5,  -7,
      -10,    0,    7,  13,
        2,    2,    7,  20,
      -20,  -18,  -17,   1,
        5,   -7,  -18, -16,
      -23,  -15,  -10,  -6
    }
};

int bonusBishop[2][32] = {
    {
      -75, -102, -78, -127,
      -53,  -90, -52,  -68,
        3,   -6,   2,   -1,
      -43,   -5,  -2,   12,
        5,  -11,   5,   12,
      -11,   19,  11,    5,
       12,   19,  12,   -2,
       11,    4,   1,   -2
    }, {
      -13,   -7,  -8,  -14,
      -28,  -15, -12,  -11,
      -10,   -5,   0,  -13,
       -2,    1,  -2,    9,
      -16,   -7,   3,    7,
      -18,    0,  -2,    7,
      -23,  -17, -20,   -5,
      -18,  -15,  -1,   -5
    }
};

int bonusRook[2][32] = {
    {
      -17, -42, -47, -52,
      -13, -31,  -5,  -1,
      -30,  16,   4,   4,
      -34, -12,   7,  -2,
      -31,   0, -19,  -5,
      -27,   2,   2,   0,
      -26,   0,  10,  12,
      -18,  -9,  -2,   6
    }, {
       27,  26,  38,  33,
        4,  13,  15,   7,
        7,   7,  14,   3,
       16,  17,  12,  11,
       11,   5,  14,  12,
       -1,  -6,   0,   4,
        0,  -3,  -4,  -1,
        0,  -6,  -4, -12
    }
};

int bonusQueen[2][32] = {
    {
      -41, -36,  -9, -10,
      -29, -67, -57, -57,
        3, -34, -33, -46,
      -22, -19, -20, -44,
       -4,  -3,  -8, -15,
       -4,  16,  -1,  -2,
        4,   8,  15,  13,
       12,  -5,  -4,   4
    }, {
       -7,  -2,  14,   6,
        0,  41,  61,  57,
       -3,  25,  56,  54,
       30,  52,  44,  71,
       24,  32,  34,  53,
       -7,  -2,  26,  14,
      -33, -29, -19,  -4,
      -51, -24, -29, -33
    }
};

int bonusKing[2][32] = {
    {
      121, 295, 121, 290,
      194, 312, 287, 194,
       74, 341, 292, 230,
       55, 191, 143, 125,
      -25, 108, 119,  73,
       92, 175, 142,  89,
      138, 133, 123, 109,
      167, 174, 163, 118
    }, {
      -46,  45,  78, 103,
      100, 150, 167, 141,
       93, 145, 144, 144,
       79, 119, 135, 139,
       69,  98, 109, 116,
       59,  73,  85,  95,
       43,  64,  75,  78,
       -5,  29,  51,  58
    }
};

