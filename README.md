# Attempt at Pong game clone using SDL2

## Known Bugs

* Ball occasionally gets 'stuck' on paddle when it intersects the paddle side
  but this is like a backhand feature :)

## Features TBD

* Document code (functions, etc.)
* Make Robot AI more 'human':
  * Intercept ball with middle of paddle
  * Add an 'overshoot' random factor
  * Move the paddle toward the middle of the screen after a volley
* Use environment variable to control console logging and in-game stats output
* Add Release build to makefile
* License
* Add packaging and distribution to makefile (build an installer exe with NSIS?)
* Cross-platform builds?
  * Windows (done)
  * Linux
  * Macintosh

## Features Completed

* Basic ball and paddle movement
* 'Robot' (computer AI) follows ball 
* Score keeping
* Idle screen
* Court edge lines across top and bottom of screen
* Apply 'English' when ball contacts outer sides of paddle or when paddle is moving
* Speed up ball slightly after each volley
* Sound effects
