#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

class QSettings;

class Settings
{
public:
	Settings(QSettings &settings);

	QString predatorShvPath();
	void setPredatorShvPath(const QString &s);
	QString shvBrokerHost();
	void setShvBrokerHost(const QString &s);
	int shvBrokerPort();
	void setShvBrokerPort(int p);
private:
	QSettings &m_settings;
};

#endif // SETTINGS_H
