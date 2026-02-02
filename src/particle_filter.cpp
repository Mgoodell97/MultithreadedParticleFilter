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

#ifdef TRACY_ENABLE
    #include "tracy/Tracy.hpp"
#endif

// Internal includes
#include "particle_filter.hpp"

// This exists to as a fast seed generator for per-thread random engines
// Aka to make particle mutation faster
static uint64_t splitmix64(uint64_t &seed)
{
    uint64_t z = (seed += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

ParticleFilter::ParticleFilter(const PF_Params& pf_params,
                               std::function<double(const double, const double, const double)> likelihood_function,
                               std::function<void(State&, const State&)> propagate_state_function,
                               const bool use_multithreading): 
    m_pf_params(pf_params), 
    m_num_particles(pf_params.num_of_particles),
    m_default_weights(m_pf_params.num_of_particles, 1.0 / static_cast<double>(m_pf_params.num_of_particles)),
    m_likelihood_function(likelihood_function),
    m_propagate_state_function(propagate_state_function), 
    m_use_multithreading(use_multithreading)
{
    if (m_use_multithreading)
    {
        const int64_t num_threads{static_cast<int64_t>(std::thread::hardware_concurrency())};
        m_pool = std::make_shared<ThreadPool>(num_threads);
        m_mutation_indicies_chunks = m_pool->getSplitWorkIndices(m_num_particles);
    }

    initializeVariables();

    this->initialize();
}

void ParticleFilter::initializeVariables()
{
    m_particles.resize(m_num_particles);
    m_particle_weights.resize(m_num_particles);
    m_particle_obersvations.resize(m_num_particles, 0.0);
    m_cumulative_weights_vector.resize(m_num_particles, 0.0);
    m_new_particles.resize(m_num_particles);
    m_mutation_indicies.resize(m_num_particles, 0); 
}

void ParticleFilter::initialize() 
{
    std::uniform_real_distribution<double> dist_x(X_MIN, X_MAX);
    std::uniform_real_distribution<double> dist_y(Y_MIN, Y_MAX);

    int64_t index = 0;
    for (int64_t i = 0; i < m_num_particles; i++) 
    {
        m_particles[i].x = dist_x(m_rand_eng);
        m_particles[i].y = dist_y(m_rand_eng);
        m_particle_weights[i] = 1.0/m_num_particles;
        index++;
    }
}

State ParticleFilter::getXHat() const
{
    if (m_use_multithreading)
    {
        return getXHatMultiThreaded();
    }
    else
    {
        return getXHatSingleThreaded();
    }
}

void ParticleFilter::mutateParticles(const std::vector<double>& std_dev)
{
    if (m_use_multithreading)
    {
        mutateParticlesMultiThreaded(std_dev);
    }
    else
    {
        mutateParticlesSingleThreded(std_dev);
    }
}

void ParticleFilter::propogateState(const State& waypoint)
{
    if (m_use_multithreading)
    {
        propogateStateMultiThreaded(waypoint);
    }
    else
    {
        propogateStateSingleThreaded(waypoint);
    }
}

void ParticleFilter::updateWeights(const double observation, const double sensor_std)
{
    if (m_use_multithreading)
    {
        updateWeightsMultiThreaded(observation, sensor_std);
    }
    else
    {
        updateWeightsSingleThreaded(observation, sensor_std);
    }
}

void ParticleFilter::resample()
{
    if (m_use_multithreading)
    {
        resampleMultiThreaded();
    }
    else
    {
        resampleSingleThreaded();
    }
}

void ParticleFilter::saveParticleStatesToFile(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    file << std::fixed << std::setprecision(6);
    file << "i,x,y,w" << "\n";

    for (int64_t i = 0; i < m_particles.size(); ++i)
    {
        if (i % 100 == 0) // Save every 100th particle to reduce file size
        {
            file << i << "," << m_particles[i].x << "," << m_particles[i].y << "," << m_particle_weights[i] << "\n";
        }
    }

    file.close();
}

// --------------- Private functions ---------------

State ParticleFilter::getXHatSingleThreaded() const
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("getXHatSingleThreaded");
    #endif

    State pf_estimate;
    pf_estimate.x = 0.0;
    pf_estimate.y = 0.0;

    for (int64_t i = 0; i < m_num_particles; i++) 
    {
        pf_estimate.x += m_particles[i].x * m_particle_weights[i];
        pf_estimate.y += m_particles[i].y * m_particle_weights[i];
    }

    return pf_estimate;
}

State ParticleFilter::getXHatMultiThreaded() const
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("getXHatMultiThreaded");
    #endif

    auto computeLocalXHat = [](const std::vector<State>& particles, const std::vector<double>& particle_weights, const std::vector<int64_t>& indices_chunk) 
    { 
        State local_estimate;
        local_estimate.x = 0.0;
        local_estimate.y = 0.0;
        for (int64_t i = indices_chunk.front(); i <= indices_chunk.back(); ++i)
        {
            local_estimate.x += particles[i].x * particle_weights[i];
            local_estimate.y += particles[i].y * particle_weights[i];
        }
        return local_estimate;
    };

    // Run local xHats
    std::vector<std::future<State>> futures;
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        auto future = m_pool->AddTask(computeLocalXHat, std::ref(m_particles), std::ref(m_particle_weights), std::ref(indices_chunk));
        futures.push_back(std::move(future));
    }
    State pf_estimate;
    pf_estimate.x = 0.0;
    pf_estimate.y = 0.0;
    m_pool->waitUntilAllTasksFinished();

    // Combine local xHats
    for (auto& future : futures)
    {
        const State local_estimate = future.get();
        pf_estimate.x += local_estimate.x;
        pf_estimate.y += local_estimate.y;
    }

    return pf_estimate;
}

