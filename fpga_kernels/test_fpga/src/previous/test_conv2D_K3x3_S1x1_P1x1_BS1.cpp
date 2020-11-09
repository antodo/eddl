//
// test_conv2D.
//
// Constants:
//
//  - CPI
//  - CPO
//  - KW = 3
//  - KH = 3
//  - PW = 1
//  - PH = 1
//  - SW = 1
//  - SH = 1
//
//  Arguments:
//
//  - W
//  - H
//  - I
//  - O
//
//  Data formats:
//
//  - kernel   : GO x GI x CPO x CPI x KH x KW
//  - bias     : O
//  - data_in  : GI x H x W x CPI
//  - data_out : GO x H x W x CPO
//
//  GI = I / CPI
//  GO = O / CPO
//
//

#include <cstdio>      /* printf, scanf, NULL */
#include <cstdlib>     /* malloc, free, rand */

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include "xcl2.hpp"

#include <ap_fixed.h>
#include <sys/time.h>

using std::vector;

// data type
#define data_type float

// CL
cl::Buffer buf;
cl::Context context;
cl::CommandQueue q;
cl::Program program;
std::string binaryFile;

#define WMAX 256
#define HMAX 256
#define IMAX 512
#define OMAX 512

#define CPI 4
#define CPO 4

#define KW 3
#define KH 3

int W;
int H;
int GI;
int GO;
int I;
int O;


// buffers
data_type *data_in; //[  IMAX * W * H * CPI               ]  __attribute__ ((__aligned__(16)));
data_type *kernel;  //[  GO * GI * CPO * CPI * KW * KH    ]  __attribute__ ((__aligned__(16)));
data_type *bias;    //[  O                                ]  __attribute__ ((__aligned__(16)));
data_type *out;     //[  GO * W * H * CPO                 ]  __attribute__ ((__aligned__(16)));
data_type *out_cpu; //[  GO * W * H * CPO                 ]  __attribute__ ((__aligned__(16)));

void allocate_buffers() {
  data_in = (data_type*)malloc(I * W * H * sizeof(data_type));
  kernel = (data_type*)malloc(I * O * KW * KH * sizeof(data_type));
  bias = (data_type*)malloc(O * sizeof(data_type));
  out = (data_type*)malloc(O * W * H * sizeof(data_type));
  out_cpu = (data_type*)malloc(O * W * H * sizeof(data_type));
}

void parse_arguments(int argc, char **argv) {
  if (argc != 6) {
    printf("syntax:\n%s <XCLBIN File> <W> <H> <I> <O>\n", argv[0]);
    exit(1);
  }

  binaryFile = argv[1];  
  W = atoi(argv[2]);
  H = atoi(argv[3]);
  I = atoi(argv[4]);
  O = atoi(argv[5]);
  if ((I % CPI) != 0) {printf("Error, I must me multiple of %d\n", CPI); exit(1);}
  if ((O % CPO) != 0) {printf("Error, O must be multiple of %d\n", CPO); exit(1);}
  GI = I / CPI;
  GO = O / CPO;
}

void deallocate_buffers() {
  free(data_in);
  free(kernel);
  free(bias);
  free(out);
  free(out_cpu);
}

void cpu_conv2d() {

  int size_out = GO * W * H * CPO;
  for (int i=0; i<size_out; i++) out_cpu[i] = 0.f;

  for (int c=0; c<I; c++) {
    for (int cout=0; cout<O; cout++) {
      for (int h=0; h<H; h++) {
        for (int w=0; w<W; w++) {
          for (int kh=0; kh<KH; kh++) {
	    for (int kw=0; kw<KW; kw++) {
	      int data_h = (h-1)+kh;
	      int data_w = (w-1)+kw;
	      int padding = (data_h == -1) | (data_w == -1) | (data_w == W) | (data_h == H);
	      // kernel position
	      int gki = c / CPI;
	      int ki = c % CPI;
	      int gko = cout / CPO;
	      int ko = cout % CPO;
	      int addr_k = (gko * KW * KH * GI * CPO * CPI) +
		           (gki * KW * KH * CPO * CPI) +
			   (ko * KW * KH * CPI) +
			   (ki * KW * KH) +
			   (kh * KW) + kw;
	      // data_in pixel position
	      int gi = c / CPI;
	      int i = c % CPI;
              int addr_p = (gi * W * H * CPI) + (data_h * W * CPI) + (data_w * CPI) + i;
	      // data_out pixel position
	      int go = cout / CPO;
	      int o = cout % CPO;
	      int addr_o = (go * W * H * CPO) + (h * W * CPO) + (w * CPO) + o;
	      // operation
	      if (!padding) out_cpu[addr_o] += data_in[addr_p] * kernel[addr_k];
	    }
	  }
	}
      }
    }
  }

  // añadimos bias
  for (int cout=0; cout<O; cout++) {
    for (int h=0; h<H; h++) {
      for (int w=0; w<W; w++) {
	// data_out pixel position
	int go = cout / CPO;
	int o = cout % CPO;
        int addr_o = (go * W * H * CPO) + (h * W * CPO) + (w * CPO) + o;
	// bias operation
        out_cpu[addr_o] += bias[cout];
      }
    }
  }
}

