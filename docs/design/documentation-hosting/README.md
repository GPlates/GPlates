# Publishing the documentation

Where the documentation built from this repository is published, how it gets there, which of its
addresses are promises, and why. It is a decision record: what was chosen, what was rejected, and
what the choice now commits us to. Today that documentation is the pyGPlates one. The GPlates user
manual is expected to join it (see *Deferred*).

How to build the documentation, and how to publish it again by hand, is in
`docs/pygplates/README.md`. The traps of the mechanism are in comments where they bite: the two
workflows, and the docstrings of `docs/pygplates/publish_docs.py` and `build_docs.py`. This
document points at them rather than repeating them.

## What is published where

GitHub Pages serves the root of the `gh-pages` branch of `GPlates/GPlates` at
<https://gplates.github.io/GPlates/>, over HTTPS, with no custom domain. The branch is generated:
it shares no history with `main` and is not part of the branching model.

```
.nojekyll               without it, Pages runs Jekyll, which drops Sphinx's '_static/'
index.html              redirects to https://www.gplates.org/docs/
404.html                what Pages serves for any missing path on the site
pygplates/
  index.html            redirects to 'stable/', keeping the fragment
  1.0/                  the pyGPlates 1.0 documentation (a copy, see below)
  <X.Y>/                one directory per release series, from 1.1 on
  dev/                  the development version, built from 'main'
  stable                a symbolic link to the newest release series
user-manual/            reserved for the GPlates user manual
```

- **`<X.Y>/` holds the documentation of the newest final release in its series.** A patch release
  replaces it. There is no directory per release, so a link to `1.1/` pins a series, not an exact
  version. Release candidates and development versions are never published there.
- **`dev/` is `main`**, rewritten after each push that CI builds and that passes its tests.
- **`stable` is a symbolic link to a series directory**, which Pages serves as that directory. It
  moves only forwards, and only when a final release of a newer series is published. It was
  created pointing at `1.0`, and stays there until pyGPlates 1.1.0 is released. So `stable/` was a
  working address before any release had been built here: links to it could be published at once
  (the package metadata names it), and the website could start redirecting to it, since it served
  exactly what the website's own copy did.
- **The root is not a landing page.** `www.gplates.org/docs/` is the documentation hub, so the
  site's `index.html` redirects there instead of competing with it. `pygplates/` redirects to
  `stable/`.
- **`404.html`** is a plain page linking to `stable/`, `dev/` and the hub. Without it a mistyped
  address, or the address of a page a later release dropped, ends on GitHub's bare "page not
  found". Its links are absolute, because Pages serves it at whatever path was asked for.

The files outside `pygplates/<version>/` (`.nojekyll`, the two redirect pages, `404.html`, the
`1.0/` copy and the first `stable` link) were committed to `gh-pages` by hand and exist nowhere
else in the repository. Everything after that is written by CI.

A ruleset, *Published documentation*, blocks deleting `gh-pages`, with no bypass
(`docs/design/versioning/README.md`, section 13, lists the rulesets). It is a ruleset of its own
because a ruleset's rules apply to every ref it names: *Permanent branches* also blocks force
pushes, and `gh-pages` has to allow them so that its history can be squashed (see *Sizes*). What
it protects is the `1.0/` copy, which no workflow can rebuild.

## How it gets there

Two workflows publish, each through `docs/pygplates/publish_docs.py`:

- **`build-test-pygplates.yml`** builds the documentation on its Linux leg, from the build tree,
  on pull requests as well as on `main`. On `main` (a push, or a manual run) its `publish-dev-docs`
  job then publishes the result as `dev/`. That job waits for all three platforms, so a push that
  breaks any of them publishes nothing.
- **`publish-docs-pygplates.yml`** runs when a GitHub release is published (or by hand, given a
  tag). For a final `PyGPlates-X.Y.Z` tag it installs that version's wheel from PyPI, builds the
  documentation from it with the pages (`.rst` sources) of the tag, publishes it as `<X.Y>/`, and
  asks for `stable` to follow. Candidates and GPlates releases fire the same event and are skipped.
  A manual run with *publish* unticked builds any release or candidate tag and then does
  everything publishing would except the push, which is how the release path is rehearsed on a
  candidate.

