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

#ifndef TARGET_H
#define TARGET_H

#include "data.h"

Bitboard pinned_piece_squares(Position *p, Color color);

Bitboard generate_rook_targets(Bitboard board, Square index);
Bitboard generate_bishop_targets(Bitboard board, Square index);
Bitboard generate_knight_targets(Square index);
Bitboard generate_queen_targets(Bitboard board, Square index);

template<MoveGenType Type>
Bitboard generate_pawn_targets(Position *p, Square index) {
    Bitboard targets = 0;
    Color color = p->color;

    if (Type != CAPTURE) {
        Bitboard empty_squares = ~p->board;
        Bitboard one_move = PAWN_ADVANCE_MASK_1[index][color] & empty_squares;
        if (one_move) {
            targets |= one_move;
            targets |= PAWN_ADVANCE_MASK_2[index][color] & empty_squares;
        }
    }

    if (Type != SILENT) {
        targets |= (p->bbs[~color] | bfi[p->info->enpassant]) & PAWN_CAPTURE_MASK[index][color];
    }

    return targets;
}

Bitboard generate_king_targets(Square index);

Bitboard color_targets(Position *p, Bitboard board, Color color);

Bitboard targeted_from(Position *p, Bitboard board, Color c, Square index);
Bitboard targeted_from_enpassant(Position *p, Color c, Square index);

Bitboard targeted_from_with_king(Position *p, Bitboard board, Color c, Square index);
Bitboard all_targets(Position *p, Bitboard board, Square index);

Bitboard can_go_to(Position *p, Color c, Square index);

Bitboard generate_pawn_targets_to_index(Position *p, Square index);
Bitboard generate_pawn_threats(Bitboard pawns, Color color);

#endif
