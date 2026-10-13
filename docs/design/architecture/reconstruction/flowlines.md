# Flowlines and motion paths

What a flowline's seed point means, how a flowline is reconstructed from it, why it was changed
to work this way, what the change costs, and the alternatives that were considered and rejected.
Where the code lives, and how it fits the reconstruction framework, is in the reconstruction
area's page, [`README.md`](README.md) (section *Half-stage rotations, flowlines, motion paths*);
this document is the design behind it.

Notation: `t` is the reconstruction time; `t0` and `tn` are the youngest and oldest of the
flowline's listed times (`gpml:times`); `T` is the spreading start time; `L(t)` is the left
plate's total rotation at `t`; `H(a->b)` is the composed half-stage spreading from time `a` to
time `b`; `s` is the stored seed point.

## 1. What a flowline is

A `gpml:Flowline` has a seed point (or several), a left plate, a right plate, and a list of
times. It draws two polylines, one on each plate, tracing the crust that left the spreading
centre at the seed point: each vertex is where the spreading centre was at one of the listed
times, carried away by its plate. Spreading is symmetric: each step halves the stage rotation
between the two plates (`FlowlineUtils::get_half_angle_rotation`), without the spreading
asymmetry or the 10 My intervals that `RotationUtils::get_half_stage_rotation` gives mid-ocean
ridges.

A `gpml:MotionPath` has a seed point, a reconstruction plate and a relative plate, and a list of
times. It is the path of the seed point on its plate relative to the other plate. Its seed point
is reconstructed by plate ID like any other geometry, so none of the questions below arise for
it; it appears here only where a flowline was made to behave as it does.

## 2. What the seed point means

The stored seed point is in the *left* plate's frame at present day. It is the point on the left
plate that was at the spreading centre at the spreading start time `T`:

- `T` is the flowline's `gpml:geometryImportTime`, if it has one. The GPGIM allows the property
  on `gpml:Flowline` since information model version 1.6.0340. Everything that digitises a
  flowline at a reconstruction time records that time as the import time, so a flowline normally
  spreads from the time it was digitised, like a mid-ocean ridge with the
  `HalfStageRotationVersion3` reconstruction method.
- Without one, `T` is the youngest listed time `t0`. That is the only meaning flowlines had
  before the property was allowed: GPlates 2.5 and pyGPlates 1.0 stored the seed point as the
  present-day position, on the left plate, of the spreading centre at `t0`, and both the drawing
  and the reverse reconstruction accumulated the half-stage steps from `t0`. So a flowline from
  an older file is not shifted (its position at any reconstruction time does not change) by
  this design.

`FlowlinePropertyFinder::get_spreading_start_time` returns `T`.

Spreading from `T` rather than from the listed times gives the seed point a meaning of its own:
the listed times only choose which vertices are drawn. Three things follow, each of which the
older meaning lacked:

- **Changing the times does not shift the flowline.** Under the old meaning the seed point meant
  "the spreading centre at `t0`", so adding or removing the youngest time redefined it and the
  whole flowline shifted, at every reconstruction time.
- **Pieces of a flowline join.** Two flowlines digitised at the same point and time, with
  different time lists (0-10 Ma and 10-40 Ma, say), lie on the one curve, as two motion paths do.
  Under the old meaning they did not, because each piece's seed point referred to a different
  time. A piece digitised at a point drawn by another piece (at any time) also joins it, exactly
  when no rotation sample falls inside a step (section 4): the older piece is then exactly where
  the second click is, and from there both take the same steps.
- **Rotations younger than `T` do not affect the flowline at and older than `T`.** The stored seed
  point is the digitised point carried to present day by the left plate alone,
  `s = inverse(L(T)) * p`, so reconstructing it to `T` gives `p` back whatever the rotations
  younger than `T`, or the right plate's rotation at `T`. Reconstruction to an older time spreads
  from there with both plates (section 3), using only rotations at `T` and older, so a plate that
  does not exist younger than `T`, or a changed 0-`T` history, changes nothing at and older than
  `T`. (Younger than `T` the flowline spreads forward from `T`, and those rotations do count.)
  This is what the `HalfStageRotationVersion3` ridge method was introduced for, and why spreading
  from present day was rejected (section 8).

