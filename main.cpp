/* Copyright 2022 Josias Allestad <me@josias.dev> and Jacob <zathaxx@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>. */
#include <algorithm>
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

const int BLOCK_SIZE = 40;

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

	auto coordinates() -> vector<tuple<int, int>>
	{
		vector<tuple<int, int>> new_loc;
		for (const auto &location : locations) {
			new_loc.push_back(
			    {std::get<0>(location) + offset_x, std::get<1>(location) + offset_y});
		}
		return new_loc;
	}

	int max_y()
	{
		auto cur = this->coordinates();
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
		auto cur = this->coordinates();
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
		auto cur = this->coordinates();
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
		auto cur = this->coordinates();
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

	// THIS IS THE SIZE OF THE GRID OF THE BLOCK NOT THE ENTIRE GRID
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
			return gs / 2;
		} else {
			return gs / 2 + 1;
		}
	}

	void rotate()
	{
		auto mid = this->middle();
		// (x, y) -> (y, -x)
		for (auto &loc : this->locations) {
			auto x = std::get<0>(loc);
			auto y = std::get<1>(loc);
			std::get<0>(loc) = y;
			std::get<1>(loc) = mid - x;
		}
	}
};

struct Minigrid {
	Block block;
	int offset_x = BLOCK_SIZE;
	int offset_y = BLOCK_SIZE * 2;

	int width = 5;
	int height = 5;
};

class GameState
{
      public:
	Block block;
	vector<Block> block_pool;
	vector<tuple<int, int, RGB>> filled;

	int score = 0;
	int level = 1;
	int level_left = 5;
	int tickspeed = 1000;

	int height = 20;
	int width = 10;

	bool gameover = false;

	Minigrid minigrid;

	GameState()
	{
		this->replenish_pool();
		this->next_block();
	}

