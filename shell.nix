{ pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    # nativeBuildInputs is usually what you want -- tools you need to run
    nativeBuildInputs = with pkgs; [
      binutils
      glibc
      llvmPackages_latest.bintools
      llvmPackages_latest.clang
      llvmPackages_latest.libclang
      llvmPackages_latest.libcxx
      llvmPackages_latest.libcxxabi
      llvmPackages_latest.libcxxClang
      llvmPackages_latest.libcxxStdenv
      llvmPackages_latest.libllvm
      llvmPackages_latest.llvm
      llvmPackages_latest.stdenv
    ];
}