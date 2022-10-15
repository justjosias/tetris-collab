NAME=TETRIS
CPP=g++
CPPFLAGS=-Wall -Wextra -Werror -g
LDFLAGS=`pkg-config --cflags --libs sdl2 SDL2_ttf SDL2_mixer SDL2_image`

CPPFILES=main.cpp

WASMFLAGS=-s ALLOW_MEMORY_GROWTH=1
WASMLIBS=-s USE_SDL=2 -s USE_SDL_MIXER=2 -s USE_SDL_TTF=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]'
ASSETS=assets/

build:
	$(CPP) $(CPPFLAGS) -o $(NAME) $(CPPFILES) $(LDFLAGS)

wasm: $(CPPFILES)
	mkdir -p dist
	em++ $^ -o dist/$(NAME).js -g -lm --bind $(WASMFLAGS) $(WASMLIBS) --preload-file $(ASSETS) --use-preload-plugins
	cp main.html dist/$(NAME).html

wasm-run: wasm
	emrun dist/$(NAME).html

clean:
	$(RM) $(NAME)*
	$(RM) -r dist/