## 3. How a flowline is reconstructed

All of it is in `src/app-logic/FlowlineUtils.{h,cc}`, and the same functions serve drawing
(`FlowlineGeometryPopulator`), reverse reconstruction (`reconstruct_flowline_seed_points`), the
seed-point velocities and Create Feature's end-point correction, so they cannot disagree.

**The seed point at `t`** is `L(t) * H(T->t) * s` (`get_seed_point_rotation`).
`fill_seed_point_rotations` builds `H(T->t)` as half-stage steps from `T` to `t`, stepping at
each listed time strictly between them: backward in time when `t` is older than `T`, forward
when younger (as for a version-3 ridge). It does this at every `t`, inside or outside the listed
times, so the seed point is always where the spreading centre is. There are no steps at all when
`t == T`, and the stored seed point is therefore `inverse(L(T)) * (digitised point)`, whatever the
times (`test_reconstruct_flowline_older_than_oldest_time` checks this for a `T` older than `tn`).

**The halves** are built outward from the seed point at `t` (`fill_flowline_rotations`): one
half-stage rotation per listed time older than `t`, each step from the previous vertex's time.
The left half is on the left plate (the right plate moving relative to it), the right half the
mirror. The right half is built in the right plate's frame, so the stored seed point is first
moved into that frame by the left-to-right frame rotation at `T`
(`get_left_to_right_plate_frame_rotation`), which is the identity only when the two plates
coincide at `T`; before PR #36 the right half was seeded from the stored point as-is, and the
halves split whenever the plates did not coincide at `t0`.

**`T` is a step boundary but never a vertex.** When `T` falls between two step ends (between `t`
and the first vertex, or between two vertices), the step is split there and its two halves
composed into one rotation. So each vertex is where the spreading centre was at its own listed
time, whatever `T` is, and the seed point's meaning does not depend on the listed times. This is
also what makes the forward-spreading case consistent: a vertex younger than `T` is reached by
spreading forward from `T` to it.

**Outside the listed times.**

| reconstruction time | seed point | halves | `pygplates.reconstruct()` |
| --- | --- | --- | --- |
| `t0 <= t < tn` | at the spreading centre | both, meeting at the seed point | a `ReconstructedFlowline` |
| `t < t0` | at the spreading centre (spreading forward from `T` when `t < T`) | both, each starting at its `t0` vertex, carried by its own plate; the seed point is not a vertex | a `ReconstructedFlowline`; the first point of each half is the `t0` point |
| `t >= tn` | at the spreading centre | none | nothing (as for a motion path) |

Younger than `t0` the halves are the flowline's `t0` shape carried along, as a motion path is
drawn outside its times; the seed vertex is omitted because it is not at a listed time
(`FlowlineGeometryPopulator::d_omit_seed_point_vertex`). At and older than `tn` GPlates still
draws the seed point at the spreading centre, as in the table, but as a plain reconstructed
geometry rather than a flowline; that geometry carries the left plate's ID (for colouring by plate
ID, say), since the stored seed point is relative to the left plate. Before this design nothing
was drawn outside `[t0, tn]` but the unrotated stored point (flowlines have no reconstruction plate
ID), and reverse reconstruction jumped across `tn`.

Spreading outside the listed times needs both plates' rotations there. A plate missing from the
rotation tree is given the identity rotation, so a flowline whose plates have no rotations at some
time jumps there; the feature's valid time is the way to hide it then. This cost is mostly one of
viewing, not digitising, and is the price of the seed point's meaning not depending on the listed
times (see the rejected clamp, section 8).

**Velocities.** `ReconstructMethodFlowline::reconstruct_feature_velocities` (GPlates velocity
layers) and `ReconstructedFlowline::reconstructed_seed_point_velocity` (pyGPlates) take the seed
point's velocity from `get_seed_point_rotation` at the two ends of the velocity time range
(`calculate_seed_point_stage_rotation`), so it is the finite difference of how the drawn seed
point moves, on both sides of `T` and outside the listed times. The `ReconstructedFlowline` stores
the listed times and `T` for this.

