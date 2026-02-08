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

#include <gtest/gtest.h>

#include "particle_filter.hpp"
#include "helper_functions.hpp"

class ParticleFilterTests : public testing::Test 
{
void SetUp() override 
    {
        m_pf_params.num_of_particles = 100000;
        m_pf_params.starting_state_lower_bound = {X_MIN, Y_MIN};
        m_pf_params.starting_state_upper_bound = {X_MAX, Y_MAX};
        m_pf_params.particle_propogation_std = {0.1, 0.1};
        m_waypoint.x = 50;
        m_waypoint.y = 50;
        m_gt_robot_state.x = 10;
        m_gt_robot_state.y = 10;
    }

protected: 
    const int64_t m_resamples = 50;
    PF_Params m_pf_params;
    const double m_sensor_std_dev = 0.001; // Very accurate sensor
    const double m_intitial_error_eps = 1.0;
    const double m_first_estimate_eps = 5.0; // This is higher as we do not have enough information to fully converge yet
    const std::vector<double> m_error_thresholds = {5.0, 2.0, 1.0, 0.5, 1e-2}; // every 10 steps we reduce the error threshold
    const double m_converged_eps = 1e-4;
    State m_waypoint; // Ideally the robot starts at 10,10 and moves to 75,75
    State m_gt_robot_state;
};

class ParticleFilterParamsTests: public ParticleFilterTests, public testing::WithParamInterface<PF_THREAD_MODE> {};

TEST_F(ParticleFilterTests, TestConstructionAndInitalization) 
{
    bool run_pf_in_parallel = true;
    ParticleFilter test_pf = ParticleFilter{m_pf_params, &likelihoodFunction, &moveEstimatedState};
}

TEST_P(ParticleFilterParamsTests, TestGetXHat)
{
    PF_THREAD_MODE run_pf_in_parallel = GetParam();
    m_pf_params.thread_mode = run_pf_in_parallel;
    
    ParticleFilter test_pf = ParticleFilter{m_pf_params, &likelihoodFunction, &moveEstimatedState};
    State estimate = test_pf.getXHat();

    State initial_state_estimate = {
        .x = (X_MIN + X_MAX)/2.0,
        .y = (Y_MIN + Y_MAX)/2.0
    };
    double l2_error = calculateError(estimate, initial_state_estimate);
    double expected_error = 0.0;
    EXPECT_NEAR(l2_error, expected_error, m_intitial_error_eps);
}

TEST_P(ParticleFilterParamsTests, TestUpdateWeights)
{
    PF_THREAD_MODE run_pf_in_parallel = GetParam();
    m_pf_params.thread_mode = run_pf_in_parallel;

    ParticleFilter test_pf = ParticleFilter{m_pf_params, &likelihoodFunction, &moveEstimatedState};
    test_pf.updateWeights(sensorFunction(m_gt_robot_state), m_sensor_std_dev);
    State estimate = test_pf.getXHat();

    double l2_error = calculateError(estimate, m_gt_robot_state);
    double expected_error = 0.0;
    EXPECT_NEAR(l2_error, expected_error, m_first_estimate_eps);
}

TEST_P(ParticleFilterParamsTests, TestPropogateState)
{
    PF_THREAD_MODE run_pf_in_parallel = GetParam();
    m_pf_params.thread_mode = run_pf_in_parallel;

    ParticleFilter test_pf = ParticleFilter{m_pf_params, &likelihoodFunction, &moveEstimatedState};
    test_pf.updateWeights(sensorFunction(m_gt_robot_state), m_sensor_std_dev);
    test_pf.propogateState({m_waypoint}); // Limited by MAX_STEP_SIZE
    State estimate = test_pf.getXHat();

    moveEstimatedState(m_gt_robot_state, m_waypoint);
    double l2_error = calculateError(estimate, m_gt_robot_state);
    double expected_error = 0.0;
    EXPECT_NEAR(l2_error, expected_error, m_first_estimate_eps);
}

TEST_P(ParticleFilterParamsTests, TestFullParticleFilterLoop)
{
    PF_THREAD_MODE run_pf_in_parallel = GetParam();
    m_pf_params.thread_mode = run_pf_in_parallel;

    double expected_error = 0.0;
    ParticleFilter test_pf = ParticleFilter{m_pf_params, &likelihoodFunction, &moveEstimatedState};
    for (uint16_t i=0; i<m_resamples;i++)
    {
        test_pf.updateWeights(sensorFunction(m_gt_robot_state), m_sensor_std_dev);
        State estimate = test_pf.getXHat();
        double l2_error = calculateError(estimate, m_gt_robot_state);

        double error_threshold = m_error_thresholds[floor(i/10)];
        EXPECT_NEAR(l2_error, expected_error, error_threshold);

        test_pf.propogateState({m_waypoint});
        moveEstimatedState(m_gt_robot_state, m_waypoint);

        test_pf.resample();
        test_pf.mutateParticles(m_pf_params.particle_propogation_std);
    }
}

INSTANTIATE_TEST_SUITE_P(TestMultiAndSingleThreaded, ParticleFilterParamsTests, testing::Values(PF_THREAD_MODE::MULTI_THREADED_WITH_THREAD_POOL,PF_THREAD_MODE::SINGLE_THREADED));
