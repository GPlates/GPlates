# Branches and versions

How this repository is branched, how a version is chosen, and why. It is a decision record — what
was chosen, what was rejected, and the evidence — rather than a tutorial: `README.md` at the
repository root is the "how do I use this repository" document, and links here.

The two development branches were unified on 2026-09-12. Everything below was measured in the
days around that, and is dated wherever it will go stale.

## 1. The decision

Before (four permanent branches, gitflow):

```
gplates    (default) ──┐                 release-gplates    ← GPlates release tags
pygplates  ────────────┘  near-identical  release-pygplates  ← pyGPlates release tags
                                          release/<product>-<version>   temporary
                                          hotfix/<product>-<version>    temporary
```

After (one permanent branch, plus one per release series):

```
gplates                    permanent, the default: both products are developed here
release/gplates-2.6        permanent, carries 2.6.0, 2.6.1, …
release/pygplates-1.1      permanent, carries 1.1.0rc1, 1.1.0, 1.1.1, …
feature/<name>, fix/<name>     temporary, off the development branch
<patch branches>               temporary, off a release series branch
```

The rules that follow from it:

- **Release tags live only on release series branches**, never on the development branch. The
  candidates, the release, and each later patch release are successive commits on the one
  branch, so its tip is always the newest X.Y.z.
- **There is no permanent 'production' branch and no `hotfix/` concept.** A patch is a commit on
  the series branch, tagged there; a patch that also applies to the development branch is merged
  there too.
- **A series branch is cut when the first release in the series is prepared**, not before. A
  branch cut ahead of need is an empty branch. As of 2026-09-12 none has been cut yet; the first
  will be `release/pygplates-1.1`.
- **Series branches are never deleted.** GDAL keeps 29 and QGIS 71 (section 4); a branch ref
  costs nothing, and deleting one loses the "check out the latest 2.6.x" contract for that series.
- The development branch keeps the name `gplates` for now and will be renamed `main` in a
  separate change (section 12).

This is the trunk-plus-release-series model that QGIS, GDAL, CGAL, LLVM and CPython use. It is no
longer gitflow, and section 3 says why.

## 2. Why one development branch

Until 2026-09-12 there were two, `gplates` and `pygplates`, one per product, kept close by sync
merges in both directions. Both products could be built from either, so the split bought nothing
on its own; what it cost was measured while deciding to end it.

**Every change had to be synced, and the sync merge is where problems surfaced.** A defect
written on one branch became visible on the other only when merged across, weeks later, where it
looked like the merge's fault. The clearest case arrived while this change was being planned.
`GeoscimlReaderTest` was written on `pygplates`, where the GPlates unit tests never run. The sync
merge `86cebd62f` (2026-09-11) was the first time it had ever executed on any platform's GPlates
CI, and two of its cases failed on macOS. Nothing about the merge was wrong: the test had been
reading freed memory since the day it was written (`only_geometry()` returns a
`non_null_ptr_to_const_type` by value and the test kept only the raw pointer, which happened to
stay valid for every geometry type but `gml:Point`), and only macOS's allocator dirties freed
memory. The same merge also left `pygplates-source-closure-test` failing on `gplates` for a
regenerated-doc drift that only a pyGPlates build could see. Both were fixed as PR #70, a month
after they were introduced.

**CI covered only one product per branch.** Each workflow ran on its own branch and built its own
product, so a change to the shared sources or the CMake source lists could break the other product
undetected. `AGENTS.md` had recorded this as a known gap and deferred it as a maintainer decision.
One branch cannot have a per-product push filter, so unifying the branches forced the decision:
**both products are now built on every push**. The added cost is less than it looks. The two
sccache namespaces are disjoint (every shared translation unit differs in `-fPIC` and
`GPLATES_PYTHON_EMBEDDING`), so the second build cannot evict the first's entries, and a change
that touches only one product hits the cache almost entirely for the other.

**The split also kept the compiler cache cold.** Measured across four `gplates` CI runs
(2026-09-11):

| run | sccache hit rate | why |
|---|---|---|
| 18 Aug, two pushes hours apart | 99.50% on all three platforms | ordinary incremental pushes |
| 6 Sept sync merge | 0% | no `gplates` run for 19 days, and GitHub evicts a cache untouched for 7 |
| 10 Sept sync merge | 0% | a cache was restored, but the merge brought 285 lines of `src/CMakeLists.txt` plus three other CMake files, and sccache keys on the command line as well as the source |
| 10 Sept, same push, `pygplates` | 98.90% | small incremental push |

