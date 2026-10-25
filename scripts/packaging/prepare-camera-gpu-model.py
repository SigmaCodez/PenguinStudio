#!/usr/bin/env python3
"""Reproduce the fixed-shape GPU RVM graph; no training or weight changes.

Requires onnx 1.17.0, numpy, and onnxruntime 1.22.0/1.22.1 (CPU).
Input: author's rvm_mobilenetv3_fp32.onnx, release v1.0.0.
Output must be checked against the pinned package checksum before distribution.
"""
import argparse
import hashlib
from pathlib import Path

import numpy as np
import onnx
import onnxruntime as ort
from onnx import numpy_helper


def convert(source: Path, destination: Path) -> None:
    expected = "88d4531297118f595bf2fd60f6f566aec2e559393802d1f436c380f0cbbd2828"
    if hashlib.sha256(source.read_bytes()).hexdigest() != expected:
        raise ValueError("Unexpected upstream RVM model checksum")
    model = onnx.load(source)
    shapes = {"src": [1, 3, 192, 320], "downsample_ratio": [1]}
    shapes.update({f"r{i}i": [1, channels, 192 // 2**i, 320 // 2**i]
                   for i, channels in enumerate((16, 20, 40, 64), 1)})
    for value in model.graph.input:
        for dimension, size in zip(value.type.tensor_type.shape.dim, shapes[value.name]):
            dimension.ClearField("dim_param")
            dimension.dim_value = size
    model.graph.initializer.append(numpy_helper.from_array(
        np.array([1.0], np.float32), "penguin_fixed_ratio"))
    for node in model.graph.node:
        for index, name in enumerate(node.input):
            if name == "downsample_ratio":
                node.input[index] = "penguin_fixed_ratio"
    model = onnx.shape_inference.infer_shapes(model)
    options = ort.SessionOptions()
    options.intra_op_num_threads = 3
    options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_BASIC
    options.optimized_model_filepath = str(destination)
    ort.InferenceSession(model.SerializeToString(), options, providers=["CPUExecutionProvider"])
    print(hashlib.sha256(destination.read_bytes()).hexdigest())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    convert(args.source, args.destination)
