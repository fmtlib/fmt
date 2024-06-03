---
hide:
  - navigation
  - toc
---

# A modern formatting library

<div class="features-container">

<div class="feature">
<h2>Safety</h2>
<p>
  Inspired by the Python's formatting facility, {fmt} provides a safe
  replacement for the <code>printf</code> family of functions. Errors in format
  strings, which are a common source of vulnerabilities in C, are <b>reported
  at compile time</b>. For example:

  <pre><code class="language-cpp"
  >fmt::format("{:d}", "I am not a number");</code></pre>

  will give a compile-time error because <code>d</code> is not a valid
  format specifier for strings. APIs like <a href="api/#format">
  <code>fmt::format</code></a> <b>prevent buffer overflow errors</b> via
  automatic memory management.
</p>
<a href="api#compile-time-format-string-checks">→ Learn more</a>
</div>

<div class="feature">
<h2>Extensibility</h2>
<p>
  Formatting of most <b>standard types</b> including all containers, dates and
  times is <b>supported out-of-the-box</b>.
  For example:
  
  <pre><code class="language-cpp"
  >fmt::print("{}", std::vector{1, 2, 3});</code></pre>

  prints the vector in a JSON-like format:

  <pre><code>[1, 2, 3]</code></pre>

  You can <b>make your own types formattable</b> and even make compile-time
  checks work for them.
</p>
<a href="api#udt">→ Learn more</a>
</div>

<div class="feature">
<h2>Performance</h2>
<p>
  {fmt} can be anywhere from <b>tens of percent to 20-30 times faster</b> than
  iostreams and <code>sprintf</code>, especially on numeric formatting.

<svg xmlns="http://www.w3.org/2000/svg" version="1.1" viewBox="20 0 550 300" aria-label="A chart." style="overflow: hidden;"><defs id="_ABSTRACT_RENDERER_ID_0"><clipPath id="_ABSTRACT_RENDERER_ID_1"><rect x="120" y="45" width="560" height="210"></rect></clipPath></defs><rect x="0" y="0" width="800" height="300" stroke="none" stroke-width="0" fill="#ffffff"></rect><g><text text-anchor="start" x="120" y="27.05" font-family="Arial" font-size="13" font-weight="bold" stroke="none" stroke-width="0" fill="#000000">double to string</text><rect x="120" y="16" width="560" height="13" stroke="none" stroke-width="0" fill-opacity="0" fill="#ffffff"></rect></g><g><rect x="120" y="45" width="560" height="210" stroke="none" stroke-width="0" fill-opacity="0" fill="#ffffff"></rect><g clip-path="url(#_ABSTRACT_RENDERER_ID_1)"><g><rect x="120" y="45" width="1" height="210" stroke="none" stroke-width="0" fill="#cccccc"></rect><rect x="213" y="45" width="1" height="210" stroke="none" stroke-width="0" fill="#cccccc"></rect><rect x="306" y="45" width="1" height="210" stroke="none" stroke-width="0" fill="#cccccc"></rect><rect x="400" y="45" width="1" height="210" stroke="none" stroke-width="0" fill="#cccccc"></rect><rect x="493" y="45" width="1" height="210" stroke="none" stroke-width="0" fill="#cccccc"></rect><rect x="586" y="45" width="1" height="210" stroke="none" stroke-width="0" fill="#cccccc"></rect><rect x="679" y="45" width="1" height="210" stroke="none" stroke-width="0" fill="#cccccc"></rect></g><g><rect x="121" y="53" width="450" height="26" stroke="#ff9900" stroke-width="1" fill="#ff9900"></rect><rect x="121" y="95" width="421" height="26" stroke="#109618" stroke-width="1" fill="#109618"></rect><rect x="121" y="137" width="341" height="26" stroke="#990099" stroke-width="1" fill="#990099"></rect><rect x="121" y="179" width="31" height="26" stroke="#3366cc" stroke-width="1" fill="#3366cc"></rect><rect x="121" y="221" width="15" height="26" stroke="#dc3912" stroke-width="1" fill="#dc3912"></rect></g><g><rect x="120" y="45" width="1" height="210" stroke="none" stroke-width="0" fill="#333333"></rect></g></g><g></g><g><g><text text-anchor="middle" x="120.5" y="272.3833333333333" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#444444">0</text></g><g><text text-anchor="middle" x="213.6667" y="272.3833333333333" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#444444">250</text></g><g><text text-anchor="middle" x="306.8333" y="272.3833333333333" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#444444">500</text></g><g><text text-anchor="middle" x="400" y="272.3833333333333" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#444444">750</text></g><g><text text-anchor="middle" x="493.1667" y="272.3833333333333" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#444444">1,000</text></g><g><text text-anchor="middle" x="586.3333" y="272.3833333333333" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#444444">1,250</text></g><g><text text-anchor="middle" x="679.5" y="272.3833333333333" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#444444">1,500</text></g><g><text text-anchor="end" x="107" y="70.95" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#222222">ostringstream</text></g><g><text text-anchor="end" x="107" y="112.74999999999999" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#222222">ostrstream</text></g><g><text text-anchor="end" x="107" y="154.55" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#222222">sprintf</text></g><g><text text-anchor="end" x="107" y="196.35" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#222222">doubleconv</text></g><g><text text-anchor="end" x="107" y="238.15" font-family="Arial" font-size="13" stroke="none" stroke-width="0" fill="#222222">fmt</text></g></g></g><g><g><text text-anchor="middle" x="300" y="291.71666666666664" font-family="Arial" font-size="13" font-style="italic" stroke="none" stroke-width="0" fill="#222222">Time (ns), smaller is better</text><rect x="120" y="280.66666666666663" width="560" height="13" stroke="none" stroke-width="0" fill-opacity="0" fill="#ffffff"></rect></g></g><g></g></svg>

