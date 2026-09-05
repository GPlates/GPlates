# `src/` architecture: two products, one tree

GPlates (the Qt desktop application) and pyGPlates (the Boost.Python extension module) are
built from this one source tree, selected by the CMake option `GPLATES_BUILD_GPLATES`
(see `AGENTS.md` at the repository root for the build mechanics). This document describes the
intended layering of `src/` and the boundary between what the two products compile.

There is one architecture doc for all of `src/` — GPlates is the superset, and the pyGPlates
boundary is marked within it rather than described separately.

## The layering

```
    GPlates only                                  .----------------------------------.
    .-----------------------------------------.   |  embedded interpreter runtime    |
    |  qt-widgets   canvas-tools              |   |  (api/Python*, api/Console*,     |
    |  view-operations   presentation   cli   |   |   api/Sleeper, api/PyOldFeature*)|
    |  gui (controllers)   data-mining        |   '----------------------------------'
    |  opengl                                 |
    |  app-logic (layers machinery: Layer*,   |
    |   *LayerProxy/Task/Params, Reconstruct- |
    |   Graph*, ApplicationState, Feature-    |
    |   CollectionFileState/IO, UserPrefs)    |
    '-----------------------------------------'
                        |
                        v   (GPlates layers on top of the core; the core never
                             includes upward into this GPlates-only layer)
    Shared core (what the pygplates module compiles, and GPlates builds on)
    .-----------------------------------------------------------------------.
    |  api (the pyGPlates bindings: Py*.cc exporters + helpers)             |
    |  app-logic (reconstruction core: ReconstructMethod*, Reconstruct-     |
    |   Context, ReconstructionTree*, topology resolvers, ...)              |
    |  file-io (core readers/writers: GPML, PLATES, OGR, rasters, ...)      |
    |  feature-visitors (the API-reached subset)                            |
    |  gui (value types only: Colour, palettes, Mipmapper)                  |
    |  scribe (binary archives + transcribing; text/XML archives are        |
    |   GPlates-only)                                                       |
    '-----------------------------------------------------------------------'
    |  model   property-values   maths   utils   global   qt-resources      |
    |  (gpgim.qrc + python.qrc shared; opengl.qrc + qt_widgets.qrc are      |
    |   GPlates-only)                                                       |
    '-----------------------------------------------------------------------'
```

pyGPlates drives reconstruction through `ReconstructMethodRegistry` / `ReconstructContext` /
`ReconstructionTreeCreator` directly and never builds a `ReconstructGraph`, which is why the
whole layers machinery (and everything only it reaches, e.g. `data-mining`) is GPlates-only.

The per-directory numbers, the full include matrix between directories, and the exact list of
files the module takes from each partially-included directory are in the generated
[dependency-matrix.md](dependency-matrix.md).

## How the boundary is defined and enforced

The module's source list is not curated by hand: it is the **include closure of the pyGPlates
API**, computed by `cmake/pygplates_source_closure.py`. The roots are discovered automatically
from the `export_*()` calls that `BOOST_PYTHON_MODULE` and `export_cpp_python_api()` make
outside the `GPLATES_PYTHON_EMBEDDING` guard (`src/api/PyGPlatesModule.cc`), plus
`src/ScribeExportPyGPlates.cc`; quoted includes are traversed, and reaching `X.h` pulls in
`X.cc`. The per-directory `gplates_only_srcs` exclusion lists (and the `gui/` allowlist) in
`src/*/CMakeLists.txt` are derived from that closure.

Enforcement, in CTest (run `ctest --test-dir <build-pygplates> -C Release`):

- **pygplates-source-closure-test** re-runs the tracer against the source list the configure step
  actually gave the `pygplates` target (`<build>/pygplates_sources.txt`), failing on over-
  *and* under-inclusion, on any reach into a forbidden directory or a Qt Widgets / OpenGL /
  Qwt angle include, and on drift of the committed `dependency-matrix.md`.
