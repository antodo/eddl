// This file is part of EDDLL an European Distributed Deep Learning Library.
// Developed within the DeepHealth project.
// Boosting AI in Europe.
//
// The MIT License (MIT)
//
// Copyright (c) 2019 Roberto Paredes Palacios, <rparedes@dsic.upv.es>

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <string.h>
#include <stdio.h>

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cublas_v2.h>

#include "../tensor.h"
#include "tensor_cuda.h"

extern cublasHandle_t p_cublas;

using namespace std;
void check_cublas(cublasStatus_t status, string func)
{
  if ( status!=  CUBLAS_STATUS_SUCCESS)
  {
     fprintf(stderr,"Error in cublas execution in %s\n",func);
     exit(1);
  }
}

void mult2D(Tensor *A, int tA, Tensor *B, int tB, Tensor *C,int incC)
{

float alfa=1.0;
float beta=(float)incC;

cublasOperation_t trA = CUBLAS_OP_N;
cublasOperation_t trB = CUBLAS_OP_N;

int ldA=A->sizes[1];
int ldB=B->sizes[1];
int ldC=B->sizes[1];
int m=B->sizes[1];
int n=A->sizes[0];
int k=B->sizes[0];

if (tA)
{
	trA = CUBLAS_OP_T;
	n=A->sizes[1];
}
if (tB)
{
	trB = CUBLAS_OP_T;
  m=B->sizes[0];
	k=B->sizes[1];
  ldC=B->sizes[0];
}

check_cublas(cublasSgemm(p_cublas,trB,trA,m,n,k,&alfa,B->gptr,ldB,A->gptr,ldA,&beta,C->gptr,ldC),"mult2D");

}