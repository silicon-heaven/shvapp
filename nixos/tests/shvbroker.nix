{
  pkgs,
  nixosModules,
  ...
}: {
  nodes = {
    shvbroker = {
      imports = [nixosModules.default];
      services.shvbroker = {
        enable = true;
        openFirewall = true;
        roles.admin.weight = 100;
        access.admin = [
          {
            method = "";
            pathPattern = "**";
            role = "su";
          }
        ];
        users.admin = {
          password = {
            format = "PLAIN";
            password = "admin!123";
          };
          roles = ["admin"];
        };
      };
    };
    client = {
      imports = [nixosModules.default];
      environment.systemPackages = [pkgs.shvapp];
    };
  };

  testScript = ''
    start_all()
    shvbroker.wait_for_open_port(3755)

    with subtest("List top level nodes"):
      res = client.succeed(
        "shvcall --server-host shvbroker"
        + " --user admin"
        + " --password 'admin!123'"
        + " --method ls"
      )
      assert '[".broker"]' in res
  '';
}
