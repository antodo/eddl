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
#include <stdlib.h>
#include <iostream>
#include <cuda.h>

///////////////////////////////////////////
__global__ void fill(float *aptr,float *bptr,int t,int aini,int at,int bini,int bt,int tot,int inc)
{
  int i=blockIdx.x;
  int j=threadIdx.x;
  int k=blockIdx.y;

  int ap=(i*at)+((aini+j)*t)+k;
  int bp=(i*bt)+((bini+j)*t)+k;

  if (bp<tot)
    if (inc) bptr[bp]+=aptr[ap];
    else bptr[bp]=aptr[ap];

}


///////////////////////////////////////////
__global__ void sum_mat_row(float* a, float* b, float* c, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   c[thread_id_x]=a[thread_id_x]+b[thread_id_x%cols];

}
///////////////////////////////////////////
__global__ void sum_mat_col(float* a, float* b, float* c, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   c[thread_id_x]=a[thread_id_x]+b[thread_id_x/cols];

}
///////////////////////////////////////////
__global__ void set(float* a, float v, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   a[thread_id_x]=v;

}

///////////////////////////////////////////
__global__ void mult(float* a, float v, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   a[thread_id_x]*=v;

}
///////////////////////////////////////////
__global__ void el_mult(float* a, float *b, float *c, long int incC, long int rows, long int cols)
{
  long int ops=rows*cols;
  long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

  if (thread_id_x < ops)
    if (incC) c[thread_id_x]+=a[thread_id_x]*b[thread_id_x];
    else c[thread_id_x]=a[thread_id_x]*b[thread_id_x];
}

///////////////////////////////////////////
__global__ void el_div(float* a, float *b, float *c, long int incC, long int rows, long int cols)
{
  long int ops=rows*cols;
  long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

  if (thread_id_x < ops)
    if (incC) c[thread_id_x]+=a[thread_id_x]/(b[thread_id_x]);
    else c[thread_id_x]=a[thread_id_x]/(b[thread_id_x]);
}

///////////////////////////////////////////
__global__ void sum(float* a, float v, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   a[thread_id_x]+=v;

}
///////////////////////////////////////////
__global__ void sum(float scA,float* a,float scB,float *b, float *c,long int incC, long int size)
{
  long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

  if (thread_id_x < size) {
    if (incC) c[thread_id_x]+=scA*a[thread_id_x]+scB*b[thread_id_x];
    else c[thread_id_x]=scA*a[thread_id_x]+scB*b[thread_id_x];
  }
}

///////////////////////////////////////////
__global__ void reduce_array_sum(float* array, long int ops, long int cols,float* result)
{
  extern __shared__ float arr_acc[];
  __shared__ float accumulate_result[1];

  long int thread_id_x = threadIdx.x +blockIdx.x*blockDim.x;
  float sum=0;
  arr_acc[thread_id_x]=0.0;

  if(thread_id_x==0)
  	accumulate_result[thread_id_x]=0.0;

  __syncthreads();
  if (thread_id_x<ops)
  {
  	for (long int i=0; i<cols;i++)
    		sum+=array[thread_id_x*cols+i];

  __syncthreads();
    	arr_acc[thread_id_x]=sum;
  __syncthreads();

  }

  if (thread_id_x==0)
  {
  	for (long int i=0; i<ops;i++)
      accumulate_result[thread_id_x]+=arr_acc[thread_id_x+i];

    result[thread_id_x]=accumulate_result[thread_id_x];//copy back to global memory from shared

  }
}

///////////////////////////////////////////
__global__ void log(float* a, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   a[thread_id_x]=log(a[thread_id_x]);

}

///////////////////////////////////////////
__global__ void exp(float* a, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   a[thread_id_x]=exp(a[thread_id_x]);

}

///////////////////////////////////////////
__global__ void sqrt(float* a, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   a[thread_id_x]=sqrt(a[thread_id_x]);

}

