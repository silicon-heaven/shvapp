{
  description = "Silicon Heaven Application's Flake";

  outputs = {
    self,
    flake-utils,
    nixpkgs,
  }:
    with builtins;
    with flake-utils.lib;
    with nixpkgs.lib; let
      packages = pkgs:
        with pkgs;
        with qt6Packages; rec {
          shvapp = stdenv.mkDerivation {
            name = "shvapp";
            src = builtins.path {
              name = "shvapp-src";
              path = ./.;
              filter = path: type: ! hasSuffix ".nix" path;
            };
            outputs = ["out" "dev"];
            buildInputs = [
              wrapQtAppsHook
              qtbase
              qtmqtt
              qtquick3d
              qtserialport
              qtsvg
              qtwebsockets
              libxkbcommon
              doctest
              lua5_3
            ];
            nativeBuildInputs = [
              cmake
            ];
          };
          default = shvapp;
        };
    in
      {
        nixosModules = import ./nixos/modules self.overlays.default;
        overlays = {
          shvapp = final: prev: packages (id prev);
          default = self.overlays.shvapp;
        };
      }
      // eachDefaultSystem (system: let
        pkgs = nixpkgs.legacyPackages.${system}.extend self.overlays.default;
      in {
        packages = filterPackages system rec {
          inherit (pkgs) shvapp;
          default = shvapp;
        };
        legacyPackages = pkgs;

        # NixOS tests work only on Linux and we target Linux only anyway.
        checks =
          optionalAttrs (hasSuffix "-linux" system)
          (import ./nixos/tests
            nixpkgs.lib
            self.legacyPackages.${system}
            self.nixosModules)
          // {inherit (self.packages.${system}) default;};

        formatter = pkgs.alejandra;
      });
}
