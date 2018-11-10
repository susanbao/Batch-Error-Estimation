# Batch-Error-Estimation

Implementation of batch error estimation algorithm based on ABC (System for Sequential Logic Synthesis and Formal Verification)

## Getting Started

### Project Structure


- inc: header files
- src: source files
- lib: built library files and standard library cell files
- benchmark: circuit files saved in BLIF (Berkerley Logic Interface Format)
- in: input patterns for circuits files

### Install ABC

Please get ABC from https://github.com/berkeley-abc/abc
Build the static library libabc.a
And put libabc.a in the folder lib/
You can learn more about ABC from http://www.eecs.berkeley.edu/~alanmi/abc/

### Compling and Usage

make
./bee.out [circuit name] [error rate]
(./bee.out c432 0.01, for example)

### Bit-based logic simulation

See more from the function network_t::GetAllValues in network.cpp

### Batch error estimation

The idea is proposed in the paper "Efficient Batch Statistical Error Estimation for Iterative Multi-level Approximate Logic Synthesis",
https://ieeexplore.ieee.org/document/8465838

See more from the function network_t::BatchErrorEstimationPro in network.cpp
