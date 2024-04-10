// SDL2 Pong Game

#include "pong.h"

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

typedef struct Ball Ball;
struct Ball {
  int x;
  int y;
};

int main(int argc, char* argv[argc+1]) {
  SDL_Event e;
  SDL_Window* window = NULL;
  // SDL_Surface* screen_surf = NULL;
  bool running = true;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Could not initialize SDL2: %s\n", SDL_GetError());
  }

  window = SDL_CreateWindow("SDL Pong",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    SCREEN_WIDTH, SCREEN_HEIGHT, 0);

  if (window == NULL) {
    fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
    return 1;
  }

  // screen_surf = SDL_GetWindowSurface(window);

  while (running) {
    
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        running = false;
      }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);
    if (key_state[SDL_SCANCODE_ESCAPE]) {
      running = false;
    }

    SDL_UpdateWindowSurface(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
  return EXIT_SUCCESS;
}