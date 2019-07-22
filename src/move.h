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

#ifndef MOVE_H
#define MOVE_H

#include "data.h"
#include "move_utils.h"
#include "target.h"

void make_move(Position *p, Move m);
void undo_move(Position *p, Move m);
void make_null_move(Position *p);
void undo_null_move(Position *p);

bool is_pseudolegal(Position *p, Move m);
bool gives_check(Position *p, Move m);

inline bool is_capture(Position *p, Move move) {
    return p->pieces[move_to(move)] != no_piece || move_type(move) == ENPASSANT;
}

inline bool is_capture_or_promotion(Position *p, Move move) {
    return is_capture(p, move) || move_type(move) == PROMOTION;
}

inline bool is_checked(Position *p) {
    Color c = p->color;
    return targeted_from(p, p->board, c, p->king_index[c]);
}

inline bool is_advanced_pawn_push(Position *p, Move move) {
    Square from = move_from(move);
    return is_pawn(p->pieces[from]) && relative_rank(from, p->color) > RANK_4;
}

bool can_king_castle(Position *p);
bool can_queen_castle(Position *p);

inline bool is_checked_color(Position *p, Color color) {
    return targeted_from(p, p->board, color, p->king_index[color]);
}

inline bool is_legal(Position *p, Move m) {
    if (move_type(m) == ENPASSANT) {
        make_move(p, m);
        // Check if the side that made the move is in check
        bool checked = is_checked_color(p, ~p->color);
        undo_move(p, m);
        return !checked;
    }

    Square to = move_to(m);
    Square from = move_from(m);
    Piece piece = p->pieces[from];

    if (is_king(piece)) {
        return std::abs(from - to) == 2 || !targeted_from_with_king(p, p->board, p->color, to);
    }

    Bitboard pinned = p->info->pinned[p->color];
    return pinned == 0 || !on(pinned, from) || (FROMTO_MASK[from][to] & p->bbs[king(p->color)]);
}

inline bool is_position_valid(Position *p) {
    if (distance(p->king_index[white], p->king_index[black]) <= 1)
        return false;

    if (p->color != white && p->color != black)
        return false;

    if (!is_king(p->pieces[p->king_index[white]]) || !is_king(p->pieces[p->king_index[black]]))
        return false;

    if (p->info->enpassant != no_sq && relative_rank(p->info->enpassant, p->color) != RANK_6)
        return false;

    return true;
}

inline Move _promoteq(Move m) {
    return m | PROMOTION_Q;
}
inline Move _promoter(Move m) {
    return m | PROMOTION_R;
}
inline Move _promoteb(Move m) {
    return m | PROMOTION_B;
}
inline Move _promoten(Move m) {
    return m | PROMOTION_N;
}

inline bool can_king_castle(Position *p) {
    Color color = p->color;
    return (p->info->castling & can_king_castle_mask[color]) &&
           !(KING_CASTLE_MASK[color] & p->board) &&
           !(KING_CASTLE_MASK_THREAT[color] & color_targets(p, ~color));
}

inline bool can_queen_castle(Position *p) {
    Color color = p->color;
    return (p->info->castling & can_queen_castle_mask[color]) &&
           !(QUEEN_CASTLE_MASK[color] & p->board) &&
           !(QUEEN_CASTLE_MASK_THREAT[color] & color_targets(p, ~color));
}
#endif

