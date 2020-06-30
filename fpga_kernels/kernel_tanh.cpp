#include <math.h>
#include <stdio.h>
extern "C" {

/*void k_tanh(float *A, float *B, long int size){

  #pragma HLS INTERFACE m_axi port=A offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi port=B offset=slave bundle=gmem
  #pragma HLS INTERFACE s_axilite port=A  bundle=control
  #pragma HLS INTERFACE s_axilite port=B  bundle=control
  #pragma HLS INTERFACE s_axilite port=size bundle=control

  for (int i = 0; i < size; i++) {
    float p=exp(A[i]);
    float n=exp(-A[i]);
    B[i] = (p-n)/(p+n);
  }
}*/



#define DATA_SIZE 4096
#define BUFFER_SIZE 1024

// TRIPCOUNT identifiers
const unsigned int c_chunk_sz = BUFFER_SIZE;
const unsigned int c_size 

void k_tanh(float *A, float *B, long int size){

  #pragma HLS INTERFACE m_axi port=A offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi port=B offset=slave bundle=gmem
  #pragma HLS INTERFACE s_axilite port=A  bundle=control
  #pragma HLS INTERFACE s_axilite port=B  bundle=control
  #pragma HLS INTERFACE s_axilite port=size bundle=control
  #pragma HLS INTERFACE s_axilite port=return bundle=control

  float buffer[BUFFER_SIZE];

  for (int i=0; i<size; i=i+BUFFER_SIZE) {

    #pragma HLS LOOP_TRIPCOUNT min=c_size/c_chunk_sz max=c_size/c_chunk_sz
    int chunk_size = BUFFER_SIZE;
    // boundary checks
    if ((i + BUFFER_SIZE) > size)
      chunk_size = size - i;

    // burst read of A vector from global memory
    read1:
    for (int j=0; j<chunk_size; j++) {
      #pragma HLS LOOP_TRIPCOUNT min=c_chunk_sz max=c_chunk_sz
      buffer[j] = A[i + j];
    }
    
    /*for (int i = 0; i < size; i++) {
      float p=exp(A[i]);
      float n=exp(-A[i]);
      B[i] = (p-n)/(p+n);
    }*/
    tanh:
    for (int j=0; j<chunk_size; j++) {
      #pragma HLS PIPELINE II=1
      #pragma HLS UNROLL FACTOR=2
      #pragma HLS LOOP_TRIPCOUNT min=c_chunk_sz max=c_chunk_sz
      // perform operation
      float p = exp(buffer[j]);
      float n = exp(-buffer[j]); 
      buffer[j] = (p-n)/(p+n);
    }

    // burst write the result
    write:
    for (int j=0; j<chunk_size; j++) {
      #pragma HLS LOOP_TRIPCOUNT min=c_chunk_sz max=c_chunk_sz
      B[i+j] = buffer[j];
    }
  }
 

}


} // end extern "C"
