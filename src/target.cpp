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

#include "target.h"
#include "bitboard.h"
#include "magic.h"

Bitboard generate_rook_targets(Bitboard board, Square index) {
    int magic = ((rookMagic[index].mask & board) * rookMagic[index].magic) >> 52;
    return rook_magic_moves[index][magic];
}

Bitboard generate_bishop_targets(Bitboard board, Square index) {
    int magic = ((bishopMagic[index].mask & board) * bishopMagic[index].magic) >> 55;
    return bishop_magic_moves[index][magic];
}

Bitboard generate_knight_targets(Square index) {
    return KNIGHT_MASKS[index];
}

Bitboard generate_queen_targets(Bitboard board, Square index) {
    return generate_bishop_targets(board, index) | generate_rook_targets(board, index);
}

Bitboard generate_king_targets(Square index) {
    return KING_MASKS[index];
}

Bitboard generate_pawn_targets_to_index(Position *p, Square index) {
    Bitboard targets = 0;
    Bitboard bbs = p->bbs[pawn(p->color)] & ROOK_MASKS_VERTICAL[index];;
    while (bbs) {
        Square sq = pop(&bbs);
        Bitboard pawn_targets = generate_pawn_targets<SILENT>(p, sq);
        if (pawn_targets & bfi[index]) {
            targets |= bfi[sq];
        }
    }
    return targets;
}

Bitboard generate_pawn_threats(Bitboard pawns, Color color) {
    return color == white ? ((pawns & ~FILE_ABB) << 7) | ((pawns & ~FILE_HBB) << 9) : ((pawns & ~FILE_HBB) >> 7) | ((pawns & ~FILE_ABB) >> 9);
}

Bitboard pinned_piece_squares(Position *p, Color color){
    Bitboard return_board = 0;
    Square k_index = p->king_index[color];

    Bitboard attackers = (p->bbs[bishop(~color)] | p->bbs[queen(~color)]) & (BISHOP_MASKS_COMBINED[k_index]);
    attackers |= (p->bbs[rook(~color)] | p->bbs[queen(~color)]) & (ROOK_MASKS_COMBINED[k_index]);

    while(attackers){
        Square index = pop(&attackers);

        Bitboard temp = p->board & BETWEEN_MASK[k_index][index];
        if (!more_than_one(temp)) {
            return_board |= temp;
        }
    }

    return return_board;
}

Bitboard targeted_from(Position *p, Bitboard board, Color color, Square index) {
    Bitboard rook_targets = generate_rook_targets(board, index);
    Bitboard bishop_targets = generate_bishop_targets(board, index);
    Bitboard knight_targets = generate_knight_targets(index);
    Bitboard pawn_targets_capture = PAWN_CAPTURE_MASK[index][color];

    return (rook_targets & (p->bbs[rook(~color)] | p->bbs[queen(~color)])) |
           (bishop_targets & (p->bbs[bishop(~color)] | p->bbs[queen(~color)])) |
           (knight_targets & p->bbs[knight(~color)]) |
           (pawn_targets_capture & p->bbs[pawn(~color)]);
}

Bitboard targeted_from_with_king(Position *p, Bitboard board, Color color, Square index) {
    Bitboard rook_targets = generate_rook_targets(board, index);
    Bitboard bishop_targets = generate_bishop_targets(board, index);
    Bitboard knight_targets = generate_knight_targets(index);
    Bitboard pawn_targets_capture = PAWN_CAPTURE_MASK[index][color];
    Bitboard king_targets = generate_king_targets(index);

    return (rook_targets & (p->bbs[rook(~color)] | p->bbs[queen(~color)])) |
           (bishop_targets & (p->bbs[bishop(~color)] | p->bbs[queen(~color)])) |
           (knight_targets & p->bbs[knight(~color)]) |
           (pawn_targets_capture & p->bbs[pawn(~color)]) |
           (king_targets & p->bbs[king(~color)]);
}

Bitboard all_targets(Position *p, Bitboard board, Square index) {
    Bitboard rook_targets = generate_rook_targets(board, index);
    Bitboard bishop_targets = generate_bishop_targets(board, index);
    Bitboard knight_targets = generate_knight_targets(index);
    Bitboard white_pawn_captures = PAWN_CAPTURE_MASK[index][black];  // Not a typo
    Bitboard black_pawn_captures = PAWN_CAPTURE_MASK[index][white];  // Not a typo
    Bitboard king_targets = generate_king_targets(index);

    Bitboard rooks = p->bbs[white_rook] | p->bbs[black_rook];
    Bitboard bishops = p->bbs[white_bishop] | p->bbs[black_bishop];
    Bitboard knights = p->bbs[white_knight] | p->bbs[black_knight];
    Bitboard queens = p->bbs[white_queen] | p->bbs[black_queen];
    Bitboard white_pawns = p->bbs[white_pawn];
    Bitboard black_pawns = p->bbs[black_pawn];
    Bitboard kings = p->bbs[white_king] | p->bbs[black_king];

    return (rook_targets & (rooks | queens)) |
           (bishop_targets & (bishops | queens)) |
           (knight_targets & knights) |
           (white_pawn_captures & white_pawns) |
           (black_pawn_captures & black_pawns) |
           (king_targets & kings);
}

Bitboard targeted_from_enpassant(Position *p, Color color, Square index) {
    return PAWN_CAPTURE_MASK[index][color] & p->bbs[pawn(~color)];
}

Bitboard can_go_to(Position *p, Color color, Square index) {
    Bitboard rook_targets = generate_rook_targets(p->board, index);
    Bitboard bishop_targets = generate_bishop_targets(p->board, index);
    Bitboard knight_targets = generate_knight_targets(index);
    Bitboard pawn_targets = generate_pawn_targets_to_index(p, index);

    return (rook_targets & (p->bbs[rook(~color)] | p->bbs[queen(~color)])) |
           (bishop_targets & (p->bbs[bishop(~color)] | p->bbs[queen(~color)])) |
           (knight_targets & p->bbs[knight(~color)]) |
           pawn_targets;
}

Bitboard color_targets(Position *p, Bitboard board, Color color) {
    Bitboard final_board = 0;

    Bitboard bbs = p->bbs[knight(color)];
    while (bbs) {
        Square sq = pop(&bbs);
        final_board |= generate_knight_targets(sq);
    }

    bbs = p->bbs[rook(color)];
    while (bbs) {
        Square sq = pop(&bbs);
        final_board |= generate_rook_targets(board, sq);
    }

    bbs = p->bbs[bishop(color)];
    while (bbs) {
        Square sq = pop(&bbs);
        final_board |= generate_bishop_targets(board, sq);
    }

    bbs = p->bbs[queen(color)];
    while (bbs) {
        Square sq = pop(&bbs);
        final_board |= generate_queen_targets(board, sq);
    }

    final_board |= generate_king_targets(p->king_index[color]);

    bbs = p->bbs[pawn(color)];
    final_board |= generate_pawn_threats(bbs, color);

    return final_board;
}
