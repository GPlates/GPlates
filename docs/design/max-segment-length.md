# Design: maximum segment length while digitising

**Status:** design only — no implementation in this PR.
**Target release:** not yet assigned.
**Related:** Weld Vertices (shares the threshold question), vertex snapping while digitising,
`ReconstructParams::topology_reconstruction_line_tessellation_degrees` (existing precedent).

This document exists so the shape of the feature is agreed before any code is written.

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

## The one decision that shapes everything else

**What happens when a click would exceed the limit?**

Three answers, and they are not variations on a theme — they are different features.

**(a) Reject the click.** The vertex is not added; the status bar says why. Honest and simple. Also
the most irritating option in normal use: the user is told "no" and must click again closer, and
they will hit it constantly when drawing long ocean boundaries where a coarse line is genuinely what
they want.

**(b) Clamp to the maximum.** The vertex is placed at the limit distance along the direction of the
click. Never do this. It puts a vertex somewhere the user did not click, silently, and the resulting
geometry is wrong in a way that looks deliberate.

**(c) Densify.** The clicked vertex is added *and* intermediate vertices are inserted along the arc
so that no segment exceeds the limit. The user gets the point they asked for, and the line follows
the sphere properly.

**(c) is almost certainly right, and it reframes the feature.** This is not a limit on where you may
click. It is **automatic densification while digitising** — the same operation GPlates already
performs at reconstruction time, applied at the point of creation so the stored geometry is good.

That reframing matters for naming, for where the setting belongs, and for scope: once it is
densification rather than restriction, the obvious follow-on is applying it to geometry that already
exists, which is a separate tool and explicitly not this one.

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
- **Insert Vertex** — inserting into an existing long segment; should the insertion also densify the
  two new segments it creates?
- **Move Vertex** — dragging a vertex can lengthen its two adjoining segments past the limit. Enforcing
  there is more intrusive and probably should not be in a first version.

A first version confined to `AddPointGeometryOperation` would be small, and would deliver most of the
value.

## Non-goals

- **Fixing existing geometry.** A tool that densifies an already-drawn feature is useful and is not
  this. Loading a project with long segments should do nothing and say nothing.
- **Replacing reconstruction-time tessellation.** That stays; it serves consumers whose source data
  we do not control.
- **Enforcing a *minimum* separation.** That is Weld Vertices, from the other direction.

## Open questions

1. Reject, clamp, or densify — the doc argues densify, but it is the decision that defines the
   feature and should be made explicitly.
2. Degrees or kilometres, and shared with Weld or not.
3. Does densification produce vertices the user can then edit individually, or should they be marked
   as generated? If a user moves one, the segment invariant breaks — is that allowed?
4. Undo granularity: does one click that inserts five vertices undo as one step? It should.
5. What is a sensible default, and should the feature be off by default? Off is safer; on is more
   useful to the people who need it and will never find the setting.
