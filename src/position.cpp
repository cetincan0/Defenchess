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

#include <vector>

#include "position.h"
#include "bitboard.h"
#include "move_utils.h"
#include "move.h"

void init_castling_rights(Position *p) {
    // black_queenside | black_kingside | white_queenside | white_kingside
    for (Square sq = A1; sq <= H8; ++sq) {
        if (sq == p->initial_rooks[black][QUEENSIDE]) {
            // Black queenside
            p->CASTLING_RIGHTS[sq] = 7;
        } else if (sq == p->initial_rooks[black][KINGSIDE]) {
            // Black kingside
            p->CASTLING_RIGHTS[sq] = 11;
        } else if (sq == p->king_index[black]) {
            // Black both
            p->CASTLING_RIGHTS[sq] = 3;
        } else if (sq == p->initial_rooks[white][QUEENSIDE]) {
            // White queenside
            p->CASTLING_RIGHTS[sq] = 13;
        } else if (sq == p->initial_rooks[white][KINGSIDE]) {
            // White kingside
            p->CASTLING_RIGHTS[sq] = 14;
        } else if (sq == p->king_index[white]) {
            // White both
            p->CASTLING_RIGHTS[sq] = 12;
        } else {
            p->CASTLING_RIGHTS[sq] = 15;
        }
    }
}

void calculate_score(Position *p) {
    p->score = Score{0, 0};
    Bitboard board = p->board;

    while (board) {
        Square sq = pop(&board);
        Piece piece = p->pieces[sq];
        p->score += pst[piece][sq];
    }
}

void calculate_hash(Position *p) {
    Bitboard board = p->board;
    Info *info = p->info;

    while(board) {
        Square square = pop(&board);
        Piece piece = p->pieces[square];
        uint64_t hash = hash_combined[piece][square];
        info->hash ^= hash;
        if (is_pawn(piece)) {
            info->pawn_hash ^= hash;
        }
    }

    info->hash ^= castling_hash[info->castling];
    if (info->enpassant != no_sq) {
        info->hash ^= enpassant_hash[file_of(info->enpassant)];
    }
    if (p->color == white) {
        info->hash ^= white_hash;
    }
}

void calculate_material(Position *p) {
    Info *info = p->info;
    int wp = count(p->bbs[pawn(white)]);
    int wn = count(p->bbs[knight(white)]);
    int wb = count(p->bbs[bishop(white)]);
    int wr = count(p->bbs[rook(white)]);
    int wq = count(p->bbs[queen(white)]);

    int bp = count(p->bbs[pawn(black)]);
    int bn = count(p->bbs[knight(black)]);
    int bb = count(p->bbs[bishop(black)]);
    int br = count(p->bbs[rook(black)]);
    int bq = count(p->bbs[queen(black)]);

    int index = wq * material_balance[white_queen]  +
                bq * material_balance[black_queen]  +
                wr * material_balance[white_rook]   +
                br * material_balance[black_rook]   +
                wb * material_balance[white_bishop] +
                bb * material_balance[black_bishop] +
                wn * material_balance[white_knight] +
                bn * material_balance[black_knight] +
                wp * material_balance[white_pawn]   +
                bp * material_balance[black_pawn];

    info->material_index = index;

    info->non_pawn_material[white] = wn * KNIGHT_MID +
                                  wb * BISHOP_MID +
                                  wr * ROOK_MID +
                                  wq * QUEEN_MID;

    info->non_pawn_material[black] = bn * KNIGHT_MID +
                                  bb * BISHOP_MID +
                                  br * ROOK_MID +
                                  bq * QUEEN_MID;
}

Position* add_pieces(Position* p) {
    for (Square sq = A1; sq <= H8; ++sq) {
        if (on(p->board, sq)) {
            for (int j = white_pawn; j <= black_king; ++j) {
                if (on(p->bbs[j], sq)) {
                    p->pieces[sq] = j;
                    break;
                } else {
                    p->pieces[sq] = no_piece;
                }
            }
        } else {
            p->pieces[sq] = no_piece;
        }
    }
    return p;
}

