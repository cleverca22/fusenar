{ stdenv, data_files, fusenar, strace }:

stdenv.mkDerivation {
  name = "fusenar-test";
  buildInputs = [ fusenar strace ];
  unpackPhase = "true";
  installPhase = ''
    function finish {
      kill $fusenarpid
      #ls -ltrh cache_dir
      exit 1
    }
    trap finish EXIT
    mkdir $out
    pwd
    mkdir cache_dir
    mkdir mount_point
    fusenar -f cache_dir ${data_files} mount_point &
    fusenarpid=$!
    sleep 5
    echo started fusenar pid $fusenarpid

    #echo listing
    #ls -ltrh mount_point
    #echo listed
    #find mount_point -type f -print0 | xargs -0 strace -e open md5sum > $out/hashes

    md5sum --check ${./hashes}

    #stat mount_point/1cxxc7fzlnqk7h6pi0rm3g84jp5ih019-inputs-file

    kill $fusenarpid

    trap - EXIT
  '';
}
