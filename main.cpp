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
#include <vector>

// I love this library
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

using std::vector;

struct RGB {
	int r;
	int g;
	int b;
};

struct Location {
	int x;
	int y;
};

class Block
{
      public:
	// The relative locations of individual blocks in a tetromino
	vector<Location> locations;

	// The current offsets to determine where the tetromino is on the game board.
	int offset_x = 3;
	int offset_y = 0;

	// The color of the block in RGB
	RGB color;

	Block() {}

	// Calculate the current coordinates by applying the offsets to locations
	vector<Location> coordinates()
	{
		vector<Location> new_loc;
		for (const auto &location : locations) {
			new_loc.push_back({location.x + offset_x, location.y + offset_y});
		}
		return new_loc;
	}

	int max_y()
	{
		auto cur = this->coordinates();
		int max = cur[0].y;
		for (const auto &loc : cur) {
			if (loc.y > max) {
				max = loc.y;
			}
		}
		return max;
	}
	int min_y()
	{
		auto cur = this->coordinates();
		int min = cur[0].y;
		for (const auto &loc : cur) {
			if (loc.y < min) {
				min = loc.y;
			}
		}
		return min;
	}
	int max_x()
	{
		auto cur = this->coordinates();
		int max = cur[0].x;
		for (const auto &loc : cur) {
			if (loc.x > max) {
				max = loc.x;
			}
		}
		return max;
	}
	int min_x()
	{
		auto cur = this->coordinates();
		int min = cur[0].x;
		for (const auto &loc : cur) {
			if (loc.x < min) {
				min = loc.x;
			}
		}
		return min;
	}

	int width()
	{
		int max_x = this->locations[0].x;
		for (const auto &loc : this->locations) {
			if (loc.x > max_x) {
				max_x = loc.x;
			}
		}
		return max_x + 1;
	}
	int height()
	{
		int max_y = this->locations[0].y;
		for (const auto &loc : this->locations) {
			if (loc.y > max_y) {
				max_y = loc.y;
			}
		}
		return max_y + 1;
	}

	// Return the size of the virtual grid surrounding the block.
	// This is essential for calculating rotation.
	// THIS IS THE SIZE OF THE GRID OF THE BLOCK NOT THE ENTIRE GRID
	int grid_size()
	{
		auto size = std::max(this->width(), this->height());
		return size;
	}

	bool collides(int x, int y)
	{
		for (const auto &loc : locations) {
			if (loc.x == x && loc.y == y) {
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
			auto x = loc.x;
			auto y = loc.y;
			loc.x = y;
			loc.y = mid - x;
		}
	}
};

struct Minigrid {
	Block block;
};

struct FilledBlock {
	int x;
	int y;
	RGB color;
};

class GameState
{
      public:
	// The falling tetromino
	Block block;

	// The next seven tetrominos on the list to fall.
	// The pool is refilled when all the contained tetrominos are used.
	vector<Block> block_pool;

	// The block locations for all the currently filled blocks.
	// This is used to render the fallen blocks, as well as determine if a row has been cleared.
	vector<FilledBlock> filled;

	int score = 0;

	int level = 1;

	// The number of rows needed to be cleared before the next level is reached.
	int level_left = 5;

	// Time (in milliseconds) between letting the tetromino fall a block.
	// Decreases every level by 75%.
	int tickspeed = 1000;

	// The height and width of the game board in blocks
	int height = 20;
	int width = 10;

	bool gameover = false;

	// The contents of the grid for previewing the next tetromino in the queue.
	Minigrid minigrid;

	GameState()
	{
		this->replenish_pool();
		this->next_block();
	}

