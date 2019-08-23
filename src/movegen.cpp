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

#include <iostream>

#include "bitboard.h"
#include "move.h"
#include "movegen.h"
#include "see.h"
#include "target.h"

bool scored_move_compare(ScoredMove lhs, ScoredMove rhs) { return lhs.score < rhs.score; }

void print_movegen(MoveGen *movegen) {
    std::cout << "movegen: ";
    for (int i = movegen->head; i < movegen->tail; i++) {
        std::cout << move_to_str(movegen->moves[i].move) << "(" << movegen->moves[i].score << "), ";
    }
    std::cout << std::endl;
}

void score_moves(MoveGen *movegen, Metadata *md, ScoreType score_type) {
    if (score_type == SCORE_CAPTURE) {
        for (uint8_t i = movegen->head; i < movegen->tail; ++i) {
            movegen->moves[i].score = score_capture_mvvlva(movegen->position, movegen->moves[i].move);
        }
    } else if (score_type == SCORE_QUIET) {
        for (uint8_t i = movegen->head; i < movegen->tail; ++i) {
            movegen->moves[i].score = score_quiet(movegen->position, md, movegen->moves[i].move);
        }
    } else { // Evasions
        for (uint8_t i = movegen->head; i < movegen->tail; ++i) {
            if (is_capture(movegen->position, movegen->moves[i].move)) {
                movegen->moves[i].score = score_capture_mvvlva(movegen->position, movegen->moves[i].move);
            } else {
                movegen->moves[i].score = score_quiet(movegen->position, md, movegen->moves[i].move) - (1 << 20);
            }
        }
    }
}

ScoredMove pick_best(ScoredMove *moves, uint8_t head, uint8_t tail) {
    std::swap(moves[head], *std::max_element(moves + head, moves + tail, scored_move_compare));
    return moves[head];
}

void insertion_sort(ScoredMove *moves, uint8_t head, uint8_t tail) {
    uint8_t i, j;
    for (i = head + 1; i < tail; ++i) {
        ScoredMove tmp = moves[i];
        for (j = i; j != head && moves[j - 1].score < tmp.score; --j) {
            moves[j] = moves[j - 1];
        }
        moves[j] = tmp;
    }
}

Move next_move(MoveGen *movegen, Metadata *md, int depth) {
    Move move;
    switch (movegen->stage) {
        case NORMAL_TTE_MOVE:
            ++movegen->stage;
            return movegen->tte_move;

        case GOOD_CAPTURES_SORT:
            generate_moves<CAPTURE>(movegen, movegen->position);
            score_moves(movegen, md, SCORE_CAPTURE);
            ++movegen->stage;
            /* fallthrough */

        case GOOD_CAPTURES:
            while (movegen->head < movegen->tail) {
                move = pick_best(movegen->moves, movegen->head++, movegen->tail).move;

                if (move == movegen->tte_move) {
                    continue;
                }

                if (see_capture(movegen->position, move, movegen->threshold)) {
                    return move;
                }

                movegen->moves[movegen->end_bad_captures++] = ScoredMove{move, 0};
            }

            ++movegen->stage;
            move = md->killers[0];
            if (move != no_move &&
                move != movegen->tte_move &&
                !is_capture(movegen->position, move) &&
                is_pseudolegal(movegen->position, move)) {
                return move;
            }
            /* fallthrough */

        case KILLER_MOVE_2:
            ++movegen->stage;
            move = md->killers[1];
            if (move != no_move &&
                move != movegen->tte_move &&
                !is_capture(movegen->position, move) &&
                is_pseudolegal(movegen->position, move)) {
                return move;
            }
            /* fallthrough */

        case COUNTER_MOVES:
            ++movegen->stage;
            move = movegen->counter_move;
            if (move != no_move &&
                move != movegen->tte_move &&
                move != md->killers[0] &&
                move != md->killers[1] &&
                !is_capture(movegen->position, move) &&
                is_pseudolegal(movegen->position, move)) {
                return move;
            }
            /* fallthrough */

        case QUIETS_SORT:
            movegen->head = movegen->tail = movegen->end_bad_captures;
            generate_moves<SILENT>(movegen, movegen->position);
            score_moves(movegen, md, SCORE_QUIET);
            insertion_sort(movegen->moves, movegen->head, movegen->tail);
            ++movegen->stage;
            /* fallthrough */

        case QUIETS:
            while (movegen->head < movegen->tail) {
                move = movegen->moves[movegen->head++].move;
                if (move != movegen->tte_move &&
                    move != md->killers[0] &&
                    move != md->killers[1] &&
                    move != movegen->counter_move) {
                    return move;
                }
            }
            ++movegen->stage;
            movegen->head = 0;
            /* fallthrough */

        case BAD_CAPTURES:
            if (movegen->head < movegen->end_bad_captures) {
                move = movegen->moves[movegen->head++].move;
                if (move != movegen->tte_move) {
                    return move;
                }
            }
            break;

        case EVASIONS_SORT:
            generate_evasions(movegen, movegen->position);
            score_moves(movegen, md, SCORE_EVASION);
            ++movegen->stage;
            /* fallthrough */

        case EVASIONS:
            while (movegen->head < movegen->tail) {
                return pick_best(movegen->moves, movegen->head++, movegen->tail).move;
            }
            break;

        case QUIESCENCE_TTE_MOVE:
            ++movegen->stage;
            return movegen->tte_move;

        case QUIESCENCE_CAPTURES_SORT:
            generate_moves<CAPTURE>(movegen, movegen->position);
            score_moves(movegen, md, SCORE_CAPTURE);
            ++movegen->stage;
            /* fallthrough */

        case QUIESCENCE_CAPTURES:
            while (movegen->head < movegen->tail) {
                move = pick_best(movegen->moves, movegen->head++, movegen->tail).move;
                if (move != movegen->tte_move) {
                    return move;
                }
            }

            if (depth != 0) {
                break;
            }

            ++movegen->stage;
            generate_quiet_checks(movegen, movegen->position);
            /* fallthrough */

        case QUIESCENCE_CHECKS:
            while (movegen->head < movegen->tail) {
                move = movegen->moves[movegen->head++].move;
                if (move != movegen->tte_move) {
                    return move;
                }
            }
            break;

        case PROBCUT_TTE_MOVE:
            ++movegen->stage;
            return movegen->tte_move;

        case PROBCUT_CAPTURES_SORT:
            generate_moves<CAPTURE>(movegen, movegen->position);
            score_moves(movegen, md, SCORE_CAPTURE);
            ++movegen->stage;
            /* fallthrough */

        case PROBCUT_CAPTURES:
            while (movegen->head < movegen->tail) {
                move = pick_best(movegen->moves, movegen->head++, movegen->tail).move;
                if (move != movegen->tte_move && see_capture(movegen->position, move, movegen->threshold)) {
                    return move;
                }
            }
            break;

        default:
            assert(false);
    }

    return no_move;
}

