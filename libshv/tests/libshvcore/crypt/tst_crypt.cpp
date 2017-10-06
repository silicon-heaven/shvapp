#include <shv/core/utils/crypt.h>
//#include <cassert>
//#include <string>
//#include <cstdio>
//#include <cstring>
//#include <iostream>
//#include <iomanip>
//#include <sstream>
//#include <list>
//#include <set>
//#include <unordered_map>
//#include <algorithm>
//#include <type_traits>

#include <QtTest/QtTest>
#include <QDebug>

using std::string;

// Check that ChainPack has the properties we want.
//#define CHECK_TRAIT(x) static_assert(std::x::value, #x)
//CHECK_TRAIT(is_nothrow_constructible<RpcValue>);
//CHECK_TRAIT(is_nothrow_default_constructible<RpcValue>);
//CHECK_TRAIT(is_copy_constructible<RpcValue>);
//CHECK_TRAIT(is_nothrow_move_constructible<RpcValue>);
//CHECK_TRAIT(is_copy_assignable<RpcValue>);
//CHECK_TRAIT(is_nothrow_move_assignable<RpcValue>);
//CHECK_TRAIT(is_nothrow_destructible<RpcValue>);

namespace {
QDebug operator<<(QDebug debug, const std::string &s)
{
	//QDebugStateSaver saver(debug);
	debug << s.c_str();

	return debug;
}
/*
std::string binary_dump(const RpcValue::Blob &out)
{
	std::string ret;
	for (size_t i = 0; i < out.size(); ++i) {
		uint8_t u = out[i];
		//ret += std::to_string(u);
		if(i > 0)
			ret += '|';
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			ret += (u & (((uint8_t)128) >> j))? '1': '0';
		}
	}
	return ret;
}
*/
}

class TestCrypt: public QObject
{
	Q_OBJECT
private:
	void cryptTest()
	{
		qDebug() << "============= shv crypt test ============\n";
		qDebug() << "------------- crypt";
		{
			shv::core::utils::Crypt crypt;
			std::string s1 = "Hello Dolly";
			std::string s2 = crypt.encrypt(s1);
			std::string s3 = crypt.decrypt(s2);
			qDebug() << s1 << "->" << s2 << "->" << s3;
			QCOMPARE(s1, s3);
		}
		{
			shv::core::utils::Crypt crypt;
			std::string s1 = "Using QtTest library 5.9.1, Qt 5.9.1 (x86_64-little_endian-lp64 shared (dynamic) debug build; by Clang 3.5.0 (tags/RELEASE_350/final))";
			std::string s2 = crypt.encrypt(s1);
			s2.insert(10, "\n")
					.insert(20, "\t")
					.insert(30, "\r\n")
					.insert(40, "\r")
					.insert(50, "\n\n")
					.insert(60, "  \t\t  ");
			std::string s3 = crypt.decrypt(s2);
			qDebug() << s1 << "->" << s2 << "->" << s3;
			QCOMPARE(s1, s3);
		}
		{
			shv::core::utils::Crypt crypt;
			std::string s1 = "H";
			std::string s2 = crypt.encrypt(s1);
			std::string s3 = crypt.decrypt(s2);
			qDebug() << s1 << "->" << s2 << "->" << s3;
			QCOMPARE(s1, s3);
		}
		{
			shv::core::utils::Crypt crypt;
			std::string s1 = "";
			std::string s2 = crypt.encrypt(s1);
			std::string s3 = crypt.decrypt(s2);
			qDebug() << s1 << "->" << s2 << "->" << s3;
			QCOMPARE(s1, s3);
		}
	}
private slots:
	void initTestCase()
	{
		//qDebug("called before everything else");
	}
	void tests()
	{
		cryptTest();
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestCrypt)
#include "tst_crypt.moc"
