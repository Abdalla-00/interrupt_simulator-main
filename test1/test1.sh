#!/bin/bash

# Navigate to the parent directory where interrupts.c is located and compile it

gcc -o interrupts interrupts.c

# Run the interrupts executable with test1/trace1.txt as input
./interrupts test1/trace1.txt

# Move the generated execution.txt to test1/execution1.txt
mv execution.txt test1/execution1.txt

# Notify the user that the test is complete
echo "Test 1 completed. Check test1/execution1.txt for the output."