void ParticleFilter::mutateParticlesSingleThreded(const std::vector<double>& std_dev)
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("mutateParticlesSingleThreded");
    #endif

    uint64_t seed = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());
    std::mt19937_64 eng(splitmix64(seed));
    std::normal_distribution<double> mutation_dx(0.0, std_dev[0]);
    std::normal_distribution<double> mutation_dy(0.0, std_dev[1]);

    for (int64_t i = 0; i < m_num_particles; i++) 
    {
        m_particles[i].x += mutation_dx(eng);
        m_particles[i].y += mutation_dy(eng);
    } 
}

void ParticleFilter::mutateParticlesMultiThreaded(const std::vector<double>& std_dev)
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("mutateParticlesMultiThreaded");
    #endif

    auto propagateParticleDistLocal = [this, std_dev](const std::vector<int64_t>& indices_chunk, 
                                                      uint64_t task_seed)
    {
        // create a local fast engine seed per-task
        uint64_t seed = task_seed;
        std::mt19937_64 eng(splitmix64(seed));

        // local distributions so there's no shared state
        std::normal_distribution<double> local_dx(0.0, std_dev[0]);
        std::normal_distribution<double> local_dy(0.0, std_dev[1]);

        for (int64_t i = indices_chunk.front(); i <= indices_chunk.back(); ++i)
        {
            m_particles[i].x += local_dx(eng);
            m_particles[i].y += local_dy(eng);
        }
    };

    std::vector<std::future<void>> futures;
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        // uint64_t seed_for_task = splitmix64(m_master_seed); // For fixed seed
        uint64_t seed_for_task = (static_cast<uint64_t>(rd()) << 32) ^ static_cast<uint64_t>(rd());

        auto future = m_pool->AddTask(propagateParticleDistLocal, 
                                        std::ref(indices_chunk),
                                        seed_for_task);
        futures.push_back(std::move(future));
    }
    m_pool->waitUntilAllTasksFinished();
}

void ParticleFilter::propogateStateSingleThreaded(const State& waypoint)
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("propogateStateSingleThreaded");
    #endif

    for (int64_t i = 0; i < m_num_particles; i++) 
    {
        m_propagate_state_function(m_particles[i], waypoint);
    }
}

