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
#include <fstream>

#include "helper_functions.hpp"

TEST(HelperFunctionTests, TestSaveStateToCSV)
{
    State test_state{
        .x = 10.0,
        .y = 15.0
    };
    std::filesystem::path filepath = std::filesystem::path("results") / "test_state.csv";

    saveStateToCSV(test_state, filepath);

    ASSERT_TRUE(std::filesystem::exists(filepath));

    std::ifstream file(filepath);
    ASSERT_TRUE(file.is_open());

    std::string header;
    std::getline(file, header);
    EXPECT_EQ(header, "i,x,y");

    std::string data;
    std::getline(file, data);
    EXPECT_EQ(data, "0,10,15");

    file.close();
    std::filesystem::remove(filepath);
}

TEST(HelperFunctionTests, TestSaveSensorReadingToCSV)
{
    double test_reading = 17.3;
    std::filesystem::path filepath = std::filesystem::path("results") / "test_reading.csv";

    saveSensorReadingToCSV(test_reading, filepath);

    ASSERT_TRUE(std::filesystem::exists(filepath));

    std::ifstream file(filepath);
    ASSERT_TRUE(file.is_open());

    std::string header;
    std::getline(file, header);
    EXPECT_EQ(header, "reading");

    std::string data;
    std::getline(file, data);
    EXPECT_EQ(data, "17.3");

    file.close();

    // Cleanup
    std::filesystem::remove(filepath);
}

TEST(HelperFunctionTests, TestSaveStateToCSV_FailsToOpen)
{
    std::filesystem::path dir = std::filesystem::path("results") / "results_state_fail";
    std::filesystem::create_directories(dir);

    std::filesystem::path filepath = dir / "test_state_fail.csv";

    {
        std::ofstream f(filepath);
        ASSERT_TRUE(f.is_open());
    }

    std::filesystem::permissions(
        filepath,
        std::filesystem::perms::owner_read,
        std::filesystem::perm_options::replace
    );

    State test_state{ .x = 1.0, .y = 2.0 };

    saveStateToCSV(test_state, filepath);

    std::ifstream file(filepath);
    ASSERT_TRUE(file.is_open());

    std::string header;
    std::getline(file, header);

    EXPECT_NE(header, "i,x,y");

    file.close();

    std::filesystem::permissions(
        filepath,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace
    );
    std::filesystem::remove_all(dir);
}

TEST(HelperFunctionTests, TestSaveSensorReadingToCSV_FailsToOpen)
{
    std::filesystem::path dir = std::filesystem::path("results") / "results_reading_fail";
    std::filesystem::create_directories(dir);

    std::filesystem::path filepath = dir / "test_reading_fail.csv";

    {
        std::ofstream f(filepath);
        ASSERT_TRUE(f.is_open());
    }

    std::filesystem::permissions(
        filepath,
        std::filesystem::perms::owner_read,
        std::filesystem::perm_options::replace
    );

    double test_reading = 17.3;

    saveSensorReadingToCSV(test_reading, filepath);

    std::ifstream file(filepath);
    ASSERT_TRUE(file.is_open());

    std::string header;
    std::getline(file, header);

    EXPECT_NE(header, "reading");

    file.close();

    std::filesystem::permissions(
        filepath,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace
    );
    std::filesystem::remove_all(dir);
}
