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

	template<unsigned V>
	struct NameSpace
	{
		static constexpr int Value = V;
	};
	template<unsigned V>
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
	struct Elesys : public NameSpace<1>
	{
	};
public:
	static const char *metaTypeName(int namespace_id, int type_id);
	static const char *metaKeyName(int namespace_id, int type_id, int tag);
};

}}}
