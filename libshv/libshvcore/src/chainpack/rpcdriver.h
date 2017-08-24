#pragma once

#include "../shvcoreglobal.h"
#include "rpcmessage.h"

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
	explicit RpcDriver();
	virtual ~RpcDriver();

	void sendMessage(const RpcValue &msg);
	using MessageReceivedCallback = std::function< void (const shv::core::chainpack::RpcValue &msg)>;
	void setMessageReceivedCallback(const MessageReceivedCallback &callback) {m_messageReceivedCallback = callback;}

	static int defaultRpcTimeout() {return s_defaultRpcTimeout;}
	static void setDefaultRpcTimeout(int tm) {s_defaultRpcTimeout = tm;}
protected:
	virtual bool isOpen() = 0;
	virtual size_t bytesToWrite() = 0;
	virtual int64_t writeBytes(const char *bytes, size_t length) = 0;
	virtual bool flushNoBlock() = 0;

	// call it when new data arrived
	void bytesRead(std::string &&bytes);
	// call it, when data are sent, when bytesToWrite() == 0
	void writePendingData();
private:
	/*
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
	*/
	struct Chunk
	{
		//std::string packedLength;
		//std::string packedHeader;
		std::string data;

		//Chunk(const ChunkHeader &h, std::string &&d) : packedHeader(h.toStringData()), data(std::move(d)) {fillPackedLength();}
		Chunk(std::string &&d) : data(std::move(d)) {}
		//Chunk(std::string &&h, std::string &&d) : packedHeader(std::move(h)), data(std::move(d)) {fillPackedLength();}

		//std::string packedLength() const;
		size_t dataLength() const {return data.length();}
	};
private:
	int processReadData(const std::string &read_data);
	void writeQueue(std::deque<Chunk> &queue, size_t &bytes_written_so_far);
private:
	MessageReceivedCallback m_messageReceivedCallback = nullptr;

	std::deque<Chunk> m_highPriorityQueue;
	size_t m_headChunkBytesWrittenSoFar = 0;
	std::string m_readData;
	static int s_defaultRpcTimeout;
};

} // namespace chainpack
} // namespace core
} // namespace shv
