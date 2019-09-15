#ifndef BOOST_TEXT_CONFIG_HPP
#define BOOST_TEXT_CONFIG_HPP

#include <boost/config.hpp>


/** There are ICU-based implementations of many operations, but those are only
    defined when BOOST_TEXT_HAS_ICU is nonzero.  If you define this, you must
    make sure the the ICU headers are in your path, and that your build
    properly links in ICU. */
#ifndef BOOST_TEXT_HAS_ICU
#    define BOOST_TEXT_HAS_ICU 0
#endif

/** There are ICU-based implementations of many operations, but those are only
    used when BOOST_TEXT_HAS_ICU and BOOST_TEXT_USE_ICU are both nonzero. */
#ifndef BOOST_TEXT_USE_ICU
#    define BOOST_TEXT_USE_ICU 0
#endif

/** When you insert into a rope, the incoming sequence may be inserted as a
    new segment, or if it falls within an existing string-segment, it may be
    inserted into the string object used to represent that segment.  This only
    happens if the incoming sequence will fit within the existing segment's
    capacity, or if the segment is smaller than a certain limit.
    BOOST_TEXT_STRING_INSERT_MAX is that limit. */
#ifndef BOOST_TEXT_STRING_INSERT_MAX
#    define BOOST_TEXT_STRING_INSERT_MAX 4096
#endif

#ifndef BOOST_TEXT_DOXYGEN

// Nothing before GCC 6 has proper C++14 constexpr support.
#if defined(__GNUC__) && __GNUC__ < 6 && !defined(__clang__)
#    define BOOST_TEXT_CXX14_CONSTEXPR
#    define BOOST_TEXT_NO_CXX14_CONSTEXPR
#elif defined(_MSC_VER) && _MSC_VER <= 1915
#    define BOOST_TEXT_CXX14_CONSTEXPR
#    define BOOST_TEXT_NO_CXX14_CONSTEXPR
#else
#    define BOOST_TEXT_CXX14_CONSTEXPR BOOST_CXX14_CONSTEXPR
#    if defined(BOOST_NO_CXX14_CONSTEXPR)
#        define BOOST_TEXT_NO_CXX14_CONSTEXPR
#    endif
#endif

// Implements separate compilation features as described in
// http://www.boost.org/more/separate_compilation.html

//  normalize macros

#if !defined(BOOST_TEXT_DYN_LINK) && !defined(BOOST_TEXT_STATIC_LINK) &&       \
    !defined(BOOST_ALL_DYN_LINK) && !defined(BOOST_ALL_STATIC_LINK)
#    define BOOST_TEXT_STATIC_LINK
#endif

#if defined(BOOST_ALL_DYN_LINK) && !defined(BOOST_TEXT_DYN_LINK)
#    define BOOST_TEXT_DYN_LINK
#elif defined(BOOST_ALL_STATIC_LINK) && !defined(BOOST_TEXT_STATIC_LINK)
#    define BOOST_TEXT_STATIC_LINK
#endif

#if defined(BOOST_TEXT_DYN_LINK) && defined(BOOST_TEXT_STATIC_LINK)
#    error Must not define both BOOST_TEXT_DYN_LINK and BOOST_TEXT_STATIC_LINK
#endif

//  enable dynamic or static linking as requested

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TEXT_DYN_LINK)
#    if defined(BOOST_TEXT_SOURCE)
#        define BOOST_TEXT_DECL BOOST_SYMBOL_EXPORT
#    else
#        define BOOST_TEXT_DECL BOOST_SYMBOL_IMPORT
#    endif
#else
#    define BOOST_TEXT_DECL
#endif

#if 0 // TODO: Disabled for now.
//  enable automatic library variant selection

#if !defined(BOOST_TEXT_SOURCE) && !defined(BOOST_ALL_NO_LIB) &&               \
    !defined(BOOST_TEXT_NO_LIB)
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define BOOST_LIB_NAME boost_text
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_TEXT_DYN_LINK)
#    define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled
#endif

#endif // doxygen

#endif
