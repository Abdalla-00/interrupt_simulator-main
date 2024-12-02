import os
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Path to tests folder
TESTS_FOLDER = "./tests"


def parse_memory_status(memory_status_path):
    memory_usage = []
    with open(memory_status_path, "r") as file:
        headers_skipped = False
        for line in file:
            if not headers_skipped:
                if line.startswith("| Time of Event"):
                    headers_skipped = True
                continue
            if "|" in line:
                parts = [p.strip() for p in line.split("|") if p.strip()]
                memory_usage.append({
                    "Time": int(parts[0]),
                    "MemoryUsed": int(parts[1]),
                    "TotalFreeMemory": int(parts[-2]),
                    "UsableFreeMemory": int(parts[-1])
                })
    return pd.DataFrame(memory_usage)


def analyze_simulation_results(algorithm_path):
    simulation_data_path = os.path.join(algorithm_path, "simulation_data.txt")
    memory_status_path = os.path.join(algorithm_path, "memory_status.txt")

    simulation_data = []
    total_runtime = 0
    with open(simulation_data_path, "r") as file:
        for line in file:
            if line.strip().isdigit():
                total_runtime = int(line.strip())
            else:
                data = {key_value.split(":")[0].strip(): key_value.split(":")[1].strip()
                        for key_value in line.strip().split(",")}
                simulation_data.append(data)

    df = pd.DataFrame(simulation_data)
    df = df.apply(pd.to_numeric, errors='ignore')

    df["TurnaroundTime"] = df["TurnaroundTime"].astype(str) + " ms"
    df["WaitTime"] = df["WaitTime"].astype(str) + " ms"
    df["ResponseTime"] = df["ResponseTime"].astype(str) + " ms"

    throughput = len(df) / (total_runtime / 1000)
    avg_turnaround_time = df["TurnaroundTime"].str.replace(" ms", "").astype(int).mean()
    avg_wait_time = df["WaitTime"].str.replace(" ms", "").astype(int).mean()
    avg_response_time = df["ResponseTime"].str.replace(" ms", "").astype(int).mean()
    total_cpu_execution_time = df["TotalCPUTime"].sum()

    memory_df = parse_memory_status(memory_status_path)
    peak_memory_used = memory_df["MemoryUsed"].max()
    avg_memory_used = memory_df["MemoryUsed"].mean()

    metrics = {
        "Throughput (processes/second)": throughput,
        "Average Turnaround Time (ms)": avg_turnaround_time,
        "Average Wait Time (ms)": avg_wait_time,
        "Average Response Time (ms)": avg_response_time,
        "Total CPU Execution Time (ms)": total_cpu_execution_time,
        "Peak Memory Used": peak_memory_used,
        "Average Memory Used": avg_memory_used
    }

    return metrics, df, memory_df


def plot_memory_usage(memory_df, test_name, algorithm_name):
    plt.figure(figsize=(12, 6))
    plt.plot(memory_df["Time"], memory_df["MemoryUsed"], label="Memory Used")
    plt.plot(memory_df["Time"], memory_df["TotalFreeMemory"], label="Total Free Memory")
    plt.plot(memory_df["Time"], memory_df["UsableFreeMemory"], label="Usable Free Memory")

    plt.title(f"Memory Usage Over Time for {test_name} - {algorithm_name}")
    plt.xlabel("Time (ms)")
    plt.ylabel("Memory (MB)")
    plt.legend()
    plt.grid()
    plt.tight_layout()
    plt.savefig(os.path.join(TESTS_FOLDER, test_name, algorithm_name, "memory_usage_plot.png"))
    plt.close()

def plot_metrics(metrics_df):
    # Loop through metrics
    for metric in ["Throughput (processes/second)", "Average Turnaround Time (ms)", 
                   "Average Wait Time (ms)", "Average Response Time (ms)", 
                   "Total CPU Execution Time (ms)", "Peak Memory Used", "Average Memory Used"]:
        plt.figure(figsize=(12, 6))
        sns.barplot(x="Test Name", y=metric, hue="Algorithm", data=metrics_df)
        plt.title(f"Comparison of {metric}")
        plt.ylabel(metric)
        plt.xticks(rotation=45)
        plt.tight_layout()

        # Sanitize file name and ensure directory exists
        sanitized_metric = metric.replace(" ", "_").replace("(", "").replace(")", "").replace("/", "_").lower()
        save_path = os.path.join(TESTS_FOLDER, f"{sanitized_metric}_comparison.png")
        os.makedirs(os.path.dirname(save_path), exist_ok=True)

        # Save the plot
        plt.savefig(save_path)
        plt.close()



def main():
    overall_metrics = []

    for test_name in os.listdir(TESTS_FOLDER):
        test_path = os.path.join(TESTS_FOLDER, test_name)
        if os.path.isdir(test_path):
            for algorithm_name in ["FCFS", "PR", "RR"]:
                algorithm_path = os.path.join(test_path, algorithm_name)
                if os.path.isdir(algorithm_path):
                    try:
                        metrics, df, memory_df = analyze_simulation_results(algorithm_path)
                        plot_memory_usage(memory_df, test_name, algorithm_name)
                        metrics["Test Name"] = test_name
                        metrics["Algorithm"] = algorithm_name
                        overall_metrics.append(metrics)

                        print(f"Results for {test_name} - {algorithm_name}:")
                        for metric, value in metrics.items():
                            print(f"  {metric}: {value}")
                        print()
                    except FileNotFoundError as e:
                        print(f"Error processing {test_name} - {algorithm_name}: {e}")

    metrics_df = pd.DataFrame(overall_metrics)
    print("\nOverall Metrics Across Simulations and Algorithms:")
    print(metrics_df)

    metrics_df.to_csv(os.path.join(TESTS_FOLDER, "overall_metrics.csv"), index=False)

    plot_metrics(metrics_df)


if __name__ == "__main__":
    main()
