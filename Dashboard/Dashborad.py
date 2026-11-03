import serial
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import threading
import sys
from matplotlib.ticker import MaxNLocator

# Restored default white background

# --- CONFIGURATION ---
COM_PORT = 'COM14'  
BAUD_RATE = 115200

sensor_coords = np.array([
    [20, 140],  [60, 180],  [140, 180], 
    [20, 60],   [100, 100], [180, 140], 
    [60, 20],   [140, 20],  [180, 60]   
])

heatmap_data = np.zeros((3, 3), dtype=int)
tdoa_data = [-1] * 9
deltas = [0] * 9
energy_history = []

data_ready = False
current_energy = 0.0
xy_coord = (0.0, 0.0)
acoustic_velocity = 0.0

def serial_listener(ser):
    global heatmap_data, tdoa_data, deltas, energy_history, data_ready, current_energy, xy_coord, acoustic_velocity
    temp_tdoas = [-1] * 9
    
    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            print(line) 
            
            if line.startswith("S") and "|" in line:
                parts = line.split('|')
                sensor_id = int(parts[0].replace('S', '').strip())
                delta_val = int(parts[3].replace('+', '').strip())
                deltas[sensor_id - 1] = delta_val
                
                tdoa_str = parts[4].replace('+', '').strip()
                if "---" not in tdoa_str:
                    temp_tdoas[sensor_id - 1] = int(tdoa_str)
                else:
                    temp_tdoas[sensor_id - 1] = -1
                
                if sensor_id == 9:
                    heatmap_data = np.array([
                        [deltas[0], deltas[1], deltas[2]],
                        [deltas[3], deltas[4], deltas[5]],
                        [deltas[6], deltas[7], deltas[8]]
                    ])
                    tdoa_data = list(temp_tdoas)

            elif "Calculated Energy" in line:
                current_energy = float(line.split(":")[1].split("uJ")[0].strip())
                energy_history.append(current_energy)
                
            elif "Impact Coordinates" in line:
                coord_parts = line.split("X:")[1].split("Y:")
                x_str = coord_parts[0].replace("mm,", "").strip()
                y_str = coord_parts[1].replace("mm", "").strip()
                xy_coord = (float(x_str), float(y_str))
                
            elif "Acoustic Velocity" in line:
                acoustic_velocity = float(line.split(":")[1].split("m/s")[0].strip())
                data_ready = True
                
        except Exception as e:
            pass

def command_sender(ser):
    while True:
        user_input = input("\n[Enter THRESHOLD]: ")
        if user_input.isdigit():
            ser.write(f"THRESH:{user_input}\n".encode('utf-8'))

if __name__ == "__main__":
    try:
        ser = serial.Serial(COM_PORT, BAUD_RATE)
        print(f"Serial Interface Active on {COM_PORT}.")
    except:
        print("Error: Port unavailable. Close Arduino Serial Monitor.")
        sys.exit()

    threading.Thread(target=serial_listener, args=(ser,), daemon=True).start()
    threading.Thread(target=command_sender, args=(ser,), daemon=True).start()

    plt.ion()
    
    fig1 = plt.figure(num="1. Spatial Amplitude Matrix", figsize=(5, 5))
    fig2 = plt.figure(num="2. Impact Localization", figsize=(5, 5))
    fig3 = plt.figure(num="3. Time Difference of Arrival Profile", figsize=(6, 4))
    fig4 = plt.figure(num="4. Kinetic Energy Dissipation", figsize=(6, 4))

    while True:
        if data_ready:
            # Figure 1: Spatial Amplitude Matrix
            fig1.clf()
            ax1 = fig1.add_subplot(111)
            sns.heatmap(heatmap_data, annot=True, fmt="d", cmap="YlOrRd", 
                        cbar_kws={'label': 'Amplitude (mV)'}, 
                        annot_kws={"size": 14, "weight": "bold"}, ax=ax1)
            ax1.set_title(f"Acoustic Amplitude Matrix\nSystem Energy: {current_energy} µJ")
            ax1.set_xticks([])
            ax1.set_yticks([])
            
            # Figure 2: Impact Localization
            fig2.clf()
            ax2 = fig2.add_subplot(111)
            ax2.scatter(sensor_coords[:, 0], sensor_coords[:, 1], c='gray', s=100, label="Sensor Nodes")
            ax2.scatter(xy_coord[0], xy_coord[1], c='red', s=300, marker='X', label="Center of Mass")
            
            for i, (x, y) in enumerate(sensor_coords):
                ax2.annotate(f"N{i+1}", (x+3, y+3), fontsize=9, color="gray")

            ax2.set_xlim(0, 200)
            ax2.set_ylim(0, 200)
            ax2.set_title(f"Center of Mass Localization (X: {xy_coord[0]:.1f}, Y: {xy_coord[1]:.1f})")
            ax2.set_xlabel("X-Axis Coordinates (mm)")
            ax2.set_ylabel("Y-Axis Coordinates (mm)")
            ax2.grid(True, linestyle='--', alpha=0.5)
            ax2.legend()
            
            # Figure 3: TDOA Profile
            fig3.clf()
            ax3 = fig3.add_subplot(111)
            sensors = ['N1', 'N2', 'N3', 'N4', 'N5', 'N6', 'N7', 'N8', 'N9']
            plot_tdoa = [0 if t < 0 else t for t in tdoa_data]
            ax3.bar(sensors, plot_tdoa, color='#2a9d8f')
            ax3.set_title(f"Temporal Propagation Delay | Wave Velocity: {acoustic_velocity:.1f} m/s")
            ax3.set_ylabel("Arrival Delay (µs)")
            ax3.grid(axis='y', linestyle='--', alpha=0.7)

            # Figure 4: Kinetic Energy Trend
            fig4.clf()
            ax4 = fig4.add_subplot(111)
            drops = list(range(1, len(energy_history) + 1))
            ax4.plot(drops, energy_history, marker='o', color='#e76f51', linewidth=2, markersize=8)
            ax4.set_title("Longitudinal Kinetic Energy Trend")
            ax4.set_xlabel("Impact Event Index")
            ax4.set_ylabel("Dissipated Energy (µJ)")
            ax4.grid(True, alpha=0.3)
            ax4.xaxis.set_major_locator(MaxNLocator(integer=True))
            
            fig1.canvas.draw_idle()
            fig2.canvas.draw_idle()
            fig3.canvas.draw_idle()
            fig4.canvas.draw_idle()
            
            fig1.canvas.flush_events()
            fig2.canvas.flush_events()
            fig3.canvas.flush_events()
            fig4.canvas.flush_events()
            
            data_ready = False
            
        plt.pause(0.1)