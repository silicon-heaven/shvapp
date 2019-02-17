#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

class QSettings;

class Settings
{
public:
	Settings(QSettings &settings);

	//QString powerSwitchName();
	//void setPowerSwitchName(const QString &s);
	//QString powerFileName();
	//void setPowerFileName(const QString &s);
	//int powerFileCheckInterval();
	//void setCheckPowerFileInterval(int sec);
private:
	QSettings &m_settings;
};

#endif // SETTINGS_H
