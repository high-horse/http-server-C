#!/bin/bash

# Number of concurrent runs
CONCURRENT=2000

# Path to your Python script
PYTHON_SCRIPT="request.py"

echo "Starting $CONCURRENT concurrent requests..."

for i in $(seq 1 $CONCURRENT); do
    python3 "$PYTHON_SCRIPT" &
done

# Wait for all background jobs to finish
wait

echo "All requests completed."