**Create Feature's end points.** The dialog's "Left-plate end-point(s)" and "Right-plate
end-point(s)" options let the user click where a half of the flowline should *end* at the
current time; `correct_end_point_to_centre` undoes that half's per-vertex rotations (the same
`fill_flowline_rotations`) to find the spreading-centre point that is then reverse reconstructed.
Because the halves are built out from the seed point at every `t`, this needs no special case for
the right plate or for `t < t0`.

**A flowline as a topology section.** A flowline used as a topological section or network
interior gets its vertex velocities from the generic half-stage path
(`ResolvedVertexSourceInfo`, which reads the feature's `gpml:reconstructionMethod` and, for
version 3, its geometry import time), not from `FlowlineUtils`. That is the one place a flowline's
reconstruction method is read, which is why `create_flowline()` and Create Feature keep writing
`HalfStageRotationVersion3`: it spreads from the same `T`, though in 10 My steps.

## 4. Known inexactness: step boundaries

Each half-stage step uses only the two plates' rotations at its two ends. Between two samples of
a rotation sequence GPlates interpolates, so the relative motion inside one sampled interval is a
rotation about one fixed pole at a constant rate, and splitting a step there changes nothing: half
of each part composed is exactly half of the whole. A step is inexact only when a rotation
*sample* lies strictly inside it, where the stage pole changes: the stage is then two rotations
about different poles, and halving their composition differs from composing their halves.

So a flowline depends on where its step boundaries fall, which are its listed times, `T`, and
the reconstruction time `t`. Between listed times the effect is small (the steps are short);
outside them, where one step spans from the nearest listed time to `t`, it grows with the
distance. The reconstruction-time dependence is not new - every version built the halves out from
the seed point at `t`, which is therefore a boundary too.

Measured with pyGPlates in a synthetic model (plate 801 spreading from 201, with 801 sampled at
0, 10, 20, 40 and 100 Ma):

- A 40-60 Ma piece viewed younger than 20 Ma, against a 10-60 Ma flowline digitised at the same
  point and time: 0 km at 20 Ma, 29 km at 15 Ma, 85 km at 1 Ma (one step 40 Ma to `t` across the
  20 and 10 Ma samples). A 0-10 Ma piece viewed older than 20 Ma: 1.3 km at 21 Ma, 12.8 km at
  30 Ma. With 801 sampled only at 0 and 100 Ma (one fixed pole), all 0.
- The same flowline digitised at 15 Ma versus spreading from 10 Ma (listed times 10, 20, 30, 40):
  identical, until a deliberately large pole change is added at 13 Ma, inside the 10-20 step,
  when it differs by up to 0.49 degrees. In that model the 20 Ma vertex, viewed at 10 Ma and at
  12 Ma and carried back to present day, differs by 0.46 degrees: the pre-existing
  reconstruction-time effect, of the same size.

The fixed-pole argument holds when one plate's rotations are given relative to the other. When
the two plates are on separate branches of the rotation tree their relative motion between
samples is only approximately about one pole, so extra boundaries can make a small difference
even with no sample inside the step.

The remedy within this design is to list times at the rotation file's sample times. Stepping at
the sample times automatically (or every 10 My, as ridges do), independently of the listed times,
would make flowlines exact, but would shift existing ones slightly, so it is left as a known
limitation (section 9).

## 5. Who writes the geometry import time

| route | import time written |
| --- | --- |
| GPlates Create Feature | the digitising time (as for every feature type the GPGIM allows it on); added before the geometry is reverse reconstructed, so the seed point is stored under this meaning and the end-point correction sees it |
| pyGPlates reverse reconstruction (`create_flowline(..., reverse_reconstruct=)`, `set_geometry(..., reverse_reconstruct=)`, `reverse_reconstruct()`) | the reconstruction time (the same code that sets it for other features; before 1.6.0340 it silently skipped flowlines) |
| `Feature.create_flowline()` without `reverse_reconstruct` | the youngest time: the position is as before, and later time edits do not shift it |
| Edit Feature Properties (times), `Feature.set_times()` | for a flowline that has times but no import time: the *old* youngest time, read before the times change (section 6) |
| Shapefile, GeoJSON, GeoPackage, OGR-GMT writers | the flowline's import time, or its youngest time if it has none (section 7) |
| `Feature.set_geometry_import_time()`, Add Property, hand-edited GPML, an OGR `IMPORT_AGE` column | whatever the user gives |

