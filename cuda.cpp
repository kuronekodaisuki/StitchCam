/*
 * cuda.cpp
 *
 *  Created on: 2015/03/19
 *      Author: mao
 */


//
//
//
#include "cuda.h"
#include <cuda_runtime_api.h>

int cudaDeviceCount()
{
	int count;
	cudaGetDeviceCount(&count);
	return count;
}

