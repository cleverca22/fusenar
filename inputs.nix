{ stdenv }:

stdenv.mkDerivation {
  name = "inputs";
  unpackPhase = "true";
  installPhase = ''
    mkdir -pv $out/dir/dir2/dir3/
    echo file > $out/file
    echo file2 > $out/dir/file2
    ln -sv file $out/link
    #ln -sv /dev/null $link
    echo #!/bin/sh > $out/script
    echo 'echo hello world' >> $out/script
    chmod +x $out/script
  '';
  outputHash = "08kh7flwc9r3ihgz288mhyvgl2nlnqjrb3dflx27mm8ffs1bkz06";
  outputHashAlgo = "sha256";
  outputHashMode = "recursive";
}
