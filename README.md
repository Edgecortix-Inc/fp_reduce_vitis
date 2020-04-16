(C) Copyright EdgeCortix Inc. 2020

# Floating-point Reduction with Xilinx Vitis
Full implementation of optimized floating-point reduction targeting Xilinx Zynq MPSoC and Alveo boards

The goal of this project is to understand basics of Vivado HLS and Xilinx Vitis, including basic C/C++
coding style, pragmas and interfaces for Xilinx HLS, and how to create a full system targeting a Xilinx
Zynq MPSoC board using the Xilinx Vitis flow which involves creating a host code using OpenCL. The
example will implement a vectorized floating-point reduction that works on an arbitrary sized array
of floats copied to DDR memory by the host, performs floating-point reduction on the FPGA, and returns
the final sum value to the host. The major optimization challenge on the kernel side is to address
the loop-carried dependency on the reduction variable while also providing the possibility of processing
multiple array indexes per clock in a vectorized fashion. On the host side, we will also want to make sure
buffers are actually shared between the ARM and the FPGA without any extra buffer copying.
Repository structure is as follows:

### Common

Includes `fp_reduce.hpp` header and Xilinx-provided OpenCL C++ header functions and make files.

### HLS Kernel

Includes the main HLS kernel, its testbench and run TCL script to use with Vivado HLS directly. The latter
two files are not required in the Vitis flow.

### Host

Includes OpenCL-based host code using Xilinx C++ header functions.

### Makefile

This is based on the Makefile from [Xilinx's Vitis example repository](https://github.com/Xilinx/Vitis_Accel_Examples)
but modified for our purpose. `make help` can be used to get the help for the Makefile.

### xrt.ini

This file is used to enable/disable profiling when running the application.

## Kernel Code Description

The kernel is comprised of two main `load` and `reduce` functions wrapped in a top-level `fp_reduce` function.
The `reduce` function implements a floating-point reduction over an arbitrary-sizes array that is passed to it
through an HLS stream (FIFO), and calculates the sum. Since floating-point addition has a latency higher than 1,
a loop initiation interval (II) of one cannot be achieved here. To compensate for this, multiple additions are
interleaved through the same adder unit to achieve a practical II of 1; i.e., for every one adder unit, X addition
are performed over X clocks, and each addition output is written to a different partial sum value. This technique
is proposed in [Xilinx's documentation](https://www.xilinx.com/support/answers/62859.html). On top of this, we
further vectorize the computation; i.e. we use as many adders per clock as the vector size (`fp_reduce.hpp -> VEC`)
so that VEC additions can be performed per cycle. However, this will further increase the latency of the loop
(`fp_reduce.hpp -> LATENCY`) and require a larger interleaving factor.  
  
For interfacing memory, we need to use the `m_axi` interface for pointer arguments to the kernel, and `s_axilite`
for value-based arguments. However, if we directly interface the `in` array with an `m_axi` interface in the
`reduce` kernel, the compiler will generate a very wide port to memory that is the size of `VEC * LATENCY * sizeof(float)`,
even though the II of the main loop is not 1. To avoid this, we create the extra `load` kernel to interface memory
and load a vector of size `VEC` (`#pragma HLS data_pack` is crucial here) per iteration in a loop with an II of 1,
and connect the `load` and `reduce` kernels using an HLS stream interface which acts a stallable FIFO between them.
These two kernels are instantiated in a top-level kernel with `#pragma HLS dataflow` to allow them to run
simultaneously and be synchronized using the FIFO.

## Host Code Description

The host code takes advantage of Xilinx's C++ headers similar to [Xilinx's own examples](https://github.com/Xilinx/Vitis_Accel_Examples)
but instead uses `enqueueMapBuffer()` and `enqueueMigrateMemObjects()` as suggested in [Xilinx's documentation](https://www.xilinx.com/html_docs/xilinx2019_2/vitis_doc/Chunk560402095.html#czb1555520653128)
to avoid extra buffer copy in SoC systems where the ARM and the FPGA share the same memory. The host application
takes the size of the input array (`--size $integer`) and the location of the FPGA bitstream (`--bin $path`) as input,
alongside with optional CPU verification (`--verify`) and verbosity flags (`--verbose`). After the OpenCL environment
is set up and the FPGA is programmed, the following steps are taken:

* Device buffers are created and mapped in the host memory space
* The input array is populated with random floating-point values
* The buffers are migrated to the device
* The kernel is executed
* The output buffer is migrated back to the host
* Kernel output is compared against CPU-only execution (optional)

## Command Examples
### Software/Hardware emulation
`make check TARGET=sw_emu DEVICE=xilinx_u250_xdma_201830_2 HOST_ARCH=x86`  
`make check TARGET=sw_emu DEVICE=zcu104_base HOST_ARCH=x86`  
Use `TARGET=hw_emu` for hardware emulation.

### Full compilation
Datacenter FPGA: `make all TARGET=hw DEVICE=xilinx_u250_xdma_201830_2`  
MPSoC FPGA: `make all TARGET=hw DEVICE=zcu104_base HOST_ARCH=aarch64 SYSROOT=/home/xilinx/petalinux_sdk/2019.2/sysroots/aarch64-xilinx-linux`

### Run command
`./fp_reduce_host --size 1600 --bin ./build.hw.zcu104_base/fp_reduce_kernel.xclbin --verbose --verify`
