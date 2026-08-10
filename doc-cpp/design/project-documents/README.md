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

## Planetary-radius metadata

Only the Primary Project Document supplies metadata. Supported front matter
must begin on its first line and use the following schema:

```markdown
---
gplates:
  schema_version: 1
  planet:
    radius_m: 6900000
---

# Honua

Project notes begin here.
```

`gplates.planet.radius_m` is a finite, positive number in metres. Schema
version 1 is supported; omitting `schema_version` currently implies version 1.
Unknown keys are ignored and are never rewritten. A bare `radius` key is not
recognized, and units are never inferred from prose.

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
4. Add valid radius front matter and save. Confirm the panel reports Project
   Markdown as the radius source and a physical measurement changes.
5. Enter an invalid radius and save. Confirm a warning appears and the exact
   Earth default is used.
6. Move a Markdown file outside GPlates. Confirm it is marked missing and use
   Locate to repair the association.
7. Remove a document from the project and confirm the file remains on disk.
