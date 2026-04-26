# TP4 - Cloud versus Edge AI

## Objective

This folder contains the work carried out for TP4 on the comparison between Cloud AI and Edge AI.

## Main Topics

- AI inference in the cloud
- AI inference on STM32
- TensorFlow model export
- ONNX conversion
- MATLAB inference
- X-CUBE-AI deployment
- Comparison of accuracy, latency, execution time, and power consumption

## Main Tasks

### Part 1 - Neural Network in the Cloud
- train a model in Python and TensorFlow
- export the trained model
- convert the model from TensorFlow to ONNX
- import the model into MATLAB
- run inference in the cloud environment
- use real sensor data collected from the STM32 board
- save classification results to a dedicated ThingSpeak channel
- use TalkBack to display the classification result on STM32

### Part 2 - Neural Network on STM32
- convert the trained model for embedded deployment
- use X-CUBE-AI for STM32 integration
- run inference directly on the STM32 board
- use real sensor data from the sensor board
- compare embedded inference with cloud inference
- evaluate accuracy and execution time
- measure or estimate power consumption during inference

## Comparison Criteria

- accuracy
- inference latency
- execution time
- memory constraints
- power consumption
- deployment complexity

## Status

In progress