`publish_docs.py` replaces exactly one `pygplates/<target>` directory, moves `stable` if asked to
and if the target is a newer series, commits, and pushes. It refuses a build that is not complete
and from scratch (an incremental Sphinx build has no index entries in its search index), a version
that does not belong to the target series, a candidate or development build in a release
directory, an older release over a newer one, and a release that drops a published page (see
*How published pages may change*). A build identical to what is published makes no commit.

In both workflows publishing is a job of its own, handed the HTML as an artifact, so that it is
the only job whose token can write to the repository: every other job's is read-only. Both publish
only in `GPlates/GPlates`: a fork that tracks `main` would otherwise publish to a `gh-pages` of its
own.

The comments at the top of `publish-docs-pygplates.yml` list the traps of the release path. The
one that shapes the release procedure: a release event runs the workflow file, `publish_docs.py`
and `build_docs.py` *as they are at the release's tag*, so a change to any of them has to be on
the release series branch before the last candidate, and reaches an existing release only through
a manual run.

## Why it is published this way

### One build script, two sources of the module

`build_docs.py` documents either an installed wheel or a build tree, and both workflows (and the
`docs-pygplates` CMake target) use it, so there is one implementation of the build.

- **Releases are documented from the wheel on PyPI**, with the pages from the release's tag. The
  documentation then describes the artifact users install (a build tree at the same tag could
  differ in build flags, Python version or optional dependencies), needs no compiler, and checks
  that the published wheel installs and imports.
- **Not from the publishing run's own wheel artifacts.** That would let the documentation be built
  before the PyPI publish is approved, but it would not check what PyPI serves.
