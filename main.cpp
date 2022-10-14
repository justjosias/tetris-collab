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

struct FilledBlock {
	int x;
	int y;
	RGB color;
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

	bool can_move(int x, int y, vector<FilledBlock> *filled)
	{
		vector<Location> block_locations = this->coordinates();
		for (const auto &floc : *filled) {
			for (const auto &bloc : block_locations) {
				if (bloc.x + x == floc.x && bloc.y + y == floc.y) {
					return false;
				}
			}
		}
		return true;
	}

	bool can_descend(vector<FilledBlock> *filled, int height)
	{
		if (this->can_move(0, 1, filled) && this->max_y() < height - 1) {
			return true;
		}
		return false;
	}
};

struct Minigrid {
	Block block;
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

	// The current score of the player. Score is added when:
	// * A tetromino descends (1 * level)
	// * A row is cleared ((rows * 100) * rows, if less than 3, otherwise 2000. Multiplied by level)
	// * With a complete clear, the former rule applies and is also multiplied by 10.
	int score = 0;

	// The current level, incremented for every five rows cleared.
	// Used as a multiplier when calculating the score.
	int level = 1;

	// The number of rows needed to be cleared before the next level is reached.
	int level_left = 5;

	// Time (in milliseconds) between letting the tetromino fall a block.
	// Decreases every level by 75%.
	unsigned int tickspeed = 1000;

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
			b.color = block_colors[num];

