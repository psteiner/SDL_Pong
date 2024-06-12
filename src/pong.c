// SDL2 Pong Game
#include "pong.h"

/*  ----------------------------------------------------------------------
    Description: initialize SDL systems and set logging level.
    Parameters: none
    Returns: App object containing initialized SDL_Window and SDL_Renderer
    objects.
*/
App* init(void) {

  srand(time(0));

  App* app = malloc(sizeof(App));

  app->log_priority = SDL_LOG_PRIORITY_INFO;
   
  SDL_LogSetPriority(LOGCAT, app->log_priority);

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

/*  ---------------------------------------------------------------------- 
    Description: Toggle SDL logging priority between SDL_LOG_PRIORITY_INFO
    and SDL_LOG_PRIORITY_DEBUG
    Parameters: 
      App* app: pointer to App object
    Returns: none
    ---------------------------------------------------------------------- */
void set_log_priority(App* app) {
  if (app->log_priority == SDL_LOG_PRIORITY_INFO) {
    app->log_priority = SDL_LOG_PRIORITY_DEBUG;
  } else {
    app->log_priority = SDL_LOG_PRIORITY_INFO;
  }
  SDL_LogSetPriority(LOGCAT, app->log_priority);
}

/*  ---------------------------------------------------------------------- 
    Description: Load WAV sound assets
    Parameters: Game object
    Returns: none
    ---------------------------------------------------------------------- */
void load_sounds(Game* game) {
  game->ball.paddle_sound = Mix_LoadWAV("../assets/paddle.wav");
  if (game->ball.paddle_sound == NULL) {
    SDL_LogError(LOGCAT, "Failed to load paddle.wav");
  }
  game->ball.wall_sound = Mix_LoadWAV("../assets/wall.wav");
  if (game->ball.wall_sound == NULL) {
    SDL_LogError(LOGCAT, "Failed to load wall.wav");
  }
  game->point_sound = Mix_LoadWAV("../assets/point.wav");
  if (game->point_sound == NULL) {
    SDL_LogError(LOGCAT, "Failed to load point.wav");
  }
}

/*  ---------------------------------------------------------------------- 
    Description: load ttf font from specified file at specified size
    Parameters: 
      char* path: path to TTF font file
      int size:   display size of font in pixels
    Returns: TTF_font* pointer to font object
    ---------------------------------------------------------------------- */
TTF_Font* load_font(char* path, int size) {
  TTF_Font* font = TTF_OpenFont(path, size);
  if (font == NULL) {
    SDL_LogError(LOGCAT,
      "Failed to load font '%s'! SDL_ttf Error: %s\n",
      path, TTF_GetError());
  }
  return font;
}

/*  ---------------------------------------------------------------------- 
    Description: Reset Game object to defaults for new game
    Parameters: Game* game - game object
    Returns: none
    ---------------------------------------------------------------------- */
void reset_game(Game* game) {
  game->score_board.player = 0;
  game->score_board.robot = 0;
  game->winner = game->over ? game->winner : NOBODY;
  reset_paddle(&game->player, PLAYER);
  reset_paddle(&game->robot, ROBOT);
  reset_ball(&game->ball, ROBOT);
  game->ball.speed = BALL_MIN_SPEED;
  game->idle = true;
  game->over = false;
}

/*  ---------------------------------------------------------------------- 
    Description: Reset specified Paddle object to defaults for given Player
    Parameters: 
      Paddle* paddle: pointer to player paddle
      Player* owner: owner of paddle (either PLAYER or ROBOT), 
      determines x position of paddle on court
    Returns: none
    ---------------------------------------------------------------------- */
void reset_paddle(Paddle* paddle, Player owner) {
  paddle->owner = owner;
  paddle->speed = PADDLE_SPEED;
  paddle->time_step = 0;
  paddle->dx = 0;
  paddle->dy = 0;
  paddle->fudge = 0;
  paddle->h = PADDLE_H;
  paddle->w = PADDLE_W;
  paddle->x = paddle->owner == PLAYER ? PLAYER_X : ROBOT_X;
  paddle->y = PADDLE_Y;
}

/*  ---------------------------------------------------------------------- 
    Description: Reset Ball object to defaults. Sets the ball's initial
    dx, dy and 'fudge' to random values within a specified range.
    Parameters:   
      Ball* ball: ball object
      Player server: player serving ball, determines the position and direction
      of the ball for service.
    Returns: none
    ---------------------------------------------------------------------- */
void reset_ball(Ball* ball, Player server) {
  ball->service = server;
  ball->speed = ball->speed < BALL_MIN_SPEED ? BALL_MIN_SPEED : ball->speed;
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
  ball->paddle_segment = 0;

  ball->time_step = 0;
  ball->x = ball->service == ROBOT ? ROBOT_SERVICE_X : PLAYER_SERVICE_X;
  ball->y = SCREEN_MID_H - BALL_SIZE / 2;
  ball->h = BALL_SIZE;
  ball->w = BALL_SIZE;
}

/*  ---------------------------------------------------------------------- 
    Description: Generate a random 'fudge' value between 0 and 14 
    that affects a paddle's movement speed, either increasing or decreasing 
    the paddle's y axis motion
    Parameters: none
    Returns: int fudge value
    ---------------------------------------------------------------------- */
int get_fudge(void) {
  return rand() % 15;
}

/*  ---------------------------------------------------------------------- 
    Description: Handle Up and Down key presses for player paddle motion. 
    Paddle moves as long as key is held down. 
    Parameters: 
      SDL_Event* e: pointer to SDL Event object
      Paddle* paddle: pointer to player paddle object
    Returns: none
    ---------------------------------------------------------------------- */
void handle_input(SDL_Event* e, Paddle* paddle) {
  if (e->type == SDL_KEYDOWN && e->key.repeat == 0) {
    // Move the sprite as long as the key is down
    switch (e->key.keysym.sym) {
    case SDLK_UP:
      paddle->dy -= paddle->speed;
      break;
    case SDLK_DOWN:
      paddle->dy += paddle->speed;
      break;
    }
  }
  if (e->type == SDL_KEYUP && e->key.repeat == 0) {
    // Stop moving once the key is released
    switch (e->key.keysym.sym) {
    case SDLK_UP:
      paddle->dy += paddle->speed;
      break;
    case SDLK_DOWN:
      paddle->dy -= paddle->speed;
      break;
    }
  }
}

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
void update_player(Ball* ball, Paddle* paddle) {
  int paddle_top = paddle->y;
  int paddle_bottom = paddle->y + paddle->h;

  int ball_top = ball->y;
  int ball_bottom = ball->y + ball->h;
  paddle->dy = 0;

  // move paddle up
  if (ball_top < paddle_top) {
    paddle->dy -= paddle->speed - ball->fudge;
  }

  // move paddle down
  if (ball_bottom > paddle_bottom) {
    paddle->dy += paddle->speed - ball->fudge;
  }
}

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
void check_collision(Ball* ball, Paddle* paddle) {
  bool collided =
    ball->x + ball->w >= paddle->x &&   // ball right edge past paddle left edge
    ball->x <= paddle->x + paddle->w && // ball left edge past paddle right edge
    ball->y + ball->h >= paddle->y &&   // ball top edge past paddle bottom
    ball->y <= paddle->y + paddle->h;   // ball bottom past paddle top

  // bounce the ball off the paddle
  if (collided) {
    play_sound(ball->paddle_sound);
    ball->dx *= -1;
    ball->fudge = get_fudge();
    // give player a chance to change the ball speed
    apply_english(ball, paddle);
  }
}

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
void apply_english(Ball* ball, Paddle* paddle) {
  // rand() % n == 0 is true n-1/n times, i.e. 5/6
  if (rand() % 6 == 0) {
    return;
  }
  // reset segment id
  ball->paddle_segment = 0;

  // divide paddle into 5 sections, apply angle and/or speed change
  // for each section
  int ball_top = ball->y;
  int paddle_top = paddle->y;
  int paddle_s1 = paddle->y + paddle->h / 5;
  int paddle_s2 = paddle->y + (paddle->h / 5) * 2;
  int paddle_s3 = paddle->y + (paddle->h / 5) * 3;
  int paddle_s4 = paddle->y + (paddle->h / 5) * 4;
  int paddle_bottom = paddle->y + paddle->h;

  bool hit_s1 = ball_top >= paddle_top && ball_top <= paddle_s1;
  bool hit_s2 = ball_top >= paddle_s1 && ball_top <= paddle_s2;
  bool hit_s3 = ball_top >= paddle_s2 && ball_top <= paddle_s3;
  bool hit_s4 = ball_top >= paddle_s3 && ball_top <= paddle_s4;
  bool hit_s5 = ball_top >= paddle_s4 && ball_top <= paddle_bottom;

  if (hit_s1) {
    ball->dy -= 2;
    ball->paddle_segment = 1;
  }
  if (hit_s2) {
    ball->dy -= 1;
    ball->paddle_segment = 2;
  }
  if (hit_s3) {
    if (ball->speed < BALL_MAX_SPEED) {
      ball->speed += 10;
    }
    ball->paddle_segment = 3;
  }
  if (hit_s4) {
    ball->dy += 1;
    ball->paddle_segment = 4;
  }

  if (hit_s5) {
    ball->dy += 2;
    ball->paddle_segment = 5;
  }
}

/*  ---------------------------------------------------------------------- 
    Description: Move the paddle up or down within the bounds of the court,
    COURT_OFFSIDE at the top edge, or COURT_HEIGHT at the bottom edge. This
    creates a buffer area at top and bottom of the court (delineated by white
    lines) that allows the ball to occasionally sneak past the paddle. 
    Parameters: 
      Paddle* paddle: pointer to player paddle
    Returns: none
    ---------------------------------------------------------------------- */
void move_paddle(Paddle* paddle) {
  paddle->y += paddle->dy * paddle->speed * paddle->time_step;
  if (paddle->y < COURT_OFFSIDE) {
    paddle->y = COURT_OFFSIDE;
  }
  if (paddle->y + paddle->h > COURT_HEIGHT) {
    paddle->y = COURT_HEIGHT - paddle->h;
  }
}

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
void move_ball(Ball* ball) {
  ball->x += ball->dx * ball->speed * ball->time_step;
  ball->y += ball->dy * ball->speed * ball->time_step;
  // bounce off top and bottom
  if (ball->y < 0 || ball->y + ball->h > SCREEN_HEIGHT) {
    play_sound(ball->wall_sound);
    ball->dy *= -1;
  }
}

/*  ---------------------------------------------------------------------- 
    Description: Renders the game scores stored in the ScoreBoard object
    Parameters: 
      App* app: pointer the App object
      ScoreBoard* score_board: pointer to the ScoreBoard object
    Returns: none
    ---------------------------------------------------------------------- */
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

  SDL_Rect renderQuad = { text_x, COURT_OFFSIDE, w, h };
  SDL_RenderCopyEx(
    app->renderer, fps_texture, NULL, &renderQuad, 0, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(fps_texture);
  fps_texture = NULL;
}

/*  ---------------------------------------------------------------------- 
    Description: Renders the game instructions on startup. When the game is
    in game over state, also renders the game winner's name.
    Parameters: 
      App* app: pointer the App object
      Game* game: pointer to the Game object
    Returns: none
    ---------------------------------------------------------------------- */
void draw_instructions(App* app, Game* game) {
  TTF_SetFontSize(game->score_board.font, 36);
  char* player_wins_text = "YAY!!! YOU WIN!!! :)\n\n";
  char* robot_wins_text = "AWWW!!! ROBOT WINS :(\n\n";
  char* instruction_text =
    "PRESS SPACEBAR TO BEGIN\n"
    "PRESS R TO RESET\n"
    "PRESS S TO TOGGLE BEEPS\n"
    "PRESS Q TO QUIT";

  char output_text[SCREEN_INSTRUCTIONS_BUF_SIZE] = "";
  if (game->winner == PLAYER) {
    strcat(output_text, player_wins_text);
  }
  if (game->winner == ROBOT) {
    strcat(output_text, robot_wins_text);
  }

  strcat(output_text, instruction_text);

  SDL_Color score_color = { .r = 255, .g = 255, .b = 255, .a = 255 };
  SDL_Surface* output_surface =
    TTF_RenderUTF8_Blended_Wrapped(
      game->score_board.font, output_text, score_color, 0);
  SDL_Texture* output_texture =
    SDL_CreateTextureFromSurface(app->renderer, output_surface);

  int w = output_surface->w;
  int h = output_surface->h;

  // TTF_SizeUTF8(font, output_text, &w, &h);
  SDL_FreeSurface(output_surface);
  output_surface = NULL;

  int text_x = (SCREEN_WIDTH - w) / 2;
  int text_y = SCREEN_MID_H - h / 2;

  SDL_Rect renderQuad = { text_x, text_y, w, h };
  SDL_RenderCopyEx(
    app->renderer, output_texture, NULL, &renderQuad, 0, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(output_texture);
  output_texture = NULL;
}

/*  ---------------------------------------------------------------------- 
    Description: Render the game court lines and net.
    Parameters: 
      App* app: pointer to the App object
    Returns: none
    ---------------------------------------------------------------------- */
void draw_court(App* app) {
  SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_Rect net_line = { .w = 3, .h = 15, .x = SCREEN_MID_W - 1 };
  for (net_line.y = COURT_OFFSIDE; net_line.y < SCREEN_HEIGHT - COURT_OFFSIDE;
    net_line.y += COURT_OFFSIDE) {
    SDL_RenderFillRect(app->renderer, &net_line);
  }

  SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  SDL_Rect court_line = { .w = SCREEN_WIDTH, .h = 1, .x = 0, .y = COURT_OFFSIDE };
  SDL_RenderFillRect(app->renderer, &court_line);

  court_line.y = SCREEN_HEIGHT - COURT_OFFSIDE;
  SDL_RenderFillRect(app->renderer, &court_line);
}

/*  ---------------------------------------------------------------------- 
    Description: Render game stats in the offcourt area at the bottom of the 
    screen. Stats are only of interest to game developers, so it is rendered
    only when SDL Log priority is higher than SDL_LOG_PRIORITY_DEBUG
    Parameters: 
      App* app: pointer to the App object
      Game* game: pointer to the Game object
    Returns: none
    ---------------------------------------------------------------------- */
void draw_stats(App* app, Game* game) {
  // only show stats for SDL_LOG_PRIORITY_DEBUG or SDL_LOG_PRIORITY_VERBOSE
  if (SDL_LogGetPriority(LOGCAT) > SDL_LOG_PRIORITY_DEBUG) {
    return;
  }
  char fps_text[SCREEN_FPS_BUF_SIZE];
  SDL_Color fps_color = { .r = 255, .g = 255, .b = 0, .a = 255 };

  double avg_fps = game->frame_count / ((SDL_GetTicks() - game->fps_ticks) / 1000.f);
  double velocity =
    sqrt((game->ball.dx * game->ball.dx) + (game->ball.dy * game->ball.dy)) *
    game->ball.speed;

  snprintf(fps_text, SCREEN_FPS_BUF_SIZE,
    "Avg FPS:%2.f Ball [dx:%2.f dy:%2.f] "
    "[x:%4.f y:%4.f] fudge:%2d speed: %d vel: %.f segment: %d",
    avg_fps, game->ball.dx, game->ball.dy,
    game->ball.x, game->ball.y, game->ball.fudge,
    game->ball.speed, velocity, game->ball.paddle_segment);

  SDL_Surface* fps_surface =
    TTF_RenderText_Blended(game->stats_font, fps_text, fps_color);
  SDL_Texture* fps_texture =
    SDL_CreateTextureFromSurface(app->renderer, fps_surface);
  int fps_w = fps_surface->w;
  int fps_h = fps_surface->h;
  SDL_FreeSurface(fps_surface);
  fps_surface = NULL;

  SDL_Rect renderQuad = { 10, 462, fps_w, fps_h };
  SDL_RenderCopyEx(
    app->renderer, fps_texture, NULL, &renderQuad, 0, NULL, SDL_FLIP_NONE);
  SDL_DestroyTexture(fps_texture);
  fps_texture = NULL;
}

/*  ---------------------------------------------------------------------- 
    Description: Plays the given sound
    Parameters: 
      Mix_Chunk* sound: pointer to the Mix_Chunk sound object
    Returns: none
    ---------------------------------------------------------------------- */
void play_sound(Mix_Chunk* sound) {
  Mix_PlayChannel(-1, sound, 0);
}

/*  ---------------------------------------------------------------------- 
    Description: Entry point to game execution
    Parameters: standard argc and argv. Note this main() is actually called by 
    SDL, which will bitterly complain if the signature is changed, e.g. 
      error: conflicting types for 'SDL_main'; have 'int(int,  char *)'
      143 | #define main    SDL_main
    Returns: Exit status expected by platform
    ---------------------------------------------------------------------- */
int main(int argc, char* argv[]) {
  (void)argc, (void)argv;

  App* app = init();

  if (app == NULL) {
    SDL_LogCritical(LOGCAT, "App init failed!");
    return EXIT_FAILURE;
  }

  Game game = {
    .score_board = {
      .font = load_font("../assets/VT323-Regular.ttf", 40),
      .player = 0,
      .robot = 0
    },
    .winner = NOBODY,
    .player = {0},
    .robot = {0},
    .ball = {0},
    .stats_font = load_font("../assets/Inconsolata-Regular.ttf", 14),
    .play_sounds = true,
    .running = true,
    .idle = true,
    .over = false,
  };

  load_sounds(&game);

  reset_paddle(&game.player, PLAYER);

  reset_paddle(&game.robot, ROBOT);

  reset_ball(&game.ball, ROBOT);

  SDL_Event e;
  game.frame_count = 0;
  game.cap_ticks = 0;
  game.step_ticks = 0;
  game.fps_ticks = SDL_GetTicks();

  while (game.running) {
    game.cap_ticks = SDL_GetTicks();

    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        game.running = false;
      }
      if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_q:
        case SDLK_ESCAPE:
          game.running = false;
          break;
        case SDLK_SPACE:
          reset_game(&game);
          game.idle = false;
          break;
        case SDLK_r:
          reset_game(&game);
          break;
        case SDLK_l:
          set_log_priority(app);
          break;
        case SDLK_s:
          game.play_sounds = !game.play_sounds;
          break;
        default:
          break;
        }
      }
      if (game.idle == false) {
        handle_input(&e, &game.player);
      }
    }

    // toggle sound effects
    if (game.play_sounds) {
      Mix_Volume(-1, MIX_MAX_VOLUME);
    } else {
      Mix_Volume(-1, 0);
    }

    // reset game
    if (game.over) {
      reset_game(&game);
    }

    if (game.idle) {
      // let AI control player paddle
      update_player(&game.ball, &game.player);
    }

    update_player(&game.ball, &game.robot);

    check_collision(&game.ball, &game.player);
    check_collision(&game.ball, &game.robot);

    double time_step = (SDL_GetTicks() - game.step_ticks) / 1000.f;
    game.ball.time_step = time_step;
    game.player.time_step = time_step;
    game.robot.time_step = time_step;

    move_paddle(&game.player);
    move_paddle(&game.robot);

    // move ball
    move_ball(&game.ball);

    game.step_ticks = SDL_GetTicks();

    // check for score
    if (game.ball.x < 0) {
      // Player scored
      game.score_board.player++;
      play_sound(game.point_sound);
      if (game.score_board.player >= MAX_SCORE) {
        game.over = true;
        game.winner = PLAYER;
      } else {
        reset_ball(&game.ball, PLAYER);
        // game.ball.x = PLAYER_SERVICE_X;
        game.ball.y = game.player.y + game.player.h / 2;
      }
    }

    if (game.ball.x > SCREEN_WIDTH) {
      // Robot scored
      game.score_board.robot++;
      play_sound(game.point_sound);
      if (game.score_board.robot >= MAX_SCORE) {
        game.over = true;
        game.winner = ROBOT;
      } else {
        reset_ball(&game.ball, ROBOT);
        game.ball.y = game.robot.y + game.robot.h / 2;
      }
    }

    //Clear screen
    SDL_SetRenderDrawColor(app->renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(app->renderer);

    draw_court(app);
    draw_score(app, &game.score_board);
    if (game.idle) {
      draw_instructions(app, &game);
    }

    SDL_SetRenderDrawColor(app->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    // draw player paddle
    SDL_Rect player_rect = {
      .h = game.player.h, .w = game.player.w,
      .x = game.player.x, .y = game.player.y
    };
    SDL_RenderFillRect(app->renderer, &player_rect);

    // draw robot paddle
    SDL_Rect robot_rect = {
      .h = game.robot.h, .w = game.robot.w,
      .x = game.robot.x, .y = game.robot.y
    };
    SDL_RenderFillRect(app->renderer, &robot_rect);

    // draw ball
    SDL_Rect ball_rect = {
      .h = game.ball.h, .w = game.ball.w,
      .x = game.ball.x, .y = game.ball.y
    };
    SDL_RenderFillRect(app->renderer, &ball_rect);

    draw_stats(app, &game);

    // Update screen
    SDL_RenderPresent(app->renderer);
    ++game.frame_count;

    // Cap frame rate
    game.frame_ticks = SDL_GetTicks() - game.cap_ticks;
    if (game.frame_ticks < SCREEN_TICKS_PER_FRAME) {
      SDL_Delay(SCREEN_TICKS_PER_FRAME - game.frame_ticks);
    }
  }

  TTF_CloseFont(game.stats_font);
  TTF_CloseFont(game.score_board.font);
  SDL_DestroyRenderer(app->renderer);
  SDL_DestroyWindow(app->window);
  Mix_FreeChunk(game.ball.wall_sound);
  Mix_FreeChunk(game.ball.paddle_sound);
  Mix_FreeChunk(game.point_sound);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
  return EXIT_SUCCESS;
}