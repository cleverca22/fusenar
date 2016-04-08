{ stdenv, fuse, nix, fetchFromGitHub, flex, bison, boost, termcolor }:

let
  newnix = stdenv.lib.overrideDerivation nix (oldAttrs: {
    src = fetchFromGitHub {
      owner = "NixOS";
      repo = "nix";
      rev = "dc82160164d6c74586b448a13443c19b5a6709c1";
      sha256 = "09zn5sczvwdf59xry24jkjfgz3s437qq8zhx0251qd2nc1gk41v8";
    };
    nativeBuildInputs = oldAttrs.nativeBuildInputs ++ [ flex bison ];
  });
in stdenv.mkDerivation {
  name = "fusenar";
  src = ./.;
  buildInputs = [ fuse nix boost termcolor ];
}
