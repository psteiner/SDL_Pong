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
#define SCREEN_FPS 60
#define SCREEN_TICKS_PER_FRAME 1000 / SCREEN_FPS

#define BALL_SIZE 10
#define BALL_MIN_SPEED 70
#define BALL_MAX_SPEED 150
#define PADDLE_W 10
#define PADDLE_H 60
#define PADDLE_SPEED 20
#define GOAL_OFFSET 15

#define MAX_SCORE 20

#define LOGCAT SDL_LOG_CATEGORY_APPLICATION

typedef struct App App;
struct App {
  SDL_Window* window;
  SDL_Renderer* renderer;
};

typedef enum {
  PLAYER,
  ROBOT
} Player;

typedef struct Ball Ball;
struct Ball {
  int speed;
  double dx;
  double dy;
  double x;
  double y;
  int fudge;
  int h;
  int w;
  double time_step;
  Player service;
};

typedef struct Paddle Paddle;
struct Paddle {
  Player owner;
  int speed;
  double time_step;
  double dx;
  double dy;
  int fudge;
  int h;
  int w;
  double x;
  double y;
};

typedef struct ScoreBoard ScoreBoard;
struct ScoreBoard {
  int player;
  int robot;
};

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


int get_fudge(void);
int get_fudge(void) {
  return rand() % 15;
}

void update_player(Ball* ball, Paddle* player);
void update_player(Ball* ball, Paddle* player) {
  int paddle_top = player->y;
  int paddle_bottom = player->y + player->h;

  int ball_top = ball->y;
  int ball_bottom = ball->y + ball->h;
  player->dy = 0;

  // move paddle up
  if (ball_top < paddle_top) {
    player->dy -= player->speed - ball->fudge;
  }

  // move paddle down
  if (ball_bottom > paddle_bottom) {
    player->dy += player->speed - ball->fudge;
  }
}


void check_collision(Ball* ball, Paddle* player);
void check_collision(Ball* ball, Paddle* player) {

  int ball_left = ball->x;
  int ball_right = ball->x + ball->w;
  int ball_top = ball->y;

  int paddle_left = player->x;
  int paddle_right = player->x + player->w;
  int paddle_top = player->y;
  int paddle_bottom = player->y + player->h;

  bool collide_player = player->owner == PLAYER &&
    ball_right > paddle_left && ball_right < paddle_right &&
    ball_top > paddle_top && ball_top < paddle_bottom;

  bool collide_robot = player->owner == ROBOT &&
    ball_left < paddle_right && ball_left > paddle_left &&
    ball_top > paddle_top && ball_top < paddle_bottom;

  // shift the ball outside the paddle to prevent trapping it 
  // inside the paddle
  if (collide_player) {
    ball->x = paddle_left - ball->w;
  }
  if (collide_robot) {
    ball->x = paddle_right + ball->w;
  }

  // bounce the ball off the paddle
  if (collide_player || collide_robot) {
    ball->dx = -ball->dx;
    ball->fudge = get_fudge();
  }
}

void move_paddle(Paddle* paddle);
void move_paddle(Paddle* paddle) {
  paddle->y += paddle->dy * paddle->speed * paddle->time_step;
  if (paddle->y < 0) {
    paddle->y = 0;
  }
  if (paddle->y + paddle->h > SCREEN_HEIGHT) {
    paddle->y = SCREEN_HEIGHT - paddle->h;
  }
}

void serve_ball(Ball* ball);
void serve_ball(Ball* ball) {
  ball->service = ROBOT;
  ball->x = 0;
  ball->y = 0;

  /*
    Service angle adjustment
    a % b:	the remainder of a divided by b
    rand() % n      ->  0 to n-1

    rand() % 7      ->  0 to 6
    rand() % 7 - 3  -> -3 to 3 
    rand() % 9 - 4  -> -4 to 4
  */

  do {
    ball->dy = rand() % 7 - 3;
  } while (ball->dy == 0);

  // rand() % 4: 0 to 3
  // range 2 to 5
  ball->dx = rand() % 4 + 2;
  if (ball->service == PLAYER) {
    ball->dx -= ball->dx;
  }

  ball->fudge = get_fudge();

  ball->time_step = 0;
  ball->x = SCREEN_MID_W;
  ball->y = SCREEN_MID_H - BALL_SIZE / 2;
  ball->h = BALL_SIZE;
  ball->w = BALL_SIZE;
}

