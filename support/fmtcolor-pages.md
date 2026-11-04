# Publishing FmtColor documentation

The `FmtColor documentation` workflow builds and publishes the documentation
after a push to `tjgolubi/fmt`'s `color-dev` branch. It does not publish from
feature branches or pull requests. Integrate and review documentation before
pushing the accepted integration; no library integration is performed by this
workflow.

## One-time GitHub setup

In the repository's **Settings → Pages**, select **GitHub Actions** as the
build and deployment source. If Actions is disabled for the fork, enable it in
the repository's Actions tab.

In **Settings → Environments → github-pages**, ensure deployment branch rules
allow `color-dev`. If using a selected-branches policy, add `color-dev`; a rule
allowing only the default `main` branch will block this workflow.

The workflow uses GitHub's built-in token and Pages deployment identity. No
personal access token or VPS credentials are needed.

## Publication

Once the workflow and documentation are integrated and pushed to `color-dev`,
open **Actions → FmtColor documentation** to follow the build and deployment.
The deployment step reports the final URL. Confirm the overview, links, and
color swatches there before adding a link on the project website.

Expected URLs (available only after a successful deployment):

- Documentation: <https://tjgolubi.github.io/fmt/>
- Color overview: <https://tjgolubi.github.io/fmt/color/index.html>

The fork-specific `support/mkdocs-pages.yml` inherits the normal documentation
configuration. It sets the fork URL and repository links and disables the
version selector because this deployment publishes a single version. Generated
HTML is uploaded as a Pages artifact, rather than committed to the source tree.

The upstream `doc.yml` workflow and `support/mkdocs` publication helper retain
their upstream destinations; this workflow uses neither for deployment.

The VPS remains an alternative host. Its deployment can use the same generated
HTML with a configuration for that host's URL.
