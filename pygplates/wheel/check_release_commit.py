# Checks a pyGPlates release tag, before '.github/workflows/build-wheels.yml' publishes it:
#
#  1. The tag names the version. It must be exactly 'PyGPlates-<version>', point at the commit
#     being checked, and name a release rather than a development version: pip never installs a
#     development version by default, and publishing one would permanently reserve its filenames
#     on PyPI for nothing. The resolver rejects most mismatched tags itself (a tag at HEAD whose
#     base is not the target), but not a tag naming a different version of the same commit.
#
#  2. The version sorts above every earlier release in its history. The resolver's own ordering
#     checks are not consulted on a commit standing on a release tag (there the target must equal
#     the tag, and the tag's own release is the nearest), so a target set backwards together with
#     a tag to match would otherwise be published. Only releases in the tagged commit's history
#     count: a patch release of an older series legitimately sorts below a newer series' releases.
#
#  3. A final release is its last candidate, unchanged. If a candidate for the release
#     ('PyGPlates-1.1.0rc1', 'PyGPlates-1.1.0rc2', ...) is in the history of the commit being
#     released, the latest of them must be that commit's parent, and the commit may change only
#     the PYGPLATES_RELEASE_VERSION line and the changelog's headings. A candidate exists to be
#     tested, and the release is meant to be what was tested: a fix found during testing makes a
#     new candidate, not a release with the fix folded in. A release with no candidate in its
#     history (a patch release, say) has nothing to compare with.
#
# These are checked here, once, rather than in the version resolver, which runs on every
# configure of the commit for good. A refusal here costs a deleted tag; one in the resolver would
# stop every later build of the commit if the rule ever turned out wrong for it - and check 2
# would, since a release tagged later on another branch can be nearer than any in the commit's
# own history. 'docs/design/versioning/README.md' (7.5) has the distinction.
#
# To release something other than the last candidate deliberately, set the repository variable
# PYGPLATES_RELEASE_ALLOW_CHANGES_SINCE_CANDIDATE to 'true' for that run, and remove it after.
# Checks 1 and 2 have no override: a release failing either is always a mistake.
#
# Run from the repository root, on a checkout of the tagged commit with the whole history and all
# tags, as
#
#     python pygplates/wheel/check_release_commit.py <tag> <version>
#
# where <version> is what 'cmake -P cmake/modules/VersionFromGit.cmake pygplates' resolves to
# there. Exits non-zero if the release is refused.

import os
import re
import subprocess
import sys

RELEASE_FILE = "cmake/modules/VersionRelease.cmake"
CHANGELOG = "CHANGELOG-pyGPlates.md"

TAG_PREFIX = "PyGPlates-"

OVERRIDE = "PYGPLATES_RELEASE_ALLOW_CHANGES_SINCE_CANDIDATE"

# A pyGPlates release version: 'X.Y.Z', optionally a candidate ('a', 'b' or 'rc' and a number) or
# a post-release. Old tags have two components ('PyGPlates-0.36'), so the patch is optional here.
RELEASE = re.compile(r"(\d+)\.(\d+)(?:\.(\d+))?(?:(a|b|rc)(\d+))?(?:\.post(\d+))?")

RANK = {"a": 1, "b": 2, "rc": 3, None: 4}


def git(*args, check=True):
    result = subprocess.run(["git", *args], capture_output=True, encoding="utf-8")
    if check and result.returncode != 0:
        sys.exit(f"error: 'git {' '.join(args)}' failed:\n{result.stderr}")
    return result


def release_key(version):
    """A sort key for a release version (PEP 440 order), or None if it is not one."""
    match = RELEASE.fullmatch(version)
    if not match:
        return None
    major, minor, patch, kind, n, post = match.groups()
    return ((int(major), int(minor), int(patch or 0)), RANK[kind], int(n or 0), int(post or 0))


def sets(content, name):
    # A 'set(NAME ...)' at a line start, whatever the case of 'set' (CMake's commands are
    # case-insensitive), so that a commented-out copy of the line is not read as the real one.
    return re.search(rf"^[ \t]*set[ \t]*\([ \t]*{name}\b", content, re.MULTILINE | re.IGNORECASE)


def without_headings(markdown):
    """The lines of a Markdown file with its headings removed, in either style: '#' lines, and
    Setext headings (a line of text underlined by a line of '=' or '-')."""
    lines = [line.rstrip("\r") for line in markdown.split("\n")]
    body = []
    i = 0
    while i < len(lines):
        if lines[i].startswith("#"):
            i += 1
        elif (lines[i].strip() and i + 1 < len(lines)
                and re.fullmatch(r"=+|-+", lines[i + 1].strip())):
            i += 2
        else:
            body.append(lines[i])
            i += 1
    return body


