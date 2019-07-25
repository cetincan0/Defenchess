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
#include <map>
#include <sys/time.h>
#include <thread>
#include <vector>

#include "data.h"
#include "eval.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "see.h"
#include "tb.h"
#include "test.h"
#include "thread.h"
#include "tt.h"
#include "tune.h"
#include "uci.h"

using namespace std;

vector<string> word_list;
Position *root_position;

vector<string> split_words(string s) {
    vector <string> tmp;
    unsigned l_index = 0;
    for (unsigned i = 0 ; i < s.length() ; i++) {
        if (s[i] == ' ') {
            tmp.push_back(s.substr(l_index, i - l_index));
            l_index = i + 1;
        }
        if (i == s.length() - 1) {
            tmp.push_back(s.substr(l_index));
        }
    }

    return tmp;
}

Move uci2move(Position *p, string s) {
    Square from = (Square) (n2i(s[0]) + 8 * (s[1] - '0') - 8);
    Square to = (Square) (n2i(s[2]) + 8 * (s[3] - '0') - 8);
    Piece piece = p->pieces[from];
    uint8_t type = NORMAL;
    if (is_pawn(piece)) {
        if (rank_of(to) == 0 || rank_of(to) == 7) {
            type = PROMOTION;
        } else if (to == p->info->enpassant) {
            type = ENPASSANT;
        }
    } else if (is_king(piece) && std::abs(from - to) == 2) {
        type = CASTLING;
    }
    return _movecast(from, to, type);
}

bool word_equal(int index, string comparison_str) {
    if (word_list.size() > (unsigned) index)
        return word_list[index] == comparison_str;
    return false;
}

void uci() {
    cout << "id name Defenchess 2.1 x64" << endl << "id author Can Cetin & Dogac Eldenk" << endl;
#ifndef NDEBUG
    cout << "debug mode on" << std::endl;
#endif
    cout << "option name Hash type spin default 16 min 1 max 16384" << endl;
    cout << "option name Threads type spin default 1 min 1 max " << MAX_THREADS << endl;
    cout << "option name SyzygyPath type string default <empty>" << endl;
    cout << "option name MoveOverhead type spin default 100 min 0 max 5000" << endl;
    cout << "uciok" << endl;
}

void perft() {
    if (word_equal(1, "test"))
        TEST_H::perft_test();
    else {
        uint64_t nodes = Perft(stoi(word_list[1]), root_position, true, is_checked(root_position));
        std::cout << nodes << std::endl;
    }
}

void debug() {
    cout << bitstring(root_position->board);
    Metadata *md = &search_threads[0].metadatas[2];
    MoveGen movegen = new_movegen(root_position, md, no_move, NORMAL_SEARCH, 0, is_checked(root_position));
    Move move;
    while ((move = next_move(&movegen, md, 0)) != no_move) {
        cout << move_to_str(move) << " ";
    }
    cout << endl;
}

void quit() {
    if (!is_searching) {
        exit(EXIT_SUCCESS);
    }

    is_timeout = true;
    quit_application = true;
}

void stop() {
    is_timeout = true;
}

void isready() {
    cout << "readyok" << endl;
}

void go() {
    std::thread think_thread(think, root_position, word_list);
    think_thread.detach();
}

void startpos() {
    root_position = start_pos();

    if (word_equal(2, "moves")) {
        for (unsigned i = 3 ; i < word_list.size() ; i++) {
            Move m = no_move;
            if (word_list[i].length() == 4) {
                m = uci2move(root_position, word_list[i]);
            } else if (word_list[i].length() == 5) {
                if (word_list[i][4] == 'n') {
                    m = _promoten(uci2move(root_position, word_list[i]));
                } else if (word_list[i][4] == 'r') {
                    m = _promoter(uci2move(root_position, word_list[i]));
                } else if (word_list[i][4] == 'b') {
                    m = _promoteb(uci2move(root_position, word_list[i]));
                } else {
                    m = _promoteq(uci2move(root_position, word_list[i]));
                }
            }
            if (m != no_move && is_pseudolegal(root_position, m)) {
                make_move(root_position, m);
            }
        }
    }
}

void cmd_fen() {
    string fen_str = word_list[2] + " " + word_list[3] + " " + word_list[4] + " " + word_list[5] + " " + word_list[6] + " " + word_list[7];

    root_position = import_fen(fen_str, 0);

    if (word_equal(8, "moves")) {
        for (unsigned i = 9 ; i < word_list.size() ; i++) {
            Move m = no_move;
            if (word_list[i].length() == 4) {
                m = uci2move(root_position, word_list[i]);
            } else if (word_list[i].length() == 5) {
                if (word_list[i][4] == 'n') {
                    m = _promoten(uci2move(root_position, word_list[i]));
                } else if (word_list[i][4] == 'r') {
                    m = _promoter(uci2move(root_position, word_list[i]));
                } else if (word_list[i][4] == 'b') {
                    m = _promoteb(uci2move(root_position, word_list[i]));
                } else {
                    m = _promoteq(uci2move(root_position, word_list[i]));
                }
            }
            if (m != no_move && is_pseudolegal(root_position, m)) {
                make_move(root_position, m);
            }
        }
    }
}

void see() {
    if (word_list[1] == "test") {
        see_test();
    } else {
        Move move = uci2move(root_position, word_list[1]);
        cout << see_capture(root_position, move, 0) << endl;
    }
}

void undo() {
    Move move = uci2move(root_position, word_list[1]);
    undo_test(root_position, move);
}

void cmd_position() {
    if (word_list[1] == "fen") 
        cmd_fen();
    if (word_list[1] == "startpos")
        startpos();
    get_ready();
}

void setoption() {
    if (word_list[1] != "name" || word_list[3] != "value") {
        return;
    }
    string name = word_list[2];
    string value = word_list[4];

    if (name == "Hash") {
        int mb = stoi(value);
        if (!mb || more_than_one(uint64_t(mb))) {
            cout << "info Hash value needs to be a power of 2!" << endl;
        }
        reset_tt(mb);
    } else if (name == "Threads") {
        num_threads = std::min(MAX_THREADS, stoi(value));
        reset_threads();
    } else if (name == "SyzygyPath") {
        init_syzygy(value);
    } else if (name == "MoveOverhead") {
        move_overhead = stoi(value);
    }
}

void ucinewgame() {
    clear_threads();
    clear_tt();
}

void eval() {
    cout << evaluate(root_position) << endl;
}

void run_command(string s) {
    if (s == "ucinewgame")
        ucinewgame();
    if (s == "position")
        cmd_position();
    if (s == "go")
        go();
    if (s == "setoption")
        setoption();
    if (s == "isready")
        isready();
    if (s == "uci")
        uci();
    if (s == "perft")
        perft();
    if (s == "debug")
        debug();
    if (s == "quit" || s == "exit")
        quit();
    if (s == "stop")
        stop();
    if (s == "see")
        see();
    if (s == "bench")
        bench();
    if (s == "undo")
        undo();
    if (s == "eval")
        eval();
#ifdef __TUNE__
    if (s == "tune")
        tune();
#endif
}

void loop() {
    cout << "Defenchess 2.1 x64 by Can Cetin and Dogac Eldenk" << endl;

    string in_str;
#ifdef __TUNE__
    tune();
#endif
    root_position = start_pos();

    while (true) {
        getline(cin, in_str);
        word_list = split_words(in_str);
        if (word_list.size() > 0) {
            run_command(word_list[0]);
        }
    }

}

