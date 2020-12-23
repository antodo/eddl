/*
* EDDL Library - European Distributed Deep Learning Library.
* Version: 0.8
* copyright (c) 2020, Universidad Politécnica de Valencia (UPV), PRHLT Research Centre
* Date: November 2020
* Author: PRHLT Research Centre, UPV, (rparedes@prhlt.upv.es), (jon@prhlt.upv.es)
* All rights reserved
*/

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "eddl/apis/eddl.h"


using namespace eddl;


//////////////////////////////////
// Text generation
// Only Decoder
//////////////////////////////////

layer ResBlock(layer l, int filters,int nconv,int half) {
  layer in=l;

  if (half)
      l=ReLu(BatchNormalization(Conv(l,filters,{3,3},{2,2})));
  else
      l=ReLu(BatchNormalization(Conv(l,filters,{3,3},{1,1})));


  for(int i=0;i<nconv-1;i++)
    l=ReLu(BatchNormalization(Conv(l,filters,{3,3},{1,1})));

  if (half)
    return Sum(BatchNormalization(Conv(in,filters,{1,1},{2,2})),l);
  else
    return Sum(l,in);
}


Tensor *onehot(Tensor *in, int vocs)
{
  int n=in->shape[0];
  int l=in->shape[1];
  int c=0;

  Tensor *out=new Tensor({n,l,vocs});
  out->fill_(0.0);

  int p=0;
  for(int i=0;i<n*l;i++,p+=vocs) {
    int w=in->ptr[i];
    if (w==0) c++;
    out->ptr[p+w]=1.0;
  }

  cout<<"padding="<<(100.0*c)/(n*l)<<"%"<<endl;
  return out;
}

int main(int argc, char **argv) {

    download_flickr();

    // Settings
    int epochs = 1;
    int batch_size = 24;

    int olength=20;
    int outvs=2000;
    int embdim=32;

    // Define network
    layer image_in = Input({3,256,256}); //Image
    layer l = image_in;

    l=ReLu(Conv(l,64,{3,3},{2,2}));

    l=ResBlock(l, 64,2,1);//<<<-- output half size
    l=ResBlock(l, 64,2,0);

    l=ResBlock(l, 128,2,1);//<<<-- output half size
    l=ResBlock(l, 128,2,0);

    l=ResBlock(l, 256,2,1);//<<<-- output half size
    l=ResBlock(l, 256,2,0);

    l=ResBlock(l, 512,2,1);//<<<-- output half size
    l=ResBlock(l, 512,2,0);

    l=GlobalAveragePool(l);

    layer lreshape=Reshape(l,{-1});


    // Decoder
    layer ldec = Input({outvs});
    ldec = ReduceArgMax(ldec,{0});
    ldec = RandomUniform(Embedding(ldec, outvs, 1,embdim),-0.05,0.05);

    ldec = Concat({ldec,lreshape});

    l = Decoder(LSTM(ldec,512,true));
    layer out = Softmax(Dense(l, outvs));

    model net = Model({image_in}, {out});
    plot(net, "model.pdf");

    optimizer opt=adam(0.001);
    //opt->set_clip_val(0.01);

    // Build model
    build(net,
          opt, // Optimizer
          {"softmax_cross_entropy"}, // Losses
          {"accuracy"}, // Metrics
          CS_GPU({1}) // one GPU
          //CS_GPU({1,1},100) // two GPU with weight sync every 100 batches
          //CS_CPU()
    );

    // View model
    summary(net);



    // Load dataset
    Tensor *x_train=Tensor::load("flickr_trX.bin","bin");
    x_train->info(); //1000,256,256,3

    Tensor *xtrain=Tensor::permute(x_train,{0,3,1,2});//1000,3,256,256

    Tensor *y_train=Tensor::load("flickr_trY.bin","bin");
    y_train->info();

    y_train=onehot(y_train,outvs);
    y_train->reshape_({y_train->shape[0],olength,outvs}); //batch x timesteps x input_dim
    y_train->info();

    // Train model
    for(int i=0;i<0;i++) {
      fit(net, {xtrain}, {y_train}, batch_size, 1);
    }


    /////////////////////////////////////////////
    // INFERENCE
    /////////////////////////////////////////////

    Tensor *timage=new Tensor({x_train->shape[0], 512}); //images reshape

    model cnn=Model({image_in},{lreshape});
    summary(cnn);
    plot(cnn,"cnn.pdf");
    build(cnn,adam(0.001),{"mse"},{"mse"},CS_GPU({1}),false);

    // forward images
    Tensor* xbatch = new Tensor({batch_size,3,256,256});

    int numbatches=x_train->shape[0]/batch_size;
    for(j=0;j<num_batches;j++)  {
        next_batch({x_train},{xbatch});
        forward(net,{xbatch});

        Tensor* ybatch=getOutput(lreshape);



        delete ybatch;
    }



    // Define only decoder for inference n-best
    layer ldecin = Input({outvs});
    layer image = Input({512});
    ldec = ReduceArgMax(ldecin,{0});
    ldec = RandomUniform(Embedding(ldec, outvs, 1,embdim),-0.05,0.05);

    ldec = Concat({ldec,image});

    l = LSTM(ldec,512,true);

    out = Softmax(Dense(l, outvs));

    model decoder=Model({ldecin,image},{out});
    plot(decoder, "decoder.pdf");

    // Build model
    build(decoder,
          adam(0.001), // Optimizer
          {"softmax_cross_entropy"}, // Losses
          {"accuracy"}, // Metrics
          CS_GPU({1}) // one GPU
          //CS_GPU({1,1},100) // two GPU with weight sync every 100 batches
          //CS_CPU()
    );
    // View model
    summary(decoder);

    copyParam(getLayer(net,"LSTM1"),getLayer(decoder,"LSTM2"));
    copyParam(getLayer(net,"dense1"),getLayer(decoder,"dense2"));
    copyParam(getLayer(net,"embedding1"),getLayer(decoder,"embedding2"));


   ////// N-best for sample

   int s=100; //sample 100





















}
