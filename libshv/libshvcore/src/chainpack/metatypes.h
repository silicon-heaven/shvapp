#pragma once

namespace shv {
namespace core {
namespace chainpack {

class MetaTypes
{
public:
	struct Tag
	{
		enum Enum {
			Invalid = 0,
			MetaTypeId,
			MetaTypeNameSpaceId,
			//MetaIMap,
			//MetaTypeName,
			//MetaTypeNameSpaceName,
			USER = 8
		};
	};

	template<int V>
	struct NameSpace
	{
		static constexpr int Value = V;
	};
	template<int V>
	struct TypeId
	{
		static constexpr int Value = V;
	};
	struct Global : public NameSpace<0>
	{
		struct ChainPackRpcMessage : public TypeId<0>
		{
			struct Tag
			{
				struct RpcCallType
				{
					enum Enum {Undefined = 0, Request, Response, Notify};
				};
				enum Enum {
					RequestId = MetaTypes::Tag::USER,
					RpcCallType,
					DeviceId,
					MAX
				};
			};
		};
		//struct SkyNetRpcMessage : public TypeId<ChainPackRpcMessage::Value + 1> {};
	};
public:
	static const char *metaTypeName(int namespace_id, int type_id);
	static const char *metaKeyName(int namespace_id, int type_id, int tag);
};

}}}
