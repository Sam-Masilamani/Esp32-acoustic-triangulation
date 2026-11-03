# Acoustic Spatial Triangulation & Impact Localization System

## Overview
This repository contains the firmware and software for an experimental hardware physics project: a solid-state acoustic impact localization system. The system detects, measures, and triangulates physical impacts on a 200x200mm aluminum plate in real-time.

## Hardware Architecture
* **Sensors:** 3x3 Piezoelectric (PZT) sensor matrix.
* **Microcontroller:** ESP32-S3 for high-speed edge computing.
* **Signal Conditioning:** LM358 operational amplifiers for analog transient capture.

## Repository Structure

### 1. `/Firmware` (C++)
Contains the edge-computing code flashed to the ESP32-S3. 
* Utilizes direct 240 MHz CPU clock cycle counting to measure Time Difference of Arrival (TDOA) with microsecond precision.
* Captures analog peak transients without the latency of heavy machine learning overhead.
* Calculates solid-state acoustic wave velocity kinematics.

### 2. `/Dashboard` (Python)
Contains the multithreaded telemetry visualization software.
* **Libraries:** `PySerial`, `Matplotlib`, `NumPy`.
* Renders a live 2D spatial heatmap of the sensor matrix.
* Implements a physics-based Center of Mass weighted averaging heuristic to triangulate exact sub-centimeter impact coordinates.
* Estimates micro-Joule ($\mu J$) kinetic energy based on signal amplitude.

## Academic Context
Developed as an undergraduate experimental physics and instrumentation project at Loyola College, Chennai.