void ParticleFilter::propogateStateMultiThreaded(const State& waypoint)
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("propogateStateMultiThreaded");
    #endif

    auto propagateParticles = [this](const std::vector<State>& particles,
                                     const State& waypoint,
                                     const std::vector<int64_t>& indices_chunk) 
    { 
        for (int64_t i = indices_chunk.front(); i <= indices_chunk.back(); ++i)
        {
            m_propagate_state_function(m_particles[i], waypoint);
        }
    };

    std::vector<std::future<void>> futures;
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        auto future = m_pool->AddTask(propagateParticles, 
                                        std::ref(m_particles), 
                                        waypoint, 
                                        std::ref(indices_chunk));
        futures.push_back(std::move(future));
    }
    m_pool->waitUntilAllTasksFinished();
}

void ParticleFilter::updateWeightsSingleThreaded(const double observation, const double sensor_std)
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("updateWeightsSingleThreaded");
    #endif

    for (int64_t i = 0; i < m_num_particles; i++) 
    {
        m_particle_obersvations[i] = sensorFunction(m_particles[i]);
    }

    for (int64_t i = 0; i < m_num_particles; i++) 
    {
        m_particle_weights[i] = m_likelihood_function(
            observation,
            m_particle_obersvations[i], 
            sensor_std
        );
    }

    normalizeWeights();
}

void ParticleFilter::updateWeightsMultiThreaded(const double observation, const double sensor_std)
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("updateWeightsMultiThreaded");
    #endif

    auto runParticlesThroughSensorFunction = [](const std::vector<State>& particles, std::vector<double>& particle_obersvations, const std::vector<int64_t>& indices_chunk) 
    { 
        for (int64_t i = indices_chunk.front(); i <= indices_chunk.back(); ++i)
        {
            particle_obersvations[i] = sensorFunction(particles[i]);
        }
    };

    auto computeLikelihoods = [this](const double observation, const double sensor_std, std::vector<double>& particle_obersvations, std::vector<double>& particle_weights, const std::vector<int64_t>& indices_chunk) 
    { 
        for (int64_t i = indices_chunk.front(); i <= indices_chunk.back(); ++i)
        {
            m_particle_weights[i] = m_likelihood_function(
                observation,
                particle_obersvations[i], 
                sensor_std);
        }
    };

    // Run sensor function in parallel
    std::vector<std::future<void>> futures;
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        auto future = m_pool->AddTask(runParticlesThroughSensorFunction, std::ref(m_particles), std::ref(m_particle_obersvations), std::ref(indices_chunk));
        futures.push_back(std::move(future));
    }
    m_pool->waitUntilAllTasksFinished();

    // Get likelihoods in parallel
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        auto future = m_pool->AddTask(computeLikelihoods, observation, sensor_std, std::ref(m_particle_obersvations), std::ref(m_particle_weights), std::ref(indices_chunk));
        futures.push_back(std::move(future));
    }
    m_pool->waitUntilAllTasksFinished();

    normalizeWeightsParallel();
}

void ParticleFilter::resampleSingleThreaded()
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("resampleSingleThreaded");
    #endif

    // Get the cumulative particle weights
    double cumulative_sum = 0.0;
    for (int64_t i = 0; i < m_num_particles; i++)
    {
        cumulative_sum += m_particle_weights[i];
        m_cumulative_weights_vector[i] = cumulative_sum;
    }

    // Generate values from 0.0 to 1/N
    const double wheel_spoke_step = cumulative_sum / static_cast<double>(m_num_particles);
    std::uniform_real_distribution<double> distribution{0.0, wheel_spoke_step};

    double wheel_spoke_start = distribution(m_rand_eng);

    // Now we make the wheel. Spin to win!
    int64_t index_candidate = 0;
    for (int64_t spoke_index = 0; spoke_index < m_num_particles; spoke_index++)
    {
        double wheel_spoke = wheel_spoke_start + static_cast<double>(wheel_spoke_step * static_cast<double>(spoke_index));
        while ((wheel_spoke - m_cumulative_weights_vector[index_candidate]) > 1e-10)
        {
            index_candidate += 1;
        }
        m_mutation_indicies[spoke_index]  = index_candidate;
    }

    // Mutate particles with wheel selections
    for (int64_t index = 0; index < m_num_particles; index++)
    {
        m_new_particles[index] = m_particles[m_mutation_indicies[index]];
    }

    m_particles = m_new_particles;
    m_particle_weights = m_default_weights;
}

