#pragma once

#include <type_traits>
#include <string>
#include <vector>
#include <iostream>
#include "basic_hash.h"
#include "type_data.h"
#include "type.h"
#include "type_hash.h"
#include "util.h"

namespace reflection
{

    template<typename Type>
    class type_factory
    {
        friend class type_data;

        friend class registry;

        template<typename T>
        friend
        class type_factory;

        using id_hash = basic_hash<uint32_t>;

    public:
        template<typename B>
        type_factory<Type> &base() noexcept
        {
            static_assert(!std::is_same_v<B, Type> && std::is_base_of_v<B, Type>, "Invalid base type.");

            type_info *info = get_type_info<B>();
            if (info != nullptr)
            {
                info->bases.push_back(info->id);
            }
            return *this;
        }

        template<typename To>
        type_factory<Type> &conversion() noexcept
        {
            static_assert(std::is_convertible_v<Type, To>, "Type is not convertable.");

            type_info *info = get_type_info<To>();

            if (info == nullptr)
            {
                // Register type
                type_factory<To>::register_type();
                info = get_type_info<To>();
            }

            if (info != nullptr)
            {
                info->conversions.push_back(info->id);
            }

            return *this;
        }

        template<typename... Args>
        type_factory<Type> &ctor() noexcept
        {
            type_info *info = _type.info();
            if (info != nullptr)
            {
                ctor_info ctor;
                ctor.type_id = _type.id();
                ctor.arity = sizeof...(Args);
                // Register the types
                (type_factory<Args>::register_type(), ...);

                // Add the constructor arguments to the ctor_info
                (ctor.args.push_back(type_hash<Args>::value()), ...);

                info->constructors.push_back(ctor);
            }

            return *this;
        }

        template<auto Data>
        type_factory &data(const std::string &name, bool serialize = true)
        {
            uint32_t hash = basic_hash<uint32_t>::hash(name);
            type_info *info = _type.info();
            if (info != nullptr)
            {
                data_info data;
                data.id = hash;
                data.name = name;
                data.flags = data_flags::none;

                if (serialize)
                {
                    data.flags |= data_flags::is_serialized;
                }

                if constexpr (std::is_member_object_pointer_v<decltype(Data)>)
                {
                    using data_type = std::remove_reference_t<std::invoke_result_t<decltype(Data), Type &>>;

                    type_factory<data_type>::register_type();
                    data.type_id = type_hash_v<data_type>;
                    if (std::is_const_v<data_type>)
                    {
                        data.flags |= data_flags::is_const;
                    }
                    data.get = &internal::get_function<Type, Data>;
                    data.set = &internal::set_function<Type, Data>;
                } else
                {
                    using data_type = std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>;

                    type_factory<data_type>::register_type();
                    data.type_id = type_hash_v<data_type>;
                    if (std::is_same_v<Type, std::remove_const_t<data_type>> || std::is_const_v<data_type>)
                    {
                        data.flags |= data_flags::is_const;
                    }
                    data.flags |= data_flags::is_static;
                    data.get = &internal::get_function<Type, Data>;
                    data.set = &internal::set_function<Type, Data>;
                }

                info->data[hash] = data;
            }

            return *this;
        }

        template<auto Getter, auto Setter>
        type_factory &data(const std::string &name, bool serialize = true)
        {
            uint32_t hash = basic_hash<uint32_t>::hash(name);

            return *this;
        }

        template<auto Func>
        type_factory &function(const std::string &name)
        {
            uint32_t hash = basic_hash<uint32_t>::hash(name);

            return *this;
        }

    private:
        explicit type_factory(const std::string &name) : _type(type_hash<Type>::value())
        {
            register_type(name);
        }

        explicit type_factory() : _type(type_hash<Type>::value())
        {
            register_type();
        }

        static void register_type(const std::string &name)
        {
            auto id = type_hash<Type>::value();
            // std::cout << "registered: " << name << " [" << id << "]\n";
            auto hash = basic_hash<uint32_t>::hash(name);

            if (type_data::instance().types.find(id) == type_data::instance().types.end())
            {
                type_info info;
                info.name = name;
                info.id = id;
                info.flags = type_data::get_flags<Type>();
                info.underlying_type_id = get_underlying_type();

                type_data::instance().type_aliases[hash] = id;
                type_data::instance().types[id] = info;
            }
            else
            {
                type_info *info = get_type_info<Type>();
                if (info != nullptr)
                {
                    type_data::instance().type_aliases[hash] = id;
                    info->name = name;
                }
            }
        }

        static void register_type()
        {
            auto id = type_hash<Type>::value();
            //std::cout << "registered: " << id << "\n";

            if (type_data::instance().types.find(id) == type_data::instance().types.end())
            {
                type_info info;
                info.id = id;
                info.flags = type_data::get_flags<Type>();
                info.underlying_type_id = get_underlying_type();

                type_data::instance().types[id] = info;
            }
        }

        static uint32_t get_underlying_type()
        {
            if constexpr (std::is_pointer_v<Type> || internal::is_pointer_like_v<Type>)
            {
                auto factory = type_factory<typename internal::remove_all_pointers<Type>::type>();
                return factory._type._id;
            }

            return type_hash<void>::value();
        }

        template<typename T>
        static type_info *get_type_info()
        {
            type t(type_hash<T>::value());
            if (t.valid())
            {
                return t.info();
            }
            return nullptr;


        }

        type _type;
    };


    template<template<typename> typename T>
    class template_type_factory : public type_factory<T<void>>
    {
    public:
        explicit template_type_factory(const std::string& name) : type_factory<T<void>>(name)
        {

        }
    };
}
