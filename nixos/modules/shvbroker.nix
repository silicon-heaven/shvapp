{
  config,
  lib,
  pkgs,
  ...
}:
with lib; let
  cnf = config.services.shvbroker;

  format = pkgs.formats.json {};
  cponFile = name: data: {
    inherit name;
    path = format.generate "shvbroker-${name}" (filterAttrsRecursive (n: v: v != null) data);
  };
  confDir = pkgs.linkFarm "shvbroker-confdir" [
    (cponFile "shvbroker.conf" cnf.config)
    (cponFile "access.cpon" cnf.access)
    (cponFile "mounts.cpon" cnf.mounts)
    (cponFile "roles.cpon" cnf.roles)
    (cponFile "users.cpon" cnf.users)
  ];
in {
  options = {
    services.shvbroker = {
      enable = mkEnableOption (mdDoc ''
        Enable Silicon Heaven broker service.
      '');
      config = mkOption {
        type = types.submodule {
          freeformType = format.type;
        };
        default = {server = {port = 3755;};};
        description = mdDoc ''
          Broker configuration.

          Any valid option can be specified. The documented options are here
          only for reference and validation of the common values.
        '';
      };
      access = mkOption {
        type = types.submodule {
          freeformType = format.type;
        };
        default = {};
        description = mdDoc ''
          Access configuration.
        '';
      };
      mounts = mkOption {
        type = types.submodule {
          freeformType = format.type;
        };
        default = {};
        description = mdDoc ''
          Mounts configuration.
        '';
      };
      roles = mkOption {
        type = types.submodule {
          freeformType = format.type;
        };
        default = {};
        description = mdDoc ''
          Roles configuration.
        '';
      };
      users = mkOption {
        type = types.submodule {
          freeformType = format.type;
        };
        default = {};
        description = mdDoc ''
          Users configuration.
        '';
      };
      openFirewall = mkEnableOption (mdDoc ''
        Open the configured port (config.server.port) in Firewall.
      '');
    };
  };

  config = mkIf cnf.enable {
    systemd.services.shvbroker = {
      description = "Silicon Heaven Broker";
      wantedBy = ["multi-user.target"];
      serviceConfig.ExecStart = "${pkgs.shvapp}/bin/shvbroker --config-dir ${confDir}";
    };
    networking.firewall = mkIf cnf.openFirewall {
      allowedTCPPorts = [cnf.config.server.port];
    };
  };
}
