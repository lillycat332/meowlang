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
    rev = "f4832c4d747cc85affe961786137722ce7cbd6e6";
    hash = "1wh2m7m0r4d7jz02dylwm95nskkkpy4pcb8k96b6isr6v2c8znxi";
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