import matplotlib.pyplot as plt
import pandas as pd
import os

def create_chart():
    try:
        # Check if the CSV exists before trying to read it
        if not os.path.exists('results.csv'):
            print("Error: 'results.csv' not found. You must run the C++ program first!")
            return

        # Load the results
        df = pd.read_csv('results.csv')
        
        plt.figure(figsize=(10, 6))
        # Blue for FCFS, Green for SJF, Red for Round Robin
        colors = ['#3498db', '#2ecc71', '#e74c3c']
        
        bars = plt.bar(df['Algorithm'], df['WaitTime'], color=colors)
        
        # Formatting the chart
        plt.title('CPU Scheduling: Average Waiting Time Comparison', fontsize=14, pad=20)
        plt.xlabel('Scheduling Algorithm', fontsize=12)
        plt.ylabel('Avg Waiting Time (ms)', fontsize=12)
        plt.grid(axis='y', linestyle='--', alpha=0.7)

        # Adding text labels on top of each bar
        for bar in bars:
            yval = bar.get_height()
            plt.text(bar.get_x() + bar.get_width()/2, yval + 0.1, f'{yval:.2f}', 
                     ha='center', va='bottom', fontweight='bold')

        plt.tight_layout()

        output_file = 'barchart.png'
        plt.savefig(output_file)
        
        print(f"Chart saved as '{output_file}'.")
        
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    create_chart()