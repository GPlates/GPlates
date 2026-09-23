# Survey prompt (stage 1)

Launch with the `architecture-writer` agent (`~/.claude/agents/architecture-writer.md`: Fable,
effort high). Delete this file once the survey is done.

---

Task: stage 1 of the architecture-diagrams work, the **survey**. You propose which areas of the
code get their own architecture page, where each area's boundary lies, which areas matter most,
and how the layer groups should change. The developer approves your proposal before any page is
written, so this is a proposal, not documentation.

## Where to work

Work in the worktree `C:/gplates/src/architecture-diagrams` (branch
`feature/architecture-diagrams`; its source tree is the current `gplates` development branch).
Read these first:

1. `AGENTS.md` (conventions, and the two-products structure).
2. `docs/plans/architecture-diagrams/PLAN.md` (the whole plan: page template, where pages live,
   the pilot, the layer diagrams, and why).
3. `docs/design/architecture/README.md` and `docs/design/architecture/dependency-matrix.md` (the
   current layering description, the include matrix, and which files the pyGPlates module
   compiles).
4. Existing area-style docs: `docs/design/scribe-system/` in the worktree, and
   `docs/design/model-system/` on branch `feature/pygplates-model-revisions` (read with
   `git show feature/pygplates-model-revisions:docs/design/model-system/README.md` etc.). That
   branch reworks the model and is not merged yet; survey the model as it is on `gplates`, but
   note where that branch changes the picture.

Prototype scripts are in `docs/plans/architecture-diagrams/prototype/`. Run them with `python`
(the conda `gplates` environment's Python is on PATH): `python mermaid_proto.py` and
`python mermaid_proto.py --pygplates` print Mermaid layer diagrams (upward edges on stderr);
`python scc.py` prints the include cycles. You may copy and modify them in your scratch space to
test alternative layer groupings, but don't edit the committed copies.

## Context for judging importance

Upcoming work the pages should serve, in roughly this order:

- a redesign of the GPGIM (the GPlates Geological Information Model: feature types and
  properties) to make it lighter and less restrictive, with a refactor of the OGR
  (shapefile/GeoPackage/etc.) file I/O that goes with it;
- the reconstruction model classes and topologies, used heavily by both products;
- rendering: GPlates will eventually move from OpenGL to Vulkan (on a separate, very incomplete
  branch; ignore it and survey the OpenGL code on `gplates`), and a new rule-based symbology
  system based on feature properties will replace or complement the current colouring
  (Python-based draw styles, the Manage Colouring dialog);
- a long backlog of feature requests, which will be triaged against the area pages.

## What to produce

Write one file: `docs/plans/architecture-diagrams/survey.md` in the worktree. Sections:

1. **Proposed areas.** For each area: name; one paragraph on what it does; its boundary as
   directory parts (a directory, or named files/classes within one when an area is only part of
   a directory) and main classes; which product(s) use it; the 3-6 files to open first; and how
   it connects to other areas. Aim for roughly 8-15 areas. An area is a subject (what the code
   is about), not a directory: some areas will span directories, and some directories (notably
   `app-logic/`, `gui/`, `utils/`, `presentation/`, `view-operations/`, `qt-widgets/`) are
   mixtures that will span several areas. Say explicitly which areas are cross-cutting.
2. **Draft overview diagram.** One Mermaid diagram of the areas and the main data flow between
   them (the level-1 view), plus a short explanation. Make it readable: aim for at most about 15
   nodes.
3. **Priority.** Rank the areas for writing pages, with a one-line reason each, using the
   context above. The pilot is already chosen: reconstruction. Say whether that choice holds up,
   and define the reconstruction area's boundary carefully, since it is written first.
4. **Layer groups.** Propose revised layer groups for the generated diagram (the `LAYERS` list
   in `mermaid_proto.py`), with the upward edges your grouping leaves and why each one is either
   acceptable or a real layering problem. Test your grouping with a modified copy of the
   script. Note where the README's "The layering" section is wrong about the current code.
5. **Existing design docs.** For `scribe-system/` and `model-system/`: which area each belongs
   to, and what the page template (in PLAN.md) would add to them.
6. **Surprises and problems.** Anything a developer returning to this code should know: dead or
   near-dead code, misplaced classes, surprising dependencies, areas that are much larger or
   smaller than their directory suggests.
7. **Working notes.** The file references behind your claims, grouped by section, so the review
   can check them.

Be thorough in reading, concise in writing: the survey should be readable in about 20 minutes.
Everything you state must be confirmed against the code (see your standing rules). The tree is
large (about 2,000 source files), so sample deliberately: headers and class declarations,
CMakeLists.txt source lists, the include matrix, and the main entry points
(`src/gplates_main.cc`, `src/api/PyGPlatesModule.cc`, `app-logic/ApplicationState`,
`presentation/Application`, etc.) before reading implementation files.
