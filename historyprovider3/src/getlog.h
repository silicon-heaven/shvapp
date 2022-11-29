#pragma once
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvjournalfilereader.h>


namespace shv::core::utils {
[[nodiscard]] chainpack::RpcValue get_log(const std::vector<std::function<ShvJournalFileReader()>>& readers, const ShvGetLogParams &params);
}


