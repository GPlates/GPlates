# Architecture diagrams for GPlates / pyGPlates

Status: agreed 2026-09-24, on the long-lived branch `feature/architecture-diagrams` (worktree
`architecture-diagrams`). Stage 1 (the survey, `survey.md`) is done, and its areas, priority and
layer proposal A (survey section 4) were approved 2026-09-25. Stage 2 (the pilot pull request) is
written: the generated diagrams, the README, the scribe move, the `AGENTS.md` pointer, and the
reconstruction page (written by the architecture-writer agent, reviewed against the code; its
evidence and layout findings are in `reconstruction-notes.md`). Next: the reconstruction refactor
assessment (see *Pilot area*), then the claim check (`claim-check-prompt.md`) and
`/code-review high`, then the developer reviews the pilot and judges the level of detail, then
the pull request is opened.

Found while porting the prototype: in a `flowchart BT` the including node is drawn *below* the
included one, so the layers came out upside down; the generator uses `TD`. And the layout ranks
by every edge, so an upward edge declared as `a -.-> b` pulls the layers out of order; it is
declared `b <-.-|n| a` instead (the comment at the code says so). Mermaid was checked by rendering
with `minlag/mermaid-cli` in Docker, since there is no Node on this machine.

## Goal

A map of how the code works, at a level of detail you can navigate, for two readers:

- the main developer, who loses track of how the detailed parts of each area work, and wants to
  find what needs improving;
- AI agents, who need the same map before upgrading an area or adding functionality to it.

It should earn its keep during the refactors ahead: the OGR / GPGIM redesign (a lighter, less
restrictive GPGIM), the Vulkan port (GPlates 3.0), and the rule-based symbology system that
follows it. Behind those sits a long list of feature requests (see *Feature requests* below).

Not goals: exhaustive per-class diagrams, an API reference, or bringing back Doxygen. Its
machinery was retired in `2d74ce5d5`; its graphs are uncurated, and it can't draw sequence
diagrams.

## Format

- **Mermaid inside Markdown**, committed in the repository. GitHub renders it, and agents read it
  as text. No extra tooling, and no images to go stale.
- **C4-style zoom levels**, without C4's full notation:
  1. *Overview*: the two products, the layer groups, the main areas, and how data flows between
     them.
  2. *One page per area*: a component diagram, and one or two sequence diagrams for the
     operations that matter.
  3. *Class diagrams only for class families that matter* (for example the `PropertyValue`
     hierarchy or the layer proxies). No code level below that.
- Drill-down is by **Markdown links** beside each diagram. Mermaid `click` links are disabled on
  GitHub.

## Where it lives

```
docs/design/architecture/
  README.md               overview (level 1), layer groups in prose, directory rules, area index
  dependency-matrix.md    generated: layer diagrams + the matrix + the module subset
  <area>/README.md        one directory per area (level 2), more pages beside it as needed
  scribe-system/          moved here from docs/design/
  model-system/           moved here when feature/pygplates-model-revisions merges
```

- Areas go **under `architecture/`**, not directly in `docs/design/`. That directory also holds
  topics that aren't code areas (`testing/`, `versioning/`, and more to come).
- **`scribe-system/` moves** into `architecture/` in the pilot pull request. Two references need
  updating: `src/api/PythonPickle.h` (twice). Its directory name stays the same, so
  `model-system/README.md`'s relative link `../scribe-system/README.md` still works once both
  have moved.
- **`model-system/`** exists only on `feature/pygplates-model-revisions`, and that branch owns it.
  It moves at the branch's next sync with `gplates` after the pilot lands, or in the merge that
  brings it to `gplates`. Put a note on that branch's row in the roadmap.
- `AGENTS.md` gets a short pointer: before changing an area, read its page; a pull request that
  changes an area's structure updates its page in the same pull request.

### Existing design doc sets

`scribe-system/` and `model-system/` are already level-2/3 docs: deep, multi-page, and recently
checked against the code. **They are not rewritten into the template.** Instead:

- each one's `README.md` gets the parts of the template it lacks: a component diagram, entry
  points, and the commit it was last checked against;
