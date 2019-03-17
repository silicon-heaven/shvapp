#include "settings.h"

#include <QSettings>

Settings::Settings(QSettings &settings)
	: m_settings(settings)
{
}

QString Settings::bfsShvPath()
{
	QString path = m_settings.value("predatorShvPath").toString();
	if(path.isEmpty())
		path = "../bfs1";
	return path;
}

void Settings::setBfsShvPath(const QString &s)
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

/*
QString Settings::powerSwitchName()
{
	return m_settings.value("power/switchName").toString();
}

void Settings::setPowerSwitchName(const QString &s)
{
	m_settings.setValue("power/switchName", s);
}
*/
QString Settings::powerFileName()
{
	return m_settings.value("power/fileName").toString();
}

void Settings::setPowerFileName(const QString &s)
{
	m_settings.setValue("power/fileName", s);
}

int Settings::powerFileCheckInterval()
{
	return m_settings.value("power/fileCheckInterval").toInt();
}

void Settings::setCheckPowerFileInterval(int sec)
{
	m_settings.setValue("power/fileCheckInterval", sec);
}
