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
	enum Enum : unsigned {
		ChainPackRpcMessage = 1,
		ElesysDataChange,
	};
};

}
