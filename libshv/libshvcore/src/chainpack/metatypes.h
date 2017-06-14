#pragma once

namespace shv {
namespace core {
namespace chainpack {

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

struct ChunkHeaderKeys {
	enum Enum : unsigned {
		ChunkId = 0,
		ChunkIndex,
		ChunkCount,
	};
};

}}}
