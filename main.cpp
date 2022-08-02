#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <vector>

// I love this library
#include <SDL.h>

using std::tuple;
using std::vector;

struct RGB {
	int r;
	int g;
	int b;
};

class Block
{
      public:
	vector<tuple<int, int>> locations;
	int offset_x = 3;
	int offset_y = 0;

	RGB color;

	Block() {}

	auto current() -> vector<tuple<int, int>>
	{
		vector<tuple<int, int>> new_loc;
		for (const auto &location : locations) {
			new_loc.push_back({std::get<0>(location) + offset_x,
					   std::get<1>(location) + offset_y});
		}
		return new_loc;
	}

	int max_y()
	{
		auto cur = this->current();
		int max = std::get<1>(cur[0]);
		for (const auto &loc : cur) {
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
		for (const auto &loc : cur) {
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
		for (const auto &loc : cur) {
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
		for (const auto &loc : cur) {
			if (std::get<0>(loc) < min) {
				min = std::get<0>(loc);
			}
		}
		return min;
	}

	int width()
	{
		int max_x = std::get<0>(this->locations[0]);
		for (const auto &loc : this->locations) {
			if (std::get<0>(loc) > max_x) {
				max_x = std::get<0>(loc);
			}
		}
		return max_x + 1;
	}
	int height()
	{
		int max_y = std::get<1>(this->locations[0]);
		for (const auto &loc : this->locations) {
			if (std::get<1>(loc) > max_y) {
				max_y = std::get<1>(loc);
			}
		}
		return max_y + 1;
	}

	int grid_size()
	{
		auto size = std::max(this->width(), this->height());
		return size;
	}

	bool collides(int x, int y)
	{
		for (const auto &loc : locations) {
			if (std::get<0>(loc) == x && std::get<1>(loc) == y) {
				return true;
			}
		}
		return false;
	}

	int middle()
	{
		auto gs = this->grid_size();
		if (gs % 2 == 0) {
			return -1;
		} else {
			return gs / 2 + 1;
		}
	}

	void rotate()
	{
		auto mid = this->middle();
		if (mid == -1) {
			// TODO: rotate even-sized grids
		} else {
			// (x, y) -> (y, -x)
			for (auto &loc : this->locations) {
				auto x = std::get<0>(loc);
				auto y = std::get<1>(loc);
				std::get<0>(loc) = y;
				std::get<1>(loc) = mid - x;
			}
		}
	}
};

class GameState
{
      public:
	Block block;
	vector<tuple<int, int, RGB>> filled;

	int score = 0;

	int height = 20;
	int width = 10;

	GameState() { this->new_block(); }

	void new_block()
	{
		vector<vector<tuple<int, int>>> block_shapes = {
		    // backwards L
		    {{1, 0}, {1, 1}, {1, 2}, {2, 0}},
		    // forwards L
		    {{1, 0}, {1, 1}, {1, 2}, {0, 0}},
		    // square
		    {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
		};

		vector<RGB> block_colors = {
		    RGB{.r = 255, .g = 0, .b = 0},
		    RGB{.r = 0, .g = 255, .b = 0},
		    RGB{.r = 0, .g = 0, .b = 255},
		};

		// Make a random number generator that provides random indices
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distr(0,
						      block_shapes.size() - 1);

		auto i = distr(gen);
		Block block;
		block.locations = block_shapes[i];
		block.color = block_colors[i];
		this->block = block;
	}

	void set_size(int h, int w)
	{
		this->height = h;
		this->width = w;
	}

	void right()
	{
		if (can_descend() && block.max_x() < this->width - 1) {
			block.offset_x += 1;
		}
	}

	void left()
	{
		if (can_descend() && block.min_x() > 0) {
			block.offset_x -= 1;
		}
	}

	bool can_descend()
	{
                // Descent code BROKEN!
		auto block_loc = block.current();
		for (const auto &sq : filled) {
			if (block.max_y() > std::get<1>(sq) - 2) {
                                for (const auto &loc : block_loc) {
                                        if (std::get<0>(loc) == std::get<0>(sq)) {
                                                return false;
                                        }
                                }
			}
		}
                // end broken section
		if (block.max_y() < this->height - 1) {
			return true;
		}

		return false;
	}

	void down()
	{
		if (can_descend()) {
			block.offset_y += 1;

		} else {
			for (const auto &loc : block.current()) {
				filled.push_back({std::get<0>(loc),
						  std::get<1>(loc),
						  block.color});
			}
			this->new_block();
		}
	}

	void rotate()
	{
		if (can_descend()) {
			this->block.rotate();
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

		for (const auto &loc : game.block.current()) {
			SDL_Rect rect;
			rect.x = std::get<0>(loc) * 40;
			rect.y = std::get<1>(loc) * 40;
			rect.w = 40;
			rect.h = 40;

			SDL_SetRenderDrawColor(
			    this->renderer, game.block.color.r,
			    game.block.color.g, game.block.color.b, 255);
			SDL_RenderFillRect(renderer, &rect);
		}

		for (const auto &loc : game.filled) {
			SDL_Rect rect;
			rect.x = std::get<0>(loc) * 40;
			rect.y = std::get<1>(loc) * 40;
			rect.w = 40;
			rect.h = 40;
			auto rgb = std::get<2>(loc);
			SDL_SetRenderDrawColor(this->renderer, rgb.r, rgb.g,
					       rgb.b, 255);
			SDL_RenderFillRect(renderer, &rect);
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

	auto last_time = SDL_GetTicks64();

	bool redraw = true;
	bool should_continue = true;
	SDL_Event event;
	while (should_continue) {
		SDL_PollEvent(&event);

		switch (event.type) {
		case SDL_QUIT:
			should_continue = false;
			break;
		case SDL_KEYDOWN:
			redraw = true;
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
			case SDLK_UP:
				ctx.game.rotate();
				break;
			default:
				break;
			}
		default:
			break;
		}

		if (SDL_GetTicks64() - last_time > 1000) {
			ctx.game.down();
			last_time = SDL_GetTicks64();
			redraw = true;
		}

		if (redraw) {
			ctx.draw();
			SDL_UpdateWindowSurface(ctx.window);
			redraw = false;
		}

		// Keep the game from hogging all the CPU
		SDL_Delay(10);
	}

	return 0;
}
