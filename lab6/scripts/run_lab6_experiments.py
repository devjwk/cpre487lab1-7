#!/usr/bin/env python3
"""
Lab 6 Automated Experiment & Slide Plot Generator
Run this on the Lab Linux PC inside the `lab6/` directory:
    python3 scripts/run_lab6_experiments.py
"""

import csv
import os
import subprocess
import matplotlib.pyplot as plt


def plot_memory_access_patterns():
    """
    Generate the 3 Memory Access Pattern charts required by Lab 6 Section 3.1.2
    for Conv2 (Input: 60x60x32, Filter: 5x5x32x32, Output: 56x56x32).
    Compares Naive loop order (p,q,m,r,s,c) vs Cache-Optimized order (p,q,r,s,c,m).
    """
    W, C = 60, 32
    P, Q, M = 56, 56, 32
    R, S = 5, 5
    max_steps = 256

    naive_in, naive_w, naive_out = [], [], []
    opt_in, opt_w, opt_out = [], [], []

    # 1. Naive loop order: (p, q, m, r, s, c)
    steps = 0
    for p in range(P):
        for q in range(Q):
            for m in range(M):
                for r in range(R):
                    for s in range(S):
                        for c in range(C):
                            naive_in.append(((p + r) * W + (q + s)) * C + c)
                            naive_w.append(((r * S + s) * C + c) * M + m)
                            naive_out.append((p * Q + q) * M + m)
                            steps += 1
                            if steps >= max_steps:
                                break
                        if steps >= max_steps:
                            break
                    if steps >= max_steps:
                        break
                if steps >= max_steps:
                    break
            if steps >= max_steps:
                break
        if steps >= max_steps:
            break

    # 2. Cache-Optimized loop order: (p, q, r, s, c, m)
    steps = 0
    for p in range(P):
        for q in range(Q):
            for r in range(R):
                for s in range(S):
                    for c in range(C):
                        for m in range(M):
                            opt_in.append(((p + r) * W + (q + s)) * C + c)
                            opt_w.append(((r * S + s) * C + c) * M + m)
                            opt_out.append((p * Q + q) * M + m)
                            steps += 1
                            if steps >= max_steps:
                                break
                        if steps >= max_steps:
                            break
                    if steps >= max_steps:
                        break
            if steps >= max_steps:
                break
        if steps >= max_steps:
            break

    fig, axes = plt.subplots(1, 3, figsize=(16, 4.8))
    x = list(range(max_steps))

    # Weight Accesses
    axes[0].plot(x, naive_w, label="Naive (R,S,C,M) - Stride +32", color="#d62728", alpha=0.85, linewidth=1.5)
    axes[0].plot(x, opt_w, label="Cache-Optimized - Sequential (+1)", color="#2ca02c", linewidth=2.2)
    axes[0].set_title("Weight / Filter Memory Access Pattern (Conv2)")
    axes[0].set_xlabel("Computation Step (Inner Loop Iteration)")
    axes[0].set_ylabel("Flat Memory Array Index")
    axes[0].legend()
    axes[0].grid(True, linestyle="--", alpha=0.4)

    # Input Feature Map Accesses
    axes[1].plot(x, naive_in, label="Naive (Repeated Scan)", color="#d62728", alpha=0.85, linewidth=1.5)
    axes[1].plot(x, opt_in, label="Cache-Optimized (Register Reuse x32)", color="#1f77b4", linewidth=2.2)
    axes[1].set_title("Input Feature Map Access Pattern (Conv2)")
    axes[1].set_xlabel("Computation Step (Inner Loop Iteration)")
    axes[1].set_ylabel("Flat Memory Array Index")
    axes[1].legend()
    axes[1].grid(True, linestyle="--", alpha=0.4)

    # Output Feature Map Accesses
    axes[2].plot(x, naive_out, label="Naive (Single Element Held)", color="#d62728", alpha=0.85, linewidth=1.5)
    axes[2].plot(x, opt_out, label="Cache-Optimized (Unit-Stride Vector)", color="#ff7f0e", linewidth=2.2)
    axes[2].set_title("Output Feature Map Access Pattern (Conv2)")
    axes[2].set_xlabel("Computation Step (Inner Loop Iteration)")
    axes[2].set_ylabel("Flat Memory Array Index")
    axes[2].legend()
    axes[2].grid(True, linestyle="--", alpha=0.4)

    plt.tight_layout()
    plt.savefig("lab6_memory_access_patterns.png", dpi=200)
    plt.close()
    print("[+] Generated lab6_memory_access_patterns.png")


def plot_benchmark_csv(csv_path="lab6_benchmark_results.csv"):
    if not os.path.exists(csv_path):
        print(f"[!] {csv_path} not found. Building and running ./build/ml first...")
        subprocess.run(["make", "clean"], check=False)
        subprocess.run(["make"], check=True)
        subprocess.run(["./build/ml"], check=True)

    tiling_labels, tiling_ms = [], []
    thread_labels, thread_ms = [], []

    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            cat = row["category"]
            param = row["parameter"]
            lat = float(row["latency_ms"])
            if cat == "baseline":
                tiling_labels.append("Naive")
                tiling_ms.append(lat)
            elif cat == "reordered":
                tiling_labels.append("Reordered")
                tiling_ms.append(lat)
            elif cat == "tiling":
                tiling_labels.append(f"Tile {param}")
                tiling_ms.append(lat)
            elif cat == "threading":
                thread_labels.append(param)
                thread_ms.append(lat)

    # Plot 1: Tiling & Reordering Latency
    if tiling_ms:
        plt.figure(figsize=(8.5, 4.8))
        colors = ["#7f7f7f", "#2ca02c"] + ["#1f77b4"] * (len(tiling_ms) - 2)
        bars = plt.bar(tiling_labels, tiling_ms, color=colors, width=0.55)
        plt.title("Lab 6: Single Image Inference Latency vs. Tiling Block Size")
        plt.ylabel("Inference Latency (ms)")
        plt.xlabel("Implementation / Tile Block Size")
        plt.grid(axis="y", linestyle="--", alpha=0.4)
        for bar in bars:
            yval = bar.get_height()
            plt.text(bar.get_x() + bar.get_width() / 2.0, yval, f"{yval:.1f}ms", va="bottom", ha="center", fontsize=9)
        plt.tight_layout()
        plt.savefig("lab6_tiling_latency.png", dpi=200)
        plt.close()
        print("[+] Generated lab6_tiling_latency.png")

    # Plot 2: Multithreading Latency
    if thread_ms:
        plt.figure(figsize=(8.5, 4.8))
        bars = plt.bar(thread_labels, thread_ms, color="#ff7f0e", width=0.5)
        plt.title("Lab 6: Inference Latency vs. Number of Threads")
        plt.ylabel("Inference Latency (ms)")
        plt.xlabel("Number of Threads (std::thread)")
        plt.grid(axis="y", linestyle="--", alpha=0.4)
        for bar in bars:
            yval = bar.get_height()
            plt.text(bar.get_x() + bar.get_width() / 2.0, yval, f"{yval:.2f}ms", va="bottom", ha="center", fontsize=9)
        plt.tight_layout()
        plt.savefig("lab6_threading_latency.png", dpi=200)
        plt.close()
        print("[+] Generated lab6_threading_latency.png")


if __name__ == "__main__":
    plot_memory_access_patterns()
    plot_benchmark_csv()
