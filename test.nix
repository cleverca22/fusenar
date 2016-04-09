{ stdenv, data_files, fusenar, strace, utillinux, valgrind }:

stdenv.mkDerivation {
  name = "fusenar-test";
  buildInputs = [ fusenar strace utillinux valgrind ];
  unpackPhase = "true";
  installPhase = ''
    function finish {
      umount -l mount_point
      kill $fusenarpid
      #ls -ltrh cache_dir
      exit 1
    }
    trap finish EXIT
    mkdir $out
    pwd
    mkdir cache_dir
    mkdir mount_point
    fusenar -s -f cache_dir ${data_files} mount_point &
    fusenarpid=$!
    sleep 2
    echo started fusenar pid $fusenarpid

    #echo listing
    # ls -ltrha mount_point/jkdplzhfbacw7fpa76v4mzh9q0ac4myc-inputs/ mount_point/1cxxc7fzlnqk7h6pi0rm3g84jp5ih019-inputs-file
    #echo listed
    # find mount_point -type f -print0 | xargs -0 md5sum > $out/hashes

    md5sum --check ${./hashes}

    ls -lh mount_point/nl3xpfypdsx36n99vgfa25f682gwqq67-etc/etc/systemd/

    #stat mount_point/1cxxc7fzlnqk7h6pi0rm3g84jp5ih019-inputs-file

    kill $fusenarpid

    trap - EXIT
  '';
}