			this->block_pool.push_back(b);
			taken.push_back(num);
		}
	}

	// Checks if any block in the top row of the board is filled
	bool is_gameover()
	{
		for (int i = 0; i < this->width; ++i) {
			if (is_filled(i, 0)) {
				return true;
			}
		}
		return false;
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
		this->gameover = this->is_gameover();
	}

	void set_size(int h, int w)
	{
		this->height = h;
		this->width = w;
	}

	void right()
	{
		if (block.can_move(1, 0, &filled) && block.max_x() < this->width - 1) {
			block.offset_x += 1;
		}
	}

	void left()
	{
		if (block.can_move(-1, 0, &filled) && block.min_x() > 0) {
			block.offset_x -= 1;
		}
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
		if (block.can_descend(&filled, height)) {
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

	Block bottom(int *dropped)
	{
		int d = 0;
		Block tmp = this->block;
		while (tmp.can_descend(&filled, height)) {
			tmp.offset_y += 1;
			d += 1;
		}
		if (dropped) {
			*dropped = d;
		}
		return tmp;
	}

	void drop()
	{
		int dropped = 0;
		this->block = this->bottom(&dropped);
		this->down();
		this->score += dropped * this->level;
	}

	bool can_rotate()
	{
		Block next_block = this->block;
		next_block.rotate();

		for (const auto &loc : next_block.coordinates()) {
			if (this->is_filled(loc.x, loc.y)) {
				return false;
			} else if (loc.x > this->width - 1) {
				if (block.can_move(-1, 0, &filled)) {
					this->left();
					continue;
				}
				return false;
			} else if (loc.x < 0) {
				if (block.can_move(1, 0, &filled)) {
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
		if (block.can_descend(&filled, height) && can_rotate()) {
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
	int height = 820;
	int width = 630;

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
	int paused = false;

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

		this->game_offset = {10, 10};

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

		int scoreLength = std::to_string(abs(game.score)).length();
		int levelLength = std::to_string(abs(game.level)).length();

		if (scoreLength > 6) {
			scoreLength = 6;
		}
		if (levelLength > 6) {
			levelLength = 6;
		}

		// Set SDL screen to gray
		SDL_SetRenderDrawColor(this->renderer, 84, 84, 84, 255);
		SDL_RenderClear(this->renderer);

		// Left and right borders of the Tetris board
		int rightBorder = this->game_offset.x + this->game.width * this->block_size;

		// Set SDL screen to black
		SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);

		SDL_Rect board = {
		    .x = this->game_offset.x,
		    .y = this->game_offset.y,
		    .w = this->game.width * this->block_size,
		    .h = this->game.height * this->block_size,
		};
		SDL_RenderFillRect(renderer, &board);

		// Scoreboard
		int box_scale = 5;

		SDL_Rect scoreboard = {
		    .x = rightBorder + (this->block_size / 4),
		    .y = this->block_size / 4,
		    .w = this->block_size * box_scale,
		    .h = this->block_size * box_scale,
		};
		SDL_RenderFillRect(renderer, &scoreboard);

		SDL_Rect scoretext = {
		    .x = scoreboard.x + scoreboard.w / 6,
		    .y = scoreboard.y,
		    .w = (scoreboard.w * 2) / 3,
		    .h = scoreboard.h / 3,
		};
		SDL_RenderFillRect(renderer, &scoretext);

		// How far the score needs to be pushed to the left
		int modifier = scoreLength * (block_size / 4);
		int levelModifier = levelLength * (block_size / 4);

		SDL_Rect livescore = {
		    .x = scoreboard.x + (scoreboard.w / 2) - modifier,
		    .y = scoreboard.y + (scoreboard.h * 2 / 5),
		    .w = (scoreboard.w / (box_scale * 2)) * scoreLength,
		    .h = scoreboard.h / 3,
		};
		SDL_RenderFillRect(renderer, &livescore);
		// End scoreboard

		// Level board
		SDL_Rect levelboard = {
		    .x = rightBorder + (this->block_size / 4),
		    .y = this->block_size * (box_scale + 1) - this->block_size / 2,
		    .w = this->block_size * box_scale,
		    .h = this->block_size * box_scale,
		};
		SDL_RenderFillRect(renderer, &levelboard);

		SDL_Rect leveltext = {
		    .x = levelboard.x + levelboard.w / 6,
		    .y = levelboard.y,
		    .w = (levelboard.w * 2) / 3,
		    .h = scoreboard.h / 3,
		};
		SDL_RenderFillRect(renderer, &leveltext);

		SDL_Rect livelevel = {
		    .x = levelboard.x + (levelboard.w / 2) - levelModifier,
		    .y = levelboard.y + (levelboard.h * 2 / 5),
		    .w = (levelboard.w / (box_scale * 2)) * levelLength,
		    .h = levelboard.h / 3,
		};
		SDL_RenderFillRect(renderer, &livelevel);
		// End level board

		// Write words to the screen
		SDL_Color White = {255, 255, 255, 255};

		// Write "SCORE"
		SDL_Surface *scoreSurfaceMessage = TTF_RenderText_Solid(this->font, "SCORE", White);
		SDL_Texture *scoreMessage =
		    SDL_CreateTextureFromSurface(renderer, scoreSurfaceMessage);
		SDL_RenderCopy(renderer, scoreMessage, NULL, &scoretext);

		// Write "LEVEL"
		SDL_Surface *levelSurfaceMessage = TTF_RenderText_Solid(this->font, "LEVEL", White);
		SDL_Texture *levelMessage =
		    SDL_CreateTextureFromSurface(renderer, levelSurfaceMessage);
		SDL_RenderCopy(renderer, levelMessage, NULL, &leveltext);

		// Write updated score to screen
		SDL_Surface *displayScore =
		    TTF_RenderText_Solid(this->font, std::to_string(game.score).c_str(), White);
		SDL_Texture *Message2 = SDL_CreateTextureFromSurface(renderer, displayScore);
		SDL_RenderCopy(renderer, Message2, NULL, &livescore);

		// Write updated level to screen
		SDL_Surface *displayLevel =
		    TTF_RenderText_Solid(this->font, std::to_string(game.level).c_str(), White);
		SDL_Texture *liveLevelDisplay =
		    SDL_CreateTextureFromSurface(renderer, displayLevel);
		SDL_RenderCopy(renderer, liveLevelDisplay, NULL, &livelevel);

		if (!this->paused && !this->game.gameover) {
			// Draw the falling tetromino
			for (const auto &loc : game.block.coordinates()) {
				SDL_Rect rect = {
				    .x = loc.x * this->block_size + this->game_offset.x,
				    .y = loc.y * this->block_size + this->game_offset.y,
				    .w = this->block_size,
				    .h = this->block_size,
				};

				SDL_SetRenderDrawColor(this->renderer, game.block.color.r,
						       game.block.color.g, game.block.color.b, 255);
				SDL_RenderFillRect(renderer, &rect);
			}

			// Draw the shadow tetromino
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			auto shadow = game.bottom(nullptr);
			for (const auto &loc : shadow.coordinates()) {
				SDL_Rect rect = {
				    .x = loc.x * this->block_size + this->game_offset.x,
				    .y = loc.y * this->block_size + this->game_offset.y,
				    .w = this->block_size,
				    .h = this->block_size,
				};

				SDL_SetRenderDrawColor(this->renderer, shadow.color.r,
						       shadow.color.g, shadow.color.b, 100);
				SDL_RenderFillRect(renderer, &rect);
			}

			// Draw the filled blocks
			for (const auto &loc : game.filled) {
				SDL_Rect rect = {
				    .x = loc.x * this->block_size + this->game_offset.x,
				    .y = loc.y * this->block_size + this->game_offset.y,
				    .w = this->block_size,
				    .h = this->block_size,
				};
				auto rgb = loc.color;
				SDL_SetRenderDrawColor(this->renderer, rgb.r, rgb.g, rgb.b, 255);
				SDL_RenderFillRect(renderer, &rect);
			}
		}

		// Draw Minigrid
		SDL_Rect mg_back = {
		    .x = rightBorder + (this->block_size / 4),
		    .y = this->block_size * (box_scale * 2 + 1),
		    .w = this->block_size * box_scale,
		    .h = this->block_size * box_scale,
		};
		SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
		SDL_RenderFillRect(renderer, &mg_back);

		for (const auto &loc : this->game.minigrid.block.locations) {

			// Used for when the minigrid does not start in the upper left hand corner
			// Takes the minimum x value and subtracts the lowest possible grid value
			// (3) If the x is greater than 3, the multiplication will result in a
			// number > 0 This will move the block to the upper left corner of the
			// minigrid.
			auto widthModifier = (game.minigrid.block.min_x() - 3) * 2;

			// Converts the width and height of the block into pixels
			auto blockWidth = (1 + widthModifier + game.minigrid.block.max_x() -
					   game.minigrid.block.min_x()) *
					  this->block_size;
			auto blockHeight =
			    (1 + game.minigrid.block.max_y() - game.minigrid.block.min_y()) *
			    this->block_size;

			// Designates the x and y coordinate of the minigrid
			auto preview_x = ((box_scale * this->block_size) - blockWidth) / 2;
			auto preview_y = ((box_scale * this->block_size) - blockHeight) / 2;

			// Creates the rectangle for the minigrid drawing
			SDL_Rect rect = {
			    .x = loc.x * this->block_size + mg_back.x + preview_x,
			    .y = loc.y * this->block_size + mg_back.y + preview_y,
			    .w = this->block_size,
			    .h = this->block_size,
			};

			SDL_SetRenderDrawColor(this->renderer, this->game.minigrid.block.color.r,
					       this->game.minigrid.block.color.g,
					       this->game.minigrid.block.color.b, 255);
			SDL_RenderFillRect(renderer, &rect);
		}
		// End minigrid drawing

		std::string message;
		if (this->game.gameover) {
			Mix_HaltChannel(-1);
			message = "GAME OVER";
		} else if (this->paused) {
			message = "PAUSED";
		} else {
			message = "";
		}

		SDL_Rect status_box = {
		    .x = board.x,
		    .y = board.h / 2 - 100,
		    .w = board.w,
		    .h = 200,
		};
		SDL_Surface *status_surface =
		    TTF_RenderText_Solid(this->font, message.c_str(), {220, 20, 60, 255});
		SDL_Texture *status_message =
		    SDL_CreateTextureFromSurface(renderer, status_surface);
		SDL_RenderCopy(renderer, status_message, NULL, &status_box);

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

int main()
{
	GameContext ctx;

	auto last_time = SDL_GetTicks();

	bool redraw = true;
	bool should_continue = true;
	bool rotation_pressed = false;

	SDL_Event event;
	while (should_continue) {
		SDL_PollEvent(&event);

		if (ctx.paused) {
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

			std::cout << "width:" << 1 + ctx.game.block.max_x() - ctx.game.block.min_x()
				  << std::endl;
			std::cout << "height:"
				  << 1 + ctx.game.block.max_y() - ctx.game.block.min_y()
				  << std::endl;

			std::cout << "min x:" << ctx.game.minigrid.block.min_x() << std::endl;
			std::cout << "min y:" << ctx.game.minigrid.block.min_y() << std::endl;

			std::cout << "max x:" << ctx.game.minigrid.block.max_x() << std::endl;
			std::cout << "max y:" << ctx.game.minigrid.block.max_y() << std::endl;

			if (!ctx.game.gameover) {
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
				case SDLK_SPACE:
					ctx.game.drop();
					break;
				default:
					break;
				}
			}
			// Unconditional keypresses
			switch (event.key.keysym.sym) {
			case SDLK_r: {
				GameState g;
				ctx.game = g;
				Mix_PlayChannel(-1, ctx.music, -1);
			} break;
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
				ctx.paused = true;
				redraw = true;
				Mix_Pause(-1);
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				ctx.paused = false;
				redraw = true;
				Mix_Resume(-1);
				break;
			case SDL_WINDOWEVENT_RESIZED:
				int w, h;
				SDL_GetWindowSize(ctx.window, &w, &h);
				ctx.width = w;
				ctx.height = h;
				ctx.block_size = double(ctx.height - ctx.game_offset.y) * 0.05;
				ctx.game_offset = {ctx.block_size / 4, ctx.block_size / 4};
				redraw = true;
				break;
			}
		default:
			break;
		}

		if (SDL_GetTicks() - last_time > ctx.game.tickspeed && !ctx.game.gameover) {
			ctx.game.down();
			last_time = SDL_GetTicks();
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
