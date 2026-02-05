# Multi-threaded Particle Filter

This repository provides a hands-on example of a multithreaded particle filter, inspired by the core concepts outlined in [Particle Filters: A Hands-On Tutorial](https://pmc.ncbi.nlm.nih.gov/articles/PMC7826670/). The implementation is written in modern C++, with the flexibility to toggle multithreading on or off through a simple boolean flag—making it easy to experiment with both single-threaded and parallel execution.

# Robot State & Sensor Model
In this example, the system estimates a 2D state consisting of xy coordinates. The simulated sensor is modeled as a distance sensor that measures, with its base fixed at the origin. This setup provides a straightforward yet illustrative way to explore how particle filters integrate sensor readings into state estimation.

Robot state: 
```math
\textbf{x} = \begin{bmatrix}
x\\
y 
\end{bmatrix}
```

Sensor model : 
```math
s = \sqrt{\left( x-\mathcal{O}_x \right)^2 + \left( y-\mathcal{O}_y \right)^2}
```

# Robot Behavior
The "robot" in this example follows a simple but dynamic routine:
* It randomly generates a waypoint in the 2D plane.
* Using a fixed step size, it begins moving toward that waypoint.
* Once the waypoint is reached, a new random target is generated, and the process repeats.

The main steps of a particle filter regardless of the number of threads is:

1. Update weights based on sensor reading
    1. Get particle filter estimate $\hat{x}$.
1. Move particles based on control input
1. Resample particles based on weights
1. Mutate the particles for new exploration

![Results](particle_filter_animation.gif)

# Building and running

Below are concise, copy-pasteable commands for common build+run scenarios. The Makefile can be used on both Windows (MinGW) and Linux. 

Common variables:
- CXX: compiler (default g++)
- tracy=1: enable Tracy profiler support 
- sanitize=1: enable ThreadSanitizer (clang only)

// Windows
cmake -B build -G "MinGW Makefiles" -DTRACY_ENABLE=ON
cmake -B build -G "MinGW Makefiles"
cmake --build build

clean
rm -r -force build

// Linux
cmake -B build -DCMAKE_CXX_COMPILER=clang++ -DSANITIZE=ON -DTRACY_ENABLE=ON
cmake --build build


### 1.1 Build using the Makefile (default g++)
Windows (MinGW):
```
mingw32-make
```
Linux:
```
make
```

### 1.2 Build with Tracy enabled
Windows (MinGW):
```
mingw32-make tracy=1
```
Linux:
```
make tracy=1
```

### 1.3 Build with clang++
Basic (no Tracy):
```
make CXX=clang++
```
With Tracy:
```
make CXX=clang++ tracy=1
```
With ThreadSanitizer (clang only):
```
make CXX=clang++ sanitize=1
```
With both sanitize and Tracy:
```
make CXX=clang++ sanitize=1 tracy=1
```

### 1.4 Direct g++ / clang++ commands (no Makefile)
MinGW g++ (no Tracy):
```
g++ main.cpp -o main.exe -lpthread -std=c++23
```
MinGW g++ with Tracy:
```
g++ main.cpp tracy/public/TracyClient.cpp -Itracy/public -std=c++23 -DTRACY_ENABLE -g -o main.exe -lws2_32 -ldbghelp -lpthread
```
clang++ (Linux) with ThreadSanitizer:
```
clang++ -fsanitize=thread -std=c++23 main.cpp -o main -lpthread
```
clang++ with Tracy and sanitizer:
```
clang++ -fsanitize=thread -g -std=c++23 main.cpp tracy/public/TracyClient.cpp -Itracy/public -DTRACY_ENABLE -o main -lpthread
```

### 2. Run the program
Windows:
```
main.exe
```

Linux / macOS:
```
./main.exe
```

### Notes
- ThreadSanitizer is supported only with clang on Linux.
- Tracy requires running the Tracy profiler UI (Tracy) to capture profiles; build with tracy=1 and run the UI before running the program to see timeline events.
- If running the sanitizer Tracy you will get warnings about the thread safety in Tracy this is not a problem for this code. 
