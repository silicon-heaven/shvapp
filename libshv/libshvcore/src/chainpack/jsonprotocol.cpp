#include "jsonprotocol.h"

#include <limits>
#include <cassert>
#include <cmath>

namespace shv {
namespace core {
namespace chainpack {

/* * * * * * * * * * * * * * * * * * * *
 * Parsing
 */

/* esc(c)
 *
 * Format char c suitable for printing in an error message.
 */
static const int max_depth = 200;

static inline std::string esc(char c)
{
	char buf[12];
	if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {
		snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
	}
	else {
		snprintf(buf, sizeof buf, "(%d)", c);
	}
	return std::string(buf);
}

static inline bool in_range(long x, long lower, long upper)
{
	return (x >= lower && x <= upper);
}

JsonProtocol::JsonProtocol(const std::string &str, std::string &err)
	: str(str)
	, err(err)
{
}

RpcValue JsonProtocol::parseJson(const std::string &in, std::string &err)
{
	JsonProtocol parser{in, err};
	RpcValue result = parser.parse_json(0);

	// Check for any trailing garbage
	parser.consume_garbage();
	if (parser.failed)
		return RpcValue();
	if (parser.i != in.size())
		return parser.fail(std::string("unexpected trailing ") + (in[parser.i]));

	return result;
}

/* fail(msg, err_ret = ChainPack())
		 *
		 * Mark this parse as failed.
		 */
RpcValue JsonProtocol::fail(std::string &&msg) {
	return fail(std::move(msg), RpcValue());
}

/* consume_whitespace()
		 *
		 * Advance until the current character is non-whitespace.
		 */
void JsonProtocol::consume_whitespace() {
	while (str[i] == ' ' || str[i] == '\r' || str[i] == '\n' || str[i] == '\t')
		i++;
}

/* consume_comment()
		 *
		 * Advance comments (c-style inline and multiline).
		 */
bool JsonProtocol::consume_comment() {
	bool comment_found = false;
	if (str[i] == '/') {
		i++;
		if (i == str.size())
			return fail("unexpected end of input after start of comment", false);
		if (str[i] == '/') { // inline comment
			i++;
			// advance until next line, or end of input
			while (i < str.size() && str[i] != '\n') {
				i++;
			}
			comment_found = true;
		}
		else if (str[i] == '*') { // multiline comment
			i++;
			if (i > str.size()-2)
				return fail("unexpected end of input inside multi-line comment", false);
			// advance until closing tokens
			while (!(str[i] == '*' && str[i+1] == '/')) {
				i++;
				if (i > str.size()-2)
					return fail(
								"unexpected end of input inside multi-line comment", false);
			}
			i += 2;
			comment_found = true;
		}
		else
			return fail("malformed comment", false);
	}
	return comment_found;
}

/* consume_garbage()
		 *
		 * Advance until the current character is non-whitespace and non-comment.
		 */
void JsonProtocol::consume_garbage() {
	consume_whitespace();
	if(strategy == Strategy::Comments) {
		bool comment_found = false;
		do {
			comment_found = consume_comment();
			if (failed)
				return;
			consume_whitespace();
		}
		while(comment_found);
	}
}

/* get_next_token()
		 *
		 * Return the next non-whitespace character. If the end of the input is reached,
		 * flag an error and return 0.
		 */
char JsonProtocol::get_next_token() {
	consume_garbage();
	if (failed) return (char)0;
	if (i == str.size())
		return fail("unexpected end of input", (char)0);

	return str[i++];
}

/* encode_utf8(pt, out)
		 *
		 * Encode pt as UTF-8 and add it to out.
		 */
void JsonProtocol::encode_utf8(long pt, std::string & out) {
	if (pt < 0)
		return;

	if (pt < 0x80) {
		out += static_cast<char>(pt);
	} else if (pt < 0x800) {
		out += static_cast<char>((pt >> 6) | 0xC0);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	} else if (pt < 0x10000) {
		out += static_cast<char>((pt >> 12) | 0xE0);
		out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	} else {
		out += static_cast<char>((pt >> 18) | 0xF0);
		out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
		out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	}
}

/* parse_string()
		 *
		 * Parse a string, starting at the current position.
		 */
std::string JsonProtocol::parse_string() {
	std::string out;
	long last_escaped_codepoint = -1;
	while (true) {
		if (i == str.size())
			return fail("unexpected end of input in string", "");

		char ch = str[i++];

		if (ch == '"') {
			encode_utf8(last_escaped_codepoint, out);
			return out;
		}

		if (in_range(ch, 0, 0x1f))
			return fail("unescaped " + esc(ch) + " in string", "");

		// The usual case: non-escaped characters
		if (ch != '\\') {
			encode_utf8(last_escaped_codepoint, out);
			last_escaped_codepoint = -1;
			out += ch;
			continue;
		}

		// Handle escapes
		if (i == str.size())
			return fail("unexpected end of input in string", "");

		ch = str[i++];

		if (ch == 'u') {
			// Extract 4-byte escape sequence
			std::string esc = str.substr(i, 4);
			// Explicitly check length of the substring. The following loop
			// relies on std::string returning the terminating NUL when
			// accessing str[length]. Checking here reduces brittleness.
			if (esc.length() < 4) {
				return fail("bad \\u escape: " + esc, "");
			}
			for (size_t j = 0; j < 4; j++) {
				if (!in_range(esc[j], 'a', 'f') && !in_range(esc[j], 'A', 'F')
					&& !in_range(esc[j], '0', '9'))
					return fail("bad \\u escape: " + esc, "");
			}

			long codepoint = strtol(esc.data(), nullptr, 16);

			// JSON specifies that characters outside the BMP shall be encoded as a pair
			// of 4-hex-digit \u escapes encoding their surrogate pair components. Check
			// whether we're in the middle of such a beast: the previous codepoint was an
			// escaped lead (high) surrogate, and this is a trail (low) surrogate.
			if (in_range(last_escaped_codepoint, 0xD800, 0xDBFF)
				&& in_range(codepoint, 0xDC00, 0xDFFF)) {
				// Reassemble the two surrogate pairs into one astral-plane character, per
				// the UTF-16 algorithm.
				encode_utf8((((last_escaped_codepoint - 0xD800) << 10)
							 | (codepoint - 0xDC00)) + 0x10000, out);
				last_escaped_codepoint = -1;
			} else {
				encode_utf8(last_escaped_codepoint, out);
				last_escaped_codepoint = codepoint;
			}

			i += 4;
			continue;
		}

		encode_utf8(last_escaped_codepoint, out);
		last_escaped_codepoint = -1;

		if (ch == 'b') {
			out += '\b';
		} else if (ch == 'f') {
			out += '\f';
		} else if (ch == 'n') {
			out += '\n';
		} else if (ch == 'r') {
			out += '\r';
		} else if (ch == 't') {
			out += '\t';
		} else if (ch == '"' || ch == '\\' || ch == '/') {
			out += ch;
		} else {
			return fail("invalid escape character " + esc(ch), "");
		}
	}
}

/* parse_number()
		 *
		 * Parse a double.
		 */
RpcValue JsonProtocol::parse_number() {
	size_t start_pos = i;

	if (str[i] == '-')
		i++;

	// Integer part
	if (str[i] == '0') {
		i++;
		if (in_range(str[i], '0', '9'))
			return fail("leading 0s not permitted in numbers");
	} else if (in_range(str[i], '1', '9')) {
		i++;
		while (in_range(str[i], '0', '9'))
			i++;
	} else {
		return fail("invalid " + esc(str[i]) + " in number");
	}

	if (str[i] != '.' && str[i] != 'e' && str[i] != 'E'
		&& (i - start_pos) <= static_cast<size_t>(std::numeric_limits<int>::digits10)) {
		return std::atoi(str.c_str() + start_pos);
	}

	// Decimal part
	if (str[i] == '.') {
		i++;
		if (!in_range(str[i], '0', '9'))
			return fail("at least one digit required in fractional part");

		while (in_range(str[i], '0', '9'))
			i++;
	}

	// Exponent part
	if (str[i] == 'e' || str[i] == 'E') {
		i++;

		if (str[i] == '+' || str[i] == '-')
			i++;

		if (!in_range(str[i], '0', '9'))
			return fail("at least one digit required in exponent");

		while (in_range(str[i], '0', '9'))
			i++;
	}

	return std::strtod(str.c_str() + start_pos, nullptr);
}

/* expect(str, res)
		 *
		 * Expect that 'str' starts at the character that was just read. If it does, advance
		 * the input and return res. If not, flag an error.
		 */
RpcValue JsonProtocol::expect(const std::string &expected, RpcValue res)
{
	assert(i != 0);
	i--;
	if (str.compare(i, expected.length(), expected) == 0) {
		i += expected.length();
		return res;
	} else {
		return fail("parse error: expected " + expected + ", got " + str.substr(i, expected.length()));
	}
}

/* parse_json()
		 *
		 * Parse a JSON object.
		 */
RpcValue JsonProtocol::parse_json(int depth)
{
	if (depth > max_depth) {
		return fail("exceeded maximum nesting depth");
	}

	char ch = get_next_token();
	if (failed)
		return RpcValue();

	if (ch == '-' || (ch >= '0' && ch <= '9')) {
		i--;
		return parse_number();
	}

	if (ch == 't')
		return expect("true", true);

	if (ch == 'f')
		return expect("false", false);

	if (ch == 'n')
		return expect("null", RpcValue(nullptr));

	if (ch == '"')
		return parse_string();

	if (ch == '{') {
		std::map<std::string, RpcValue> data;
		ch = get_next_token();
		if (ch == '}')
			return data;

		while (1) {
			if (ch != '"')
				return fail("expected '\"' in object, got " + esc(ch));

			std::string key = parse_string();
			if (failed)
				return RpcValue();

			ch = get_next_token();
			if (ch != ':')
				return fail("expected ':' in object, got " + esc(ch));

			data[std::move(key)] = parse_json(depth + 1);
			if (failed)
				return RpcValue();

			ch = get_next_token();
			if (ch == '}')
				break;
			if (ch != ',')
				return fail("expected ',' in object, got " + esc(ch));

			ch = get_next_token();
		}
		return data;
	}

	if (ch == '[') {
		std::vector<RpcValue> data;
		ch = get_next_token();
		if (ch == ']')
			return data;

		while (1) {
			i--;
			data.push_back(parse_json(depth + 1));
			if (failed)
				return RpcValue();

			ch = get_next_token();
			if (ch == ']')
				break;
			if (ch != ',')
				return fail("expected ',' in list, got " + esc(ch));

			ch = get_next_token();
			(void)ch;
		}
		return data;
	}

	return fail("expected value, got " + esc(ch));
}

/* * * * * * * * * * * * * * * * * * * *
 * Serialization
 */

void JsonProtocol::dumpJson(std::nullptr_t, std::string &out)
{
	out += "null";
}

void JsonProtocol::dumpJson(double value, std::string &out)
{
	if (std::isfinite(value)) {
		char buf[32];
		snprintf(buf, sizeof buf, "%.17g", value);
		out += buf;
	}
	else {
		out += "null";
	}
}

void JsonProtocol::dumpJson(RpcValue::Int value, std::string &out)
{
	out += std::to_string(value);
}

void JsonProtocol::dumpJson(RpcValue::UInt value, std::string &out)
{
	out += std::to_string(value);
}

void JsonProtocol::dumpJson(bool value, std::string &out)
{
	out += value ? "true" : "false";
}

void JsonProtocol::dumpJson(RpcValue::DateTime value, std::string &out)
{
	out += value.toString();
}

void JsonProtocol::dumpJson(const std::string &value, std::string &out)
{
	out += '"';
	for (size_t i = 0; i < value.length(); i++) {
		const char ch = value[i];
		if (ch == '\\') {
			out += "\\\\";
		} else if (ch == '"') {
			out += "\\\"";
		} else if (ch == '\b') {
			out += "\\b";
		} else if (ch == '\f') {
			out += "\\f";
		} else if (ch == '\n') {
			out += "\\n";
		} else if (ch == '\r') {
			out += "\\r";
		} else if (ch == '\t') {
			out += "\\t";
		} else if (static_cast<uint8_t>(ch) <= 0x1f) {
			char buf[8];
			snprintf(buf, sizeof buf, "\\u%04x", ch);
			out += buf;
		} else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
				 && static_cast<uint8_t>(value[i+2]) == 0xa8) {
			out += "\\u2028";
			i += 2;
		} else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
				 && static_cast<uint8_t>(value[i+2]) == 0xa9) {
			out += "\\u2029";
			i += 2;
		} else {
			out += ch;
		}
	}
	out += '"';
}

void JsonProtocol::dumpJson(const RpcValue::Blob &value, std::string &out)
{
	std::string s = value.toString();
	dumpJson(s, out);
}

void JsonProtocol::dumpJson(const RpcValue::List &values, std::string &out)
{
	bool first = true;
	out += "[";
	for (const auto &value : values) {
		if (!first)
			out += ", ";
		value.dumpJson(out);
		first = false;
	}
	out += "]";
}

void JsonProtocol::dumpJson(const RpcValue::Map &values, std::string &out)
{
	bool first = true;
	out += "{";
	for (const auto &kv : values) {
		if (!first)
			out += ", ";
		dumpJson(kv.first, out);
		out += ": ";
		kv.second.dumpJson(out);
		first = false;
	}
	out += "}";
}

void JsonProtocol::dumpJson(const RpcValue::IMap &values, std::string &out)
{
	bool first = true;
	out += "{";
	for (const auto &kv : values) {
		if (!first)
			out += ", ";
		dumpJson(kv.first, out);
		out += ": ";
		kv.second.dumpJson(out);
		first = false;
	}
	out += "}";
}

}}}