///////////////////////////////////////////
__global__ void sqr(float* a, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   a[thread_id_x]*=a[thread_id_x];

}

///////////////////////////////////////////
__global__ void mask(float* a, float v, long int rows, long int cols)
{
 long int ops=rows*cols;
 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < ops)
   a[thread_id_x]=a[thread_id_x]<v;

}

///////////////////////////////////////////

__global__ void reduce_sum2D(float *a,float *b,long int rows,long int cols,long int axis)
{
  long int ops=rows*cols;
  long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

  if (thread_id_x < ops)
    if (axis==0)
        b[thread_id_x%cols]+=a[thread_id_x];
    else
        b[thread_id_x/cols]+=a[thread_id_x];
}
///////////////////////////////////////////
__global__ void cent(float* a, float* b, float* c, long int size)
{

 long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

 if (thread_id_x < size){
   c[thread_id_x]=0;
   if (a[thread_id_x]) c[thread_id_x]-=a[thread_id_x]*log(b[thread_id_x]);
   if (a[thread_id_x]!=1.0) c[thread_id_x]-=(1.0-a[thread_id_x])*log(1.0-b[thread_id_x]);
  }
}


__global__ void accuracy(float* T, float* N,float* acc,long int cols, long int total_ops, int* MC_err)
{

long int thread_id_x = threadIdx.x + blockIdx.x*blockDim.x;
long int result_t=T[thread_id_x*cols];
float result_n=N[thread_id_x*cols];

long int row_max_t=0;
long int row_max_n=0;

long int aux_t;
float aux_n;
if (thread_id_x < total_ops)
{
  for(long int i=1;i<cols;i++)
  {
   aux_t=T[thread_id_x*cols+i];
   aux_n=N[thread_id_x*cols+i];

	if (aux_t>result_t)
	 {
  		result_t=aux_t;
      row_max_t=i;
   }
  if (aux_n>result_n)
	 {
		result_n=aux_n;
    row_max_n=i;
   }
  }

  acc[thread_id_x]=row_max_t;
  atomicAdd(MC_err,(long int)(row_max_t==row_max_n));
}

}

///////////////////////////////////////////
__global__ void relu(float *a,float *b,long int size)
{
  long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

  if (thread_id_x < size){
    if (a[thread_id_x]>0.0) b[thread_id_x]=a[thread_id_x];
    else b[thread_id_x]=0.0;
   }
}


__global__ void d_relu(float *d,float *i,float *pd,long int size)
{
  long int thread_id_x = threadIdx.x+blockIdx.x*blockDim.x;

  if (thread_id_x < size){
    if (i[thread_id_x]>0.0) pd[thread_id_x]=d[thread_id_x];
    else pd[thread_id_x]=0.0;
   }

}

///////////////////////////////////////////
__global__ void softmax(float* E,float* N,float* auxE ,long int sample_ndim, long int n_vals)
{
    float C_value=0;
    long int thread_id_x = threadIdx.x + blockIdx.x*blockDim.x;
    float maxCoef = E[thread_id_x*sample_ndim];
    float actualCoef = 0;
    if (thread_id_x<n_vals)
    {

	    for (long int cA = 1; cA < sample_ndim; cA++)
    		if (E[thread_id_x*sample_ndim+cA] > maxCoef)
    			 maxCoef=E[thread_id_x*sample_ndim+cA];

	    for (long int cA = 0; cA < sample_ndim; cA++)
  		{
  			actualCoef=expf(E[thread_id_x*sample_ndim+cA]-maxCoef);
  			auxE[thread_id_x*sample_ndim+cA]=actualCoef;
        C_value+=actualCoef;
  		}

      for (long int cA=0; cA < sample_ndim; cA++)
	       N[thread_id_x*sample_ndim+cA]=auxE[thread_id_x*sample_ndim+cA]/C_value;
    }

}
























///////////////////////////////////////////


///////////////////////////////////////////


///////////////////////////////////////////