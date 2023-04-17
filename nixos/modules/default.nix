overlay: let
  modules = {
    shvbroker = import ./shvbroker.nix;
  };
in
  modules
  // {
    default = {
      imports = builtins.attrValues modules;
      config.nixpkgs.overlays = [overlay];
    };
  }
