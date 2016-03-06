/// Header used to temporarily disable all warnings (or at least as many as possible)
///
/// Usage:
/// #include <sfz/PushWarnings.hpp>
/// // Code for which warnings will not be generated
/// #include <sfz/PopWarnings.hpp>

#define SFZ_WARNINGS_PUSHED

#if defined(_MSC_VER)
#pragma warning(push, 1)

#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#pragma clang diagnostic ignored "-Wpedantic"

#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
