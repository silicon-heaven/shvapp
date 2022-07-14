#include <shv/chainpack/rpcvalue.h>

namespace mock_sites {
// TODO: Name this properly
const auto fin_slave_broker_sites = shv::chainpack::RpcValue::fromCpon(R"(
{
  "_meta":{
    "HP3":{}
  },
  "eyas":{
    "_meta":{
      "FL":{"hiddenNode":true}
    },
    "opc":{
      "_meta":{
        "HP":{},
        "name":"Raide Jokeri depot shvgate",
        "type":"DepotG2"
      }
    }
  }
}
)");

const auto fin_master_broker_sites = shv::chainpack::RpcValue::fromCpon(R"(
{
  "_meta":{
    "HP3":{}
  },
  "fin":{
    "_meta":{"name":"Finland", "name_cz":"Finsko"},
    "hel":{
      "_meta":{"name":"Helsinki", "name_cz":"Helsinki"},
      "tram":{
        "hel002":{
          "_meta":{
            "HP3":{}
          },
          "eyas":{
            "_meta":{
              "FL":{"hiddenNode":true}
            },
            "opc":{
              "_meta":{
                "HP":{},
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
}
