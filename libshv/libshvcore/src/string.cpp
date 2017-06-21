#include "string.h"

#include <algorithm>

namespace shv {
namespace core {

const char* String::WhiteSpaceChars = " \t\n\r\f\v";

std::string::size_type String::indexOf(const std::string & str_haystack, const std::string &str_needle, String::CaseSensitivity case_sensitivity)
{
	auto it = std::search(
				  str_haystack.begin(), str_haystack.end(),
				  str_needle.begin(), str_needle.end(),
				  (case_sensitivity == CaseSensitive) ?
					  [](char a, char b) { return std::tolower(a) == std::tolower(b); }:
					  [](char a, char b) { return a == b;}
	);
	return (it == str_haystack.end())? std::string::npos: it - str_haystack.begin();
}

bool String::equal(std::string const& a, std::string const& b, String::CaseSensitivity case_sensitivity)
{
	if (a.length() == b.length()) {
		return std::equal(
					b.begin(), b.end(),
					a.begin(),
					(case_sensitivity == CaseSensitive) ?
						[](char a, char b) { return std::tolower(a) == std::tolower(b); }:
						[](char a, char b) { return a == b;}
		);
	}
	else {
		return false;
	}
}

std::vector<std::string> String::split(const std::string &str, char delim, SplitBehavior split_behavior)
{
	using namespace std;
	vector<string> ret;
	size_t pos = 0;
	while(true) {
		size_t pos2 = str.find_first_of(delim, pos);
		string s = (pos2 == string::npos)? str.substr(pos): str.substr(pos, pos - pos2);
		if(split_behavior == KeepEmptyParts || !s.empty())
			ret.push_back(s);
		if(pos2 == string::npos)
			break;
		pos2 = pos;
	}
	return ret;
}

}}
