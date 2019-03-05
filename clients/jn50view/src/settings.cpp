#include "settings.h"

#include <QSettings>

Settings::Settings(QSettings &settings)
	: m_settings(settings)
{
}

QString Settings::predatorShvPath()
{
	return m_settings.value("predatorShvPath").toString();
}

void Settings::setPredatorShvPath(const QString &s)
{
	m_settings.setValue("predatorShvPath", s);
}

QString Settings::shvBrokerHost()
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

int Settings::shvBrokerPort()
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
