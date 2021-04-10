// (C) Copyright 2020 EdgeCortix Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License. You may obtain
// a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.

#include "../common/fp_reduce.hpp"

const int vec = VEC;
const int latency = LATENCY;

void load(VECTOR *in, hls::stream<VECTOR> &out, int size);
void reduce(hls::stream<VECTOR> &in, float *out, int size);

extern "C" {
void fp_reduce(VECTOR *in, float *out, int size)
{
    #pragma HLS interface m_axi port=in  offset=slave num_read_outstanding=16 max_read_burst_length=16 bundle=gmem1
    #pragma HLS interface m_axi port=out offset=slave bundle=gmem2
    #pragma HLS data_pack variable=in
    #pragma HLS interface s_axilite port=in      bundle=control
    #pragma HLS interface s_axilite port=out     bundle=control
    #pragma HLS interface s_axilite port=size    bundle=control
    #pragma HLS interface s_axilite port=return  bundle=control

    static hls::stream<VECTOR> fifo;
    #pragma HLS stream variable=fifo depth=32

    #pragma HLS dataflow
    load(in, fifo, size);
    reduce(fifo, out, size);
}
}

void load(VECTOR *in, hls::stream<VECTOR> &out, int size)
{
    main: for(int i = 0; i < size; i++)
    {
        #pragma HLS pipeline II=1
        VECTOR in_temp;

        in_temp = in[i];

        out.write(in_temp);
    }
}

void reduce(hls::stream<VECTOR> &in, float *out, int size)
{
    float partial_sum[LATENCY];

    init: for(int i = 0; i < LATENCY; i++)
    {
        #pragma HLS unroll
        partial_sum[i] = 0;
    }

    main: for(int i = 0; i < size; i += LATENCY)
    {
        #pragma HLS pipeline II=latency

        partial_sum: for (int j = 0; j < LATENCY; j++)
        {
            #pragma HLS unroll
            VECTOR in_tmp = in.read();

            vector_compute: for (int k = 0; k < VEC; k++)
            {
                #pragma HLS unroll
                partial_sum[j] += in_tmp.array[k];
            }
        }
    }

    *out = 0;
    final_sum: for (int i = 0; i < LATENCY; i++)
    {
        #pragma HLS unroll
        *out += partial_sum[i];
    }
}

