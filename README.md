# Attempt at Pong game clone using SDL2

## Known Bugs

* Ball occasionally gets 'stuck' on paddle when it intersects the paddle side
  but this is like a backhand feature :)

## Features TBD

* Prefer robot intercepting ball with middle of paddle
* Sound effects
* Use environment variable to control console logging and in-game stats output
* function documentation
* Release build
* License
* Packaging and distribution
* Cross-platform build
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
