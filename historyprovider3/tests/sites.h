#include "utils.h"
#include <shv/chainpack/rpcvalue.h>

namespace mock_sites {
// TODO: Name this properly
const auto fin_slave_broker = R"(
{
  "_meta":{
    "HP3":{"type": "HP3"}
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
)"_cpon;

const auto fin_master_broker = R"(
{
  "_meta":{
    "HP3":{"type": "HP3"}
  },
  "fin":{
    "_meta":{"name":"Finland", "name_cz":"Finsko"},
    "hel":{
      "_meta":{"name":"Helsinki", "name_cz":"Helsinki"},
      "tram":{
        "hel002":{
          "_meta":{
            "HP3":{"type":"HP3"}
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
)"_cpon;

const auto pushlog_hp = R"(
{
  "_meta":{
    "HP3":{"type": "HP3"}
  },
  "pushlog":{
    "_meta":{
      "HP3":{"pushLog": true}
    }
  }
}
)"_cpon;

const auto master_hp_with_slave_pushlog = R"(
{
  "_meta":{
    "HP3":{"type": "HP3"}
  },
  "master": {
    "_meta":{
      "HP3":{"type": "HP3"}
    },
    "pushlog":{
      "_meta":{
        "HP3":{"pushLog": true}
      }
    }
  }
}
)"_cpon;

const auto legacy_hp = R"(
{
  "_meta":{
    "HP3":{"type": "HP3"}
  },
  "legacy": {
    "_meta":{
      "HP":{}
    },
  }
}
)"_cpon;

const auto two_devices = R"(
{
  "_meta":{
    "HP3":{"type": "HP3"}
  },
  "one": {
    "_meta":{
      "HP3":{}
    },
  },
  "two": {
    "_meta":{
      "HP3":{}
    },
  }
}
)"_cpon;
}
