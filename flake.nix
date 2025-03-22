{
  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSupportedSystem =
        f: nixpkgs.lib.genAttrs supportedSystems (system: f {
          pkgs = import nixpkgs { inherit system; };
        });
    in
    {
      packages = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.stdenv.mkDerivation {
          pname = "tree-but-cooler";
          version = "1.0";
          src = ./.;
          dontConfigure = true;
          nativeBuildInputs = with pkgs; [ gnumake ];
          buildPhase = "make";
          installPhase = ''
            mkdir -p $out/bin
            cp tree_but_cooler $out/bin/
          '';
        };
      });

      apps = forEachSupportedSystem ({ pkgs }: {
        default = {
          type = "app";
          program = "${self.packages.${pkgs.system}.default}/bin/tree_but_cooler";
        };
      });

      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell {
          packages = with pkgs; [ gnumake ];
        };
      });
    };
}
