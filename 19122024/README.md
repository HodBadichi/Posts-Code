# Binary Search Benchmark

This project implements a binary search algorithm with a benchmarking framework to measure its performance. The benchmark generates a sorted array of configurable size and performs multiple random searches on it.

## Features

- Efficient binary search implementation
- Configurable array and search sizes
- Human-readable output with proper units (KB, MB, GB for sizes; µs, ms, s for time)
- Memory-efficient implementation
- Optimized compilation with GCC

## Building

To build the project, simply run:

```bash
make
```

To clean the build files:

```bash
make clean
```

## Usage

Run the benchmark with two parameters:
1. Size of the sorted array
2. Number of search operations to perform

Example:
```bash
./binary_search_benchmark 1000000 1000
```

This will:
- Create a sorted array of 1,000,000 elements
- Perform 1,000 random searches
- Display timing and size information

## Output

The program will display:
- Array size (in appropriate units)
- Number of searches performed
- Total execution time
- Average time per search

## Implementation Details

- The binary search is implemented iteratively for better performance
- The sorted array contains even numbers to ensure unique values
- Search keys are randomly generated within the range of possible values
- Time measurements are done using high-precision system timers
- Memory is properly managed with allocation and cleanup