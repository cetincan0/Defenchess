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

#include "move_utils.h"

using namespace std;

void show_position_png(Position *p){
    FILE* pFile = fopen ("scripts/.tmpboard.txt", "w");
    for (Piece t : p->pieces){
        fprintf(pFile, "%d\n", unsigned(t));
    }
    fclose(pFile);

    // This file is not included in the repository
    exit(system("python3 scripts/imggen.py"));
}

string move_to_str(Position *p, Move m) {
    if (m == null_move) {
        return "null";
    }

    Square from = move_from(m);
    Square to = move_to(m);

    if (chess960 && move_type(m) == CASTLING) {
        Color color = rank_of(to) == RANK_1 ? white : black;
        to = p->initial_rooks[color][CASTLE_TYPE[to]];
    }

    string move_str = int_to_notation(from) + to_string(rank_of(from) + 1) +
                      int_to_notation(to) + to_string(rank_of(to) + 1);

    if (move_type(m) == PROMOTION) {
        move_str += piece_chars[get_promotion_piece(m, black)];
    }

    return move_str;
}

int n2i(char c) {
    switch(c) {
        case 'a':
            return 0;
        case 'b':
            return 1;
        case 'c':
            return 2;
        case 'd':
            return 3;
        case 'e':
            return 4;
        case 'f':
            return 5;
        case 'g':
            return 6;
        case 'h':
            return 7;
        default:
            return -1;
    }
}

string i2n(int i) {
    switch(i) {
        case 0:
            return "a";
        case 1:
            return "b";
        case 2:
            return "c";
        case 3:
            return "d";
        case 4:
            return "e";
        case 5:
            return "f";
        case 6:
            return "g";
        case 7:
            return "h";
        default:
            return "0";
    }
}

string int_to_notation(int a) {
    return i2n(a % 8);
}
