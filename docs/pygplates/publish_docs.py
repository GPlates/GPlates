#!/usr/bin/env python
"""
Publish a built pyGPlates documentation tree to the 'gh-pages' branch.

GitHub Pages serves 'gh-pages' at https://gplates.github.io/GPlates/. Its layout:

    .nojekyll               without it, Pages runs Jekyll, which drops every '_'-prefixed
                            directory - including Sphinx's '_static/' and '_images/'
    index.html              redirects to https://www.gplates.org/docs/, the documentation hub
    pygplates/
      index.html            redirects to 'stable/'
      1.0/  1.1/  ...       one real directory per release series (a newer patch release replaces
                            it; an older one is refused)
      dev/                  built from 'main' by 'build-test-pygplates.yml', on each push it builds
      stable                a symbolic link to the newest release series
    user-manual/            reserved for the GPlates user manual (not yet published)

This script replaces exactly one of the 'pygplates/<target>' directories, and (optionally) moves
'stable', in a checkout of 'gh-pages', then commits and pushes. Nothing else on the branch is
touched, so the publishers ('dev' from 'main', a release series on each release) never edit the
same files.

They can still race to push, so a rejected push is retried: fetch the new tip, reset to it, apply
the change again, push again. That is deliberately *not* a shared 'concurrency' group in the
workflows. By default a group holds at most one *pending* job, and a newly queued one cancels it,
so a 'dev' publish could silently cancel a release publish that was waiting its turn. A group can
be given 'queue: max', which lets up to 100 wait, but it protects the branch only if every
workflow that pushes declares it, and a release event runs the workflow file as it was at the
release's tag. A rejected push is the server's own lock, and holds whatever the other publisher
does.

Usage:
    publish_docs.py --html <dir> --pages-dir <gh-pages checkout> --target <dev|X.Y>
                    [--update-stable] [--source <text>] [--no-push]

    --html            the Sphinx HTML output (from 'build_docs.py' or the 'docs-pygplates' target)
    --pages-dir       a git checkout of 'gh-pages' whose 'origin' can be pushed to
    --target          'dev', or a release series 'X.Y'
    --update-stable   point 'stable' at the target if it is the newest release series
    --source          what was built, for the commit message (eg, "main at 0123abcd")
    --no-push         commit, but do not push (for trying it out); also allows a release series
                      target to take a candidate or development build

The checkout may be shallow ('actions/checkout' fetches one commit by default).
"""

import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

PUSH_ATTEMPTS = 5


def git(pages_dir, *args, check=True, capture=False, input_bytes=None):
    """Run git in the 'gh-pages' checkout."""
    result = subprocess.run(["git", "-C", str(pages_dir), *args], check=check, input=input_bytes,
                            stdout=subprocess.PIPE if capture else None)
    return result.stdout.decode("utf-8").strip() if capture else result.returncode


def check_html(html_dir):
    """Refuse to publish anything that is not a complete, from-scratch Sphinx build.

    An incremental Sphinx rebuild produces HTML identical to a clean build except for
    'searchindex.js', where it silently drops every index entry. The pyGPlates 1.0 documentation
    was published that way (its 'indexentries' is empty), so check rather than trust.
    Returns the documented version.
    """
    for required in ("index.html", "searchindex.js", "_static/documentation_options.js"):
        if not (html_dir / required).is_file():
            raise SystemExit(f"error: '{html_dir}' is not a Sphinx HTML build (no '{required}').")

    search_index = (html_dir / "searchindex.js").read_text(encoding="utf-8")
    index = json.loads(search_index[search_index.index("(") + 1:search_index.rindex(")")])
    if not index.get("indexentries"):
        raise SystemExit(
            f"error: '{html_dir / 'searchindex.js'}' has no index entries, so it is an\n"
            f"       incremental build. Build from scratch (the default for 'build_docs.py' and\n"
            f"       the 'docs-pygplates' target) before publishing."
        )

    options = (html_dir / "_static" / "documentation_options.js").read_text(encoding="utf-8")
    match = re.search(r"VERSION:\s*'([^']+)'", options)
    if match is None:
        raise SystemExit("error: no VERSION in '_static/documentation_options.js'.")
    return match.group(1)


def series_key(series):
    """'1.10' -> (1, 10), so that series compare numerically rather than as strings."""
    return tuple(int(part) for part in series.split("."))


def final_release_key(version):
    """'1.1.10' -> (1, 1, 10); None for anything but a final release (eg, '1.1.0rc1')."""
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", version)
    return tuple(int(part) for part in match.groups()) if match else None


def published_version(pages_dir, target):
    """Return the version 'pygplates/<target>' documents on gh-pages, or None if there is none."""
    path = f"pygplates/{target}/_static/documentation_options.js"
    if not git(pages_dir, "ls-tree", "HEAD", path, capture=True):
        return None
    options = git(pages_dir, "show", f"HEAD:{path}", capture=True)
    match = re.search(r"VERSION:\s*'([^']+)'", options)
    return match.group(1) if match else None


def current_stable(pages_dir):
    """Return the series 'pygplates/stable' points at, or None if there is no such link.

    Read from git rather than the file system, since a Windows checkout writes a symbolic link
    as a plain file holding the target (unless 'core.symlinks' is enabled).
    """
    entry = git(pages_dir, "ls-tree", "HEAD", "pygplates/stable", capture=True)
    if not entry:
        return None
    mode, _, rest = entry.partition(" ")
    if mode != "120000":
        raise SystemExit(f"error: 'pygplates/stable' on gh-pages is not a symbolic link ({entry}).")
    blob = rest.split()[1]
    return git(pages_dir, "cat-file", "blob", blob, capture=True)


