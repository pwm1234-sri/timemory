// MIT License
//
// Copyright (c) 2020, The Regents of the University of California,
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

#include "gtest/gtest.h"

#include "timemory/timemory.hpp"

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

using namespace tim::component;

static int    _argc       = 0;
static char** _argv       = nullptr;

using mutex_t = std::mutex;
using lock_t  = std::unique_lock<mutex_t>;

//--------------------------------------------------------------------------------------//

namespace details
{
//--------------------------------------------------------------------------------------//
//  Get the current tests name
//
inline std::string
get_test_name()
{
    return ::testing::UnitTest::GetInstance()->current_test_info()->name();
}

// this function consumes approximately "n" milliseconds of real time
inline void
do_sleep(long n)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(n));
}

// this function consumes an unknown number of cpu resources
inline long
fibonacci(long n)
{
    return (n < 2) ? n : (fibonacci(n - 1) + fibonacci(n - 2));
}

// this function consumes approximately "t" milliseconds of cpu time
void
consume(long n)
{
    // a mutex held by one lock
    mutex_t mutex;
    // acquire lock
    lock_t hold_lk(mutex);
    // associate but defer
    lock_t try_lk(mutex, std::defer_lock);
    // get current time
    auto now = std::chrono::steady_clock::now();
    // try until time point
    while(std::chrono::steady_clock::now() < (now + std::chrono::milliseconds(n)))
        try_lk.try_lock();
}

// get a random entry from vector
template <typename Tp>
size_t
random_entry(const std::vector<Tp>& v)
{
    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, v.size() - 1);
    return v.at(dist(rng));
}

}  // namespace details

//--------------------------------------------------------------------------------------//

class derived_tests : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tim::settings::verbose()     = 0;
        tim::settings::debug()       = false;
        tim::settings::json_output() = true;
        tim::settings::mpi_thread()  = false;
        tim::dmp::initialize(_argc, _argv);
        tim::timemory_init(_argc, _argv);
        tim::settings::dart_output() = true;
        tim::settings::dart_count()  = 1;
        tim::settings::banner()      = false;
        tim::enable_signal_detection();
    }

    static void TearDownTestSuite()
    {
        tim::timemory_finalize();
        tim::dmp::finalize();
    }
};

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_tuple_wc_cc)
{
    std::cout << '\n';
    using toolset_t = tim::component_tuple<wall_clock, cpu_clock, cpu_util>;

    toolset_t obj(details::get_test_name());
    obj.start();
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_TRUE(obj.get<cpu_util>()->is_derived());
    auto manual_calc = 100. * obj.get<cpu_clock>()->get() / obj.get<wall_clock>()->get();
    ASSERT_NEAR(obj.get<cpu_util>()->get(), manual_calc, 1.0e-6);
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_tuple_wc_uc_sc)
{
    std::cout << '\n';
    using toolset_t = tim::auto_tuple<wall_clock, user_clock, system_clock, cpu_util>;

    toolset_t obj(details::get_test_name());
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_TRUE(obj.get<cpu_util>()->is_derived());
    auto manual_calc = 100. *
                       (obj.get<user_clock>()->get() + obj.get<system_clock>()->get()) /
                       obj.get<wall_clock>()->get();
    ASSERT_NEAR(obj.get<cpu_util>()->get(), manual_calc, 1.0e-6);
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_tuple_wc)
{
    std::cout << '\n';
    using toolset_t = tim::auto_tuple<wall_clock, cpu_util>;

    toolset_t obj(details::get_test_name());
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_FALSE(obj.get<cpu_util>()->is_derived());
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_list_wc_cc)
{
    std::cout << '\n';
    using toolset_t = tim::component_list<wall_clock, cpu_clock, cpu_util>;

    toolset_t::get_initializer() = [](toolset_t& cl) {
        cl.initialize<wall_clock, cpu_clock, cpu_util>();
    };

    toolset_t obj(details::get_test_name());
    obj.start();
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_TRUE(obj.get<cpu_util>()->is_derived());
    auto manual_calc = 100. * obj.get<cpu_clock>()->get() / obj.get<wall_clock>()->get();
    ASSERT_NEAR(obj.get<cpu_util>()->get(), manual_calc, 1.0e-6);
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_list_wc_uc_sc)
{
    std::cout << '\n';
    using toolset_t = tim::auto_list<wall_clock, user_clock, system_clock, cpu_util>;

    toolset_t::get_initializer() = [](toolset_t& cl) {
        cl.initialize<wall_clock, user_clock, system_clock, cpu_util>();
    };

    toolset_t obj(details::get_test_name());
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_TRUE(obj.get<cpu_util>()->is_derived());
    auto manual_calc = 100. *
                       (obj.get<user_clock>()->get() + obj.get<system_clock>()->get()) /
                       obj.get<wall_clock>()->get();
    ASSERT_NEAR(obj.get<cpu_util>()->get(), manual_calc, 1.0e-6);
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_list_wc)
{
    std::cout << '\n';
    using toolset_t = tim::auto_list<wall_clock, cpu_util>;

    toolset_t::get_initializer() = [](toolset_t& cl) {
        cl.initialize<wall_clock, cpu_util>();
    };

    toolset_t obj(details::get_test_name());
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_FALSE(obj.get<cpu_util>()->is_derived());
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_hybrid_wc_cc)
{
    std::cout << '\n';
    using list_type = tim::component_list<cpu_clock, cpu_util>;
    using toolset_t = tim::component_hybrid<tim::component_tuple<wall_clock>, list_type>;

    list_type::get_initializer() = [](list_type& cl) {
        cl.initialize<cpu_clock, cpu_util>();
    };

    toolset_t obj(details::get_test_name());
    obj.start();
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_TRUE(obj.get_component<cpu_util>()->is_derived());
    auto manual_calc = 100. * obj.get_component<cpu_clock>()->get() /
                       obj.get_component<wall_clock>()->get();
    ASSERT_NEAR(obj.get_component<cpu_util>()->get(), manual_calc, 1.0e-6);
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_hybrid_wc_uc_sc)
{
    std::cout << '\n';
    using toolset_t = tim::auto_hybrid<tim::component_tuple<wall_clock, user_clock>,
                                       tim::component_list<system_clock, cpu_util>>;
    using list_type = typename toolset_t::list_t;

    list_type::get_initializer() = [](list_type& cl) {
        cl.initialize<system_clock, cpu_util>();
    };

    toolset_t obj(details::get_test_name());
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_TRUE(obj.get_component<cpu_util>()->is_derived());
    auto manual_calc = 100. *
                       (obj.get_component<user_clock>()->get() +
                        obj.get_component<system_clock>()->get()) /
                       obj.get_component<wall_clock>()->get();
    ASSERT_NEAR(obj.get_component<cpu_util>()->get(), manual_calc, 1.0e-6);
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

TEST_F(derived_tests, cpu_util_hybrid_wc)
{
    std::cout << '\n';
    using toolset_t =
        tim::auto_hybrid<tim::component_tuple<wall_clock>, tim::component_list<cpu_util>>;
    using list_type = typename toolset_t::list_t;

    list_type::get_initializer() = [](list_type& cl) { cl.initialize<cpu_util>(); };

    toolset_t obj(details::get_test_name());
    details::consume(1000);
    obj.stop();

    std::cout << obj << "\n";

    ASSERT_FALSE(obj.get_component<cpu_util>()->is_derived());
    std::cout << '\n';
}

//--------------------------------------------------------------------------------------//

int
main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    _argc = argc;
    _argv = argv;
    return RUN_ALL_TESTS();
}

//--------------------------------------------------------------------------------------//