There is no `create_flowline()` parameter for the import time: a user who wants one other than
the digitising time or the youngest time has `set_geometry_import_time()`. Users cannot easily
hand-make a seed point for any other time without reverse reconstruction anyway.

## 6. Editing a legacy flowline's times

A flowline without an import time spreads from its youngest time, so editing its times would
shift it. The two routes that edit times therefore first give such a flowline an import time at
its old youngest time, which changes nothing about where it is and then lets the edit change only
its vertices:

- GPlates: `EditFeaturePropertiesWidget::commit_edit_widget_data` runs the edit through
  `FlowlineUtils::edit_property_without_shifting_flowline`, which reads the youngest time before
  the edit and adds the property after it (so it cannot disturb the edit widget mid-commit), only
  for `gpml:times` of a `gpml:Flowline` without an import time. The time sequence editor commits
  whenever it holds two or more times, changed or not, so this can add the property to a flowline
  whose times were not changed; harmless, since that is where it already spreads from.
- pyGPlates: `Feature.set_times()` does the same.

Routes that do not: `Feature.set()` with `gpml:times`, modifying the `GpmlArray` in place, and in
GPlates deleting the times property and adding it again with Add Property. They are documented
(in the `set_times` docstring and at `edit_property_without_shifting_flowline`) rather than
covered: an upgrade on load was rejected (section 8), and these are the routes of someone editing
the model directly.

## 7. Compatibility

**Flowlines without an import time** keep the released meaning (section 2), verified against that
meaning computed from first principles (no difference) and by
`test_reconstruct_flowline_without_geometry_import_time`.

**Flowlines that already carry an import time** different from their youngest time change
position. Before 1.6.0340 one could only get there past the information model: GPlates' Add
Property dialog lists every GPGIM property and adds it after a warning; pyGPlates with
`VerifyInformationModel.no`; a hand-edited GPML file; or an OGR file's `IMPORT_AGE` column, which
`OgrReader` turns into `gpml:geometryImportTime` for any feature type. The property was stored
and ignored.

**OGR formats and `IMPORT_AGE`.** Shapefile, GeoJSON, GeoPackage and OGR-GMT do not store a
flowline's times (they are lost on a round trip) but do store its import time as `IMPORT_AGE`.
Earlier versions wrote no value for a feature without one, which Shapefile, GeoJSON and
GeoPackage read back as none, but OGR-GMT, which cannot hold an empty value, reads back as 0. So
every flowline saved to OGR-GMT by an earlier version comes back with an import time of 0, and
once given times again spreads from present day rather than from its youngest time: shifted,
unless its youngest time was 0. That cannot be told from a flowline really digitised at present
day, so it is documented in both changelogs with the remedy (delete the import time); reading
`IMPORT_AGE` 0 back as none for flowlines was rejected because it would break the common case
instead. The writers now write a flowline's youngest time when it has no import time, so a
flowline read back from any of these formats and given different times is not shifted.

**GPML version.** The GPGIM version went from 1.6.0339, which GPlates 2.5 and pyGPlates 1.0 both
shipped, to 1.6.0340, by the convention that every GPGIM edit bumps it. GPlates 2.5 therefore
reports "Partially supported GPML version" for every GPML file saved by 2.6, flowline or not
(pyGPlates discards read warnings). Both older versions read a flowline's `geometryImportTime` as
an ordinary property (the GPML reader accepts any property for any feature type), keep it, save
it back and ignore it, so they draw such a flowline the old way: identically when its import time
is its youngest time, elsewhere otherwise.

