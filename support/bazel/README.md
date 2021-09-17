# Bazel support

Bazel is an open-source build tool.
More information about Bazel can be found [here](https://bazel.build/).

## Using the fmt repository with Bazel

Even though the {fmt} repository does not contain a `WORKSPACE` file in its root directory,
there is an easy approach to use the {fmt} repository with Bazel out of the box.
This is demonstrated in the following example.

Add to your `WORKSPACE` file:

```python
load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")

# Fetch all files from fmt including the BUILD file `support/bazel/BUILD.bazel`
new_git_repository(
    name = "fmt_workaround",
    branch = "master",
    remote = "https://github.com/fmtlib/fmt/",
    build_file_content = "# Empty build file on purpose"
)

# Now the BUILD file `support/bazel/BUILD.bazel` can be used:
new_git_repository(
    name = "fmt",
    branch = "master",
    remote = "https://github.com/fmtlib/fmt/",
    build_file = "@fmt_workaround//:support/bazel/BUILD.bazel"
)
```

Create a `BUILD.bazel` file and add a dependency to {fmt}:

```python
cc_binary( # Build a binary
    name = "Demo", # Name of the binary
    srcs = ["main.cpp"], # List of files - we only have main.cpp
    deps = ["@fmt//:fmt"], # Depend on fmt
)
```

Make use of {fmt} in `main.cpp`:

```C++
#include "fmt/core.h"

int main() {
  fmt::print("The answer is {}.\n", 42);
}
```

The expected output of this example is `The answer is 42`.

## Bazelize fmt

First downloading a build file and then making use of it can be considered as a bit unclean, nevertheless, it works.

A cleaner Bazel solution would be to move the `WORKSPACE` and `BUILD` files to the root folder of the {fmt} Git repository.

In favor of keeping the {fmt} project directory clean, those files were not added to the project root directory. 

If you do not like this, you can fork this repository and move the files `BUILD.bazel`, `WORKSPACE.bazel`, `.bazelrc`, and `.bazelversion` from this folder (`support/bazel`) to the root folder of this project.

This way {fmt} gets bazelized and can be used in your Bazel builds.

**Example**

Create a `WORKSPACE.bazel` file with the following content:

```python
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

# Fetch bazelized fmt
git_repository(
    name = "fmt",
    branch = "bazel-support", # A copy of master where BUILD.bazel, WORKSPACE.bazel, .bazelrc and .bazelversion are moved to root
    remote = "https://github.com/<user_or_organisation>/fmt", # replace <user_or_organisation> by a valid account
)
```

Create a `BUILD.bazel` file and add a dependency to {fmt} (same as above).

Make use of {fmt} in `main.cpp` (same as above).
