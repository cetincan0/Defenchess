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

#include "data.h"
#include "uci.h"
#include "search.h"
#include "thread.h"

int main(int argc, char* argv[]) {
    init();
    get_ready();
    if (argc > 1 && strcmp(argv[1], "bench") == 0) {
        // Calling bench exits the program
        bench();
        return 0;
    }
    loop();
    return 0;
}