So `gplates` scored 99.5% on the days it had nothing to compile and 0% on the days it did: its
real work arrived as occasional large merges, far enough apart to expire the cache and large
enough to invalidate what survived. With one branch every push is small and frequent, which is the
regime that already measured 98–99%.

**The two version counters drifted.** The development number is a first-parent count (section 7),
and each branch counts its own line. A merge adds exactly one commit to the receiving line however
many it carries, so routine syncing never converged them:

| measured | `gplates` | `pygplates` |
|---|---|---|
| 2026-09-10 | `1.1.0.dev40` / `2.6.0-47` | `1.1.0.dev46` / `2.6.0-54` |
| 2026-09-11, after PR #69 was synced | `1.1.0.dev41` / `2.6.0-48` | `1.1.0.dev47` / `2.6.0-55` |
| 2026-09-12, after PR #70 | `1.1.0.dev42` / `2.6.0-49` | `1.1.0.dev47` / `2.6.0-55` |

Two branches minting different numbers for the same product is tolerable only while nothing
publishes from the lower one. It also decided how the unification had to be done (section 11).

## 3. Why development happens on the default branch

gitflow's defining feature is that `master` holds the production-ready state and development
happens elsewhere. The default branches of GPlates' peers, queried live on 2026-09-11:

| repository | default branch | shape |
|---|---|---|
| qgis/QGIS | `master` | development; `release-3_44`, `release-4_0`, … per series |
| OSGeo/gdal | `master` | development; `release/1.4` … `release/3.13` per series |
| OSGeo/PROJ | `master` | development |
| Kitware/VTK | `master` | development, plus a rolling `release` branch |
| CGAL/cgal | `main` | development; `6.1.x-branch`, `6.2.x-branch` |
| llvm/llvm-project | `main` | development; `release/N.x` |
| python/cpython | `main` | development; `3.12`, `3.13`, … |
| numpy/numpy, scikit-learn/scikit-learn | `main` | development |
| qt/qtbase | `dev` | development |
| opencv/opencv | `5.x` | the next major's development branch |
| nvie/gitflow | `develop` | gitflow, by its author |
| boostorg/boost | `master` | gitflow proper: `master` (stable) + `develop`, default `master` |

Among the projects that resemble GPlates — C++, versioned, desktop, geospatial or scientific — the
pattern is near-unanimous: **the default branch is where development happens**, and "production"
is expressed as per-series release branches and tags. Boost is the one real counter-example and
the closest thing to a control. (Boost's branch list also carries `feature/gha` and
`fix/boost-1.91.0-gh-release`, independent corroboration of the `fix/<name>` convention here.)

gitflow's author reached a similar view. Vincent Driessen added a reflection to *A successful Git
branching model* on 5 March 2020: the model still suits teams "building explicitly versioned
software or maintaining multiple versions in production" — GPlates qualifies — but it came to be
applied dogmatically, and "panaceas don't exist. Consider your own context." The part that has
aged is specifically `master = production-ready`. It rests on a 2010 assumption that cloning is
how users obtain software. For GPlates it is not: GPlates users take binaries from gplates.org,
pyGPlates users take wheels from PyPI and packages from conda-forge. The people who clone are
developers and packagers, and GitHub's Releases page and the tags serve "give me the production
state" better than a branch does.

## 4. Why permanent per-series branches

Release branches are kept forever by the peers that have them, and the reason is patching, not
archival. Counted with full pagination (2026-09-11):

- GDAL: 29 `release/X.Y` branches, `release/1.4` through `release/3.13`. None deleted.
- QGIS: 71 `release-X_Y` branches, `release-0_0_11` through `release-4_2`. None deleted.

GDAL's `release/3.8` branch carries 18 tags:

```
v3.8.0beta1 v3.8.0RC1 v3.8.0RC2 v3.8.0
v3.8.1RC1 v3.8.1RC2 v3.8.1RC3 v3.8.1
v3.8.2RC1 v3.8.2  v3.8.3RC1 v3.8.3RC2 v3.8.3RC3 v3.8.3
v3.8.4RC1 v3.8.4  v3.8.5RC1 v3.8.5
```

One branch, six public releases, one preparation period; and its in-flight list showed
`backport-15205-to-release/3.13`, the short-lived patch branch pattern.

