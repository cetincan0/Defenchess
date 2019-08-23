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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "data.h"
#include "bitboard.h"
#include "move.h"
#include "target.h"

void print_movegen(MoveGen *movegen);
MoveGen new_movegen(Position *p, Metadata *md, Move tt_move, uint8_t type, int threshold, bool in_check);

void generate_evasions(MoveGen *movegen, Position *p);
void generate_king_evasions(MoveGen *movegen, Position *p);
Move next_move(MoveGen *movegen, Metadata *md, int depth);

inline int score_quiet(Position *p, Metadata *md, Move move) {
    Square from = move_from(move);
    Square to = move_to(move);

    Move prev_move = (md-1)->current_move;
    Piece piece = p->pieces[from];
    Square prev_to = move_to(prev_move);
    Piece prev_piece = (md-1)->moved_piece;

    Move followup_move = (md-2)->current_move;
    Square followup_to = move_to(followup_move);
    Piece followup_piece = (md-2)->moved_piece;

    assert(prev_piece == no_piece || (prev_piece >= white_pawn && prev_piece <= black_king));
    assert(followup_piece == no_piece || (followup_piece >= white_pawn && followup_piece <= black_king));
    return p->my_thread->history[p->color][from][to] +
           p->my_thread->counter_move_history[prev_piece][prev_to][piece][to] +
           p->my_thread->followup_history[followup_piece][followup_to][piece][to];
}

inline int score_capture_mvvlva(Position *p, Move move) {
    Piece from_piece = p->pieces[move_from(move)];
    Piece to_piece = p->pieces[move_to(move)];

    // For a pawn capturing a queen, we get 1200 - 2
    // For a queen capturing a pawn, we get 100 - 10 or something
    return mvvlva_values[to_piece][from_piece];
}

inline bool no_moves(MoveGen *movegen) {
    return movegen->tail == movegen->head;
}

inline void append_move(Move m, MoveGen *movegen) {
#ifdef __PERFT__
    if (is_legal(movegen->position, m)){
        movegen->moves[movegen->tail++] = ScoredMove{m, UNDEFINED};
    }
#else
    movegen->moves[movegen->tail++] = ScoredMove{m, UNDEFINED};
#endif
}

// Should not have to check legality...
inline void append_evasion(Move m, MoveGen *movegen) {
    movegen->moves[movegen->tail++] = ScoredMove{m, UNDEFINED};
}

template<MoveGenType Type> inline Bitboard type_mask(Position *p) {
    if (Type == SILENT) {
        return ~p->board;
    } else if (Type == CAPTURE) {
        return p->bbs[~p->color];
    }
    return ~p->bbs[p->color];
}

template<MoveGenType Type>
void generate_piece_moves(MoveGen *movegen, Position *p) {
    Bitboard mask = type_mask<Type>(p);

    Bitboard bbs = p->bbs[knight(p->color)];
    while (bbs) {
        Square sq = pop(&bbs);
        Bitboard b = generate_knight_targets(sq) & mask;
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(sq, index, NORMAL);
            append_move(m, movegen);
        }
    }
    
    bbs = p->bbs[bishop(p->color)];
    while (bbs) {
        Square sq = pop(&bbs);
        Bitboard b = generate_bishop_targets(p->board, sq) & mask;
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(sq, index, NORMAL);
            append_move(m, movegen);
        }
    }

    bbs = p->bbs[rook(p->color)];
    while (bbs) {
        Square sq = pop(&bbs);
        Bitboard b = generate_rook_targets(p->board, sq) & mask;
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(sq, index, NORMAL);
            append_move(m, movegen);
        }
    }

    bbs = p->bbs[queen(p->color)];
    while (bbs) {
        Square sq = pop(&bbs);
        Bitboard b = generate_queen_targets(p->board, sq) & mask;
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(sq, index, NORMAL);
            append_move(m, movegen);
        }
    }
}

inline void append_promos(Move m, MoveGen *movegen) {
    append_move(_promoteq(m), movegen);
    append_move(_promoten(m), movegen);
    append_move(_promoter(m), movegen);
    append_move(_promoteb(m), movegen);
}

