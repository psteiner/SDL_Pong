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
#define COURT_OFFSIDE 20
#define COURT_HEIGHT SCREEN_HEIGHT - COURT_OFFSIDE
#define SCREEN_FPS_BUF_SIZE 100
#define SCREEN_INSTRUCTIONS_BUF_SIZE 50
#define SCREEN_FPS 60
#define SCREEN_TICKS_PER_FRAME 1000 / SCREEN_FPS

#define BALL_SIZE 10
#define BALL_MIN_SPEED 70
#define BALL_MAX_SPEED 150
#define PADDLE_W 10
#define PADDLE_H 60
#define PADDLE_SPEED 20
#define PADDLE_Y SCREEN_MID_H - PADDLE_H / 2
#define GOAL_OFFSET 15
#define PLAYER_X SCREEN_WIDTH - PADDLE_W - GOAL_OFFSET
#define ROBOT_X GOAL_OFFSET
#define PLAYER_SERVICE_X SCREEN_WIDTH - PADDLE_W - GOAL_OFFSET - PADDLE_W / 2
#define ROBOT_SERVICE_X PADDLE_W + GOAL_OFFSET

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
  bool idle;
  TTF_Font* font;
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

void reset_paddle(Paddle* player);
void reset_paddle(Paddle* player) {
  player->speed = PADDLE_SPEED;
  player->time_step = 0;
  player->dx = 0;
  player->dy = 0;
  player->fudge = 0;
  player->h = PADDLE_H;
  player->w = PADDLE_W;
  player->x = player->owner == PLAYER ? PLAYER_X : ROBOT_X;
  player->y = PADDLE_Y;
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

void change_ball_speed(Ball* ball, Paddle* player);
void change_ball_speed(Ball* ball, Paddle* player) {
  // give the player a chance to change the ball speed
  // if the ball hit in the sweet spot,
  // increase its speed up to BALL_MAX_SPEED, otherwise 
  // reduce its speed down to BALL_MIN_SPEED

  if (get_fudge() % 3) {
    return;
  }

  int sweet_spot_top = player->y + player->h / 3;
  int sweet_spot_bottom = player->y + (player->h / 3) * 2;

  if (ball->y > sweet_spot_top && ball->y < sweet_spot_bottom) {
    // ball is in sweet spot, increase its speed
    if (ball->speed < BALL_MAX_SPEED) {
      ball->speed += 10;
    }
  } else {
    // ball hit the paddle frame, decrease its speed
    if (ball->speed >= BALL_MIN_SPEED) {
      ball->speed -= 10;
    }
  }
}

void check_collision(Ball* ball, Paddle* player);
void check_collision(Ball* ball, Paddle* player) {

  int ball_left = ball->x;
  int ball_right = ball->x + ball->w;
  int ball_top = ball->y;
  int ball_bottom = ball->y + ball->h;

  int paddle_left = player->x;
  int paddle_right = player->x + player->w;
  int paddle_top = player->y;
  int paddle_bottom = player->y + player->h;

  bool collide_player = player->owner == PLAYER &&
    ball_right > paddle_left && 
    ball_right < paddle_right &&
    ball_top > paddle_top && 
    ball_top < paddle_bottom &&
    ball_bottom < paddle_bottom &&
    ball_bottom > paddle_top;

  bool collide_robot = player->owner == ROBOT &&
    ball_left < paddle_right && 
    ball_left > paddle_left &&
    ball_top > paddle_top && 
    ball_top < paddle_bottom &&
    ball_bottom < paddle_bottom &&
    ball_bottom > paddle_top;

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
    // give player a chance to change the ball speed
    change_ball_speed(ball, player);
  }
}

void move_paddle(Paddle* paddle);
void move_paddle(Paddle* paddle) {
  paddle->y += paddle->dy * paddle->speed * paddle->time_step;
  if (paddle->y < COURT_OFFSIDE) {
    paddle->y = COURT_OFFSIDE;
  }
  if (paddle->y + paddle->h > COURT_HEIGHT) {
    paddle->y = COURT_HEIGHT - paddle->h;
  }
}

void reset_ball(Ball* ball);
void reset_ball(Ball* ball) {
  ball->service = ROBOT;
  ball->speed = BALL_MIN_SPEED;
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

  // rand() % 4:     0 to 3
  // rand() % 4 + 2: 2 to 5
  ball->dx = rand() % 4 + 2;
  if (ball->service == PLAYER) {
    ball->dx *= -1;
  }

  ball->fudge = get_fudge();

  ball->time_step = 0;
  ball->x = ball->service == ROBOT ? ROBOT_SERVICE_X : PLAYER_SERVICE_X;
  ball->y = SCREEN_MID_H - BALL_SIZE / 2;
  ball->h = BALL_SIZE;
  ball->w = BALL_SIZE;
}

void move_ball(Ball* ball);
void move_ball(Ball* ball) {
  ball->x += ball->dx * ball->speed * ball->time_step;
  ball->y += ball->dy * ball->speed * ball->time_step;
  // bounce off top and bottom
  if (ball->y < 0 || ball->y + ball->h > SCREEN_HEIGHT) {
    ball->dy *= -1;
  }
}

