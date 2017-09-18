#pragma once

namespace shv {
namespace core {
namespace chainpack {

struct MetaTypeNameSpaceId {
	enum Enum {
		Global = 0,
		//Eyas,
		//Elesys,
	};
};

struct GlobalMetaTypeId {
	enum Enum {
		ChainPackRpcMessage = 1,
		//SkyNetRpcMessage, // like ChainPackRpcMessage + SkyNetRpcMessageMetaKeys
		//ElesysDataChange,
	};
};

struct ReservedMetaKey
{
	enum Enum {
		Invalid = 0,
		MetaTypeId,
		MetaTypeNameSpaceId,
		//MetaIMap,
		//MetaTypeName,
		//MetaTypeNameSpaceName,
		MAX_KEY = 8
	};
	//static const char* name(Enum e);
};

struct ChainPackRpcMessageMetaKey
{
	struct RpcCallType
	{
		enum Enum {Undefined = 0, Request, Response, Notify};
	};
	enum Enum {
		RequestId = ReservedMetaKey::MAX_KEY,
		RpcCallType,
		DeviceId,
		MAX_KEY
	};
};
/*
struct SkyNetRpcMessageMetaKey
{
	enum Enum {
		DeviceId = ChainPackRpcMessageMetaKey::MAX_KEY,
		MAX_KEY
	};
};
*/
class MetaType
{
public:
	static const char *metaTypeName(int meta_type_namespace_id, int meta_type_id);
	static const char *metaKeyName(int meta_type_namespace_id, int meta_type_id, int meta_key_id);
};

}}}