def refuse(message, allow_override=False):
    if allow_override and os.environ.get(OVERRIDE, "") == "true":
        print(f"::warning::{message} Allowed by {OVERRIDE}.")
        sys.exit(0)
    print(f"error: {message}", file=sys.stderr)
    if allow_override:
        print(f"  To release it anyway, set the repository variable {OVERRIDE} to 'true' and "
              "re-run; remove the variable afterwards.", file=sys.stderr)
    sys.exit(1)


if len(sys.argv) != 3:
    sys.exit("usage: check_release_commit.py <tag> <version>")
tag, version = sys.argv[1:]

# 1. The tag names the version.
if release_key(version) is None or not re.fullmatch(r"\d+\.\d+\.\d+.*", version):
    refuse(f"'{version}' is not a release version (a development version is counted from git, "
           f"and cannot be released). Set the release target in '{RELEASE_FILE}', commit, and "
           "tag that commit.")
if tag != f"{TAG_PREFIX}{version}":
    refuse(f"The tag '{tag}' does not match the version '{version}' resolved at it (from "
           f"'{RELEASE_FILE}' and the repository's tags). Tag the release '{TAG_PREFIX}{version}', "
           "or set the release target to the version the tag names.")
head = git("rev-parse", "HEAD").stdout.strip()
if git("rev-list", "-n", "1", tag).stdout.strip() != head:
    refuse(f"The tag '{tag}' does not point at the commit checked out here ({head[:12]}). Check "
           "out the tag and run this again.")
print(f"'{tag}' names the release '{version}'.")

# 2. Above every earlier release in its history. 'git tag --merged' lists the tags whose commits
# HEAD can reach, which includes the ones on HEAD itself.
at_head = set(git("tag", "--points-at", "HEAD", "--list", f"{TAG_PREFIX}*").stdout.split())
for earlier in git("tag", "--merged", "HEAD", "--list", f"{TAG_PREFIX}*").stdout.split():
    if earlier in at_head:
        continue
    key = release_key(earlier[len(TAG_PREFIX):])
    if key is not None and key >= release_key(version):
        refuse(f"'{version}' does not sort above '{earlier}', which is already in its history. "
               f"Set the release target in '{RELEASE_FILE}' to the next release, and tag that.")
print(f"'{version}' sorts above every earlier release in its history.")

# 3. A final release is its last candidate, unchanged.
key = release_key(version)
if key[1] != RANK[None] or key[3] != 0:
    print(f"'{version}' is not a final release, so there is no candidate to compare it with.")
    sys.exit(0)

candidates = []
for candidate_tag in git("tag", "--list", f"{TAG_PREFIX}{version}*").stdout.split():
    candidate = re.fullmatch(rf"{TAG_PREFIX}{re.escape(version)}(a|b|rc)(\d+)", candidate_tag)
    if not candidate:
        continue
    commit = git("rev-list", "-n", "1", candidate_tag).stdout.strip()
    if git("merge-base", "--is-ancestor", commit, "HEAD", check=False).returncode == 0:
        candidates.append(((RANK[candidate.group(1)], int(candidate.group(2))), candidate_tag,
                           commit))

if not candidates:
    print(f"No candidate for '{version}' in this release's history, so nothing to compare with.")
    sys.exit(0)

_, last_tag, last_commit = max(candidates)
parent = git("rev-parse", "HEAD^1").stdout.strip()
if parent != last_commit:
    refuse(f"'{version}' is not one commit on top of its last candidate '{last_tag}': the "
           f"commit being released has parent {parent[:12]}, and the candidate is "
           f"{last_commit[:12]}. Changes made since the candidate belong in a new candidate.",
           allow_override=True)

# The changes, file by file: only the target line, and only the changelog's headings.
unexpected = []
for path in git("diff", "--name-only", parent, "HEAD").stdout.split():
    if path == RELEASE_FILE:
        diff = git("diff", "--no-ext-diff", "--unified=0", parent, "HEAD", "--", path).stdout
        for line in diff.splitlines():
            if line.startswith(("+++", "---")) or not line.startswith(("+", "-")):
                continue
            text = line[1:].rstrip("\r")
            if text.strip() and not sets(text, "PYGPLATES_RELEASE_VERSION"):
                unexpected.append(f"{path}: '{text.strip()}'")
    elif path == CHANGELOG:
        before = git("show", f"{parent}:{path}").stdout
        after = git("show", f"HEAD:{path}").stdout
        if without_headings(before) != without_headings(after):
            unexpected.append(f"{path}, other than its headings")
    else:
        unexpected.append(path)
if unexpected:
    refuse(f"'{version}' is not its last candidate '{last_tag}' unchanged. The release commit "
           f"may change only the PYGPLATES_RELEASE_VERSION line in {RELEASE_FILE} and headings "
           f"in {CHANGELOG}, but it also changes: " + "; ".join(unexpected) + ". Anything else "
           "belongs in a new candidate.", allow_override=True)

print(f"'{version}' is its last candidate '{last_tag}', unchanged.")
