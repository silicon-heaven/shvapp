#pragma once

#include "../shvcoreglobal.h"
#include "rpcvalue.h"

#include <functional>
#include <string>
#include <deque>
#include <map>

namespace shv {
namespace core {
namespace chainpack {

class SHVCORE_DECL_EXPORT RpcDriver
{
public:
	enum class SendPriority {High, Low};
public:
	explicit RpcDriver();
	virtual ~RpcDriver();

	void sendMessage(const shv::core::chainpack::RpcValue& msg, SendPriority priority = SendPriority::High);
	using MessageReceivedCallback = std::function< void (const shv::core::chainpack::RpcValue &msg)>;
	void setMessageReceivedCallback(const MessageReceivedCallback &callback) {m_messageReceivedCallback = callback;}
protected:
	virtual bool isOpen() = 0;
	virtual size_t bytesToWrite() = 0;
	virtual int64_t writeBytes(const char *bytes, size_t length) = 0;

	// call it when new data arrived
	void bytesRead(std::string &&bytes);
	// call it, when data are sent, when bytesToWrite() == 0
	void writePendingData();
private:
	class ChunkHeader : public shv::core::chainpack::RpcValue::IMap
	{
	public:
		shv::core::chainpack::RpcValue::UInt chunkId() const;
		void setChunkId(shv::core::chainpack::RpcValue::UInt id);
		shv::core::chainpack::RpcValue::UInt chunkIndex() const;
		void setChunkIndex(shv::core::chainpack::RpcValue::UInt id);
		shv::core::chainpack::RpcValue::UInt chunkCount() const;
		void setChunkCount(shv::core::chainpack::RpcValue::UInt id);
		std::string toStringData() const;
	};

	struct Chunk
	{
		std::string packedLength;
		std::string packedHeader;
		std::string data;

		Chunk(const ChunkHeader &h, std::string &&d) : packedHeader(h.toStringData()), data(std::move(d)) {fillPackedLength();}
		Chunk(std::string &&d) : data(std::move(d)) {fillPackedLength();}
		Chunk(std::string &&h, std::string &&d) : packedHeader(std::move(h)), data(std::move(d)) {fillPackedLength();}

		void fillPackedLength();
		size_t dataLength() const {return packedLength.length() + packedHeader.length() + data.length();}
	};
private:
	int processReadData(const std::string &read_data);
	void writeQueue(std::deque<Chunk> &queue);
private:
	MessageReceivedCallback m_messageReceivedCallback = nullptr;

	int m_lowPriorityChunkLength = 1000;
	static unsigned m_chunkId;
	std::deque<Chunk> m_highPriorityQueue;
	std::deque<Chunk> m_lowPriorityQueue;
	std::map<unsigned, std::vector<std::string>> m_incompletePacketChunks;
	std::string m_readData;
};

} // namespace chainpack
} // namespace core
} // namespace shv