- **Not from TestPyPI.** The upload there is a rehearsal of the metadata (the sdist and one
  platform's wheels), not something to install from. So the release documentation is necessarily
  built after the release is public, and cannot test the wheels before they go out.
- **`dev` comes from CI's build tree.** A development version is never on PyPI, and nightly wheels
  are not worth their machinery for this. The build-and-test job already has a built module, so
  `dev` costs one Sphinx run. Only the Linux leg builds it: the documentation comes from the
  docstrings, which are the same on every platform.

### The release documentation has a workflow of its own

It is started by the GitHub release, which is created by hand after the PyPI publish
(`pygplates/wheel/README.md`), and is not a job in `build-wheels.yml`. A documentation failure
then cannot turn a successful publishing run red, and the documentation can be run again on its
own. It could not ride on `build-test-pygplates.yml` either, which runs for `main` and never for
a tag.

The order matters: a release created before the wheels are on PyPI fails this workflow, once it
has retried for a few minutes, and a manual run then repeats it.

### GitHub Pages, not Read the Docs

Read the Docs would have given `stable` and `latest` addresses, a version menu and pull request
previews for nothing, and needs no compiler when it installs the wheel. But it builds in its own
container, under time limits too short to compile pyGPlates (a cold build takes 30 to 60 minutes
in CI), so it could document released wheels and never `dev`. GitHub Pages takes HTML from either
source. It is also where GPlately's documentation is, and it stays inside the GPlates
organisation.

Two things were given up. There are no previews: a pull request's documentation is built, to
check it, but published nowhere. And there is no version menu (see below).

One further doubt about Read the Docs was never tested, since it was already ruled out: the
release tags are named `PyGPlates-1.1.0`, which is not a version as PEP 440 writes one, so its
choice of `stable` might not have ranked them correctly.

### A `gh-pages` branch, not a deployment from a workflow artifact

A deployment from an artifact replaces the whole site, and the versions have to accumulate. Each
publish of `dev`, or of one release, would have to carry every other version with it, `1.0/`
included, which no workflow can rebuild. On a branch, each publisher replaces its own directory
and leaves the rest. The price is that the branch is in the repository, and grows (see *Sizes*).

The layout is the one GPlately's `gh-pages` uses: a real directory per version, and symbolic
links for the names that move. A link cannot disagree with the directory it names, where a second
copy of the tree would have to be rewritten in step with it.

### A retried push, not a `concurrency` group

A `dev` publish and a release publish can race to push. Each replaces a different directory, so
either order is right, and the only thing to handle is the rejected push: `publish_docs.py`
fetches the new tip, applies its change again and pushes again. A `concurrency` group shared by
the workflows was rejected, and the script's docstring says why. In short, a group protects the
branch only if every workflow that pushes declares it, a release runs the workflow file as it was
at its tag, and by default a group cancels a waiting job when another arrives, so a `dev` publish
could silently cancel a release's. A rejected push is the server's own lock, and holds whatever
the other publisher does.

(`build-test-pygplates.yml` does have a concurrency group, for its own reason: a newer push
cancels an older run's build. That also keeps `dev` publishes in order. The comment on its
*Publish* step covers the one case it does not, a re-run of an older run.)

### This repository, not the website's, and not one of its own

Until 1.1 the documentation was committed to `GPlates/new-gplates-website`, which served it at
`www.gplates.org/docs/pygplates/`.

- **Publishing into the website's repository** would have kept those addresses, which papers
  cite, with nothing to redirect. But it needs a credential for another repository (a personal
  access token or a GitHub App), where publishing here uses the workflow's own token. It commits
  generated HTML to a repository of over 100 MB, and rebuilds the whole website for each update.
  And a `dev` version rewritten on every push to `main` was not something to commit there, so it
  would still have needed a second host and a second mechanism.
- **A repository of its own** (`GPlates/pygplates-docs`, say) would have put its own name in the
  URL in place of `GPlates/`, at the cost of a new repository and the same credential.

Publishing here has one host, no new repository and no credential. Its cost is that every old
address has to be redirected by the website (see *The old URLs*).

### `gplates.github.io/GPlates/pygplates/`, with no `docs.gplates.org`

A `docs.gplates.org` subdomain pointing at this repository's Pages site was offered and declined.

- `www.gplates.org/docs/` is the documentation hub. It lists the user manual and the pyGPlates
  documentation, and already links out to `gplates.github.io` for GPlately and rgplates. The
  pyGPlates documentation on `gplates.github.io` follows that pattern.
- A `docs.gplates.org` root would be a second hub, and could not replace the first: a custom
  domain set on this repository's Pages site covers this site only, so GPlately's and rgplates'
  documentation would stay where they are.
- It can be added later without breaking a link. A custom domain replaces the
  `gplates.github.io/GPlates` part of the address, and GitHub then redirects the old `github.io`
  addresses to it (as `gplates.github.io/gplates-web-service/` redirects to `gwsdoc.gplates.org`).
  So starting without one loses nothing. The address is spelled out in several places that would
  then be worth updating (the canonical URL in the two workflows, the package metadata, the
  website's stubs), but none of them would break.

The `pygplates/` segment is needed because this repository builds two products. Without it
`GPlates/1.1/` would read as GPlates 1.1, which is a real release of the desktop application. The
user manual's segment is to be `user-manual/` rather than `gplates/`, which would read
`GPlates/gplates/`, and which matches the website's `/docs/user-manual/`.

### `dev` and `stable`, but no `latest`

On Read the Docs `latest` means the development branch, which readers easily take for the newest
release. And once two names for one tree are published, neither can be withdrawn without breaking
links. A `latest` link to `dev` can be added at any time; it could not be removed again.

### No version switcher

A version menu was the one feature of Read the Docs with no equivalent here, and it is not a
reason to choose a host. Each version is reached by its address, and the repository's `README.md`
(*Documentation*) is where the versions are listed.

### Every page names its `stable/` page as canonical

The same page is published several times: at `stable/` and in its series' directory (which are one
tree), in each older series, and in `dev/` ahead of its release. Without a canonical URL a search
engine picks one of the copies to show, and may pick an old one, or `dev/` with its unreleased
API. So each page carries `<link rel="canonical">` naming the same path below `stable/`, in the
`dev` build as well as every release build. That is what Read the Docs and the Python
documentation do: every copy points at the current release's page. A `dev` page that has not been
released yet names an address that does not exist until it is, which is expected to be ignored
until then.

The mechanism is Sphinx's `html_baseurl`, which `build_docs.py` takes from the environment
variable `PYGPLATES_DOCS_BASE_URL`; both workflows set it. It is an environment variable rather
than an option so that a workflow can set it while building a tag whose copy of the script
predates it: an unknown variable is ignored, where an unknown option would fail the build. A
local build does not set it, and its pages name no canonical URL.

The `1.0/` copy has no canonical URL and, being a copy, is not edited to add one. While `stable`
points at it, that matters little: the two addresses serve the same pages. Afterwards it is an old
version that does not defer to the current one, which is the case the convention exists for. That
was accepted: very little links to `1.0/` itself, since the old addresses redirect to `stable/`.

## The old URLs

Until 1.1 the documentation was served unversioned at `www.gplates.org/docs/pygplates/`, and
those addresses are in papers, notebooks and other projects' pages. They come in four shapes: the
directory itself, the top-level pages (often with a fragment), `generated/<name>.html` and
`sample-code/<name>.html`. Each keeps working, by redirecting to the same path below `stable/`,
though some forms of them redirect only in a browser (see *What the website does*).

### An unversioned link means the current documentation

The old addresses go to `stable/`, not to `1.0/`. That is the convention of versioned
documentation (`docs.python.org/3/`, Read the Docs' `stable`). Redirecting to `1.0/` could never
break, but it would send every reader of an old news post or answer to frozen pages, without
later corrections and with 1.0's empty search index, when most such links exist to show how to do
something, which the current documentation does better. Whoever means a particular version links
to `<X.Y>/`.

The cost of that choice is the rest of this document's subject: the pages the old addresses
name now have to exist in every future release.

### What the website does

The website is itself a GitHub Pages site (built by Gatsby from `GPlates/new-gplates-website`),
and Pages serves static files only: it cannot answer with an HTTP redirect. Until
GPlates/new-gplates-website#10 it served its own copy of the 1.0 documentation at the old
addresses (the same tree as `1.0/` here). That pull request removed the copy and left three
things in its place:

- **A stub at each of the 129 pages of the 1.0 documentation**, at the page's own path. A stub
  redirects to the same path below `stable/` with a script, which keeps the query and the
  fragment, and falls back to an immediate meta refresh, which keeps neither. It also names the
  new address as canonical.
- **A script on the website's 404 page** (in `gatsby-ssr.js`, which puts it on every page of the
  site, ahead of the inlined CSS in `<head>` so that it runs before most of the page has
  downloaded). Pages serves the 404 page for any missing path, with the address unchanged, and
  the script sends anything below `/docs/pygplates` to the same path below `stable/`, with its
  query and fragment. It catches what the stubs do not: names in the wrong case, and the
  documentation's other files.
- **The 1.0 `objects.inv`, kept at its old address**, so that an intersphinx mapping to the old
  URL still works. The inventory's links are relative, so they land on the stubs.

Why this shape:

- **Stubs, rather than the 404 page alone.** A stub answers 200, and Google treats an immediate
  meta refresh as a permanent redirect ([Redirects and Google Search][google-redirects]), so the
  old address passes its ranking on. A 404 is simply dropped, and a crawler may never run the 404
  page's script: Google may not render a page that answers 404
  ([JavaScript SEO basics][google-javascript]). Tools that run no JavaScript (a link checker,
  `curl`) get a page that answers 200 and names the new address, rather than a 404.
- **Stubs with their real `.html` names.** The website once had stubs without an extension, for
  old lowercase addresses, and removed them: Pages serves a file without an extension as
  `application/octet-stream`, so a browser downloads it instead of following it.
- **A lowercase map for `generated/` only.** Search engines hold older addresses of the reference
  pages that are lowercase and have no extension (`generated/pygplates.rotationmodel`). Pages is
  case-sensitive, so the 404 script looks such a name up in a table
  (`src/data/pygplates-doc-index.json` there) of the 95 `generated/` pages of 1.0. Everything else
  in the old tree is lowercase already, and Pages finds `foo.html` for `foo` by itself, on both
  sites.
- **No generator for that table, and none for the stubs.** Generating the table from
  `build_docs.py` was considered and dropped. Both describe the old addresses only, which exist
  for the pages the website served up to 1.0 and no others: nothing is published there again, so
  a page added later never had an old address. The sets are frozen, and neither can drift.

What remains unredirected: an old address that is not one of the 129 pages (a wrongly cased name,
an image) answers 404 to anything that does not run JavaScript, and redirects only in a browser.
A search engine is likely to drop such an address rather than pass its ranking on. That includes
the lowercase `generated/` names, which a web search index still held in November 2026; they
answered 404 to a crawler before #10 as well.

The stubs and the script are permanent. A search engine that re-crawls an old address finds the
redirect and lists the new address instead, but the old addresses still matter after that.
Papers, notebooks and other sites keep their links, and a search engine passes those links'
weight on to the new pages only while the redirects stay: Google's guidance for a site move is to
keep redirects as long as possible, and at least a year
([Site moves and migrations][google-site-moves]). Removing them would break every link to the old
addresses.

The website has no checks on pull requests. It builds and deploys on each push to its `main`, so
a mistake in the redirect goes live at the merge.

[google-redirects]: https://developers.google.com/search/docs/crawling-indexing/301-redirects
[google-javascript]: https://developers.google.com/search/docs/crawling-indexing/javascript/javascript-seo-basics
[google-site-moves]: https://developers.google.com/search/docs/crawling-indexing/site-move-with-url-changes

### What the website now relies on, for good

The stubs never change, so two things about this repository's documentation are fixed from now on,
both addressed through `stable/`:

- **the 129 page paths of the 1.0 documentation** (8 top-level pages, 95 under `generated/` and
  26 under `sample-code/`), which the stubs and the 404 script redirect to;
- **the targets of the 1.0 inventory**: an intersphinx link made through the old `objects.inv`
  names a page and an anchor as they were in 1.0. Checked against the development documentation
  in November 2026, every one still resolves, the Primer's through its anchor redirect (below).
  The one exception never did: Sphinx lists a module index (`py-modindex.html`) that it does not
  generate for these pages.

A page removed in some later release breaks its stub's target exactly as it breaks a direct link
to `stable/`, and the same rule covers both. The chain is: stub, then the page at the old path
below `stable/`, then wherever that page sends the reader.

The kept `objects.inv` is insurance. No intersphinx mapping to the pyGPlates inventory was found
in the GPlates organisation's repositories (GPlately and plate-model-manager map only to
Python's). A mapping to the old address never sees anything added after 1.0, so a project that
wants one should name `https://gplates.github.io/GPlates/pygplates/stable/`.

New links are `gplates.github.io` ones: whoever copies a link copies it from the site they are
reading. The repository's own links (the package metadata, `README.md`) name `stable/` directly,
and the website's current pages link there too. A redirect is only for links
already published.

## How published pages may change

A page's path below `stable/`, and below each `<X.Y>/`, is a published address. `dev/` promises
nothing.

**A release that removes or renames a page keeps a page at its old path.** That page either
redirects to its replacement, or says when the page was removed and links to the last version
that documented it (`<X.Y>/<page>`). The Primer is the example: it was one page in 1.0 and is
several now, and `pygplates_primer.html` remains, as the Primer's contents page, and redirects a
link to one of its old sections to the page that section moved to.

The rule follows from where links arrive:

- A link to `stable/`, and every old `www.gplates.org` address, follows `stable` to each new
  series. So a series that becomes `stable` must have every page of the series it replaces.
- A link to `<X.Y>/` pins a series, which is how to cite a particular version: link to the series,
  not to `stable/`. So a patch release must keep every page its series already has.

### The dropped-page check

`publish_docs.py` compares the HTML page paths of a build with those already published, and
refuses a build that lacks one.

- **In CI, on every build of the documentation** (`--check-against`, pull requests included), the
  build is compared with the series `stable` points at. The pull request that drops a page fails,
  rather than the release that would have published the gap, where nothing could be done about
  it. In `GPlates/GPlates` the check has to find `gh-pages` to compare with (`--require-ref`), so
  that a change to the checkout cannot turn it off unnoticed; a fork's own run, which normally has
  no `gh-pages`, passes with a note.
- **When a release is published**, as a backstop, it is compared with its own series as
  published, and with the series `stable` points at if it is about to replace it. A rehearsal runs
  the same check (in `GPlates/GPlates` only, since a fork has no `gh-pages` to rehearse against).

**A page can be given up only in the open.** Where a page really cannot be kept at its old path, it
is listed in `GIVEN_UP_PAGES` in `publish_docs.py`, with the reason, and both checks skip it. That
makes giving a page up a change reviewed in the pull request that drops it, rather than an option
someone has to remember to pass when publishing. It is meant to be rare: the rule is to keep a
page.

The check knows pages only. It does not look at anchors within a page, at files other than HTML
pages (images, the `_sources/` copies), or at HTML that Sphinx or the theme keeps in its own
`_`-prefixed directories, which a version of either may add or drop.

**What it cannot see: pages that are new in a series not yet released.** Until a series is
published, CI compares with the previous one, so a pull request on `main` that renames a page new
in the coming series passes. If that pull request is not also in the series' release, the release
publishes the page, `stable` moves to it, and from then on every build of `main` fails the check,
in whichever pull request comes next. So before a series' first final release, its last
candidate's page list is compared with `dev/`'s (a step in the release procedure).

### Anchors

A link with a fragment that no longer exists still arrives, at the top of the right page. Two
sets of old anchors are kept anyway, because links to them are known to be published, and the
build checks both (as warnings, which `-W` makes errors):

- **The old single-page Primer's.** `write_primer_anchors` in `conf.py.in` writes a map from each
  old anchor to the page now holding its section, which a script on `pygplates_primer.html` uses
  to redirect. Anchors made from labels are found from the labels, so the map follows a section
  that moves again. Anchors made from section titles are a frozen list
  (`_OLD_PRIMER_TITLE_ANCHORS`), since a title's anchor is not unique across pages.
- **Fifteen section anchors of the 1.0 documentation** that 1.1 renamed or regrouped, on three
  pages that kept their paths (`_OLD_PAGE_ANCHORS`). Each section now holding the content carries
  a label that gives the old anchor back, and `check_old_anchors` fails the build if a later
  rewrite of those pages drops one. The labels have the anchors' plain names (`feature`,
  `import`, `topologies`), and a label's name is shared by the whole documentation, so no other
  page can define the same one.

