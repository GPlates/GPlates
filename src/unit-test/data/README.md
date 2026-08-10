# Unit test data

Data files used by the GPlates C++ unit tests (the `gplates-unit-test` executable built from
`src/unit-test/`).

Tests locate this directory through the `GPLATES_UNIT_TEST_DATA_DIR` compile definition, which is
set in `src/CMakeLists.txt`. Paths are therefore absolute and independent of the working directory
a test happens to run in — do not assume a test's working directory when adding fixtures. For
example:

```cpp
CptPalette cpt_palette(GPLATES_UNIT_TEST_DATA_DIR "/cpt/cpt_unit_test.txt");
```

## This is not the sample data shipped to users

The datasets that accompany a GPlates release are compiled separately by researchers in our group.
They are practical, real-world model data and are much larger than anything here:

- Release-compatible data: https://www.gplates.org/download/#download-gplates-compatible-data
  (kept current for each release)
- Tutorials, which carry their own datasets: https://tutorials.gplates.org/

Nothing in this directory is installed or packaged. It exists only to be read by tests.

## What is under test

| Directory | Used by |
|---|---|
| `cpt/cpt_unit_test.txt` | `CptPaletteTest.cc` |
| `citcoms-velocity-domains/` | `GenerateVelocityDomainCitcomsTest.cc` |

**Everything else is not yet referenced by any test.** It arrived on 2026-08-11 from the old
top-level `sample-data/` directory, which nothing in the build, the installer or the test suite
referenced. It was retained rather than deleted because it is the repository's only example of
several formats GPlates must read, and so is the natural starting point for file-format coverage
when comprehensive C++ testing is taken on (bringing GPlates on par with pyGPlates, which is
already comprehensively tested).

Anything still unreferenced at that point can simply be deleted.

## Contents

- **`citcoms-velocity-domains/`** — 48 CitcomS velocity domain meshes, `<r>.mesh.<n>.gpml.gz` for
  resolutions 9, 17, 33 and 65 across caps 0–11.
- **`cpt/`** — `cpt_unit_test.txt` plus six example colour palettes covering the regular and
  categorical CPT flavours. Distinct from the palettes GPlates ships as Qt resources in
  `src/qt-resources/*.cpt`.
- **`deformation/`** — deformation network fixtures (`def_*.gpml`), crustal thickness and velocity
  scalar coverages, a velocity domain, and 21 `DEF_TEST_t*Ma.xy` files.
- **`geojson/`** — a single GeoJSON file; the only one in the repository.
- **`gpml/`** — topological feature fixtures (`topology_test_*`), deformation source networks,
  co-registration seed/target pairs, symbology test features, and `error_missing_ref.gpml` (a
  deliberately broken file for error-path testing). `all_caps.gpml` is the largest file here at
  246 KB.
- **`plates4-line/`** — PLATES4 `.dat` line files; the only ones in the repository.
- **`plates4-rotation/`** — a `.rot` rotation file and `grot_example.grot`; the only `.grot` in the
  repository.
- **`shapefiles/`** — three shapefiles; the only ones in the repository.
- **`sym/`** — symbol files for the `.sym` format read by `src/file-io/SymbolFileReader.cc`, plus a
  GPML file of matching test features. The only `.sym` files in the repository.

Two fixture families are worth noting. `rotations_about_axes` is the same rotation model expressed
both as GPML (`gpml:TotalReconstructionSequence`) and as a PLATES4 `.rot` file. Separately, the
`*_rotates_about_{x,y,z}` files are synthetic geometry for those rotations to act on, present as
PLATES4 `.dat` line files and — for `front_line` — as shapefiles. Between them they cover reading a
rotation model in two formats and a geometry file in two formats.

## Known anomalies

Recorded here so a future audit does not have to rediscover them. None has been corrected, since
none of these files is under test yet.

- **`shapefiles/`** — the three `.shp` files are byte-identical, as are the three `.shx`. Only the
  `.dbf` attribute tables differ. This appears intentional (one geometry, three attribute sets), and
  all nine files are required: each shapefile needs its own `.shp` and `.shx` to load.
- **`gpml/deformation_test_src_net_4_points.gpml`** is byte-identical to
  `gpml/deformation_test_src_net_5_points.gpml`, despite names implying different point counts. One
  of the two is likely misnamed or was copied over in error.
- **`deformation/def_EW_shearing.gpml`** is byte-identical to
  `gpml/deformation_test_src_net_MULTI-triangle.gpml`.
- **`deformation/rotations_about_axes.rot`** and **`plates4-rotation/rotations_about_axes.rot`**
  share a name but differ in content, so the two directories are not interchangeable.
- **`deformation/DEF_TEST_t*Ma.xy`** are exported *output* from a deformation run, not inputs. They
  have no accompanying runner or expected-result comparison, so their provenance and intended
  tolerance are unknown.

Duplicate files cost nothing in the repository — git stores content once and addresses it by hash,
regardless of how many paths point at it.
