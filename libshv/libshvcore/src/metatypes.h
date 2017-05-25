#pragma once

namespace shv {

struct MetaTypeNameSpaceId {
	enum Enum : unsigned {
		Global = 0,
		Eyas,
		Elesys,
	};
};

struct GlobalMetaTypeId {
	enum Enum : uint64_t {
		ChainPackRpcMessage = 1,
		ElesysDataChange,
	};
};

}