void split_fen(std::string s, std::vector<std::string> &m) {
    unsigned l_index = 0;
    for (unsigned i = 0 ; i < s.length() ; i++) {
        if (s[i] == ' ') {
            m.push_back(s.substr(l_index, i - l_index));
            l_index = i + 1;
        }
        if (i == s.length() - 1) {
            m.push_back(s.substr(l_index));
        }
    }
}

Position* import_fen(std::string fen, int thread_id){
    std::vector<std::string> matches;
    split_fen(fen, matches);
    
    Color color;
    if (matches[1][0] == 'w') {
        color = white;
    } else if (matches[1][0] == 'b') {
        color = black;
    } else {
        assert(false);
        return 0;
    }

    int halfmove_clock;
    if (matches[5] != "") {
        halfmove_clock = stoi(matches[5]);
    } else {
        halfmove_clock = 1;
    }

    SearchThread *t = get_thread(thread_id);
    t->root_ply = 2 * (halfmove_clock - 1) + color;
    Info *info = &t->infos[t->root_ply];
    Position *p = &t->position;
    p->info = info;
    t->search_ply = t->root_ply;
    p->color = color;

    p->bbs[white_occupy] = 0;
    p->bbs[black_occupy] = 0;

    p->bbs[white_knight] = 0;
    p->bbs[white_rook] = 0;
    p->bbs[white_bishop] = 0;
    p->bbs[white_pawn] = 0;
    p->bbs[white_king] = 0;
    p->bbs[white_queen] = 0;

    p->bbs[black_knight] = 0;
    p->bbs[black_rook] = 0;
    p->bbs[black_bishop] = 0;
    p->bbs[black_pawn] = 0;
    p->bbs[black_king] = 0;
    p->bbs[black_queen] = 0;
    
    p->board = 0;

    for (Square sq = A1; sq <= H8; ++sq){
        p->pieces[sq] = no_piece;
    }

    int sq = A8;
    for(char& c : matches[0]) {
        Piece piece = no_piece;
        switch(c) {
        case 'P':
            piece = white_pawn;
            break;
        case 'p':
            piece = black_pawn;
            break;
        case 'N':
            piece = white_knight;
            break;
        case 'n':
            piece = black_knight;
            break;
        case 'B':
            piece = white_bishop;
            break;
        case 'b':
            piece = black_bishop;
            break;
        case 'R':
            piece = white_rook;
            break;
        case 'r':
            piece = black_rook;
            break;
        case 'Q':
            piece = white_queen;
            break;
        case 'q':
            piece = black_queen;
            break;
        case 'K':
            piece = white_king;
            p->king_index[white] = (Square) sq;
            break;
        case 'k':
            piece = black_king;
            p->king_index[black] = (Square) sq;
            break;
        case '/':
            sq -= 16;
            break;
        case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8':
            sq += c - '0';
        }
        if (piece != no_piece) {
            p->pieces[sq] = piece;
            p->bbs[piece] |= bfi[sq];
            p->bbs[piece_color(piece)] |= bfi[sq++];
        }
    }

    info->castling = 0;

    p->initial_rooks[white][QUEENSIDE] = A1;
    p->initial_rooks[white][KINGSIDE] = H1;
    p->initial_rooks[black][QUEENSIDE] = A8;
    p->initial_rooks[black][KINGSIDE] = H8;

    for(char& c : matches[2]) {
        if (c == 'K') {
            info->castling |= can_king_castle_mask[white];
        } else if (c == 'Q') {
            info->castling |= can_queen_castle_mask[white];
        } else if (c == 'k') {
            info->castling |= can_king_castle_mask[black];
        } else if (c == 'q') {
            info->castling |= can_queen_castle_mask[black];
        } else if (c != '-') {
            // Castling rights for Chess960
            Color castling_color = c >= 'a' ? black : white;
            Square rook_sq = (Square) (c - (castling_color == white ? 'A' : 'a'));
            rook_sq = relative_square(rook_sq, castling_color);
            if (p->pieces[rook_sq] != rook(castling_color)) {
                continue;
            }

            if (rook_sq < p->king_index[castling_color]) {
                info->castling |= can_queen_castle_mask[castling_color];
                p->initial_rooks[castling_color][QUEENSIDE] = rook_sq;
            } else {
                info->castling |= can_king_castle_mask[castling_color];
                p->initial_rooks[castling_color][KINGSIDE] = rook_sq;
            }
        }
    }
    init_castling_rights(p);

    if (matches[3][0] != '-') {
        info->enpassant = (Square)(((matches[3][1] - '1') << 3) + matches[3][0] - 'a');
    } else {
        info->enpassant = no_sq;
    }

    if (matches[4] != "") {
        info->last_irreversible = stoi(matches[4]);
    } else {
        info->last_irreversible = 0;
    }
    p->board = p->bbs[white] | p->bbs[black];
    info->hash = info->pawn_hash = 0;
    info->pinned[white] = pinned_piece_squares(p, white);
    info->pinned[black] = pinned_piece_squares(p, black);
    info->previous = nullptr;
    calculate_score(p);
    calculate_hash(p);
    calculate_material(p);
    p->my_thread = t;
    return p;
}