	void replenish_pool()
	{
		vector<vector<Location>> block_shapes = {
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
				if (is_filled(loc.x + b.offset_x, loc.y) + b.offset_y) {
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
		vector<Location> block_locations = block.coordinates();
		for (const auto &floc : filled) {
			for (const auto &bloc : block_locations) {
				if (bloc.x + x == floc.x && bloc.y + y == floc.y) {
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
				this->filled.erase(std::remove_if(this->filled.begin(),
								  this->filled.end(),
								  [y](auto f) { return f.y == y; }),
						   this->filled.end());

				// Bring down rest of blocks
				for (auto &loc : this->filled) {
					if (loc.y <= y) {
						loc.y += 1;
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
				filled.push_back({loc.x, loc.y, block.color});
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
			if (this->is_filled(loc.x, loc.y)) {
				return false;
			} else if (loc.x > this->width - 1) {
				if (this->can_move(-1, 0)) {
					this->left();
					continue;
				}
				return false;
			} else if (loc.x < 0) {
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
			if (loc.x == x && loc.y == y) {
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
	// SDL values used for rendering the game
	SDL_Window *window;
	SDL_Surface *window_surface;
	SDL_Renderer *renderer;

	// The height and width of the game window
	int height = 800;
	int width = 1000;

	// The main font used for rendering text to the screen.
	// Currently Sans.ttf
	TTF_Font *font;

	// The x and y offsets for the position of the game itself.
	// In most cases x will be determined by whatever makes the
	// game centered and y will be zero.
	Location game_offset;

	// An instance of GameState, which contains the inner-workings of the game.
	// State for the game should not be stored elsewhere.
	GameState game;

	// The song to be run in the background.
	// Loaded from Korobeiniki.wav
	Mix_Chunk *music;

	// The size (in pixels) of individual blocks.
	// A tetromino consists of multiple blocks.
	int block_size = double(height) * 0.05;

	// Initializes SDL and the game state
	GameContext()
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
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
		game.set_size(20, 10);
		this->game = game;

		this->game_offset = {(width - (game.width * this->block_size)) / 2, 0};

		TTF_Init();
		this->font = TTF_OpenFont("Sans.ttf", 14);
		if (!this->font) {
			throw "Failed to load Sans.ttf";
		}

		Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
		Mix_Volume(-1, 20);

		this->music = Mix_LoadWAV("Korobeiniki.wav");
		Mix_PlayChannel(-1, this->music, -1);

		SDL_SetWindowResizable(window, SDL_TRUE);
	}

	void draw()
	{
		// Number of digits in each statistic type
		int scoreLength = 6;
		int levelLength = 6;
		
		if (std::to_string(abs(game.score)).length() < 6) {
		scoreLength = std::to_string(abs(game.score)).length();
		}
		
		if (std::to_string(abs(game.level)).length() < 6) {
		levelLength = std::to_string(abs(game.level)).length();
		}

		
		
		//Set SDL screen to gray
		SDL_SetRenderDrawColor(this->renderer, 84, 84, 84, 255);
		SDL_RenderClear(this->renderer);

		this->game_offset = {(width - (game.width * this->block_size)) / 2, 0};

		// Left and right borders of the Tetris board
		int leftBorder = this->game_offset.x;
		int rightBorder = this->game_offset.x + this->game.width * this->block_size;

		//Set SDL screen to black
		SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);

		SDL_Rect board = {
		    .x = leftBorder,
		    .y = game_offset.y,
		    .w = this->game.width * this->block_size,
		    .h = this->game.height * this->block_size,
		};
		SDL_RenderFillRect(renderer, &board);

		// Scoreboard
		SDL_Rect scoreboard = {
		    .x = rightBorder + this->block_size - 10,
		    .y = this->block_size,
		    .w = this->block_size * 6,
		    .h = this->block_size * 6,
		};
		SDL_RenderFillRect(renderer, &scoreboard);

		SDL_Rect scoretext = {
		    .x = scoreboard.x + (this->block_size / 2),
		    .y = this->block_size,
		    .w = this->block_size * 5,
		    .h = (this->block_size * 6) / 3,
		};
		SDL_RenderFillRect(renderer, &scoretext);


		// How far the score needs to be pushed to the left
		int modifier = scoreLength * (block_size / 4) * 2;
		int levelModifier = levelLength * (block_size / 4) * 2;

		SDL_Rect livescore = {
		    .x = scoretext.x + (scoretext.w / 2) - modifier,
		    .y = scoretext.y + (this->block_size * 2),
		    .w = (this->block_size * 2) * (double(scoreLength) / 2),
		    .h = (this->block_size * 6) / 1.75,
		};
		SDL_RenderFillRect(renderer, &livescore);
		// End scoreboard

		// Level board
		SDL_Rect levelboard = {
		    .x = (rightBorder + this->block_size - 10),
		    .y = this->block_size * 8,
		    .w = this->block_size * 6,
		    .h = this->block_size * 6,
		};
		SDL_RenderFillRect(renderer, &levelboard);

		SDL_Rect leveltext = {
		    .x = scoreboard.x + (this->block_size / 2),
		    .y = this->block_size * 8,
		    .w = this->block_size * 5,
		    .h = (this->block_size * 6) / 3,
		};
		SDL_RenderFillRect(renderer, &leveltext);

		SDL_Rect livelevel = {
		    .x = leveltext.x + (leveltext.w / 2) - levelModifier,
		    .y = leveltext.y + (this->block_size * 2),
		    .w = (this->block_size * 2) * (double(levelLength) / 2),
		    .h = (this->block_size * 6) / 1.75,
		};
		SDL_RenderFillRect(renderer, &livelevel);
		// End level board



		// Write words to the screen
		SDL_Color White = {255, 255, 255};

		
		// Write "SCORE"
		SDL_Surface *scoreSurfaceMessage = TTF_RenderText_Solid(this->font, "SCORE", White);
		SDL_Texture *scoreMessage = SDL_CreateTextureFromSurface(renderer, scoreSurfaceMessage);
		SDL_RenderCopy(renderer, scoreMessage, NULL, &scoretext);

		// Write "LEVEL"
		SDL_Surface *levelSurfaceMessage = TTF_RenderText_Solid(this->font, "LEVEL", White);
		SDL_Texture *levelMessage = SDL_CreateTextureFromSurface(renderer, levelSurfaceMessage);
		SDL_RenderCopy(renderer, levelMessage, NULL, &leveltext);

		// Write updated score to screen
		SDL_Surface *displayScore =
		TTF_RenderText_Solid(this->font, std::to_string(game.score).c_str(), White);
		SDL_Texture *Message2 = SDL_CreateTextureFromSurface(renderer, displayScore);
		SDL_RenderCopy(renderer, Message2, NULL, &livescore);

		// Write updated level to screen
		SDL_Surface *displayLevel =
		TTF_RenderText_Solid(this->font, std::to_string(game.level).c_str(), White);
		SDL_Texture *liveLevelDisplay = SDL_CreateTextureFromSurface(renderer, displayLevel);
		SDL_RenderCopy(renderer, liveLevelDisplay, NULL, &livelevel);

		for (const auto &loc : game.block.coordinates()) {
			SDL_Rect rect = {
			    .x = loc.x * this->block_size + this->game_offset.x,
			    .y = loc.y * this->block_size,
			    .w = this->block_size,
			    .h = this->block_size,
			};

			SDL_SetRenderDrawColor(this->renderer, game.block.color.r,
					       game.block.color.g, game.block.color.b, 255);
			SDL_RenderFillRect(renderer, &rect);
		}

		for (const auto &loc : game.filled) {
			SDL_Rect rect = {
			    .x = loc.x * this->block_size + this->game_offset.x,
			    .y = loc.y * this->block_size,
			    .w = this->block_size,
			    .h = this->block_size,
			};
			auto rgb = loc.color;
			SDL_SetRenderDrawColor(this->renderer, rgb.r, rgb.g, rgb.b, 255);
			SDL_RenderFillRect(renderer, &rect);
		}

		// Draw Minigrid
		auto mg_size = this->block_size * 6;
		SDL_Rect mg_back = {
		    .x = leftBorder - mg_size - this->block_size + 10,
		    .y = this->block_size,
		    .w = mg_size,
		    .h = mg_size,
		};
		SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &mg_back);

		auto block_margin = leftBorder - this->block_size * 6 - this->block_size + 10;

		auto tmp_block_size = mg_size / this->game.minigrid.block.grid_size();
		for (const auto &loc : this->game.minigrid.block.locations) {
			if (tmp_block_size > this->block_size) {
				tmp_block_size = this->block_size;
			}
			SDL_Rect rect = {
			    .x = loc.x * tmp_block_size + block_margin + block_size,
			    .y = loc.y * tmp_block_size + this->block_size * 2,
			    .w = tmp_block_size,
			    .h = tmp_block_size,
			};

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
		Mix_FreeChunk(this->music);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
};

int main(int argc, char **argv)
{
	GameContext ctx;

	auto last_time = SDL_GetTicks();

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
				ctx.game.score += 1 * ctx.game.level;
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
				Mix_Pause(-1);
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				paused = false;
				Mix_Resume(-1);
				break;
			case SDL_WINDOWEVENT_RESIZED:
				int w, h;
				SDL_GetWindowSize(ctx.window, &w, &h);
				ctx.width = w;
				ctx.height = h;
				ctx.block_size = double(ctx.height) * 0.05;
				redraw = true;
				break;
			}
		default:
			break;
		}

		if (SDL_GetTicks() - last_time > ctx.game.tickspeed) {
			ctx.game.down();
			last_time = SDL_GetTicks();
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