void cpu_print_data_in() {
  printf("data in:\n");
  for (int c=0; c<I; c++) {
    printf(" channel %d:\n", c);
    printf("   ");
    for (int h=0; h<H; h++) {
      for (int w=0; w<W; w++) {
	// data_in pixel position
	int gi = c / CPI;
	int i = c % CPI;
	int addr_p = (gi * W * H * CPI) + (h * W * CPI) + (w * CPI) + i;
	//
        printf("%6.2f ", float(data_in[addr_p]));
      }
      printf("\n");
      printf("   ");
    }
    printf("\n");
  }
}

void cpu_print_kernels() {
  printf("kernels:\n");
  for (int cout=0; cout<O; cout++) {
    for (int c=0; c<I; c++) {
      printf("kernel c=%d cout %d:\n", c, cout);
      for (int kh=0; kh<KH; kh++) {
        for (int kw=0; kw<KW; kw++) {
           // kernel position
           int gki = c / CPI;
           int ki = c % CPI;
           int gko = cout / CPO;
           int ko = cout % CPO;
           int addr_k = (gko * KW * KH * GI * CPO * CPI) +
                        (gki * KW * KH * CPO * CPI) +
                        (ko * KW * KH * CPI) +
                        (ki * KW * KH) +
                        (kh * KW) + kw;
	  printf("%6.2f ", float(kernel[addr_k]));
	}
	printf("\n");
      }
    }
  }
}

void cpu_print_bias() {
  printf("bias:\n");
  for (int cout=0; cout<O; cout++) {
    printf("%6.2f ", float(bias[cout]));
  }
  printf("\n");
}

void cpu_print_out() {
  printf("output: cpu (fpga)\n");
  for (int cout=0; cout<O; cout++) {
    printf("channel %d:\n", cout);
    for (int h=0; h<H; h++) {
      for (int w=0; w<W; w++) {
	// data_out pixel position
	int go = cout / CPO;
	int o = cout % CPO;
        int addr_o = (go * W * H * CPO) + (h * W * CPO) + (w * CPO) + o;
        printf(" %10.6f (%10.6f) (diff %10.6f) | ", float(out_cpu[addr_o]), float(out[addr_o]), float(out_cpu[addr_o]-out[addr_o]));
      }
      printf("\n");
    }
  }
}

void check_result() {

  int error = 0;
  for (int cout=0; cout<O; cout++) {
    for (int h=0; h<H; h++) {
      for (int w=0; w<W; w++) {
	// data_out pixel position
	int go = cout / CPO;
	int o = cout % CPO;
        int addr_o = (go * W * H * CPO) + (h * W * CPO) + (w * CPO) + o;
        if (fabs(float(out_cpu[addr_o]) - float(out[addr_o])) > 0.001) {
          printf("Results mismatch at cout %d h %d w %d: %6.4f %6.4f (diff %6.4f)\n", cout, h, w, float(out_cpu[addr_o]), float(out[addr_o]), fabs(float(out_cpu[addr_o]-out[addr_o])));
          error = 1;
	  return;
	}
      }
    }
  }
  if (!error) printf("results OK!\n"); else {
    printf("results differ:\n");
    //cpu_print_out();
  }
}


//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

// An event callback function that prints the operations performed by the OpenCL
// runtime.
void event_cb(cl_event event1, cl_int cmd_status, void *data) {
  cl_int err;
  cl_command_type command;
  cl::Event event(event1, true);
  OCL_CHECK(err, err = event.getInfo(CL_EVENT_COMMAND_TYPE, &command));
  cl_int status;
  OCL_CHECK(err,
            err = event.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &status));
  const char *command_str;
  const char *status_str;
  switch (command) {
  case CL_COMMAND_READ_BUFFER:
    command_str = "buffer read";
    break;
  case CL_COMMAND_WRITE_BUFFER:
    command_str = "buffer write";
    break;
  case CL_COMMAND_NDRANGE_KERNEL:
    command_str = "kernel";
    break;
  case CL_COMMAND_MAP_BUFFER:
    command_str = "kernel";
    break;
  case CL_COMMAND_COPY_BUFFER:
    command_str = "kernel";
    break;
  case CL_COMMAND_MIGRATE_MEM_OBJECTS:
    command_str = "buffer migrate";
    break;
  default:
    command_str = "unknown";
  }
  switch (status) {
  case CL_QUEUED:
    status_str = "Queued";
    break;
  case CL_SUBMITTED:
    status_str = "Submitted";
    break;
  case CL_RUNNING:
    status_str = "Executing";
    break;
  case CL_COMPLETE:
    status_str = "Completed";
    break;
  }
  printf("[%s]: %s %s\n", reinterpret_cast<char *>(data), status_str,
         command_str);
  fflush(stdout);
}

// Sets the callback for a particular event
void set_callback(cl::Event event, const char *queue_name) {
  cl_int err;
  OCL_CHECK(err,
            err = event.setCallback(CL_COMPLETE, event_cb, (void *)queue_name));
}

//---------------------------------------------------------------------------------------------------------------------

int main(int argc, char **argv) {

  parse_arguments(argc, argv);

  printf("Test CONV: [GIxWxHxCPI] = [%dx%dx%dx%d] -> [GOxWxHxCPO] = [%dx%dx%dx%d] (kernel [%dx%d], stride [1x1], padding [1x1])\n", GI, W, H, CPI, GO, W, H, CPO, KW, KH);

  allocate_buffers();

  cl_int err;
  cl::Kernel kernel_conv2d;

  std::cout << "Creating Context..." << std::endl;
  auto devices = xcl::get_xil_devices();
  auto device = devices[0];
  OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
  OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
  std::string device_name = device.getInfo<CL_DEVICE_NAME>();
  auto fileBuf = xcl::read_binary_file(binaryFile);
  cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
  devices.resize(1);

  OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));
  std::cout << "Device " << device_name.c_str() << ": program successful!" << std::endl;

  OCL_CHECK(err, kernel_conv2d = cl::Kernel(program,"k_conv2D_K3x3_S1x1_P1x1_BS1", &err));
  std::cout << "Kernel sucessfully created" << std::endl ;

  size_t size_data_in_bytes = W * H * I * sizeof(data_type);
  size_t size_output_in_bytes = W * H * O * sizeof(data_type);
  size_t size_kernel_in_bytes = KW * KH * I * O * sizeof(data_type);
  size_t size_bias_in_bytes = O * sizeof(data_type);
  // Allocate memory on the host and fill with random data.

  //-----------------------------
  // fill data vector with random data
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

  std::cout << "Filling buffer with useful data" << std::endl ;
  int addr = 0;
  for (int gi=0; gi<GI; gi++) {
    for (int h=0; h<H; h++) {
      for (int w=0; w<W; w++) {
        for (int c=0; c<CPI; c++) {
          data_type value = (gi * W * H * CPI) + (c * W * H) + (data_type)(h * W) + w; //c+1; // (data_type)((c * 25) + (h * W) + w);
          data_in[addr] = dist(gen); //value;
          addr++;
	}
      }
    }
  }

  std::cout << "Filling kernel buffer with useful data" << std::endl;
  int kernel_id = 1;
  for (int c=0; c<I; c++) {
    for (int cout=0; cout<O; cout++) {
      for (int kh=0; kh<KH; kh++) {
	for (int kw=0; kw<KW; kw++) {
          data_type value = (data_type)kernel_id;
          int addr_k = (cout * I * KW * KH) + (c * KW * KH) + (kh * KW) + kw;
          kernel[addr_k] = dist(gen);
        }
      }
      kernel_id++;
    }
  }

  std::cout << "Filling bias buffer with useful data" << std::endl;
  for (int cout=0; cout<O; cout++) bias[cout] = dist(gen);

  //-----------------------------
  // THIS PAIR OF EVENTS WILL BE USED TO TRACK WHEN A KERNEL IS FINISHED WITH
  // THE INPUT BUFFERS. ONCE THE KERNEL IS FINISHED PROCESSING THE DATA, A NEW
  // SET OF ELEMENTS WILL BE WRITTEN INTO THE BUFFER.
  vector<cl::Event> kernel_events(GO);
  vector<cl::Event> read_events(1);
  vector<cl::Event> write_events(3);
  cl::Buffer buffer_a;
  cl::Buffer buffer_b;
  cl::Buffer buffer_k;
  cl::Buffer buffer_bias;

  //-----------------------------
  // Allocate Buffer in Global Memory
  // Buffers are allocated using CL_MEM_USE_HOST_PTR for efficient memory and
  // Device-to-host communication
  std::cout << "Creating Buffers..." << std::endl;

  OCL_CHECK(err, buffer_a = cl::Buffer(context, CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR , size_data_in_bytes, data_in, &err));
  OCL_CHECK(err, buffer_b = cl::Buffer(context, CL_MEM_WRITE_ONLY  | CL_MEM_USE_HOST_PTR , size_output_in_bytes, out, &err));
  OCL_CHECK(err, buffer_k = cl::Buffer(context, CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR , size_kernel_in_bytes, kernel, &err));
  OCL_CHECK(err, buffer_bias = cl::Buffer(context, CL_MEM_READ_ONLY  | CL_MEM_USE_HOST_PTR , size_bias_in_bytes, bias, &err));

  //Arguments for loop
  int offset_bias = 0;  //offset to pointer bias each loop
  int offset_kernel = 0; //offset to pointer kernel each loop
  int offset_data_out = 0; //offset to poiter output data loop

  //-----------------------------
  // Copy input data to device global memory
  // std::cout << "Copying data (Host to Device)..." << std::endl;
  // Because we are passing the write_events, it returns an event object
  // that identifies this particular command and can be used to query
  // or queue a wait for this particular command to complete.
  OCL_CHECK(err, err = q.enqueueMigrateMemObjects( {buffer_a}, 0 /*0 means from host*/, NULL, &write_events[0]));
  set_callback(write_events[0], "ooo_queue");

  OCL_CHECK(err, err = q.enqueueMigrateMemObjects( {buffer_k}, 0 /*0 means from host*/, NULL, &write_events[1]));
  set_callback(write_events[1], "ooo_queue");

  OCL_CHECK(err, err = q.enqueueMigrateMemObjects( {buffer_bias}, 0 /*0 means from host*/, NULL, &write_events[2]));
  set_callback(write_events[2], "ooo_queue");  
  
  // timint stats
  unsigned long long prof_time;
  struct timeval prof_t1;
  gettimeofday(&prof_t1, NULL);

  for (int o_iter = 0; o_iter < GO; o_iter++){
    // set kernel arguments
    int arg = 0;
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, buffer_a));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, H));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, W));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, I));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, buffer_k));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, buffer_bias));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, buffer_b));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, O));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, offset_bias));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, offset_kernel));
    OCL_CHECK(err, err = kernel_conv2d.setArg(arg++, offset_data_out));

    // Update the offset poiter to bias, kernels and output data
    offset_bias = offset_bias + CPO;
    offset_kernel = offset_kernel + KW * KH * CPO * GI * CPI;
    offset_data_out = offset_data_out +  H * W;

    //-----------------------------
    // printf("Enqueueing NDRange kernel.\n");
    // This event needs to wait for the write buffer operations to complete
    // before executing. We are sending the write_events into its wait list to
    // ensure that the order of operations is correct.
    // Launch the Kernel
    std::vector<cl::Event> waitList;
    waitList.push_back(write_events[0]);
    waitList.push_back(write_events[1]);
    waitList.push_back(write_events[2]);
    OCL_CHECK(err, err = q.enqueueNDRangeKernel(kernel_conv2d, 0, 1, 1, &waitList, &kernel_events[o_iter]));
    set_callback(kernel_events[o_iter], "ooo_queue");
  }

  // we wait all kernels to have completed
  for (int o_iter = 0; o_iter < GO; o_iter++) {
    OCL_CHECK(err, err = kernel_events[o_iter].wait());
  }

  // timing
  struct timeval prof_t2;
  gettimeofday(&prof_t2, NULL);
  prof_time = ((prof_t2.tv_sec - prof_t1.tv_sec) * 1000000) + (prof_t2.tv_usec - prof_t1.tv_usec);
  printf("Timing: %8lld usec\n", prof_time);

  // std::cout << "Getting Results (Device to Host)..." << std::endl;
  std::vector<cl::Event> eventList;
  eventList.push_back(kernel_events[0]);
  // This operation only needs to wait for the kernel call.
  OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_b}, CL_MIGRATE_MEM_OBJECT_HOST, &eventList, &read_events[0]));
  set_callback(read_events[0], "ooo_queue");
  OCL_CHECK(err, err = read_events[0].wait());

  // Wait for all of the OpenCL operations to complete
  std::cout << "Waiting..." << std::endl;
  OCL_CHECK(err, err = q.flush());
  OCL_CHECK(err, err = q.finish());

  std::cout << "computing conv in CPU..." << std::endl;

  // cpu_print_data_in();
  // cpu_print_kernels();
  // cpu_print_bias();
  cpu_conv2d();
  // cpu_print_out();

  check_result();

  //-----------------------------
  std::cout << "" << std::endl;
  std::cout << "All done" << std::endl;
  std::cout << "quit now" << std::endl;

  deallocate_buffers();

  // exit
  return 0;
}