MoveGen new_movegen(Position *p, Metadata *md, Move tte_move, uint8_t type, int threshold, bool in_check) {
    int movegen_stage;
    Move tm;
    if (in_check) {
        tm = no_move;
        movegen_stage = EVASIONS_SORT;
    } else {
        if (type == NORMAL_SEARCH) {
            tm = tte_move != no_move && is_pseudolegal(p, tte_move) ? tte_move : no_move;
            movegen_stage = NORMAL_TTE_MOVE;
        } else {
            tm = tte_move != no_move && is_pseudolegal(p, tte_move) && is_capture(p, tte_move) ? tte_move : no_move;
            movegen_stage = type == QUIESCENCE_SEARCH ? QUIESCENCE_TTE_MOVE : PROBCUT_TTE_MOVE;
        }

        if (tm == no_move) {
            ++movegen_stage;
        }
    }

    Square prev_to = move_to((md-1)->current_move);
    MoveGen movegen = {
        {}, // Moves
        p, // Position
        tm, // tte_move
        p->my_thread->counter_moves[p->pieces[prev_to]][prev_to], // counter move
        movegen_stage, // stage
        0, // head
        0, // tail
        0, // end bad captures
        threshold // threshold
    };
    assert(!(md->ply == 0 && (md-1)->current_move != no_move));
    return movegen;
}

void generate_evasions(MoveGen *movegen, Position *p) {
    Info *info = p->info;
    generate_king_evasions(movegen, p);
    // How many pieces causing check ?
    Bitboard attackers = targeted_from(p, p->board, p->color, p->king_index[p->color]);
    int piece_count = count(attackers);

    // If one attacker: Capture piece causing check or block the way
    if (piece_count == 1) {
        // Capture or block attacker
        Square attacker_index = lsb(attackers);

        // King captures are already handled in generate_king_evasions
        Bitboard capture_attackers = targeted_from(p, p->board, ~p->color, attacker_index);
        while (capture_attackers) {
            Square index = pop(&capture_attackers);
            if (is_pawn(p->pieces[index])) {
                if (relative_rank(index, p->color) == RANK_7) {
                    Move m = _movecast(index, attacker_index, PROMOTION);
                    append_move(_promoteq(m), movegen);
                    append_move(_promoter(m), movegen);
                    append_move(_promoteb(m), movegen);
                    append_move(_promoten(m), movegen);
                } else {
                    Move m = _movecast(index, attacker_index, NORMAL);
                    append_move(m, movegen);
                }
            } else {
                Move m = _movecast(index, attacker_index, NORMAL);
                append_move(m, movegen);
            }
        }

        // Enpassant capturers
        if (info->enpassant != no_sq && ENPASSANT_INDEX[info->enpassant] == attacker_index) {
            capture_attackers = targeted_from_enpassant(p, ~p->color, info->enpassant);
            while (capture_attackers) {
                Square index = pop(&capture_attackers);
                Move m = _movecast(index, info->enpassant, ENPASSANT);
                append_move(m, movegen);
            }
        }

        Bitboard between_two = BETWEEN_MASK[p->king_index[p->color]][attacker_index];

        while (between_two) {
            Square blocking_square = pop(&between_two);
            Bitboard blockers = can_go_to(p, ~p->color, blocking_square);
            while (blockers) {
                Square index = pop(&blockers);
                if (is_pawn(p->pieces[index]) && relative_rank(index, p->color) == RANK_7) {
                    Move m = _movecast(index, blocking_square, PROMOTION);
                    append_move(_promoteq(m), movegen);
                    append_move(_promoter(m), movegen);
                    append_move(_promoteb(m), movegen);
                    append_move(_promoten(m), movegen);
                } else {
                    Move m = _movecast(index, blocking_square, NORMAL);
                    append_move(m, movegen);
                }
            }
        }
    }
}

