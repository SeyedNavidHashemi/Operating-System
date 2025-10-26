# multithreaded-audio-processing: A Performance Comparison

A comprehensive audio processing system that implements and compares serial and parallel implementations of various digital signal processing (DSP) filters. This project demonstrates the performance improvements achievable through parallel computing using POSIX threads.

## Overview

This project applies multiple audio filters (Band-pass, Notch, FIR, and IIR) to WAV audio files, comparing the execution time between sequential and multi-threaded implementations. It serves as a practical demonstration of parallel programming concepts and performance optimization techniques.

## Features

- **Four DSP Filters**: Band-pass, Notch, FIR, and IIR filters
- **Parallel Implementation**: Multi-threaded processing using POSIX threads
- **Performance Comparison**: Built-in timing mechanisms to measure execution time
- **WAV File Support**: Reads and writes standard audio files

## Technical Details

### Filters Implemented

1. **Band-pass Filter**: Filters frequencies within a specific range
2. **Notch Filter**: Removes specific frequency components
3. **FIR Filter**: Finite Impulse Response filter with configurable order
4. **IIR Filter**: Infinite Impulse Response filter with configurable parameters

### Parallelization Strategy

- Data partitioning across multiple threads
- Thread-safe processing with proper synchronization
- Configurable thread count (default: 25 threads)
- Minimal overhead through efficient thread management

## Project Structure

```
.
├── serial/          # Sequential implementation
│   ├── main.cpp
│   └── Makefile
├── parallel/        # Parallel implementation
│   ├── main.cpp
│   └── Makefile
├── input.wav        # Input audio file
├── OS-CA3.pdf       # Assignment document
└── README.md
```

## Requirements

- C++11 or higher
- libsndfile library for audio file I/O
- POSIX threads (pthread)
- Make build system
- Linux/Unix environment (or WSL on Windows)

## Installation

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install libsndfile1-dev build-essential
```

**macOS:**
```bash
brew install libsndfile
```

## Building the Project

### Serial Version
```bash
cd serial
make
```

### Parallel Version
```bash
cd parallel
make
```

## Usage

### Running the Serial Implementation
```bash
cd serial
./VoiceFilters.out ../input.wav
```

### Running the Parallel Implementation
```bash
cd parallel
./VoiceFilters.out ../input.wav
```

### Expected Output
```
Read: X.XX ms
Band-pass Filter: X.XX ms
Notch Filter: X.XX ms
FIR Filter: X.XX ms
IIR Filter: X.XX ms
Execution: X.XX ms
```

Output files will be generated in the current directory with the following naming convention:
- `output_BP_[ser|par].wav` - Band-pass filtered audio
- `output_Notch_[ser|par].wav` - Notch filtered audio
- `output_FIR_[ser|par].wav` - FIR filtered audio
- `output_IIR_[ser|par].wav` - IIR filtered audio

## Performance Analysis

The project is designed to measure and compare:
- Individual filter processing times
- Total execution time
- Parallel speedup compared to serial implementation

Key factors affecting performance:
- Audio file duration and sample rate
- Number of threads used
- System architecture and core count
- Memory bandwidth and cache efficiency

## Configuration

### Filter Parameters

Edit the constants in `main.cpp` to adjust filter behavior:

```cpp
const int delta_f = 2;      // Band-pass filter parameter
const int f0 = 50;          // Notch filter center frequency
const int n = 2;            // Notch filter order
const int M = 100;          // FIR filter order
const int N = 100;          // IIR filter order
const int NumOfThreads = 25; // Number of threads (parallel only)
```

## Technologies Used

- **Language**: C++
- **Parallel Programming**: POSIX Threads (pthread)
- **Audio Processing**: libsndfile
- **Build System**: Make

## Academic Context

This project was developed as a course assignment for an Operating Systems class, focusing on:
- Thread-based parallelization
- Performance optimization
- Digital signal processing
- System programming

## Future Enhancements

- GPU acceleration using CUDA or OpenCL
- Real-time audio processing capabilities
- Additional filter types
- Interactive parameter tuning
- Graphical user interface for visualization

## License

This project is for educational purposes only.