- **pygplates-linkage-test** inspects the built module's direct shared-library dependencies
  (`dumpbin` / `readelf` / `otool`), failing if one of GPlates' GUI/rendering libraries
  (Qt Widgets, Qt Gui, Qt Svg, the Qt OpenGL modules, Qwt, GLEW) appears — or the platform's
  own OpenGL (GLVND's `libOpenGL`/`libGLX`/`libGLdispatch`, the legacy `libGL`, the macOS
  framework, `opengl32.dll`). There is no exemption: the module links only `Qt6::Core` and
  `Qt6::Core5Compat`, and a GL library on its link line means a GUI library has crept back in
  (`Qt6::Gui`'s imported target propagates Qt's own OpenGL dependency onto everything linking
  it, which is how the Linux wheel once needed `libGL.so.1` just to be imported).

## Rules for new files

- **New file in a GPlates-only directory** (`qt-widgets`, `opengl`, `canvas-tools`,
  `view-operations`, `presentation`, `cli`, `data-mining`, `gui`, `unit-test`): nothing to
  do — those directories (and everything not on the `gui/` allowlist) are excluded from the
  module by default.
- **New file in a shared directory**: if the API cannot reach it, add its `.h` *and* `.cc` to
  that directory's `gplates_only_srcs`; if the API can reach it, just list it in `srcs`. The
  closure test tells you which, and fails with the include chain when a listing is wrong.
  Keep `.h`/`.cc` pairs together: AUTOMOC mocs any Q_OBJECT header listed in the module's
  sources, and a moc'ed header whose `.cc` is not compiled fails at link time.
- **A shared TU must not reference a symbol defined only in a GPlates-only TU.** The Release
  linker can hide such a reference (`/OPT:REF` discards unreferenced code); a **Debug**
  pyGPlates link is the reliable check.
- **Python bindings that need the embedded interpreter** go behind `GPLATES_PYTHON_EMBEDDING`
  (a compile definition only `gplates-lib` has) and are registered inside the guarded block of
  `export_cpp_python_api()`.
- **Error reporting in shared code** goes through return values / exceptions /
  `ReadErrorAccumulation` — never a `QMessageBox` or other widget (shared code cannot assume
  a GUI, or even Qt Widgets at link time). When shared code genuinely needs a GUI decision,
  or a Qt Gui/Widgets facility, inject an interface the GPlates side implements and registers
  from `presentation/FileIOInjections.cc` (`register_file_io_injections`, called by both the
  GUI's `Application::initialise()` and the command-line path in `gplates_main.cc` — the CLI
  never constructs an `Application`, so a registration made only there silently regresses
  `gplates convert-file-format` and friends). Three examples: `file-io/PropertyMapper.h`
  (implemented by `qt-widgets/ShapefilePropertyMapper`, set with
  `OgrReader::set_property_mapper`); `RasterReader::set_rgba_reader_factory` (the
  `QImage`-based `file-io/RgbaRasterReader` for BMP/GIF/JPEG/PNG/SVG rasters — without it a
  module reports such a raster as unreadable, which pyGPlates cannot observe); and
  `GeoscimlProfile::set_progress_reporter_factory` (the `QProgressDialog` in
  `qt-widgets/GeoscimlProgressDialog` — without it the `.gsml` reader runs silently).

## Deferred / follow-up work (not in the build-graph-split PR)

- Decouple `GmlFile` from its `ProxiedRasterCache` member (TODO at
  `property-values/GmlFile.h`): today loading a raster GPML opens the raster through GDAL at
  parse time. Worth doing on its own merits, but note that it does **not** shrink the module —
  it removes exactly one translation unit. `RasterReader`/GDAL, `gui/Mipmapper` and
  `gui/RasterColourPalette` are reached independently of `GmlFile`, through the
  `RasterBandReaderHandle` member of `property-values/RawRaster.h`, which
  `app-logic/ExtractRasterFeatureProperties.h` puts on the API's own feature-collection
  classification path.
- Physically move the `app-logic` layers machinery into its own directory, and the ~17
  embedded-interpreter files out of `src/api/` (they serve `gui/PythonManager`, not the API).
  Directory moves multiply merge risk across the two develop branches, the `vulkan` branch
  and the downstream fork, so this waits for a dedicated both-develop-branches commit at a
  quiet merge point.
- Dissolve `src/feature-visitors/` and delete the dead `deprecated/` subtrees (separate PR,
  already planned).
