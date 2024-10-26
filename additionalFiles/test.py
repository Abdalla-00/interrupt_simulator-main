"""
Python script used to automate the calculating of cpu usage, io usage, and overhead ratios based on
the output trace files from running the simulator. Prints results to terminal.
"""

def parse_log_file(filename):
    # Initialize accumulators for CPU, I/O, and overhead times
    cpu_usage = 0
    io_activities = 0
    overhead = 0

    # Open the file and read the data
    with open(filename, "r") as file:
        lines = file.readlines()

    # Iterate through each line to calculate times
    for line in lines:
        # Split the line by commas to extract the timestamp, duration of event and event details
        parts = line.strip().split(", ")
        timestamp = int(parts[0])  # The start time of the event
        duration = int(parts[1])   # The duration of the event
        event_type = parts[2]      # The event description

        if "CPU usage" in event_type:
            cpu_usage += duration
        elif "END_IO" in event_type:
            io_activities += duration
        else:
            overhead += duration

    # Total time = CPU usage + I/O activities + overhead
    total_time = cpu_usage + io_activities + overhead

    # Calculate the ratios
    cpu_ratio = cpu_usage / total_time if total_time != 0 else 0
    io_ratio = io_activities / total_time if total_time != 0 else 0
    overhead_ratio = overhead / total_time if total_time != 0 else 0

    # Return the results
    return {
        "cpu_usage": cpu_usage,
        "io_activities": io_activities,
        "overhead": overhead,
        "total_time": total_time,
        "cpu_ratio": cpu_ratio,
        "io_ratio": io_ratio,
        "overhead_ratio": overhead_ratio,
    }

def printResults(results):
    # Print the results
    print(f"CPU Usage: {results['cpu_usage']} ms")
    print(f"I/O Activities: {results['io_activities']} ms")
    print(f"Overhead: {results['overhead']} ms")
    print(f"Total Time: {results['total_time']} ms")
    print(f"CPU Usage Ratio: {results['cpu_ratio']:.2%}")
    print(f"I/O Activities Ratio: {results['io_ratio']:.2%}")
    print(f"Overhead Ratio: {results['overhead_ratio']:.2%}")

if __name__ == "__main__":

    # Go through the execution output files in the test1 and test2 directories
    for i in range(1,3):
        filename = f"./test{i}/execution{i}.txt"
        try:
            result = parse_log_file(filename)
            print(f"Results for test{i}:")
            printResults(result)
            print("\n")
        except FileNotFoundError:
            print(f"File not found: {filename}")
            print("\n")


    # Go through the execution output files in the otherTests directory
    for i in range(3, 21):
        filename = f"./otherTests/test{i}/execution{i}.txt"
        try:
            result = parse_log_file(filename)
            print(f"Results for test{i}:")
            printResults(result)
            print("\n")
        except FileNotFoundError:
            print(f"File not found: {filename}")
            print("\n")



