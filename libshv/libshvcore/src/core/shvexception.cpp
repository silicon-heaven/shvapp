#include "shvexception.h"

#include <iostream>

bool shv::core::Exception::s_abortOnException = false;

shv::core::Exception::Exception(const std::string &_msg, const std::string &_where)
	: m_msg(_msg)
	, m_where(_where)
{
	std::cerr << " SHV_EXCEPTION: " << where() << message() << std::endl;
	if(isAbortOnException())
		abort();
}

const char *shv::core::Exception::what() const noexcept
{
	return m_msg.c_str();
}
