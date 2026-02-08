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

#include <fstream>
#include <iostream>

#include "helper_functions.hpp"

void saveStateToCSV(const State& state, const std::filesystem::path& filepath)
{
    // Ensure parent directory exists
    std::filesystem::create_directories(filepath.parent_path());

    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filepath << std::endl;
        return;
    }

    file << "i,x,y\n";
    file << 0 << "," << state.x << "," << state.y << "\n";
}

void saveSensorReadingToCSV(double reading, const std::filesystem::path& filepath)
{
    std::filesystem::create_directories(filepath.parent_path());

    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filepath << std::endl;
        return;
    }

    file << "reading\n";
    file << reading << "\n";
}

double calculateError(const State& estimated_state, const State& true_state)
{
    double error_x = estimated_state.x - true_state.x;
    double error_y = estimated_state.y - true_state.y;

    return std::sqrt(error_x * error_x + error_y * error_y );
}

void ensure_parent_dir_exists(const std::filesystem::path& file_path)
{
    std::filesystem::create_directories(file_path.parent_path());
}

void createDirectoriesIfNotExist()
{
    std::vector<std::filesystem::path> directories = {std::filesystem::path("results"),
                                                      std::filesystem::path("results") / "estimated_results",
                                                      std::filesystem::path("results") / "true_state_results",
                                                      std::filesystem::path("results") / "pf_estimates",
                                                      std::filesystem::path("results") / "sensor_readings"};
    for (const auto& dir : directories)
    {
        std::filesystem::create_directories(dir);
    }
}