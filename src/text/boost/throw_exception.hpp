#ifndef TEXT_BOOST_THROW_EXCEPTION_HPP
#define TEXT_BOOST_THROW_EXCEPTION_HPP

namespace boost {
template <typename E>
void throw_exception(const E& e) { throw e; }
}

#endif  // TEXT_BOOST_THROW_EXCEPTION_HPP
