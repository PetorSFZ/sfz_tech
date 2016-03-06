/// Header used to temporarily disable all warnings (or at least as many as possible)
///
/// Usage:
/// #include <sfz/PushWarnings.hpp>
/// // Code for which warnings will not be generated
/// #include <sfz/PopWarnings.hpp>

#ifndef SFZ_WARNINGS_PUSHED
#error "Must push warnings before they can be popped."
#endif
#undef SFZ_WARNINGS_PUSHED

#if defined(_MSC_VER)
#pragma warning(pop)

#elif defined(__clang__)
#pragma clang diagnostic pop

#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif