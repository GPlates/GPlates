import multiprocessing
import pygplates


# [fragment: worker-model]
# The topological model of a worker process.
# Each worker process has its own, which 'initialise_worker()' sets when the process starts.
worker_topological_model = None


def initialise_worker(topological_model):
    """Called once in each worker process, with the topological model that the main process gave the pool."""
    global worker_topological_model
    worker_topological_model = topological_model
# [end: worker-model]


# [fragment: worker-function]
def find_total_subduction_zone_length(time):
    """Called in a worker process, once for each time. Returns the time and the length in kms."""

    # [fragment: resolve-in-worker]
    # Resolve the topologies at 'time', in this worker process.
    topological_snapshot = worker_topological_model.topological_snapshot(time)
    # [end: resolve-in-worker]

    # Accumulate the lengths of the subduction zones along the resolved plate boundaries.
    total_subduction_zone_length = 0
    for shared_boundary_section in topological_snapshot.get_resolved_topological_sections():
        if shared_boundary_section.get_feature().get_feature_type() == pygplates.FeatureType.gpml_subduction_zone:
            for shared_sub_segment in shared_boundary_section.get_shared_sub_segments():
                total_subduction_zone_length += shared_sub_segment.get_resolved_geometry().get_arc_length()

    # [fragment: return-result]
    # The result is sent back to the main process, so it must be something that can be pickled.
    # Numbers can, and so can geometries and features, but resolved topologies cannot.
    return time, total_subduction_zone_length * pygplates.Earth.mean_radius_in_kms
    # [end: return-result]
# [end: worker-function]


# [fragment: main-guard]
# Only the main process runs the code below. A worker process imports this script to find the two
# functions above, and without this test each worker would run the code below too.
if __name__ == '__main__':
    # [end: main-guard]

    # [fragment: create-topological-model]
    # Create a topological model from the topological plate polygon features (can also include deforming networks)
    # and rotation file(s). This is done once, in the main process.
    topological_model = pygplates.TopologicalModel('topologies.gpml', 'rotations.rot')
    # [end: create-topological-model]

    # Our geological times will be from 0Ma to 20Ma in 1 My intervals.
    times = range(0, 21)

    # [fragment: create-pool]
    # Start two worker processes (leave out 'processes' to start one for each CPU).
    # The topological model is pickled and sent to each worker process, which passes it to 'initialise_worker()'.
    with multiprocessing.Pool(
            processes=2,
            initializer=initialise_worker,
            initargs=(topological_model,)) as pool:
        # [end: create-pool]

        # [fragment: map-times]
        # Share the times among the worker processes, and gather their results in the order of the times.
        results = pool.map(find_total_subduction_zone_length, times)
        # [end: map-times]

    # [fragment: print-results]
    for time, total_subduction_zone_length_in_kms in results:
        print(f'At time {time}Ma, total subduction zone length is {total_subduction_zone_length_in_kms:f} kms.')
    # [end: print-results]
