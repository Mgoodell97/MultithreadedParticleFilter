#include <gtest/gtest.h>

#include "particle_filter.hpp"
#include "helper_functions.hpp"

class ParticleFilterTests : public testing::Test 
{
protected: 
    int64_t m_resamples = 50;
    PF_Params m_pf_params;
    double converged_eps = 1e-6;
    double non_converged_eps = 1.0;
};

class ParticleFilterParamsTests: public ParticleFilterTests, public testing::WithParamInterface<bool> {};

TEST_F(ParticleFilterTests, TestConstructionAndInitalization) 
{
    bool run_pf_in_parallel = true;
    ParticleFilter test_pf = ParticleFilter{m_pf_params, &likelihoodFunction, &moveEstimatedState, run_pf_in_parallel};
}

// TEST_F(ParticleFilterTests, TestGetXHat) 
// {
//     double l2_error_sum = 0.0;
//     double l2_error_count = 0.0;
//     std::vector<double> l2_errors;
//     for (int64_t i = 0; i < m_resamples; ++i)
//     {
//             bool run_pf_in_parallel = true;
//             ParticleFilter test_pf = ParticleFilter{m_pf_params, &likelihoodFunction, &moveEstimatedState, run_pf_in_parallel};
//             State estimate = test_pf.getXHat();

//             State actual_robot_state;
//             actual_robot_state.x = (X_MIN + X_MAX) / 2.0;
//             actual_robot_state.y = (Y_MIN + Y_MAX) / 2.0;
//             double l2_error = calculateError(estimate, actual_robot_state);
//             std::cerr << "x estimate: " << estimate.x << ", y estimate: " << estimate.y << "\n";
//             std::cerr << "L2 Error for run " << i << ": " << l2_error << "\n";
//             l2_errors.push_back(l2_error);
//             l2_error_sum += l2_error;
//             l2_error_count++;
//     }
//     // calculate std
//     double l2_std_sum = 0.0;
//     double l2_mean = l2_error_sum / l2_error_count;
//     for (const auto& err : l2_errors)
//     {
//         double diff = err - l2_mean;
//         l2_std_sum += diff * diff;
//     }
//     double l2_std = std::sqrt(l2_std_sum / l2_error_count);
//     std::cerr << "Average L2 Error over " << l2_error_count << "\n";
//     std::cerr << "STD: " << l2_std << "\n";
//     double expected_value = 50.0;

//     // EXPECT_NEAR(l2_error, expected_value, eps);
// }

TEST_P(ParticleFilterParamsTests, TestGetXHat)
{
    bool run_pf_in_parallel = GetParam();
    ParticleFilter test_pf = ParticleFilter{m_pf_params, &likelihoodFunction, &moveEstimatedState, run_pf_in_parallel};
    State estimate = test_pf.getXHat();

    State actual_robot_state = {
        .x = (X_MIN + X_MAX)/2.0,
        .y = (Y_MIN + Y_MAX)/2.0
    };
    double l2_error = calculateError(estimate, actual_robot_state);

    double expected_error = 0.0;
    EXPECT_NEAR(l2_error, expected_error, non_converged_eps);
}

INSTANTIATE_TEST_SUITE_P(GetXHat, ParticleFilterParamsTests, testing::Values(true,false));