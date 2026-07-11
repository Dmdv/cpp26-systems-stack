#pragma once

// Folly pulls in fmt, which defines free functions named isfinite/isnan/….
// On Apple platforms <math.h> also exposes those as macros, so include order
// can explode. Neutralize the macros before any Folly/fmt use in this TU.
#include <cmath>
#ifdef isnan
#  undef isnan
#endif
#ifdef isinf
#  undef isinf
#endif
#ifdef isfinite
#  undef isfinite
#endif
#ifdef signbit
#  undef signbit
#endif
#ifdef fpclassify
#  undef fpclassify
#endif
