{ stdenv, fuse, nix, boost, termcolor }:

stdenv.mkDerivation {
  name = "fusenar";
  src = ./.;
  enableParallelBuilding = true;
  dontStrip = true;
  buildInputs = [ fuse nix boost termcolor ];
}
