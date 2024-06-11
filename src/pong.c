// SDL2 Pong Game
#include "pong.h"

App* init(void) {

  srand(time(0));
  // SDL_LOG_PRIORITY_INFO
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

void reset_game(Game* game) {
  game->score_board.player = 0;
  game->score_board.robot = 0;
  game->winner = game->over ? game->winner : NOBODY;
  reset_paddle(&game->player);
  reset_paddle(&game->robot);
  reset_ball(&game->ball);
  game->idle = true;
  game->over = false;
}

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

void reset_paddle(Paddle* paddle) {
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

int get_fudge(void) {
  return rand() % 15;
}

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

void check_collision(Ball* ball, Paddle* paddle) {

  // see https://www.jeffreythompson.org/collision-detection/rect-rect.php
  // for explanation of this algorithm
  bool collided =
    ball->x + ball->w >= paddle->x &&   // ball right edge past paddle left edge
    ball->x <= paddle->x + paddle->w && // ball left edge past paddle right edge
    ball->y + ball->h >= paddle->y &&   // ball top edge past paddle bottom
    ball->y <= paddle->y + paddle->h;   // ball bottom past paddle top

  // bounce the ball off the paddle
  if (collided) {
    ball->dx *= -1;
    ball->fudge = get_fudge();
    // give player a chance to change the ball speed
    apply_english(ball, paddle);
  }
}

void apply_english(Ball* ball, Paddle* paddle) {
  // rand() % n == 0 is true n-1/n times, i.e. 5/6
  int skip = rand() % 6;
  if (skip == 0) {
    SDL_LogDebug(LOGCAT, "skipped english");
    return;
  }
  // reset segment id
  ball->paddle_segment = 0;
  double dy_before = ball->dy;
  int ball_speed_before = ball->speed;

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

  SDL_LogDebug(LOGCAT, "s1:%d s2:%d s3:%d s4:%d s5:%d",
    hit_s1, hit_s2, hit_s3, hit_s4, hit_s5);

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
    SDL_LogDebug(LOGCAT, "ball->speed before:%2d after:%2d",
      ball_speed_before, ball->speed);
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

  SDL_LogDebug(LOGCAT, "segment: %d | ball->dy before:%2.f after:%2.f",
    ball->paddle_segment, dy_before, ball->dy);
}


void move_paddle(Paddle* paddle) {
  paddle->y += paddle->dy * paddle->speed * paddle->time_step;
  if (paddle->y < COURT_OFFSIDE) {
    paddle->y = COURT_OFFSIDE;
  }
  if (paddle->y + paddle->h > COURT_HEIGHT) {
    paddle->y = COURT_HEIGHT - paddle->h;
  }
}

void reset_ball(Ball* ball) {
  ball->service = ROBOT;
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

void move_ball(Ball* ball) {
  ball->x += ball->dx * ball->speed * ball->time_step;
  ball->y += ball->dy * ball->speed * ball->time_step;
  // bounce off top and bottom
  if (ball->y < 0 || ball->y + ball->h > SCREEN_HEIGHT) {
    ball->dy *= -1;
  }
}

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

void draw_instructions(App* app, Game* game) {
  TTF_SetFontSize(game->score_board.font, 36);
  char* player_wins_text = "YAY!!! YOU WIN!!! :)\n\n";
  char* robot_wins_text = "AWWW!!! ROBOT WINS :(\n\n";
  char* instruction_text = 
    "PRESS SPACEBAR TO BEGIN\nPRESS R TO RESET\nPRESS ESCAPE TO QUIT";

  char output_text[SCREEN_INSTRUCTIONS_BUF_SIZE];
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

void draw_stats(App* app, int frame_count, Uint32 fps_ticks, TTF_Font* fps_font, Ball* ball) {
  char fps_text[SCREEN_FPS_BUF_SIZE];
  SDL_Color fps_color = { .r = 255, .g = 255, .b = 0, .a = 255 };

  double avg_fps = frame_count / ((SDL_GetTicks() - fps_ticks) / 1000.f);
  double velocity =
    sqrt((ball->dx * ball->dx) + (ball->dy * ball->dy)) *
    ball->speed;

  snprintf(fps_text, SCREEN_FPS_BUF_SIZE,
    "Avg FPS:%2.f Ball [dx:%2.f dy:%2.f] "
    "[x:%4.f y:%4.f] fudge:%2d speed: %d vel: %.f segment: %d",
    avg_fps, ball->dx, ball->dy,
    ball->x, ball->y, ball->fudge, ball->speed, velocity, ball->paddle_segment);

  SDL_Surface* fps_surface =
    TTF_RenderText_Blended(fps_font, fps_text, fps_color);
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


int main(int argc, char* argv[]) {
  (void)argc, (void)argv;

  App* app = init();

  if (app == NULL) {
    SDL_LogCritical(LOGCAT, "App init failed!");
    return EXIT_FAILURE;
  }

  TTF_Font* stats_font =
    TTF_OpenFont("../assets/Inconsolata-Regular.ttf", 14);
  if (stats_font == NULL) {
    SDL_LogError(LOGCAT,
      "Failed to load font! SDL_ttf Error: %s\n",
      TTF_GetError());
  }

  TTF_Font* score_font =
    TTF_OpenFont("../assets/VT323-Regular.ttf", 40);
  if (score_font == NULL) {
    SDL_LogError(LOGCAT,
      "Failed to load font! SDL_ttf Error: %s\n",
      TTF_GetError());
  }

  Game game = {
    .score_board = {.font = score_font, .player = 0, .robot = 0 },
    .winner = NOBODY,
    .player = {0},
    .robot = {0},
    .ball = {0},
    .running = true,
    .idle = true,
    .over = false,
  };

  // Paddle player = { 0 };
  game.player.owner = PLAYER;
  reset_paddle(&game.player);

  // Paddle robot = { 0 };
  game.robot.owner = ROBOT;
  reset_paddle(&game.robot);

  // Ball ball = { 0 };
  reset_ball(&game.ball);

  SDL_Event e;
  int frame_count = 0;
  Uint32 frame_ticks = 0;
  Uint32 cap_ticks = 0;
  Uint32 step_ticks = 0;
  Uint32 fps_ticks = SDL_GetTicks();

  while (game.running) {
    cap_ticks = SDL_GetTicks();

    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        game.running = false;
      }
      if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
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
        default:
          break;
        }
      }
      if (game.idle == false) {
        handle_input(&e, &game.player);
      }
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

    double time_step = (SDL_GetTicks() - step_ticks) / 1000.f;
    game.ball.time_step = time_step;
    game.player.time_step = time_step;
    game.robot.time_step = time_step;

    move_paddle(&game.player);
    move_paddle(&game.robot);

    // move ball
    move_ball(&game.ball);

    step_ticks = SDL_GetTicks();

    // check for score
    if (game.ball.x < 0) {
      // Player scored
      game.score_board.player++;
      if (game.score_board.player >= MAX_SCORE) {
        game.over = true;
        game.winner = PLAYER;
      } else {
        reset_ball(&game.ball);
        // update defaults
        game.ball.service = PLAYER;
        game.ball.x = PLAYER_SERVICE_X;
        game.ball.y = game.player.y + game.player.h / 2;
      }
    }

    if (game.ball.x > SCREEN_WIDTH) {
      // Robot scored
      game.score_board.robot++;
      if (game.score_board.robot >= MAX_SCORE) {
        game.over = true;
        game.winner = ROBOT;
      } else {
        reset_ball(&game.ball);
        // update defaults
        game.ball.service = ROBOT;
        game.ball.x = ROBOT_SERVICE_X;
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

    draw_stats(app, frame_count, fps_ticks, stats_font, &game.ball);

    // Update screen
    SDL_RenderPresent(app->renderer);
    ++frame_count;

    // Cap frame rate
    frame_ticks = SDL_GetTicks() - cap_ticks;
    if (frame_ticks < SCREEN_TICKS_PER_FRAME) {
      SDL_Delay(SCREEN_TICKS_PER_FRAME - frame_ticks);
    }
  }

  TTF_CloseFont(stats_font);
  TTF_CloseFont(game.score_board.font);
  SDL_DestroyRenderer(app->renderer);
  SDL_DestroyWindow(app->window);
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
  return EXIT_SUCCESS;
}