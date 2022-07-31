build:
	g++ -Wall -Wextra -o TETRIS main.cpp `pkg-config --cflags --libs sdl2`
