# Defenchess, a chess engine
# Copyright 2017-2019 Can Cetin, Dogac Eldenk
# 
# Defenchess is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either VERSION 3 of the License, or
# (at your option) any later VERSION.
# 
# Defenchess is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Defenchess.  If not, see <http://www.gnu.org/licenses/>.

CC      = g++
CFLAGS  = -DNDEBUG -Wall -Wcast-qual -Wextra -Wshadow -pedantic -std=c++11 -m64 -msse3 -mpopcnt -flto
WFLAGS  = -DNDEBUG -Wall -Wcast-qual -Wextra -Wshadow -pedantic -std=c++11 -m64 -msse3 -mpopcnt
OLDWFLAGS  = -DNDEBUG -Wall -Wcast-qual -Wextra -Wshadow -pedantic -std=c++11 -m64
DFLAGS  = -g -Wall -Wcast-qual -Wextra -Wshadow -pedantic -std=c++11 -m64 -msse3 -mpopcnt -flto -D__DEBUG__
TARGET  = Defenchess
OPT     = -O3
VERSION = 2.2
OBJECTS = bitboard.o data.o eval.o move.o move_utils.o params.o position.o pst.o search.o see.o target.o test.o timecontrol.o thread.o tt.o tune.o uci.o magic.o main.o movegen.o tb.o fathom/tbprobe.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OPT) $^ -o $@ -pthread

%.o: %.cpp
	$(CC) $(CFLAGS) $(OPT) -c $^ -o $@ -pthread

winrelease:
	$(CC) $(WFLAGS) -static $(OPT) *.cpp fathom/tbprobe.cpp -o $(TARGET)_$(VERSION)_popcnt.exe

oldwinrelease:
	$(CC) $(OLDWFLAGS) -static $(OPT) *.cpp fathom/tbprobe.cpp -o $(TARGET)_$(VERSION)_nopopcnt.exe

release: $(OBJECTS)
	$(CC) $(CFLAGS) $(OPT) $^ -o $(TARGET)_$(VERSION) -pthread

tune:
	$(CC) $(CFLAGS) $(OPT) -D__TUNE__ *.cpp fathom/tbprobe.cpp -o $(TARGET)_tune -pthread

feature: $(OBJECTS)
	$(CC) $(CFLAGS) $(OPT) $^ -o $(feature) -pthread

perft:
	$(CC) $(CFLAGS) $(OPT) -D__PERFT__ *.cpp fathom/tbprobe.cpp -o $(TARGET)_perft -pthread

debug:
	$(CC) $(DFLAGS) *.cpp fathom/tbprobe.cpp -o $(TARGET)_debug -pthread

fsanitize: $(OBJECTS)
	$(CC) $(CFLAGS) -fsanitize=undefined -fuse-ld=gold $^ -o $(TARGET)_debug -pthread

clean:
	rm -f $(TARGET) *.o fathom/*.o