void draw_score(App* app, ScoreBoard* score_board);
void draw_score(App* app, ScoreBoard* score_board) {
  char score_text[SCREEN_FPS_BUF_SIZE];
  SDL_Color score_color = { .r = 255, .g = 255, .b = 255, .a = 255 };

  snprintf(score_text, SCREEN_FPS_BUF_SIZE,
    "%.2d   %.2d", score_board->robot, score_board->player);
  SDL_Surface* score_surface =
    TTF_RenderText_Blended(score_board->font, score_text, score_color);
  SDL_Texture* fps_texture =
    SDL_CreateTextureFromSurface(app->renderer, score_surface);

  int w = 0;
  int h = 0;

  TTF_SizeUTF8(score_board->font, score_text, &w, &h);
  SDL_FreeSurface(score_surface);
  score_surface = NULL;

  int text_x = (SCREEN_WIDTH - w) / 2;

  SDL_Rect renderQuad = { text_x, 10, w, h };
  SDL_RenderCopyEx(
    app->renderer, fps_texture, NULL, &renderQuad, 0, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(fps_texture);
  fps_texture = NULL;
}

void draw_instructions(App* app, TTF_Font* font);
void draw_instructions(App* app, TTF_Font* font) {

  TTF_SetFontSize(font, 36);
  char instruction_text[SCREEN_INSTRUCTIONS_BUF_SIZE];
  SDL_Color score_color = { .r = 255, .g = 255, .b = 255, .a = 255 };

  snprintf(instruction_text,
    SCREEN_INSTRUCTIONS_BUF_SIZE,
    "PRESS SPACEBAR TO BEGIN\nR TO RESET\nESCAPE TO QUIT");

  SDL_Surface* instruction_surface =
    TTF_RenderUTF8_Blended_Wrapped(font, instruction_text, score_color, 0);
  SDL_Texture* instruction_texture =
    SDL_CreateTextureFromSurface(app->renderer, instruction_surface);

  int w = instruction_surface->w;
  int h = instruction_surface->h;

  // TTF_SizeUTF8(font, instruction_text, &w, &h);
  SDL_FreeSurface(instruction_surface);
  instruction_surface = NULL;

  int text_x = (SCREEN_WIDTH - w) / 2;
  int text_y = SCREEN_MID_H - h / 2;

  SDL_Rect renderQuad = { text_x, text_y, w, h };
  SDL_RenderCopyEx(
    app->renderer, instruction_texture, NULL, &renderQuad, 0, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(instruction_texture);
  instruction_texture = NULL;
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
    "Avg FPS:%2.f Ball [dx:%2.f dy:%2.f] [x:%4.f y:%4.f] fudge:%2d, speed: %d",
    avg_fps, ball->dx, ball->dy, ball->x, ball->y, ball->fudge, ball->speed);

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

  TTF_Font* fps_font =
    TTF_OpenFont("../assets/Inconsolata-Regular.ttf", 14);
  if (fps_font == NULL) {
    SDL_LogError(LOGCAT,
      "Failed to load font! SDL_ttf Error: %s\n",
      TTF_GetError());
  }

  Uint32 cap_ticks = 0;
  Uint32 frame_ticks = 0;

  ScoreBoard score_board = { .idle = true, .player = 18, .robot = 18 };

  TTF_Font* score_font =
    TTF_OpenFont("../assets/VT323-Regular.ttf", 40);
  if (fps_font == NULL) {
    SDL_LogError(LOGCAT,
      "Failed to load font! SDL_ttf Error: %s\n",
      TTF_GetError());
  }

  score_board.font = score_font;

  Ball ball = { 0 };
  reset_ball(&ball);
  // ball.speed = BALL_MIN_SPEED;

  Paddle player = { 0 };
  player.owner = PLAYER;
  reset_paddle(&player);

  Paddle robot = { 0 };
  robot.owner = ROBOT;
  reset_paddle(&robot);

  SDL_Event e;
  bool running = true;
  bool game_over = false;
  bool game_idle = true;
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
        case SDLK_SPACE:
          score_board.player = 0;
          score_board.robot = 0;
          reset_paddle(&player);
          reset_paddle(&robot);
          reset_ball(&ball);
          game_idle = false;
          break;
        case SDLK_r:
          game_over = true;
          game_idle = true;
          break;
        default:
          break;
        }
      }
      if (game_idle == false) {
        handle_input(&e, &player);
      }
    }

    // reset game
    if (game_over) {
      score_board.player = 0;
      score_board.robot = 0;
      reset_paddle(&player);
      reset_paddle(&robot);
      reset_ball(&ball);
      // ball.speed = BALL_MIN_SPEED;
      game_over = false;
      game_idle = true;
    }

    if (game_idle) {
      update_player(&ball, &player);
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
      if (score_board.player >= MAX_SCORE) {
        game_over = true;
      } else {
        reset_ball(&ball);
        // update defaults
        ball.service = PLAYER;
        ball.x = PLAYER_SERVICE_X;
        ball.y = player.y + player.h / 2;
      }
    }

    if (ball.x > SCREEN_WIDTH) {
      // Robot scored
      score_board.robot++;
      if (score_board.robot >= MAX_SCORE) {
        game_over = true;
      } else {
        reset_ball(&ball);
        // give the robot a little advantage...
        // ball.speed = ball.speed < BALL_MAX_SPEED ? ball.speed + 5 : BALL_MAX_SPEED;
        // update defaults
        ball.service = ROBOT;
        ball.x = ROBOT_SERVICE_X;
        ball.y = robot.y + player.h / 2;
      }
    }

    //Clear screen
    SDL_SetRenderDrawColor(app->renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(app->renderer);

    draw_net(app);
    draw_score(app, &score_board);
    if (game_idle) {
      draw_instructions(app, score_font);
    }

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
  TTF_CloseFont(score_board.font);
  SDL_DestroyRenderer(app->renderer);
  SDL_DestroyWindow(app->window);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
  return EXIT_SUCCESS;

}