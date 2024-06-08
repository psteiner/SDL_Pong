// SDL2 Pong Game
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
#define SCREEN_FPS_BUF_SIZE 50
#define SCREEN_FPS 30
#define SCREEN_TICKS_PER_FRAME 1000 / SCREEN_FPS

#define BALL_SIZE 10
#define PADDLE_W 10
#define PADDLE_H 60
#define PADDLE_SPEED 5
#define GOAL_OFFSET 15

#define MAX_SCORE 20

#define LOGCAT SDL_LOG_CATEGORY_APPLICATION

typedef struct App App;
struct App {
  SDL_Window* window;
  SDL_Renderer* renderer;
};

typedef struct Ball Ball;
struct Ball {
  int speed;
  int dx;
  int dy;
  int fudge;
  SDL_Rect ball;
};

typedef enum {
  PLAYER,
  ROBOT
} Paddle_Owner;

typedef struct Paddle Paddle;
struct Paddle {
  Paddle_Owner owner;
  int speed;
  int dx;
  int dy;
  SDL_Rect paddle;
};

typedef struct ScoreBoard ScoreBoard;
struct ScoreBoard {
  int player;
  int robot;
};

SDL_Texture* g_fps_texture;

App* init(void);

App* init(void) {

  srand(time(0));
  //  SDL_LOG_PRIORITY_INFO
  SDL_LogSetPriority(LOGCAT, SDL_LOG_PRIORITY_DEBUG);

  App* app = malloc(sizeof(App));

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    fprintf(stderr, "Could not initialize SDL2: %s\n", SDL_GetError());
  }
  //Set texture filtering to linear
  if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
    SDL_LogWarn(LOGCAT, "Linear texture filtering not enabled!");
  }

  app->window = SDL_CreateWindow("SDL Pong",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

  if (app->window == NULL) {
    SDL_LogError(LOGCAT, "Could not create window: %s\n", SDL_GetError());
  }

  app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_PRESENTVSYNC);
  if (app->renderer == NULL) {
    SDL_LogCritical(LOGCAT,
      "Renderer could not be created! SDL Error: %s\n",
      SDL_GetError());
  }
  SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);

  //Initialize PNG loading
  int imgFlags = IMG_INIT_PNG;
  if (!(IMG_Init(imgFlags) & imgFlags)) {
    SDL_LogCritical(LOGCAT,
      "SDL_image could not initialize! SDL_image Error: %s\n",
      IMG_GetError());
  }

  //Initialize SDL_ttf
  if (TTF_Init() == -1) {
    SDL_LogCritical(LOGCAT,
      "SDL_ttf could not initialize! SDL_ttf Error: %s\n",
      TTF_GetError());
  }

  //Initialize SDL_mixer
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    SDL_LogCritical(LOGCAT,
      "SDL_mixer could not initialize! SDL_mixer Error: %s\n",
      Mix_GetError());
  }

  return app;
}

void move_ball(Ball* ball);
void move_ball(Ball* ball) {
  ball->ball.x += ball->dx * ball->speed;
  ball->ball.y += ball->dy * ball->speed;
  // bounce off top and bottom
  if (ball->ball.y < 0 || ball->ball.y > SCREEN_HEIGHT) {
    ball->dy *= -1;
  }
}

void handle_input(SDL_Event* e, Paddle* player);
void handle_input(SDL_Event* e, Paddle* player) {
  if (e->type == SDL_KEYDOWN && e->key.repeat == 0) {
    // Move the sprite as long as the key is down
    switch (e->key.keysym.sym) {
    case SDLK_UP:
      player->dy -= player->speed;
      break;
    case SDLK_DOWN:
      player->dy += player->speed;
      break;
    }
  }
  if (e->type == SDL_KEYUP && e->key.repeat == 0) {
    // Stop moving once the key is released
    switch (e->key.keysym.sym) {
    case SDLK_UP:
      player->dy += player->speed;
      break;
    case SDLK_DOWN:
      player->dy -= player->speed;
      break;
    }
  }
}

