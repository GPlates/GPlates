# Claim-check prompt (stage 2 review)

An independent check of every statement the pilot pull request makes about the code. Launch with
the `Agent` tool, `subagent_type: "general-purpose"`, `model: "fable"` (if Fable is unavailable,
`"opus"`), passing everything below the line as the prompt. Use a fresh agent, not the
`architecture-writer` that wrote the page: the point is a second reader. Run `/code-review high`
separately for `cmake/pygplates_source_closure.py`; this check covers the prose and diagrams.
Afterwards: fix what it finds, re-render any changed diagram (see *Rendering* in the prompt),
commit, and delete this file and `claim-check.md` once their findings are folded in.

---

Task: check, claim by claim, that the architecture documents added on this branch describe the
code correctly. You are a reviewer, not an author: don't rewrite anything, report.

## Where to work

Worktree `C:/gplates/src/architecture-diagrams`, branch `feature/architecture-diagrams`. Its `src/`
is the `gplates` development branch at `553ec966e`; the documents are the commits
`4f2835215..ca0b6d19b` (`git log 553ec966e..HEAD` to see them; `c6da19d58` and earlier are plan
commits and out of scope). Read `AGENTS.md` first for how the repository is laid out.

## What to check

1. `docs/design/architecture/reconstruction/README.md`: the whole page, every section, every box
   and arrow of its four Mermaid diagrams. Its author's evidence is in
   `docs/plans/architecture-diagrams/reconstruction-notes.md`; use it to find code quickly, but
   don't trust it: confirm each claim in the source yourself.
2. `docs/design/architecture/README.md`: the sections *What is here*, *Overview* (the diagram and
   the prose under it), *Areas* (the table) and *The layer groups*. The later sections predate
   this branch; skip them unless something above contradicts them.
3. `docs/design/architecture/scribe-system/README.md`: only the sections this branch added,
   *Components* and *Entry points* (see `git show cd84cb9fa`).
4. `docs/design/architecture/dependency-matrix.md`: only the hand-written prose that the
   generator emits (the paragraphs in `layer_doc_lines` in `cmake/pygplates_source_closure.py`).
   Check that the prose describes what the generator does. The numbers are computed; don't
   recount them.
5. `AGENTS.md`: the *Architecture pages* section.
6. The refactor assessment, `C:/gplates/gplates-planning/plans/area-refactors/reconstruction.md`
   (a separate, private repository; `git pull` it first, and don't write to it). It is judgement, not description: each part of the area is rated *fine*, *touch-up*,
   *restructure* or *rewrite*. Don't argue with a rating as a matter of taste; check that the
   facts each rating rests on are true (the duplication, coupling or brittleness it cites is
   really in the code, where it says), and flag a part of the area the assessment leaves out.
   Also check the other direction: the page (item 1) must not contain refactor proposals or
   ratings, only the weaknesses behind them, stated as facts about the current design.

Things are wrong in more ways than being false. Flag:

- **false**: the code does something else;
- **unconfirmed**: you could not find it in the code (say where you looked);
- **stale-prone**: true today but stated at a level of detail (line numbers, exact counts, a
  private helper's name) that ordinary refactoring will break without anyone noticing;
- **misleading**: literally true, but a reader would come away with the wrong idea, for example
  an arrow whose direction or meaning the legend doesn't explain;
- **missing**: something a reader changing this area would need and the page doesn't say. Keep
  this to things that matter, not a wish list.
- **unsupported** (item 6 only): a rating whose stated reason the code doesn't bear out.

The page describes the code as it is on `gplates`; the plan's rules for pages are in
`docs/plans/architecture-diagrams/PLAN.md` (*Area pages*). A page may state known weaknesses,
but not progress or plans, and must not link to private documents.

## Rendering

If you want to see a diagram, render it with mermaid-cli in Docker (there is no Node on this
machine; Docker Desktop may need starting): extract the block to a `.mmd` file in your scratch
space and run
`MSYS_NO_PATHCONV=1 docker run --rm --entrypoint sh -v "<scratch>:/data" minlag/mermaid-cli -c 'cd /data && mmdc -p /puppeteer-config.json -i x.mmd -o x.png -s 1.3 -b white'`.

## Output

Write one file, `docs/plans/architecture-diagrams/claim-check.md`, and change nothing else. For
each finding: the document and the quoted sentence (or diagram edge), the category, what the code
actually does, with file and line, and a suggested correction in one line. Order findings by
document, most serious first within each. End with a short list of the claims you confirmed that
were hardest to confirm, so the developer knows what was really tested. Don't commit.

Your final message: the number of findings per category, the most serious three, and anything
you could not check.