- ASCII diagrams become Mermaid only where that is clearer (scribe's two-stage pipeline diagram
  is fine as it is);
- the overview links to them like any other area.

## The generated layer diagrams

These extend `cmake/pygplates_source_closure.py`, which already computes the data, so the
existing `pygplates-source-closure` drift check keeps them current for free.

Prototype: [`prototype/mermaid_proto.py`](prototype/mermaid_proto.py) prints the Mermaid
(`--pygplates` for the module-only diagram, and the upward edges on stderr), and
[`prototype/scc.py`](prototype/scc.py) prints the cycles. Both import the closure script. To be
ported into it, not copied: they go when this plan is deleted. Version 2 is rendered as a
private artifact at https://claude.ai/artifact/HSs3U2rgkb7UTWNH8gkodv.

**How it works:**

- Each directory is split into its **pyGPlates-module part and its GPlates-only part**, using the
  closure. A directory the module never reaches (`opengl`, `data-mining`, ...) is wholly
  GPlates-only. (Prototype v1 coloured those blue by mistake; fixed in v2.)
- The **intended layering** is a hand-written list of layer groups, each naming the directory
  parts in it. That list is the design statement, and the diagram measures the code against it.
  Edges within a group aren't drawn, since anything in a group may include anything else in it.
  Downward edges are **transitively reduced** (168 become 65). **Upward** edges are all drawn in
  red with their include counts.
- `utils` is marked **cross-cutting**: its upward edges are drawn in amber rather than red. It
  holds utilities for many directories, so some of them are fine.
- **Two diagrams**: both products together, and the pyGPlates module on its own (the module
  nodes and the edges between them). The combined one shows the boundary; the pyGPlates one is
  cleaner for pyGPlates work.

**What the code actually has.** Measured with strongly connected components, before any intent
is applied, the code has almost no layering:

| edges counted | groups that form one cycle |
| --- | --- |
| every include | two: the whole shared core (9 parts) and the whole GPlates-only half (11 parts) |
| 3 or more includes | `global`/`maths`/`scribe`/`utils`; `app-logic`/`file-io`/`gui`/`model`/`property-values`; the GPlates-only half |
| 10 or more includes | `app-logic`/`file-io`/`model`/`property-values`; GPlates-only `app-logic` with `opengl`; the GPlates UI directories |

So the layers are something to aim for, not something to discover, and the red edges are the
gap. The layer groups in prototype v2 were written by hand: the shared core loosely follows the
README's ASCII diagram, and the GPlates-only half was split into *engine* and *user interface*
by judgement. v1 also put `qt-widgets` above the rest of the UI, the usual shape for a Qt
application; the 163 includes from `gui` into `qt-widgets` showed that shape doesn't hold, so v2
merges them into one group.

**Upward edges against the v2 groups:**

- in the pyGPlates module: `property-values -> file-io` (11), `model -> app-logic` (7),
  `property-values -> gui` (5), `model -> file-io` (2);
- GPlates-only: `opengl -> gui` (8), `file-io -> gui` (5), `opengl -> view-operations` (3),
  `data-mining -> gui` (1).

These overlap with the survey in the pending **Directory layering** item
(`feature/directory-layering`), which already names the files behind most of them (for example
all 7 `model -> app-logic` includes are in `WeakObserverVisitor.cc`). The two pieces of work fit
together: the layer groups say what directory-layering fixes against, and each fix it makes
removes a red edge.

**Decisions:**

- The README's ASCII diagram in *The layering* goes. The README keeps prose: what each layer
  group is for, what an upward include means, and a link to the diagrams. The diagrams live
  only in the generated `dependency-matrix.md`, so nothing generated is mixed into a
  hand-written file.
- The layer-group list lives in the script, beside `FORBIDDEN_DIRS`, with a comment pointing at
  the README. The drift check fails if a directory part isn't in any group, so a new directory
  can't go unplaced.
- The groups are provisional. The Fable agent writing the area pages proposes regroupings as it
  learns the code; you approve them.
