# PTXD
A decompiler for NVIDIA PTX ISA

## User Guide

Build ptxd:
```shell
make
```
<br>

## Development

Setup development environment:
```shell
git clone https://github.com/iamabhi747/ptxd
cd ptxd
make init
```
<br>

Run tests:
```shell
make test
```
<br>

Build research samples:<br>
requires `nvcc` & `cuobjdump` from NVIDIA CUDA Toolkit
```shell
make research