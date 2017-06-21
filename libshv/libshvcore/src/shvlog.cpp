#include "shvlog.h"
#include "string.h"

#include <map>
#include <cctype>
//#include <algorithm>
#include <iostream>

#ifdef __unix
#include <unistd.h>
#include <cstdio>
#endif

namespace shv {
namespace core {

namespace {

struct LogFilter
{
	ShvLog::Level defaultModulesLogTreshold = ShvLog::Level::Info;
	std::map<std::string, ShvLog::Level> modulesTresholds;
	std::map<std::string, ShvLog::Level> categoriesTresholds;
	ShvLog::Level defaultCategoriesLogTreshold = ShvLog::Level::Invalid;
} s_globalLogFilter;

bool s_logLongFileNames = false;

std::string moduleFromFileName(const char *file_name)
{
	if(s_logLongFileNames)
		return std::string(file_name);
	std::string ret(file_name);
	auto ix = ret.find_last_of('/');
#ifndef __unix
	if(ix == std::string::npos)
		ix = ret.find_last_not_of('\\');
#endif
	if(ix != std::string::npos)
		return ret.substr(ix + 1);
	return ret;
}

/*
Here, \033 is the ESC character, ASCII 27.
It is followed by [,
then zero or more numbers separated by ;,
and finally the letter m.

		 foreground background
black        30         40
red          31         41
green        32         42
yellow       33         43
blue         34         44
magenta      35         45
cyan         36         46
white        37         47

reset             0  (everything back to normal)
bold/bright       1  (often a brighter shade of the same colour)
underline         4
inverse           7  (swap foreground and background colours)
bold/bright off  21
underline off    24
inverse off      27
 */
enum TTYColor {Black=0, Red, Green, Yellow, Blue, Magenta, Cyan, White};

std::ostream & setTtyColor(std::ostream &out, TTYColor color, bool bright, bool bg_color)
{
	out << "\033[" << (bright? '1': '0') << ';' << (bg_color? '4': '3') << char('0' + color) << 'm';
	return out;
}

void default_message_output(ShvLog::Level level, const ShvLog::LogContext &context, const std::string &message)
{
	static int n = 0;
	bool is_tty = false;
#ifdef __unix
	is_tty = ::isatty(STDERR_FILENO);
#endif
	auto set_tty_color = [is_tty](TTYColor color, bool bright = false, bool bg_color = false) -> std::ostream & {
		if(is_tty)
			return setTtyColor(std::cerr, color, bright, bg_color);
		return std::cerr;
	};
	set_tty_color(TTYColor::Green, true) << ++n;
	TTYColor log_color;
	bool stay_bright = false;
	switch(level) {
	case ShvLog::Level::Fatal:
		stay_bright = true; log_color = TTYColor::Red; set_tty_color(log_color, true) << "|F";
		break;
	case ShvLog::Level::Error:
		stay_bright = true; log_color = TTYColor::Red; set_tty_color(log_color, true) << "|E";
		break;
	case ShvLog::Level::Warning:
		stay_bright = true; log_color = TTYColor::Magenta; set_tty_color(log_color, true) << "|W";
		break;
	case ShvLog::Level::Info:
		log_color = TTYColor::Cyan; set_tty_color(log_color, true) << "|I";
		break;
	case ShvLog::Level::Debug:
		log_color = TTYColor::White; set_tty_color(log_color, true) << "|D";
		break;
	default:
		log_color = TTYColor::Yellow; set_tty_color(log_color, true) << "|?";
		break;
	};
	set_tty_color(log_color, true) << '[' << moduleFromFileName(context.file) << ':' << context.line << "] ";
	set_tty_color(log_color, false) << message;
	std::cerr << "\33[0m";
	std::cerr << std::endl;
}

ShvLog::MessageOutput message_output = default_message_output;

std::pair<std::string, ShvLog::Level> parseCategoryLevel(const std::string &category)
{
	auto ix = category.find(':');
	ShvLog::Level level = ShvLog::Level::Debug;
	std::string cat = category;
	if(ix != std::string::npos) {
		std::string s = category.substr(ix + 1, 1);
		char l = s.empty()? '\0': std::toupper(s[0]);
		cat = category.substr(0, ix);
		String::trim(cat);
		if(l == 'D')
			level = ShvLog::Level::Debug;
		else if(l == 'I')
			level = ShvLog::Level::Info;
		else if(l == 'W')
			level = ShvLog::Level::Warning;
		else if(l == 'E')
			level = ShvLog::Level::Error;
		else
			level = ShvLog::Level::Invalid;
	}
	return std::pair<std::string, ShvLog::Level>(cat, level);
}

void setModulesTresholds(const std::vector<std::string> &tresholds)
{
	if(tresholds.empty()) {
		s_globalLogFilter.defaultModulesLogTreshold = ShvLog::Level::Debug;
	}
	else for(std::string module : tresholds) {
		std::pair<std::string, ShvLog::Level> lev = parseCategoryLevel(module);
		if(lev.first.empty())
			s_globalLogFilter.defaultModulesLogTreshold = lev.second;
		else
			s_globalLogFilter.modulesTresholds[lev.first] = lev.second;
	}
}

std::vector<std::string> setModulesTresholdsFromArgs(const std::vector<std::string> &args)
{
	std::vector<std::string> ret;
	s_globalLogFilter.modulesTresholds.clear();
	for(size_t i=0; i<args.size(); i++) {
		const std::string &s = args[i];
		if(s == "-d" || s == "--debug") {
			i++;
			std::string tresholds =  (i < args.size())? args[i]: std::string();
			setModulesTresholds(String::split(tresholds, ','));
		}
		else {
			ret.push_back(s);
		}
	}
	return ret;
}

void setCategoriesTresholds(const std::vector<std::string> &tresholds)
{
	for(std::string module : tresholds) {
		std::pair<std::string, ShvLog::Level> lev = parseCategoryLevel(module);
		if(lev.first.empty())
			s_globalLogFilter.defaultCategoriesLogTreshold = lev.second;
		else
			s_globalLogFilter.categoriesTresholds[lev.first] = lev.second;
	}
}

std::vector<std::string> setCategoriesTresholdsFromArgs(const std::vector<std::string> &args)
{
	using namespace std;
	vector<string> ret;
	s_globalLogFilter.categoriesTresholds.clear();
	std::vector<string> tresholds;
	for(size_t i=0; i<args.size(); i++) {
		const string &s = args[i];
		if(s == "-v" || s == "--verbose") {
			i++;
			if(i < args.size()) {
					setCategoriesTresholds(String::split(args[i], ','));
			}
		}
		else {
			ret.push_back(s);
		}
	}
	setCategoriesTresholds(tresholds);
	return ret;
}

}

ShvLog::~ShvLog()
{
	if (!--stream->ref) {
		std::streambuf *buff = stream->ts.rdbuf();
		std::stringbuf *sbuff = dynamic_cast<std::stringbuf*>(buff);
		if(sbuff) {
			std::string msg = sbuff->str();
			if (stream->space && !msg.empty() && msg.back() == ' ')
				msg.pop_back();
			message_output(stream->level, stream->context, msg);
		}
		delete stream;
	}
}

std::vector<std::string> ShvLog::setGlobalTresholds(int argc, char *argv[])
{
	std::vector<std::string> ret;
	for(int i=1; i<argc; i++) {
		const std::string &s = argv[i];
		if(s == "-lh" || s == "--log-help") {
			i++;
			std::cout << "log options:" << std::endl;
			std::cout << "-lh, --log-help" << std::endl;
			std::cout << "\t" << "Show logging help" << std::endl;
			std::cout << "-lfn, --log-long-file-names" << std::endl;
			std::cout << "\t" << "Log long file names" << std::endl;
			std::cout << "-d, --log-file [<pattern>]:[D|I|W|E]" << std::endl;
			std::cout << "\t" << "Set file log treshold" << std::endl;
			std::cout << "\t" << "set treshold for all files containing pattern to treshold" << std::endl;
			std::cout << "\t" << "when pattern is not set, set treshold for all files" << std::endl;
			std::cout << "\t" << "when treshold is not set, set treshold D (Debug) for all files containing pattern" << std::endl;
			std::cout << "\t" << "when nothing is not set, set treshold D (Debug) for all files" << std::endl;
			std::cout << "\t" << "Examples:" << std::endl;
			std::cout << "\t\t" << "-d" << "\t\t" << "set treshold D (Debug) for all files" << std::endl;
			std::cout << "\t\t" << "-d :W" << "\t\t" << "set treshold W (Warning) for all files" << std::endl;
			std::cout << "\t\t" << "-d foo" << "\t\t" << "set treshold D for all files containing 'foo'" << std::endl;
			std::cout << "\t\t" << "-d bar:W" << "\t" << "set treshold W (Warning) for all files containing 'bar'" << std::endl;
			std::cout << "-v, --log-category [<pattern>]:[D|I|W|E]" << std::endl;
			std::cout << "\t" << "Set category log treshold" << std::endl;
			std::cout << "\t" << "set treshold for all categories containing pattern to treshold" << std::endl;
			std::cout << "\t" << "the same rules as for module logging are applied to categiries" << std::endl;
			exit(0);
		}
		else if(s == "-lfn" || s == "--log-long-file-names") {
			i++;
			s_logLongFileNames = true;
		}
		else {
			ret.push_back(argv[i]);
		}
	}
	ret = setModulesTresholdsFromArgs(ret);
	ret = setCategoriesTresholdsFromArgs(ret);
	ret.insert(ret.begin(), argv[0]);
	return ret;
}

void ShvLog::setMessageOutput(ShvLog::MessageOutput out)
{
	message_output = out;
}

bool ShvLog::isMatchingLogFilter(ShvLog::Level level, const ShvLog::LogContext &log_context)
{
	const LogFilter &log_filter = s_globalLogFilter;
	if(level == ShvLog::Level::Fatal) {
		return true;
	}
	if(log_context.category && log_context.category[0]) {
		// if category is specified, than module logging rules are ignored
		std::string cat(log_context.category);
		auto it = log_filter.categoriesTresholds.find(cat);
		ShvLog::Level category_level = (it == log_filter.categoriesTresholds.end())? log_filter.defaultCategoriesLogTreshold: it->second;

		return (level <= category_level);
	}
	{
		std::string module = moduleFromFileName(log_context.file);
		for (auto& kv : log_filter.modulesTresholds) {
			if(String::indexOf(module, kv.first, String::CaseInsensitive) != std::string::npos)
				return level <= kv.second;
		}
	}
	return level <= log_filter.defaultModulesLogTreshold;
}

}}
