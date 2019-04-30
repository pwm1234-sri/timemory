// MIT License
//
// Copyright (c) 2019, The Regents of the University of California,
// through Lawrence Berkeley National Laboratory (subject to receipt of any
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <future>
#include <iterator>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

#include <timemory/auto_macros.hpp>
#include <timemory/auto_tuple.hpp>
#include <timemory/component_tuple.hpp>
#include <timemory/environment.hpp>
#include <timemory/manager.hpp>
#include <timemory/mpi.hpp>
#include <timemory/papi.hpp>
#include <timemory/rusage.hpp>
#include <timemory/signal_detection.hpp>
#include <timemory/testing.hpp>

using namespace tim::component;

using auto_tuple_t = tim::auto_tuple<real_clock, system_clock, cpu_clock, cpu_util>;

//--------------------------------------------------------------------------------------//
// saxpy calculation
__global__ void
saxpy(int64_t n, float a, float* x, float* y)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if(i < n)
        y[i] = a * x[i] + y[i];
}
//--------------------------------------------------------------------------------------//

void
print_info(const std::string&);
void
print_string(const std::string& str);
void
test_1_saxpy();

//======================================================================================//

int
main(int argc, char** argv)
{
    tim::env::parse();
    auto* timing =
        new tim::component_tuple<real_clock, system_clock, cpu_clock, cpu_util>(
            "Tests runtime", true);

    timing->start();

    CONFIGURE_TEST_SELECTOR(1);

    int num_fail = 0;
    int num_test = 0;

    std::cout << "# tests: " << tests.size() << std::endl;
    try
    {
        RUN_TEST(1, test_1_saxpy, num_test, num_fail);
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    timing->stop();
    std::cout << "\n" << *timing << std::endl;

    TEST_SUMMARY(argv[0], num_test, num_fail);
    delete timing;

    exit(num_fail);
}

//======================================================================================//

void
print_info(const std::string& func)
{
    if(tim::mpi_rank() == 0)
    {
        std::cout << "\n[" << tim::mpi_rank() << "]\e[1;33m TESTING \e[0m["
                  << "\e[1;36m" << func << "\e[0m"
                  << "]...\n"
                  << std::endl;
    }
}

//======================================================================================//

void
print_string(const std::string& str)
{
    std::stringstream _ss;
    _ss << "[" << tim::mpi_rank() << "] " << str << std::endl;
    std::cout << _ss.str();
}

//======================================================================================//

void
test_1_saxpy()
{
    print_info(__FUNCTION__);
    TIMEMORY_AUTO_TUPLE(auto_tuple_t, "");

    int64_t     N = 20 * (1 << 23);
    float *     x, *y, *d_x, *d_y;
    int         block        = 512;
    int         ngrid        = (N + block - 1) / block;
    float       milliseconds = 0.0f;
    float       maxError     = 0.0f;
    cudaEvent_t start, stop;
    cuda_event* _evt = nullptr;

    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[malloc]");
        x = (float*) malloc(N * sizeof(float));
        y = (float*) malloc(N * sizeof(float));
    }

    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[cudaMalloc]");
        cudaMalloc(&d_x, N * sizeof(float));
        cudaMalloc(&d_y, N * sizeof(float));
    }

    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[assign]");
        for(int i = 0; i < N; i++)
        {
            x[i] = 1.0f;
            y[i] = 2.0f;
        }
    }

    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[create_event]");
        cudaEventCreate(&start);
        cudaEventCreate(&stop);
        _evt                     = new cuda_event();
        _evt->get_precision()    = 12;
        _evt->get_format_flags() = std::ios_base::scientific;
    }

    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[H2D]");
        cudaMemcpy(d_x, x, N * sizeof(float), cudaMemcpyHostToDevice);
        cudaMemcpy(d_y, y, N * sizeof(float), cudaMemcpyHostToDevice);
    }

    for(int i = 0; i < 5; ++i)
    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[", i, "]");
        _evt->start();
        cudaEventRecord(start);

        // Perform SAXPY on 1M elements
        saxpy<<<ngrid, block>>>(N, 2.0f, d_x, d_y);

        cudaEventRecord(stop);
        _evt->stop();

        cudaEventSynchronize(stop);
        float tmp = 0.0f;
        cudaEventElapsedTime(&tmp, start, stop);
        milliseconds += tmp;
    }

    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[D2H]");
        cudaMemcpy(y, d_y, N * sizeof(float), cudaMemcpyDeviceToHost);
    }

    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[check]");
        for(int64_t i = 0; i < N; i++)
        {
            maxError = max(maxError, abs(y[i] - 4.0f));
        }
    }

    {
        TIMEMORY_AUTO_TUPLE(auto_tuple_t, "[output]");
        std::cout << "Event: " << _evt << std::endl;
        printf("Max error: %f\n", maxError);
        printf("Effective Bandwidth (GB/s): %f\n", N * 4 * 3 / milliseconds / 1e6);
        printf("Runtime (s): %16.12e\n", milliseconds / 1e6);
    }

    delete _evt;
}

//======================================================================================//
