# MultithreadedParticleFilter

This repository provides a hands-on example of a multithreaded particle filter, inspired by the core concepts outlined in [Particle Filters: A Hands-On Tutorial](https://pmc.ncbi.nlm.nih.gov/articles/PMC7826670/). The implementation is written in modern C++, with the flexibility to toggle multithreading on or off through a simple boolean flag—making it easy to experiment with both single-threaded and parallel execution.

# Robot State & Sensor Model
In this demonstration, the system estimates a 2D state consisting of  coordinates. The simulated sensor is modeled as a distance sensor that measures , with its base fixed at the origin. This setup provides a straightforward yet illustrative way to explore how particle filters integrate sensor readings into state estimation.

Robot state: 
$
\textbf{x} = \begin{bmatrix}
x\\
y 
\end{bmatrix}
$

Sensor model : $s = \sqrt{\left( x-\mathcal{O}_x \right)^2 + \left( y-\mathcal{O}_y \right)^2}$

# Robot Behavior
The "robot" in this example follows a simple but dynamic routine:
* It randomly generates a waypoint in the 2D plane.
* Using a fixed step size, it begins moving toward that waypoint.
* Once the waypoint is reached, a new random target is generated, and the process repeats.

The main steps of a particle filter regardless of the number of threads is:

1. Update weights based on sensor reading
1. Move particles based on control input
    1. Get particle filter estimate $\hat{x}$.
1. Resample particles based on weights
1. Mutate the particles for new exploration

![Results](particle_filter_animation.gif)