void update_player(Ball* ball, Paddle* player);
void update_player(Ball* ball, Paddle* player) {
  int paddle_top = player->paddle.y;
  int paddle_bottom = player->paddle.y + player->paddle.h;

  int ball_top = ball->ball.y;
  int ball_bottom = ball->ball.y + ball->ball.h;
  player->dy = 0;

  if (ball_top < paddle_top) {
    player->dy -= player->speed + ball->fudge;
  }

  if (ball_bottom > paddle_bottom) {
    player->dy += player->speed + ball->fudge;
  }
}

void check_collision(Ball* ball, Paddle* player);
void check_collision(Ball* ball, Paddle* player) {

  int ball_left = ball->ball.x;
  int ball_right = ball->ball.x + ball->ball.w;
  int ball_top = ball->ball.y;
  // int ball_bottom = ball->ball.y + ball->ball.h;

  int paddle_left = player->paddle.x;
  int paddle_right = player->paddle.x + player->paddle.w;
  int paddle_top = player->paddle.y;
  int paddle_bottom = player->paddle.y + player->paddle.h;

  bool collide_player = player->owner == PLAYER &&
    ball_right > paddle_left && ball_right < paddle_right &&
    ball_top > paddle_top && ball_top < paddle_bottom;

  bool collide_robot = player->owner == ROBOT &&
    ball_left < paddle_right && ball_left > paddle_left &&
    ball_top > paddle_top && ball_top < paddle_bottom;

  // shift the ball outside the paddle to prevent trapping it 
  // inside the paddle
  if (collide_player) {
    ball->ball.x = paddle_left - ball->ball.w;
  }
  if (collide_robot) {
    ball->ball.x = paddle_right + ball->ball.w;
  }

  if (collide_player || collide_robot) {
    ball->dx = -ball->dx;
  }
}

void move_paddle(Paddle* paddle);
void move_paddle(Paddle* paddle) {
  paddle->paddle.y += paddle->dy;
  if (paddle->paddle.y < 0) {
    paddle->paddle.y = 0;
  }
  if (paddle->paddle.y + paddle->paddle.h > SCREEN_HEIGHT) {
    paddle->paddle.y = SCREEN_HEIGHT - paddle->paddle.h;
  }
}

void serve_ball(Ball* ball);
void serve_ball(Ball* ball) {
  ball->speed = 1;

  do {
    ball->fudge = rand() % 4 - 4;
  } while (ball->fudge == 0);

  SDL_LogDebug(LOGCAT, "fudge: %i\n", ball->fudge);

  do {
    ball->dy = rand() % 6 - 2;
  } while (ball->dy == 0);
  ball->dy = 2;
  // ball->dx = rand() % 4 + 2;
  ball->dx = 2;
  ball->ball.x = SCREEN_MID_W;
  ball->ball.y = SCREEN_MID_H - BALL_SIZE / 2;
  ball->ball.h = BALL_SIZE;
  ball->ball.w = BALL_SIZE;
  SDL_LogDebug(LOGCAT, "Ball dx: %d dy: %d\n", ball->dx, ball->dy);
}

void draw_score(ScoreBoard* score_board);
void draw_score(ScoreBoard* score_board) {

}

void draw_net(App* app);
void draw_net(App* app) {
  SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_Rect line = { .w = 3, .h = 12, .x = SCREEN_MID_W - 1 };
  int gap = 16;
  for (line.y = gap; line.y < SCREEN_HEIGHT - gap; line.y += gap) {
    SDL_RenderFillRect(app->renderer, &line);
  }
}

