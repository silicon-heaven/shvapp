#pragma once

#include "../shvcoreglobal.h"

#include <string>
#include <vector>

namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT String
{
public:
	enum CaseSensitivity {CaseSensitive, CaseInsensitive};
	enum SplitBehavior {KeepEmptyParts, SkipEmptyParts};

	static bool equal(const std::string &a, const std::string &b, String::CaseSensitivity case_sensitivity);
	static std::string::size_type indexOf(const std::string & str_haystack, const std::string &str_needle, String::CaseSensitivity case_sensitivity);
	static const char * WhiteSpaceChars;
	static std::string& rtrim(std::string& s, const char* t = WhiteSpaceChars)
	{
		s.erase(s.find_last_not_of(t) + 1);
		return s;
	}
	static std::string& ltrim(std::string& s, const char* t = WhiteSpaceChars)
	{
		s.erase(0, s.find_first_not_of(t));
		return s;
	}
	static std::string& trim(std::string& s, const char* t = WhiteSpaceChars)
	{
		return ltrim(rtrim(s, t), t);
	}
	static std::vector<std::string> split(const std::string &str, char delim = ' ', SplitBehavior split_behavior = SkipEmptyParts);

};

}}
