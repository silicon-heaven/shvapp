#pragma once

#include "../shvcoreglobal.h"

#include <sstream>
#include <functional>
#include <vector>

namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT ShvLog
{
public:
	enum class Level : int { Invalid = -1, Fatal, Error, Warning, Info, Debug };

	class SHVCORE_DECL_EXPORT LogContext
	{
	public:
		LogContext() : line(0), file(nullptr), function(nullptr), category(nullptr) {}
		LogContext(const char *file_name, int line_number, const char *function_name, const char *category_name = nullptr)
			: line(line_number), file(file_name), function(function_name), category(category_name) {}

		int line;
		const char *file;
		const char *function;
		const char *category;
	};
	using MessageOutput = std::function<void (Level level, const LogContext &context, const std::string &message)>;
private:
	struct Stream
	{
		//Stream(Level level) : ts(&buffer), level(level) {}
		Stream(Level level, const LogContext &context, std::streambuf *buff = nullptr)
			: ts((buff == nullptr)? &buffer: buff)
			, level(level)
			, context(context)
		{ }
		//Stream(std::streambuf *buff) : ts(buff) {}

		std::ostream ts;
		std::stringbuf buffer;
		int ref = 1;
		bool space = true;
		//bool message_output;
		Level level = Level::Debug;
		LogContext context;
	} *stream;
	//void putUcs4(uint ucs4);
	//void putString(const QChar *begin, size_t length);
	//void putByteArray(const char *begin, size_t length, Latin1Content content);
public:
	//inline ShvLog(std::ostream *device) : stream(new Stream(device)) {}
	//inline ShvLog(QString *string) : stream(new Stream(string)) {}
	inline ShvLog(Level level, const LogContext &context, std::streambuf *buff = nullptr) : stream(new Stream(level, context, buff)) {}
	inline ShvLog(const ShvLog &o) : stream(o.stream) { ++stream->ref; }
	inline ShvLog &operator=(const ShvLog &other);
	~ShvLog();

	static std::vector<std::string> setGlobalTresholds(int argc, char *argv[]);
	static bool isMatchingLogFilter(ShvLog::Level level, const ShvLog::LogContext &log_context);
	static void setMessageOutput(MessageOutput out);
	static ShvLog::MessageOutput messageOutput();

	inline void swap(ShvLog &other) noexcept { std::swap(stream, other.stream); }
	//ShvLog &resetFormat();
	inline ShvLog &space() { stream->space = true; stream->ts << ' '; return *this; }
	inline ShvLog &nospace() { stream->space = false; return *this; }
	inline ShvLog &maybeSpace() { if (stream->space) stream->ts << ' '; return *this; }
	//int verbosity() const { return stream->verbosity(); }
	//void setVerbosity(int verbosityLevel) { stream->setVerbosity(verbosityLevel); }
	//bool autoInsertSpaces() const { return stream->space; }
	//void setAutoInsertSpaces(bool b) { stream->space = b; }
	//inline ShvLog &quote() { stream->unsetFlag(Stream::NoQuotes); return *this; }
	//inline ShvLog &noquote() { stream->setFlag(Stream::NoQuotes); return *this; }
	//inline ShvLog &maybeQuote(char c = '"') { if (!(stream->testFlag(Stream::NoQuotes))) stream->ts << c; return *this; }
	/*
	//inline ShvLog &operator<<(QChar t) { putUcs4(t.unicode()); return maybeSpace(); }
	inline ShvLog &operator<<(bool t) { stream->ts << (t ? "true" : "false"); return maybeSpace(); }
	inline ShvLog &operator<<(char t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(signed short t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(unsigned short t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(signed int t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(unsigned int t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(signed long t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(unsigned long t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(int64_t t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(uint64_t t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(float t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(double t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(const char* t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(const std::string & t) { stream->ts << t; return maybeSpace(); }
	//inline ShvLog &operator<<(const QByteArray & t) { putByteArray(t.constData(), t.size(), ContainsBinary); return maybeSpace(); }
	*/
	template<typename T>
	inline ShvLog &operator<<(T t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(const void * t) { stream->ts << t; return maybeSpace(); }
	inline ShvLog &operator<<(std::nullptr_t) { stream->ts << "(nullptr)"; return maybeSpace(); }

	static std::string modulesLogInfo();
	static std::string categoriesLogInfo();
	static const char *levelToString(ShvLog::Level level);
	static const char *logCLIHelp();
};

inline ShvLog &ShvLog::operator=(const ShvLog &other)
{
	if (this != &other) {
		ShvLog copy(other);
		std::swap(stream, copy.stream);
	}
	return *this;
}

}}
