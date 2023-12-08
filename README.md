# perfaware
Homework solutions for Casey Muratori's [Performance-Aware Programming](https://www.computerenhance.com/p/table-of-contents) course.


## Included projects
Most projects (currently) require that either `cl` (MSVC) or `gcc`/`g++` be accessible via the PATH environment variable, though most build scripts can be modified to use `clang`, etc.


### cli_sim8086
The original homework assignment program from Part 1 of the course. A command line interface application that simulates an 8086 processor with a limited subset of x86 instructions supported. Built by executing [build-sim8086.bat](build-sim8086.bat) (using MSVC) or [build-sim8086.sh](build-sim8086.sh) (using clang).


### sim8086_win32
A Win32 version of the original 8086 simulator with a GUI. Inspired by Jeremy English's [Sim8088](https://codeberg.org/jeng/Sim8088) and [RemedyBG](https://remedybg.handmade.network/). This likewise is my attempt to gain some familiarity using IMGUI (and [DearImGui](https://github.com/ocornut/imgui) specifically).

Built by executing [build-sim8086-win32.bat](build-sim8086-win32.bat). On an interesting note, the program features Handmade Hero-style hot-loading of the application layer, which is extremely handy while building a GUI. Casey is a genius. To take advantage of hot-reloading, simply make your changes to the GUI script with the application running and re-execute the build script.

![image](assets/win32_sim8086_50.png "screenshot")


### sim8086_linux
A Linux port of `sim8086_win32`. Uses GLFW for the window implementation. Built by executing [build-sim8086-linux.sh](build-sim8086-linux.sh). Produced in order to gain experience working with OS layers and to gain some familiarity with Linux system calls.


### haversine-gen
Part of the homework assignment for Part 2. Generates the Haversine distance pairs that will be used by the Haversine processor. Build by executing [build-haversine-gen.bat](build-haversine-gen.bat).


### haversine-proc
More homework for Part 2. Consumes the Haversine distance pairs created by the generator. Build by executing [build-haversine-proc.bat](build-haversine-proc.bat).
