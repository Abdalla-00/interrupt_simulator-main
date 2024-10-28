#!/bin/bash

# Navigate to the parent directory where interrupts.c is located and compile it
gcc -o interrupts interrupts.c

# Run the interrupts executable with test2/trace2.txt as input
./interrupts test2/trace2.txt

# Move the generated execution.txt to test2/execution2.txt
mv execution.txt test2/execution2.txt

# Notify the user that the test is complete
echo "Test 2 completed. Check test2/execution2.txt for the output."
