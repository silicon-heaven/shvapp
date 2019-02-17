#include "settings.h"

#include <QSettings>

Settings::Settings(QSettings &settings)
	: m_settings(settings)
{
}