**This is why the permanent branch is named per series, not per version.** A branch is worth
keeping alive only if commits will be added to it later. A tag already names a commit, keeps it
reachable and is checkout-able, and does it better, being immutable; a branch that will never
receive another commit is only a second, mutable name for a commit the tag already names.
`release/gplates-2.6.0` is frozen the moment `GPlates-2.6.0` is tagged on it, since a released
version's content cannot change, so a 2.6.1 fix forces either a false name or a new branch per
patch, and in both cases "kept alive" is doing no work. `release/gplates-2.6` describes an ongoing
line: the tip is always the newest 2.6.x, and it is the unambiguous backport target.

## 5. What was rejected

- **Keeping gitflow.** Sections 2 and 3. Its permanent production branch answered a question
  ("what is released?") that tags and the Releases page answer better, and its two-develop-branch
  form here had measurable costs.
- **One interleaved release branch for both products.** The two `release-*` branches were
  justified only by there being two products, so merging them into one was considered. With one
  branch the tip means "the state at the most recent release of *either* product", so checking it
  out to build GPlates right after a pyGPlates release gives an untested GPlates, which breaks the
  "check out this branch to compile the release" contract. Per-series branches solve the same
  problem better, being scoped to one product *and* one release line.
- **A release branch per version.** Section 4.
- **Cutting series branches in advance.** An empty branch, and a name that implies a release is
  being prepared when it is not.
- **Renaming the development branch to `main` in the same change.** Deferred (section 12) because
  it is the only step that disturbs the downstream forks, and doing everything else first meant
  everything else could land immediately.

## 6. What deleting `release-gplates` and `release-pygplates` lost: nothing

Verified with `git merge-base --is-ancestor` before deletion:

- `release-gplates`'s tip `f31ac9455` is tagged `GPlates-2.5`.
- `release-pygplates`'s tip `27d48c91f` is tagged `PyGPlates-1.0.0`.
- Each tag's ancestry covers that branch's entire first-parent chain, back through 2.3 / 0.36.

Tags are refs, so every commit stays reachable, and GitHub lists tags and releases independently
of branches, so the old releases work exactly as before. The one recipe lost is
`git log --first-parent release-gplates release-pygplates`; section 10 has the replacement.

Deleting the old branches did not need the new series branches to exist first, because what the
old branches carried is tags. The first series branch is cut whenever the first release is.

`gplates-3.0-dev` was deleted at the same time. It was a leftover from the pre-Vulkan 3.0 work,
frozen since 2023-07-28, and its tip `0e51c7615` is an ancestor of `feature/vulkan`
(`git rev-list --count gplates-3.0-dev --not feature/vulkan` is 0), so every commit on it is
carried by a live branch.

## 7. How a version is chosen

`cmake/modules/VersionFromGit.cmake` computes it; its header comment is the reference, and this
section is the reasoning. There are two inputs: a hand-edited **release target** in
`cmake/modules/VersionRelease.cmake`, and the repository's tags.

### 7.1 The algorithm

1. List the tags matching the product's prefix (`GPlates-*` or `PyGPlates-*`).
2. Split each into a **base** and a **development number**, discarding any that do not parse. That
   is what excludes `GPlates-1.5+hellinger-testing`, `GPlates-0.9.10.1` and the couple of hundred
   `svn-migration/*` tags. Two-component tags are normalised (`GPlates-2.5` → `2.5.0`), so the
   historical tags count without retagging.
3. For each, `distance = git rev-list --count --first-parent <tag>..HEAD`.
4. Skip any tag at distance 0 that is not *at* HEAD. Those are tags HEAD is an ancestor of — they
   name a later commit — and without this a `git bisect` step or a `HEAD~10` build scored 0, won
   the minimum, and aborted the configure.
5. The nearest tag wins; among equally near tags the largest development number wins, so the
   count cannot go backwards.
6. **N = that tag's development number + its distance**, and the version is `join(target, N)`.

The base always comes from the *target*, never from the tag. A tag contributes a number and a
position in history; tags' bases are read only by the guards. That is most visible with anchor
tags (section 9): tagging `GPlates-2.5.0-2000` while the target is `2.6.0` yields `2.6.0-2000`.

