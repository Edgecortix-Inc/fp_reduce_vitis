# (C) Copyright 2020 EdgeCortix Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

#include <algorithm>
#include <cstdlib>
#include "xcl2.hpp"
#include "../common/fp_reduce.hpp"

static inline void usage(char **argv)
{
	std::cout << "Usage: " << argv[0] << " --size <Array size in floats> --bin <XCLBIN file> --verbose --verify" << std::endl;
}

int main(int argc, char **argv)
{
    int size;
    bool verbose = 0, verify = 0;
    std::string fpga_bitstream;
    int arg = 1;

	while (arg < argc)
	{
		if(strcmp(argv[arg], "--size") == 0)
		{
			size = std::atoi(argv[arg + 1]);
			arg += 2;
		}
		else if (strcmp(argv[arg], "--verbose") == 0)
		{
			verbose = 1;
			arg += 1;
		}
		else if (strcmp(argv[arg], "--verify") == 0)
		{
			verify = 1;
			arg += 1;
		}
		else if (strcmp(argv[arg], "--bin") == 0)
		{
			fpga_bitstream = argv[arg + 1];
			arg += 2;
		}
		else if (strcmp(argv[arg], "-h") == 0 || strcmp(argv[arg], "--help") == 0)
		{
			usage(argv);
			return EXIT_SUCCESS;
		}
		else
		{
			std::cout << "Invalid input!" << std::endl;
			usage(argv);
			return EXIT_FAILURE;
		}
	}

    cl_int err;
    cl::Context context;
    cl::CommandQueue queue;
    cl::Kernel fp_reduce_kernel;

    size_t size_bytes = sizeof(float) * size;
    int indexes = (int)size / VEC;
    if ((int)size % (VEC * LATENCY) != 0)
    {
        std::cout << "Number of input indexes must be a multiple of vector size * FADD latency (" << VEC << " * " << LATENCY << ")" << std::endl;
        return EXIT_FAILURE;
    }

    // Initialize context, create queue, program the FPGA and create kernel
    fpga_init(&context, &queue, &fp_reduce_kernel, fpga_bitstream, "fp_reduce", verbose);

    // Allocate device buffers
    if (verbose) std::cout << "Allocating buffers..." << std::endl;
    OCL_CHECK(err, cl::Buffer device_array(context, CL_MEM_READ_ONLY, size_bytes, NULL, &err));
    OCL_CHECK(err, cl::Buffer device_sum(context, CL_MEM_WRITE_ONLY, sizeof(float), NULL, &err));

    // Set kernel arguments
    OCL_CHECK(err, err = fp_reduce_kernel.setArg(0, device_array));
    OCL_CHECK(err, err = fp_reduce_kernel.setArg(1, device_sum));
    OCL_CHECK(err, err = fp_reduce_kernel.setArg(2, indexes));

    // Map device buffers in host
    VECTOR* host_array = (VECTOR *) queue.enqueueMapBuffer(device_array, CL_TRUE, CL_MAP_WRITE, 0, size_bytes, NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("%s:%d Error calling enqueueMapBuffer, error code is: %d\n", __FILE__,__LINE__, err);
        exit(EXIT_FAILURE);
    }
    float* host_sum = (float *) queue.enqueueMapBuffer(device_sum, CL_TRUE, CL_MAP_READ, 0, sizeof(float), NULL, NULL, &err);
    if (err != CL_SUCCESS)
    {
        printf("%s:%d Error calling enqueueMapBuffer, error code is: %d\n", __FILE__,__LINE__, err);
        exit(EXIT_FAILURE);
    }

    // Populate array from host using random numbers
    if (verbose) std::cout << "Generating random input data..." << std::endl;
    srand(7);
    for (int i = 0; i < indexes; i++)
    {
        for (int j = 0; j < VEC; j++)
        {
            host_array[i].array[j] = (float)rand() / (float)(RAND_MAX);
        }
    }

    // Migrate buffer from host to device
    OCL_CHECK(err, err = queue.enqueueMigrateMemObjects({device_array, device_sum}, 0));

    // Launch the kernel
    if (verbose) std::cout << "Executing kernel..." << std::endl;
    OCL_CHECK(err, err = queue.enqueueTask(fp_reduce_kernel));

    // Wait for kernel to finish execution
    OCL_CHECK(err, err = queue.finish());

    // Migrate output from device to host
    OCL_CHECK(err, err = queue.enqueueMigrateMemObjects({device_sum}, CL_MIGRATE_MEM_OBJECT_HOST));

    // Wait for migration to complete
    OCL_CHECK(err, err = queue.finish());

    float ref_sum = 0.0;
    if (verify)
    {
        if (verbose) std::cout << "Calculating reference output on CPU..." << std::endl;
        for (int i = 0; i < indexes; i++)
        {
            for (int j = 0; j < VEC; j++)
            {
                ref_sum += host_array[i].array[j];
            }
        }
    }

    printf("\nSum: %0.12f\n", *host_sum);
    if (verify) printf("Reference: %0.12f\n", ref_sum);

    return EXIT_SUCCESS;
}
