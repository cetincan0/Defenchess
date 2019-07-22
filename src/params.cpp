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
    PAWN_MID = 100, PAWN_END = 136,
    KNIGHT_MID = 442, KNIGHT_END = 446,
    BISHOP_MID = 487, BISHOP_END = 478,
    ROOK_MID = 731, ROOK_END = 742,
    QUEEN_MID = 1416, QUEEN_END = 1424;

int piece_values[NUM_PIECE] = {0, 0, PAWN_MID, PAWN_MID, KNIGHT_MID, KNIGHT_MID, BISHOP_MID, BISHOP_MID,
                                     ROOK_MID, ROOK_MID, QUEEN_MID, QUEEN_MID, 100 * QUEEN_MID, 100 * QUEEN_MID};

Score
    double_pawn_penalty = {0, 21},
    bishop_pawn_penalty = {6, 11};

int king_danger_shelter_bonus = 17,
    king_danger_queen_penalty = 726,
    king_danger_pinned_penalty = 84,
    king_danger_init = 38,
    king_zone_attack_penalty = 61,
    king_danger_weak_penalty = 59,
    king_danger_weak_zone_penalty = 82,
    queen_check_penalty = 371,
    knight_check_penalty = 319,
    rook_check_penalty = 482,
    bishop_check_penalty = 96,
    pawn_distance_penalty = 10;

int pawn_shelter[4][8] = {
    { -3, 43, 45,  23,  21, 57,  104},
    {-34, 34, 24, -19, -23, 21,  -39},
    {  0, 61, 23,   7,  12, 66,  107},
    { -9, 57, -7,   4,  -4, 33,  100}
};
int pawn_storm[2][4][8] = {
    {
        {16, -144, -42, 18,  -8, -10, -1, -29},
        {26, -108,  19, 21,   6,   5,  7,  22},
        { 7,   -7,  72, 46,  22,   2, -1,  15},
        {11,   46,  57, 56,  14,  -8, -5,  -9}
    }, {
        {0, 0,  46,  -1, -38,   2,  29,   2},
        {0, 0,  65,  -9, -20, -11,  -4,  66},
        {0, 0, 137,  37,  31,  29,  82,  32},
        {0, 0, 118,  29,  26,  -1, -59,   3}
    }
};
int tempo = 12;
int ATTACK_VALUES[6] = {0, 0, 74, 42, 35, 10};
int bishop_pair = 53;

// connected_bonus[opposed][adjacent][rank]
// cannot be opposed on rank 7
// cannot be not adjacent on rank 2
Score connected_bonus[2][2][8] = {
    // unopposed
    {
        // not adjacent
        {{0, 0}, {0, 0}, {29, -4}, {31, -1}, {60, 22}, {148, 39}, {656, -46}, {0, 0}},
        // adjacent
        {{0, 0}, {7, -13}, {18, -5}, {34, 20}, {104, 48}, {316, 85}, {269, 338}, {0, 0}}
    },
    // opposed
    {
        // not adjacent
        {{0, 0}, {0, 0}, {17, 3}, {5, -1}, {34, 13}, {81, 50}, {0, 0}, {0, 0}},
        // adjacent
        {{0, 0}, {5, 1}, {15, -2}, {17, 5}, {38, 28}, {131, 132}, {0, 0}, {0, 0}}
    }
};

Score mobility_bonus[6][32] = {
    {},  // NO PIECE
    {},  // PAWN
    { {-47, -24}, {-38, -57}, {-21, -27}, {-11, -13}, {-6, -1}, {0, 4}, {10, 10}, {13, 15}, {15, 15} }, // KNIGHT
    { {-50, 3}, {-23, -22}, {1, -7}, {4, 7}, {15, 17}, {24, 25}, {31, 31}, {33, 33}, {37, 37}, {52, 33}, {54, 32}, {72, 24}, {64, 18}, {108, 11} },  // BISHOP
    { {-38, 34}, {-19, -8}, {2, 7}, {3, 28}, {4, 39}, {4, 53}, {4, 59}, {8, 63}, {12, 70}, {15, 76}, {18, 81}, {25, 84}, {34, 80}, {44, 80}, {66, 86} },  // ROOK
    { {-15, 94}, {-44, 16}, {-20, 0}, {6, -31}, {4, 8}, {21, 7}, {22, 40}, {27, 41}, {31, 49}, {34, 54}, {39, 59}, {41, 67}, {39, 77}, {40, 85}, {45, 86}, {41, 93}, {39, 90}, {36, 92}, {40, 88}, {52, 84}, {50, 71}, {65, 56}, {112, 42}, {147, 21}, {81, 38}, {119, 26}, {62, 7}, {38, 5}, {92, 145} }  // QUEEN
};

Score passed_pawn_bonus[8] = {
    {0, 0}, {0, 2}, {0, 2}, {7, 3}, {25, 11}, {85, 59}, {177, 69} // Pawn is never on RANK_8
};

int passer_my_distance[7] = {0, 1, 2, 7, 13, 19, 18};
int passer_enemy_distance[7] = {0, -1, -1, 9, 26, 38, 46};
Score passer_blocked[2][7] = {
    {{0, 0}, {21, 18}, {3, 8}, {12, 21}, {19, 25}, {22, 34}, {292, 87}},
    {{0, 0}, {7, 7}, {-8, -7}, {-6, -8}, {16, -29}, {-27, -69}, {-17, -83}}
};
Score passer_unsafe[2][7] = {
    {{0, 0}, {11, 12}, {18, 23}, {-14, 27}, {-17, 43}, {70, 68}, {257, 52}},
    {{0, 0}, {6, -3}, {3, 9}, {5, -6}, {8, -16}, {71, -48}, {42, -83}}
};