`join` is spelt per product, so that each stays orderable under its own rules (and GPlates under
Debian's too):

| | N = 0 | N > 0 |
|---|---|---|
| GPlates, plain base | `2.6.0` | `2.6.0-8` |
| GPlates, pre-release base | `2.6.0-rc.1` | `2.6.0-rc.1.8` |
| pyGPlates, plain base | `1.1.0` | `1.1.0.dev10` |
| pyGPlates, pre-release base | `1.1.0rc1` | `1.1.0rc1.dev5` |

**Why `--first-parent`:** it counts the number of times the branch tip has advanced — one per
merged pull request or direct push — rather than every commit on every branch merged in. **Why
`rev-list --count` and not `git describe`:** `git describe --first-parent` requires the tag to sit
*on* the first-parent line and fails outright here ("No tags can describe"). Under this model no
release tag is ever on the development branch's first-parent line (section 8.1), and
`rev-list --count --first-parent <tag>..HEAD` needs no such ancestry.

**Why derive it at all:** a hand-incremented development number has to anticipate the order in
which branches will *merge*, which is not known while editing the file. Two pull requests opened
from the same base both take the next number, and whichever merges second is wrong. That happened
here — `7c6c6243f` jumped dev7 to dev9 because `c967acd21` had taken dev8 on another line the same
day — and the two products' counters had drifted apart. Git already knows the merge order.

Everything else is a fallback for builds with no repository, in this order: an existing CMake
variable (`-D…`), an environment variable of the same name, git, `PKG-INFO` (the pyGPlates
sdist), `cmake/modules/VersionRecorded.cmake` (written into the sdist by `cmake/version.py`),
then a fatal error. A shallow clone is refused outright rather than allowed to produce a smaller,
plausible number.

### 7.2 The guards

Four checks abort the configure rather than let a bad version reach a package:

1. **Standing on a release tag whose base is not the target.** Tagging `PyGPlates-1.1.0` while the
   target still says `1.0.0` is a fatal error naming what to set the target to. This is what makes
   a tagged release and its target agree by construction, and it is why `build-wheels.yml` can
   check the tag against the resolved version in its first minute.
2. **The target has already been released.** The nearest release is a plain release of the target
   and HEAD is past it: the next commit after tagging `1.1.0` would otherwise emit `1.1.0.dev1`,
   which sorts *below* the `1.1.0` just published. The fix is to set the next target, which
   `VersionRelease.cmake` says to do immediately after tagging.
3. **The target sorts below the nearest release.** Added on 2026-09-12 after verifying the hole:
   with `PyGPlates-1.0.0` long released, a target of `0.9.0` resolved to `0.9.0.dev49` and exited
   0. Check 2 fired only on *equality*; nothing required the target to be *greater*. The
   comparison is product-aware, because CMake's own `VERSION_LESS` reads `1.1.0rc1` and `1.1.0` as
   equal: numeric head first, then the suffix rank (pyGPlates `.postN` > final > `rcN` > `bN` >
   `aN`; GPlates final > `-rc.N` > `-beta.N` > `-alpha.N`). Check 2 is now one of this check's
   messages.
4. **The target skips a version.** Check 3 passes `1.2.0` after a released `1.0.0`, which is the
   accidentally-skipped `1.1` case. So the target's numeric head must be the reference's, or its
   next patch, next minor or next major:

   | reference | allowed targets | rejected |
   |---|---|---|
   | `1.0.0` (a release) | `1.0.1`, `1.1.0`, `2.0.0`, and candidates of those (`1.0.1rc1`, …) | `1.0.2`, `1.2.0`, `3.0.0`, `1.0.0rc2` |
   | `1.0.0rc1` (a candidate on this line) | `1.0.0rc2`, `1.0.0` | `1.0.1`, `1.1.0` |
   | `1.0.0rc1` (a candidate on another branch) | `1.0.1`, `1.1.0`, `2.0.0`, and candidates of those | `1.0.0`, `1.0.0rc2`, `1.0.2`, `1.2.0` |

   A candidate binds only the line it is on. Whether the reference tag is on HEAD's first-parent
   line is decided by walking back `HEAD~n`, with `n` the reference's distance: `~` follows first
   parents, so that commit is the tag's own if the tag is on the line, and otherwise the commit
   the series branch was cut from. On the line — the series branch, or a patch branch off it —
   only the candidate's own head may follow, as another candidate or as the release. Off the
   line — the development branch once the series branch cut from it has its first candidate, or
   a feature branch cut before then — the candidate's head counts as released, and *staying* on
   it is refused: the `1.1.0` line has moved to the series branch, which is minting
   `1.1.0rc1.devN`, and the development branch's count has restarted from the branch point
   (section 8.2), so left on `1.1.0` it would re-issue `1.1.0.dev1`, `dev2`, … for new commits.
   After a final release, same-head targets are rejected by check 3 wherever the tag is.

   The first cut of this rule made no on-line/off-line distinction, and a review before the
   branch merged found what that meant: the first candidate on *any* series branch became the
   development branch's reference (every series tag ties at the branch point, and the highest
   base wins), so every configure there aborted until the release was final — and, because both
   versions are always resolved, a GPlates candidate stopped every pyGPlates build too. The only
   accepted target in that window was the candidate's own head, which is the one that re-issues
   numbers. Hence the distinction, and the extra rows in the scenario table below.

   **This one is policy, not correctness**, and the error message says so, because it can be
   loosened without breaking anything. The evidence supports it: the GPlates release line has no
   skips (the apparent `0.9.5` → `0.9.7.1` gap is a released 0.9.6 whose tag went missing, and the
   big early pyGPlates jumps used the SVN revision as the minor version). A side benefit: a fork
   cannot quietly jump to `2.8.0` while upstream is on `2.6` (section 9).

