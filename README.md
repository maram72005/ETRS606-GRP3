# ETRS606 - Embedded AI (Edge AI) Labs and Project

## Overview

This repository gathers the practical labs and project work carried out as part of the **ETRS606 - Embedded AI** module.

The main objective of this course is to design embedded systems combining **sensors, microcontrollers, connectivity, and artificial intelligence**, with a particular focus on **Edge AI deployment on STM32 platforms**.

Throughout the project, we study the main engineering trade-offs involved in deploying AI models on constrained hardware, including:

- memory footprint
- computational complexity
- inference latency
- model accuracy
- energy consumption

---

## Course Context

This project explores the full workflow of an embedded AI system:

- neural network design and training
- embedded programming on STM32
- sensor interfacing through I2C
- network communication with Ethernet
- cloud connectivity with ThingSpeak / MATLAB
- comparison between **Cloud AI** and **Edge AI**
- model deployment workflow from **TensorFlow** to **ONNX**, **MATLAB**, and **STM32**

---

## Hardware Used

The project is based on the following development boards and peripherals:

- **NUCLEO-N657X0**  
  ARM Cortex-M33 microcontroller up to 160 MHz, 320 KB RAM, 512 KB Flash, with advanced hardware resources including an NPU for AI inference.

- **NUCLEO-F446RE**  
  ARM Cortex-M4 microcontroller running at 180 MHz, with 128 KB RAM and 512 KB Flash.

- **X-NUCLEO-IKS01A3 Sensor Shield**  
  MEMS sensor expansion board including environmental and motion sensors such as:
  - temperature
  - humidity
  - pressure
  - acceleration
  - magnetic field

---

## Software and Tools

The following tools are used throughout the project:

- **Python**
- **TensorFlow / Keras**
- **STM32CubeIDE**
- **STM32CubeMX**
- **HAL drivers**
- **FreeRTOS**
- **LwIP**
- **ThingSpeak**
- **MATLAB**
- **ONNX**
- **X-CUBE-AI**

---

## Repository Structure

Recommended repository structure:

```text
ETRS606-GRP3/
├── README.md
├── TP1_MNIST/
├── TP2_STM32_Sensors_Ethernet/
├── TP3_Cloud_Connectivity/
├── TP4_Cloud_vs_Edge_AI/
└── docs/
