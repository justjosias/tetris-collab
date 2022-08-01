NAME=TETRIS
CPP=g++
CPPFLAGS=-Wall -Wextra
LDFLAGS=`pkg-config --cflags --libs sdl2`

CPPFILES=main.cpp

build:
	$(CPP) $(CPPFLAGS) -o $(NAME) $(CPPFILES) $(LDFLAGS)
