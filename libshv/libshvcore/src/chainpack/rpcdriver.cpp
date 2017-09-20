#include "rpcdriver.h"
#include "metatypes.h"
#include "chainpackprotocol.h"
#include "../core/shvexception.h"
#include "../core/log.h"

#include <sstream>
#include <iostream>

#define logRpc() shvCDebug("rpc")
#define logLongFiles() shvCDebug("LongFiles")

namespace shv {
namespace core {
namespace chainpack {

int RpcDriver::s_defaultRpcTimeout = 5000;
/*
core::chainpack::RpcValue::UInt RpcDriver::ChunkHeader::chunkId() const
{
	return (count(shv::core::chainpack::ChunkHeaderKeys::ChunkId) == 0)? 0: at(shv::core::chainpack::ChunkHeaderKeys::ChunkId).toUInt();
}

void RpcDriver::ChunkHeader::setChunkId(core::chainpack::RpcValue::UInt id)
{
	(*this)[shv::core::chainpack::ChunkHeaderKeys::ChunkId] = id;
}

core::chainpack::RpcValue::UInt RpcDriver::ChunkHeader::chunkIndex() const
{
	return (count(shv::core::chainpack::ChunkHeaderKeys::ChunkIndex) == 0)? 0: at(shv::core::chainpack::ChunkHeaderKeys::ChunkIndex).toUInt();
}

void RpcDriver::ChunkHeader::setChunkIndex(core::chainpack::RpcValue::UInt id)
{
	(*this)[shv::core::chainpack::ChunkHeaderKeys::ChunkIndex] = id;
}

core::chainpack::RpcValue::UInt RpcDriver::ChunkHeader::chunkCount() const
{
	return (count(shv::core::chainpack::ChunkHeaderKeys::ChunkCount) == 0)? 0: at(shv::core::chainpack::ChunkHeaderKeys::ChunkCount).toUInt();
}

void RpcDriver::ChunkHeader::setChunkCount(core::chainpack::RpcValue::UInt id)
{
	(*this)[shv::core::chainpack::ChunkHeaderKeys::ChunkCount] = id;
}

std::string RpcDriver::ChunkHeader::toStringData() const
{
	std::ostringstream os;
	shv::core::chainpack::ChainPackProtocol::writeChunkHeader(os, *this);
	return os.str();
}
*/
/*
std::string RpcDriver::Chunk::packedLength() const
{
	std::ostringstream os;
	shv::core::chainpack::ChainPackProtocol::writeUIntData(os, data.length());
	return os.str();
}
*/
RpcDriver::RpcDriver()
{
}

RpcDriver::~RpcDriver()
{
}

void RpcDriver::sendMessage(const shv::core::chainpack::RpcValue &msg)
{
	using namespace std;
	shvLogFuncFrame() << msg.toStdString();
	std::ostringstream os_packed_data;
	shv::core::chainpack::ChainPackProtocol::write(os_packed_data, msg);
	std::string packed_data = os_packed_data.str();
	logRpc() << "send message: packed data: " << (packed_data.size() > 50? "<... long data ...>" : packed_data);

	logLongFiles() << "High priority message enqueued:" << packed_data.length() << "bytes";
	m_highPriorityQueue.push_back(Chunk{std::move(packed_data)});

	writePendingData();
}

void RpcDriver::writePendingData()
{
	if(!isOpen()) {
		shvError() << "write data error, socket is not open!";
		return;
	}
	if(bytesToWrite() > 0) {
		logRpc() << "skipping write because of bytesToWrite:" << bytesToWrite() << "try to flush";
		bool ok = flushNoBlock();
		logRpc() << "any data flushed:" << ok << "rest bytesToWrite:" << bytesToWrite();
		// wait for bytesWritten signal and send data from right queue then
		//shvWarning() << "writePendingData() called for not empty socket buffer";
		return;
	}
	if(!m_highPriorityQueue.empty()) {
		logRpc() << "writePendingData(), HI prio queue len:" << m_highPriorityQueue.size();
		writeQueue(m_highPriorityQueue, m_headChunkBytesWrittenSoFar);
	}
}

namespace {
const unsigned PROTOCOL_VERSION = 1;
}

void RpcDriver::writeQueue(std::deque<Chunk> &queue, size_t &bytes_written_so_far)
{
	//static int hi_cnt = 0;
	const Chunk &chunk = queue[0];

	if(bytes_written_so_far == 0) {
		std::string protocol_version_data;
		{
			std::ostringstream os;
			shv::core::chainpack::ChainPackProtocol::writeUIntData(os, PROTOCOL_VERSION);
			protocol_version_data = os.str();
		}
		{
			std::ostringstream os;
			shv::core::chainpack::ChainPackProtocol::writeUIntData(os, chunk.data.length() + protocol_version_data.length());
			std::string data = os.str();
			auto len = writeBytes(data.data(), data.length());
			if(len < 0)
				SHV_EXCEPTION("Write socket error!");
			if(len < (int)data.length())
				SHV_EXCEPTION("Design error! Chunk length shall be always written at once to the socket");
		}
		{
			auto len = writeBytes(protocol_version_data.data(), protocol_version_data.length());
			if(len < 0)
				SHV_EXCEPTION("Write socket error!");
			if(len != 1)
				SHV_EXCEPTION("Design error! Protocol version shall be always written at once to the socket");
		}
	}

	{
		//int data_written_so_far;
		const std::string &data = chunk.data;
		//int len_to_write = data.length() - bytes_written_so_far;
		auto len = writeBytes(data.data() + bytes_written_so_far, data.length() - bytes_written_so_far);
		if(len < 0)
			SHV_EXCEPTION("Write socket error!");
		if(len == 0)
			SHV_EXCEPTION("Design error! At least 1 byte of data shall be always written to the socket");

		//logLongFiles() << "#" << ++hi_cnt << "HiPriority message written to socket, data written:" << len << "of:" << (data.length() - bytes_written_so_far);
		logRpc() << "writeQueue - data len:" << data.length() << "start index:" << bytes_written_so_far << "bytes written:" << len << "remaining:" << (data.length() - bytes_written_so_far - len);
		bytes_written_so_far += len;
		if(bytes_written_so_far == chunk.data.length()) {
			bytes_written_so_far = 0;
			queue.pop_front();
			//shvInfo() << len << "bytes written (all)";
			//return true;
		}
	}
	//shvWarning() << "not all bytes written:" << len << "of" << data.length();
	//return false;
}
/*
/// this implementation expects, that Chunk can be always written at once to the socket
void RpcDriver::writeQueue(std::deque<Chunk> &queue)
{
	const Chunk &chunk = queue[0];

	const std::string* fields[] = {&chunk.packedLength, &chunk.packedHeader, &chunk.data};
	static constexpr int FLD_CNT = 3;
	for (int i = 0; i < FLD_CNT; ++i) {
		const std::string* fld = fields[i];
		auto len = writeBytes(fld->data(), fld->length());
		if(len < 0)
			SHV_EXCEPTION("Write socket error!");
		if(len < (int)fld->length())
			SHV_EXCEPTION("Design error! Chunk shall be always written at once to the socket");
	}
	queue.pop_front();
}
*/

void RpcDriver::bytesRead(std::string &&bytes)
{
	logRpc() << bytes.length() << "bytes of data read";
	m_readData += std::string(std::move(bytes));
	while(true) {
		int len = processReadData(m_readData);
		//shvInfo() << len << "bytes of" << m_readData.size() << "processed";
		if(len > 0)
			m_readData = m_readData.substr(len);
		else
			break;
	}
}

int RpcDriver::processReadData(const std::string &read_data)
{
	logRpc() << __FUNCTION__ << "data len:" << read_data.length();
	using namespace shv::core::chainpack;

	std::istringstream in(read_data);
	bool ok;

	uint64_t chunk_len = ChainPackProtocol::readUIntData(in, &ok);
	if(!ok)
		return 0;

	size_t read_len = (size_t)in.tellg() + chunk_len;

	uint64_t protocol_version = ChainPackProtocol::readUIntData(in, &ok);
	if(!ok)
		return 0;

	if(protocol_version != PROTOCOL_VERSION)
		SHV_EXCEPTION("Unsupported protocol version");

	logRpc() << "\t chunk len:" << chunk_len << "read_len:" << read_len << "stream pos:" << in.tellg();
	if(read_len > read_data.length())
		return 0;

	RpcValue msg = ChainPackProtocol::read(in);
	{
		logRpc() << "\t emitting message received:" << msg.toStdString();
		//logLongFiles() << "\t emitting message received:" << msg.dumpText();
		if(m_messageReceivedCallback)
			m_messageReceivedCallback(msg);
	}
	return read_len;
}

} // namespace chainpack
} // namespace core
} // namespace shv