**Which release is "the nearest release"** for checks 3 and 4 is a separate question from which
tag supplies the count, and is computed separately:

- **Plain releases only.** An anchor tag (development number ≠ 0) says nothing about what has
  been released, and a fork standing on `GPlates-2.6.0-2000` with target `2.6.0` is the normal
  configuration. So the nearest tag of any kind supplies N, while the guard looks at the nearest
  *release* tag, which may be much further back.
- **Among releases tied at that distance, the highest base.** From the development branch,
  `GPlates-2.6.0` and `GPlates-2.6.1` will tie (both on the series branch, both off the
  first-parent line — section 8.1). Comparing against `2.6.0` would make a legitimate `2.6.2` look
  like a skip; `2.6.1` gets it right.
- **Never the greatest tag anywhere.** A maintenance series branch legitimately targets `2.6.1`
  while `GPlates-2.7.0` exists on another series branch. "Nearest" is what scopes the check to
  the line being descended from.

**The honest limit of checks 3 and 4:** they catch a target set backwards or skipping, not a typo
that lands one step forwards. `1.10` normalises to `1.10.0` and, from a `1.9.0` reference, is a
legitimate next minor. And they judge a commit by the tags that exist *now*: a development
commit made after a series branch was cut but before the target was bumped configured fine when
it was made, and fails ("already released", or "being released on another branch") when
`git bisect` revisits it after the tags exist. Bumping the target in the same sitting as the cut
keeps that window empty; a bisect that lands in it anyway can pass the version as a `-D` define.

The pure parts of the resolver — splitting, joining, ordering and the target checks — are tested
by `cmake/modules/VersionFromGitTest.cmake`, registered with CTest as `version-resolver-test` in
both build trees, and runnable directly with `cmake -P`. It ends by running the whole resolver
once against the repository it is in, so the git path gets exercised and a mistake in
`VersionRelease.cmake` fails a test as well as the configure. (An earlier version wrote the
expected nearest releases down as literals, `2.5.0` and `1.0.0`, which would have gone red at the
first release after it was written; the resolver finds them itself.)

### 7.3 Worked scenarios

pyGPlates unless stated; `T` is the target in `VersionRelease.cmake`.