void ParticleFilter::resampleMultiThreaded()
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("resampleMultiThreaded");
    #endif

    auto resampleChunk = [this](const std::vector<double>& cumulative_weights_vector, 
                            const double wheel_spoke_start, 
                            const double wheel_spoke_step, 
                            std::vector<int64_t>& mutation_indicies, 
                            const std::vector<int64_t>& indices_chunk) 
    { 
        double wheel_spoke = wheel_spoke_start + (wheel_spoke_step * static_cast<double>(indices_chunk.front()));

        // Binary search for the first index where cumulative_weights_vector[i] >= wheel_spoke
        const auto it = std::lower_bound(
            cumulative_weights_vector.begin(),
            cumulative_weights_vector.end(),
            wheel_spoke
        );
        int64_t index_candidate = std::distance(cumulative_weights_vector.begin(), it);

        for (const auto& spoke_index : indices_chunk)
        {
            wheel_spoke = wheel_spoke_start + (wheel_spoke_step * static_cast<double>(spoke_index));
            while ((wheel_spoke - cumulative_weights_vector[index_candidate]) > 1e-10)
            {
                index_candidate += 1;
            }
            mutation_indicies[spoke_index]  = index_candidate;
        }
    };

    auto assignNewParticles = [](const std::vector<int64_t>& mutation_indicies, 
                                 const std::vector<State>& old_particles, 
                                 std::vector<State>& new_particles,
                                 const std::vector<int64_t>& indices_chunk) 
    { 
        for (const auto& index : indices_chunk)
        {
            new_particles[index] = old_particles[mutation_indicies[index]];
        }
    };
    
    workEfficientParallelPrefixSum(m_particle_weights, m_cumulative_weights_vector);

    // Generate values from 0.0 to 1/N
    const double cumulative_sum = m_cumulative_weights_vector.back();
    const double wheel_spoke_step = cumulative_sum / static_cast<double>(m_num_particles);
    std::uniform_real_distribution<double> distribution{0.0, wheel_spoke_step};
    double wheel_spoke_start = distribution(m_rand_eng);

    // Mutation indicies for next generation
    // Run resampling in parallel
    std::vector<std::future<void>> futures;
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        auto future = m_pool->AddTask(resampleChunk, std::ref(m_cumulative_weights_vector), wheel_spoke_start, wheel_spoke_step, std::ref(m_mutation_indicies), std::ref(indices_chunk));
        futures.push_back(std::move(future));
    }
    m_pool->waitUntilAllTasksFinished();

    // Mutate particles with wheel selections
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        auto future = m_pool->AddTask(assignNewParticles, std::ref(m_mutation_indicies), std::ref(m_particles), std::ref(m_new_particles), std::ref(indices_chunk));
        futures.push_back(std::move(future));
    }
    m_pool->waitUntilAllTasksFinished();

    m_particles = m_new_particles;
    m_particle_weights = m_default_weights;
}

