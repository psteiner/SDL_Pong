# Attempt at Pong game clone using SDL2

## Known Bugs

* Ball occasionally gets 'stuck' when it intersects the side or end
of the paddle (maybe a 'backhand' feature? :)
* Ball occasionally gets stuck when it intersects the window top or bottom edge

## Features TBD

* Make Robot AI more 'human':
  * Intercept ball with middle of paddle
  * Add an 'overshoot' random factor
  * Move the paddle toward the middle of the screen after a volley
* Add Release build to makefile
* License
* Add packaging and distribution to Makefile (build an installer exe with NSIS?)
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
* Use ~~environment variable~~ L key to control console logging and in-game stats output
* Document code (functions, etc.)
