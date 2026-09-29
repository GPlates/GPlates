# Checks the commit a pyGPlates release tag names, before '.github/workflows/build-wheels.yml'
# publishes it:
#
#  1. GPlates is frozen there. A pyGPlates release is tagged on a pyGPlates release series branch,
#     and the commit that cuts one replaces GPLATES_RELEASE_VERSION with GPLATES_FROZEN_VERSION in
#     'cmake/modules/VersionRelease.cmake'. Nothing else notices if that is forgotten: everything
#     works until GPlates is released on a series branch cut later, and from then on every
#     configure of this branch fails - this release's tag included, which by then cannot be
#     repaired. So it is checked on every release tag, candidates included.
#
#  2. A final release is its last candidate, unchanged. If a candidate for the release
#     ('PyGPlates-1.1.0rc1', 'PyGPlates-1.1.0rc2', ...) is in the history of the commit being
#     released, the latest of them must be that commit's parent, and the commit may change only
#     the PYGPLATES_RELEASE_VERSION line and changelog headings. A candidate exists to be tested,
#     and the release is meant to be what was tested: a fix found during testing makes a new
#     candidate, not a release with the fix folded in. A release with no candidate in its history
#     (a patch release, say) has nothing to compare with.
#
# These are checked here, once, rather than in the version resolver, which runs on every
# configure of the commit for good. A refusal here costs a deleted tag; one in the resolver would
# stop every later build of the commit if the rule ever turned out wrong for it. The resolver's
# own guard on a release - its target only on the commit tagged as it - is about correctness
# instead (the version would sort below the candidate), which is why that one is in the resolver.
# 'docs/design/versioning/README.md' (7.5) has the distinction.
#
# To release something other than the last candidate deliberately, set the repository variable
# PYGPLATES_RELEASE_ALLOW_CHANGES_SINCE_CANDIDATE to 'true' for that run, and remove it after.
# The freeze has no override: a release without it is always a mistake.
#
# Run from the repository root with the resolved pyGPlates version as the only argument, on a
# checkout of the tagged commit with the whole history and all tags; exits non-zero if the release
# is refused.

import os
import re
import subprocess
import sys

RELEASE_FILE = "cmake/modules/VersionRelease.cmake"
CHANGELOG = "CHANGELOG-pyGPlates.md"

OVERRIDE = "PYGPLATES_RELEASE_ALLOW_CHANGES_SINCE_CANDIDATE"


def git(*args, check=True):
    result = subprocess.run(["git", *args], capture_output=True, text=True)
    if check and result.returncode != 0:
        sys.exit(f"error: 'git {' '.join(args)}' failed:\n{result.stderr}")
    return result


def sets(content, name):
    # A 'set(NAME ...)' at a line start, whatever the case of 'set' (CMake's commands are
    # case-insensitive), so that a commented-out copy of the line is not read as the real one.
    return re.search(rf"^[ \t]*set[ \t]*\([ \t]*{name}\b", content, re.MULTILINE | re.IGNORECASE)


def refuse(message):
    if os.environ.get(OVERRIDE, "") == "true":
        print(f"::warning::{message} Allowed by {OVERRIDE}.")
        sys.exit(0)
    print(f"error: {message}", file=sys.stderr)
    print(f"  To release it anyway, set the repository variable {OVERRIDE} to 'true' and "
          "re-run; remove the variable afterwards.", file=sys.stderr)
    sys.exit(1)


version = sys.argv[1]
match = re.fullmatch(r"(\d+\.\d+\.\d+)((a|b|rc)(\d+))?(\.post\d+)?", version)
if not match:
    sys.exit(f"error: '{version}' is not a release version.")

# 1. The freeze.
release_file = open(RELEASE_FILE, encoding="utf-8").read()
frozen = sets(release_file, "GPLATES_FROZEN_VERSION")
if not frozen or sets(release_file, "GPLATES_RELEASE_VERSION"):
    sys.exit(f"error: '{RELEASE_FILE}' does not freeze GPlates. A pyGPlates release series branch "
             "replaces 'set(GPLATES_RELEASE_VERSION ...)' with 'set(GPLATES_FROZEN_VERSION ...)' "
             "in the commit that cuts it (see that file). Without it, the first GPlates release "
             "made on a series branch cut later stops every configure here, this tag included. "
             "Delete the tag, freeze GPlates in a new commit, and tag that.")
print("GPlates is frozen on this branch.")

# 2. A final release is its last candidate, unchanged.
if match.group(2) or match.group(5):
    print(f"'{version}' is not a final release, so there is no candidate to compare it with.")
    sys.exit(0)

rank = {"a": 1, "b": 2, "rc": 3}
candidates = []
for tag in git("tag", "--list", f"PyGPlates-{version}*").stdout.split():
    candidate = re.fullmatch(rf"PyGPlates-{re.escape(version)}(a|b|rc)(\d+)", tag)
    if not candidate:
        continue
    commit = git("rev-list", "-n", "1", tag).stdout.strip()
    if git("merge-base", "--is-ancestor", commit, "HEAD", check=False).returncode == 0:
        candidates.append(((rank[candidate.group(1)], int(candidate.group(2))), tag, commit))

if not candidates:
    print(f"No candidate for '{version}' in this release's history, so nothing to compare with.")
    sys.exit(0)

_, last_tag, last_commit = max(candidates)
parent = git("rev-parse", "HEAD^1").stdout.strip()
if parent != last_commit:
    refuse(f"'{version}' is not one commit on top of its last candidate '{last_tag}': the "
           f"commit being released has parent {parent[:12]}, and the candidate is "
           f"{last_commit[:12]}. Changes made since the candidate belong in a new candidate.")

# The changed lines, file by file. Only the target line and changelog headings may change.
allowed_line = {
    RELEASE_FILE: lambda line: sets(line, "PYGPLATES_RELEASE_VERSION"),
    CHANGELOG: lambda line: line.startswith("#"),
}
unexpected = []
for path in git("diff", "--name-only", parent, "HEAD").stdout.split():
    if path not in allowed_line:
        unexpected.append(path)
        continue
    diff = git("diff", "--no-ext-diff", "--unified=0", parent, "HEAD", "--", path).stdout
    for line in diff.splitlines():
        if line.startswith(("+++", "---")) or not line.startswith(("+", "-")):
            continue
        text = line[1:].rstrip("\r")
        if text.strip() and not allowed_line[path](text):
            unexpected.append(f"{path}: '{text.strip()}'")
if unexpected:
    refuse(f"'{version}' is not its last candidate '{last_tag}' unchanged. The release commit "
           f"may change only the PYGPLATES_RELEASE_VERSION line in {RELEASE_FILE} and headings "
           f"in {CHANGELOG}, but it also changes: " + "; ".join(unexpected) + ". Anything else "
           "belongs in a new candidate.")

print(f"'{version}' is its last candidate '{last_tag}', unchanged.")
