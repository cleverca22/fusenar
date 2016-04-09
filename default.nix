{ stdenv, fuse, nix, boost, termcolor }:

stdenv.mkDerivation {
  name = "fusenar";
  src = ./.;
  buildInputs = [ fuse nix boost termcolor ];
}
