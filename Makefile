build:
	g++ -o TETRIS main.cpp `pkg-config --cflags --libs sdl2`
