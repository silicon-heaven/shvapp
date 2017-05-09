#pragma once

#include "../chainpackmessage.h"

namespace shv {
namespace chainpackrpc {
namespace protocol {

class JsonProtocol final
{

public:
	enum class Strategy { Standard, Comments };

	JsonProtocol(const std::string &str, std::string &err);

	static Message parseJson(const std::string & in, std::string & err);
	static Message parseJson(const char * in, std::string & err)
	{
		if (in) {
			return parseJson(std::string(in), err);
		}
		else {
			err = "null input";
			return nullptr;
		}
	}
private:
	const std::string &str;
	std::string &err;
	size_t i = 0;
	bool failed = false;
	const Strategy strategy = Strategy::Comments;


	Message fail(std::string &&msg);

	template <typename T>
	T fail(std::string &&msg, const T err_ret) {
		if (!failed)
			err = std::move(msg);
		failed = true;
		return err_ret;
	}

	void consume_whitespace();
	bool consume_comment();
	void consume_garbage();
	char get_next_token();
	void encode_utf8(long pt, std::string & out);
	std::string parse_string();
	Message parse_number();
	Message expect(const std::string &expected, Message res);
	Message parse_json(int depth);
public:
	static void dumpJson(std::nullptr_t, std::string &out);
	static void dumpJson(double value, std::string &out);
	static void dumpJson(int value, std::string &out);
	static void dumpJson(unsigned int value, std::string &out);
	static void dumpJson(bool value, std::string &out);
	static void dumpJson(Message::DateTime value, std::string &out);
	static void dumpJson(const std::string &value, std::string &out);
	static void dumpJson(const Message::Blob &value, std::string &out);
	static void dumpJson(const Message::List &values, std::string &out);
	static void dumpJson(const Message::Map &values, std::string &out);
};

}
} // namespace chainpack
} // namespace shv
