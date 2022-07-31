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

	SDL_Window *window;
	SDL_Surface *window_surface;

	Game()
	{
		this->height = 20;
		this->width = 10;

		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			throw "Failed to initialize SDL2";
		}

		this->window =
		    SDL_CreateWindow("TETRIS", SDL_WINDOWPOS_CENTERED,
				     SDL_WINDOWPOS_CENTERED, 400, 800, 0);
		if (window == nullptr) {
			throw "Failed to create window";
		}
		this->window_surface = SDL_GetWindowSurface(window);

		if (window_surface == nullptr) {
			throw "Failed to get window surface";
		}
	}

	void decorate_window() {
                
        }

        ~Game() {
                SDL_DestroyWindow(window);
                SDL_Quit();
        }
};

int main(int argc, char **argv)
{
	Game game;

	SDL_UpdateWindowSurface(game.window);

	SDL_Event event;
        SDL_PollEvent(&event);
	while (event.type != SDL_QUIT) {
		SDL_PollEvent(&event);
	}

        return 0;
}