void ParticleFilter::workEfficientParallelPrefixSum(const std::vector<double>& input_vec, std::vector<double>& result)
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("workEfficientParallelPrefixSum");
    #endif
    // [start_index, end_index)
    auto localCumlativeSumFunc = [](const int64_t start_index, const int64_t end_index, const std::vector<double>& input_vec, std::vector<double>& result) 
    { 
        for (int64_t i = start_index; i < end_index; ++i)
        {
            if (i == start_index)
            {
                result[i] = input_vec[i];
            }
            else
            {
                result[i] = result[i - 1] + input_vec[i];
            }
        }
    };

    auto addEndIndexValue = [](const int64_t start_index, const int64_t end_index, const double end_index_value, std::vector<double>& result) 
    { 
        for (int64_t i = start_index; i < end_index; ++i)
        {
            result[i] += end_index_value;
        }
    };

    const int64_t n = input_vec.size();
    const int64_t p = std::min(m_pool->m_number_of_threads, n);

    // Step 1: Local cumulative sums
    std::vector<std::future<void>> futures;
    for (int64_t i = 0; i < p; ++i)
    {
        const int64_t start_index = std::floor((i * n)/(p));
        const int64_t end_index   = std::floor(((i+1) * n)/(p));
        auto future = m_pool->AddTask(localCumlativeSumFunc, start_index, end_index, std::ref(input_vec), std::ref(result));
        futures.push_back(std::move(future));
    }
    m_pool->waitUntilAllTasksFinished();

    // Step 2: Get end index values
    std::vector<double> end_index_values(p);
    for (int64_t i = 0; i < p; ++i)
    {
        const int64_t start_index = std::floor((i * n)/(p));
        const int64_t end_index   = std::floor(((i+1) * n)/(p));
        end_index_values[i] = result[end_index - 1];
    }
    localCumlativeSumFunc(0, end_index_values.size(), end_index_values, end_index_values);

    // Step 3: Add end index values to local sums
    for (int64_t i = 1; i < p; ++i)
    {
        const int64_t start_index = std::floor((i * n)/(p));
        const int64_t end_index   = std::floor(((i+1) * n)/(p));
        const double end_index_value = end_index_values[i-1];
        auto futures = m_pool->AddTask(addEndIndexValue, start_index, end_index, end_index_value, std::ref(result));
    }

    m_pool->waitUntilAllTasksFinished();
    // Done!
}

void ParticleFilter::normalizeWeights()
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("normalizeWeights");
    #endif

    double paritlce_weight_sum = std::accumulate(m_particle_weights.begin(), m_particle_weights.end(), 0.0);

    for (int64_t i = 0; i < m_num_particles; i++) 
    {
        m_particle_weights[i] /= paritlce_weight_sum;
    }
}

void ParticleFilter::normalizeWeightsParallel()
{
    #ifdef TRACY_ENABLE
        ZoneScopedN("normalizeWeightsParallel");
    #endif

    auto computeLocalSum = [](std::vector<double>& particle_weights, const std::vector<int64_t>& indices_chunk) 
    { 
        double local_sum = 0.0;
        for (int64_t i = indices_chunk.front(); i <= indices_chunk.back(); ++i)
        {
            local_sum += particle_weights[i];
        }
        return local_sum;
    };

    auto normalizeLocal = [](std::vector<double>& particle_weights, const double paritlce_weight_sum, const std::vector<int64_t>& indices_chunk) 
    { 
        double local_result = 0.0;
        for (int64_t i = indices_chunk.front(); i <= indices_chunk.back(); ++i)
        {
            particle_weights[i] /= paritlce_weight_sum;
        }
    };

    // Run summation in parallel
    std::vector<std::future<double>> futures;
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        auto future = m_pool->AddTask(computeLocalSum, std::ref(m_particle_weights), std::ref(indices_chunk));
        futures.push_back(std::move(future));
    }
    m_pool->waitUntilAllTasksFinished();

    // Combine local sums
    double paritlce_weight_sum = 0.0;
    for (auto& future : futures)
    {
        paritlce_weight_sum += future.get();
    }

    // Run normilization in parallel
    std::vector<std::future<void>> norm_futures;
    for (const auto& indices_chunk : m_mutation_indicies_chunks)
    {
        auto norm_future = m_pool->AddTask(normalizeLocal, std::ref(m_particle_weights), paritlce_weight_sum, std::ref(indices_chunk));
        norm_futures.push_back(std::move(norm_future));
    }
    m_pool->waitUntilAllTasksFinished();
}
