#include "settings.h"

Settings::Settings()
{
}

QString Settings::password()
{
	QString pwd = m_settings.value("password").toString();
	if(pwd.isEmpty())
		pwd = "pmdp321";
	return pwd;
}

void Settings::setPassword(const QString &s)
{
	m_settings.setValue("password", s);
}

QString Settings::predatorShvPath()
{
	QString path = m_settings.value("predatorShvPath").toString();
	if(path.isEmpty())
		path = "shv/cze/plz/pow/jn50/conv";
	return path;
}

void Settings::setPredatorShvPath(const QString &s)
{
	m_settings.setValue("predatorShvPath", s);
}

QString Settings::shvBrokerHost() const
{
	QString s = m_settings.value("shvBrokerHost").toString();
	if(s.isEmpty())
		s = "nirvana.elektroline.cz";
	return s;
}

void Settings::setShvBrokerHost(const QString &s)
{
	m_settings.setValue("shvBrokerHost", s);
}

int Settings::shvBrokerPort() const
{
	int p = m_settings.value("shvBrokerPort").toInt();
	if(p == 0)
		p = 3756;
	return p;
}

void Settings::setShvBrokerPort(int p)
{
	m_settings.setValue("shvBrokerPort", p);
}

QString Settings::shvBrokerUrl() const
{
	auto host = shvBrokerHost();
	if(shvBrokerPort() > 0)
		host = host + ':' + QString::number(shvBrokerPort());
	return host;
}
