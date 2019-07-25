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

#ifndef SEARCH_H
#define SEARCH_H

#include <string>
#include <vector>

#include "data.h"
#include "timecontrol.h"

const int razoring_margin = 350;
const int futility_move_counts[2][8] = {
    {2, 3, 4,  7, 11, 15, 20, 26}, // not improving
    {5, 6, 9, 14, 21, 30, 41, 54}, // improving
};

inline int lmr(bool is_pv, int depth, int num_moves) {
    assert(depth >= 1 && num_moves >= 1);
    return reductions[is_pv][std::min(depth, 63)][std::min(num_moves, 63)];
}

extern uint8_t timer_count;
extern int myremain;
extern int total_remaining;
extern volatile bool is_timeout;
extern int think_depth_limit;
extern bool quit_application;

extern struct timeval curr_time, start_ts;

inline int time_passed() {
    return (((curr_time.tv_sec - start_ts.tv_sec) * 1000000) + (curr_time.tv_usec - start_ts.tv_usec)) / 1000;
}

inline int bench_time(struct timeval s, struct timeval e) {
    return (((e.tv_sec - s.tv_sec) * 1000000) + (e.tv_usec - s.tv_usec)) / 1000;
}


inline void init_time(Position *p, std::vector<std::string> word_list) {
    timer_count = 0;
    is_timeout = false;

    if (word_list.size() <= 1) {
        myremain = 10000;
        total_remaining = 10000;
        return;
    }

    think_depth_limit = MAX_PLY;

    if (word_list[1] == "movetime") {
        myremain = stoi(word_list[2]) * 99 / 100;
        total_remaining = myremain;
    }
    else if (word_list[1] == "infinite") {
        myremain = 3600000;
    }
    else if (word_list[1] == "depth") {
        myremain = 3600000;
        think_depth_limit = stoi(word_list[2]);
    }
    else if (word_list.size() > 1) {
        int moves_to_go = 0;
        int black_remaining = 0;
        int white_remaining = 0;
        int black_increment = 0;
        int white_increment = 0;

        for (unsigned i = 1 ; i < word_list.size() ; i += 2) {
            if (word_list[i] == "wtime")
                white_remaining = stoi(word_list[i + 1]);
            if (word_list[i] == "btime")
                black_remaining = stoi(word_list[i + 1]);
            if (word_list[i] == "winc")
                white_increment = stoi(word_list[i + 1]);
            if (word_list[i] == "binc")
                black_increment = stoi(word_list[i + 1]);
            if (word_list[i] == "movestogo")
                moves_to_go = stoi(word_list[i + 1]);
        }

        if (white_remaining > 0 || black_remaining > 0) {
            TTime t = get_myremain(
                p->color == white ? white_increment : black_increment,
                p->color == white ? white_remaining : black_remaining,
                moves_to_go
            );
            myremain = t.optimum_time;
            total_remaining = t.maximum_time;
        }
    }
}

int alpha_beta_quiescence(Position *p, Metadata *md, int alpha, int beta, int depth, bool in_check);
void think(Position *p, std::vector<std::string> word_list);
void print_pv();
void bench();

// Positions taken from Ethereal
const std::string benchmarks[36] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10",
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11",
    "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
    "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
    "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
    "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
    "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
    "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
    "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
    "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
    "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
    "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
    "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26",
    "6k1/6p1/6Pp/ppp5/3pn2P/1P3K2/1PP2P2/3N4 b - - 0 1",
    "3b4/5kp1/1p1p1p1p/pP1PpP1P/P1P1P3/3KN3/8/8 w - - 0 1",
    "8/6pk/1p6/8/PP3p1p/5P2/4KP1q/3Q4 w - - 0 1",
    "7k/3p2pp/4q3/8/4Q3/5Kp1/P6b/8 w - - 0 1",
    "8/2p5/8/2kPKp1p/2p4P/2P5/3P4/8 w - - 0 1",
    "8/1p3pp1/7p/5P1P/2k3P1/8/2K2P2/8 w - - 0 1",
    "8/pp2r1k1/2p1p3/3pP2p/1P1P1P1P/P5KR/8/8 w - - 0 1",
    "8/3p4/p1bk3p/Pp6/1Kp1PpPp/2P2P1P/2P5/5B2 b - - 0 1",
    "5k2/7R/4P2p/5K2/p1r2P1p/8/8/8 b - - 0 1",
    "6k1/6p1/P6p/r1N5/5p2/7P/1b3PP1/4R1K1 w - - 0 1",
    "1r3k2/4q3/2Pp3b/3Bp3/2Q2p2/1p1P2P1/1P2KP2/3N4 w - - 0 1",
    "6k1/4pp1p/3p2p1/P1pPb3/R7/1r2P1PP/3B1P2/6K1 w - - 0 1",
    "8/3p3B/5p2/5P2/p7/PP5b/k7/6K1 w - - 0 1",
    "8/8/8/8/5kp1/P7/8/1K1N4 w - - 0 1",
    "8/8/8/5N2/8/p7/8/2NK3k w - - 0 1",
    "8/3k4/8/8/8/4B3/4KB2/2B5 w - - 0 1",
    "8/8/1P6/5pr1/8/4R3/7k/2K5 w - - 0 1",
    "8/2p4P/8/kr6/6R1/8/8/1K6 w - - 0 1",
    "8/8/3P3k/8/1p6/8/1P6/1K3n2 b - - 0 1",
    "8/R7/2q5/8/6k1/8/1P5p/K6R w - - 0 124",
    "6k1/3b3r/1p1p4/p1n2p2/1PPNpP1q/P3Q1p1/1R1RB1P1/5K2 b - - 0 1",
    "r2r1n2/pp2bk2/2p1p2p/3q4/3PN1QP/2P3R1/P4PP1/5RK1 w - - 0 1",
};

#endif
