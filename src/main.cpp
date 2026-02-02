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

#include <iostream>
#include <iomanip>

#ifdef TRACY_ENABLE
    #include "tracy/Tracy.hpp"
#endif

// Internal includes
#include "helper_functions.hpp"
#include "state_functions.hpp"
#include "particle_filter.hpp"

int main() {
    std::cout << std::setprecision(17);

    #ifdef TRACY_ENABLE
        std::cout << "Waiting 2s to load tracy." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Starting code." << std::endl;
    #endif

    // Particle params
    bool run_pf_in_parallel = true;
    int64_t resamples = 100;
    PF_Params pf_params;
    std::vector<double> particle_propogation_std{5,5};
    ParticleFilter pf{pf_params, &likelihoodFunction, &moveEstimatedState, run_pf_in_parallel};

    // Sensor noise
    State robot_state = generateWaypoint(); // Abusing this function
    State waypoint = generateWaypoint();
    double sensor_std_dev = 2.5;
    std::normal_distribution<double> sensor_noise_distribution(0, sensor_std_dev);

    // For visualizations
    State estimated_state = pf.getXHat();
    saveStateToCSV(estimated_state, "results\\estimated_results\\estimated_state_0.csv");
    saveStateToCSV(robot_state, "results\\true_state_results\\true_state_0.csv");
    pf.saveParticleStatesToFile("results\\pf_estimates\\pf_estimates_0.csv");

    double autual_reading = sensorFunction(robot_state); 
    double noisy_reading = autual_reading + sensor_noise_distribution(rng_generator);

    saveSensorReadingToCSV(autual_reading, "results\\sensor_readings\\actual_sensor_reading_0.csv");
    saveSensorReadingToCSV(noisy_reading, "results\\sensor_readings\\noisy_sensor_reading_0.csv");

    // Running the PF!
    for (int64_t i = 1; i < resamples; ++i)
    {
        #ifdef TRACY_ENABLE
            ZoneScopedN("Main Loop");
        #endif
        
        autual_reading = sensorFunction(robot_state); 
        noisy_reading = autual_reading + sensor_noise_distribution(rng_generator);

        saveSensorReadingToCSV(autual_reading, "results\\sensor_readings\\actual_sensor_reading_" +  std::to_string(i) + ".csv");
        saveSensorReadingToCSV(noisy_reading, "results\\sensor_readings\\noisy_sensor_reading_" +  std::to_string(i) + ".csv");

        std::cout << i << "---------------------------\n";
        saveStateToCSV(robot_state, "results\\true_state_results\\true_state_" + std::to_string(i) + ".csv");

        {
            #ifdef TRACY_ENABLE
                ZoneScopedN("Whole PF loop");
            #endif
            // 1. Update weights based on sensor reading
            // Technically steps 1-2 are combined but it's better if we split them to see the particle scores without making a whole extra copy of the particles to later process
            pf.updateWeights(noisy_reading, sensor_std_dev);

            State estimated_state = pf.getXHat();
            saveStateToCSV(estimated_state, "results\\estimated_results\\estimated_state_" + std::to_string(i) + ".csv");
            pf.saveParticleStatesToFile("results\\pf_estimates\\pf_estimates_" + std::to_string(i) + ".csv");

            calculateAndPrintError(estimated_state, robot_state);

            pf.propogateState(waypoint);                  // 2. Move particles based on control input
            pf.resample();                                // 3. Resample particles based on weights
            pf.mutateParticles(particle_propogation_std); // 4. Add some noise to particles
        }

        // PF operations done 
        moveActualState(robot_state, waypoint);
        
    }

    std::cout << "Main thread exiting." << std::endl;
}