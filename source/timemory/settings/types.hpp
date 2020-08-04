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
 * \file timemory/settings/types.hpp
 * \brief Declare the settings types
 */

#pragma once

#include "timemory/compat/macros.h"
#include "timemory/settings/macros.hpp"
#include "timemory/utility/argparse.hpp"
#include "timemory/utility/serializer.hpp"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace tim
{
//
//--------------------------------------------------------------------------------------//
//
//                              settings
//
//--------------------------------------------------------------------------------------//
//
std::string
get_local_datetime(const char* dt_format);
//
//--------------------------------------------------------------------------------------//
//
struct settings;
//
/// \struct tim::base::vsettings
/// \brief Base class for storing settings
struct vsettings
{
    using parser_t = argparse::argument_parser;

    vsettings(const std::string& _name = "", const std::string& _environ = "",
              const std::string& _descript = "", std::vector<std::string> _cmdline = {},
              int32_t _count = -1, int32_t _max_count = -1)
    : m_count(_count)
    , m_max_count(_max_count)
    , m_name(_name)
    , m_environ(_environ)
    , m_description(_descript)
    , m_cmdline(_cmdline)
    {}

    virtual ~vsettings() = default;

    virtual void                       parse()                                  = 0;
    virtual void                       add_argument(argparse::argument_parser&) = 0;
    virtual std::shared_ptr<vsettings> clone()                                  = 0;

    const auto& get_name() const { return m_name; }
    const auto& get_env_name() const { return m_environ; }
    const auto& get_description() const { return m_description; }
    const auto& get_command_line() const { return m_cmdline; }
    const auto& get_count() const { return m_count; }
    const auto& get_max_count() const { return m_max_count; }

    void set_count(int32_t v) { m_count = v; }
    void set_max_count(int32_t v) { m_max_count = v; }

    auto get_type_index() const { return m_type_index; }
    auto get_value_index() const { return m_value_index; }

protected:
    std::type_index          m_type_index  = std::type_index(typeid(void));
    std::type_index          m_value_index = std::type_index(typeid(void));
    int32_t                  m_count       = -1;
    int32_t                  m_max_count   = -1;
    std::string              m_name        = "";
    std::string              m_environ     = "";
    std::string              m_description = "";
    std::vector<std::string> m_cmdline     = {};
};
//
/// \struct tim::tsettings
/// \brief Implements a specific setting
template <typename Tp, typename Vp = Tp>
struct tsettings : public vsettings
{
    using type       = Tp;
    using value_type = Vp;
    using base_type  = vsettings;

    template <typename Up = Vp, enable_if_t<!std::is_reference<Up>::value> = 0>
    tsettings()
    : base_type()
    , m_value(Tp{})
    {}

    template <typename... Args>
    tsettings(Vp _value, Args&&... _args)
    : base_type(std::forward<Args>(_args)...)
    , m_value(_value)
    {
        m_type_index  = std::type_index(typeid(type));
        m_value_index = std::type_index(typeid(value_type));
    }

    Tp&       get() { return m_value; }
    const Tp& get() const { return m_value; }
    void      set(Tp&& _value) { m_value = std::forward<Tp>(_value); }

    virtual void parse() final { set(get_env<Tp>(m_environ, get())); }

    virtual void add_argument(argparse::argument_parser& p) final
    {
        if(!m_cmdline.empty())
        {
            if(std::is_same<Tp, bool>::value)
                m_max_count = 1;
            p.add_argument(m_cmdline, m_description)
                .action(get_action())
                .count(m_count)
                .max_count(m_max_count);
        }
    }

    virtual std::shared_ptr<vsettings> clone() final
    {
        return std::make_shared<tsettings<Tp>>(m_value, m_name, m_environ, m_description,
                                               m_cmdline, m_count, m_max_count);
    }

    template <typename Archive, typename Up = Vp,
              enable_if_t<!std::is_reference<Up>::value> = 0>
    void serialize(Archive& ar, const unsigned int)
    {
        ar(cereal::make_nvp("name", m_name));
        ar(cereal::make_nvp("environ", m_environ));
        ar(cereal::make_nvp("description", m_description));
        ar(cereal::make_nvp("count", m_count));
        ar(cereal::make_nvp("max_count", m_max_count));
        ar(cereal::make_nvp("cmdline", m_cmdline));
        ar(cereal::make_nvp("value", m_value));
    }

private:
    template <typename Up = Tp, enable_if_t<std::is_same<Up, bool>::value> = 0>
    auto get_action()
    {
        return [&](parser_t& p) {
            auto id = argparse::helpers::ltrim(
                m_cmdline.back(), [](int c) { return static_cast<char>(c) == '-'; });
            auto val = p.get<std::string>(id);
            if(val.empty())
                m_value = true;
            else
            {
                namespace regex_const      = std::regex_constants;
                const auto regex_constants = regex_const::ECMAScript | regex_const::icase;
                const std::string pattern  = "^(off|false|no|n|f|0)$";
                if(std::regex_match(val, std::regex(pattern, regex_constants)))
                    m_value = false;
                else
                    m_value = true;
            }
        };
    }

    template <typename Up = Tp, enable_if_t<!std::is_same<Up, bool>::value &&
                                            !std::is_same<Up, std::string>::value> = 0>
    auto get_action()
    {
        return [&](parser_t& p) {
            auto id = argparse::helpers::ltrim(
                m_cmdline.back(), [](int c) { return static_cast<char>(c) == '-'; });
            m_value = p.get<Up>(id);
        };
    }

    template <typename Up = Tp, enable_if_t<std::is_same<Up, std::string>::value> = 0>
    auto get_action()
    {
        return [&](parser_t& p) {
            auto id = argparse::helpers::ltrim(
                m_cmdline.back(), [](int c) { return static_cast<char>(c) == '-'; });
            auto _vec = p.get<std::vector<std::string>>(id);
            if(_vec.empty())
                m_value = "";
            else
            {
                std::stringstream ss;
                for(auto& itr : _vec)
                    ss << ", " << itr;
                m_value = ss.str().substr(2);
            }
        };
    }

private:
    using base_type::m_count;
    using base_type::m_description;
    using base_type::m_environ;
    using base_type::m_max_count;
    using base_type::m_name;
    using base_type::m_type_index;
    value_type m_value;
};
//
//--------------------------------------------------------------------------------------//
//
}  // namespace tim

TIMEMORY_SET_CLASS_VERSION(2, ::tim::settings)
TIMEMORY_SET_CLASS_VERSION(0, ::tim::vsettings)
// CEREAL_FORCE_DYNAMIC_INIT(timemory_settings_t)

namespace cereal
{
template <typename Archive, typename Tp>
void
save(Archive& ar, std::shared_ptr<tim::tsettings<Tp, Tp&>> obj)
{
    auto _obj = obj->clone();
    ar(_obj);
}
}  // namespace cereal
