// bare bones minimal SDL2 program
// based on https://gist.github.com/transmutrix/8c8299e7322cdf5acb2225c5cf4c9e03
/* Displays a window that changes color until you close it. */

#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <SDL.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

int main(int arc, char* argv[]) {
  SDL_Event e;
  SDL_Window* window = NULL;
  SDL_Surface* screen_surf = NULL;
  int running = true;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "Could not initialize SDL2: %s\n", SDL_GetError());
  }

  window = SDL_CreateWindow("Hello World",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    SCREEN_WIDTH, SCREEN_HEIGHT, 0);

  if (window == NULL) {
    fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
    return 1;
  }

  screen_surf = SDL_GetWindowSurface(window);

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

    const double time = SDL_GetTicks() / 1000.0;

    SDL_FillRect(screen_surf, NULL, SDL_MapRGB(screen_surf->format,
      (int)((sin(time * M_PI * 0.25) + 1) / 2 * 0xff),
      (int)((sin((time + M_PI) * M_PI * .25) + 1) / 2 * 0xff),
      (int)((sin((time + M_PI / 2) * M_PI * .25) + 1) / 2 * 0xff)));

    SDL_UpdateWindowSurface(window);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}