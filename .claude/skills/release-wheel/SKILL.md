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

- Confirm the working tree is clean and on the `pygplates` branch, up to date with the GitHub
  remote. Name that remote explicitly rather than assuming `origin` — not every checkout has one.
- If this is a first pass at a release, suggest an `rc` version (e.g. `1.1.0rc1`). pip ignores
  release candidates by default, so it exercises the whole pipeline — tag check, TestPyPI
  rehearsal, approval gate, real PyPI upload — at low stakes.

## Steps

1. **Bump the version.** Set `PYGPLATES_PEP440_VERSION` in `cmake/modules/Version.cmake` to the
   target version and commit. A `.dev` version cannot be released — the run rejects it.
2. **Tag exactly `PyGPlates-<version>`.** The tag's version string must match
   `PYGPLATES_PEP440_VERSION` character for character; the run fails in its first minute if they
   disagree. Push the tag to the GitHub remote. Ask which remote if there is more than one.
3. **Wait for the build.** `build-wheels.yml` builds the sdist and the full matrix — roughly
   2.5 hours warm, 4.5 cold — then uploads the sdist plus one platform's wheels to TestPyPI as a
   rehearsal.
4. **Review, then approve.** The run sits at *waiting* until a maintainer approves the `pypi`
   deployment (repository page → the run → "Review deployments"). This is the moment to eyeball
   the TestPyPI project page. Approval waits expire after 30 days.
5. **Publish.** On approval the whole matrix uploads to PyPI with PEP 740 attestations.

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
