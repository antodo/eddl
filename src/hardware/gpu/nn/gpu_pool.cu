/*
* EDDL Library - European Distributed Deep Learning Library.
* Version: 0.8
* copyright (c) 2020, Universidad Politécnica de Valencia (UPV), PRHLT Research Centre
* Date: November 2020
* Author: PRHLT Research Centre, UPV, (rparedes@prhlt.upv.es), (jon@prhlt.upv.es)
* All rights reserved
*/

#include <cstdio>
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cublas_v2.h>

#include "eddl/hardware/gpu/nn/gpu_tensor_nn.h"
#include "eddl/hardware/gpu/nn/gpu_tensor_nn_kernels.h"

#include "eddl/hardware/gpu/gpu_hw.h"
#include "eddl/hardware/gpu/gpu_tensor.h"
#include "eddl/hardware/gpu/gpu_kernels.h"

#include "eddl/tensor/tensor.h"
#include "eddl/descriptors/descriptors.h"


void gpu_mpool2D(PoolDescriptor *D){
    int device=D->I->gpu_device;
    cudaSetDevice(device);

    setDims(D->O);
    maxpool2d<<<dimGrid,dimBlock>>>(D->I->ptr, D->I->shape[0],D->ir,D->ic,D->iz,D->kr,D->kc,D->O->ptr,D->r,D->c,D->z, D->sr,D->sc,D->padrt,D->padrb,D->padcl,D->padcr, D->indX->ptr, D->indY->ptr);

    check_cuda(cudaDeviceSynchronize(),"gpu_mpool");
}


void gpu_mpool2D_back(PoolDescriptor *D){
    int device=D->I->gpu_device;
    cudaSetDevice(device);

    setDims(D->D)
    maxpool2d_back<<<dimGrid,dimBlock>>>(D->D->ptr, D->ID->ptr, D->I->shape[0],D->ir,D->ic,D->iz,D->kr,D->kc,D->O->ptr,D->r,D->c,D->z, D->sr,D->sc,D->padrt,D->padrb,D->padcl,D->padcr, D->indX->ptr, D->indY->ptr);

    check_cuda(cudaDeviceSynchronize(),"gpu_mpool_back");
}


void gpu_avgpool2D(PoolDescriptor *D){
    int device=D->I->gpu_device;
    cudaSetDevice(device);

    setDims(D->O);
    avgpool2d<<<dimGrid,dimBlock>>>(D->I->ptr, D->I->shape[0], D->ir,D->ic,D->iz,D->kr,D->kc,D->O->ptr,D->r,D->c,D->z, D->sr,D->sc,D->padrt,D->padrb,D->padcl,D->padcr);

    check_cuda(cudaDeviceSynchronize(),"gpu_avgpool");
}

void gpu_avgpool2D_back(PoolDescriptor *D){
    int device=D->I->gpu_device;
    cudaSetDevice(device);

    setDims(D->D)
    avgpool2d_back<<<dimGrid,dimBlock>>>(D->D->ptr, D->ID->ptr, D->I->shape[0], D->ir,D->ic,D->iz,D->kr,D->kc,D->O->ptr,D->r,D->c,D->z, D->sr,D->sc,D->padrt,D->padrb,D->padcl,D->padcr);

    check_cuda(cudaDeviceSynchronize(),"gpu_avgpool_back");
}