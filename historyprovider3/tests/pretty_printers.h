#pragma once

#include <doctest/doctest.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcvalue.h>
#include <vector>
#include <sstream>

namespace std {
    doctest::String toString(const std::vector<int64_t>& values) {
		std::ostringstream res;
		res << "std::vector<int64_t>{";
		for (const auto& value : values) {
			res << value << ", ";
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

    doctest::String toString(const RpcMessage& value) {
        return value.toPrettyString().c_str();
    }
}

