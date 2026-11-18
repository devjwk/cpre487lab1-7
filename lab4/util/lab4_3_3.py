import csv
import numpy as np
import tensorflow as tf
from tiny_imagenet import TinyImagenetDataset

# Run inference on the CPU
tf.config.set_visible_devices([], "GPU")

# Load the existing FP32 model
model = tf.keras.models.load_model(
    "CNN_TinyImageNet.h5",
    compile=False
)

# Collect the output of each layer
layers = [
    layer for layer in model.layers
    if not isinstance(layer, tf.keras.layers.InputLayer)
]

activation_model = tf.keras.Model(
    inputs=model.inputs,
    outputs=[layer.output for layer in layers]
)

# Reuse the prepared dataset when available
builder = TinyImagenetDataset()
builder.download_and_prepare(
    download_dir=str(__import__("pathlib").Path.home()
                     / "tensorflow-datasets" / "downloads")
)

train_size = builder.info.splits["train"].num_examples
assert train_size == 100000, f"Unexpected training set size: {train_size}"

train_raw = builder.as_dataset(
    split="train",
    shuffle_files=False
)

# Select 1,000 distinct training examples reproducibly
rng = np.random.default_rng(487)
selected_indices = rng.choice(
    train_size,
    size=1000,
    replace=False
)

np.save("selected_train_indices.npy", selected_indices)

selected_mask = np.zeros(train_size, dtype=bool)
selected_mask[selected_indices] = True
selected_mask = tf.constant(selected_mask)

# Normalize each selected image exactly once
selected_train = (
    train_raw.enumerate()
    .filter(lambda index, sample: selected_mask[index])
    .map(lambda index, sample:
         tf.cast(sample["image"], tf.float32) / 255.0)
    .batch(32)
)

# Track global activation ranges across all selected images
activation_min = [np.inf] * len(layers)
activation_max = [-np.inf] * len(layers)
processed_images = 0

for images in selected_train:
    outputs = activation_model(images, training=False)

    for i, output in enumerate(outputs):
        values = output.numpy()
        activation_min[i] = min(
            activation_min[i], float(values.min())
        )
        activation_max[i] = max(
            activation_max[i], float(values.max())
        )

    processed_images += int(images.shape[0])
    print(f"Processed images: {processed_images}/1000", flush=True)

assert processed_images == 1000

# Save one minimum and maximum per layer
with open("activation_minmax.csv", "w", newline="") as file:
    writer = csv.writer(file)
    writer.writerow(["Layer", "Activation Min", "Activation Max"])

    for layer, minimum, maximum in zip(
        layers, activation_min, activation_max
    ):
        writer.writerow([layer.name, minimum, maximum])

print(f"\n{'Layer':<20} {'Activation Min':>16} {'Activation Max':>16}")

for layer, minimum, maximum in zip(
    layers, activation_min, activation_max
):
    print(f"{layer.name:<20} {minimum:>16.8g} {maximum:>16.8g}")

print("\nSaved: activation_minmax.csv")
print("Saved: selected_train_indices.npy")