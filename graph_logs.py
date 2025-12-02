import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.ticker as ticker
from collections import Counter
import sys
import time

if len(sys.argv) < 2:
    print("Usage: python3 graph_logs.py <logfile")
    sys.exit(1)

log_file = sys.argv[1]
update_interval_ms = 2000

EVENTS = ["Packets Sent", "Packets Received", "Fails to Receive ACK", "Proxy Delayed", "Proxy Dropped"]
COLORS = ['#5cb85c', '#d9534f', '#663399', '#f0ad4e', '#337ab7']

fig, ax = plt.subplots(figsize=(8, 6))

def read_and_process_logs():
    sent = Counter()
    received = Counter()
    failed = Counter()
    delayed = Counter()
    dropped = Counter()

    try:
        with open(log_file, "r") as f:
            for line in f:
                if "Sent" in line:
                    sent[1] += 1
                elif "Received" in line:
                    received[1] += 1
                elif "Failed to receive ACK" in line:
                    failed[1] += 1
                elif "Delayed" in line:
                    delayed[1] += 1
                elif "Dropped" in line:
                    dropped[1] += 1
    except FileNotFoundError:
        print(f"Log file not found: {log_file}", file=sys.stderr)
        return [0, 0, 0, 0, 0]

    total_counts = [
        sum(sent.values()),
        sum(received.values()),
        sum(failed.values()),
        sum(delayed.values()),
        sum(dropped.values())
    ]

    return total_counts

def update(frame):
    
    new_counts = read_and_process_logs()
    
    ax.clear() 
    
    component_name = "System" # Default name
    
    if log_file.startswith("client"):
        component_name = "Client"
    elif log_file.startswith("proxy"):
        component_name = "Proxy"
    elif log_file.startswith("server"):
        component_name = "Server"

    title = f"Live Total Event Counts Summary for {component_name} (Updated: {time.strftime('%H:%M:%S')})"

    if new_counts:
        max_count = max(new_counts)
        bars = ax.bar(EVENTS, new_counts, color=COLORS)
        
        ax.set_ylim(0, max_count * 1.1 + 1)
        ax.set_xlabel("Event Type")
        ax.set_ylabel("Total Event Count")
        ax.grid(axis='y', linestyle='--', alpha=0.6)
        
        ax.yaxis.set_major_locator(ticker.MaxNLocator(integer=True))

        for bar, count in zip(bars, new_counts):
            yval = bar.get_height()
            ax.text(bar.get_x() + bar.get_width()/2, yval + max_count * 0.02, 
                     int(yval), ha='center', va='bottom', fontsize=10)

    ax.set_title(title)

    return []

ani = animation.FuncAnimation(
    fig, 
    update, 
    interval=update_interval_ms, 
    blit=False, 
    cache_frame_data=False
)

plt.show()