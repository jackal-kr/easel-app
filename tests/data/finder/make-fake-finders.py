#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Makes the three fake subject finders the tests use (spec 0035, T004, T009): tiny ONNX models with the real model's input and output
names and shapes. fake-finder.onnx (side 64) answers with the input's red plane, so a red place is subject and anything else
background; slow-finder.onnx (side 1024) runs a chain of 200 7 x 7 convolutions so that a run lasts long enough to be cancelled;
fail-finder.onnx (side 64) loads but fails in every run: it gathers at an index made from the input (its largest value times 1000),
out of range, which no check at loading can see.
Needs the `onnx` package; run it from this folder. The output files are kept in the repository."""
import onnx
from onnx import TensorProto, helper, numpy_helper
import numpy as np

def model(side, nodes, inits, name, out_shape=True):
    inp = helper.make_tensor_value_info('input_image', TensorProto.FLOAT, [1, 3, side, side])
    out = helper.make_tensor_value_info('output', TensorProto.FLOAT, [1, 1, side, side] if out_shape else ['n', 'c', 'h'])
    graph = helper.make_graph(nodes, name, [inp], [out], inits)
    m = helper.make_model(graph, opset_imports=[helper.make_opsetid('', 17)], producer_name='easel-tests')
    m.ir_version = 9
    onnx.checker.check_model(m)
    onnx.save(m, name + '.onnx')

starts = numpy_helper.from_array(np.array([0], dtype=np.int64), 'starts')
ends = numpy_helper.from_array(np.array([1], dtype=np.int64), 'ends')
axes = numpy_helper.from_array(np.array([1], dtype=np.int64), 'axes')
model(64, [helper.make_node('Slice', ['input_image', 'starts', 'ends', 'axes'], ['output'])], [starts, ends, axes], 'fake-finder')

w = numpy_helper.from_array(np.full((1, 1, 7, 7), 1.0 / 49.0, dtype=np.float32), 'w')
nodes = [helper.make_node('Slice', ['input_image', 'starts', 'ends', 'axes'], ['c0'])]
for i in range(200):
    nodes.append(helper.make_node('Conv', [f'c{i}', 'w'], [f'c{i + 1}' if i < 199 else 'output'], pads=[3, 3, 3, 3]))
model(1024, nodes, [starts, ends, axes, w], 'slow-finder')

thousand = numpy_helper.from_array(np.array(1000.0, dtype=np.float32), 'thousand')
model(64, [helper.make_node('ReduceMax', ['input_image'], ['top'], keepdims=0),
           helper.make_node('Mul', ['top', 'thousand'], ['far']),
           helper.make_node('Cast', ['far'], ['index'], to=TensorProto.INT64),
           helper.make_node('Gather', ['input_image', 'index'], ['output'], axis=3)], [thousand], 'fail-finder', out_shape=False)
