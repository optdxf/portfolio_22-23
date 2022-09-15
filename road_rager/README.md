# Road Rager: Overview

A multiplayer racing game written in C by a group of 4.

# Development Log

## IMPORTANT NOTES:
- **Important**: You need to have access to two ports: your port (`cs-port`) and `cs-port + 1`.
- Run `make all` to compile and run both the servers and the client.
- Run client_v2.html. This has re-scalable window sizes.
- To ensure the best performance, `make clean` if you want to play multiplayer multiple times consecutively.

## Game Log


#### Tues 17 May 2022, 5:00pm - 7:00pm
- **Who**: Dominic, Enoch, Markus, Miguel
- **What**: Discussed plan for week, created file structure.
- **Bugs**: N/A
- **Resources**: N/A

#### Wed 18 May 2022, 10:30pm - 3:30pm
- **Who**: Dominic, Markus
- **What**: Dominic worked on multiplayer, Markus on basic car physics: acceleration, braking, steering
- **Bugs**: Multiplayer not working
- **Resources**: N/A

#### Thu 19 May 2022, 10:00am - 1:00pm
- **Who**: Markus, Enoch
- **What**: Implemented finish line logic, basic map. Dominic customized the makefile and made a nice resizable wrapper for the html/js file (respects aspect ratio).
- **Bugs**: Car can go through walls!
- **Resources**: N/A

#### Thu 19 May 2022, 2:00pm - 3:00pm 
- **Who**: Markus, Miguel
- **What**: Markus added tracks & updated car physics (drift physics), Miguel implemented image rendering and integrated embedded images with polygons.
- **Bugs**: Car can still go through walls! Took a long time to debug an error in image rendering; we found
that the image files were corrupted on input to VSCode through LiveShare.
- **Resources**: Online guides for implementing image rendering, StackExchange

#### Mon 23 May 2022, 12:00pm - 2:00pm
- **Who**: Enoch, Miguel
- **What**: Miguel implemented text rendering and imported some fonts. Enoch created a timer, display time and track previous lap time with text rendering. Dominic helped with image rendering (trying out SVG rendering, which we might end up using)
- **Bugs**: Memory overflows, fixed by truncating time double and freeing the malloc. Text rendering faced significantly fewer issues than image rendering. Right now the text and images are being loaded from disk every
step, so Miguel will implement image caching soon.
- **Resources**: stack overflow

#### Thurs 26 May 2022, 12:00pm - 2:00pm 
- **Who**: Enoch, Miguel, Dominic, Markus
- **What**: Enoch worked on additional maps, Miguel on caching images, Dominic on networking code and bug fixes, Markus on car-wall collision resolution
- **Bugs**: Can still go through walls, blocks made for curves not tight enough.
- **Resources**: stack overflow

#### Fri 27 May 2022, 12:00pm - 6:00pm 
- **Who**: Enoch, Miguel, Dominic, Markus
- **What**: Enoch worked on integrating map images with map polygons, Markus updated collisions further by considering normal and tangential components separately, Dominic worked on further networking code and explored image animations, Miguel began working on UI and menus.
- **Bugs**: Collisions with curves do not proveed as intended, UI element placing misalignments
- **Resources**: stack overflow

#### Sat 28 May 2022, 2:00pm - 6:00pm 
- **Who**: Enoch, Miguel, Dominic, Markus
- **What**: Enoch continued to work on maps, Miguel worked on camera viewpoint in sdl_wrapper (rendering only part of the scene), Dominic worked on framework for the UI and menu (and callbacks for mouse events like mouse enter/leave, mouse button down/up), Markus beautified tracks rendering by implementing opacity and varying thickness, as well as adding functionality to rotate bodies about arbitrary point to make car handling feel more realistic (car rotating about rear axle)
- **Bugs**: Memory leaks
- **Resources**: WebPlotDigitizer, StackExchange

#### Tue 31 May 2022, 4:00pm - 6:00pm
- **Who**: Enoch, Miguel, Dominic, Markus
- **What**: Markus updated collisions by ignoring collisions for objects far apart and adding collision flags for cars, Dominic completed restructuring of file structure to implement networking and continued working on the UI framework; also ported most of the codebase to support a generic "server"/client model for use in local/multiplayer. Enoch continued to work on maps. Miguel continued working on the menu and UI, selecting the spots for everything to be drawn.
- **Bugs**: Car gets stuck
- **Resources**: stack overflow

#### Sat 4 June 2022, 9:00am - 6:00pm 
- **Who**: Enoch, Miguel, Markus
- **What**: Markus finalized collisions, eliminating driving through walls by adding forced centroid displacements, added friction from off-track grass, Enoch programmed collision orientation vectors for wall curves and defined grass polygons for the tracks, Miguel worked more on the menu UI, all three worked on gameplay logic for single- and multi-player lap counting and countdown timers, Dominic continued to work on networking and made sprites for the game
- **Bugs**: None
- **Resources**: None

#### Sun-Mon 5-6 June 2022, 8:00am - 6:00am (real)
- **Who**: Enoch, Miguel, Dominic, Markus
- **What**: Dominic finalized multiplayer, Enoch implemented multiple car types. Enoch, Miguel, Markus continued to gamify single player and re-implemented lap logic. Markus added additional car braking physics to allow for more aggressive turns at high speeds, and polished car-car collisions by implementing the collision flag to prevent cars from driving through each other. Enoch polished maps. Miguel polished up general UI. All worked on documentation.
- **Bugs**: Many memory leaks, body_init_with_info was not used for tracks
- **Resources**: stack overflow, ASAN






