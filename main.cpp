#include <cmath>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#include <SDL.h>

using namespace std;

class Block
{
      public:
	vector<tuple<int, int>> locations;
	int offset_x;
	int offset_y;

	Block()
	{
		this->locations.push_back(make_tuple(0, 0));
		this->offset_x = 0;
		this->offset_y = 0;
	}

	auto current() -> vector<tuple<int, int>>
	{
		vector<tuple<int, int>> new_loc;
		for (auto loc : locations) {
			new_loc.push_back(make_tuple(get<0>(loc) + offset_x,
						     get<1>(loc) + offset_y));
		}
		return new_loc;
	}
};

class GameState
{
      public:
	GameState() {}
};

class GameContext
{
      public:
	int height;
	int width;

	SDL_Window *window;
	SDL_Surface *window_surface;
	SDL_Renderer *renderer;

        GameState state;

	GameContext()
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

		this->renderer =
		    SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	}

	void decorate_window()
	{
		int x = 100;
		int y = 100;
		SDL_SetRenderDrawColor(this->renderer, 255, 0, 0, 255);
		SDL_RenderDrawPoint(this->renderer, x, y);
		SDL_RenderPresent(renderer);
	}

	void draw_circle(int center_x, int center_y, int radius_)
	{
		// Setting the color to be RED with 100% opaque (0% trasparent).
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

		// Drawing circle
		for (int x = center_x - radius_; x <= center_x + radius_; x++) {
			for (int y = center_y - radius_;
			     y <= center_y + radius_; y++) {
				if ((std::pow(center_y - y, 2) +
				     std::pow(center_x - x, 2)) <=
				    std::pow(radius_, 2)) {
					SDL_RenderDrawPoint(renderer, x, y);
				}
			}
		}

		// Show the change on the screen
		SDL_RenderPresent(renderer);
	}

	~GameContext()
	{
		SDL_DestroyRenderer(renderer);
		SDL_FreeSurface(window_surface);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
};

int main(int argc, char **argv)
{
	GameContext ctx;

	ctx.draw_circle(200, 100, 50);
	SDL_UpdateWindowSurface(ctx.window);

	SDL_Event event;
	while (true) {
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
			break;
		}
	}

	return 0;
}
