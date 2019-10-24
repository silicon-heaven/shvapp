#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>

class Settings
{
public:
	Settings();

	QString password();
	void setPassword(const QString &s);
	QString predatorShvPath();
	void setPredatorShvPath(const QString &s);
	QString shvBrokerHost();
	void setShvBrokerHost(const QString &s);
	int shvBrokerPort();
	void setShvBrokerPort(int p);
private:
	QSettings m_settings;
};

#endif // SETTINGS_H
