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

#include "state_functions.hpp"

TEST(StateFunctionTest, TestGenerateWaypoint) 
{
    const State waypoint = generateWaypoint();
    EXPECT_GE(waypoint.x, X_MIN);
    EXPECT_LE(waypoint.x, X_MAX);
    EXPECT_GE(waypoint.y, Y_MIN);
    EXPECT_LE(waypoint.y, Y_MAX);
}

TEST(StateFunctionTest, TestMoveActualState) 
{
    State test_state{
        .x = 10.0,
        .y = 15.0
    };
    State close_waypoint{
        .x = 11.0,
        .y = 15.0
    };
    State far_waypoint{
        .x = 50.0,
        .y = 15.0
    };

    moveActualState(test_state, close_waypoint);
    State expected_close_waypoint{
        .x = 11.0,
        .y = 15.0
    };
    EXPECT_NEAR(test_state.x, expected_close_waypoint.x, 1e-6);
    EXPECT_NEAR(test_state.y, expected_close_waypoint.y, 1e-6);

    moveActualState(test_state, far_waypoint);
    State expected_final_waypoint{
        .x = 13.0,
        .y = 15.0
    };
    EXPECT_NEAR(test_state.y, expected_final_waypoint.y, 1e-6);
    EXPECT_NEAR(test_state.y, expected_final_waypoint.y, 1e-6);
}