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

#pragma once

// Internal includes
#include "thread_pool.hpp"
#include "state_functions.hpp"

struct PF_Params
{
    int64_t num_of_particles{1000000};
    std::vector<double> starting_state_lower_bound{X_MIN,Y_MIN};
    std::vector<double> starting_state_upper_bound{X_MAX,Y_MAX};
    std::vector<double> particle_propogation_std{5,5};
};

class ParticleFilter {
public:
    ParticleFilter(const PF_Params& pf_params,
                   std::function<double(const double, const double, const double)> likelihood_function,
                   std::function<void(State&, const State&)> propagate_state_function,
                   const bool use_multithreading = true);
    void initialize();

    // Core PF functions
    State getXHat() const;
    void mutateParticles(const std::vector<double>& std_dev);
    void propogateState(const State& waypoint);
    void updateWeights(const double observation, const double sensor_std);
    void resample();

    // For visualizations
    void saveParticleStatesToFile(const std::string& filename) const;

private:
    State getXHatSingleThreaded() const;
    State getXHatMultiThreaded() const;

    void mutateParticlesSingleThreded(const std::vector<double>& std_dev);
    void mutateParticlesMultiThreaded(const std::vector<double>& std_dev);

    void propogateStateSingleThreaded(const State& waypoint);
    void propogateStateMultiThreaded(const State& waypoint);

    void updateWeightsSingleThreaded(const double observation, const double sensor_std);
    void updateWeightsMultiThreaded(const double observation, const double sensor_std);

    void resampleSingleThreaded();
    void resampleMultiThreaded();
    void workEfficientParallelPrefixSum(const std::vector<double>& input_output, std::vector<double>& result);

    void normalizeWeights();
    void normalizeWeightsParallel();

    void initializeVariables();

    PF_Params m_pf_params;
    int64_t m_num_particles; // This is in PF params but's it used enough it's worth having a direct copy
    std::vector<State> m_particles;
    std::vector<double> m_particle_weights;
    std::function<double(const double, const double, const double)> m_likelihood_function;
    std::function<void(State&, const State&)> m_propagate_state_function;
    std::default_random_engine m_rand_eng;
    std::vector<double> m_default_weights; // For fast reallocation of default weights after each resampleSingleThreaded

    // Variables used often so it's worth not initializing them each time
    State m_pf_estimate;
    std::vector<double> m_particle_obersvations;
    std::vector<double> m_cumulative_weights_vector;
    std::vector<State> m_new_particles; // Tmp storage for resampling
    std::vector<int64_t> m_mutation_indicies;

    // Multithreading variables
    bool m_use_multithreading = true;
    std::shared_ptr<ThreadPool> m_pool; // For parallel processing
    std::vector<std::vector<int64_t>> m_mutation_indicies_chunks;
};
