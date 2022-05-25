//
//  libmeow.cpp
//  meowlang
//
//  Created by Lilly Cham on 24/05/2022.
//

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

///================== //
/// Library functions //
///================== //

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

namespace libmeow {
  /// putchard - putchar that takes a double and returns 0.
  extern "C" DLLEXPORT double putchard(double X) {
    fputc((char)X, stderr);
    return 0;
  }

  /// printd - printf that takes a double prints it as "%f\n", returning 0.
  extern "C" DLLEXPORT double printd(double X) {
    fprintf(stderr, "%f\n", X);
    return 0;
  }

  /// returnd - return a double value
  extern "C" DLLEXPORT double returnd(double X) { return X; }

  // sqrt - square root
  extern "C" DLLEXPORT double sqrt(double X) { return std::sqrt(X); }

  // pow - power
  extern "C" DLLEXPORT double pow(double X, double Y) { return std::pow(X, Y); }

  // sin - sine
  extern "C" DLLEXPORT double sin(double X) { return std::sin(X); }

  // cos - cosine
  extern "C" DLLEXPORT double cos(double X) { return std::cos(X); }

  // tan - tangent
  extern "C" DLLEXPORT double tan(double X) { return std::tan(X); }
} // namespace libmeow