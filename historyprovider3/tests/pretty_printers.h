#pragma once

#include <doctest/doctest.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcvalue.h>
#include <vector>
#include <sstream>

// https://stackoverflow.com/a/56766138
template <typename T>
constexpr auto type_name()
{
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}


namespace std {
    template <typename Type>
    doctest::String toString(const std::vector<Type>& values) {
        std::ostringstream res;
        res << "std::vector<" << type_name<Type>() << ">{\n";
        for (const auto& value : values) {
            if constexpr (std::is_same<Type, shv::core::utils::ShvJournalEntry>()) {
                res << "    " << value.toRpcValue().toCpon() << ",\n";
            } else {
                res << "    " << value << ",\n";
            }
        }
        res << "}";
        return res.str().c_str();
    }
}

namespace shv::chainpack {

    doctest::String toString(const RpcValue& value) {
        return value.toCpon().c_str();
    }

    doctest::String toString(const RpcValue::List& value) {
        return RpcValue(value).toCpon().c_str();
    }

    doctest::String toString(const RpcValue::Map& value) {
        return RpcValue(value).toCpon().c_str();
    }

    doctest::String toString(const RpcMessage& value) {
        return value.toPrettyString().c_str();
    }
}

