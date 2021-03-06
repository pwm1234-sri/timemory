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
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

/** \file timemory/variadic/lightweight_tuple.hpp
 * This is the C++ class that bundles together components and enables
 * operation on the components as a single entity
 *
 */

#pragma once

#include "timemory/backends/dmp.hpp"
#include "timemory/general/source_location.hpp"
#include "timemory/mpl/apply.hpp"
#include "timemory/mpl/filters.hpp"
#include "timemory/operations/types.hpp"
#include "timemory/settings/declaration.hpp"
#include "timemory/storage/types.hpp"
#include "timemory/tpls/cereal/cereal.hpp"
#include "timemory/utility/macros.hpp"
#include "timemory/variadic/base_bundle.hpp"
#include "timemory/variadic/functional.hpp"
#include "timemory/variadic/types.hpp"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <iostream>
#include <string>

//======================================================================================//

namespace tim
{
//======================================================================================//
// variadic list of components
//
template <typename... Types>
class lightweight_tuple
: public stack_bundle<available_t<type_list<Types...>>>
, public concepts::comp_wrapper
{
    // manager is friend so can use above
    friend class manager;

public:
    using bundle_type         = stack_bundle<available_t<type_list<Types...>>>;
    using this_type           = lightweight_tuple<Types...>;
    using captured_location_t = source_location::captured;

    using data_type         = typename bundle_type::data_type;
    using impl_type         = typename bundle_type::impl_type;
    using tuple_type        = typename bundle_type::tuple_type;
    using sample_type       = typename bundle_type::sample_type;
    using reference_type    = typename bundle_type::reference_type;
    using user_bundle_types = typename bundle_type::user_bundle_types;

    using apply_v     = apply<void>;
    using size_type   = typename bundle_type::size_type;
    using string_t    = typename bundle_type::string_t;
    using string_hash = typename bundle_type::string_hash;

    template <template <typename> class Op, typename Tuple = impl_type>
    using operation_t = typename bundle_type::template generic_operation<Op, Tuple>::type;

    template <template <typename> class Op, typename Tuple = impl_type>
    using custom_operation_t =
        typename bundle_type::template custom_operation<Op, Tuple>::type;

    // used by gotcha
    using component_type   = lightweight_tuple<Types...>;
    using type             = convert_t<tuple_type, lightweight_tuple<>>;
    using initializer_type = std::function<void(this_type&)>;

    static constexpr bool is_component      = false;
    static constexpr bool has_gotcha_v      = bundle_type::has_gotcha_v;
    static constexpr bool has_user_bundle_v = bundle_type::has_user_bundle_v;

public:
    static initializer_type& get_initializer()
    {
        static initializer_type _instance = [](this_type&) {};
        return _instance;
    }

public:
    template <typename T, typename... U>
    struct quirk_config
    {
        static constexpr bool value =
            is_one_of<T,
                      contains_one_of_t<quirk::is_config, concat<Types..., U...>>>::value;
    };

public:
    lightweight_tuple() = default;

    template <typename... T, typename Func = initializer_type>
    explicit lightweight_tuple(const string_t& key, quirk::config<T...> = {},
                               const Func& = get_initializer());

    template <typename... T, typename Func = initializer_type>
    explicit lightweight_tuple(const captured_location_t& loc, quirk::config<T...> = {},
                               const Func& = get_initializer());

    template <typename... T, typename Func = initializer_type>
    explicit lightweight_tuple(size_t _hash, quirk::config<T...> = {},
                               const Func& = get_initializer());

    ~lightweight_tuple();

    //------------------------------------------------------------------------//
    //      Copy construct and assignment
    //------------------------------------------------------------------------//
    lightweight_tuple(const lightweight_tuple&)     = default;
    lightweight_tuple(lightweight_tuple&&) noexcept = default;

    lightweight_tuple& operator=(const lightweight_tuple& rhs) = default;
    lightweight_tuple& operator=(lightweight_tuple&&) noexcept = default;

    lightweight_tuple clone(bool store, scope::config _scope = scope::get_default());

public:
    //----------------------------------------------------------------------------------//
    // public static functions
    //
    static constexpr std::size_t size() { return std::tuple_size<tuple_type>::value; }
    /// requests the component initialize their storage
    static void init_storage();

    //----------------------------------------------------------------------------------//
    // public member functions
    //
    void push();
    void pop();
    template <typename... Args>
    void measure(Args&&...);
    template <typename... Args>
    void sample(Args&&...);
    template <typename... Args>
    void start(Args&&...);
    template <typename... Args>
    void stop(Args&&...);
    template <typename... Args>
    this_type& record(Args&&...);
    template <typename... Args>
    void reset(Args&&...);
    template <typename... Args>
    auto get(Args&&...) const;
    template <typename... Args>
    auto             get_labeled(Args&&...) const;
    data_type&       data();
    const data_type& data() const;

    using bundle_type::hash;
    using bundle_type::key;
    using bundle_type::laps;
    using bundle_type::rekey;
    using bundle_type::store;

    //----------------------------------------------------------------------------------//
    // construct the objects that have constructors with matching arguments
    //
    template <typename... Args>
    void construct(Args&&... _args)
    {
        using construct_t = operation_t<operation::construct>;
        apply_v::access<construct_t>(m_data, std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//
    /// provide preliminary info to the objects with matching arguments
    //
    void assemble() { invoke::assemble(m_data, *this); }

    template <typename... Args, size_t N = sizeof...(Args), enable_if_t<N != 0, int> = 0>
    void assemble(Args&&... _args)
    {
        invoke::assemble(m_data, std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//
    /// provide conclusive info to the objects with matching arguments
    //
    void derive() { invoke::derive(m_data, *this); }

    template <typename... Args, size_t N = sizeof...(Args), enable_if_t<N != 0, int> = 0>
    void derive(Args&&... _args)
    {
        invoke::derive(m_data, std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//
    // mark a beginning position in the execution (typically used by asynchronous
    // structures)
    //
    template <typename... Args>
    void mark_begin(Args&&... _args)
    {
        invoke::mark_begin(m_data, std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//
    // mark a beginning position in the execution (typically used by asynchronous
    // structures)
    //
    template <typename... Args>
    void mark_end(Args&&... _args)
    {
        invoke::mark_end(m_data, std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//
    // store a value
    //
    template <typename... Args>
    void store(Args&&... _args)
    {
        invoke::store(m_data, std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//
    // perform a auditd operation (typically for GOTCHA)
    //
    template <typename... Args>
    void audit(Args&&... _args)
    {
        invoke::audit(m_data, std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//

    template <template <typename> class OpT, typename... Args>
    void invoke(Args&&... _args)
    {
        invoke::invoke<OpT>(m_data, std::forward<Args>(_args)...);
    }

    //----------------------------------------------------------------------------------//
    // get member functions taking either a type
    //
    template <typename T, enable_if_t<is_one_of<T, data_type>::value, int> = 0>
    T* get()
    {
        return &(std::get<index_of<T, data_type>::value>(m_data));
    }

    template <typename T, enable_if_t<is_one_of<T, data_type>::value, int> = 0>
    const T* get() const
    {
        return &(std::get<index_of<T, data_type>::value>(m_data));
    }

    template <typename T, enable_if_t<!is_one_of<T, data_type>::value, int> = 0>
    T* get() const
    {
        void*       ptr   = nullptr;
        static auto _hash = std::hash<std::string>()(demangle<T>());
        get(ptr, _hash);
        return static_cast<T*>(ptr);
    }

    void get(void*& ptr, size_t _hash) const
    {
        using get_t = operation_t<operation::get>;
        apply_v::access<get_t>(m_data, ptr, _hash);
    }

    //----------------------------------------------------------------------------------//
    /// this is a simple alternative to get<T>() when used from SFINAE in operation
    /// namespace which has a struct get also templated. Usage there can cause error
    /// with older compilers
    template <typename U, typename T = std::remove_pointer_t<decay_t<U>>,
              enable_if_t<trait::is_available<T>::value && is_one_of<T, data_type>::value,
                          int> = 0>
    auto get_component()
    {
        return get<T>();
    }

    //----------------------------------------------------------------------------------//

    template <
        typename T, typename... Args,
        enable_if_t<!is_one_of<T, reference_type>::value && has_user_bundle_v, int> = 0>
    void init(Args&&...)
    {
        using bundle_t = decltype(std::get<0>(std::declval<user_bundle_types>()));
        this->init<bundle_t>();
        this->get<bundle_t>()->insert(component::factory::get_opaque<T>(m_scope),
                                      component::factory::get_typeids<T>());
    }

    //----------------------------------------------------------------------------------//

    template <
        typename T, typename... Args,
        enable_if_t<!is_one_of<T, reference_type>::value && !has_user_bundle_v, int> = 0>
    void init(Args&&...)
    {}

    //----------------------------------------------------------------------------------//
    //  variadic initialization
    //
    template <typename T, typename... Tail, enable_if_t<sizeof...(Tail) == 0, int> = 0>
    void initialize()
    {
        this->init<T>();
    }

    template <typename T, typename... Tail, enable_if_t<(sizeof...(Tail) > 0), int> = 0>
    void initialize()
    {
        this->init<T>();
        this->initialize<Tail...>();
    }

    //----------------------------------------------------------------------------------//
    /// apply a member function to a type that is in variadic list AND is available
    ///
    template <typename T, typename Func, typename... Args,
              enable_if_t<is_one_of<T, data_type>::value, int> = 0>
    void type_apply(Func&& _func, Args&&... _args)
    {
        auto&& _obj = get<T>();
        ((_obj).*(_func))(std::forward<Args>(_args)...);
    }

    template <typename T, typename Func, typename... Args,
              enable_if_t<!is_one_of<T, data_type>::value, int> = 0>
    void type_apply(Func&&, Args&&...)
    {}

    //----------------------------------------------------------------------------------//
    // this_type operators
    //
    this_type& operator-=(const this_type& rhs);
    this_type& operator-=(this_type& rhs);
    this_type& operator+=(const this_type& rhs);
    this_type& operator+=(this_type& rhs);

    //----------------------------------------------------------------------------------//
    // generic operators
    //
    template <typename Op>
    this_type& operator-=(Op&& rhs)
    {
        using minus_t = operation_t<operation::minus>;
        apply_v::access<minus_t>(m_data, std::forward<Op>(rhs));
        return *this;
    }

    template <typename Op>
    this_type& operator+=(Op&& rhs)
    {
        using plus_t = operation_t<operation::plus>;
        apply_v::access<plus_t>(m_data, std::forward<Op>(rhs));
        return *this;
    }

    template <typename Op>
    this_type& operator*=(Op&& rhs)
    {
        using multiply_t = operation_t<operation::multiply>;
        apply_v::access<multiply_t>(m_data, std::forward<Op>(rhs));
        return *this;
    }

    template <typename Op>
    this_type& operator/=(Op&& rhs)
    {
        using divide_t = operation_t<operation::divide>;
        apply_v::access<divide_t>(m_data, std::forward<Op>(rhs));
        return *this;
    }

    //----------------------------------------------------------------------------------//
    // friend operators
    //
    friend this_type operator+(const this_type& lhs, const this_type& rhs)
    {
        this_type tmp(lhs);
        return tmp += rhs;
    }

    friend this_type operator-(const this_type& lhs, const this_type& rhs)
    {
        this_type tmp(lhs);
        return tmp -= rhs;
    }

    template <typename Op>
    friend this_type operator*(const this_type& lhs, Op&& rhs)
    {
        this_type tmp(lhs);
        return tmp *= std::forward<Op>(rhs);
    }

    template <typename Op>
    friend this_type operator/(const this_type& lhs, Op&& rhs)
    {
        this_type tmp(lhs);
        return tmp /= std::forward<Op>(rhs);
    }

    //----------------------------------------------------------------------------------//
    //
    template <bool PrintPrefix = true, bool PrintLaps = true>
    void print(std::ostream& os) const
    {
        using print_t = typename bundle_type::print_t;
        if(size() == 0 || m_hash == 0)
            return;
        std::stringstream ss_data;
        apply_v::access_with_indices<print_t>(m_data, std::ref(ss_data), false);
        if(PrintPrefix)
        {
            update_width();
            std::stringstream ss_prefix;
            std::stringstream ss_id;
            ss_id << get_prefix() << " " << std::left << key();
            ss_prefix << std::setw(output_width()) << std::left << ss_id.str() << " : ";
            os << ss_prefix.str();
        }
        os << ss_data.str();
        if(m_laps > 0 && PrintLaps)
            os << " [laps: " << m_laps << "]";
    }

    //----------------------------------------------------------------------------------//
    //
    friend std::ostream& operator<<(std::ostream& os, const this_type& obj)
    {
        obj.print<true, true>(os);
        return os;
    }

    //----------------------------------------------------------------------------------//
    //
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int)
    {
        std::string _key   = "";
        auto        keyitr = get_hash_ids()->find(m_hash);
        if(keyitr != get_hash_ids()->end())
            _key = keyitr->second;

        ar(cereal::make_nvp("hash", m_hash), cereal::make_nvp("key", _key),
           cereal::make_nvp("laps", m_laps));

        if(keyitr == get_hash_ids()->end())
        {
            auto _hash = add_hash_id(_key);
            if(_hash != m_hash)
                PRINT_HERE("Warning! Hash for '%s' (%llu) != %llu", _key.c_str(),
                           (unsigned long long) _hash, (unsigned long long) m_hash);
        }

        ar(cereal::make_nvp("data", m_data));
    }

public:
    int64_t         laps() const { return bundle_type::laps(); }
    std::string     key() const { return bundle_type::key(); }
    uint64_t        hash() const { return bundle_type::hash(); }
    void            rekey(const string_t& _key) { bundle_type::rekey(_key); }
    bool&           store() { return bundle_type::store(); }
    const bool&     store() const { return bundle_type::store(); }
    const string_t& prefix() const { return bundle_type::prefix(); }
    const string_t& get_prefix() const { return bundle_type::get_prefix(); }

protected:
    static int64_t output_width(int64_t w = 0) { return bundle_type::output_width(w); }
    void           update_width() const { bundle_type::update_width(); }
    void compute_width(const string_t& _key) const { bundle_type::compute_width(_key); }

protected:
    // protected member functions
    data_type&       get_data();
    const data_type& get_data() const;
    void             set_prefix(const string_t&) const;
    void             set_prefix(size_t) const;
    void             set_scope(scope::config);

protected:
    // objects
    using bundle_type::m_hash;
    using bundle_type::m_is_pushed;
    using bundle_type::m_laps;
    using bundle_type::m_scope;
    using bundle_type::m_store;
    mutable data_type m_data = data_type{};
};

//======================================================================================//

template <typename... Types>
auto
get(const lightweight_tuple<Types...>& _obj)
    -> decltype(std::declval<lightweight_tuple<Types...>>().get())
{
    return _obj.get();
}

//--------------------------------------------------------------------------------------//

template <typename... Types>
auto
get_labeled(const lightweight_tuple<Types...>& _obj)
    -> decltype(std::declval<lightweight_tuple<Types...>>().get_labeled())
{
    return _obj.get_labeled();
}

//--------------------------------------------------------------------------------------//

}  // namespace tim

//======================================================================================//
//
//      std::get operator
//
namespace std
{
//--------------------------------------------------------------------------------------//

template <std::size_t N, typename... Types>
typename std::tuple_element<N, std::tuple<Types...>>::type&
get(::tim::lightweight_tuple<Types...>& obj)
{
    return get<N>(obj.data());
}

//--------------------------------------------------------------------------------------//

template <std::size_t N, typename... Types>
const typename std::tuple_element<N, std::tuple<Types...>>::type&
get(const ::tim::lightweight_tuple<Types...>& obj)
{
    return get<N>(obj.data());
}

//--------------------------------------------------------------------------------------//

template <std::size_t N, typename... Types>
auto
get(::tim::lightweight_tuple<Types...>&& obj)
    -> decltype(get<N>(std::forward<::tim::lightweight_tuple<Types...>>(obj).data()))
{
    using obj_type = ::tim::lightweight_tuple<Types...>;
    return get<N>(std::forward<obj_type>(obj).data());
}

//======================================================================================//
}  // namespace std

//--------------------------------------------------------------------------------------//
