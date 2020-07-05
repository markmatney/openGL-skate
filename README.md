# openGL-skate

Skateboarding animation using OpenGL, assignment for CS 174A, Intro to Computer Graphics, UCLA Spring 2014

## Installation

1. Install a C++ compiler and the [GLEW](http://glew.sourceforge.net/) and [freeglut](http://freeglut.sourceforge.net/) libraries.
2. Compile:
    ```bash
    g++ -o skate -lGLEW -lGL -lglut lib/*.{cxx,cpp} src/skate.cpp
    ```
3. Run:
    ```bash
    ./skate
    ```

## Notes

If you want to create your own animation, you can use `src/anim.cpp` as a starter template.

You can see [a video of my original submission](https://www.youtube.com/watch?v=6CXe2BEXGGs) on YouTube. [Here are some of the others from the class](https://www.youtube.com/playlist?list=UUeLhfHp5O2MZfD6CXZ330qA&playnext=1&index=1), for your entertainment :-)