Position* start_pos(){
    Info *info = &main_thread.infos[0];
    Position *p = &main_thread.position;
    p->info = info;

    Bitboard white_occupied_bb = 0x000000000000FFFF;
    Bitboard black_occupied_bb = 0xFFFF000000000000;

    Bitboard white_rooks_bb = 0x0000000000000081;
    Bitboard white_knights_bb = 0x0000000000000042;
    Bitboard white_bishops_bb = 0x0000000000000024;
    Bitboard white_king_bb = 0x0000000000000010;
    Bitboard white_queen_bb = 0x0000000000000008;
    Bitboard white_pawns_bb = 0x000000000000FF00;

    Bitboard black_rooks_bb = 0x8100000000000000;
    Bitboard black_knights_bb = 0x4200000000000000;
    Bitboard black_bishops_bb = 0x2400000000000000;
    Bitboard black_king_bb = 0x1000000000000000;
    Bitboard black_queen_bb = 0x0800000000000000;
    Bitboard black_pawns_bb = 0x00FF000000000000;

    p->bbs[white_occupy] = white_occupied_bb;
    p->bbs[black_occupy] = black_occupied_bb;

    p->bbs[white_knight] = white_knights_bb;
    p->bbs[white_rook] = white_rooks_bb;
    p->bbs[white_bishop] = white_bishops_bb;
    p->bbs[white_pawn] = white_pawns_bb;
    p->bbs[white_king] = white_king_bb;
    p->bbs[white_queen] = white_queen_bb;

    p->bbs[black_knight] = black_knights_bb;
    p->bbs[black_rook] = black_rooks_bb;
    p->bbs[black_bishop] = black_bishops_bb;
    p->bbs[black_pawn] = black_pawns_bb;
    p->bbs[black_king] = black_king_bb;
    p->bbs[black_queen] = black_queen_bb;

    p->initial_rooks[white][QUEENSIDE] = A1;
    p->initial_rooks[white][KINGSIDE] = H1;

    p->initial_rooks[black][QUEENSIDE] = A8;
    p->initial_rooks[black][KINGSIDE] = H8;

    p->king_index[white] = E1;
    p->king_index[black] = E8;
    init_castling_rights(p);

    p->board = black_occupied_bb | white_occupied_bb;
    add_pieces(p);
    p->color = white;
    info->castling = 15;
    info->enpassant = no_sq;
    info->hash = info->pawn_hash = 0;
    main_thread.search_ply = main_thread.root_ply = 0;
    info->pinned[white] = pinned_piece_squares(p, white);
    info->pinned[black] = pinned_piece_squares(p, black);
    info->previous = nullptr;
    calculate_score(p);
    calculate_hash(p);
    calculate_material(p);
    p->my_thread = &main_thread;
    info->last_irreversible = 0;
    return p;
}
