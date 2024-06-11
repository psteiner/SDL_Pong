#ifndef PONG_H
#define PONG_H

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <math.h>
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
#define SCREEN_INSTRUCTIONS_BUF_SIZE 100
#define SCREEN_FPS 60
#define SCREEN_TICKS_PER_FRAME 1000 / SCREEN_FPS

#define BALL_SIZE 10
#define BALL_MIN_SPEED 70
#define BALL_MAX_SPEED 120
#define PADDLE_W 10
#define PADDLE_H 60
#define PADDLE_SPEED 20
#define PADDLE_Y SCREEN_MID_H - PADDLE_H / 2
#define GOAL_OFFSET 15
#define PLAYER_X SCREEN_WIDTH - PADDLE_W - GOAL_OFFSET
#define ROBOT_X GOAL_OFFSET
//                        | -1 |  BS  |   PW   |   GO   ||<- Screen Width
#define PLAYER_SERVICE_X (SCREEN_WIDTH - GOAL_OFFSET - PADDLE_W - BALL_SIZE) - 1
#define ROBOT_SERVICE_X (PADDLE_W + GOAL_OFFSET) + 1

#define MAX_SCORE 20

#define LOGCAT SDL_LOG_CATEGORY_APPLICATION

typedef struct App App;
struct App {
  SDL_Window* window;
  SDL_Renderer* renderer;
};

typedef enum {
  NOBODY,
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
  int paddle_segment;
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
  TTF_Font* font;
};

typedef struct Game Game;
struct Game {
  ScoreBoard score_board;
  Player winner;
  Paddle player;
  Paddle robot;
  Ball ball;
  int frame_count;
  Uint32 frame_ticks;
  Uint32 cap_ticks;
  Uint32 step_ticks;
  Uint32 fps_ticks;
  TTF_Font* stats_font;
  bool running;
  bool idle;
  bool over;
};

App* init(void);
void reset_game(Game* game);
void handle_input(SDL_Event* e, Paddle* paddle);
void reset_paddle(Paddle* paddle);
int get_fudge(void);
void update_player(Ball* ball, Paddle* paddle);
void apply_english(Ball* ball, Paddle* paddle);
void check_collision(Ball* ball, Paddle* paddle);
void move_paddle(Paddle* paddle);
void reset_ball(Ball* ball);
void move_ball(Ball* ball);
void draw_score(App* app, ScoreBoard* score_board);
void draw_instructions(App* app, Game* game);
void draw_court(App* app);
void draw_stats(App* app, Game* game);

#endif