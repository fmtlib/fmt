---
title: Change Log
copyright: 2026 Terry Golubiewski, all rights reserved.
---

# Change Log

## Unreleased

### 🚀 Features

- *(color)* [**breaking**] Extend colors, text styles, and structured styled text — `fmt::color` becomes a namespace of RGB constants.
Emphasis composition returns emphasis. Terminal colors use palette indices
instead of SGR codes, and color getters return `color_t`. Overlapping terminal
colors compose through RGB instead of throwing. Packed color/style
representations and formatting-context layouts change. Styled replacement
fields inherit and restore enclosing styles rather than unconditionally
resetting to terminal defaults.

