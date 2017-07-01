#include "rpcdriver.h"
#include "metatypes.h"
#include "chainpackprotocol.h"
#include "../shvexception.h"
#include "../log.h"

#include <sstream>
#include <iostream>

#define logRpc() shvCDebug("rpc")
#define logLongFiles() shvCDebug("LongFiles")

namespace shv {
namespace core {
namespace chainpack {

core::chainpack::RpcValue::UInt RpcDriver::ChunkHeader::chunkId() const
{
	return (count(shv::core::chainpack::ChunkHeaderKeys::ChunkId) == 0)? 0: at(shv::core::chainpack::ChunkHeaderKeys::ChunkId).toUInt();
}

void RpcDriver::ChunkHeader::setChunkId(core::chainpack::RpcValue::UInt id)
{
	at(shv::core::chainpack::ChunkHeaderKeys::ChunkId) = id;
}

core::chainpack::RpcValue::UInt RpcDriver::ChunkHeader::chunkIndex() const
{
	return (count(shv::core::chainpack::ChunkHeaderKeys::ChunkIndex) == 0)? 0: at(shv::core::chainpack::ChunkHeaderKeys::ChunkIndex).toUInt();
}

void RpcDriver::ChunkHeader::setChunkIndex(core::chainpack::RpcValue::UInt id)
{
	at(shv::core::chainpack::ChunkHeaderKeys::ChunkIndex) = id;
}

core::chainpack::RpcValue::UInt RpcDriver::ChunkHeader::chunkCount() const
{
	return (count(shv::core::chainpack::ChunkHeaderKeys::ChunkCount) == 0)? 0: at(shv::core::chainpack::ChunkHeaderKeys::ChunkCount).toUInt();
}

void RpcDriver::ChunkHeader::setChunkCount(core::chainpack::RpcValue::UInt id)
{
	at(shv::core::chainpack::ChunkHeaderKeys::ChunkCount) = id;
}

std::string RpcDriver::ChunkHeader::toStringData() const
{
	std::ostringstream os;
	shv::core::chainpack::ChainPackProtocol::writeChunkHeader(os, *this);
	return os.str();
}

void RpcDriver::Chunk::fillPackedLength()
{
	std::ostringstream os;
	shv::core::chainpack::ChainPackProtocol::writeUIntData(os, packedHeader.length() + data.length());
	packedLength = os.str();
}

unsigned RpcDriver::m_chunkId = 0;

RpcDriver::RpcDriver()
{
}

RpcDriver::~RpcDriver()
{
}

void RpcDriver::sendMessage(const shv::core::chainpack::RpcValue &msg, SendPriority priority)
{
	using namespace std;
	shvLogFuncFrame() << msg.dumpText();
	std::ostringstream os_packed_data;
	shv::core::chainpack::ChainPackProtocol::write(os_packed_data, msg);
	std::string packed_data = os_packed_data.str();
	logRpc() << "packed data: " << packed_data;

	if(priority == SendPriority::High) {
		logLongFiles() << "High priority message enqueued:" << packed_data.length() << "bytes";
		m_highPriorityQueue.push_back(Chunk{std::move(packed_data)});
	}
	else {
		unsigned chunk_count = packed_data.length() / m_lowPriorityChunkLength;
		if(packed_data.length() % m_lowPriorityChunkLength)
			chunk_count++;
		if(chunk_count > 1) {
			logLongFiles() << "Sending long message:" << chunk_count << "chunks, every of:" << m_lowPriorityChunkLength << "bytes";
			m_chunkId++;
			for (unsigned chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
				std::string chunk_data = packed_data.substr(chunk_index * m_lowPriorityChunkLength, m_lowPriorityChunkLength);
				logRpc() << "\t len:" << chunk_data.length() << " chunk_data:" << chunk_data;
				ChunkHeader header;
				header.setChunkId(m_chunkId);
				header.setChunkIndex(chunk_index);
				header.setChunkCount(chunk_count);
				logRpc() << "\t chunk #" << chunk_index << "len:" << chunk_data.length();
				m_lowPriorityQueue.push_back(Chunk{header, std::move(chunk_data)});
			}
			logLongFiles() << chunk_count << "chunks, enqueued";
		}
		else {
			m_lowPriorityQueue.push_back(Chunk{std::move(packed_data)});
		}
	}
	writePendingData();
}

void RpcDriver::writePendingData()
{
	logRpc() << "writePendingData()";
	if(!isOpen()) {
		shvError() << "write data error, socket is not open!";
		return;
	}
	if(bytesToWrite() > 0) {
		// wait for bytesWritten signal and send data from right queue then
		//shvWarning() << "writePendingData() called for not empty socket buffer";
		return;
	}
	if(!m_highPriorityQueue.empty()) {
		writeQueue(m_highPriorityQueue);
	}
	else if(!m_lowPriorityQueue.empty()) {
		writeQueue(m_lowPriorityQueue);
	}
}

/*
bool RpcDriver::writeQueue(std::deque<Chunk> &queue, size_t &bytes_written_so_far)
{
	static int hi_cnt = 0;
	static int lo_cnt = 0;
	const Chunk &chunk = queue[0];
	while(true) {
		int data_written_so_far;
		const std::string &data = chunk.fieldAt(bytes_written_so_far, data_written_so_far);
		int len_to_write = data.length() - data_written_so_far;
		qint64 len = socket()->write(data.data() + data_written_so_far, len_to_write);
		if(len < 0) {
			//emit socketError();
			SHV_EXCEPTION("Write socket error!");
			return false;
		}
		//if((size_t)len != data.length() - bytes_written_so_far)
		//	shvWarning() << "not all data written";
		bool hi_queue = (&queue == &m_highPriorityQueue);
		if(hi_queue)
			logLongFiles() << "#" << ++hi_cnt << "HiPriority message written to socket, data written:" << len << "of:" << (data.length() - bytes_written_so_far);
		else
			logLongFiles() << "#" << ++lo_cnt << "LoPriority message written to socket, data written:" << len << "of:" << (data.length() - bytes_written_so_far);
		logRpc() << ((hi_queue)? "HI-Queue": "LO-Queue") << "data len:" << data.length() << "start index:" << bytes_written_so_far << "bytes written:" << len << "remaining:" << (data.length() - bytes_written_so_far - len);
		bytes_written_so_far += len;
		if(bytes_written_so_far == chunk.dataLength()) {
			bytes_written_so_far = 0;
			queue.pop_front();
			//shvInfo() << len << "bytes written (all)";
			return true;
		}
		if(len < len_to_write)
			break;
	}
	//shvWarning() << "not all bytes written:" << len << "of" << data.length();
	return false;
}
*/
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

void RpcDriver::bytesRead(std::string &&bytes)
{
	//shvInfo() << ba.size() << "bytes of data read";
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
	logRpc() << "\t chunk len:" << chunk_len << "read_len:" << read_len << "stream pos:" << in.tellg();
	if(read_len > read_data.length())
		return 0;
	core::chainpack::RpcValue msg;
	bool complete_msg = false;
	const RpcValue::IMap header = ChainPackProtocol::readChunkHeader(in, &ok);
	if(ok) {
		auto chunk_data_start = in.tellg();
		logRpc() << "\t chunk header found:" << RpcValue(header).dumpText() << "stream pos:" << chunk_data_start;
		unsigned id = header.at(shv::core::chainpack::ChunkHeaderKeys::ChunkId).toUInt();
		unsigned ix = header.at(shv::core::chainpack::ChunkHeaderKeys::ChunkIndex).toUInt();
		unsigned cnt = header.at(shv::core::chainpack::ChunkHeaderKeys::ChunkCount).toUInt();
		logLongFiles() << "Chunk header:" << id << "chunk:" << ix << '/' << cnt;
		std::vector<std::string>& chunks = m_incompletePacketChunks[id];
		if(chunks.size() == 0)
			chunks.resize(cnt);
		if(chunks.size() != cnt)
			SHV_EXCEPTION("Wrong chunk count detected!");
		std::string chunk = read_data.substr(chunk_data_start, read_len - chunk_data_start);
		logRpc() << "\t len: " << chunk.length() << "chunk data:" << chunk;
		chunks[ix] = std::move(chunk);
		if(chunks[ix].length() == 0)
			SHV_EXCEPTION("Empty chunk received!");
		bool all_chunks_received = true;
		for (unsigned i = 0; i < cnt; ++i) {
			if(chunks[i].length() == 0) {
				all_chunks_received = false;
				break;
			}
		}
		if(all_chunks_received) {
			std::string data;
			for (unsigned i = 0; i < cnt; ++i) {
				data += chunks[i];
			}
			std::istringstream ins(data);
			msg = ChainPackProtocol::read(ins);
			complete_msg = true;
		}
	}
	else {
		msg = ChainPackProtocol::read(in);
		complete_msg = true;
	}
	if(complete_msg) {
		logRpc() << "\t emitting message received:" << msg.dumpText();
		logLongFiles() << "\t emitting message received:" << msg.dumpText();
		if(m_messageReceivedCallback)
			m_messageReceivedCallback(msg);
	}
	return read_len;
}

} // namespace chainpack
} // namespace core
} // namespace shv
