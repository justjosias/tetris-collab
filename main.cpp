#include <chrono>
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
		this->locations.push_back(std::make_tuple(4, 0));
		this->locations.push_back(std::make_tuple(4, 1));
		this->locations.push_back(std::make_tuple(4, 2));
		this->locations.push_back(std::make_tuple(5, 0));
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

	int max_y()
	{
		auto cur = this->current();
		int max = std::get<1>(cur[0]);
		for (auto loc : cur) {
			if (std::get<1>(loc) > max) {
				max = std::get<1>(loc);
			}
		}
		return max;
	}
	int min_y()
	{
		auto cur = this->current();
		int min = std::get<1>(cur[0]);
		for (auto loc : cur) {
			if (std::get<1>(loc) < min) {
				min = std::get<1>(loc);
			}
		}
		return min;
	}
	int max_x()
	{
		auto cur = this->current();
		int max = std::get<0>(cur[0]);
		for (auto loc : cur) {
			if (std::get<0>(loc) > max) {
				max = std::get<0>(loc);
			}
		}
		return max;
	}
	int min_x()
	{
		auto cur = this->current();
		int min = std::get<0>(cur[0]);
		for (auto loc : cur) {
			if (std::get<0>(loc) < min) {
				min = std::get<0>(loc);
			}
		}
		return min;
	}

	bool collides(int x, int y)
	{
		for (auto loc : locations) {
			if (std::get<0>(loc) == x && std::get<1>(loc) == y) {
				return true;
			}
		}
		return false;
	}
};

class GameState
{
      public:
	vector<Block> blocks;
	int current_block = 0;

	int score = 0;

        int height = 20;
	int width = 10;

	GameState()
	{
		Block block;
		this->blocks.push_back(block);
	}

	void set_size(int h, int w)
	{
		this->height = h;
		this->width = w;
	}

	void right() {
                if (this->blocks[current_block].max_x() < this->width - 1) {
                        this->blocks[current_block].offset_x += 1;
                }
        }

	void left() {
                if (this->blocks[current_block].min_x() > 0) {
                        this->blocks[current_block].offset_x -= 1;
                }
        }

	void down()
	{
		if (this->blocks[current_block].max_y() < this->height - 1) {
			this->blocks[current_block].offset_y += 1;
		}
	}
};

class GameContext
{
      public:
	SDL_Window *window;
	SDL_Surface *window_surface;
	SDL_Renderer *renderer;

	int height = 800;
	int width = 400;

	GameState game;

	// Initializes SDL and the game state
	GameContext()
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			throw "Failed to initialize SDL2";
		}

		this->window = SDL_CreateWindow(
		    "TETRIS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		    this->width, this->height, 0);
		if (window == nullptr) {
			throw "Failed to create window";
		}

		this->renderer =
		    SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

		GameState game;
		game.set_size(40, 20);
	}

	void draw()
	{
		SDL_RenderClear(this->renderer);

		for (auto block : this->game.blocks) {
			for (auto loc : block.current()) {
				SDL_Rect rect;
				rect.x = std::get<0>(loc) * 40;
				rect.y = std::get<1>(loc) * 40;
				rect.w = 40;
				rect.h = 40;

				SDL_SetRenderDrawColor(this->renderer, 255, 0,
						       0, 255);
				SDL_RenderFillRect(renderer, &rect);
			}
		}

		SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);

		// Left line
		SDL_RenderDrawLine(renderer, 0, 0, 0, this->height);
		// Top line
		SDL_RenderDrawLine(renderer, 0, 0, this->height, 0);
		// Right line
		SDL_RenderDrawLine(renderer, this->width - 1, 0,
				   this->width - 1, this->height);
		// Bottom line
		SDL_RenderDrawLine(renderer, 0, this->height - 1, this->width,
				   this->height - 1);

		for (int i = 0; i < this->height; i++) {
			SDL_RenderDrawLine(renderer, 0, i * 40, this->height,
					   i * 40);
		}

		for (int i = 0; i < this->width; i++) {
			SDL_RenderDrawLine(renderer, i * 40, 0, i * 40,
					   this->height);
		}

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

	auto last_time = std::chrono::high_resolution_clock::now();

	bool should_continue = true;
	SDL_Event event;
	while (should_continue) {
		SDL_PollEvent(&event);

		switch (event.type) {
		case SDL_QUIT:
			should_continue = false;
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_RIGHT:
				ctx.game.right();
				break;
			case SDLK_LEFT:
				ctx.game.left();
				break;
			case SDLK_DOWN:
				ctx.game.down();
				break;
			default:
				break;
			}
		default:
			break;
		}

		auto now = std::chrono::high_resolution_clock::now();
		auto diff = now - last_time;
		auto t1 =
		    std::chrono::duration_cast<std::chrono::milliseconds>(diff);
		if (t1.count() > 1000) {
			ctx.game.down();
			last_time = now;
		}

		ctx.draw();
		SDL_UpdateWindowSurface(ctx.window);
	}

	return 0;
}
