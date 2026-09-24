# Reconstruction page: working notes

Evidence behind `docs/design/architecture/reconstruction/README.md`. Paths are under `src/`,
line numbers from the tree at `553ec966e`. Terse by design; this file goes with the plan.

## Section 1 (boundary)

- GPlates-only list: `app-logic/CMakeLists.txt:301-436` (`gplates_only_srcs`). Reconstruction
  files in it: `TotalReconstructionSequenceRotationInserter/Interpolater/TimePeriodFinder`,
  `TRSUtils`, `PalaeomagUtils`, plus the layer files (`ReconstructionLayerProxy/Task/Params`,
  `ReconstructLayerProxy/Task/Params`, `ReconstructionParams`, `VelocityParams`,
  `Reconstruction`, `ReconstructGraph`). Everything else named on the page is in the module.
- Graph/builder/populator: `ReconstructionGraph.h:48-62,91-136,174-239,267-313`,
  `ReconstructionGraphBuilder.h:46-114`, `ReconstructionGraphPopulator.h:48-134`.
- Tree and creators: `ReconstructionTree.h:49-57,245-254,266-273,356-396`,
  `ReconstructionTreeCreator.h:87-140,164-256,265-298,304-376,382-444,450-502`;
  file-local impls: `ReconstructionTreeCreator.cc:52-99` (uncached),
  `ReconstructContext.cc:50-94` (identity), `ReconstructionLayerProxy.cc:46-102` (delegate).
- Methods and registry: `ReconstructMethodType.h:44-54`, `ReconstructMethodRegistry.cc:273-329`
  (six registrations), `ReconstructMethodInterface.h:55-335`,
  `ReconstructMethodFiniteRotation.h` (transform + `operator<`).
- Populators used by the methods: `ReconstructMethodFlowline.cc:32,151`,
  `ReconstructMethodMotionPath.cc:30`, `ReconstructMethodSmallCircle.cc:33`.
- `ReconstructionFeatureProperties.h:89-173` (getters), `ReconstructParams.h:79-244` (getters),
  `ReconstructHandle.h:34-76`, `TimeSpanUtils.h` (`TimeRange`, `TimeSampleSpan`).
- Outputs: `ReconstructionGeometry.h:64-156`, `ReconstructedFeatureGeometry.h:58-60` (bases),
  subclasses' base lines: `ReconstructedFlowline.h:43`, `ReconstructedMotionPath.h:50`,
  `ReconstructedSmallCircle.h:37`, `ReconstructedVirtualGeomagneticPole.h:60`,
  `TopologyReconstructedFeatureGeometry.h:52`; `ReconstructedScalarCoverage.h:61` derives from
  `ReconstructionGeometry` (not RFG) and is left to the topologies/scalar-coverage side.
  `MultiPointVectorField.h:40-140`, `VelocityDeltaTime.h` (enum), `VelocityUnits.h`.
- Seams: `ReconstructMethodInterface.h:122-140` (`Context` with optional
  `TopologyReconstruct`); `TopologyReconstruct::create` callers by grep:
  `ReconstructLayerProxy.cc:787`, `api/PyTopologicalModel.cc`. `PlateVelocityUtils.h:169`
  (`solve_velocities_on_surfaces`) and `:213,289-330` (stage rotation / velocity vector).
  Maths: `maths/FiniteRotation.h:297-307` (`interpolate`, comment says quaternion SLERP).
- Products: `api/PyRotationModel.h:60-264`, `api/PyReconstructSnapshot.cc:429-551`,
  `api/PyReconstructModel.h` (KeyValueCache of snapshots by `real_t` time),
  `ReconstructionLayerProxy.cc:118-137`, `ReconstructLayerProxy.cc:95-108`.

## Section 2 (component diagrams)

