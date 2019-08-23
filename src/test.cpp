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
#include "move_utils.h"
#include "movegen.h"
#include "magic.h"
#include "position.h"
#include "search.h"
#include "see.h"
#include "target.h"
#include "test.h"

std::string fen[7] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1"
};

typedef struct SeeResult {
    std::string fen;
    Move move;
    bool result;
} SeeResult;

SeeResult see_results[] = {
    { "5k2/8/8/8/4p3/5P2/8/5K2 w - - 0 1",       _movecast(F3, E4, NORMAL), true },
    { "5k2/8/8/3p4/4p3/5P2/8/5K2 w - - 0 1",     _movecast(F3, E4, NORMAL), true },
    { "5k2/8/8/3b4/4p3/5P2/8/5K2 w - - 0 1",     _movecast(F3, E4, NORMAL), true },
    { "5k2/8/8/8/4p3/5B2/8/5K2 w - - 0 1",       _movecast(F3, E4, NORMAL), true },
    { "5k2/8/8/5p2/4p3/5B2/8/5K2 w - - 0 1",     _movecast(F3, E4, NORMAL), false },
    { "5k2/8/8/3b4/4p3/5B2/8/5K2 w - - 0 1",     _movecast(F3, E4, NORMAL), false },
    { "5k2/8/8/3b4/4p3/3P1B2/8/5K2 w - - 0 1",   _movecast(F3, E4, NORMAL), true },
    { "5k2/8/8/3b1b2/4p3/3B1B2/8/5K2 w - - 0 1", _movecast(F3, E4, NORMAL), false },
    { "5k2/8/8/3q4/4p3/5B2/8/5K2 w - - 0 1",     _movecast(F3, E4, NORMAL), false },
    { "5k2/8/8/3q4/4p3/5B2/5N2/5K2 w - - 0 1",   _movecast(F3, E4, NORMAL), true },
    { "5k2/8/8/3q4/4p3/5B2/8/4RK2 w - - 0 1",    _movecast(F3, E4, NORMAL), true },
    { "5k2/8/8/3q4/4p3/5B2/8/5K1Q w - - 0 1",    _movecast(F3, E4, NORMAL), true },
    { "5k2/1b6/8/3q4/4p3/5B2/8/5K1Q w - - 0 1",  _movecast(F3, E4, NORMAL), false },
    { "5k2/8/8/8/4q3/5P2/8/5K2 w - - 0 1",       _movecast(F3, E4, NORMAL), true },
    { "5k2/4n3/8/3n4/2B5/8/8/5K2 w - - 0 1",     _movecast(C4, D5, NORMAL), false },
    { "5k2/4n3/8/3b4/8/2N5/8/5K2 w - - 0 1",     _movecast(C3, D5, NORMAL), true },
    { "5k2/8/8/8/3qp1R1/8/8/5K2 w - - 0 1",      _movecast(G4, E4, NORMAL), false },
    { "5k2/8/8/8/3qp1RR/8/8/5K2 w - - 0 1",      _movecast(G4, E4, NORMAL), true },
    { "5k2/8/8/8/1q1rp1RR/8/8/5K2 w - - 0 1",    _movecast(G4, E4, NORMAL), false },
    { "r1bqkbnr/pppp1ppp/n7/4p3/PP6/B7/2PPPPPP/RN1QKBNR b KQkq - 0 1", _movecast(A6, B4, NORMAL), true }
};

void see_test() {
    for (int i = 0; i < 20; ++i) {
        SeeResult see_result = see_results[i];
        Position *p = import_fen(see_result.fen, 0);
        std::cout << "Testing position " << i << std::endl;
        if (see_capture(p, see_result.move, 0) != see_result.result) {
            assert(false);
        }
    }
    std::cout << "Success!" << std::endl;
}