**Sphinx's numbered anchors (`#id4`) are not published addresses** and are not kept. A section
whose title repeats an earlier one on the page (*Sample code*, *Details*) gets a number instead
of a name, in order, so adding a sub-example renumbers every one after it. Measured in November
2026: of the 66 numbered section anchors in the 1.0 pages, 42 still exist on the same page in the
development documentation, and at least 28 of those are now on a section with a different heading.

Other anchors are not checked. In particular an API entry's anchor
(`#pygplates.Feature.get_name`) goes when the method goes, and a link to it lands at the top of
the class's page.

## The 1.0 documentation

`1.0/` is a copy of the tree the website served (`static/docs/pygplates/` in
`GPlates/new-gplates-website`, the same git tree), not a rebuild. The `PyGPlates-1.0.0` tag
predates `build_docs.py` and has the documentation in an older layout (`doc-python-api/`), so the
release workflow cannot build it; and the copy is exactly what 1.0's users have been reading.

- Its search index has no index entries: it was published from an incremental build. Search still
  works from page text and object names. `publish_docs.py` now refuses such a build.
- It has no canonical URL (see above).
- No workflow can rewrite it: the release workflow refuses a tag without `build_docs.py`.
- Since the website deleted its copy (GPlates/new-gplates-website#10), this is the only one
  being served, which is why the branch is protected from deletion. The website repository's
  history still holds the tree.

Copying such a tree has a trap on Windows. A checkout there converts every HTML file to CRLF line
endings (`core.autocrlf` is on by default), and a `diff -r` between the copy and that same
checkout still passes. Copy from a checkout with `core.autocrlf` off, and compare git tree hashes
(`git rev-parse <commit>:<path>` on both sides), not files.

## Sizes

`gh-pages` is in the repository, so every full clone downloads it, and so does every CI job that
checks out with `fetch-depth: 0` (the build-and-test and wheel workflows do, for the version
count). Measured on the branch in November 2026:

| | files | size | packed, by itself |
|---|---|---|---|
| `1.0/` | 327 (129 pages) | 17 MB | 6.0 MiB |
| `dev/` | 422 (178 pages) | 19 MB | 6.2 MiB |

- **A new tree costs far less than its own packed size** when the branch already holds a similar
  one: adding the first `dev/` to a branch holding `1.0/` added 0.7 MiB. Over half of a tree's
  bytes are the theme's fonts, which are the same files in both and are stored once. A new release
  series should cost no more.
- **Each `dev` publish adds about 0.1 MiB, at least.** Every page carries the exact development
  version in its title, and that version changes with each merge to `main`, so each publish
  rewrites every page (two lines of each) even when no documentation changed. That is the steady
  growth: some hundreds of publishes add tens of MiB.
- **Publishing the same release again adds nothing.** The one file that records where a build ran,
  Sphinx's `.buildinfo`, never reaches the branch (the artifact upload leaves hidden files out),
  and the rest of a rebuild is identical, so the script finds nothing to commit.

The whole branch packed to 6.9 MiB after its first four commits. If it grows too large, squash its
history and force push: the ruleset allows that, and nothing depends on the branch's history.

Size was not a reason to avoid a branch. What makes a documentation branch large is what its pages
hold: GPlately's `gh-pages` is over 400 MB, with single notebook pages of over 20 MB, where the
largest page here is about 0.5 MB.

## Deferred

- **A redirect page at a `generated/` path.** The rule that a removed or renamed page keeps a page
  at its old path cannot be followed today for the API reference, the pages most likely to go,
  when a class or function is removed or renamed. A top-level, `primer/` or `sample-code/` page
  is an `.rst` file, and its replacement can be another. But `generated/` is written by
  autosummary: `build_docs.py` deletes it before each build and copies only the directories it
  lists, and `conf.py.in` sets no `html_extra_path`, so nothing hand-written can reach
  `generated/` in the output. Candidates are an `html_extra_path` tree of redirect pages, or a
  `build-finished` step that writes them from a table, as `write_primer_anchors` writes its map.
  Nothing has needed it yet (every page of 1.0 is still built). Until it exists, a pull request
  that removes or renames a class or function with a page in the series `stable` points at has to
  give that page up, in `GIVEN_UP_PAGES`, for the dropped-page check to pass.
- **The GPlates user manual, as `user-manual/<version>/`.** The intent is to keep its sources in
  this repository, so that it changes with GPlates, and to publish it beside the pyGPlates
  documentation, with the website's `/docs/user-manual/` redirecting as `/docs/pygplates/` does.
  The layout leaves room for it and nothing else is prepared: `publish_docs.py` writes only below
  `pygplates/`. Each product's publisher should write only its own directory, as now, so that the
  retried push stays sufficient.
- **A custom domain**, and **a `latest` name**: both can be added later, for the reasons above.

Not planned:

- **A 404 page that looks for the missing page in an older version** (by requesting the same path
  under each series, and going to the newest that has it). It would be a safety net for a removed
  page that nobody kept. The page rule and its check should make it unnecessary.
- **Rebuilding the 1.0 documentation** to give it a full search index. It means building from the
  1.0.0 tag's old layout, for pages that are otherwise exactly what was published.
