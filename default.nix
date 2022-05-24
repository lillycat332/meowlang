# { pkgs, lib, stdenv, fetchFromGitHub }:
let
  pkgs = import <nixpkgs> {};
  stdenv = pkgs.llvmPackages_13.stdenv;
in
pkgs.stdenv.mkDerivation {
  name = "meowlang";
  version = "0.1.0";

  src = pkgs.fetchFromGitHub {
    owner = "lillycat332";
    repo = "meowlang";
    rev = "master";
    hash = "sha256-lUG1yufj8OkN/Ycy2h7VoCNgAaTaLGH8sAW7+IGuXiQ=";
  };

  buildInputs = [ pkgs.clang pkgs.llvm pkgs.libllvm pkgs.libcxx pkgs.libcxxabi pkgs.libclang ];

  installPhase = ''
    make
  '';

  meta = {
    description = "A toy programming language";
    homepage = "https://github.com/lillycat332/meowlang";
    license = "BSD";
  };
}