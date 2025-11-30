import os
import pandas as pd
import matplotlib.pyplot as plt


# Data processing helpers

# Load csv and compute median for each group
def load_and_reduce(csv_path: str):
    df = pd.read_csv(csv_path)

    median_df = (
        df.groupby(["capacity", "pattern", "object_size_profile"])[
            ["pool_op_latency_ns", "new_delete_op_latency_ns"]
        ]
        .median()
        .reset_index()
    )
    return median_df


# Sort rows: Batch-small, Batch-large, Rolling-small, Rolling-large
def order_groups(df: pd.DataFrame) -> pd.DataFrame:
    pattern_order = {"batch": 0, "rolling": 1}
    size_order = {"small": 0, "large": 1}

    df = df.sort_values(
        by=["capacity", "pattern", "object_size_profile"],
        key=lambda col: (
            col.map(pattern_order)
            if col.name == "pattern"
            else col.map(size_order)
            if col.name == "object_size_profile"
            else col
        ),
    )
    return df


# Add labels like 1024-B-S
def make_short_labels(df: pd.DataFrame) -> pd.DataFrame:
    def short_label(row):
        pat = "B" if row["pattern"] == "batch" else "R"
        size = "S" if row["object_size_profile"] == "small" else "L"
        return f"{row['capacity']}-{pat}-{size}"

    df["label"] = df.apply(short_label, axis=1)
    return df


# Plotting helpers

def make_figure():
    return plt.subplots(figsize=(16, 6))


def apply_y_grid(ax):
    ax.grid(axis="y", linestyle="dotted", color="gray", alpha=0.6)


def apply_common_x_axis(ax, df):
    x = list(range(len(df)))
    ax.set_xticks(x)
    ax.set_xticklabels(df["label"], rotation=55, ha="right")

    capacities = df["capacity"].unique()
    group_size = 4  # 4 patterns per capacity

    for i in range(1, len(capacities)):
        ax.axvline(i * group_size - 0.5, color="black", linewidth=0.4, alpha=0.4)

    return x


def add_caption(fig, text):
    plt.subplots_adjust(bottom=0.22)
    fig.text(
        0.5,
        0.015,
        text,
        ha="center",
        fontsize=9,
    )


# Chart 1: Median latency per operation
def plot_ns_per_op(csv_path: str):
    df = load_and_reduce(csv_path)
    df = order_groups(df)
    df = make_short_labels(df)

    fig, ax = make_figure()

    x = apply_common_x_axis(ax, df)
    apply_y_grid(ax)

    bar_width = 0.45

    ax.bar(
        [i - bar_width / 2 for i in x],
        df["pool_op_latency_ns"],
        width=bar_width,
        label="Arena pool",
    )
    ax.bar(
        [i + bar_width / 2 for i in x],
        df["new_delete_op_latency_ns"],
        width=bar_width,
        label="new/delete",
    )

    ax.set_ylabel("Median latency per operation (ns)")
    ax.set_title(
        "Median latency per operation (ns)\nArena pool vs new/delete",
        pad=18,
    )
    ax.legend()

    add_caption(
        fig,
        "Label format: <capacity>-<pattern>-<object size>, "
        "where B = batch, R = rolling; S = small objects, L = large objects.\n"
        "Chart shows median per-operation latency for each workload configuration (capacity x pattern x object size).",
    )

    fig.subplots_adjust(left=0.06, right=0.98)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    out_path = os.path.join(script_dir, "..", "plots", "latency-median-arena-vs-newdelete.png")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    fig.savefig(out_path, dpi=200)


# Chart 2 (Line): Speedup vs capacity
def plot_speedup_vs_capacity(csv_path: str):
    df = load_and_reduce(csv_path)

    df["speedup"] = df["new_delete_op_latency_ns"] / df["pool_op_latency_ns"]

    capacity_map = {
        1024: "1K",
        8192: "8K",
        65536: "64K",
        262144: "256K",
    }
    df["capacity_label"] = df["capacity"].map(capacity_map)

    df = df.sort_values("capacity")

    fig, ax = make_figure()

    combos = [
        ("batch", "small",  "Batch / Small",  "#1f77b4", 2.0),  
        ("batch", "large",  "Batch / Large",  "#ff7f0e", 2.0),  
        ("rolling", "small","Rolling / Small","#2ca02c", 2.0),
        ("rolling", "large","Rolling / Large","#d62728", 2.0),
    ]

    marker_size = 9

    for pattern, size, label, color, lw in combos:
        sub = df[(df["pattern"] == pattern) & (df["object_size_profile"] == size)]
        ax.plot(
            sub["capacity_label"],
            sub["speedup"],
            marker="o",
            markersize=marker_size,
            linewidth=lw,
            label=label,
            color=color,
        )

    ax.grid(axis="y", linestyle="dotted", color="gray", alpha=0.4, linewidth=0.35)

    ymax = df["speedup"].max()
    ax.set_ylim(0, ymax * 1.15)

    ax.set_ylabel("Speedup (x)")
    ax.set_xlabel("Capacity")
    ax.set_title(
        "Speedup of Arena pool over new/delete",
        pad=28,
    )

    ax.legend()

    add_caption(
        fig,
        "Rolling workloads show stable ~2x gains; batch workloads show larger improvements that taper with capacity."
    )

    fig.subplots_adjust(left=0.06, right=0.98)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    out_path = os.path.join(
        script_dir, "..", "plots", "speedup-vs-capacity.png"
    )
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    fig.savefig(out_path, dpi=200)


# Chart 3 (Heatmap): Speedup by pattern x object-size
def plot_speedup_heatmap(csv_path: str):
    df = load_and_reduce(csv_path)

    df["speedup"] = df["new_delete_op_latency_ns"] / df["pool_op_latency_ns"]

    agg = (
        df.groupby(["pattern", "object_size_profile"])["speedup"]
        .median()
        .reset_index()
    )

    pivot = agg.pivot(index="pattern", columns="object_size_profile", values="speedup")
    pivot = pivot.loc[["batch", "rolling"], ["small", "large"]]

    row_labels = ["Batch", "Rolling"]
    col_labels = ["Small", "Large"]

    fig, ax = plt.subplots(figsize=(8, 6))

    heatmap = ax.imshow(pivot.values, cmap="viridis")

    cbar = fig.colorbar(heatmap, ax=ax)
    cbar.set_label("Speedup (x)")

    ax.set_xticks(range(2))
    ax.set_yticks(range(2))
    ax.set_xticklabels(col_labels)
    ax.set_yticklabels(row_labels)

    for i in range(2):
        for j in range(2):
            val = pivot.values[i, j]
            text_color = "black" if i == 0 else "white"

            ax.text(
                j,
                i,
                f"{val:.1f}x",
                ha="center",
                va="center",
                fontsize=13,
                color=text_color,
            )

    ax.set_title(
        "Speedup heatmap: pattern x object size\nArena pool vs new/delete",
        pad=18,
    )

    add_caption(
        fig,
        "Heatmap shows median speedup aggregated across all capacities."
    )

    fig.subplots_adjust(left=0.15, right=0.95)

    script_dir = os.path.dirname(os.path.abspath(__file__))
    out_path = os.path.join(script_dir, "..", "plots", "speedup-pattern-vs-size.png")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    fig.savefig(out_path, dpi=200)


if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    csv_path = os.path.join(script_dir, "..", "results", "results.csv")
    plot_ns_per_op(csv_path)
    plot_speedup_vs_capacity(csv_path)
    plot_speedup_heatmap(csv_path)
