#!/bin/bash

# Step 1: Compile the executable
echo "Compiling source files..."

g++ -o ../interrupts ../interrupts.cpp

# Check if compilation succeeded
if [ $? -ne 0 ]; then
    echo "Compilation failed. Exiting!"
    exit 1
fi

# Step 2: Run the executable for each test folder
for folder in test{1..10}; do
    echo "Processing $folder..."

    for scheduler in FCFS PR RR; do
        echo " Running $scheduler scheduler..."

        # Define paths
        INPUT_FILE="$folder/input_data.txt"
        OUTPUT_DIR="$folder/$scheduler"

        # Ensure subfolders exist
        mkdir -p "$OUTPUT_DIR"

        # Run the executable
        ../interrupts "$INPUT_FILE" "$scheduler"

        # Move the generated files to the appropriate folder
        mv execution.txt "$OUTPUT_DIR/"
        mv memory_status.txt "$OUTPUT_DIR/"
        mv simulation_data.txt "$OUTPUT_DIR/"

        echo "      Results saved in $OUTPUT_DIR/"
    done
done

echo "All simulations completed!"
