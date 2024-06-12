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
#define PLAYER_SERVICE_X SCREEN_WIDTH - GOAL_OFFSET - PADDLE_W - BALL_SIZE - 1
#define ROBOT_SERVICE_X PADDLE_W + GOAL_OFFSET + 1

#define MAX_SCORE 20

#define LOGCAT SDL_LOG_CATEGORY_APPLICATION

typedef struct App App;
struct App {
  SDL_Window* window;
  SDL_Renderer* renderer;
  SDL_LogPriority log_priority;
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
  Mix_Chunk* wall_sound;
  Mix_Chunk* paddle_sound;
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
  Mix_Chunk* point_sound;
  bool play_sounds;
  bool running;
  bool idle;
  bool over;
};

/*  ----------------------------------------------------------------------
    Description: initialize SDL systems
    Parameters: none
    Returns: App object containing initialized SDL_Window and SDL_Renderer
    objects.
*/
App* init(void);


/*  ---------------------------------------------------------------------- 
    Description: Toggle SDL logging priority between SDL_LOG_PRIORITY_INFO
    and SDL_LOG_PRIORITY_DEBUG
    Parameters: 
      App* app: pointer to App object
    Returns: none
    ---------------------------------------------------------------------- */
void set_log_priority(App* app);

/*  ---------------------------------------------------------------------- 
    Description: Load WAV sound assets
    Parameters: Game object
    Returns: none
    ---------------------------------------------------------------------- */
void load_sounds(Game* game);

/*  ---------------------------------------------------------------------- 
    Description: load ttf font from specified file at specified size
    Parameters: 
      char* path: path to TTF font file
      int size:   display size of font in pixels
    Returns: TTF_font* pointer to font object
    ---------------------------------------------------------------------- */
TTF_Font* load_font(char* path, int size);


/*  ---------------------------------------------------------------------- 
    Description: Reset Game object to defaults for new game
    Parameters: Game* game - game object
    Returns: none
    ---------------------------------------------------------------------- */
void reset_game(Game* game);

/*  ---------------------------------------------------------------------- 
    Description: Reset specified Paddle object to defaults for given Player
    Parameters: 
      Paddle* paddle: pointer to player paddle
      Player* owner: owner of paddle (either PLAYER or ROBOT), 
      determines x position of paddle on court
    Returns: none
    ---------------------------------------------------------------------- */
void reset_paddle(Paddle* paddle, Player owner);

/*  ---------------------------------------------------------------------- 
    Description: Reset Ball object to defaults. Sets the ball's initial
    dx, dy and 'fudge' to random values within a specified range.
    Parameters:   
      Ball* ball: ball object
      Player server: player serving ball, determines the position and direction
      of the ball for service.
    Returns: none
    ---------------------------------------------------------------------- */
void reset_ball(Ball* ball, Player server);

/*  ---------------------------------------------------------------------- 
    Description: Generate a random 'fudge' value between 0 and 14 
    that affects a paddle's movement speed, either increasing or decreasing 
    the paddle's y axis motion
    Parameters: none
    Returns: int fudge value
    ---------------------------------------------------------------------- */
int get_fudge(void);

/*  ---------------------------------------------------------------------- 
    Description: Handle Up and Down key presses for player paddle motion. 
    Paddle moves as long as key is held down. 
    Parameters: 
      SDL_Event* e: pointer to SDL Event object
      Paddle* paddle: pointer to player paddle object
    Returns: none
    ---------------------------------------------------------------------- */
void handle_input(SDL_Event* e, Paddle* paddle);

/*  ---------------------------------------------------------------------- 
    Description: Updates the paddle oject's dy value to move the paddle 
    toward the game ball's position. The dy value is set to the paddle speed 
    minus the 'fudge' factor, which may speed up or slow down the paddle.
    Called by the Robot paddle when in play state. Called by both player and
    robot paddles when game is in Idle state, to simulate a game.
    Parameters: 
      Ball* ball: the game ball
      Paddle* paddle: the paddle object
    Returns: none
    ---------------------------------------------------------------------- */
void update_player(Ball* ball, Paddle* paddle);

