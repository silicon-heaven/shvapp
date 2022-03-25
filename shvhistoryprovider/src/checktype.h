#pragma once

enum class CheckType {
	ReplaceDirtyLog,       //on device apperance we need to remove dirty log as soon as possible, because it is inconsistent
	CheckDirtyLogState,    //periodic check, dirty log is replaced only if it is too big or too old
	TrimDirtyLogOnly,      //only trim dirty log if it is possible
};
