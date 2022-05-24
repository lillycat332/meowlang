{ stdenv, fetchFromGitHub, go }:

stdenv.mkDerivation rec {
  name = "meowlang-${version}";
  version = "0.1.0";

  src = fetchFromGitHub {
    owner = "lillycat332";
    repo = "meowlang";
    rev = "1937e31606e4dc0f7263133334d429f956502276";
    sha256 = "13ch85f1y4j2n4dbc6alsxbxfd6xnidwi2clibssk5srkz3mx794";
  };

  buildInputs = [ clang llvm libllvm libcxx libcxxabi libclang ];

  meta = with stdenv.lib; {
    description = "A toy programming language";
    homepage = "https://github.com/lillycat332/meowlang";
    license = licenses.bsd2;
    platforms = stdenv.lib.platforms.linux ++ stdenv.lib.platforms.darwin;
  };
}