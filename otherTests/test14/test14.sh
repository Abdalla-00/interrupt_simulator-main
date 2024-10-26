#!/bin/bash

# Navigate to the parent directory where interrupts.c is located and compile it
gcc -o interrupts interrupts.c

# Run the interrupts executable with test3/trace3.txt as input
./interrupts otherTests/test14/trace14.txt

# Move the generated execution.txt to test3/execution3.txt
mv execution.txt otherTests/test14/execution14.txt

# Notify the user that the test is complete
echo "Test 14 completed. Check otherTests/test14/execution14.txt for the output."