void move_ball(Ball* ball);
void move_ball(Ball* ball) {
  ball->x += ball->dx * ball->speed * ball->time_step;
  ball->y += ball->dy * ball->speed * ball->time_step;
  // bounce off top and bottom
  if (ball->y < 0 || ball->y > SCREEN_HEIGHT) {
    ball->dy *= -1;
  }
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

void draw_stats(App* app, int frame_count, Uint32 fps_ticks, TTF_Font* fps_font, Ball* ball);
void draw_stats(App* app, int frame_count, Uint32 fps_ticks, TTF_Font* fps_font, Ball* ball) {
  char fps_text[SCREEN_FPS_BUF_SIZE];
  SDL_Color fps_color = { .r = 255, .g = 255, .b = 0, .a = 255 };

  double avg_fps = frame_count / ((SDL_GetTicks() - fps_ticks) / 1000.f);

  snprintf(fps_text, SCREEN_FPS_BUF_SIZE, 
    "Avg FPS: %0.f Ball dy:%2.f fudge: %d, speed; %d", 
    avg_fps, ball->dy, ball->fudge, ball->speed);

  SDL_Surface* fps_surface =
    TTF_RenderText_Blended(fps_font, fps_text, fps_color);
  SDL_Texture* fps_texture =
    SDL_CreateTextureFromSurface(app->renderer, fps_surface);
  int fps_w = fps_surface->w;
  int fps_h = fps_surface->h;
  SDL_FreeSurface(fps_surface);
  fps_surface = NULL;

  SDL_Rect renderQuad = { 10, 460, fps_w, fps_h };
  SDL_RenderCopyEx(
    app->renderer, fps_texture, NULL, &renderQuad, 0, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(fps_texture);
  fps_texture = NULL;
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

  Uint32 cap_ticks = 0;
  Uint32 frame_ticks = 0;

  ScoreBoard score_board = { .player = 0, .robot = 0 };

  Ball ball = { 0 };
  serve_ball(&ball);
  ball.speed = BALL_MIN_SPEED;

  Paddle player = {
    .owner = PLAYER,
    .speed = PADDLE_SPEED,
    .time_step = 0,
    .dx = 0,
    .dy = 0,
    .fudge = 0,
    .h = PADDLE_H,
    .w = PADDLE_W,
    .x = SCREEN_WIDTH - PADDLE_W - GOAL_OFFSET,
    .y = SCREEN_MID_H - PADDLE_H / 2
  };

  Paddle robot = {
    .owner = ROBOT,
    .speed = PADDLE_SPEED,
    .time_step = 0,
    .dx = 0,
    .dy = 0,
    .fudge = 0,
    .h = PADDLE_H,
    .w = PADDLE_W,
    .x = GOAL_OFFSET,
    .y = SCREEN_MID_H - PADDLE_H / 2
  };

  SDL_Event e;
  bool running = true;
  int frame_count = 0;
  Uint32 fps_ticks = SDL_GetTicks();
  Uint32 step_ticks = 0;

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

    double time_step = (SDL_GetTicks() - step_ticks) / 1000.f;
    ball.time_step = time_step;
    player.time_step = time_step;
    robot.time_step = time_step;

    move_paddle(&player);
    move_paddle(&robot);

    // move ball
    move_ball(&ball);

    step_ticks = SDL_GetTicks();

    // check for score
    if (ball.x < 0) {
      // Player scored
      score_board.player++;
      serve_ball(&ball);
      ball.service = PLAYER;
    }
    if (ball.x > SCREEN_WIDTH) {
      // Robot scored
      score_board.robot++;
      serve_ball(&ball);
      ball.service = ROBOT;
      ball.speed = ball.speed < BALL_MAX_SPEED ? ball.speed + 5 : BALL_MAX_SPEED;
    }

    //Clear screen
    SDL_SetRenderDrawColor(app->renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(app->renderer);

    draw_net(app);
    draw_score(&score_board);

    // draw player paddle
    SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_Rect player_rect =
    { .h = player.h, .w = player.w, .x = player.x, .y = player.y };

    // draw robot paddle
    SDL_RenderFillRect(app->renderer, &player_rect);
    SDL_Rect robot_rect =
    { .h = robot.h, .w = robot.w, .x = robot.x, .y = robot.y };
    SDL_RenderFillRect(app->renderer, &robot_rect);

    // draw ball
    SDL_Rect ball_rect =
    { .h = ball.h, .w = ball.w, .x = ball.x, .y = ball.y };
    SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRect(app->renderer, &ball_rect);

    draw_stats(app, frame_count, fps_ticks, fps_font, &ball);

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
  SDL_DestroyRenderer(app->renderer);
  SDL_DestroyWindow(app->window);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
  return EXIT_SUCCESS;

}