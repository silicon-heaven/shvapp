#pragma once

#include <stdexcept>
#include <string>
/*
#define SHV_EXCEPTION(e) { \
	std::string msg(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " SHV_EXCEPTION: " + e); \
	std::cerr << msg << "\n"; \
	throw std::runtime_error(msg); \
	}
*/
#define SHV_EXCEPTION(e) { \
	throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " SHV_EXCEPTION: " + e); \
	}
