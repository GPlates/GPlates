---
gplates:
  schema_version: 1

  # The planet this project is built on.
  #
  # Earth is 6371000. A larger world means a degree of arc covers more ground, so this is what
  # lets GPlates report real distances rather than angles that mean something different per
  # project.
  #
  # Optional. Leave the whole section out and GPlates uses Earth's radius - a project that says
  # nothing about its planet is describing Earth, and there is no reason to make you write that
  # out. Setting it to something unusable is a different matter: GPlates says so, uses Earth's
  # radius, and carries on reading the rest of this document.
  planet:
    radius_m: 6371000

  # Intended resolution - the level of detail you mean to work at, expressed as the longest
  # segment you want a line to have, in kilometres.
  #
  # This is optional. Leave the whole section out and GPlates uses its own default; it will not
  # invent a number on your behalf.
  #
  # It is a statement of intent, not a rule the program enforces behind your back. Tools consult
  # it - for example, holding Shift while placing a vertex clamps that vertex to this distance
  # from the previous one, so a line cannot quietly become coarser than you meant.
  resolution:

    # Used for a new feature, and for any feature type not listed below.
    default_km: 500

    # Per-feature-type overrides, for the things that need finer or coarser detail than the rest
    # of the project. Used when you are adding to a feature that already exists, based on that
    # feature's type.
    #
    # Write the type name without its "gpml:" prefix - a colon inside a YAML key has to be
    # quoted, and that is an easy thing to get wrong by hand.
    #
    # Uncomment and adjust whichever of these you actually use. The names must match GPlates
    # feature types exactly; see Edit > Preferences > Active Feature Types for the full list.
    by_feature_type:
      # Coastlines and rifts are where detail shows, so they want shorter segments.
      # Coastline: 100
      # ContinentalRift: 100
      # OrogenicBelt: 150

      # Mid-ocean ridges and transforms carry real structure but at a larger scale.
      # MidOceanRidge: 250
      # Transform: 250
      # SubductionZone: 250

      # Ocean floor and craton interiors can be coarse without anyone minding.
      # OceanicCrust: 1000
      # Craton: 1000
---

# Project notes

Everything below the `---` markers is ordinary Markdown and is yours. GPlates reads only the
front matter above; it never rewrites this file.

Associate this document with a project through the project documents panel, and mark it as the
Primary Project Document. GPlates then reads the settings above whenever the file is saved or
reloaded.

## What this world is

Write whatever is useful to you here - the premise, the constraints you have set yourself, what
you have decided and what is still open. It travels with the project, which is the point.
