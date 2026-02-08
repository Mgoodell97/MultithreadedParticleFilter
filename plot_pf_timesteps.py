# -*- coding: utf-8 -*-
"""
Custom Non‑Commercial License

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
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter
import os
from matplotlib.collections import LineCollection


plt.close('all')

x_lim = [0, 100]
y_lim = [0, 100]

show_parallel_results = True

class State:
    def __init__(self, x, y):
        self.x = x
        self.y = y

# Determine number of frames
def get_max_index():
    files = os.listdir("results/true_state_results")
    indices = [int(f.split('_')[2].split('.')[0]) for f in files if f.endswith('.csv')]
    return max(indices)

fig, (ax_main, ax_sensor) = plt.subplots(
    2, 1,
    figsize=(6, 8),
    gridspec_kw={'height_ratios': [4, 1]}
)


def print_error(frame, true_state, estimated_state):
    # Since we don't have all the particles this is a rough guess of our estimate not our true estimate
    error = np.sqrt((true_state.x-estimated_state.x)**2+(true_state.y-estimated_state.y)**2)
    print(f"Error at frame[{frame}] : {error}")

def update(index):
    ax_main.clear()
    ax_sensor.clear()
    pf_results_df = pd.DataFrame()
    
    true_state_df = pd.read_csv(f"results/true_state_results/true_state_{index}.csv")
    estimated_state_df = pd.read_csv(f"results/estimated_results/estimated_state_{index}.csv")
    pf_results_df = pd.read_csv(f"results/pf_estimates/pf_estimates_{index}.csv")

    true_state = State(*true_state_df.loc[0, ['x', 'y']])
    estimated_state = State(*estimated_state_df.loc[0, ['x', 'y']])
    px = pf_results_df['x'].values
    py = pf_results_df['y'].values
    pw = pf_results_df['w'].values

    print_error(index, true_state, estimated_state)
    
    ax_main.set_xlim(x_lim)
    ax_main.set_ylim(y_lim)
    ax_main.set_xlabel('x (m)')
    ax_main.set_ylabel('y (m)')
    ax_main.set_title(f'Particle filter at time {index}')

    ax_main.scatter(estimated_state.x, estimated_state.y, color='blue', zorder=5, label='Estimated state')
    ax_main.scatter(true_state.x, true_state.y, color='green', zorder=3, label='True state')

    # Normalize weights to max opacity of 0.05
    alpha_vals = 0.05 * (pw / np.max(pw))
    ax_main.scatter(px, py, color=(1, 0, 0, 1), alpha=alpha_vals, zorder=0)
    ax_main.scatter(px[0]-100, py[0]-100, color=(1, 0, 0, 1), alpha=0.5, zorder=0, label='Particles')

    ax_main.legend(loc='upper right')
    ax_main.grid(True)

    # Sensor plot
    actual_sensor_reading_df = pd.read_csv(f"results/sensor_readings//actual_sensor_reading_{index}.csv")
    noisy_sensor_reading_df = pd.read_csv(f"results/sensor_readings/noisy_sensor_reading_{index}.csv")
    
    actual_sensor_reading = actual_sensor_reading_df['reading'].iloc[0]
    noisy_sensor_reading = noisy_sensor_reading_df['reading'].iloc[0]
        
    ax_sensor.set_xlim([0, 150])
    ax_sensor.set_ylim([-3, 10])
    ax_sensor.set_yticks([])
    ax_sensor.set_xlabel('Sensor reading (m)')
    ax_sensor.plot([0, actual_sensor_reading], [1,1], label=f"Actual sensor reading: {round(actual_sensor_reading,2)}")
    ax_sensor.plot([0, noisy_sensor_reading], [0,0], label=f"Noisy sensor reading: {round(noisy_sensor_reading,2)}")
    
    ax_sensor.set_title("Sensor readings visualized")
    ax_sensor.legend(loc='upper right')
    ax_sensor.grid(True)
    
    plt.tight_layout()

# Create animation
n = get_max_index()
anim = FuncAnimation(fig, update, frames=range(n + 1), interval=200)

# Save as MP4
writer = FFMpegWriter(fps=5)
anim.save("particle_filter_animation.mp4", writer=writer)