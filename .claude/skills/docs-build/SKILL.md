---
name: docs-build
description: Build the pyGPlates Python API documentation (Sphinx, doc-python-api). Use when asked to build, rebuild or check the Python API docs, or to verify a docstring change renders correctly.
---

# Build the pyGPlates API documentation

## Prerequisites

The docs are built by the `doc-python-api` CMake target, so a configured pyGPlates build tree is
required (see `/build-pygplates`). Sphinx and its theme come from an overlay environment file:

```
conda env update -n gplates -f env.docs.yml
```

Sphinx must run in the **same interpreter pyGPlates was built against**, because autodoc imports
the built module in-process. The CMake target handles this by invoking
`${GPLATES_PYTHON_EXECUTABLE} -m sphinx`. Never substitute a `sphinx-build` from `PATH`.

## Build from scratch

**Always build from scratch for anything that will be published.** An incremental rebuild
produces HTML identical to a clean build except for `searchindex.js`, where it silently drops
*every* index entry (`indexentries` goes from hundreds to 0). Nothing warns about this.

So before building, delete the doctree and generated-stub caches. Use `cmake -E rm -rf` rather
than `rm -rf`, which does not exist in the Anaconda Prompt (cmd) or PowerShell that the build
skills direct Windows users to:

```
cmake -E rm -rf <build-dir>/doc-python-api/_doctrees <build-dir>/doc-python-api/generated
cmake --build <build-dir> --config Release --target doc-python-api
```

Output lands at `<build-dir>/doc-python-api/html/index.html`.

An incremental build (skipping the deletion) is acceptable only for a quick local look at a page
you are actively editing — say so explicitly when you do it.

## Things that will bite

- Sphinx runs with `-W`, so every warning is a build failure.
- If a class was removed or renamed, its stale `generated/*.rst` must be deleted or the build
  fails. The `rm -rf` above covers this.
- Editing values in `conf.py.in` invalidates doctrees; editing *code* in it does not — another
  reason to build from scratch.
- Never set `maximum_signature_line_length`. It breaks the `[optional]` pseudo-arglist rendering
  that this API's docstrings depend on.

## Verify

After the build, confirm `searchindex.js` actually has index entries rather than assuming the
build was clean — a zero count means the from-scratch deletion did not take effect.

Conventions for the docstrings themselves are in `doc-python-api/README.md`; see the
`/api-docstring` skill.