void generate_king_evasions(MoveGen *movegen, Position *p) {
    Square k_index = p->king_index[p->color];

    // Remove the king from the board temporarily
    p->board ^= bfi[k_index];
    Bitboard b = generate_king_targets(k_index) & ~p->bbs[p->color] & ~color_targets(p, ~p->color);
    p->board ^= bfi[k_index];
 
    while (b) {
        Square index = pop(&b);
        Move m = _movecast(k_index, index, NORMAL);
        append_evasion(m, movegen);
    }
}

void generate_quiet_checks(MoveGen *movegen, Position *p) {
    Square king_index = p->king_index[~p->color];
    Bitboard non_capture = ~p->board;

    // Knight checks
    Bitboard knight_checkers = KNIGHT_MASKS[king_index] & non_capture;
    Bitboard knights = p->bbs[knight(p->color)];
    while (knights) {
        Square sq = pop(&knights);
        Bitboard knight_checks = KNIGHT_MASKS[sq] & knight_checkers;
        while (knight_checks) {
            Move m = _movecast(sq, pop(&knight_checks), NORMAL);
            append_move(m, movegen);
        }
    } 

    // Bishop checks
    Bitboard bishop_checkers = generate_bishop_targets(p->board, king_index) & non_capture;
    Bitboard bishops = p->bbs[bishop(p->color)];
    while (bishops) {
        Square sq = pop(&bishops);
        Bitboard bishop_checks = generate_bishop_targets(p->board, sq) & bishop_checkers;
        while (bishop_checks) {
            Move m = _movecast(sq, pop(&bishop_checks), NORMAL);
            append_move(m, movegen);
        }
    }


    // Rook checks
    Bitboard rook_checkers = generate_rook_targets(p->board, king_index) & non_capture;
    Bitboard rooks = p->bbs[rook(p->color)];
    while (rooks) {
        Square sq = pop(&rooks);
        Bitboard rook_checks = generate_rook_targets(p->board, sq) & rook_checkers;
        while (rook_checks) {
            Move m = _movecast(sq, pop(&rook_checks), NORMAL);
            append_move(m, movegen);
        }
    }


    // Queen checks
    Bitboard queen_checkers = rook_checkers | bishop_checkers;
    Bitboard queens = p->bbs[queen(p->color)];
    while (queens) {
        Square sq = pop(&queens);
        Bitboard queen_checks = generate_queen_targets(p->board, sq) & queen_checkers;
        while (queen_checks) {
            Move m = _movecast(sq, pop(&queen_checks), NORMAL);
            append_move(m, movegen);
        }
    }

    // Possible discovered checks
    Bitboard pinned_pieces = p->info->pinned[~p->color] & p->bbs[p->color];
    while (pinned_pieces) {
        Square sq = pop(&pinned_pieces);
        int pin_piece = piece_type(p->pieces[sq]);
        Bitboard move_locations = 0;

        switch(pin_piece) {
            case KNIGHT:
                move_locations |= ~FROMTO_MASK[sq][king_index] & generate_knight_targets(sq) & non_capture;
                break;
            case BISHOP:
                move_locations |= ~FROMTO_MASK[sq][king_index] & generate_bishop_targets(p->board, sq) & non_capture;
                break;
            case ROOK:
                move_locations |= ~FROMTO_MASK[sq][king_index] & generate_rook_targets(p->board, sq) & non_capture;
                break;
            case QUEEN:
                move_locations |= ~FROMTO_MASK[sq][king_index] & generate_queen_targets(p->board, sq) & non_capture;
                break;
            case KING:
                move_locations |= ~FROMTO_MASK[sq][king_index] & generate_king_targets(sq) & non_capture;
                break;
            case PAWN:
                move_locations |= ~FROMTO_MASK[sq][king_index] & generate_pawn_targets<SILENT>(p, sq) & ~(RANK_1BB | RANK_8BB);
                break;
        }

        while (move_locations) {
            Square move_index = pop(&move_locations);
            Move m = _movecast(sq, move_index, NORMAL);
            append_move(m, movegen);
        }
    }
}
