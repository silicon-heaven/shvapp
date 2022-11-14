#include "utils.h"
#include <shv/core/utils/shvjournalfilereader.h>

std::vector<shv::core::utils::ShvJournalEntry> read_entries_from_file(const QString& file_path)
{
	std::vector<shv::core::utils::ShvJournalEntry> res;
	shv::core::utils::ShvJournalFileReader reader(file_path.toStdString());
	while (reader.next()) {
		res.push_back(reader.entry());
	}

	return res;
}