void perft_test(){
    // Positions taken from https://chessprogramming.wikispaces.com/Perft+Results
    printf("\nStarted testing !\n\n");

    init();
    init_magic();

    int total_result = 597452135;

    int results[] = {119060324, 193690690, 11030083, 15833292, 89941194, 164075551, 3821001};
    int depths[] = {6, 5, 6, 5, 5, 5, 6};
    int results_perft[] = {0,0,0,0,0,0,0};

    int nodes_total = 0;

    struct timeval tv1, tv2, tv3, tv4;
    gettimeofday(&tv3, NULL);

    for (int i = 0; i < 7; i++){
        Position *p = import_fen(fen[i].c_str(), 0);

        int t_depth = depths[i];

        std::cout << "perft " << t_depth << std::endl;
        
        gettimeofday(&tv1, NULL);
        
        results_perft[i] = Perft(t_depth, p, true, is_checked(p));
        nodes_total += results_perft[i];
        
        gettimeofday(&tv2, NULL);
        std::cout << "===============\n";
        uint64_t nodesps = results_perft[i] / ((double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
        (double) (tv2.tv_sec - tv1.tv_sec));
        std::cout << "Nodes searched :  " << results_perft[i] << "\nNodes per second   :  " << nodesps;

        printf("\nTime taken in execution = %f seconds\n\n",
        (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 +
        (double) (tv2.tv_sec - tv1.tv_sec));
    }

    gettimeofday(&tv4, NULL);
    std::cout << "\n\n\n=@==================@=\n";
    for (int i = 0; i < 7; i++){
        if (results_perft[i] > results[i]){
            std::cout << "!!! In position " << i+1 << " we generated " << results_perft[i] - results[i] << " more nodes !" << std::endl;
        }else if (results_perft[i] < results[i]){
            std::cout << "!!! In position " << i+1 << " we generated " << results[i] - results_perft[i] << " fewer nodes !" << std::endl;
        }else{
            std::cout << "@In position " << i+1 << " we generated " << results_perft[i] << " nodes perfectly !" << std::endl;
        }
    }

    if (nodes_total > total_result){
        std::cout << "!!! we generated: " << nodes_total - total_result << " more nodes " << std::endl;
    } else if (nodes_total < total_result){
        std::cout << "!!! we generated: " << total_result - nodes_total << " fewer nodes !" << std::endl;
    }else {
        std::cout << "@we generated: " << nodes_total << " nodes perfectly !" << std::endl;
    }


    std::cout << "\n----------------------------\n";
        uint64_t nodesps = nodes_total / ((double) (tv4.tv_usec - tv3.tv_usec) / 1000000 +
        (double) (tv4.tv_sec - tv3.tv_sec));
        std::cout << "Total nodes searched :  " << nodes_total << "\nTotal nodes per second   :  " << nodesps;

        printf("\nTotal execution time = %f seconds\n",
        (double) (tv4.tv_usec - tv3.tv_usec) / 1000000 +
        (double) (tv4.tv_sec - tv3.tv_sec));

}

void undo_test(Position *p, Move move) {
    Position tmp;
    Info info;
    std::memcpy(&tmp, p, sizeof(Position));
    std::memcpy(&info, p->info, sizeof(Info));

    make_move(p, move);
    undo_move(p, move);
    for (Square sq = A1; sq <= H8; ++sq) {
        assert(p->pieces[sq] == tmp.pieces[sq]);
    }
    for (int i = white_occupy; i <= black_king; ++i) {
        assert(p->bbs[i] == tmp.bbs[i]);
    }
    assert(p->score.midgame == tmp.score.midgame);
    assert(p->score.endgame == tmp.score.endgame);
    assert(p->king_index[white] == tmp.king_index[white]);
    assert(p->king_index[black] == tmp.king_index[black]);
    assert(p->board == tmp.board);
    assert(p->color == tmp.color);

    assert(p->info->pawn_hash == info.pawn_hash);
    assert(p->info->last_irreversible == info.last_irreversible);
    assert(p->info->castling == info.castling);
    assert(p->info->material_index == info.material_index);
    assert(p->info->non_pawn_material[white] == info.non_pawn_material[white]);
    assert(p->info->non_pawn_material[black] == info.non_pawn_material[black]);
    assert(p->info->enpassant == info.enpassant);
    assert(p->info->hash == info.hash);
    assert(p->info->pinned[white] == info.pinned[white]);
    assert(p->info->pinned[black] == info.pinned[black]);
    assert(p->info->captured == info.captured);
}

uint64_t fastPerft(int depth, Position *p, bool root, bool in_check) {
    if (depth == 0) {
        return 1ULL;
    }
    uint64_t move_nodes = 0, nodes = 0;
    const bool is_leaf = depth == 2;

    Metadata *md = &p->my_thread->metadatas[2];
    MoveGen movegen = new_movegen(p, md, no_move, NORMAL_SEARCH, 0, in_check);
    Move move;
    while ((move = next_move(&movegen, md, 0)) != no_move) {
        if (root && depth == 1) {
            move_nodes = 1;
            ++nodes;
        } else {
            undo_test(p, move);
            bool checks = gives_check(p, move);
            make_move(p, move);
            if (is_leaf) {
                MoveGen movegen_leaf = new_movegen(p, md, no_move, NORMAL_SEARCH, 0, checks);
                if (checks) {
                    generate_evasions(&movegen_leaf, p);
                } else {
                    generate_moves<ALL>(&movegen_leaf, p);
                }
                move_nodes = movegen_leaf.tail;
            } else {
                move_nodes = Perft(depth - 1, p, false, checks);
            }
            nodes += move_nodes;
            undo_move(p, move);
        }
        if (root) {
            std::cout << move_to_str(move) << ": " << move_nodes << std::endl;
        }
    }

    return nodes;
}

void pseudolegal_test(Position *p) {
    // Generate all moves and test pseudo legal
    for (Square a = A1; a <= H8; ++a) {
        for (Square b = A1; b <= H8; ++b) {
            Move gen_move = _movecast(a, b, NORMAL);
            if (is_pseudolegal(p, gen_move)) {
                MoveGen movegen = new_movegen(p, nullptr, no_move, NORMAL_SEARCH, 0, is_checked(p));
                generate_moves<ALL>(&movegen, p);
                bool found = false;
                for (uint8_t move_idx = movegen.head; move_idx < movegen.tail; ++move_idx) {
                    if (movegen.moves[move_idx].move == gen_move) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cout << move_to_str(gen_move) << std::endl;
                    show_position_png(p);
                }
                assert(found);
            }
        }
    }
}

uint64_t Perft(int depth, Position *p, bool root, bool in_check) {
    return fastPerft(depth, p, root, in_check);
    // pseudolegal_test(p);
    if (depth == 0) {
        return 1ULL;
    }
    uint64_t move_nodes = 0, nodes = 0;

    Metadata *md = &p->my_thread->metadatas[2];
    MoveGen movegen = new_movegen(p, md, no_move, NORMAL_SEARCH, 0, in_check);
    Move move;
    while ((move = next_move(&movegen, md, 0)) != no_move) {
        if (!is_legal(p, move)) {
            continue;
        }
        bool checks = gives_check(p, move);
        make_move(p, move);
        move_nodes = Perft(depth - 1, p, false, checks);
        nodes += move_nodes;
        undo_move(p, move);
        if (root) {
            std::cout << move_to_str(move) << ": " << move_nodes << std::endl;
        }
    }

    return nodes;
}
