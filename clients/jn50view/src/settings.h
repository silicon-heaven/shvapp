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
	QString shvBrokerHost() const;
	void setShvBrokerHost(const QString &s);
	int shvBrokerPort() const;
	void setShvBrokerPort(int p);

	QString shvBrokerUrl() const;

private:
	QSettings m_settings;
};

#endif // SETTINGS_H
