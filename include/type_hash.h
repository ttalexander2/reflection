#pragma once

#include <cstdint>

namespace reflection
{

    using type_id = uint32_t;

    namespace internal
    {
        struct type_hash final
        {
            [[nodiscard]] static type_id next() noexcept
            {
                static type_id value{};
                return value++;
            }
        };
    }

    template<typename T, typename = void>
    struct type_index final
    {
        static type_id value() noexcept
        {
            static const type_id value = internal::type_hash::next();
            return value;
        }
    };

    template<typename T, typename = void>
    struct type_hash final
    {
        [[nodiscard]] static constexpr type_id value() noexcept
        {
            return type_index<std::remove_cv_t<std::remove_reference_t<T>>>::value();
        }
    };

    template<typename T, typename = void>
    inline type_id type_hash_v = type_hash<T>::value();
}