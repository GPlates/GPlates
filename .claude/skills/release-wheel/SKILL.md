---
name: release-wheel
description: Walk through publishing a pyGPlates release to PyPI - version bump, release tag, and the approval gate. Invoke as /release-wheel <version>.
disable-model-invocation: true
---

# Publish a pyGPlates release

Target version: `$ARGUMENTS` (a PEP 440 version such as `1.1.0` or `1.1.0rc1`). If none was
given, ask for it before doing anything.

Full reference: @pygplates/wheel/README.md ("Publishing a release").

Publishing is irreversible in one important respect — **PyPI reserves every uploaded filename
forever**, and deleting a release does not free its names. Confirm each step with the user before
committing, tagging, or pushing. Do not push a tag without explicit approval.

## Before starting

- Confirm the working tree is clean and up to date with the GitHub remote. Name that remote
  explicitly rather than assuming `origin` — not every checkout has one.
- Know which branch the release is being cut on. A release is prepared on the permanent release
  series branch `release/pygplates-<major>.<minor>`, cut from `gplates` when the first release in
  that series is prepared, and tagged **there** — release tags belong on the series branches and
  nowhere else (the root `README.md` has the branching model). Candidates and later patch
  releases are tagged on the same branch, each a further commit on it.
- If this is a first pass at a release, suggest an `rc` version (e.g. `1.1.0rc1`). pip ignores
  release candidates by default, so it exercises the whole pipeline — tag check, TestPyPI
  rehearsal, approval gate, real PyPI upload — at low stakes.

## Steps

1. **Set the release target.** Set `PYGPLATES_RELEASE_VERSION` in
   `cmake/modules/VersionRelease.cmake` to the target version, and commit. Development versions
   are counted from git and cannot be released — the run rejects a `.dev` version.
   For the release itself (not a candidate), rename `pyGPlates <X.Y> (unreleased)` in
   `CHANGELOG-pyGPlates.md` to the release version in the same commit. Read the section through
   with the user first: it is the release notes, built up one pull request at a time. After a
   candidate, this commit must change nothing else: the release is the last candidate unchanged,
   and a fix makes another candidate instead (the run refuses a final release that is not its
   last candidate plus this one commit).
   - **When this commit cuts the series branch** (the first release in the series), it also
     freezes GPlates, which is not released from a pyGPlates series branch. First run
     `cmake -P cmake/modules/VersionFromGit.cmake gplates` on the `gplates` commit being cut
     from, then replace the `set(GPLATES_RELEASE_VERSION …)` line with
     `set(GPLATES_FROZEN_VERSION <that version>)`. Without it, the first GPlates release made on
     its own series branch would stop every configure on this one, its release tags included,
     so the run refuses a release tag whose commit does not freeze GPlates.
2. **Tag exactly `PyGPlates-<version>`, on the release commit, then check it.** That is the
   commit on the `release/pygplates-<major>.<minor>` branch being released, whose target is the
   version: a candidate, the release itself or a later patch. Tag it locally first — a local tag
   starts nothing — and check that `cmake -P cmake/modules/VersionFromGit.cmake pygplates` gives
   exactly `<version>`, which is what the run checks in its first minute. Check at the tag, not
   before it: the release after a candidate is refused on an untagged commit, since
   `<version>.devN` would sort below the candidate. Then run
   `python pygplates/wheel/check_release_commit.py <version>`, the run's own check of the freeze
   and (for a release after a candidate) of the candidate being released unchanged. If either
   check disagrees, delete the local tag and fix the commit. Then push the tag to the GitHub
   remote. Ask which remote if there is more than one.
3. **Wait for the build.** `build-wheels.yml` builds the sdist and the full matrix — roughly
   2.5 hours warm, 4.5 cold — then uploads the sdist plus one platform's wheels to TestPyPI as a
   rehearsal.
4. **Review, then approve.** The run sits at *waiting* until a maintainer approves the `pypi`
   deployment (repository page → the run → "Review deployments"). This is the moment to eyeball
   the TestPyPI project page. Approval waits expire after 30 days.
5. **Publish.** On approval the whole matrix uploads to PyPI with PEP 740 attestations. Then
   create the GitHub Release for the tag (`gh release create PyGPlates-<version>`, with
   `--prerelease` for a candidate): the releases page is where the getting-started docs send
   people to find the latest release tag, and nothing creates the entry for them.
6. **Move the targets on.** A tag changes what the next version on each branch is called, and
   the resolver refuses to configure until `VersionRelease.cmake` says so:
   - on the series branch, after the *release*, set the target to the next patch (`1.1.1` after
     `1.1.0`) so later fixes there configure. After a candidate, nothing: the next commit on the
     branch either sets the next candidate (and fixes follow it) or is the release, tagged as
     soon as it is made.
   - on `gplates`, if this was the *first* tag in the series (the first candidate, or the
     release when there was none), set the target to the next minor (`1.2.0`). The development
     branch's count restarts at that moment, so left on `1.1.0` it would re-issue versions it
     has already used. In the same commit, open a `pyGPlates <next minor> (unreleased)` section
     above the series' section in `CHANGELOG-pyGPlates.md`. Later tags in the series need nothing
     on `gplates`, except that once the release (or a patch release) is final its changelog
     section there should match the series branch's.
   Commit each on its own branch. The rules are in `docs/design/versioning/README.md` (7.2).

## Recovery

- **A build failed** → "Re-run failed jobs". Only the failed platform rebuilds, and the publish
  jobs then run as if the run had been green. Works within the 90-day artifact retention window.
- **An upload failed partway** → also "Re-run failed jobs". `skip-existing` skips the files that
  landed and uploads the rest.
- **Something is wrong with the release itself** → do *not* approve. Cancel the run, fix, bump the
  version and re-tag. Stop a suspect release *before* approval, because filenames are permanent.

## Traps

- **Never rename `.github/workflows/build-wheels.yml`.** The Trusted Publishing registrations bind
  to the repository *and the workflow filename*; renaming it silently breaks publishing. The same
  applies to moving the repository.
- **Adding a Python version** requires updating both `[tool.cibuildwheel].build` in
  `pyproject.toml` *and* `PYTHON_VERSIONS` in `pygplates/wheel/versions.sh`, then rebuilding the
  Linux images. The sdist job fails if the two disagree.
- PyPI's default project quota is 10 GiB and a full release is ~1.2 GiB — about seven releases.
  Deleting superseded `rc` releases frees quota (only the filenames stay reserved). TestPyPI has
  the same quota and accretes rehearsals.
