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

#include "eddl/layers/core/layer_core.h"

using namespace std;

int LActivation::total_layers = 0;

LActivation::LActivation(Layer *parent, string act, vector<float> params, string name, int dev, int mem) : LinLayer(name, dev, mem){
    // Set default name
    if(name.empty()) this->name = act + to_string(++total_layers);

    this->act = act;
    this->params = params;

    input = parent->output;
#ifdef DEBUG_FPGA
    printf("creating output for RELU\n");
#endif
    output = new Tensor(input->shape, dev);
    delta_bp = 0;

    // Softmax checks
    if(this->act=="softmax"){
        // Set default axis if none was specified
        if(this->params.empty()){
            this->params.push_back(-1);
            std::cerr << "No axis for 'softmax' was specified. Using last one (-1) as default " << "(LActivation::Softmax)" << endl;
        }

        // Check number of axis
        if(this->params.size()>1){
            msg("Only one axis is supported ("  + std::to_string(this->params.size()) + " were specified)", "LActivation::Softmax");
        }

        // Replace -1 axis with last one
        int lastAxis = (int)input->shape.size()-1;
        if((int)this->params[0]==-1){
            this->params[0] = lastAxis;
        }

        // Check bounds
        if((int)this->params[0] <0 || (int)this->params[0]>lastAxis){
            msg("The axis has to be a number from 0 to (number_of_dimensions - 1)", "LActivation::Softmax");
        }
    }

    parent->addchild(this);
    addparent(parent);
}


void LActivation::forward(){

    if (act == "relu"){
        tensorNN::ReLu(this->input, this->output);

    }else if (act == "thresholded_relu"){
        float alpha = this->params[0];
        tensorNN::ThresholdedReLu(this->input, this->output, alpha);

    }else if (act == "elu"){
        float alpha = this->params[0];
        tensorNN::ELu(this->input, this->output, alpha);

    }else if (act == "selu"){
        // https://mlfromscratch.com/activation-functions-explained/#selu
        float alpha = this->params[0];
        float scale = this->params[1];

        tensorNN::ELu(this->input, this->output, alpha);
        this->output->mult_(scale);

    }else if (act == "exp"){
        tensorNN::Exp(this->input, this->output);

    }else if (act == "softplus"){
        tensorNN::Softplus(this->input, this->output);

    }else if (act == "softsign"){
        tensorNN::Softsign(this->input, this->output);

    }else if (act == "softmax_deprecated"){  // TODO: Deprecated
        tensorNN::Softmax(this->input, this->output);

    }else if (act == "softmax"){
        int axis = (int)this->params[0];
        tensorNN::FullSoftmax(this->input, this->output, axis);

    }else if (act == "sigmoid"){
        tensorNN::Sigmoid(this->input, this->output);

    }else if (act == "hard_sigmoid"){
        tensorNN::HardSigmoid(this->input, this->output);

    }else if (act == "leaky_relu"){
        float alpha = this->params[0];
        tensorNN::LeakyReLu(this->input, this->output, alpha);

    }else if (act == "tanh"){
        tensorNN::Tanh(this->input, this->output);

    }else if (act == "linear"){
        float alpha = this->params[0];
        tensorNN::Linear(this->input, this->output, alpha);
    }
}


void LActivation::backward(){
    if (delta_bp){
        Tensor::inc(delta, parent[0]->delta);
    }else {
        if (act == "relu"){
            tensorNN::D_ReLu(delta, input, parent[0]->delta);

        }else if (act == "thresholded_relu"){
            float alpha = this->params[0];
            tensorNN::D_ThresholdedReLu(delta, input, parent[0]->delta, alpha);

        }else if (act == "elu"){
            float alpha = this->params[0];
            tensorNN::D_ELu(delta, input, parent[0]->delta, alpha);

        }else if (act == "selu"){
            // https://mlfromscratch.com/activation-functions-explained/#selu
            float alpha = this->params[0];
            float scale = this->params[1];

            tensorNN::D_ELu(delta, input, parent[0]->delta, alpha);
            this->output->mult_(scale);

        }else if (act == "exp"){
            tensorNN::D_Exp(delta, output, parent[0]->delta);

        }else if (act == "softplus"){
            tensorNN::D_softplus(delta, output, parent[0]->delta);

        }else if (act == "softsign"){
            tensorNN::D_softsign(delta, output, parent[0]->delta);

        }else if (act == "softmax_deprecated"){  // TODO: Deprecaated
            tensorNN::D_Softmax(delta, output, parent[0]->delta);

        }else if (act == "softmax"){
            int axis = (int)this->params[0];
            tensorNN::D_FullSoftmax(delta, output, parent[0]->delta, axis);

        }else if (act == "sigmoid"){
            tensorNN::D_Sigmoid(delta, output, parent[0]->delta);

        }else if (act == "hard_sigmoid"){
            tensorNN::D_HardSigmoid(delta, input, parent[0]->delta);

        }else if (act == "leaky_relu"){
            float alpha = this->params[0];
            tensorNN::D_LeakyReLu(delta, input, parent[0]->delta, alpha);

        }else if (act == "tanh"){
            tensorNN::D_Tanh(delta, output, parent[0]->delta);

        }else if (act == "linear"){
            float alpha = this->params[0];
            tensorNN::D_Linear(delta, input, parent[0]->delta, alpha);
        }
    }
}


void LActivation::save(std::ofstream &ofs, string format){
    // Save act
    // Save param for "lrelu"
}

void LActivation::load(std::ifstream &ifs, string format){
    // Load act
    // Load param for "lrelu"
}

Layer *LActivation::share(int c, int bs, vector<Layer *> p){
    LActivation *n = new LActivation(p[0], this->act, this->params, "share_"+to_string(c)+this->name, this->dev, this->mem_level);
    n->orig = this;
    n->delta_bp = delta_bp;

    return n;
}

Layer *LActivation::clone(int c, int bs, vector<Layer *> p, int todev){

    LActivation *n = new LActivation(p[0], this->act, this->params,  "clone_" + name, todev, this->mem_level);
    n->orig = this;
    n->delta_bp = delta_bp;

    return n;
}


string LActivation::plot(int c){
    string s;

    if (c) s = name + " [label=" + "\"" + name + "\",style=filled,fontsize=12,fillcolor=LightBlue,shape=box]";
    else s = name + " [label=" + "\"" + name + "\",style=filled,fontsize=12,fillcolor=LightSalmon,shape=box]";

    return s;
}