Score rook_file_bonus[2] = {{15, 0}, {39, 14}};

Score isolated_pawn_penalty[2] = {{36, 28}, {31, 16}},
      backward_pawn_penalty[2] = {{32, 33}, {2, 15}};

Score pawn_push_threat_bonus = {31, 14};

Score minor_threat_bonus[6] = {
    { 0,  0}, // Empty
    {11, 28}, // Pawn
    {26, 51}, // Knight
    {44, 23}, // Bishop
    {65, 22}, // Rook
    {45, 22}  // Queen
    // { 0, 0},  // King should never be called
};

Score rook_threat_bonus[6] = {
    { 0,  0}, // Empty
    {11, 26}, // Pawn
    {41, 39}, // Knight
    {47, 38}, // Bishop
    { 0,  0}, // Rook
    {92, 12}  // Queen
    // { 0, 0}  // King should never be called
};

Score pawn_threat_bonus[6] = {
    { 0,  0}, // Empty
    { 0,  0}, // Pawn
    {62, 49}, // Knight
    {62, 67}, // Bishop
    {55, 59}, // Rook
    {62,  6}  // Queen
    // { 0, 0}  // King should never be called
};

Score pst[NUM_PIECE][64];

int bonusPawn[2][32] = {
    {
        0,    0,    0,    0,
        2,  -44,   -6,   18,
      -19,   -2,   12,   18,
        1,    1,    6,   24,
       -7,    0,    7,   20,
       -1,   -3,   -2,    5,
      -13,    0,   -3,   -5,
        0,    0,    0,    0,
    }, {
        0,    0,    0,    0,
      -39,   21,   28,   20,
      -12,    2,   -3,    6,
        2,   -2,   -4,   -6,
        1,   -3,   -5,   -9,
        2,   -2,    1,    5,
       -1,   -1,   10,    4,
        0,    0,    0,    0,
    }
};

int bonusKnight[2][32] = {
    {
     -270, -146,  -90,   11,
      -62,  -26,    4,   29,
       -7,   24,   34,   34,
       11,   15,   29,   30,
       -5,    7,   18,   22,
      -26,   -7,   -8,    5,
      -32,  -23,  -11,   -9,
      -57,  -39,  -24,   -9,
    }, {
      -62,  -20,  -12,   -7,
       -9,    0,    0,   16,
      -14,    9,   20,   12,
        4,    1,   20,   26,
       -9,   11,   14,   22,
      -33,  -14,  -13,   12,
      -50,  -10,  -21,  -10,
      -34,  -36,  -28,  -22,
    }
};

int bonusBishop[2][32] = {
    {
        3,  -31, -109,  -12,
      -47,  -65,  -23,  -40,
      -18,   40,   26,    5,
       -5,   -1,   18,   13,
        2,    8,    5,   18,
       -5,    7,    6,    1,
        3,    9,   17,   -4,
       -1,   15,  -13,  -11,
    }, {
      -19,    5,  -12,   -1,
      -13,   -1,    6,   10,
        0,    0,    3,   11,
      -11,    4,   -4,    5,
      -16,  -11,    4,   -5,
      -25,  -12,  -10,    3,
      -33,  -26,  -27,   -8,
      -41,  -40,  -20,  -17,
    }
};

int bonusRook[2][32] = {
    {
       32,   12,   -9,   23,
        6,  -16,    7,   25,
        7,   14,    6,   27,
        2,    7,   17,    6,
      -22,    0,  -14,    2,
      -31,  -11,  -14,  -15,
      -46,  -15,   -1,   -6,
      -13,  -12,   -3,    1,
    }, {
       15,   22,   12,   10,
       13,   20,   16,   11,
       11,   15,   16,    8,
       12,   17,   17,   15,
        9,    9,   13,    7,
       -4,   -3,   -6,   -8,
      -16,  -14,  -14,  -12,
       -1,   -9,   -9,  -14,
    }
};

int bonusQueen[2][32] = {
    {
      -13,   12,   11,   11,
      -16,  -45,  -24,  -28,
       13,   -7,  -22,  -24,
       -1,  -19,    0,  -33,
       -8,   12,    0,   -4,
       -1,    7,   10,   -1,
       10,    2,   20,    5,
       18,   -7,   -7,   -5,
    }, {
       16,  -10,   11,   19,
       16,   37,   28,   38,
       -8,   13,   31,   32,
       22,   46,   22,   46,
        7,    9,    9,   11,
      -19,   -5,   -8,   -7,
      -58,  -38,  -47,  -17,
      -67,  -44,  -42,  -27,
    }
};

int bonusKing[2][32] = {
    {
       60,  266,  227,  326,
      231,  348,  202,  220,
      182,  310,  266,  268,
      170,  423,  331,  267,
      108,  243,  229,  233,
      100,  140,  163,  175,
      145,  152,  125,  119,
      137,  179,  155,  113,
    }, {
      -53,  154,  117,   83,
       79,  113,  115,  101,
      119,  111,  122,  102,
       94,   93,  110,  110,
       71,   90,   97,  107,
       60,   76,   83,   91,
       52,   63,   73,   75,
       24,   43,   52,   57,
    }
};

