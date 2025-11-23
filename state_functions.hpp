// Custom Non‑Commercial License

// Copyright (c) 2025 Mgoodell97

// Permission is hereby granted, free of charge, to any individual or
// non‑commercial entity obtaining a copy of this software and associated
// documentation files (the "Software"), to use, copy, modify, merge, publish,
// and distribute the Software for personal, educational, or research purposes,
// subject to the following conditions:

// 1. Commercial Use:
//    Any company, corporation, or organization intending to use the Software
//    must first notify the copyright holder and obtain explicit written
//    permission. Commercial use without such permission is strictly prohibited.

// 2. Unauthorized Commercial Use:
//    If a company is found to be using the Software without prior authorization,
//    the copyright holder is entitled to receive 1% of the company’s gross
//    profits moving forward, enforceable as a licensing fee.

// 3. Artificial Intelligence / Machine Learning Use:
//    If the Software is incorporated into machine learning
//    models, neural networks, generative pre‑trained transformers (GPTs), or similar AI systems,
//    the company deploying such use is solely responsible for compliance with
//    this license. Responsibility cannot be shifted to the provider of training
//    data or third‑party services.

// 4. Attribution:
//    The above copyright notice and this permission notice shall be included in
//    all copies or substantial portions of the Software.

// Disclaimer:
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cmath>
#include <random>
#include <fstream>

#pragma once

double X_MIN = 0.0;
double Y_MIN = 0.0;

double X_MAX = 100.0;
double Y_MAX = 100.0;

double MAX_STEP_SIZE = 2.0;

std::random_device rd;
std::mt19937 rng_generator(rd());

std::uniform_real_distribution<double> x_waypoint_dist(X_MIN, X_MAX);
std::uniform_real_distribution<double> y_waypoint_dist(Y_MIN, Y_MAX);

struct State
{
    double x;
    double y;
};

double sensorFunction(const State& state) 
{
    // Assume the sensor is at the origin (0,0) and measures distance
    return std::sqrt(std::pow(state.x - 0.0, 2) + std::pow(state.y - 0.0, 2));
}   

double likelihoodFunction(const double sensor_observation, const double estimate_observation, const double sensor_std) 
{
    const double diff_over_sig = (sensor_observation - estimate_observation)/sensor_std;

    // We don't care about the demominator since we will normalize later
    return std::exp(-0.5 * (std::pow(diff_over_sig, 2)));
}

// Generate a new random waypoint
State generateWaypoint()
{
    State wp;

    wp.x = x_waypoint_dist(rng_generator);
    wp.y = y_waypoint_dist(rng_generator);
    return wp;
}

void moveActualState(State& state, State& waypoint)
{
    double dx = waypoint.x - state.x;
    double dy = waypoint.y - state.y;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist >= MAX_STEP_SIZE)
    {
        state.x += (dx / dist) * MAX_STEP_SIZE;
        state.y += (dy / dist) * MAX_STEP_SIZE;
    }
    else
    {
        state.x = waypoint.x;
        state.y = waypoint.y;
        waypoint = generateWaypoint();
    }
}

void moveEstimatedState(State& state,const State& waypoint)
{
    double dx = waypoint.x - state.x;
    double dy = waypoint.y - state.y;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist >= MAX_STEP_SIZE)
    {
        state.x += (dx / dist) * MAX_STEP_SIZE;
        state.y += (dy / dist) * MAX_STEP_SIZE;
    }
    else
    {
        state.x = waypoint.x;
        state.y = waypoint.y;
    }
}

void saveStateToCSV(const State& state, const std::string& filename) 
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    file << "i,x,y" << "\n";
    file << 0 << "," << state.x << "," << state.y << "\n";
    file.close();
}

void saveSensorReadingToCSV(const double readings, const std::string& filename) 
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    file << "reading" << "\n";
    file << readings << "\n";
    file.close();
}

void calculateAndPrintError(const State& estimated_state, const State& true_state)
{
    double error_x = estimated_state.x - true_state.x;
    double error_y = estimated_state.y - true_state.y;

    double l2_error = std::sqrt(error_x * error_x + error_y * error_y );

    std::cout << "    Error: " << l2_error << "\n";
}