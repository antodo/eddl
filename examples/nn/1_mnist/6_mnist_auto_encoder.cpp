/*
* EDDL Library - European Distributed Deep Learning Library.
* Version: 0.7
* copyright (c) 2020, Universidad Politécnica de Valencia (UPV), PRHLT Research Centre
* Date: April 2020
* Author: PRHLT Research Centre, UPV, (rparedes@prhlt.upv.es), (jon@prhlt.upv.es)
* All rights reserved
*/

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "eddl/apis/eddl.h"


using namespace eddl;

//////////////////////////////////
// mnist_auto_encoder.cpp:
// A very basic MLP for mnist
// Using train_batch for training
// and eval_batch fot test
//////////////////////////////////

int main(int argc, char **argv) {

    // Download dataset
    download_mnist();

    // Settings
    int epochs = 5;
    int batch_size = 100;

    // Define network
    layer in = Input({784});
    layer l = in;  // Aux var

    l = Activation(Dense(l, 256), "relu");
    l = Activation(Dense(l, 128), "relu");
    l = Activation(Dense(l, 64), "relu");
    l = Activation(Dense(l, 128), "relu");
    l = Activation(Dense(l, 256), "relu");

    layer out = Dense(l, 784);

    model net = Model({in}, {out});

    // Build model
    build(net,
          sgd(0.001, 0.9), // Optimizer
          {"mean_squared_error"}, // Losses
          {"mean_squared_error"}, // Metrics
          CS_GPU({1}) // one GPU
          //CS_GPU({1,1},100) // two GPU with weight sync every 100 batches
          //CS_CPU()
          //CS_FPGA({1})
    );

    // View model
    summary(net);
    plot(net, "model.pdf");

    // Load dataset
    Tensor* x_train = Tensor::load("mnist_trX.bin");
    // Preprocessing
    x_train->div_(255.0f);

    // Train model
    fit(net, {x_train}, {x_train}, batch_size, epochs);

}
