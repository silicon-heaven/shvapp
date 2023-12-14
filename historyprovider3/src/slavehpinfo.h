#pragma once

#include "logtype.h"
#include <QString>
#include <string>

struct SlaveHpInfo {
	LogType log_type;
	std::string shv_path;
	std::string leaf_sync_path;
	QString cache_dir_path;
};
