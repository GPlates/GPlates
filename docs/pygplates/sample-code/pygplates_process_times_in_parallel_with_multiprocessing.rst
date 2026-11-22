.. _pygplates_process_times_in_parallel_with_multiprocessing:

Process times in parallel with multiprocessing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This example shares a calculation that is done at each of a sequence of geological times among
several processes, using a :class:`multiprocessing.Pool<multiprocessing.pool.Pool>` from the Python standard library.

Many pyGPlates workflows do the same thing at one time after another, and the times do not depend
on each other. They can be done at the same time by worker processes, each on its own CPU.

What the main process gives a worker process, and what the worker gives back, is
`pickled <https://docs.python.org/3/library/pickle.html>`_ to send it from one process to the other.
Most pyGPlates objects can be pickled, including :class:`pygplates.RotationModel`,
:class:`pygplates.TopologicalModel`, :class:`pygplates.ReconstructModel`, their snapshots,
features, feature collections and geometries. In the reference, the page of a class that can be
pickled says so.

Here the calculation is the total length of subduction zones, as in
:ref:`pygplates_find_total_ridge_and_subduction_zone_lengths`, and the main process sends each
worker process a topological model.

.. contents::
   :local:
   :depth: 2

Data files
""""""""""

``rotations.rot``
    A rotation file. It must contain rotations for the plate IDs of the features that the
    topologies are built from.

``topologies.gpml``
    Topological plate polygon features, with or without deforming network features. Some of their
    boundary sections should be subduction zones, or every length is zero.

Files of these kinds are in the GPlates `sample data <https://www.gplates.org/download/>`_.

Sample code
"""""""""""

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py

Details
"""""""

The code that runs in the worker processes is in functions defined at the top level of the script.
A worker process finds a function by importing the script and looking up the function's name, so
a function defined inside another function, or a ``lambda``, will not do.

Each worker process keeps its own topological model in a global variable.
``initialise_worker()`` is called once in each worker process, when it starts, and sets it.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: worker-model

``find_total_subduction_zone_length()`` is called in a worker process, once for each time.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: worker-function

The worker process resolves the topologies itself, using its own topological model.

That is the point of giving each worker process a model: the model is sent once to each worker,
and each time is then only a number. A :class:`pygplates.TopologicalSnapshot` can be pickled too,
but what is pickled is what it was made from (its topological features, rotations and time), not
its resolved topologies, which are resolved again when it is unpickled. So sending a snapshot for
each time would send every feature and rotation for each time, and resolve each time twice: once
in the main process to make the snapshot, and again in the worker process.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: resolve-in-worker

The value that the function returns is pickled and sent back to the main process.

Numbers and strings can be pickled. So can geometries (such as a
:meth:`resolved boundary polygon<pygplates.ResolvedTopologicalBoundary.get_resolved_boundary>`) and
features (such as a :meth:`resolved feature<pygplates.ResolvedTopologicalBoundary.get_resolved_feature>`).

The objects that hold reconstructed or resolved geometries cannot, such as
:class:`pygplates.ResolvedTopologicalBoundary`, :class:`pygplates.ReconstructedFeatureGeometry` and
the shared sub-segments used here. Returning one of them fails with an error from ``multiprocessing``.
So a worker returns what it calculated from them, not the objects themselves.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: return-result

The rest of the script is inside ``if __name__ == '__main__':``, which is true only in the main process.

This matters wherever a worker process is a new Python interpreter that imports the script, as it
is on Windows and macOS. Without the test each worker would itself create a model and a pool of workers.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: main-guard

The :class:`topological model<pygplates.TopologicalModel>` is created once, in the main process,
from the topological features and the rotation file.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: create-topological-model

A :class:`multiprocessing.Pool<multiprocessing.pool.Pool>` starts the worker processes.

Its *initializer* and *initargs* arguments are the function to call in each worker process as it
starts, and the arguments to call it with. This is where the topological model is pickled and
sent, once for each worker process.

Without *processes* the pool starts as many worker processes as the computer has CPUs. Each has a
copy of the model, and resolves its own snapshots, so memory can limit how many are worth starting.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: create-pool

``pool.map()`` calls the function once for each time, in whichever worker process is free, and
returns the results as a list in the order of the times.

The ``with`` statement shuts the worker processes down when it ends, so the results are gathered inside it.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: map-times

Finally the main process prints the results.

.. sample-code:: pygplates_process_times_in_parallel_with_multiprocessing.py
   :fragment: print-results

.. note:: A worker process could instead load the files itself, in ``initialise_worker()``. Sending
   the model keeps the loading in one place, and also works for a model that was not loaded from
   files as it is: one created from features that the main process created or modified.

See also
""""""""

- Primer: :ref:`pygplates_primer_topological_model`
- Reference: :class:`pygplates.TopologicalModel`, :class:`pygplates.TopologicalSnapshot`,
  :class:`pygplates.ReconstructModel`, :class:`pygplates.RotationModel`,
  :meth:`pygplates.TopologicalSnapshot.get_resolved_topological_sections`
- Sample code: :ref:`pygplates_find_total_ridge_and_subduction_zone_lengths`,
  :ref:`pygplates_calculate_velocities_in_dynamic_plates`
