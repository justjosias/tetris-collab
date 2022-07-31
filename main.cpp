#include <iostream>
#include <string>
#include <vector>

#include <SDL.h>

using namespace std;

class Game
{
      public:
	int height;
	int width;

	Game()
	{
		this->height = 20;
		this->width = 10;
	}
};

int main()
{
	Game game;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		cerr << "Failed to initialize the SDL2 library\n";
		return 1;
	}

	SDL_Window *window =
	    SDL_CreateWindow("TETRIS", SDL_WINDOWPOS_CENTERED,
			     SDL_WINDOWPOS_CENTERED, 680, 480, 0);

	if (!window) {
		cerr << "Failed to create window\n";
		return 1;
	}

	SDL_Surface *window_surface = SDL_GetWindowSurface(window);

	if (!window_surface) {
		cerr << "Failed to get the surface from the window\n";
		return 1;
	}

	SDL_UpdateWindowSurface(window);

	SDL_Event event;
	while (event.type != SDL_QUIT) {
		SDL_PollEvent(&event);
	}
}