void draw_stats(App* app, int frame_count, Uint32 fps_ticks, TTF_Font* fps_font);
void draw_stats(App* app, int frame_count, Uint32 fps_ticks, TTF_Font* fps_font) {
  char fps_text[SCREEN_FPS_BUF_SIZE];
  SDL_Color fps_color = { .r = 255, .g = 0, .b = 0, .a = 255 };

  double avg_fps = frame_count / ((SDL_GetTicks() - fps_ticks) / 1000.f);

  snprintf(fps_text, SCREEN_FPS_BUF_SIZE, "Avg FPS: %0.0f", avg_fps);
  SDL_Surface* fps_surface = TTF_RenderText_Blended(fps_font, fps_text, fps_color);
  g_fps_texture = SDL_CreateTextureFromSurface(app->renderer, fps_surface);
  int fps_w = fps_surface->w;
  int fps_h = fps_surface->h;
  SDL_FreeSurface(fps_surface);
  fps_surface = NULL;

  SDL_Rect renderQuad = { 10, 460, fps_w, fps_h };

  SDL_RenderCopyEx(
    app->renderer, g_fps_texture, NULL, &renderQuad, 0, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(g_fps_texture);
  g_fps_texture = NULL;

}


int main(int argc, char* argv[]) {
  (void)argc, (void)argv;

  App* app = init();

  if (app == NULL) {
    SDL_LogCritical(LOGCAT, "App init failed!");
    return EXIT_FAILURE;
  }

  //Open the font
  TTF_Font* fps_font =
    TTF_OpenFont("../assets/Inconsolata-Regular.ttf", 14);
  if (fps_font == NULL) {
    SDL_LogError(LOGCAT,
      "Failed to load font! SDL_ttf Error: %s\n",
      TTF_GetError());
  }
  Uint32 fps_ticks = 0;
  int frame_count = 0;

  Uint32 cap_ticks = 0;
  Uint32 frame_ticks = 0;

  ScoreBoard score_board = { .player = 0, .robot = 0 };

  Ball ball = { 0 };
  serve_ball(&ball);

  Paddle player = {
    .owner = PLAYER,
    .dx = 0,
    .dy = 0,
    .speed = PADDLE_SPEED,
    .paddle = {
      .h = PADDLE_H,
      .w = PADDLE_W,
      .x = SCREEN_WIDTH - PADDLE_W - GOAL_OFFSET,
      .y = SCREEN_MID_H - PADDLE_H / 2
    }
  };

  Paddle robot = {
    .owner = ROBOT,
    .dx = 0,
    .dy = 0,
    .speed = PADDLE_SPEED,
    .paddle = {
      .h = PADDLE_H,
      .w = PADDLE_W,
      .x = GOAL_OFFSET,
      .y = SCREEN_MID_H - PADDLE_H / 2
    }
  };

  SDL_Event e;
  bool running = true;
  fps_ticks = SDL_GetTicks();

  while (running) {
    cap_ticks = SDL_GetTicks();

    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        running = false;
      }
      if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
          running = false;
          break;
        default:
          break;
        }
      }
      handle_input(&e, &player);
    }

    update_player(&ball, &robot);

    check_collision(&ball, &player);
    check_collision(&ball, &robot);

    move_paddle(&player);
    move_paddle(&robot);

    // move ball
    move_ball(&ball);

    // check for score
    if (ball.ball.x < 0) {
      // Player scored
      score_board.player++;
      serve_ball(&ball);
    }
    if (ball.ball.x > SCREEN_WIDTH) {
      // Robot scored
      score_board.robot++;
      serve_ball(&ball);
    }

    //Clear screen
    SDL_SetRenderDrawColor(app->renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(app->renderer);

    draw_net(app);
    draw_score(&score_board);

    // draw paddles
    SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(app->renderer, &player.paddle);
    SDL_RenderFillRect(app->renderer, &robot.paddle);

    // draw ball
    SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(app->renderer, &ball.ball);

    draw_stats(app, frame_count, fps_ticks, fps_font);

    // Update screen
    SDL_RenderPresent(app->renderer);
    ++frame_count;

    // Cap frame rate
    frame_ticks = SDL_GetTicks() - cap_ticks;
    if (frame_ticks < SCREEN_TICKS_PER_FRAME) {
      SDL_Delay(SCREEN_TICKS_PER_FRAME - frame_ticks);
    }
  }

  TTF_CloseFont(fps_font);
  SDL_DestroyTexture(g_fps_texture);
  SDL_DestroyRenderer(app->renderer);
  SDL_DestroyWindow(app->window);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
  return EXIT_SUCCESS;

}