| situation | T | nearest tag | result |
|---|---|---|---|
| development branch, 46 first-parent commits past the last release | `1.1.0` | `PyGPlates-1.0.0`, dev 0, distance 46 | `1.1.0.dev46` |
| series branch cut, preparing the first candidate | `1.1.0rc1` | `PyGPlates-1.0.0`, distance 48 | `1.1.0rc1.dev48` |
| standing on the tag `PyGPlates-1.1.0rc1` | `1.1.0rc1` | at HEAD, dev 0, base = T | `1.1.0rc1` |
| development branch, 2 commits after the series branch was cut, target not moved | `1.1.0` | `PyGPlates-1.1.0rc1`, distance 2, off the line | **fatal** — `1.1.0` is being released on another branch; set T to `1.2.0` |
| same, target moved on | `1.2.0` | `PyGPlates-1.1.0rc1`, distance 2 | `1.2.0.dev2` |
| a feature branch cut before the series branch, not yet merged with the development branch | `1.1.0` | `PyGPlates-1.1.0rc1`, off the line | **fatal** — merge the development branch in |
| two commits later, a second candidate decided | `1.1.0rc2` | `PyGPlates-1.1.0rc1`, distance 2 | `1.1.0rc2.dev2` |
| standing on the release tag `PyGPlates-1.1.0` | `1.1.0` | at HEAD, dev 0, base = T | `1.1.0` |
| … but the target was never moved off `1.0.0` | `1.0.0` | at HEAD, base ≠ T | **fatal** — set T to `1.1.0` |
| series branch, 3 commits after the release, target not bumped | `1.1.0` | `PyGPlates-1.1.0`, distance 3, base = T | **fatal** — already released |
| same, after bumping the target for the patch line | `1.1.1` | `PyGPlates-1.1.0`, distance 3 | `1.1.1.dev3` |
| development branch after the release, target bumped | `1.2.0` | `PyGPlates-1.1.0`, distance 5 | `1.2.0.dev5` |
| development branch, target left at `0.9.0` | `0.9.0` | `PyGPlates-1.0.0` | **fatal** — sorts below the release |
| development branch, target set to `1.3.0` | `1.3.0` | `PyGPlates-1.0.0` | **fatal** — skips 1.1 and 1.2 |
| fork standing on its own anchor `GPlates-2.6.0-2000` | `2.6.0` | at HEAD, dev 2000 | `2.6.0-2000` |
| fork, 5 commits past that anchor | `2.6.0` | the anchor, dev 2000, distance 5 | `2.6.0-2005` |
| `git bisect` at a commit below every tag | `1.1.0` | tags HEAD is an ancestor of are skipped | counts from the nearest tag *behind* |

## 8. Three things that look like bugs and are not

Each is surprising the first time a developer meets it.

### 8.1 On the development branch, the count runs from the branch point, not from the tag

A release tag sits on a series branch, so it is never on the development branch's first-parent
line — whether or not the series branch is merged back, since such a merge takes the series branch
as its *second* parent. That sounds like a problem and is not: the count from such a tag equals
the count from the commit the series branch was **cut from**. Everything reachable from the tag,
including the branch point and all history before it, is excluded, and the series branch's own
commits were never on the development line to begin with.

Verified on a synthetic topology (`A0 A1 A2` on main; a series branch cut, with `R1 R2` and the
tag on it; then `M1 M2 M3`, the merge back, and `M4`):

```
main first-parent:  M4  Merge  M3  M2  M1  A2  A1  A0      # the tag's commit is absent
count from the tag           : 5
count from the branch point  : 5
```

and the same equality with the series branch never merged back. So **N is the number of times
the development tip has advanced since that release diverged**, which is a more meaningful
quantity than distance to a tagged commit would have been.

The shape already existed in-tree before the model changed: `PyGPlates-1.0.0rc1` is on
`f4de85708`, a commit of the old `release/pygplates-1.0.0` branch. From the old `pygplates`
branch, `PyGPlates-1.0.0rc1` and `PyGPlates-1.0.0` were both at first-parent distance 46, while
the unrestricted counts were 394 and 350: the commits between the two tags live on the release
branch and cannot contribute. The tie is harmless — both carry development number 0, so the
emitted version is identical — and the guards choose their reference release separately from the
count (section 7.2).

### 8.2 When a series branch gets its first tag, N on the development branch drops

Because of 8.1, the moment the first tag lands on a series branch — `PyGPlates-1.1.0rc1`, or
`PyGPlates-1.1.0` if there was no candidate — the development branch starts counting from a much
later branch point, and N falls from, say, 120 to 3. That is safe only because the release target
must be bumped at the same moment, and the guards make it compulsory in both cases: after a
release, leaving the target on the version just released is exactly what guard 2 refuses; after
a candidate, the off-the-line rule in guard 4 refuses the candidate's head for the same reason.
So the base rises as the counter falls, and the emitted version still moves forward:
`2.6.0-120` → `2.7.0-3`. (Before that rule was line-aware, the candidate case was the opposite:
the bump was *forbidden*, and the development branch re-issued `1.1.0.dev1`, `dev2`, … for new
commits until the release was final — section 7.2.)

### 8.3 The number is monotonic along one first-parent line, not across lines

Measured on 2026-09-11:

| | commit | resolved |
|---|---|---|
| `pygplates` tip | `818761237` | `1.1.0.dev46` |
| `feature/version-from-git`, 3 commits on top | `9de01b089` | `1.1.0.dev49` |
| after that branch merged as one PR merge commit | — | `1.1.0.dev47` |

A feature branch counts its own commits (46 + 3), and the number **drops** when the branch merges,
because the development line gains only the single merge commit (46 + 1). Two feature branches
cut from the same base mint the *same* numbers as each other. A developer *will* notice a pull
request build numbered higher than the merge that follows it.

This is inherent, not a defect, and it does not reintroduce the bug the scheme was built to fix.
That bug was collisions among numbers that became **permanent** on the development line. Here the
development line's numbers remain distinct and increasing; only unmerged, unpublished work can
collide, and nothing publishes from it (`build-wheels.yml` refuses a `.dev` version on a tag).

The same property is why the two old development branches could not simply be merged either way
round (section 11), and why a version string is not a globally unique name for a commit
(section 10).

## 9. Anchor tags and forks

Because the nearest tag wins, a tag placed on a branch takes over the numbering from that point.
A tag with a **non-zero** development number is an *anchor*: it supplies its number and its
position, and nothing else. It is not a release, and the guards ignore it when choosing a
reference release.

**Upstream depends on one right now.** The `gplates` branch's first-parent line runs back through
the 2013 `python-api` branch, and no ancestor of any GPlates release tag newer than that sits on
it, so without `GPlates-2.6.0-47` the count runs from 2013 and gives `2.6.0-1206`. Deleting the
tag would silently restore the larger number. It stops being load-bearing when
`release/gplates-2.6` is cut and `GPlates-2.6.0` tagged there, because that tag is then the
nearest.

**A fork keeps its own numbers with one.** Commits on a fork advance its own first-parent line,
so by default a fork mints the same development version strings as upstream for different code.
Tagging the fork's branch `GPlates-2.6.0-2000` makes that the nearest tag, and the fork counts on
from 2000. Because the base comes from the target, the anchor's own version is just a label and
can name whichever release the fork started from. The advice given to the 17 forks (all with
default branch `gplates`, 2026-09-11) is to do that rather than to change the release version:
editing `GPLATES_RELEASE_VERSION` to `2.7.0` while upstream is on `2.6` produces two different
projects both publishing something called GPlates 2.7.0, and the no-skip guard now refuses the
larger jumps anyway.

## 10. Recipes

**What a checkout resolves to**, without configuring a build:

```
cmake -P cmake/modules/VersionFromGit.cmake gplates
cmake -P cmake/modules/VersionFromGit.cmake pygplates
```

