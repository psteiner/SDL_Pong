#ifndef PONG_H
#define PONG_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_MID_W SCREEN_WIDTH / 2
#define SCREEN_MID_H SCREEN_HEIGHT / 2

const int SCREEN_FPS = 60;
const int SCREEN_TICK_PER_FRAME = 1000 / SCREEN_FPS;

#define LOGCAT SDL_LOG_CATEGORY_APPLICATION

typedef struct App App;
struct App {
  SDL_Window* window;
  SDL_Renderer* renderer;
};

App* init(void);

#endif