#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

class QSettings;

class Settings
{
public:
	Settings(QSettings &settings);

	QString bfsShvPath();
	void setBfsShvPath(const QString &s);
	QString shvBrokerUser();
	void setShvBrokerUser(const QString &s);
	QString shvBrokerHost() const;
	void setShvBrokerHost(const QString &s);
	int shvBrokerPort() const;
	void setShvBrokerPort(int p);

	QString powerFileName();
	void setPowerFileName(const QString &s);
	int powerFileCheckInterval();
	void setCheckPowerFileInterval(int sec);

	QString shvBrokerUrl() const;
private:
	QSettings &m_settings;
};

#endif // SETTINGS_H
