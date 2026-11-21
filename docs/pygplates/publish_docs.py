#!/usr/bin/env python
"""
Publish a built pyGPlates documentation tree to the 'gh-pages' branch.

GitHub Pages serves 'gh-pages' at https://gplates.github.io/GPlates/. Its layout:

    .nojekyll               without it, Pages runs Jekyll, which drops every '_'-prefixed
                            directory - including Sphinx's '_static/' and '_images/'
    index.html              redirects to https://www.gplates.org/docs/, the documentation hub
    404.html                served by Pages for any missing path on the site; it links back to
                            'stable/', 'dev/' and the hub
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

No published page is dropped. Links to 'stable/' (and every old www.gplates.org/docs/pygplates/
link, which redirects there) follow it to each new release, so a newer series that becomes
'stable' must still have every page of the series 'stable' points at. And a link to '<X.Y>/' pins
that series, so a patch release must keep every page its series already has. A page that a
release removes or renames keeps a page at its old path instead
('docs/design/documentation-hosting/README.md'). '--check-against' compares a build with
the published pages without publishing anything: CI runs it on every 'dev' build, so that the pull
request that drops a page fails, rather than the release. Publishing a release checks again, as a
backstop. A page given up deliberately, where no page can be kept at its path, is listed with its
reason in 'GIVEN_UP_PAGES' below, which both checks honour: giving a page up is then a change
reviewed in the pull request that does it.

Usage:
    publish_docs.py --html <dir> --pages-dir <gh-pages checkout> --target <dev|X.Y>
                    [--update-stable] [--source <text>] [--no-push]
    publish_docs.py --html <dir> --check-against <gh-pages ref> [--require-ref]

    --html            the Sphinx HTML output (from 'build_docs.py' or the 'docs-pygplates' target)
    --pages-dir       a git checkout of 'gh-pages' whose 'origin' can be pushed to
    --target          'dev', or a release series 'X.Y'
    --update-stable   point 'stable' at the target if it is the newest release series
    --source          what was built, for the commit message (eg, "main at 0123abcd")
    --no-push         commit, but do not push (for trying it out); also allows a release series
                      target to take a candidate or development build
    --check-against   only compare the HTML with the pages of the series 'stable' points at in
                      this ref of 'gh-pages', in the repository of the current directory (eg,
                      'origin/gh-pages'), and fail if any is missing; a ref that doesn't exist (a
                      fork with no 'gh-pages', say) is noted and passes, unless --require-ref
    --require-ref     with --check-against, fail instead when the ref, its 'stable', or any page
                      of it cannot be found (CI passes it in GPlates/GPlates, so that the check
                      cannot be lost unnoticed, eg by a change to the checkout)

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

# Published pages given up deliberately: page path (relative to a series directory, eg
# 'generated/pygplates.Foo.html') -> why. Only for a page that cannot keep a page at its old path
# (see the module docstring); the dropped-page checks skip these. Empty: no page has been given up.
GIVEN_UP_PAGES = {
}


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
    if not git(pages_dir, "ls-tree", "--full-tree", "HEAD", path, capture=True):
        return None
    options = git(pages_dir, "show", f"HEAD:{path}", capture=True)
    match = re.search(r"VERSION:\s*'([^']+)'", options)
    return match.group(1) if match else None


def current_stable(pages_dir, ref="HEAD"):
    """Return the series 'pygplates/stable' points at, or None if there is no such link.

    Read from git rather than the file system, since a Windows checkout writes a symbolic link
    as a plain file holding the target (unless 'core.symlinks' is enabled).
    """
    entry = git(pages_dir, "ls-tree", "--full-tree", ref, "pygplates/stable", capture=True)
    if not entry:
        return None
    mode, _, rest = entry.partition(" ")
    if mode != "120000":
        raise SystemExit(f"error: 'pygplates/stable' on gh-pages is not a symbolic link ({entry}).")
    blob = rest.split()[1]
    return git(pages_dir, "cat-file", "blob", blob, capture=True)


def is_page(path):
    """Whether 'path' (relative to a documentation tree) is one of its pages.

    Not an HTML file under a '_' directory ('_static/', '_sources/', ...): those belong to Sphinx
    and the theme, which may add or drop one between versions.
    """
    return path.endswith(".html") and not path.startswith("_")


def html_pages(html_dir):
    """Return the path of every page in a built documentation tree, relative to it."""
    paths = (page.relative_to(html_dir).as_posix() for page in html_dir.rglob("*.html"))
    return {path for path in paths if is_page(path)}


def published_pages(pages_dir, ref, series):
    """Return the path of every page of 'pygplates/<series>' at 'ref', relative to it."""
    prefix = f"pygplates/{series}/"
    listing = git(pages_dir, "ls-tree", "-r", "--full-tree", "--name-only", ref, "--", prefix,
                  capture=True)
    return {path[len(prefix):] for path in listing.splitlines() if is_page(path[len(prefix):])}


def check_no_dropped_pages(old_pages, new_pages, old_series, what, links_through):
    """Refuse 'what' if it lacks any of 'old_pages', the pages published in 'old_series'.

    'links_through' says which published links would break ('stable/', or the series' own).
    """
    dropped = sorted(old_pages - new_pages - GIVEN_UP_PAGES.keys())
    if dropped:
        raise SystemExit(
            f"error: {what} lacks {len(dropped)} of the {len(old_pages)} pages published in "
            f"pygplates/{old_series}:\n         " + "\n         ".join(dropped) + "\n"
            f"       Links to them through {links_through}\n"
            f"       would break. A page that is removed or renamed keeps a page at its old path\n"
            f"       that redirects ('docs/design/documentation-hosting/README.md'), or, if that\n"
            f"       cannot be done, is listed in 'GIVEN_UP_PAGES' in\n"
            f"       'docs/pygplates/publish_docs.py'.")


def apply(pages_dir, html_dir, version, target, update_stable):
    """Replace 'pygplates/<target>' with 'html_dir', and move 'stable' if asked to.

    Stages the change and returns a description of it for the commit message.
    """
    stable = current_stable(pages_dir)
    if target != "dev":
        # These are all checked on every attempt, since a retry starts from a newer tip. First, a
        # release series holds its newest release: re-publishing 1.1.0 (by a dispatch, or a re-run
        # within the artifact's retention) must not replace 1.1.1.
        published = published_version(pages_dir, target)
        new_key = final_release_key(version)
        published_key = final_release_key(published) if published else None
        if new_key and published_key and published_key > new_key:
            raise SystemExit(f"error: pygplates/{target} already documents {published}, which is "
                             f"newer than {version}.")

        # Then a release keeps the pages its series already has, since a link to '<X.Y>/' pins it;
        # and a newer series about to become 'stable' keeps the pages of the one 'stable' points
        # at, since links through 'stable/' follow it.
        new_pages = html_pages(html_dir)
        check_no_dropped_pages(published_pages(pages_dir, "HEAD", target), new_pages, target,
                               f"pyGPlates {version}", f"'{target}/'")
        if update_stable and stable is not None and series_key(target) > series_key(stable):
            check_no_dropped_pages(published_pages(pages_dir, "HEAD", stable), new_pages, stable,
                                   f"pyGPlates {version}",
                                   "'stable/' (and the old www.gplates.org URLs)")

    target_dir = pages_dir / "pygplates" / target
    if target_dir.exists():
        shutil.rmtree(target_dir)
    shutil.copytree(html_dir, target_dir)
    # Only this directory is staged. 'git add -A' over the whole tree would, on a checkout without
    # symbolic links, turn 'stable' into an ordinary file.
    git(pages_dir, "add", "--all", "--", f"pygplates/{target}")

    if not update_stable:
        return f"pygplates/{target}"

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
    parser.add_argument("--pages-dir", help="a git checkout of 'gh-pages'")
    parser.add_argument("--target", help="'dev', or a release series 'X.Y'")
    parser.add_argument("--update-stable", action="store_true",
                        help="point 'stable' at the target if it is the newest release series")
    parser.add_argument("--source", help="what was built, for the commit message")
    parser.add_argument("--no-push", action="store_true", help="commit, but do not push")
    parser.add_argument("--check-against", metavar="REF",
                        help="only check the HTML against the 'stable' pages of this gh-pages ref")
    parser.add_argument("--require-ref", action="store_true",
                        help="with --check-against, fail if there is nothing to compare with")
    args = parser.parse_args(argv)

    # Keep this script's messages in order with git's (unbuffered) ones when a CI log captures both.
    sys.stdout.reconfigure(line_buffering=True)

    if args.check_against:
        return check_against(Path(args.html).resolve(), args.check_against, args.require_ref)
    if not args.pages_dir or not args.target:
        parser.error("--pages-dir and --target are required, unless --check-against is given")

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


def check_against(html_dir, ref, require_ref):
    """Check a build against the pages of the series 'stable' points at in 'ref' (of gh-pages)."""
    version = check_html(html_dir)
    repo_dir = Path.cwd()

    def nothing_to_compare(reason):
        if require_ref:
            raise SystemExit(f"error: {reason}, so this build cannot be checked (--require-ref).")
        print(f"note: {reason}, so there are no published pages to compare with.")
        return 0

    if not git(repo_dir, "rev-parse", "--verify", "--quiet", f"{ref}^{{commit}}", check=False,
               capture=True):
        return nothing_to_compare(f"no '{ref}' in the repository of {repo_dir}")
    stable = current_stable(repo_dir, ref)
    if stable is None:
        return nothing_to_compare(f"'{ref}' has no 'pygplates/stable'")
    old_pages = published_pages(repo_dir, ref, stable)
    if not old_pages:
        return nothing_to_compare(f"'{ref}' has no pages in 'pygplates/{stable}'")
    check_no_dropped_pages(old_pages, html_pages(html_dir), stable, f"This build ({version})",
                           "'stable/' (and the old www.gplates.org URLs), once it is released,")
    print(f"This build ({version}) has all {len(old_pages)} pages published in pygplates/{stable}, "
          f"the series 'stable' points at in {ref}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
