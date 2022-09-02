NAME=TETRIS
CPP=g++
CPPFLAGS=-Wall -Wextra -g
LDFLAGS=`pkg-config --cflags --libs sdl2 SDL2_ttf`

CPPFILES=main.cpp

build:
	$(CPP) $(CPPFLAGS) -o $(NAME) $(CPPFILES) $(LDFLAGS)
