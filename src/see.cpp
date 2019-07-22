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

#include "bitboard.h"
#include "data.h"
#include "move_utils.h"
#include "see.h"
#include "target.h"

Square get_smallest_attacker(Position *p, Bitboard targeters, Color color) {
    Piece piece = color == white ? white_pawn : black_pawn;
    Piece target = color == white ? white_king : black_king;

    for (; piece <= target; piece += 2) {
        Bitboard intersection = p->bbs[piece] & targeters;
        if (intersection) {
            return lsb(intersection);
        }
    }
    assert(false);
    return 0;
}

bool see_capture(Position *p, Move move, int threshold) {
    int m_type = move_type(move);
    if (m_type != NORMAL) {
        return threshold <= 0;
    }

    Square from = move_from(move);
    Square to = move_to(move);
    Color color = piece_color(p->pieces[from]);
    
    int piece_value = piece_values[p->pieces[to]];
    int capturer_value = piece_values[p->pieces[from]];

    int balance = piece_value - threshold;
    if (balance < 0) {
        return false;
    }

    balance -= capturer_value;
    if (balance >= 0) {
        return true;
    }

    bool opponent_to_move = true;

    Bitboard board = p->board ^ bfi[from];
    Bitboard targeters = all_targets(p, board, to) & board;

    Bitboard rooks = p->bbs[white_rook] | p->bbs[black_rook];
    Bitboard bishops = p->bbs[white_bishop] | p->bbs[black_bishop];
    Bitboard queens = p->bbs[white_queen] | p->bbs[black_queen];

    while (true) {
        Bitboard my_targeters = targeters & p->bbs[~color];
        if (!my_targeters) {
            break;
        }

        Square attacker_sq = get_smallest_attacker(p, my_targeters, ~color);
        board ^= bfi[attacker_sq];
        int p_type = piece_type(p->pieces[attacker_sq]);
        if (p_type == PAWN || p_type == BISHOP || p_type == QUEEN) {
            targeters |= generate_bishop_targets(board, to) & (bishops | queens);
        }

        if (p_type == ROOK || p_type == QUEEN) {
            targeters |= generate_rook_targets(board, to) & (rooks | queens);
        }
        targeters &= board;

        balance += piece_values[p->pieces[attacker_sq]];;

        opponent_to_move = !opponent_to_move;

        if (balance < 0) {
            break;
        }

        balance = - balance - 1;
        color = ~color;
    }
    return opponent_to_move;
}
