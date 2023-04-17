{
  description = "Silicon Heaven Application's Flake";

  outputs = {
    self,
    flake-utils,
    nixpkgs,
  }:
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
            src = ./.;
            outputs = ["out" "dev"];
            buildInputs = [
              wrapQtAppsHook
              qtbase
              qtmqtt
              qtserialport
              qtquick3d
              qtsvg
              libxkbcommon
              doctest
            ];
            nativeBuildInputs = [
              cmake
            ];
          };
          default = shvapp;

          qtmqtt = qtModule rec {
            pname = "qtmqtt";
            inherit (qtbase) version;
            src = fetchurl {
              url = "https://github.com/qt/qtmqtt/archive/refs/tags/v${version}.tar.gz";
              sha256 = "P2CpIHVx0ya4eJf/UYFEitJEdCa/Qn7PD1CUCmY4TrE=";
              name = "v${version}.tar.gz";
            };
            qtInputs = [qtbase];
            nativeBuildInputs = [pkg-config];
          };
        };
    in
      {
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

        checks.default = pkgsSelf.default;

        formatter = pkgs.alejandra;
      });
}
