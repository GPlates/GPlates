# Project Markdown documents

GPlates projects and sessions can associate ordinary UTF-8 Markdown (`.md`)
files. Open **Window > Project Documents** to manage them. The files remain
external files: they can be edited in other applications, tracked in Git, and
are never deleted when removed from a GPlates project.

## Managing documents

- **Create** writes a new Markdown file and associates it with the current
  session. The first document is made primary automatically.
- **Add** associates an existing Markdown file without copying it.
- **Remove** removes only the association. It does not delete the file.
- **Set Primary** designates the one document whose front matter can supply
  typed project metadata. A project can also have no primary document.
- **Save**, **Save As**, and **Reload** use the integrated plain-text editor.
  The Preview tab renders the saved or edited Markdown with Qt's Markdown
  renderer. Normal editor undo and redo shortcuts remain available.
- **External Editor** and **Show File** hand the document to the desktop.
- **Locate** repairs an association when a document has moved.

The document list shows primary, unsaved, missing, parse-warning, and
externally modified states. GPlates prompts before discarding unsaved edits.
If another program changes a clean file, GPlates reloads it and reparses
primary metadata. If the in-app editor also has changes, GPlates preserves
them and marks the conflict so the user can choose Save or Reload.

Document order, display names, paths, and the primary designation are saved in
the existing session/project archive. Paths use the same relative-path and
missing-file recovery machinery as other project files; document contents are
not embedded in the archive.

## Typed project metadata

Only the Primary Project Document supplies metadata. Supported front matter
must begin on its first line and use the following schema:

```markdown
---
gplates:
  schema_version: 1
  planet:
    radius_m: 6900000
  resolution:
    default_km: 500
    by_feature_type:
      MidOceanRidge: 250
  reconstruction:
    required_timestamps_ma: "1000, 950, 900, 850, 800, 750, 700, 650, 600, 560, 520, 480, 440, 400, 370, 340, 310, 280, 250, 225, 200, 180, 160, 140, 120, 100, 80, 60, 40, 20, 10, 0"
    granularity_my: 5
  subduction:
    initiation_my: 10
    propagation_km_per_my: 30
    reversal_my: 8
    breakoff_my: 15
---

# Honua

Project notes begin here.
```

`gplates.planet.radius_m` is a finite, positive number in metres. It is
optional: a document that says nothing about its planet is describing Earth, and
GPlates applies Earth's radius without complaint. Schema version 1 is supported;
omitting `schema_version` currently implies version 1. Unknown keys are ignored
and are never rewritten. A bare `radius` key is not recognized, and units are
never inferred from prose.

`gplates.reconstruction.required_timestamps_ma` is an optional quoted,
comma-separated scalar. Ages must be finite values from 0 through 10000 Ma,
strictly descending from older to younger, with no duplicates and at least two
entries. GPlates preserves the supplied values as the authoritative Project
Timeline; it does not sort, regularize, interpolate, or rewrite them. Hold
**Alt** while clicking the backward (`<<`) or forward (`>>`) View buttons to
move to the adjacent older or younger Project Timestamp.

`gplates.resolution.default_km` is the intended level of detail, expressed as
the longest segment a line should have. `by_feature_type` overrides it for named
feature types, written without their `gpml:` prefix because a colon inside a
YAML key would have to be quoted. It is a statement of intent, not a rule
enforced behind the user's back: holding **Shift** while placing a vertex clamps
that vertex to this distance from the previous one, and a plain click is
untouched.

`gplates.reconstruction.granularity_my` is the step, in My, the world is meant
to be evolved by, and the `gplates.subduction` rates say how quickly subduction
spreads once it exists: `initiation_my` from onset to a working arc,
`propagation_km_per_my` along a trench's own strike, `reversal_my` for a
polarity reversal, and `breakoff_my` for slab detachment after a collision.
These describe processes that take a known amount of time, so a tool modelling
one can tell whether the project's step makes it a single event or something
watched over several steps. They are rules of thumb rather than constants, which
is why they live in the project rather than being compiled in.

Every field is optional, and each stands or falls on its own. Numbers must be
finite and positive when present: saying nothing leaves a consumer its own
default, which is deliberately not the same as saying zero.

A bad value costs only its own field. That field is left unset, so its consumer
applies the default it would have used had the document said nothing, and the
reason is reported — naming the offending value, and saying that the rest of the
document is still being used. A radius of zero does not discard the timestamp
schedule; an unreadable schedule does not discard the resolution; one unusable
`by_feature_type` override does not discard the others or the project default.

This applies only to values. A document that could not be parsed at all — a
missing closing delimiter, broken indentation, a duplicate key, a construct
outside the supported subset, an unsupported `schema_version` — fails as a
whole, because nothing was successfully read and there is nothing to salvage.

After the primary document is saved or reloaded, GPlates reparses it and
updates the project-wide effective radius. Measurement distances and polygon
areas, reconstructed linear velocities, and velocity-field calculations use
that value. Geometry and rotations remain angular/unit-sphere calculations.

No primary document, no front matter, an unreadable document, or invalid or
unsupported metadata preserves the existing behavior: GPlates uses its exact
existing Earth equatorial-radius constant. Invalid metadata also produces a
non-blocking diagnostic in the Project Documents panel.

## Supported YAML subset and limitations

The front-matter reader intentionally supports only indentation-based nested
mappings and scalar values. It rejects malformed indentation, duplicate keys,
sequences, flow mappings, anchors, aliases, tags, and block scalars with a
visible warning. It reads but never rewrites the front matter. General YAML,
structured metadata editing, embedded attachments, and per-feature Markdown
links are outside this feature.

Radius changes apply only after saving or reloading the primary document.
Earth-specific scientific models retain their established constants; the
project radius is used only when generic angular quantities are converted to
physical distance, area, or linear velocity.

## Manual test checklist

1. Create `project.md`, then add a second Markdown document.
2. Set `project.md` as primary, save the GPlates project, close it, and reopen
   it. Confirm document order and the primary marker are restored.
3. Edit Markdown and confirm the Preview tab renders it.
4. Add valid radius and timestamp front matter and save. Confirm the panel
   reports Project Markdown as the radius source, a physical measurement
   changes, and Alt-click navigation follows the exact irregular schedule.
5. Enter an invalid radius and save. Confirm a warning appears and the exact
   Earth default is used.
6. Restore the radius, duplicate one timestamp, and save. Confirm the radius
   remains active while Project Timeline navigation reports the invalid
   schedule and falls back to the ordinary frame step.
7. Move a Markdown file outside GPlates. Confirm it is marked missing and use
   Locate to repair the association.
8. Remove a document from the project and confirm the file remains on disk.
