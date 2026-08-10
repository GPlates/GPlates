# Design: maximum segment length while digitising

**Status:** implemented, in a shape this document did not anticipate — see **Outcome** below before
relying on anything here.
**Shipped in:** 2.6.0-dev8-SR4, on `gplates` as the Shift-clamp.
**Related:** Weld Vertices (shares the threshold question), vertex snapping while digitising,
`ReconstructParams::topology_reconstruction_line_tessellation_degrees` (existing precedent).

This document exists so the shape of the feature is agreed before any code is written. It is kept
after the fact because the reasoning below is what the shipped feature was argued from, including
the parts the outcome overturned.

---

## Outcome

The recommendation below — **(a) reject or warn**, delivered as an advisory — was built and
rejected. A status-bar message that appears after the click has already landed turned out to be the
wrong shape of advisory: passive, easy to miss, and arriving too late to change the decision it was
commenting on. It was reverted rather than shipped.

What shipped instead is a fourth option this document did not consider: **clamping, but only when
the user asks for it.** Holding **Shift** while placing a vertex puts it no further from the
previous vertex than the project's resolution, measured along the great circle towards the click. A
plain click is untouched.

That is not a reversal of the verdict on **(b)** below, and the wording there should be read with
this in mind. The objection to clamping was that it puts a vertex somewhere the user did not click,
*silently*, in a way that looks deliberate. Every word of that stands. Shift removes the only thing
that made it wrong: the clamp becomes the thing the user asked for rather than something done
behind their back, and the unmodified click remains available at all times. The principle survived;
it was the assumption that clamping could only be unconditional that did not.

The digitisation tools also turn out to be the one place where Shift is free. It means "add to the
selection" elsewhere in GPlates, but these tools place points rather than select anything, so there
is no selection for Shift to extend — `DigitiseGeometry` had never overridden `handle_shift_left_click`
at all.

### What this settles, and what it does not

- **Units: kilometres**, from `gplates.resolution.default_km` in the Primary Project Document,
  falling back to 500 km. So the threshold travels with the project and means the same thing on a
  planet of any size, which the degrees option could not manage.
- **Where the setting lives: a project setting.** As predicted below, that followed from the units
  decision.
- **Per-segment, as drawn** — open question 3.
- **Off unless asked for** — open question 5, answered by the Shift gate rather than by a preference.
- **Still open:** whether Weld Vertices and vertex snapping share this threshold. The argument below
  that all three should draw on one concept is unaffected by anything that has shipped, and
  `resolution` is now the obvious candidate for it.
- **Not taken:** the advice that this should follow Weld rather than precede it. It preceded it.
  Whether that costs anything will show up when Weld needs a threshold of its own.

Per-feature-type overrides (`resolution.by_feature_type`) exist but deliberately do not apply while
digitising: a geometry being drawn has no feature type yet, since the user chooses that when the
feature is created.

---

## The problem

Two consecutive vertices are joined by a **great-circle arc**. That arc is the shortest path on the
sphere between them, and for a long segment it is not the path the user drew — it bows away from the
line they sketched on screen, by an amount that grows with segment length and with distance from the
projection's centre. A coastline clicked out in half a dozen points is not the coastline that was
intended.

It gets worse under reconstruction. Every vertex is rotated by its own plate's finite rotation, and
the arc between two vertices is regenerated from the rotated endpoints. A long segment therefore
does not deform the way the real boundary would — it stays a great circle joining two points that
have moved, which is not the same shape.

**GPlates already agrees with this.** `ReconstructParams` carries
`topology_reconstruction_enable_line_tessellation` and
`topology_reconstruction_line_tessellation_degrees`: when topologies are reconstructed through time,
long lines are broken into shorter pieces first, precisely so they deform sensibly. That machinery
exists because coarse geometry reconstructs badly.

What is missing is anything preventing the geometry being coarse **in the first place**. Tessellation
at reconstruction time patches over the symptom for one consumer. The stored geometry is still
coarse, and every other consumer — Weld Vertices, Split Plate, Boolean Polygons, anything working to
a distance tolerance — inherits the problem.

There is also a plainer, more common annoyance: a stray click far from where you meant creates an
enormous segment. It is easy not to notice and irritating to find later.

## What the feature is actually for

The purpose is **drawing discipline**, not geometric fidelity.

The problem being solved is a human one: it is easy, while digitising, to lay down a single enormous
segment across a region that in reality has structure — and then not notice. Real coastlines,
rifts and subduction zones are not smooth. A boundary drawn as three points across an ocean is not
"a bit coarse", it is a different boundary, and the detail it is missing is detail only the person
drawing it can supply.

So the feature exists to **stop the user going too fast**, and prompt them to put the variation in
themselves.

This distinction decides everything below, and it rules out the option that first looks most
attractive.

## The one decision that shapes everything else

**What happens when a click would exceed the limit?**

Three answers, and they are not variations on a theme — they are different features.

**(a) Reject the click, or warn.** The vertex is not added, or is added with a visible complaint; the
status bar says why. The user clicks again, closer, and the line acquires the detail they would
otherwise have skipped.