**Motion paths** were never affected. Their times could already omit present day; the time
sequence editor's insistence on a 0 Ma row (which it could not enforce, since the 0 cell could be
edited to anything) was the only thing keeping GUI-made flowlines drawn at present day, and was
dropped once flowlines are drawn younger than their youngest time. `Feature.create_motion_path()`
no longer writes a `ByPlateId` reconstruction method: nothing reads a motion path's method
(GPlates' Create Feature never wrote one). The property stays in the GPGIM for both feature
types. For flowlines, removing it would make `create_flowline()` raise when verified, and Create
Feature offers a flowline's left and right plates only through the reconstruction method control,
which it shows only when the GPGIM allows the property. For motion paths, files written by
pyGPlates 1.0 carry it, and `set_reconstruction_method()` on one would otherwise raise.

## 8. Rejected alternatives

- **Spreading from present day for every flowline.** Pieces join and the times do not shift the
  flowline, but the flowline then depends on the plates' relative motion between present day and
  its youngest time: missing or changed rotations there shift it at every reconstruction time,
  which the old meaning never did and the version-3 ridge method exists to avoid. It also shifted
  every existing flowline whose times do not start at present day. Implemented and then replaced
  by spreading from the import time.
- **Opting in by reconstruction method** (`HalfStageRotationVersion2` meaning "from present
  day", anything else the old meaning). Same dependence on the 0-`t0` rotations for new
  flowlines; and a flowline's method, which nothing had read, would start to mean something
  different for flowlines than for ridges (for a ridge, version 3 without an import time means
  present day; for a flowline the youngest time).
- **A `HalfStageRotationVersion4` for "flowlines exactly like ridges"** (shared half-stage code,
  10 My steps). An enumeration value applies to ridges too, where it would mean the same as
  version 3.
- **An upgrade handler adding the import time on load.** GPML-only (keyed on the file's GPGIM
  version), so Shapefiles and the other formats would not be covered; and every older file with a
  flowline would load as unsaved in GPlates. Adding it when the times are edited covers every
  format, when it matters.
- **Freezing the seed point to the left plate outside the listed times** (older than `tn`,
  carried from the spreading centre at `tn`). Symmetric and continuous, and it needs only the left
  plate's rotations there; but changing the oldest time shifted the seed point at older times,
  pieces digitised older than their oldest time did not join, and a seed point digitised there
  was stored offset by a half-stage that drawing then undid. The frozen rule only survived a
  missing *right* plate, and the young end already took that risk.
- **A `create_flowline()` import-time parameter.** No option; advanced users have
  `set_geometry_import_time()`.
- **Returning a seed-only `ReconstructedFlowline` from `pygplates.reconstruct()`** at and older
  than `tn`. Little use, and an API change (the halves would be `None`); that flowlines younger
  than a non-zero youngest time are returned is a by-product, since most start at present day.
- **Reading `IMPORT_AGE` 0 back as none for flowlines.** Breaks a flowline really digitised at
  present day, the common case (section 7).
- **A widget-wide before/after-edit hook** in Edit Feature Properties instead of the one call.
  One case does not show its shape.

## 9. Known limitations left as they are

- Flowlines are not reconstructed exactly like ridges: they step at their listed times, not at the
  rotation sample times or every 10 My (section 4). Changing that would shift existing flowlines
  slightly, so it would need an opt-in, and a reconstruction-method value is not a suitable one
  (section 8).
- Colouring flowlines by the layer's draw style; today each half is coloured by its plate.
- `OgrReader` accepts the reconstruction method `"ByPlateID"` but the GPGIM value is `ByPlateId`,
  so a motion path saved with one to an OGR format reloads with an invalid-method warning and
  loses the property.
- The `" Recostruction plate id: "` header typo in the GMT motion-path export is kept, since
  scripts may parse it.

## 10. Tests

`src/unit-test/FlowlineUtilsTest.cc` (GoogleTest, with `src/unit-test/data/flowline/`): the
end-point correction with and without an import time between the listed times, the default import
time, editing times without shifting, and the seed point spreading older than the oldest time.
`pygplates/test/test_app_logic/test.py`: the seed point and vertices against the formulas of
section 3 computed independently (`test_reconstruct_flowline_vertices`), changing times and
joining pieces, rotations younger than the import time, the legacy meaning, the oldest-time
behaviour, the OGR round trip, velocities, and exported names.
