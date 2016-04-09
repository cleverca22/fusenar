{ stdenv }:

stdenv.mkDerivation {
  name = "inputs";
  outputs = [ "out" "file" ];
  unpackPhase = "true";
  installPhase = ''
    mkdir -pv $out/dir
    echo file > $out/file
    echo file2 > $out/dir/file2
    ln -sv file $out/link
    echo file3 > $file
    #ln -sv /dev/null $link
  '';
}
