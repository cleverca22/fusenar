let
  pkgs = import <nixpkgs> {};
  callPackage = pkgs.newScope self;
  self = rec {
    termcolor = callPackage ./termcolor.nix {};
    fusenar = callPackage ./default.nix {};
    inputs = callPackage ./inputs.nix {};
    data_files = callPackage ./datafiles.nix { files = [ inputs inputs.file tiny_container_bare.config.system.build.etc ]; };
    do_test = callPackage ./test.nix {};
    tiny_container_bare = (import <nixpkgs/nixos> {
      configuration = { ... }:
      {
        boot.isContainer = true;
        nix.readOnlyStore = false;
        users.users.root.password = "root";
        networking.hostName = "fuse-container";
      };
    });
    tiny_container = tiny_container_bare.config.system.build.toplevel;
    container_data = callPackage ./datafiles.nix { files = [ tiny_container ]; };
    container = callPackage ./container.nix {};

    guest = (import <nixpkgs/nixos> {
      configuration = { ... }:
      {
        imports = [ <nixpkgs/nixos/modules/virtualisation/qemu-vm.nix> ];
        environment.systemPackages = [ container ];
        users.users.root.password = "root";
        networking.hostName = "qemu-host";
      };
    }).config.system.build.vm;
  };
in self
