#include "settings.h"

#include <QSettings>

Settings::Settings(QSettings &settings)
	: m_settings(settings)
{
}

QString Settings::powerSwitchName()
{
	return m_settings.value("power/switchName").toString();
}

void Settings::setPowerSwitchName(const QString &s)
{
	m_settings.setValue("power/switchName", s);
}

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
