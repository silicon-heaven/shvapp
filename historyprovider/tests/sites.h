#include <shv/chainpack/rpcvalue.h>

namespace mock_sites {
// TODO: Name this properly
const auto fin_slave_broker = shv::chainpack::RpcValue::fromCpon(R"(
{
  "_meta":{
    "HP3":{"slave": true}
  },
  "eyas":{
    "_meta":{
      "FL":{"hiddenNode":true}
    },
    "opc":{
      "_meta":{
        "HP3":{},
        "name":"Raide Jokeri depot shvgate",
        "type":"DepotG2"
      }
    }
  }
}
)");

const auto fin_master_broker = shv::chainpack::RpcValue::fromCpon(R"(
{
  "_meta":{
    "HP3":{"slave": true}
  },
  "fin":{
    "_meta":{"name":"Finland", "name_cz":"Finsko"},
    "hel":{
      "_meta":{"name":"Helsinki", "name_cz":"Helsinki"},
      "tram":{
        "hel002":{
          "_meta":{
            "HP3":{"slave":true}
          },
          "eyas":{
            "_meta":{
              "FL":{"hiddenNode":true}
            },
            "opc":{
              "_meta":{
                "HP3":{},
                "name":"Raide Jokeri depot shvgate",
                "type":"DepotG2"
              }
            }
          }
        }
      }
    }
  }
}
)");

const auto pushlog_hp = shv::chainpack::RpcValue::fromCpon(R"(
{
  "_meta":{
    "HP3":{"slave": true}
  },
  "pushlog":{
    "_meta":{
      "HP3":{"pushLog": true}
    }
  }
}
)");

const auto master_hp_with_slave_pushlog = shv::chainpack::RpcValue::fromCpon(R"(
{
  "_meta":{
    "HP3":{"slave": true}
  },
  "master": {
    "_meta":{
      "HP3":{"slave": true}
    },
    "pushlog":{
      "_meta":{
        "HP3":{"pushLog": true}
      }
    }
  }
}
)");
}
