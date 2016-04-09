{ stdenv, files, nix }:

stdenv.mkDerivation {
  name = "data";
  buildInputs = [ nix ];
  unpackPhase = "true";
  installPhase = ''
    mkdir $out
    export NIX_REMOTE=daemon
    for x in $(nix-store -qR --readonly-mode ${stdenv.lib.concatStringsSep " " files}); do nix-store --dump $x > $out/$(basename $x).nar;done
  '';
}
