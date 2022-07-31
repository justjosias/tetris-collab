build:
	#g++ -o TETRIS `pkg-config --cflags --libs sdl2` main.cpp
	g++ -o TETRIS main.cpp `pkg-config --cflags --libs sdl2`
