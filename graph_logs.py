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

PROXY_EVENTS = [
    "Packets Sent",
    "Packets Received",
    "Client to Server Delayed",
    "Server to Client Delayed",
    "Client to Server Dropped",
    "Server to Client Dropped"
]

PROXY_COLORS = ['#5cb85c', '#d9534f', '#f0ad4e', '#663399', '#f0ad4e', '#337ab7']

OTHER_EVENTS = [
    "Packets Sent",
    "Packets Received",
    "Failed to Receive ACK"
]

OTHER_COLORS = ['#5cb85c', '#d9534f', '#663399']

fig, ax = plt.subplots(figsize=(8, 6))

def read_and_process_logs():

    is_proxy = log_file.startswith("proxy")

    if is_proxy:
        sent = Counter()
        received = Counter()
        c2s_delayed = Counter()
        s2c_delayed = Counter()
        c2s_dropped = Counter()
        s2c_dropped = Counter()
    else:
        sent = Counter()
        received = Counter()
        failed = Counter()

    try:
        with open(log_file, "r") as f:
             for line in f:
                if "Sent" in line:
                    sent[1] += 1
                elif "Received" in line:
                    received[1] += 1
                elif is_proxy and "Delayed Client to Server" in line:
                    c2s_delayed[1] += 1
                elif is_proxy and "Delayed Server to Client" in line:
                    s2c_delayed[1] += 1
                elif is_proxy and "Dropped Client to Server" in line:
                    c2s_dropped[1] += 1
                elif is_proxy and "Dropped Server to Client" in line:
                    s2c_dropped[1] += 1
                elif not is_proxy and "Failed to receive ACK" in line:
                    failed[1] += 1
    except FileNotFoundError:
        print(f"Log file not found: {log_file}", file=sys.stderr)
        if is_proxy:
            return [0]*6
        else:
            return [0]*3

    if is_proxy:
        total_counts = [
            sum(sent.values()),
            sum(received.values()),
            sum(c2s_delayed.values()),
            sum(s2c_delayed.values()),
            sum(c2s_dropped.values()),
            sum(s2c_dropped.values())
        ]
    else:
        total_counts = [
            sum(sent.values()),
            sum(received.values()),
            sum(failed.values())
        ]

    return total_counts

def update(frame):
    
    new_counts = read_and_process_logs()
    
    ax.clear() 
    
    component_name = "System"
    
    if "client" in log_file:
        component_name = "Client"
        EVENTS = OTHER_EVENTS
        COLORS = OTHER_COLORS
    elif "proxy" in log_file:
        component_name = "Proxy"
        EVENTS = PROXY_EVENTS
        COLORS = PROXY_COLORS
    elif "server" in log_file:
        component_name = "Server"
        EVENTS = OTHER_EVENTS
        COLORS = OTHER_COLORS

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