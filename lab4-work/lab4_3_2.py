import csv
import tensorflow as tf
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

model = tf.keras.models.load_model(
    "CNN_TinyImageNet.h5",
    compile=False
)

layers = [
    layer for layer in model.layers
    if isinstance(layer, (tf.keras.layers.Conv2D, tf.keras.layers.Dense))
]

# Arrange four layers in each column to match the report layout
fig, axes = plt.subplots(4, 2, figsize=(12, 14))
rows = []

for i, layer in enumerate(layers):
    weights, biases = layer.get_weights()

   # Plot only the original FP32 weights
    ax = axes[i % 4, i // 4]
    ax.hist(weights.flatten(), bins=100)
    ax.set_title(f"{layer.name} Weight Distribution")
    ax.set_xlabel("Weight value (FP32)")
    ax.set_ylabel("Count")

    rows.append([
        layer.name,
        float(weights.min()),
        float(weights.max()),
        float(biases.min()),
        float(biases.max())
    ])

fig.tight_layout()
fig.savefig("weight_histograms.png", dpi=300)
plt.close(fig)

# Save the minimum and maximum values of weights and biases
header = ["Layer", "Weight Min", "Weight Max", "Bias Min", "Bias Max"]

with open("weight_bias_minmax.csv", "w", newline="") as file:
    writer = csv.writer(file)
    writer.writerow(header)
    writer.writerows(rows)

print(
    f"{'Layer':<14} {'Weight Min':>12} {'Weight Max':>12}"
    f" {'Bias Min':>12} {'Bias Max':>12}"
)

for name, w_min, w_max, b_min, b_max in rows:
    print(
        f"{name:<14} {w_min:>12.6f} {w_max:>12.6f}"
        f" {b_min:>12.6f} {b_max:>12.6f}"
    )

print("\nSaved: weight_histograms.png")
print("Saved: weight_bias_minmax.csv")