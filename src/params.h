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

#ifndef PARAMS_H
#define PARAMS_H

#include "const.h"

extern int
    PAWN_MID, PAWN_END,
    KNIGHT_MID, KNIGHT_END,
    BISHOP_MID, BISHOP_END,
    ROOK_MID, ROOK_END,
    QUEEN_MID, QUEEN_END;

extern int piece_values[NUM_PIECE];

extern Score
    double_pawn_penalty,
    bishop_pawn_penalty;

extern int
    king_danger_shelter_bonus,
    king_danger_queen_penalty,
    king_danger_pinned_penalty,
    king_danger_init,
    king_danger_weak_penalty,
    king_danger_weak_zone_penalty,
    queen_check_penalty,
    knight_check_penalty,
    rook_check_penalty,
    bishop_check_penalty,
    pawn_distance_penalty,
    king_zone_attack_penalty,
    queen_safety_bonus;

extern int pawn_shelter[4][8],
           pawn_storm[2][4][8];
extern int tempo;
extern int ATTACK_VALUES[6];
extern int bishop_pair;

extern Score connected_bonus[2][2][8];
extern Score mobility_bonus[6][32];
extern Score passed_pawn_bonus[8];
extern int passer_my_distance[7];
extern int passer_enemy_distance[7];
extern Score passer_blocked[2][7];
extern Score passer_unsafe[2][7];

extern Score rook_file_bonus[2];
extern Score isolated_pawn_penalty[2],
             backward_pawn_penalty[2];
extern Score pawn_push_threat_bonus;
extern Score minor_threat_bonus[6];
extern Score rook_threat_bonus[6];
extern Score pawn_threat_bonus[6];

extern Score pst[NUM_PIECE][64];

extern int bonusPawn[2][32];
extern int bonusKnight[2][32];
extern int bonusBishop[2][32];
extern int bonusRook[2][32];
extern int bonusQueen[2][32];
extern int bonusKing[2][32];

#endif
