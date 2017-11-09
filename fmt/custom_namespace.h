/*
Formatting library for C++

Copyright (c) 2012 - 2016, Victor Zverovich
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FMT_CUSTOM_NAMESPACE_H_
#define FMT_CUSTOM_NAMESPACE_H_

/*
This file adds support for using fmt library defining a custom namespace in your code
to avoid conflicting with other fmt library versions or 'encapsulate' the implementation
you are using inside another namespace.

Those four macros below provide this feature:

- FMT_CUSTOM_NAMESPACE - 1 if you want a custom namespace prefix, 0 otherwise
- FMT_CUSTOM_NAMESPACE_USING_NAMESPACE - the 'using namespace CUSTOM_NAMESPACE;' used into fmt source code, and
MUST terminate with a semicolon. It also could be used by your application if needed (but it's not recommended);
- FMT_CUSTOM_NAMESPACE_PREFIX - the 'CUSTOM_NAMESPACE' namespace prefix for the encapsulation namespace;
- FMT_CUSTOM_NAMESPACE_BEGIN - the 'namespace CUSTOM_NAMESPACE {' namespace begin for the encapsulation namespace;
- FMT_CUSTOM_NAMESPACE_END - the '} // namespace CUSTOM_NAMESPACE' namespace end for the encapsulation namespace;

Please note that you could use more than one namespace nested, as the example below:

#define FMT_CUSTOM_NAMESPACE                      1
#define FMT_CUSTOM_NAMESPACE_USING_NAMESPACE      using namespace my_cpp_library::formatting_libraries;
#define FMT_CUSTOM_NAMESPACE_PREFIX               my_cpp_library::formatting_libraries
#define FMT_CUSTOM_NAMESPACE_BEGIN                namespace my_cpp_library { namespace formatting_libraries {
#define FMT_CUSTOM_NAMESPACE_END                  } }

Then, in your code, you use this fmt in following way:

#include <fmt/format.h>
#include <iostream>

template <typename ... args>
std::string format_some_string(char const * mask, args && ... argsv)
{
	my_cpp_library::formatting_libraries::fmt::MemoryWriter writer;

	writer.write(mask, std::forward<args>(argsv) ...);
	
	return writer.str();
}

int main(int argc, char ** argv)
{
	std::cout << format_some_string("{} from somewhere", "Hello") << std::endl;

	return 0;
}

Below, the macro definition you could adjust for your needs, please pay attention to example above to show
how to change the final fmt library namespace.
*/

#define FMT_CUSTOM_NAMESPACE                      0
#define FMT_CUSTOM_NAMESPACE_USING_NAMESPACE      // using namespace my_cpp_library::formatting_libraries;
#define FMT_CUSTOM_NAMESPACE_PREFIX               // my_cpp_library::formatting_libraries
#define FMT_CUSTOM_NAMESPACE_BEGIN                // namespace my_cpp_library { namespace formatting_libraries {
#define FMT_CUSTOM_NAMESPACE_END                  // } }

/*
If you doesn't define FMT_CUSTOM_NAMESPACE to 1, bad things happen, so we disable custom namespaces defined
above, to allow build tests and unit tests pass without any false negatives...
*/

#if (FMT_CUSTOM_NAMESPACE != 1)
# undef  FMT_CUSTOM_NAMESPACE_USING_NAMESPACE
# define FMT_CUSTOM_NAMESPACE_USING_NAMESPACE
# undef  FMT_CUSTOM_NAMESPACE_PREFIX
# define FMT_CUSTOM_NAMESPACE_PREFIX
# undef  FMT_CUSTOM_NAMESPACE_BEGIN
# define FMT_CUSTOM_NAMESPACE_BEGIN
# undef  FMT_CUSTOM_NAMESPACE_END
# define FMT_CUSTOM_NAMESPACE_END
#endif

#endif // FMT_CUSTOM_NAMESPACE_H_