- Deferred: making the test fail when a *new* upward edge appears (a ratchet). Not until the
  groups have settled, which is after the move and its enabling refactors: the ratchet is turned
  on at the start of the area refactors (piece 8 of the umbrella plan).

## Area pages

Each page, in this order:

1. What the area is for, and its boundary (which directory parts and which main classes).
2. A component diagram.
3. One or two sequence diagrams for its key operations.
4. How it works, in prose: the part that gets forgotten.
5. Constraints and traps, and why the area is shaped this way.
6. Known weaknesses and deferred work. `AGENTS.md` allows deferred work in design docs; it does
   not allow progress.
7. Entry points: the files to open first.
8. The commit it was last checked against.

Beside the page, the agent also writes:

- **Layout evidence**, in the area's notes file in this plan directory (`<area>-notes.md`,
  never in `docs/design/`): files that belong to another area, directories with no single
  subject, and upward includes caused by code sitting in the wrong file. Input to stage 4.
- **A refactor assessment**, in the private planning repository, as
  `plans/area-refactors/<area>.md` (umbrella plan, *Refactoring*): each part of the area rated
  *fine*, *touch-up*, *restructure* or *rewrite*, with the reason and where in the code, and
  what upcoming work the area must be shaped for, taken from the topic plans there whose
  `Areas:` line names the area ("none known" if none). It is judgement for planning, not a
  description, so it stays off the page, which states only the weaknesses behind it, as facts.
  It is kept out of this directory because these notes become public with this plan's pull
  requests and are deleted with it, while the assessment is needed until the area's refactor
  (after the move), and becomes that refactor's plan. The notes here never cite it.

### Which areas

These are **discovered, not fixed in advance**. The first Fable task (stage 1 below) surveys the
tree and proposes the area list: each area's boundary in directory parts and main classes, which
areas matter most, and how the layer groups should change. You approve the list. It is then
revised as pages get written, when an area turns out to be two areas or to cross a directory
boundary.

Starting candidates, for the survey to confirm or replace: pyGPlates bindings (`api/`, and the
embedded interpreter side of it), reconstruction, topologies, model and property values (GPGIM),
file I/O (OGR, GPML, rotation files), GPlates layers (`ReconstructGraph`, proxies), rendering,
colouring and draw styles, and scribe (exists).

### Pilot area: reconstruction

The pilot is reconstruction, because the pilot is where you judge the level of detail, so it
should be typical of the code the pages are for: data flowing through the core (rotations,
reconstruction trees, reconstruct methods, reconstructed geometries) and serving both products.
Its sequence diagrams test the template properly.

Rejected: the pyGPlates bindings (`api/`). It is atypical: a page about it would mostly cover
binding machinery (export registration, conversions, wrapper types) rather than what the code
does. It is also the area the unmerged Docs E branch is editing. It stays on the area list.

The model area is out while `feature/pygplates-model-revisions` is unmerged.

The pilot page was written before the refactor assessment was added to the template. Its
assessment is written separately, by the `architecture-writer` agent, into
`plans/area-refactors/reconstruction.md` in the planning repository, before the claim check
runs, so the check covers it too.

### Branches

Every page is written on `gplates` and describes the code there, rendering included.
`feature/diligent-migration` is far from complete (only arrows render on `feature/vulkan`, and
nothing renders on `feature/diligent-migration` yet), and a lot of work will land on `gplates`
before it merges. That branch updates pages as it changes the areas, and its pages will diverge.
When 3.0 merges, its versions win for rendering and symbology.

### During refactors

An area page describes the code **as it is**. The target design for a refactor (for example the
lighter GPGIM) belongs in that refactor's plan until it lands. The pull request that completes
the refactor then rewrites the page. So the page from before the refactor is the baseline the
refactor is planned against, and it is replaced when the refactor ends.

## How the pages are written

- **One fresh Fable subagent per task** (survey, or one area), with a self-contained prompt: this
  template, the area's boundary, and the rule that every box, arrow and statement must be checked
  against the code. This follows the `CLAUDE.md` rule for distillation, for the same reason: a
  mistake here is silent and persistent.
