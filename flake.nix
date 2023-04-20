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
      packages = {
        system,
        pkgs,
      }:
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
        overlays.default = final: prev:
          packages {
            inherit (prev) system;
            pkgs = prev;
          };
      }
      // eachDefaultSystem (system: let
        pkgs = nixpkgs.legacyPackages.${system};
        pkgsSelf = self.packages.${system};
      in {
        packages = packages {inherit system pkgs;};
        legacyPackages = pkgs.extend self.overlays.default;

        # NixOS tests work only on Linux and we target Linux only anyway.
        checks =
          optionalAttrs (hasSuffix "-linux" system)
          (import ./nixos/tests
            nixpkgs.lib
            self.legacyPackages.${system}
            self.nixosModules)
          // {inherit (pkgsSelf) default;};

        formatter = pkgs.alejandra;
      });
}
