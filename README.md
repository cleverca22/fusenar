example usage:

    nix-build fusenar/root.nix -A guest
    ./result/bin/run-nixos-vm

login as root/root

    start_container
login as root/root again


you are now inside a container with /nix/store mounted via fusenar

if this can be finished, you will be able to p2p share your .nar files with other users, without the cost of storing the packed and unpacked form of every derivation