- I review the output against the code, then you review it.
- Keep diagrams at the level of concepts and main classes, so ordinary refactoring doesn't make
  them wrong.

## Stages

The branch is long-lived, but its work reaches `gplates` in several pull requests. Each one
distils what it completed, and the last one deletes this plan.

1. **Survey** (Fable): the proposed area list and boundaries, the areas that matter most, and
   proposed changes to the layer groups. It goes into this plan, not the repository.
2. **Pilot pull request**:
   - the generated diagrams in `cmake/pygplates_source_closure.py` and `dependency-matrix.md`;
   - the README reworked: overview, layer groups in prose, area index, ASCII diagram removed;
   - `scribe-system/` moved under `architecture/`, and its README retrofitted;
   - the `AGENTS.md` pointer;
   - the reconstruction area page.

   After it, you judge the level of detail before we go on.
3. **The priority area pages**, one pull request per area (or two small areas together), in
   the survey's order: reconstruction (the pilot), GPGIM, feature file I/O, topologies, layers,
   colouring, scene rendering, OpenGL.
4. **Layout design** for the source reorganisation (umbrella plan:
   `gplates-planning/plans/source-reorganisation.md`, private). From the areas and their
   file-level boundaries: the proposed directory tree and its names, the namespace policy
   (namespacing by area, a common root, or coarser; say first what namespaces are for), where
   the pyGPlates / GPlates split sits in the tree, and the design of the migration script. Its
   output is a design document, approved before anything moves. It also lists:
   - the **enabling refactors** (umbrella piece 4a): the files that must be split, and the code
     that must move between files, before the script can place every file in one area. They
     land on `gplates` before the move, as small behaviour-preserving pull requests;
   - the **target sub-structure** of an area whose refactor would reshape it, but only if the
     chosen namespace policy puts namespaces at sub-directory level. With namespaces per area,
     a move inside an area rewrites only `#include` paths, so each refactor (piece 8) shapes
     its own area's inside when it starts, with its plan in hand.

   The Fable agent writing pages notes layout evidence and a refactor assessment as it goes (see
   *Area pages*), so this stage starts from collected evidence rather than a fresh survey.
5. **After the move:** paths updated in the pages already written, and the stable areas'
   pages (export, sessions, canvas tools, auxiliary tools) written against the new tree.
6. **`model-system/`** moved and retrofitted once `feature/pygplates-model-revisions` merges
   (before the move, which waits for it).

## Feature requests

The list of requests is its own plan. This is how it uses the area pages.

1. **Collect.** Each request becomes a short record: what the user wants, and who asked.
2. **Triage against the pages.** For each request, answer from the area pages first: which areas
   it touches, and whether the current design can take it. Three outcomes:
   - *fits*: the design already has a place for it (a new property value, another reader);
   - *extends*: it needs an extension point the area lacks;
   - *restructures*: it needs a structural change.

   An agent can do the first pass by reading only the pages, then check its answer against the
   code. If a request can't be triaged from a page, the page is missing something, so fix the
   page.
3. **Cluster.** Requests that need the same structural change justify that change as an
   enabling refactor. This is where priorities come from: "lighter GPGIM" or "rule-based
   symbology" is worth doing because of the requests it unblocks, and the cluster says which
   ones.
4. **Record in two places.** The request list holds the triage: areas, outcome, and enabling
   change. The area page's *Known weaknesses and deferred work* gets the structural limitation,
   stated as a limitation of the design. The request itself stays out of the design docs: it
   describes what's wanted, not what is.

   The list is **private**, and the pages are public, so the links run one way: the list
   links to the page, and the page never links to the list (a public reader would find a dead
   link). A page links to a request only once it has become a public issue, which happens when
   it is triaged, clustered and given a milestone. Until then the limitation stands on its own.
5. **Implement.** The work's plan starts from the area pages. The pull request updates them, and
   the list marks the request done. When a refactor removes a limitation, that entry leaves the
   page.

## Open questions

- Where the private request list lives, and how requests are collected. (For the
  feature-request plan, not this one.)
