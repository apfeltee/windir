
CXX = clang++
CXX = g++
CFLAGS = -g3 -ggdb
LFLAGS = -luser32 -lshlwapi

all:
	$(CXX) -Wall -Wextra main.cpp -O3 -o windir.exe $(LFLAGS)