**(b) Clamp to the maximum.** The vertex is placed at the limit distance along the direction of the
click. Never do this. It puts a vertex somewhere the user did not click, silently, and the resulting
geometry is wrong in a way that looks deliberate.

> Read "silently" as load-bearing here. This is what shipped, gated behind Shift so that it is not
> silent — see **Outcome**. As an unconditional response to a plain click, the verdict stands.

**(c) Densify.** The clicked vertex is added *and* intermediate vertices are inserted along the arc
so that no segment exceeds the limit.

**(a) is right, and (c) is a trap.**

Densification inserts vertices **along the existing great-circle arc**. The result has more points
and *exactly the same shape* — a smooth arc, now finely sampled. It adds no variation, because there
is no new information to add: the software does not know where the coastline should bend, and
inventing plausible-looking wiggle would be worse than leaving it straight.

Densifying therefore satisfies a segment-length rule while defeating the entire purpose of having
one. Worse, it does so invisibly — the user is told nothing, the vertex count goes up, and the line
still lacks the structure they meant to draw. It would make the problem *harder* to notice, not
easier.

Densification is genuinely useful for reconstruction fidelity — which is exactly why GPlates already
does it at reconstruction time, where a machine-generated arc is all that is wanted. It is the wrong
answer here.

## Reject or warn?

Given (a), a second question: hard block, or advisory?

- **Hard block** guarantees the invariant, and will be infuriating the first time someone legitimately
  wants a long straight segment — a plate boundary that really is a straight transform, a quick
  scaffold line, a topology section.
- **Advisory** — draw the offending segment in a warning colour, or say so in the status bar, and let
  the user decide — preserves judgement and still catches the case this exists for, which is *not
  noticing*.

Advisory is probably right, since the failure being prevented is inattention rather than intent. A
user who deliberately wants one long segment should not have to go and find a setting to turn off.

> This is the recommendation that was built and rejected. The reasoning in the second half held —
> the user who wants a long segment does not have to turn anything off — but it was delivered by
> making the limit opt-in rather than by making the warning ignorable. See **Outcome**.

## Units, and why this should not be decided alone

Two candidates:

- **Arc-degrees.** The existing precedent — `line_tessellation_degrees` is in degrees. Independent of
  planet size, which is either a virtue or a defect depending on your point of view.
- **Kilometres**, via the project's planetary radius. More meaningful to a worldbuilder, who thinks
  in "no segment longer than 200 km", and now possible since the fork has `PlanetaryParameters`. On a
  planet twice Earth's radius, one degree is twice as far, so a degree-based threshold silently
  means something different per project.

**Weld Vertices carries exactly the same open question**, and vertex snapping while digitising will
carry it a third time. Three tools with three separately-invented distance thresholds, in possibly
different units, would be a poor outcome. Whatever is decided should be decided once and shared —
ideally a single "geometry tolerance" concept these tools all draw on.

This is the strongest argument for not implementing this feature first. It should follow Weld, not
precede it.

## Where the setting lives

- **A tool option**, in the Task Panel beside the digitisation tools, is the most discoverable and
  the easiest to turn off for one line.
- **A preference** is right if the value is a property of how someone works rather than of the
  current task.
- **A project setting** is right if the value depends on planet size and map scale — which it does,
  if the units are kilometres.

Probably a preference for the default plus a tool-level toggle to suspend it, but this depends
entirely on the units decision above.

## Which tools

- **Digitise New Polyline / Polygon** — the obvious case, via `AddPointGeometryOperation`.
- **Insert Vertex** — the natural way to fix a segment the warning flagged, so it should not fight the
  user while they are doing exactly what they were asked to do.
- **Move Vertex** — dragging a vertex can lengthen its two adjoining segments past the limit. Enforcing
  there is more intrusive and probably should not be in a first version.

A first version confined to `AddPointGeometryOperation` would be small, and would deliver most of the
value.

## Non-goals

- **Fixing existing geometry.** Loading a project with long segments should do nothing and say
  nothing. Auditing a whole project for coarse boundaries is a reasonable idea and is not this one.
- **Replacing reconstruction-time tessellation.** That stays; it serves consumers whose source data
  we do not control.
- **Enforcing a *minimum* separation.** That is Weld Vertices, from the other direction.

## Open questions

Questions 1, 2, 3 and 5 have since been answered, several of them differently from the expectation
recorded here; **Outcome** has the resolutions. Question 4 and the shared-threshold half of
question 2 are genuinely still open.

1. Hard block or advisory warning. The doc argues advisory, since the failure being prevented is
   inattention rather than intent.
2. Degrees or kilometres, and shared with Weld or not.
3. Does the check apply per-segment as it is drawn, or to the finished geometry on completion?
   Per-segment catches it while the user is still thinking about that part of the line.
4. Should an existing feature be checked when opened for editing, or only newly drawn segments?
5. What is a sensible default, and should the feature be off by default? Off is safer; on is more
   useful to the people who need it and will never find the setting.