The library <b>minimizes dynamic memory allocations</b> and can optionally
<a href="api#compile-api">compile format strings</a> to optimal code.
</p>
</div>

<div class="feature">
<h2>Unicode support</h2>
<p>
  {fmt} provides <b>portable Unicode support</b> on major operating systems
  with UTF-8 and <code>char</code> strings. For example:

  <pre><code class="language-cpp"
  >fmt::print("Слава Україні!");</code></pre>

  will be printed correctly on Linux, macOS and even Windows console regardless
  of the codepages.
</p>
<p>
  The default is <b>locale-independent</b> but you can opt into localized
  formatting and {fmt} makes it work with Unicode, working around problems in
  the standard libary.
</p>
</div>

<div class="feature">
<h2>Fast compilation</h2>
<p>
  The library makes extensive use of <b>type erasure</b> to achieve fast
  compilation. <code>fmt/base.h</code> provides a subset
  of the API with <b>minimal include dependencies</b> and enough functionality
  to replace all uses of <code>*printf</code>.
</p>
<p>
  Code using {fmt} is usually several times faster to compile than the
  equivalent iostreams code and while <code>printf</code> compiles faster still,
  the gap is narrowing.
</p>
<a href="https://github.com/fmtlib/fmt?tab=readme-ov-file#compile-time-and-code-bloat">
→ Learn more</a>
</div>

<div class="feature">
<h2>Small binary footprint</h2>
<p>
  Type erasure is also used to prevent template bloat resulting in <b>compact
  per-call binary code</b>. For example, a call to <code>fmt::print</code> with
  a single argument is less than <a href="https://godbolt.org/g/TZU4KF">ten
  x86-64 instructions</a>, comparable to <code>printf</code> despite adding
  runtime safety and much smaller than the equivalent iostreams code.
</p>
<p>
  The library itself has small binary footprint and some components such as
  floating-point formatting can be disabled to make it even smaller for
  resource constrained devices.
</p>
</div>

<div class="feature">
<h2>Portability</h2>
<p>
{fmt} has a <b>small self-contained codebase</b> with the core consisting of
just three header files and no external dependencies.
</p>
<p>
The library is highly portable and requires only on a minimal <b>subset of
C++11</b> features which are available in GCC 4.8, Clang 3.4, MSVC 19.0 (2015)
and later. Newer compiler and standard library features are used if available
and enable additional functionality.
</p>
<p>
Where possible, the output of formatting functions is <b>consistent across
platforms</b>.
</p>
</p>
</div>

<div class="feature">
<h2>Open source</h2>
<p>
  {fmt} is in top hundred open-source libraries on GitHub and has <b>hundreds of
  all-time contributors</b>.
</p>
<p>
  Permissive MIT <a href="https://github.com/fmtlib/fmt#license">license</a>
  allows using the library both in open-source and commercial projects.
</p>
</div>

</div>