def apply(pages_dir, html_dir, version, target, update_stable):
    """Replace 'pygplates/<target>' with 'html_dir', and move 'stable' if asked to.

    Stages the change and returns a description of it for the commit message.
    """
    if target != "dev":
        # A release series holds its newest release: re-publishing 1.1.0 (by a dispatch, or a
        # re-run within the artifact's retention) must not replace 1.1.1. Checked on every attempt,
        # since a retry starts from a newer tip.
        published = published_version(pages_dir, target)
        new_key = final_release_key(version)
        published_key = final_release_key(published) if published else None
        if new_key and published_key and published_key > new_key:
            raise SystemExit(f"error: pygplates/{target} already documents {published}, which is "
                             f"newer than {version}.")

    target_dir = pages_dir / "pygplates" / target
    if target_dir.exists():
        shutil.rmtree(target_dir)
    shutil.copytree(html_dir, target_dir)
    # Only this directory is staged. 'git add -A' over the whole tree would, on a checkout without
    # symbolic links, turn 'stable' into an ordinary file.
    git(pages_dir, "add", "--all", "--", f"pygplates/{target}")

    if not update_stable:
        return f"pygplates/{target}"

    stable = current_stable(pages_dir)
    if stable is not None and series_key(stable) >= series_key(target):
        # A patch release of an older series (1.1.x after 1.2.0) must not move 'stable' back.
        print(f"'stable' stays at {stable} (not older than {target})")
        return f"pygplates/{target}"

    # Written as a git object, not a file system link, so this also works on Windows.
    blob = git(pages_dir, "hash-object", "-w", "--stdin", capture=True,
               input_bytes=target.encode("utf-8"))
    git(pages_dir, "update-index", "--add", "--cacheinfo", f"120000,{blob},pygplates/stable")
    print(f"'stable' moves from {stable} to {target}")
    return f"pygplates/{target}, and 'stable' to it"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--html", required=True, help="the Sphinx HTML output directory")
    parser.add_argument("--pages-dir", required=True, help="a git checkout of 'gh-pages'")
    parser.add_argument("--target", required=True, help="'dev', or a release series 'X.Y'")
    parser.add_argument("--update-stable", action="store_true",
                        help="point 'stable' at the target if it is the newest release series")
    parser.add_argument("--source", help="what was built, for the commit message")
    parser.add_argument("--no-push", action="store_true", help="commit, but do not push")
    args = parser.parse_args(argv)

    # Keep this script's messages in order with git's (unbuffered) ones when a CI log captures both.
    sys.stdout.reconfigure(line_buffering=True)

    # Also what keeps the target inside 'pygplates/': no '..', no '/', and never 'stable' itself.
    if args.target != "dev" and not re.fullmatch(r"\d+\.\d+", args.target):
        raise SystemExit(f"error: --target must be 'dev' or a release series 'X.Y', not "
                         f"'{args.target}'.")
    if args.update_stable and args.target == "dev":
        raise SystemExit("error: 'stable' is for releases; it cannot point at 'dev'.")

    html_dir = Path(args.html).resolve()
    pages_dir = Path(args.pages_dir).resolve()
    version = check_html(html_dir)
    if args.target != "dev":
        if not version.startswith(f"{args.target}."):
            raise SystemExit(f"error: the HTML documents version {version}, not the "
                             f"{args.target} series.")
        # A release series holds final releases only. A candidate or development build can still
        # be tried out with '--no-push' (which is how the release path is rehearsed on a candidate).
        if not re.fullmatch(r"\d+\.\d+\.\d+", version) and not args.no_push:
            raise SystemExit(f"error: {version} is not a final release, so it cannot be published "
                             f"as the {args.target} series (use --no-push to try it out).")

    subject = f"Publish the pyGPlates {version} docs"
    if args.source:
        subject += f" from {args.source}"

    for attempt in range(1, PUSH_ATTEMPTS + 1):
        what = apply(pages_dir, html_dir, version, args.target, args.update_stable)
        if git(pages_dir, "diff", "--cached", "--quiet", check=False) == 0:
            print("Nothing changed; nothing to publish.")
            return 0
        git(pages_dir, "commit", "--quiet", "-m", subject, "-m", f"Replaces {what}.")
        if args.no_push:
            print(f"Committed, not pushed (--no-push): {subject}")
            return 0
        if git(pages_dir, "push", "origin", "HEAD:refs/heads/gh-pages", check=False) == 0:
            print(f"Published: {subject}")
            return 0

        # Most likely another publisher pushed first. Start again from its tip; the two never
        # change the same directory, so re-applying cannot undo its change.
        print(f"Push rejected (attempt {attempt} of {PUSH_ATTEMPTS}); retrying from the new tip.")
        git(pages_dir, "fetch", "--quiet", "--depth=1", "origin", "gh-pages")
        git(pages_dir, "reset", "--quiet", "--hard", "FETCH_HEAD")

    raise SystemExit(f"error: could not push to gh-pages after {PUSH_ATTEMPTS} attempts.")


if __name__ == "__main__":
    sys.exit(main())
