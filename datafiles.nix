{ stdenv, inputs, nix }:

stdenv.mkDerivation {
  name = "data";
  buildInputs = [ nix ];
  unpackPhase = "true";
  installPhase = ''
    mkdir $out
    nix-store --dump ${inputs} > $out/$(basename ${inputs}).nar
    nix-store --dump ${inputs.file} > $out/$(basename ${inputs.file}).nar
  '';
}
