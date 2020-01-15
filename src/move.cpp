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

#include "move.h"
#include "target.h"
#include "movegen.h"

void insert_piece_no_hash(Position *p, Square sq, Piece piece) {
    Color color = piece_color(piece);
    p->pieces[sq] = piece;
    Bitboard bb = bfi[sq];
    p->bbs[piece] |= bb;
    p->bbs[color] |= bb;
    p->board |= bb;
    p->score += pst[piece][sq];
}

void remove_piece_no_hash(Position *p, Square sq, Piece piece) {
    Color color = piece_color(piece);
    p->pieces[sq] = no_piece;
    Bitboard bb = bfi[sq];
    p->bbs[piece] ^= bb;
    p->bbs[color] ^= bb;
    p->board ^= bb;
    p->score -= pst[piece][sq];
}

void move_piece_no_hash(Position *p, Square from, Square to, Piece piece, Color color) {
    // Same as move_piece without the hash changes
    p->pieces[from] = no_piece;
    p->pieces[to] = piece;
    Bitboard from_to = bfi[from] ^ bfi[to];
    p->bbs[piece] ^= from_to;
    p->bbs[color] ^= from_to;
    p->board ^= from_to;
    p->score += pst[piece][to] - pst[piece][from];
}

void insert_rook(Position *p, Square sq, Piece piece) {
    Color color = piece_color(piece);
    p->pieces[sq] = piece;
    Bitboard bb = bfi[sq];
    p->bbs[piece] |= bb;
    p->bbs[color] |= bb;
    p->board |= bb;

    uint64_t h = hash_combined[piece][sq];
    p->info->hash ^= h;
    p->score += pst[piece][sq];
}

void remove_rook(Position *p, Square sq, Piece piece) {
    Color color = piece_color(piece);
    p->pieces[sq] = no_piece;
    Bitboard bb = bfi[sq];
    p->bbs[piece] ^= bb;
    p->bbs[color] ^= bb;
    p->board ^= bb;

    uint64_t h = hash_combined[piece][sq];
    p->info->hash ^= h;
    p->score -= pst[piece][sq];
}

void move_piece(Position *p, Square from, Square to, Piece piece, Color color) {
    p->pieces[from] = no_piece;
    p->pieces[to] = piece;
    Bitboard from_to = bfi[from] ^ bfi[to];
    p->bbs[piece] ^= from_to;
    p->bbs[color] ^= from_to;
    p->board ^= from_to;

    uint64_t h = hash_combined[piece][from] ^ hash_combined[piece][to];
    Info *info = p->info;
    info->hash ^= h;
    if (is_pawn(piece)) {
        info->pawn_hash ^= h;
    }
    p->score += pst[piece][to] - pst[piece][from];
}

void capture(Position *p, Square to, Piece captured, Color opponent) {
    p->bbs[captured] ^= bfi[to];
    p->bbs[opponent] ^= bfi[to];
    p->board ^= bfi[to];

    uint64_t h = hash_combined[captured][to];
    Info *info = p->info;
    info->hash ^= h;
    if (is_pawn(captured)) {
        info->pawn_hash ^= h;
    } else {
        info->non_pawn_material[opponent] -= piece_values[captured];
    }
    info->material_index -= material_balance[captured];
    p->score -= pst[captured][to];
}

void capture_enpassant(Position *p, Square to, Square enpassant_to, Piece captured, Color opponent) {
    p->bbs[captured] ^= bfi[enpassant_to];
    p->bbs[opponent] ^= bfi[enpassant_to];
    p->pieces[enpassant_to] = no_piece;
    p->board ^= bfi[enpassant_to];

    uint64_t h = hash_combined[captured][to];
    Info *info = p->info;
    info->hash ^= h;
    info->pawn_hash ^= h;
    info->material_index -= material_balance[captured];
    p->score -= pst[captured][enpassant_to];
}

void promote(Position *p, Square to, Piece pawn, Piece promotion_piece, Color color) {
    p->pieces[to] = promotion_piece;
    p->bbs[pawn] ^= bfi[to];
    p->bbs[promotion_piece] ^= bfi[to];

    uint64_t h = hash_combined[pawn][to];
    Info *info = p->info;
    info->pawn_hash ^= h;
    info->hash ^= h ^ hash_combined[promotion_piece][to];
    info->material_index += material_balance[promotion_piece] - material_balance[pawn];
    info->non_pawn_material[color] += piece_values[promotion_piece];
    p->score += pst[promotion_piece][to] - pst[pawn][to];
}