**Which tags are releases.** The [Releases page](https://github.com/GPlates/GPlates/releases) is
the canonical list. On the command line:

```
git -c versionsort.suffix=a -c versionsort.suffix=b -c versionsort.suffix=rc tag --list 'GPlates-*' --sort=version:refname
```

(`PyGPlates-*` for pyGPlates.) The `versionsort.suffix` settings are what sort `PyGPlates-1.0.0rc1`
*before* `PyGPlates-1.0.0` rather than after it. Not every tag listed is a release: **a tag
carrying a development number is an anchor, never a release** — `GPlates-2.6.0-47` sorts last in
that list, precisely where the newest release would be expected — and a few very old tags, such
as `GPlates-1.5+hellinger-testing`, are neither. A release tag is `GPlates-<version>` or
`PyGPlates-<version>` with no development number.

**From a version string back to the commit.** The development number counts first-parent commits
on from the nearest tag, so subtract that tag's own development number and count that far along
the branch:

```
# pyGPlates 1.1.0.dev48, counting from PyGPlates-1.0.0 (development number 0)
git rev-list --first-parent --reverse PyGPlates-1.0.0..gplates | sed -n '48p'

# GPlates 2.6.0-56, counting from the anchor GPlates-2.6.0-47 (development number 47): 56 - 47 = 9
git rev-list --first-parent --reverse GPlates-2.6.0-47..gplates | sed -n '9p'
```

Both return `1b699ffe8`, the unification merge. Count along the branch the build came from — the
development branch, or a release series branch — because of section 8.3: the same string can name
different commits on different lines. Concretely, `1.1.0.dev42` was the `gplates` tip at
`79fa87d8d` on 2026-09-11, and counting 42 along today's `gplates` lands on `255dceb25`, because
the unification made the old `pygplates` line the one `gplates` follows. A version minted before
the unification cannot be found by counting at all.

**So tag any development build that is handed to someone**, and the commit is findable by name.
It costs nothing, verified on a synthetic six-commit repository: before tagging, A5 resolved to
`1.1.0.dev5` and A3 to `1.1.0.dev3`; after tagging A3 as `PyGPlates-1.1.0.dev3`, A3, A4 and A5
still resolved to dev3, dev4 and dev5. A tag whose development number equals the count already in
force is a numerical no-op, deleting it again restores the same numbers, and `build-wheels.yml`
excludes `PyGPlates-*.dev*` from its publish trigger, so such a tag cannot start a release run.
The distinction worth keeping is between a tag that merely *records* a build, which is freely
deletable, and an anchor that *re-bases* the count, which is not while it is load-bearing.

## 11. How the two development branches were unified

Recorded because the merge commit `1b699ffe8` looks odd in the history — a merge that changed no
file, made on the branch that was about to be deleted — and a future reader will wonder why.

The two branches resolved to different development numbers for the same product (section 2), and
**whichever branch's first-parent line survived the merge would determine the numbers
afterwards.** Had `gplates`'s line won, pyGPlates would have gone `.dev47 → .dev43` and GPlates
`-55 → -50`: both counters running backwards, so two different trees could mint the same version
string, the exact failure the derived scheme exists to eliminate. The resolver could not catch it:
`1.1.0.dev43` sorts perfectly well above the last *release*, `1.0.0`; what it sorts below is a
development version already minted on the other branch, and nothing anywhere records that a
development number was ever issued. The scheme derives numbers from history and has no memory of
what it has emitted.

So it was got right by construction. The merge was made **on `pygplates`, with `gplates` as the
second parent**, and pushed to `gplates` as a fast-forward:

```
git checkout pygplates && git merge --no-ff gplates
git push public HEAD:gplates
```

Three details mattered:

- **`--no-ff` was load-bearing.** The sync merge `86cebd62f` had made `pygplates` an ancestor of
  `gplates`, so a plain `git merge gplates` would have fast-forwarded `pygplates` to `79fa87d8d`,
  created no commit, reported "Everything up-to-date" on push, and left the counters continuing
  from `.dev42` / `-49` — the failure above, arrived at by a command that looked like it succeeded.
- **The merge could not go through a pull request.** A PR merge commit takes the base branch as
  its *first* parent, which would have put `gplates`'s line back in front. The fast-forward push
  was the mechanism; the documentation and CI changes went in a pull request on top.
- **The parent order made no difference to the forks.** Either order is a fast-forward for
  `gplates` (its tip is one of the merge's parents either way) and produces the same tree
  (`git merge-tree --write-tree` gave `99f91b38d`, identical to `gplates`'s own tree). The branch
  the forks track was never rewound. The order mattered only to the derived numbers.

The counters were computed from the first-parent distances before the merge rather than guessed
(`PyGPlates-1.0.0..pygplates` was 47 and `GPlates-2.6.0-47..pygplates` was 8, so the merge sits
at 48 and 9) and verified afterwards: `1.1.0.dev48` and `2.6.0-56`. No anchor tags were needed.
The fallback, had the ordering proved awkward, was to keep `gplates`'s line and re-anchor both
counters forward with two anchor tags on the merge commit; it was the fallback rather than the
plan because choosing the numbers by hand is the class of error the scheme removed.

Then the one open pull request based on `pygplates` was retargeted to `gplates` (deleting the
base branch of an open PR closes it), and `pygplates` was deleted from both remotes.

## 12. Follow-up: rename the development branch to `main`

`main` is the right name once there is a single development branch — `develop` was the
alternative, but with `release-gplates` / `release-pygplates` gone there is no "main release
branch" for `main` to collide with, which was the only argument against it.

It is a separate change so the forks get notice first. GitHub redirects web URLs, moves branch
protection rules, retargets open PRs and shows contributors a banner; it does **not** redirect raw
file URLs, does **not** make Actions workflows follow the rename (the five workflow files that
name `gplates` must be edited again), and does **not** redirect `git pull` of the old name. Forks
keep their own branch called `gplates`, and GitHub's "Sync fork" matches branches *by name*, so
afterwards it has nothing to sync against until the fork renames too. The redirect also survives
only while the old name is unoccupied, so a stale clone pushing `gplates` would silently reoccupy
it; a ruleset blocking creation of that name is worth considering.
