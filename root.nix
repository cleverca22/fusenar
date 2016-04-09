let
  pkgs = import <nixpkgs> {};
  callPackage = pkgs.newScope self;
  self = {
    termcolor = callPackage ./termcolor.nix {};
    fusenar = callPackage ./default.nix {};
    inputs = callPackage ./inputs.nix {};
    data_files = callPackage ./datafiles.nix {};
    do_test = callPackage ./test.nix {};
  };
in self
