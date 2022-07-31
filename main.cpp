#include <iostream>
#include <string>
#include <vector>

#include <SDL.h>

class Game
{
      public:
	std::vector<std::string> blocks;

        int height;
        int width;

	Game() {
                this->height = 20;
                this->width = 10;
                this->blocks.push_back("l-shaped");
        }

	void list_blocks()
	{
		for (std::string block : this->blocks) {
			std::cout << block << std::endl;
		}
	}
};

int main()
{
	Game game;
	game.list_blocks();

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cerr << "Failed to initialize the SDL2 library\n";
		return -1;
	}

	SDL_Window *window =
	    SDL_CreateWindow("SDL2 Window", SDL_WINDOWPOS_CENTERED,
			     SDL_WINDOWPOS_CENTERED, 680, 480, 0);

	if (!window) {
		std::cerr << "Failed to create window\n";
		return -1;
	}

	SDL_Surface *window_surface = SDL_GetWindowSurface(window);

	if (!window_surface) {
		std::cout << "Failed to get the surface from the window\n";
		return -1;
	}

	SDL_UpdateWindowSurface(window);

        SDL_Event event;
        while (event.type != SDL_QUIT) {
                SDL_PollEvent(&event);
        }

}
