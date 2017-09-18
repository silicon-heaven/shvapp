#include "metatypes.h"

namespace shv {
namespace core {
namespace chainpack {

const char *MetaType::metaKeyName(int meta_type_namespace_id, int meta_type_id, int meta_key_id)
{
	switch(meta_key_id) {
	case ReservedMetaKey::MetaTypeId: return "T";
	case ReservedMetaKey::MetaTypeNameSpaceId: return "S";
	}
	if(meta_type_namespace_id == MetaTypeNameSpaceId::Global) {
		if(meta_type_id == GlobalMetaTypeId::ChainPackRpcMessage /*|| meta_type_id == GlobalMetaTypeId::SkyNetRpcMessage*/) {
			switch(meta_key_id) {
			case ChainPackRpcMessageMetaKey::RequestId: return "RqId";
			case ChainPackRpcMessageMetaKey::RpcCallType: return "Type";
			case ChainPackRpcMessageMetaKey::DeviceId: return "DevId";
			}
		}
		/*
		else if(meta_type_id == GlobalMetaTypeId::SkyNetRpcMessage) {
			switch(meta_key_id) {
			case SkyNetRpcMessageMetaKey::DeviceId: return "DeviceId";
			}
		}
		*/
	}
	return "";
}


const char *MetaType::metaTypeName(int meta_type_namespace_id, int meta_type_id)
{
	if(meta_type_namespace_id == MetaTypeNameSpaceId::Global) {
		switch(meta_type_id) {
		case GlobalMetaTypeId::ChainPackRpcMessage: return "Rpc";
		//case GlobalMetaTypeId::SkyNetRpcMessage: return "SkyRpc";
		}
	}
	return "";
}

}}}
