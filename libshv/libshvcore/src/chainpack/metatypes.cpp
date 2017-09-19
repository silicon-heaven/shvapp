#include "metatypes.h"

namespace shv {
namespace core {
namespace chainpack {

const char *MetaTypes::metaKeyName(int namespace_id, int type_id, int tag)
{
	switch(tag) {
	case MetaTypes::Tag::MetaTypeId: return "T";
	case MetaTypes::Tag::MetaTypeNameSpaceId: return "S";
	}
	if(namespace_id == MetaTypes::Global::Value) {
		if(type_id == MetaTypes::Global::ChainPackRpcMessage::Value /*|| meta_type_id == GlobalMetaTypeId::SkyNetRpcMessage*/) {
			switch(tag) {
			case MetaTypes::Global::ChainPackRpcMessage::Tag::RequestId: return "RqId";
			case MetaTypes::Global::ChainPackRpcMessage::Tag::RpcCallType: return "Type";
			case MetaTypes::Global::ChainPackRpcMessage::Tag::DeviceId: return "DevId";
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


const char *MetaTypes::metaTypeName(int namespace_id, int type_id)
{
	if(namespace_id == MetaTypes::Global::Value) {
		switch(type_id) {
		case MetaTypes::Global::ChainPackRpcMessage::Value: return "Rpc";
		//case GlobalMetaTypeId::SkyNetRpcMessage: return "SkyRpc";
		}
	}
	return "";
}

}}}
