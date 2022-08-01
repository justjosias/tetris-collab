#include <cmath>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

// I love this library
#include <SDL.h>

using std::tuple;
using std::vector;

class Block
{
      public:
	vector<tuple<int, int>> locations;
	int offset_x;
	int offset_y;

	Block()
	{
		this->locations.push_back(std::make_tuple(0, 0));
		this->offset_x = 0;
		this->offset_y = 0;
	}

	auto current() -> vector<tuple<int, int>>
	{
		vector<tuple<int, int>> new_loc;
		for (auto location : locations) {
			new_loc.push_back(
			    std::make_tuple(std::get<0>(location) + offset_x,
					    std::get<1>(location) + offset_y));
		}
		return new_loc;
	}
};

class GameState
{
      public:
	vector<Block> blocks;

	int score = 0;

	GameState() {}

	void set_size(int h, int w)
	{
		this->height = h;
		this->width = w;
	}

      private:
	int height = 20;
	int width = 10;
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

	// Initializes SDL and the game state
	GameContext()
	{
		this->height = 20;
		this->width = 10;

		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			throw "Failed to initialize SDL2";
		}

		this->window =
		    SDL_CreateWindow("TETRIS", SDL_WINDOWPOS_CENTERED,
				     SDL_WINDOWPOS_CENTERED, 1000, 800, 0);
		if (window == nullptr) {
			throw "Failed to create window";
		}

		this->renderer =
		    SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

		GameState state;
		state.set_size(this->height, this->width);
	}

	void draw()
	{
                SDL_RenderClear(this->renderer);
                        
                SDL_Rect rect;
                rect.x = 200;
                rect.y = 200;
                rect.w = 100;
                rect.h = 100;
                
                //SDL_SetRenderDrawColor(this->renderer, 255, 0, 0, 255);
                SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);
                SDL_RenderDrawRect(this->renderer, &rect);
                SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
                SDL_RenderPresent(this->renderer);
	}

	~GameContext()
	{
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
};

int main(int argc, char **argv)
{
	GameContext ctx;

	SDL_Event event;

        bool should_continue = true;
	while (should_continue) {
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
		        should_continue = false;
		}

                ctx.draw();
                SDL_UpdateWindowSurface(ctx.window);
	}

	return 0;
}