/*  ---------------------------------------------------------------------- 
    Description: Check for collision between the ball and the given paddle.
    See https://www.jeffreythompson.org/collision-detection/rect-rect.php
    for an explanation of collision algorithm.
    If a collision is detected:
      - play the paddle sound
      - flip the ball's dx so it rebounds from the paddle
      - update the ball's 'fudge' factor for the next collision
      - call apply_english() to change the ball's speed or angle of return
    Parameters:
      Ball* ball: pointer to game ball object
      Paddle* paddle: pointer to player paddle
    Returns: none
    ---------------------------------------------------------------------- */
void check_collision(Ball* ball, Paddle* paddle);

/*  ---------------------------------------------------------------------- 
    Description: Applies 'English' to the ball on rebound from the paddle.
    Calculates a value based on which of 5 segments the ball makes contact:
      - For the outermost segments 1 and 5 the ball's dy value is increased by 2.
      - For the inner segments 2 and 4 the ball's dy value is increased by 1.
      - For the middle segment 3 the ball's speed is increased by 10, up to 
        BALL_MAX_SPEED.
    Parameters:
      Ball* ball: pointer to game ball object
      Paddle* paddle: pointer to player paddle
    Returns: none

    'paddle_segment' is only used for display in the stats string

    By a random 1 in 6 chance the function may return early without 
    changing the ball.
    ---------------------------------------------------------------------- */
void apply_english(Ball* ball, Paddle* paddle);

/*  ---------------------------------------------------------------------- 
    Description: Move the paddle up or down within the bounds of the court,
    COURT_OFFSIDE at the top edge, or COURT_HEIGHT at the bottom edge. This
    creates a buffer area at top and bottom of the court (delineated by white
    lines) that allows the ball to occasionally sneak past the paddle. 
    Parameters: 
      Paddle* paddle: pointer to player paddle
    Returns: none
    ---------------------------------------------------------------------- */
void move_paddle(Paddle* paddle);

/*  ---------------------------------------------------------------------- 
    Description: Update the ball's x and y position to move it across the court.
    The new positions are the product of the ball's dx, speed and the time_step
    which adjusts the dx and speed to consitent frame independent motion.
    If the ball's position intersects the court wall, the wall sound is played
    and the ball's dy value is flipped so that it rebounds from the court wall.
    Parameters: 
      Ball* ball: pointer to the game ball object
    Returns: none
    ---------------------------------------------------------------------- */
void move_ball(Ball* ball);

/*  ---------------------------------------------------------------------- 
    Description: Renders the game scores stored in the ScoreBoard object
    Parameters: 
      App* app: pointer the App object
      ScoreBoard* score_board: pointer to the ScoreBoard object
    Returns: none
    ---------------------------------------------------------------------- */
void draw_score(App* app, ScoreBoard* score_board);

/*  ---------------------------------------------------------------------- 
    Description: Renders the game instructions on startup. When the game is
    in game over state, also renders the game winner's name.
    Parameters: 
      App* app: pointer the App object
      Game* game: pointer to the Game object
    Returns: none
    ---------------------------------------------------------------------- */
void draw_instructions(App* app, Game* game);

/*  ---------------------------------------------------------------------- 
    Description: Render the game court lines and net.
    Parameters: 
      App* app: pointer to the App object
    Returns: none
    ---------------------------------------------------------------------- */
void draw_court(App* app);

/*  ---------------------------------------------------------------------- 
    Description: Render game stats in the offcourt area at the bottom of the 
    screen. Stats are only of interest to game developers, so it is rendered
    only when SDL Log priority is higher than SDL_LOG_PRIORITY_DEBUG
    Parameters: 
      App* app: pointer to the App object
      Game* game: pointer to the Game object
    Returns: none
    ---------------------------------------------------------------------- */
void draw_stats(App* app, Game* game);

/*  ---------------------------------------------------------------------- 
    Description: Plays the given sound
    Parameters: 
      Mix_Chunk* sound: pointer to the Mix_Chunk sound object
    Returns: none
    ---------------------------------------------------------------------- */
    void play_sound(Mix_Chunk* sound);

#endif