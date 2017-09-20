#pragma once

#include "../shvcoreglobal.h"

#include <stdexcept>
#include <string>

namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT Exception : public std::exception
{
public:
	static constexpr bool Throw = true;
public:
	Exception(const std::string& _msg, const std::string& _where = std::string());
	~Exception() throw() override {}
public:
	//virtual void log();
	virtual std::string message() const {return m_msg;}
	virtual std::string where() const {return m_where;}
	const char* what() const noexcept override;
	//operator const char *() const  throw(){return what();}
	static void setAbortOnException(bool on) {s_abortOnException = on;}
	static bool isAbortOnException() {return s_abortOnException;}
protected:
	static bool s_abortOnException;
	std::string m_msg;
	std::string m_where;
};

}}

#define SHV_EXCEPTION(e) throw shv::core::Exception(e, std::string(__FILE__) + ":" + std::to_string(__LINE__));
/*
#define SHV_EXCEPTION(e) { \
	throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " SHV_EXCEPTION: " + e); \
	}
*/
