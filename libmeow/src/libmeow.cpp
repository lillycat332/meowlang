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

  // square root
  extern "C" DLLEXPORT double squareroot(double X) { return std::sqrt(X); }

  // raise to power
  extern "C" DLLEXPORT double power(double X, double Y) {
    return std::pow(X, Y);
  }

  // sine
  extern "C" DLLEXPORT double sine(double X) { return std::sin(X); }

  // cosine
  extern "C" DLLEXPORT double cosine(double X) { return std::cos(X); }

  // tangent
  extern "C" DLLEXPORT double tangent(double X) { return std::tan(X); }
} // namespace libmeow