	void replenish_pool()
	{
		vector<vector<tuple<int, int>>> block_shapes = {
		    // J Shape
		    {{1, 0}, {1, 1}, {1, 2}, {2, 0}},
		    // L Shape
		    {{1, 0}, {1, 1}, {1, 2}, {0, 0}},
		    // O Shope
		    {{0, 0}, {0, 1}, {1, 0}, {1, 1}},
		    // I Shape
		    {{1, 0}, {1, 1}, {1, 2}, {1, 3}},
		    // T shape
		    {{1, 0}, {0, 1}, {1, 1}, {1, 2}},
		    // Z shape
		    {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
		    // S shape
		    {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
		};

		// Make a random number generator that provides random indices
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distr(0, block_shapes.size() - 1);

		vector<RGB> block_colors = {
		    RGB{255, 0, 0},   RGB{0, 255, 0},  RGB{0, 0, 255},	RGB{255, 255, 0},
		    RGB{0, 255, 255}, RGB{90, 0, 255}, RGB{255, 0, 90},
		};

		vector<int> taken;
		for (size_t i = 0; i < block_shapes.size(); ++i) {
			auto num = distr(gen);
			while (std::find(taken.begin(), taken.end(), num) != taken.end()) {
				num = distr(gen);
			}
			Block b;
			b.locations = block_shapes[num];
			for (auto loc : b.locations) {
				if (is_filled(std::get<0>(loc) + b.offset_x, std::get<1>(loc)) +
				    b.offset_y) {
					this->gameover = true;
				}
			}
			b.color = block_colors[num];

			this->block_pool.push_back(b);
			taken.push_back(num);
		}
	}

	void next_block()
	{
		if (this->block_pool.size() > 0) {
			auto i = this->block_pool.size() - 1;
			this->block = this->block_pool[i];
			this->block_pool.erase(this->block_pool.begin() + i);
		}
                if (this->block_pool.size() < 1) {
			this->replenish_pool();
		}
                auto i = this->block_pool.size() - 1;
                this->minigrid.block = this->block_pool[i];
	}

	void set_size(int h, int w)
	{
		this->height = h;
		this->width = w;
	}

	void right()
	{
		if (can_move(1, 0) && block.max_x() < this->width - 1) {
			block.offset_x += 1;
		}
	}

	void left()
	{
		if (can_move(-1, 0) && block.min_x() > 0) {
			block.offset_x -= 1;
		}
	}

	bool can_descend()
	{
		if (can_move(0, 1) && block.max_y() < this->height - 1) {
			return true;
		}
		return false;
	}

	bool can_move(int x, int y)
	{
		vector<tuple<int, int>> block_locations = block.coordinates();
		for (const auto &floc : filled) {
			for (const auto &bloc : block_locations) {
				if (std::get<0>(bloc) + x == std::get<0>(floc) &&
				    std::get<1>(bloc) + y == std::get<1>(floc)) {
					return false;
				}
			}
		}
		return true;
	}

	void clear_complete()
	{
		int rows = this->height;
		for (int y = 0; y < this->height; ++y) {
			bool filled = true;
			for (int x = 0; x < this->width; ++x) {
				if (!is_filled(x, y)) {
					rows--;
					filled = false;
					break;
				}
			}
			if (filled) {
				// Erase filled row
				this->filled.erase(
				    std::remove_if(this->filled.begin(), this->filled.end(),
						   [y](auto f) { return std::get<1>(f) == y; }),
				    this->filled.end());

				// Bring down rest of blocks
				for (auto &loc : this->filled) {
					if (std::get<1>(loc) <= y) {
						std::get<1>(loc) += 1;
					}
				}
			}
		}
		auto to_add = 0;
		if (rows <= 3) {
			to_add = ((rows * 100) * rows);
		} else {
			to_add = 2000;
		}
		to_add *= this->level;

		// Check for a perfect clear
		if (this->filled.size() == 0) {
			to_add *= 10;
		}

		this->score += to_add;

		this->level_left -= rows;
		if (this->level_left < 1) {
			this->level_left = 5;
			this->level += 1;
			this->tickspeed *= 0.75;
		}
	}

	void down()
	{
		if (can_descend()) {
			block.offset_y += 1;
			score += 1 * this->level;
		} else {
			for (const auto &loc : block.coordinates()) {
				filled.push_back({std::get<0>(loc), std::get<1>(loc), block.color});
			}

			this->clear_complete();
			this->next_block();
		}
	}

	bool can_rotate()
	{
		Block next_block = this->block;
		next_block.rotate();

		for (const auto &loc : next_block.coordinates()) {
			if (this->is_filled(std::get<0>(loc), std::get<1>(loc))) {
				return false;
			} else if (std::get<0>(loc) > this->width - 1) {
				if (this->can_move(-1, 0)) {
					this->left();
					continue;
				}
				return false;
			} else if (std::get<0>(loc) < 0) {
				if (this->can_move(1, 0)) {
					this->right();
					continue;
				}
				return false;
			}
		}

		return true;
	}

	bool is_filled(int x, int y)
	{
		for (const auto &loc : filled) {
			if (std::get<0>(loc) == x && std::get<1>(loc) == y) {
				return true;
			}
		}
		return false;
	}

	void rotate()
	{
		if (can_descend() && can_rotate()) {
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
	int width = 1000;

	tuple<int, int> game_offset;

	GameState game;

	// Initializes SDL and the game state
	GameContext()
	{
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			throw "Failed to initialize SDL2";
		}

		this->window =
		    SDL_CreateWindow("TETRIS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				     this->width, this->height, 0);
		if (window == nullptr) {
			throw "Failed to create window";
		}

		this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

		GameState game;
		game.set_size(40, 20);

		SDL_SetWindowResizable(window, SDL_TRUE);

		this->game_offset = {(width - (game.width * BLOCK_SIZE / 2)) / 2, 0};
	}

	void draw()
	{
		SDL_SetRenderDrawColor(this->renderer, 84, 84, 84, 255);
		SDL_RenderClear(this->renderer);

		int leftBorder = std::get<0>(this->game_offset);
		int rightBorder = std::get<0>(this->game_offset) + this->game.width * BLOCK_SIZE;

		SDL_Rect board;
		board.x = leftBorder;
		board.y = std::get<1>(game_offset);
		board.w = this->game.width * BLOCK_SIZE;
		board.h = this->game.height * BLOCK_SIZE;
		SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &board);

		SDL_Rect score;
		score.x = rightBorder + BLOCK_SIZE - 10;
		score.y = 40;
		score.w = 240;
		score.h = 240;
		SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &score);

		for (const auto &loc : game.block.coordinates()) {
			SDL_Rect rect;
			rect.x = std::get<0>(loc) * BLOCK_SIZE + std::get<0>(this->game_offset);
			rect.y = std::get<1>(loc) * BLOCK_SIZE;
			rect.w = BLOCK_SIZE;
			rect.h = BLOCK_SIZE;

			SDL_SetRenderDrawColor(this->renderer, game.block.color.r,
					       game.block.color.g, game.block.color.b, 255);
			SDL_RenderFillRect(renderer, &rect);
		}

		for (const auto &loc : game.filled) {
			SDL_Rect rect;
			rect.x = std::get<0>(loc) * BLOCK_SIZE + std::get<0>(this->game_offset);
			rect.y = std::get<1>(loc) * BLOCK_SIZE;
			rect.w = BLOCK_SIZE;
			rect.h = BLOCK_SIZE;
			auto rgb = std::get<2>(loc);
			SDL_SetRenderDrawColor(this->renderer, rgb.r, rgb.g, rgb.b, 255);
			SDL_RenderFillRect(renderer, &rect);
		}

		SDL_SetRenderDrawColor(this->renderer, 255, 255, 255, 255);

		// Left line
		SDL_RenderDrawLine(renderer, 0, 0, 0, this->height);
		// Top line
		SDL_RenderDrawLine(renderer, 0, 0, this->width, 0);
		// Right line
		SDL_RenderDrawLine(renderer, this->width - 1, 0, this->width - 1, this->height);
		// Bottom line
		SDL_RenderDrawLine(renderer, 0, this->height - 1, this->width, this->height - 1);
		// Left Border
		SDL_RenderDrawLine(renderer, leftBorder, 0, leftBorder, this->height);
		// Right Border
		SDL_RenderDrawLine(renderer, rightBorder, 0, rightBorder, this->height);

		// Draw Minigrid
		SDL_Rect mg_back;
		mg_back.x = this->game.minigrid.offset_x;
		mg_back.y = this->game.minigrid.offset_y;
		mg_back.w = this->game.minigrid.width * BLOCK_SIZE;
		mg_back.h = this->game.minigrid.height * BLOCK_SIZE;
		SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &mg_back);

		for (const auto &loc : this->game.minigrid.block.locations) {
			SDL_Rect rect;
			rect.x =
			    (std::get<0>(loc) + 1) * BLOCK_SIZE + this->game.minigrid.offset_x + 1;
			rect.y = (std::get<1>(loc) + 1) * BLOCK_SIZE + this->game.minigrid.offset_y;
			rect.w = BLOCK_SIZE;
			rect.h = BLOCK_SIZE;

			SDL_SetRenderDrawColor(this->renderer, this->game.minigrid.block.color.r,
					       this->game.minigrid.block.color.g,
					       this->game.minigrid.block.color.b, 255);
			SDL_RenderFillRect(renderer, &rect);
		}
		// End minigrid drawing

		SDL_SetRenderDrawColor(this->renderer, 84, 84, 84, 255);
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
	bool rotation_pressed = false;
	bool paused = false;

	SDL_Event event;
	while (should_continue) {
		SDL_PollEvent(&event);

		if (paused) {
			if (event.type == SDL_WINDOWEVENT &&
			    event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
			} else {
				continue;
			}
		}

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
				if (!rotation_pressed) {
					ctx.game.rotate();
					rotation_pressed = true;
				}
				break;
			default:
				break;
			}
			break;
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_UP) {
				rotation_pressed = false;
			}
			break;
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_FOCUS_LOST:
				paused = true;
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				paused = false;
				break;
			}
		default:
			break;
		}

		if (SDL_GetTicks64() - last_time > ctx.game.tickspeed) {
			ctx.game.down();
			last_time = SDL_GetTicks64();
			redraw = true;
		}

		if (redraw) {
			ctx.draw();
			SDL_UpdateWindowSurface(ctx.window);
			redraw = false;
		}

		if (ctx.game.gameover) {
			break;
		}

		// Keep the game from hogging all the CPU
		SDL_Delay(10);
	}

	return 0;
}
