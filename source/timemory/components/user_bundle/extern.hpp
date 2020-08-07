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

/**
 * \file timemory/components/user_bundle/extern.hpp
 * \brief Include the extern declarations for user_bundle components
 */

#pragma once

// #include "timemory/components/extern.hpp"
#include "timemory/components/base.hpp"
#include "timemory/components/extern/common.hpp"
#include "timemory/components/macros.hpp"
#include "timemory/components/types.hpp"
#include "timemory/components/user_bundle/components.hpp"
#include "timemory/components/user_bundle/types.hpp"
#include "timemory/runtime/configure.hpp"
#include "timemory/runtime/enumerate.hpp"

#if !defined(TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT)
#    if defined(TIMEMORY_SOURCE) && defined(TIMEMORY_USER_BUNDLE_SOURCE)
#        define TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT(NAME, HAS_DATA, VAR, ...)          \
            TIMEMORY_INSTANTIATE_EXTERN_TEMPLATE(                                        \
                struct tim::component::base<TIMEMORY_ESC(tim::component::NAME),          \
                                            __VA_ARGS__>)                                \
            TIMEMORY_INSTANTIATE_EXTERN_OPERATIONS(TIMEMORY_ESC(component::NAME),        \
                                                   HAS_DATA)                             \
            TIMEMORY_INSTANTIATE_EXTERN_STORAGE(TIMEMORY_ESC(component::NAME), VAR)
#    elif defined(TIMEMORY_USE_EXTERN) || defined(TIMEMORY_USE_USER_BUNDLE_EXTERN)
#        define TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT(NAME, HAS_DATA, VAR, ...)          \
            TIMEMORY_DECLARE_EXTERN_TEMPLATE(                                            \
                struct tim::component::base<TIMEMORY_ESC(tim::component::NAME),          \
                                            __VA_ARGS__>)                                \
            TIMEMORY_DECLARE_EXTERN_OPERATIONS(TIMEMORY_ESC(component::NAME), HAS_DATA)  \
            TIMEMORY_DECLARE_EXTERN_STORAGE(TIMEMORY_ESC(component::NAME), VAR)
#    else
#        define TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT(NAME, HAS_DATA, VAR, ...)
#    endif
#endif

TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT(
    TIMEMORY_ESC(user_bundle<tim::component::global_bundle_idx, tim::api::native_tag>),
    false, user_global_bundle, void)

TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT(
    TIMEMORY_ESC(user_bundle<tim::component::tuple_bundle_idx, tim::api::native_tag>),
    false, user_tuple_bundle, void)

TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT(
    TIMEMORY_ESC(user_bundle<tim::component::list_bundle_idx, tim::api::native_tag>),
    false, user_list_bundle, void)

TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT(
    TIMEMORY_ESC(user_bundle<tim::component::ompt_bundle_idx, tim::api::native_tag>),
    false, user_ompt_bundle, void)

TIMEMORY_EXTERN_USER_BUNDLE_COMPONENT(
    TIMEMORY_ESC(user_bundle<tim::component::mpip_bundle_idx, tim::api::native_tag>),
    false, user_mpip_bundle, void)