- Diagram 1 edges: populator → builder `ReconstructionGraphPopulator.cc:199-202`; builder →
  graph `ReconstructionGraphBuilder.cc:143-161`; graph → tree `ReconstructionTree.cc:146-175`;
  creator wraps impl `ReconstructionTreeCreator.h:87-140`; inheritance
  `ReconstructionTreeCreator.h:304-306,382-384,450-452`, `ReconstructionLayerProxy.cc:46-48`;
  generator owns graph via `boost::bind` copy `ReconstructionTreeCreator.cc:311-331`;
  adaptor forwards `ReconstructionTreeCreator.cc:373-385`; delegate forwards
  `ReconstructionLayerProxy.cc:57-83`; `ReconstructionLayerProxy` owns a generator
  `ReconstructionLayerProxy.cc:123-130`; `RotationModel` owns a
  `CachedReconstructionTreeCreatorImpl` (generator or adaptor) `api/PyRotationModel.h:223`,
  `api/PyRotationModel.cc:269-275,297-302`.
- Diagram 2 edges: registry `can_reconstruct`/`create` functions
  `ReconstructMethodRegistry.h:59-75`; `ReconstructContext` uses registry
  `ReconstructContext.cc:170-171,211-214`;
  context states `ReconstructContext.h:413-451`, `.cc:193-237`; `Reconstruction` = handle + RFG
  `ReconstructContext.h:97-124`; RFG observes feature `ReconstructedFeatureGeometry.h:58-60`,
  `.cc:53`; velocities `ReconstructMethodInterface.h:241-259`; `ReconstructUtils::reconstruct`
  creates a `ReconstructContext` `ReconstructUtils.cc:186-203`; `ReconstructLayerProxy` owns one
  `ReconstructLayerProxy.cc:99`; `PyReconstructSnapshot` uses the registry directly
  `api/PyReconstructSnapshot.cc:460-528`.

## Section 3 (sequence diagrams)

- (a) `api/PyReconstruct.cc:289-413` (reconstruct → `ReconstructSnapshot::create` → get or
  export), `api/PyReconstructSnapshot.cc:400-427` (wraps the rotation model in
  `RotationModel::create(rotation_model, 1, anchor_plate_id)`, i.e. an adaptor with cache size 1),
  `:440-551` (the loop), `api/PyRotationModel.cc:289-316` (adaptor creation),
  `ReconstructMethodByPlateId.cc:427-468` (tree request, composed rotation, `RFG::create` with
  transform), `ReconstructionTreeCreator.cc:257-263,373-385,334-346` (cache lookups).
  `ReconstructUtils::reconstruct` callers by grep: `cli/CliReconstructCommand.cc:234`,
  `app-logic/GeometryCookieCutter.cc`, `app-logic/CoRegistrationLayerProxy.cc`, `api/CoReg.cc`.
  `ReconstructUtils::reconstruct_geometry` callers: `api/PyFeature.cc:951` (from
  `reverse_reconstruct_geometry`, used by `feature_handle_set_geometry`, `.def("set_geometry"`
  at `:5217`), `api/PyReconstruct.cc` (`reverse_reconstruct`), `PartitionFeatureUtils.cc`,
  `qt-widgets/CreateFeatureDialog.cc`, `view-operations/FocusedFeatureGeometryManipulator.cc`.