void make_move(Position *p, Move move) {
    SearchThread *my_thread = p->my_thread;
    ++my_thread->search_ply;

    Info *info = p->info;
    std::memcpy(info + 1, info, info_size);
    Info *new_info = info + 1;
    p->info = new_info;
    new_info->previous = info;

    ++new_info->last_irreversible;
    Square from = move_from(move);
    Square to = move_to(move);
    Color color = p->color;
    Color opponent = ~color;
    Piece piece = p->pieces[from];
    int m_type = move_type(move);
    Piece captured = m_type == ENPASSANT ? pawn(opponent) : p->pieces[to];

    assert(piece_color(piece) == color);
    assert(!(piece_color(captured) == opponent && is_king(captured)));

    new_info->enpassant = no_sq;
    new_info->hash = info->hash ^ white_hash;

    if (info->enpassant != no_sq) {
        // Clear the enpassant hash from the previous position
        new_info->hash ^= enpassant_hash[file_of(info->enpassant)];
    }

    if (captured != no_piece) {
        new_info->last_irreversible = 0;
        if (m_type == ENPASSANT) {
            Square enpassant_to = ENPASSANT_INDEX[to];
            assert(piece == pawn(color));
            assert(to == info->enpassant);
            assert(info->enpassant != 0);
            assert(relative_rank(to, color) == RANK_6);
            assert(p->pieces[to] == no_piece);
            assert(p->pieces[enpassant_to] == pawn(opponent));

            capture_enpassant(p, to, enpassant_to, captured, opponent);
        } else if (m_type != CASTLING) {
            capture(p, to, captured, m_type == CASTLING ? color : opponent);
        }
    }

    if (m_type != CASTLING) {
        move_piece(p, from, to, piece, color);
    }

    if (is_king(piece)) {
        p->king_index[color] = to;
        if (m_type == CASTLING) {
            new_info->last_irreversible = 0;
            Square rook_sq = p->initial_rooks[color][CASTLE_TYPE[to]];
            remove_rook(p, rook_sq, rook(color));
            move_piece(p, from, to, piece, color);
            insert_rook(p, ROOK_MOVES_CASTLE_TO[to], rook(color));
        }
    } else if (is_pawn(piece)) {
        new_info->last_irreversible = 0;
        if ((from ^ to) == 16) {
            if (p->bbs[pawn(opponent)] & ADJACENT_MASK[to]) {
                new_info->enpassant = ENPASSANT_INDEX[from];
                new_info->hash ^= enpassant_hash[file_of(new_info->enpassant)];
            }
        } else if (m_type == PROMOTION) {
            assert(relative_rank(to, color) == RANK_8);
            promote(p, to, piece, get_promotion_piece(move, color), color);
        }
    }

    if (new_info->castling && (p->CASTLING_RIGHTS[from] & p->CASTLING_RIGHTS[to])) {
        int castling_rights = p->CASTLING_RIGHTS[from] & p->CASTLING_RIGHTS[to];
        new_info->castling &= castling_rights;
        new_info->hash ^= castling_hash[castling_rights];
    }

    p->color = opponent;
    new_info->captured = captured;
    new_info->pinned[white] = pinned_piece_squares(p, white);
    new_info->pinned[black] = pinned_piece_squares(p, black);

    assert(is_position_valid(p));
}

void make_null_move(Position *p) {
    SearchThread *my_thread = p->my_thread;
    ++my_thread->search_ply;

    Info *info = p->info;
    std::memcpy(info + 1, info, info_size);
    Info *new_info = info + 1;
    p->info = new_info;
    new_info->previous = info;

    ++new_info->last_irreversible;

    new_info->enpassant = no_sq;
    new_info->hash = info->hash ^ white_hash;

    if (info->enpassant != no_sq) {
        new_info->hash ^= enpassant_hash[file_of(info->enpassant)];
    }

    p->color = ~p->color;
    new_info->pinned[white] = pinned_piece_squares(p, white);
    new_info->pinned[black] = pinned_piece_squares(p, black);

    assert(is_position_valid(p));
}

void undo_move(Position *p, Move move) {
    --p->my_thread->search_ply;
    p->color = ~p->color;

    Info *info = p->info;
    Color color = p->color;
    Square from = move_from(move);
    Square to = move_to(move);
    Piece piece = p->pieces[to];

    if (is_king(piece)) {
        p->king_index[color] = from;
    }
    if (move_type(move) == NORMAL) {
        move_piece_no_hash(p, to, from, piece, color);
        if (info->captured != no_piece) {
            insert_piece_no_hash(p, to, info->captured);
        }
    } else if (move_type(move) == PROMOTION) {
        remove_piece_no_hash(p, to, piece);
        insert_piece_no_hash(p, from, pawn(color));
        if (info->captured != no_piece) {
            insert_piece_no_hash(p, to, info->captured);
        }
    } else if (move_type(move) == CASTLING) {
        Square rook_from = p->initial_rooks[color][CASTLE_TYPE[to]];
        Square rook_to = relative_square(file_of(to) == FILE_G ? F1 : D1, color);
        remove_piece_no_hash(p, rook_to, rook(color));
        move_piece_no_hash(p, to, from, piece, color);
        insert_piece_no_hash(p, rook_from, rook(color));
    } else { // Enpassant
        move_piece_no_hash(p, to, from, piece, color);
        assert(is_pawn(piece));
        assert(to == info->previous->enpassant);
        assert(relative_rank(to, color) == RANK_6);
        assert(info->captured == pawn(~color));
        insert_piece_no_hash(p, pawn_backward(to, color), info->captured);
    }
    p->info = info->previous;
}

