# Doxygen extensions

This directory holds the Doxygen-related submodules used by the docs
build:

- **`doxygen-awesome-css/`** — [`jothepro/doxygen-awesome-css`](https://github.com/jothepro/doxygen-awesome-css)
  pinned via `.gitmodules`. Provides the modern theme used by the
  Doxygen output.

The `_config/Doxyfile` references stylesheets / JS files from inside
this directory, so initialise the submodule before generating docs:

```bash
git submodule update --init --recursive
```
