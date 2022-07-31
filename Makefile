build:
	g++ -o TETRIS `pkg-config --cflags --libs sdl2` main.cpp