void undo_null_move(Position *p) {
    --p->my_thread->search_ply;
    p->color = ~p->color;
    p->info = p->info->previous;
}

bool is_pseudolegal(Position *p, Move move) {
    Square from = move_from(move);
    Piece piece = p->pieces[from];
    int p_type = piece_type(piece);
    int m_type = move_type(move);

    if (m_type != NORMAL) {
        MoveGen movegen = blank_movegen;
        movegen.position = p;
        generate_moves<ALL>(&movegen, p);
        for (uint8_t i = movegen.head; i < movegen.tail; ++i) {
            Move gen_move = movegen.moves[i].move;
            if (move == gen_move) {
                return true;
            }
        }
        return false;
    }

    if (promotion_type(move)) {
        return false;
    }

    if (piece == no_piece) {
        return false;
    }
    if (piece_color(piece) != p->color) {
        return false;
    }

    Square to = move_to(move);
    if (p->bbs[p->color] & bfi[to]) {
        return false;
    }

    Bitboard b = 0;
    if (p_type == KING) {
        b = generate_king_targets(from);
    } else if (p_type == PAWN) {
        if (relative_rank(to, p->color) == RANK_8) {
            return false;
        }
        if (file_of(from) != file_of(to) && !(p->board & bfi[to])) {  // Enpassant
            return false;
        }
        b = generate_pawn_targets<ALL>(p, from);
    } else if (p_type == KNIGHT) {
        b = generate_knight_targets(from);
    } else if (p_type == BISHOP) {
        b = generate_bishop_targets(p->board, from);
    } else if (p_type == ROOK) {
        b = generate_rook_targets(p->board, from);
    } else if (p_type == QUEEN) {
        b = generate_queen_targets(p->board, from);
    }
    return b & bfi[to];
}

bool gives_check(Position *p, Move m) {
    Square their_king_index = p->king_index[~p->color];
    Bitboard their_king = bfi[their_king_index];

    Square from = move_from(m);
    Square to = move_to(m);
    Piece curr_piece = p->pieces[from];
    int p_type = piece_type(curr_piece);

    if (p_type == PAWN) {
        // Directly checks
        if (PAWN_CAPTURE_MASK[to][p->color] & their_king) {
            return true;
        }

        // Causes a discovered check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, ~p->color), from)) {
            return true;
        }

        // Promotion check
        if (move_type(m) == PROMOTION) {
            if ((is_queen_promotion(m) && (generate_queen_targets(p->board ^ bfi[from], to) & their_king)) ||
                (is_rook_promotion(m) && (generate_rook_targets(p->board ^ bfi[from], to) & their_king)) ||
                (is_bishop_promotion(m) && (generate_bishop_targets(p->board ^ bfi[from], to) & their_king)) ||
                (is_knight_promotion(m) && (generate_knight_targets(to) & their_king))) {
                return true;
            }
        }

        if (move_type(m) == ENPASSANT) {
            make_move(p, m);
            if (is_checked(p)) {
                undo_move(p, m);
                return true;
            }
            undo_move(p, m);
        } 

    } else if (p_type == KING) {
        // Causes a discover check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, ~p->color), from)) {
            return true;
        }

        // Castling check
        if (move_type(m) == CASTLING) {
            make_move(p, m);
            if (is_checked(p)) {
                undo_move(p, m);
                return true;
            }
            undo_move(p, m);
        }
    } else if (p_type == QUEEN) {
        // Directly checks
        if (generate_queen_targets(p->board, to) & their_king) {
            return true;
        }

        // Causes a discovered check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, ~p->color), from)) {
            return true;
        }
    } else if (p_type == ROOK) {
        // Directly checks
        if (generate_rook_targets(p->board, to) & their_king) {
            return true;
        }

        // Causes a discovered check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, ~p->color), from)) {
            return true;
        }
    } else if (p_type == BISHOP) {
        // Directly checks
        if (generate_bishop_targets(p->board, to) & their_king) {
            return true;
        }

        // Causes a discovered check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, ~p->color), from)) {
            return true;
        }
    } else if (p_type == KNIGHT) {
        // Directly checks
        if (KNIGHT_MASKS[to] & their_king) {
            return true;
        }

        // Causes a discovered check
        if (!on(FROMTO_MASK[from][to], their_king_index) && on(pinned_piece_squares(p, ~p->color), from)) {
            return true;
        }
    }

    return false;
}
