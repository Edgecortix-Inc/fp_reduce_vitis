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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../common/fp_reduce.hpp"

#define SIZE (LATENCY * VEC * 500)

int main()
{
    const int indexes = SIZE / VEC;
    VECTOR *data = (VECTOR*) malloc(indexes * sizeof(VECTOR));
    float ref = 0.0;

    srand(7);
    for (int i = 0; i < indexes; i++)
    {
        for (int j = 0; j < VEC; j++)
        {
            data[i].array[j] = (float)rand() / (float)(RAND_MAX);
            ref += data[i].array[j];
        }
    }

    float sum = 0;
    fp_reduce(data, &sum, indexes);

    printf("Sum: %0.12f\nReference: %0.12f\n", sum, ref);
}