template<MoveGenType Type>
void generate_all_pawn_moves(MoveGen *movegen, Position *p) {
    Color color = p->color;
    Bitboard pawns = p->bbs[pawn(color)];
    Bitboard relative_rank3 = color == white ? RANK_3BB : RANK_6BB;
    Bitboard relative_rank7 = color == white ? RANK_7BB : RANK_2BB;
    Bitboard promoting_pawns = pawns & relative_rank7;
    Bitboard non_promoting_pawns = pawns ^ promoting_pawns;
    Bitboard capture_squares = p->bbs[~color] | bfi[p->info->enpassant];
    Bitboard one_move, two_move, can_two_move, left_captures, right_captures, empty_squares;
    int down = color == white ? -8 : 8;
    int down_left = color == white ? -9 : 9;
    int down_right = color == white ? -7 : 7;

    // No promos
    if (Type != CAPTURE) {
        empty_squares = ~p->board;
        one_move = empty_squares & (color == white ? (non_promoting_pawns << 8) : (non_promoting_pawns >> 8));
        can_two_move = one_move & relative_rank3;
        two_move = empty_squares & (color == white ? (can_two_move << 8) : (can_two_move >> 8));

        while (one_move) {
            Square to = pop(&one_move);
            Move m = _movecast(to + down, to, NORMAL);
            append_move(m, movegen);
        }

        while (two_move) {
            Square to = pop(&two_move);
            Move m = _movecast(to + 2 *down, to, NORMAL);
            append_move(m, movegen);
        }
    }

    if (Type != SILENT) {
        left_captures = capture_squares & (color == white ? ((non_promoting_pawns & ~FILE_ABB) << 7) : ((non_promoting_pawns & ~FILE_HBB) >> 7));
        right_captures = capture_squares & (color == white ? ((non_promoting_pawns & ~FILE_HBB) << 9) : ((non_promoting_pawns & ~FILE_ABB) >> 9));

        while (left_captures) {
            Square to = pop(&left_captures);
            if (to == p->info->enpassant) {
                Move m = _movecast(to + down_right, to, ENPASSANT);
                append_move(m, movegen);
            } else {
                Move m = _movecast(to + down_right, to, NORMAL);
                append_move(m, movegen);
            }
        }

        while (right_captures) {
            Square to = pop(&right_captures);
            if (to == p->info->enpassant) {
                Move m = _movecast(to + down_left, to, ENPASSANT);
                append_move(m, movegen);
            } else {
                Move m = _movecast(to + down_left, to, NORMAL);
                append_move(m, movegen);
            }
        }

        // Consider all promotions captures so that they're also evaluated
        // in quiescence search
        if (promoting_pawns) {
            one_move = ~p->board & (color == white ? (promoting_pawns << 8) : (promoting_pawns >> 8));
            left_captures = capture_squares & (color == white ? ((promoting_pawns & ~FILE_ABB) << 7) : ((promoting_pawns & ~FILE_HBB) >> 7));
            right_captures = capture_squares & (color == white ? ((promoting_pawns & ~FILE_HBB) << 9) : ((promoting_pawns & ~FILE_ABB) >> 9));

            while (one_move) {
                Square to = pop(&one_move);
                Move m = _movecast(to + down, to, PROMOTION);
                append_promos(m, movegen);
            }

            while (left_captures) {
                Square to = pop(&left_captures);
                Move m = _movecast(to + down_right, to, PROMOTION);
                append_promos(m, movegen);
            }

            while (right_captures) {
                Square to = pop(&right_captures);
                Move m = _movecast(to + down_left, to, PROMOTION);
                append_promos(m, movegen);
            }
        }
    }

}

template<MoveGenType Type>
void generate_king_moves(MoveGen *movegen, Position *p) {
    Square sq = p->king_index[p->color];
    Bitboard b = 0;
    if (Type != CAPTURE) {
        if (can_king_castle(p)) {
            b |= bfi_king_castle[p->color];
        }
        if (can_queen_castle(p)) {
            b |= bfi_queen_castle[p->color];
        }
        while (b) {
            Square index = pop(&b);
            Move m = _movecast(sq, index, CASTLING);
            append_move(m, movegen);
        }
    }

    Bitboard mask = type_mask<Type>(p);
    b = generate_king_targets(sq) & mask;
    while (b) {
        Square index = pop(&b);
        Move m = _movecast(sq, index, NORMAL);
        append_move(m, movegen);
    }
}

template<MoveGenType Type>
void generate_moves(MoveGen *movegen, Position *p) {
    generate_all_pawn_moves<Type>(movegen, p);
    generate_piece_moves<Type>(movegen, p);
    generate_king_moves<Type>(movegen, p);
}

void generate_quiet_checks(MoveGen *movegen, Position *p);
#endif
