############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2019 Xilinx, Inc. All Rights Reserved.
############################################################
open_project fp_reduce_hls_ip
set_top fp_reduce
add_files ../common/fp_reduce.hpp
add_files fp_reduce_kernel.cpp
add_files -tb fp_reduce_kernel_test.cpp -cflags "-Wno-unknown-pragmas" -csimflags "-Wno-unknown-pragmas"
open_solution "solution1" -reset
set_part {xczu7ev-ffvc1156-2-e} -tool vivado
create_clock -period 5 -name default
config_compile -name_max_length 50
csim_design -clean
csynth_design
#cosim_design #co-sim fails due to this bug: https://forums.xilinx.com/t5/High-Level-Synthesis-HLS/C-RTL-Cosimulation-hangs-on-wide-data-using-struct-axi-m/td-p/815908
#export_design -rtl verilog -format ip_catalog
