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

#include <string.h>
#include <hls_stream.h>

#define VEC 4           // Vector size; i.e. the total number of indexes processed per cycle
#define LATENCY 4 * VEC // Latency of the floating-point reduction loop based on the vector size VEC, and a target operating frequency of 200 MHz

typedef struct
{
    float array[VEC];
} VECTOR;

extern "C" {
void fp_reduce(VECTOR *in, float *out, int size);
}