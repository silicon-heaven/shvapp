#pragma once
#include <shv/core/utils/shvjournalentry.h>
#include <QString>
#include <vector>

std::vector<shv::core::utils::ShvJournalEntry> read_entries_from_file(const QString& file_path);
