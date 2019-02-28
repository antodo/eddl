// This file is part of EDDLL an European Distributed Deep Learning Library.
// Developed within the DeepHealth project.
// Boosting AI in Europe.
//
// The MIT License (MIT)
//
// Copyright (c) 2019
//           Roberto Paredes Palacios, <rparedes@dsic.upv.es>
//           Jon Ander Gómez, <jon@dsic.upv.es>
//
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

#include <stdio.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "layer.h"

int activation_created=1;

using namespace std;

LActivation::LActivation(Layer *parent,string act):LActivation(parent,act,"activation"+to_string(activation_created),DEV_CPU){}
LActivation::LActivation(Layer *parent,string act,string name):LActivation(parent,act,name,DEV_CPU){}
LActivation::LActivation(Layer *parent,string act,int dev):LActivation(parent,act,"activation"+to_string(activation_created),dev){}
LActivation::LActivation(Layer *parent,string act,string name,int d):LinLayer(name,d)
{

  activation_created++;

  this->act=act;

  input=parent->output;
  output=new Tensor(input->getshape(),d);
  delta=new Tensor(output->getshape(),d);
  delta_bp=0;

  parent->addchild(this);
  addparent(parent);
}


// virtual
void LActivation::forward()
{

  if (act=="relu")
    Tensor::ReLu(input,output);
  else if (act=="softmax")
    {
      Tensor::Softmax(input,output);
    }

}


void LActivation::backward()
{


  if (parent.size())
    {
      if (delta_bp)
        {
          Tensor::inc(delta,parent[0]->delta);
        }
      else
        {
          if (act=="relu")
            Tensor::D_ReLu(delta,input,parent[0]->delta);
          else if (act=="softmax")
            Tensor::D_Softmax(delta,output,parent[0]->delta);
        }
    }
}


Layer *LActivation::share(int c,int bs,vector<Layer*>p)
{

  LActivation *n=new LActivation(p[0],act,"share_"+to_string(c)+name,dev);
  n->orig=this;
  n->delta_bp=delta_bp;

  return n;
}
Layer *LActivation::clone(int c,int bs,vector<Layer*>p,int todev)
{

  LActivation *n=new LActivation(p[0],act,"clone_"+to_string(todev)+name,todev);
  n->orig=this;
  n->delta_bp=delta_bp;

  return n;
}


void LActivation::info()
{
  cout<<"\n===============\n";
  cout<< "Layer LActivation "<<name<<"\n";
  cout<< "Parent layer:"<<parent[0]->name<<"\n";
  cout<< "Child layers:\n";
  if (child.size())
    for(int i = 0; i != child.size(); i++)
      cout<< child[i]->name<<"\n";
  else cout<<"None\n";
  cout<<"LInput:\n";
  input->info();
  cout<<"No Params\n";
  cout<<"Output:\n";
  output->info();
  cout<<"===============\n\n";
}
string LActivation::plot(int c)
{
    string s;

    if (c) s=name+" [label="+"\""+name+"\",style=filled,fontsize=12,fillcolor=bisque4,shape=box]";
    else s=name+" [label="+"\""+name+"\",style=filled,fontsize=12,fillcolor=LightSalmon,shape=box]";

    return s;
}