- (b) `ApplicationState.cc:170-212` (`reconstruct` → `update_layer_tasks` → emit
  `reconstructed`), `ReconstructGraph.cc:316-397` (`Reconstruction::create`, then
  `LayerTask::update` per active layer), `ReconstructionLayerTask.cc:112-125`,
  `ReconstructLayerTask.cc:273-309`, `presentation/VisualLayers.cc:227` (connects
  `reconstructed`), `presentation/VisualLayer.cc:102-167` (`create_rendered_geometries` →
  `LayerOutputRenderer`), `presentation/LayerOutputRenderer.cc:242`
  (`get_reconstructed_feature_geometries_spatial_partition()`),
  `ReconstructLayerProxy.h:276-296` (no-arg overload uses current time and params),
  `ReconstructLayerProxy.cc:228-249,1276-1290,1250-1273` (spatial partition →
  `cache_reconstructed_features` → `ReconstructContext::get_reconstructed_features`),
  `:1380-1416,1419-1463` (`create_reconstruction_info`, `get_or_create_reconstruct_context`),
  `:648-658` (context uses the rotation proxy's delegate creator),
  `ReconstructionLayerProxy.cc:118-137,140-176` (lazy generator; always returns a delegate).

## Section 4 (how it works)

- Populator: property names `ReconstructionGraphPopulator.cc:267-282`, disabled samples skipped
  `:249-253`, ≥2 samples `:189-196`, drop when a ref frame is missing `:175-187`. Builder:
  plates on demand `ReconstructionGraphBuilder.cc:51-95`, multiple edges per pair `:97-108`,
  pole copies into pools `:110-123`, `push_front` `:138-139`, fresh graph after build
  `:151-160`, distant-past extension `:164-253`.
- "Last inserted wins": `push_front` at `ReconstructionGraphBuilder.cc:138-139,250-251`; the
  tree takes the first edge that fits `ReconstructionTree.cc:318-339,345-361`; first reversed
  edge only (`break` at `:337`); moving plate already present or equal to anchor is skipped
  `:392-414`; visiting order `ReconstructionTreeCreator.cc:104-120` (`visit_feature_collections`)
  and `AppLogicUtils.h:127,150`. The tree comment's own words about order:
  `ReconstructionTree.cc:220-223,249-253,298-303`.
- Tree cutting rules: `ReconstructionTree.cc:160-175` (missing anchor → empty tree),
  `:306-361` (reverse only from anchor or when parent is reversed; one incoming edge),
  `:372-378` (time range test), `:184-304` (the worked examples).
- Edge rotation: `ReconstructionTree.cc:36-126` (exact match, distant past/future, SLERP with
  axis hint), reversed edge `ReconstructionTree.h:192-202`, composition `.cc:129-143`, mutable
  caches `.h:235-237`, graph held by shared pointer `.h:400-404,224-227`.
- `get_composed_absolute_rotation` identity for anchor and unknown plate
  `ReconstructionTree.h:356-396`; equivalence test `.h:256-273`.
- Creators: value handle and `MapPredicate` `ReconstructionTreeCreator.h:87-140`; LRU cache
  `utils/KeyValueCache.h:18-21`; key `std::pair<real_t, plate id>` `.h:352`; `real_t`
  `operator<` uses `EPSILON` `maths/Real.h:273-280`. Cache sizes: default 1
  `ReconstructionTreeCreator.h:169`; 150 `api/PyRotationModel.cc:246`; 512
  `ReconstructionLayerProxy.h:60-74`; slots + 1 `ReconstructLayerProxy.cc:668-677`.
  Rotation proxy invalidation: `ReconstructionLayerProxy.cc:178-288` (time change does not
  invalidate; anchor, params, add/remove/modify do).
- Registry order: reverse iteration `ReconstructMethodRegistry.cc:131-155,158-183`; "order of
  registration does not matter" `:276-280`. Detection rules: by-plate-ID needs only a regular
  geometry `ReconstructMethodByPlateId.cc:131-252`; half-stage needs the enumeration and
  left/right plate IDs and a geometry `ReconstructMethodHalfStageRotation.cc` (class
  `CanReconstructFeature`, lines shown by awk, enumerations `HalfStageRotation`,
  `...Version2`, `...Version3`); small circle `gpml:SmallCircle` and VGP
  `gpml:VirtualGeomagneticPole` by feature type (their `CanReconstructFeature` classes);
  flowline `gpml:Flowline` `FlowlineUtils.h:84`; motion path `gpml:MotionPath`
  `MotionPathUtils.h:84`. One registry in `ApplicationState.cc:90`; temporaries in
  `api/PyReconstructSnapshot.cc:460`, `api/PyReconstruct.cc:460`, `ReconstructUtils.cc:294,388`,
  `ReconstructUtils.h:108,135`.
- Method instances are context-bound: `ReconstructMethodInterface.h:107-121` (NOTE), caches in
  `ReconstructMethodByPlateId.h:170-181`, `.cc:355-373,663-712,715-777`.
- `ReconstructContext`: `set_features` `ReconstructContext.cc:141-190` (drops features no
  method accepts, comment `:158-169`), `create_context_state` `:193-237`, weak refs and slot
  reuse `:219-236`, rebuild on `set_features` `:897-946`, handle per call e.g. `:264-265`,
  outputs `.h:544-659`, `get_reconstructed_features` keeps inactive features `.h:576-581`,
  `assign_geometry_property_handles` `.cc:824-894` (temporary identity context state
  `:836-847`), purpose of the handle `.h:78-92` (comment) and `.cc:63-67`; includers that use
  it: `opengl/GLReconstructedStaticPolygonMeshes.h`, `opengl/GLRasterCoRegistration.h`,
  `data-mining/*` (grep of `ReconstructContext.h`).
- `ReconstructHandle`: `ReconstructHandle.h:44,46-51,65-75`; `utils/Counter64.h` comment on
  wraparound. Use by finders `ReconstructedFeatureGeometryFinder.h:36-110`,
  `ReconstructionGeometryFinder.h` class comment.
- RFG: bases and subscription `ReconstructedFeatureGeometry.h:58-60`, `.cc:42-62,53`;
  `WeakObserver.h:128-136` (constructor subscribes), `WeakObserverPublisher.h:36-42` (linked
  list per publisher); `is_valid`/`get_feature_ref` `.h:281-319`, `.cc:113-121`; members
  `.h:484-544`; two create forms `.h:141-222`; lazy transform `.cc:124-143`;
  `ReconstructMethodFiniteRotation` ordering by parameters `ReconstructMethodFiniteRotation.h`
  (`operator<`, `less_than_compare_finite_rotation_parameters`), by plate ID in
  `ReconstructMethodByPlateId.cc:115-124`; `TopologyReconstructedFeatureGeometry.h:108,131`
  overrides.
- Velocities: default path `ReconstructMethodInterface.h:241-259`, `.cc:39-149` (re-visits
  properties `:49-50`, throwaway RFG `:89-106`, codomain element `:140-144`);
  `PlateVelocityUtils.cc:1108-1194` (`calculate_stage_rotation` fallbacks and identity);
  `VelocityDeltaTime.h` enum; by-plate-ID with topologies `ReconstructMethodByPlateId.cc:472-620`;
  `RFG::reconstructed_geometry_point_velocities` `ReconstructedFeatureGeometry.cc:156-187`,
  `ResolvedVertexSourceInfo.cc:95-160` (by plate ID or half-stage from the RFG's method type).
- Half-stage: `RotationUtils.h` (`DEFAULT_TIME_INTERVAL_HALF_STAGE_ROTATION = 10.0`,
  `get_half_stage_rotation` overloads), `RotationUtils.cc:191-300` (version 1 formula, version
  2 asymmetry, version 3 spreading start time = geometry import time). Flowlines:
  `FlowlineGeometryPopulator.cc:120-215` (tree per time, `get_stage_pole`, halved).
- Topology seam: `ReconstructMethodByPlateId.cc:383-421` (emit
  `TopologyReconstructedFeatureGeometry` when `is_valid(time)`), `:715-777`
  (`create_geometry_time_span` with params),
  `ReconstructUtils.cc:322-334` (clears `topology_reconstruct`, "A bit hacky" comment),
  `ReconstructLayerProxy.cc:648-798` (combines time spans of all topology layers,
  `TopologyReconstruct::create`).
- GPlates driver: `ReconstructLayerProxy.h:57,933-1035,1146-1149,1194-1201` (cache, key,
  weak map), `MAX_NUM_RECONSTRUCTIONS_IN_CACHE = 4` `.h`, cache size 1 with topologies
  `.cc:998-1040`, `set_features` in `add/remove/modified_reconstructable_feature_collection`
  `.cc:1091-1170`, reset on rotation proxy change `.cc:1043-1052`,
  `NotYetImplementedException` `.cc:1380-1403`.

## Section 5 and 6 (traps, weaknesses)

- Name clash 1: `Reconstruction.h:49-58` (aggregate of active layer proxies) vs
  `ReconstructContext.h:94-124`. Name clash 2: `ReconstructionGraph.h` vs `ReconstructGraph.h`
  (GPlates-only, layers). Stale include guard `ReconstructionGraphPopulator.h:137`
  (`GPLATES_APP_LOGIC_RECONSTRUCTIONTREEPOPULATOR_H`).
- Snapshot semantics: header comments `ReconstructionTreeCreator.h:67-70,152-155` and the code
  that copies poles `ReconstructionGraphBuilder.cc:110-123`; deprecated per-call graph
  `api/PyReconstructionTree.cc:358-370`.
- Silent identities: `ReconstructionTree.cc:160-172`, `.h:377-396`,
  `ReconstructMethodByPlateId.h:158-160`, `.cc:675-679`.
- `ReconstructHandle` thread comment `ReconstructHandle.h:69`; no current-handle accessor
  `:46-51`.
- Invalid features skipped: `ReconstructContext.cc:272,315,376`.
- `BY_PLATE_ID` lenient: `ReconstructMethodByPlateId.cc:156-170`.
- pyGPlates own loop and stale include: `api/PyReconstructSnapshot.cc:40` (only occurrence of
  `ReconstructContext` in the file), `:460-528`.
- `#if 0` resolved-geometry API: `ReconstructMethodInterface.h:186-201`,
  `ReconstructContext.h:521-534`; TODO at `ReconstructMethodInterface.h:170-178`.
- Ordering by list, not rule: `ReconstructionGraphBuilder.cc:125-137`.
- `model -> app-logic` upward include: `ReconstructedFeatureGeometry.cc:206-211`
  (`accept_weak_observer_visitor` → `visit_reconstructed_feature_geometry`), survey section 4.
- Stale comments `api/PyReconstructionTree.cc:469,521,583` (`ReconstructUtils::get_stage_pole`;
  the function is `RotationUtils::get_stage_pole`, `RotationUtils.h:116-121`).

## Not confirmed, left out

- Whether `TopologyReconstruct::create_geometry_time_span` tessellates before or after the
  first time slot, and what `GeometryTimeSpan::is_valid` means for partially subducted
  geometries. Not read; the page only says "where the geometry still exists".
- Why `ReconstructLayerProxy` keeps `ReconstructedFeature`s rather than RFGs as the primary
  cache: only the code comment (`ReconstructLayerProxy.cc:135-139`) says it is cheaper to
  derive the other shapes. Left as the comment says, not asserted as measured.
- The purpose of the geometry property handle ("keep per-geometry GL objects alive") is the
  header comment plus the includer list; I did not read `GLReconstructedStaticPolygonMeshes`.
- `ReconstructionFeatureProperties` vs `FeatureVisitorThatGuaranteesNotToModify` mentioned in
  `ApplicationState.cc:186-200` comment: the populator derives from `ConstFeatureVisitor`, so
  the comment looks stale, but I did not trace the visitor classes. Not on the page.
- Mermaid could not be rendered locally (no node); syntax reviewed by eye against the rules in
  the task (quoted labels, no bare `end` ids, no semicolons in messages).

## Layout evidence (for the source-reorganisation stage)

Boundary disagreements with the survey (1.1):

- `ReconstructedScalarCoverage` is listed by the survey neither here nor explicitly in 1.2; it
  derives from `ReconstructionGeometry`, is produced by `ReconstructScalarCoverageLayerProxy`,
  and its inputs are scalar-coverage time spans, so it belongs with `ScalarCoverage*`
  (topologies/deformation), not here.
- `GeometryUtils`, `GeometrySetter`, `GeometryFinder`, `GeometryTypeFinder` (survey: "helpers"
  in this area) are general geometry-on-sphere/property-value helpers: includers by directory
  are `GeometryUtils` api 7, app-logic 21, file-io 7, gui 3, opengl 1, presentation 1,
  qt-widgets 5, view-operations 2; `GeometryTypeFinder` app-logic 1, file-io 3, gui 1,
  view-operations 2. They are a maths/model bridge, not reconstruction. Suggest a
  "geometry utilities" home beside the model or maths, not under reconstruction.
- `TimeSpanUtils` (survey: this area) is used by 5 app-logic files and `api`; its main clients
  are `TopologyReconstruct` (`resolved_*_time_span_type`), `ReconstructContext` time spans and
  `ScalarCoverageTimeSpan`. It is shared infrastructure between reconstruction and topologies;
  either home works, but it should not be described as reconstruction-specific.
- `PlateVelocityUtils` (survey: topologies) contains the by-plate-ID stage-rotation and
  velocity functions this area's default velocity path depends on (`calculate_stage_rotation`,
  `calculate_velocity_vector`, `StageRotationCalculator`) alongside
  `solve_velocities_on_surfaces` and `TopologicalNetworksVelocities`. The file has two
  subjects; the by-plate-ID half belongs here and the surface solver in topologies.
- `ResolvedVertexSourceInfo` (survey: topologies, by the `Resolved*` glob) is what
  `ReconstructedFeatureGeometry::reconstructed_geometry_point_velocities` uses; it is included
  by 14 app-logic files. It sits between the two areas.
- `MultiPointVectorField` is consumed by `VelocityFieldCalculatorLayerProxy` (layers),
  file-io writers (5) and gui/presentation; produced here and in `PlateVelocityUtils`. Fine here.
- `TotalReconstructionSequencePlateIdFinder` is used by
  `file-io/FeatureCollectionFileFormatClassify.cc` and six qt-widgets rotation dialogs as well
  as `TRSUtils`/`RotationInserter`; it is a
  rotation-feature reader and belongs with the rotation editing helpers, whichever directory
  they get.
- The GPlates-only rotation editing helpers (`TotalReconstructionSequenceRotationInserter`,
  `...Interpolater`, `...TimePeriodFinder`, `TRSUtils`, `PalaeomagUtils`) are included only by
  qt-widgets dialogs (`TotalReconstructionSequencesDialog`, `EditTotalReconstructionSequence*`,
  `CreateTotalReconstructionSequenceDialog`, `ApplyReconstructionPoleAdjustmentDialog`,
  `CalculateReconstructionPoleDialog`) and one data-mining file
  (`TotalReconstructionSequenceTimePeriodFinder`). They are feature-editing support for the
  rotation dialogs (survey area 1.12), not reconstruction machinery.
- `SmallCircleGeometryPopulator.h` is included by `ReconstructUtils.cc:38` and never used there
  (one occurrence in the file): a stale include.
- `api/PyReconstructSnapshot.cc:40` includes `ReconstructContext.h` and never uses it.
- Files with no single subject: `ReconstructUtils.h` mixes classification predicates
  (`is_reconstruction_feature`, `has_reconstructable_features`), the convenience `reconstruct`
  overloads, and the geometry-rotation templates (`reconstruct_by_plate_id`,
  `reconstruct_as_half_stage`); `ReconstructionGeometryUtils.h` (1205 lines) is the
  visitor/downcast toolbox for every `ReconstructionGeometry` type including the topological
  ones, and is included from api, cli, file-io, gui, presentation, qt-widgets and
  view-operations; a reorganisation that splits reconstruction from topologies has to decide
  where the shared `ReconstructionGeometry` family (base, visitor, finder, utils) lives, since
  the visitor names 15 concrete types from three areas (`ReconstructionGeometryVisitor.h:58-72`).
- Misleading names: `Reconstruction.h` (layer output aggregate), `ReconstructGraph.h` (layer
  graph), `ReconstructionGraphPopulator.h`'s include guard, and the "Reconstruct" versus
  "Reconstruction" prefix split, which carries no consistent meaning (`ReconstructionTree` and
  `ReconstructionGeometry` are rotation-side and output-side respectively; `ReconstructContext`
  and `ReconstructMethod*` are feature-side).
- The model's `WeakObserverVisitor.h` forward-declares the RFG family; any move of the
  `ReconstructionGeometry` classes changes that include.

## Layer groups

No change proposed from reading this area. Everything on the page is in `app-logic` (shared
core) with its GPlates-only part in the engine, matching the survey's `LAYERS`. The one upward
edge this area causes (`model -> app-logic` through `WeakObserverVisitor`) is already listed
in the survey's table.
