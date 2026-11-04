/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GPLATES_MATHS_POLYGONOVERLAY_H
#define GPLATES_MATHS_POLYGONOVERLAY_H

#include <iosfwd>
#include <vector>
#include <boost/optional.hpp>

#include "GeometryIntersect.h"
#include "GreatCircleArc.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	/**
	 * The overlay of a subject geometry (a polygon or a polyline), P, with a clip polygon, Q.
	 *
	 * Every ring of both inputs, exterior and interior, is intersected with every ring of the
	 * other (GeometryIntersect). The intersection records are grouped into overlay *vertices* by
	 * the original vertex they name (by identity, never by distance), and every ring is cut into
	 * *pieces* between consecutive vertices. Later stages classify each piece as inside or outside
	 * the other input, and link the pieces an operation keeps into output rings.
	 *
	 * The working structures (rings, stops, vertices, pieces) are declared here, rather than
	 * hidden in the '.cc', so that the unit tests can check the structure stage by stage.
	 */
	namespace PolygonOverlay
	{
		//! Which input a ring, a piece or a vertex belongs to.
		enum Side
		{
			SUBJECT = 0,  // P, the geometry being cut
			CLIP = 1      // Q, the polygon cutting it
		};


		//! A non-common piece's class: whether it lies inside the other input.
		enum Class
		{
			CLASS_UNKNOWN,
			CLASS_INSIDE,
			CLASS_OUTSIDE
		};

		//! A common pair's class: whether the two inputs' interiors are on the same side of it.
		enum CommonClass
		{
			COMMON_UNKNOWN,
			COMMON_SAME_SIDE,
			COMMON_OPPOSITE_SIDES
		};

		//! How a piece's class was decided.
		enum Method
		{
			METHOD_NONE,
			METHOD_VERTEX_TEST,    // point-in-polygon at an original vertex of the piece
			METHOD_DEDUCED,        // from the classes at one of its vertices
			METHOD_MIDPOINT_TEST,  // point-in-polygon at a single arc's midpoint, by margin
			METHOD_SECTOR,         // a common pair's class, by the sector walk (or its partner's)
			METHOD_ANCHORLESS,     // a common chain with no anchor
			METHOD_REPAIRED,       // flipped by repair
			METHOD_ALONG           // a polyline piece along the clip's boundary
		};

		//! Where the subject lies relative to the clip, as a whole.
		enum Result
		{
			RESULT_INSIDE,
			RESULT_OUTSIDE,
			RESULT_INTERSECTING
		};


		/**
		 * An original vertex of one input: its ring and its index within that ring.
		 *
		 * Ring 0 is the exterior ring (or the polyline), and ring 'i + 1' is interior ring 'i'.
		 */
		class RingVertex
		{
		public:
			RingVertex(
					unsigned int ring_,
					unsigned int index_) :
				ring(ring_),
				index(index_)
			{  }

			unsigned int ring;
			unsigned int index;
		};


		/**
		 * One ring of one input (or the whole polyline), as the overlay reads it.
		 *
		 * Segment indices in a GeometryIntersect::Graph run through the exterior ring and then each
		 * interior ring, so 'first_segment_index' maps a global index to this ring. Vertex 'j' of a
		 * ring is the start point of its segment 'j'. A polyline's last vertex is vertex
		 * 'num_segments', which a record names as the start of the fictitious one-past-the-last
		 * segment.
		 */
		class RingView
		{
		public:
			Side side;

			//! 0 for the exterior ring (or the polyline), 'i + 1' for interior ring 'i'.
			unsigned int ring_index;

			//! The global segment index of the ring's segment 0.
			unsigned int first_segment_index;

			//! A ring: its vertex count; a polyline: one less.
			unsigned int num_segments;

			//! False only for a polyline.
			bool closed;

			//! The ring's 'num_segments' arcs.
			const GreatCircleArc *segments;

			//! The ring's stops are '[first_stop, end_stop)' in the overlay's stop list.
			unsigned int first_stop;
			unsigned int end_stop;

			//! Number of original vertices (one more than the segments for a polyline).
			unsigned int
			num_vertices() const
			{
				return closed ? num_segments : num_segments + 1;
			}

			//! The position of original vertex @a vertex_index.
			const PointOnSphere &
			vertex(
					unsigned int vertex_index) const
			{
				return vertex_index < num_segments
						? segments[vertex_index].start_point()
						: segments[num_segments - 1].end_point();  // a polyline's last vertex
			}
		};


		/**
		 * A point at which a ring is cut (a record), or an original vertex between cuts.
		 *
		 * The stops of a ring are stored contiguously, in ring order, so that a piece is a range of
		 * its ring's stops (cyclic for a closed ring) and no positions are copied.
		 */
		class Stop
		{
		public:
			enum Kind
			{
				ORIGINAL_VERTEX,  // an original vertex that is in no record
				RECORD,           // records of one overlay vertex, at one point of the ring
				DANGLING_END      // a polyline's first or last vertex, when in no record
			};

			Kind kind;

			//! Ring-local: the segment arriving here and the one leaving (equal inside a segment).
			unsigned int segment_in;
			unsigned int segment_out;

			//! Whether the stop is ring vertex 'segment_out' (always, except inside a segment).
			bool at_vertex;

			//! The overlay vertex, for a RECORD stop (unused otherwise).
			unsigned int vertex;

			//! Its records: 'num_records' from 'first_record' in the overlay's stop-record list.
			unsigned int first_record;
			unsigned int num_records;

			//! The original vertex, or the record's position inside a segment.
			PointOnSphere position;

			//! The angle fraction along 'segment_out' (0 at a vertex).
			double ratio;
		};


		//! One end of a piece, as registered at an overlay vertex.
		class End
		{
		public:
			End(
					unsigned int piece_,
					bool at_end_) :
				piece(piece_),
				at_end(at_end_)
			{  }

			unsigned int piece;
			//! False: the piece's start; true: its end.
			bool at_end;
		};


		/**
		 * What leaves an overlay vertex along one piece, or along a common pair of pieces (the
		 * subject and the clip running together leave as one ray).
		 */
		class Ray
		{
		public:
			/**
			 * The exact tangent of the ray's first arc of non-zero length, at that arc's start (not
			 * normalised). Unset when every arc of the piece lies on a zero-length segment.
			 */
			boost::optional<Vector3D> direction;

			//! Which memberships change across the ray (both, for a common ray).
			bool flips_subject;
			bool flips_clip;

			//! The subject (clip) piece end it carries: both for a common ray.
			boost::optional<End> subject_end;
			boost::optional<End> clip_end;

			/**
			 * The bundle rule's quantities (phase4b-design.md section 3.6): the angle from the
			 * vertex's output position to the start of the ray's first arc, that arc's length, the
			 * angle from the output position to the arc's far end, and the half-angle of the cone
			 * within which the arc's direction, seen from the output position, can differ from its
			 * exact tangent. The cone is 0 when the arc starts at the output position, pi when the
			 * arc is no longer than its offset (or there is no direction).
			 */
			double delta;
			double first_arc_length;
			double reach;
			double cone;

			bool
			is_common() const
			{
				return subject_end && clip_end;
			}
		};


		/**
		 * The bundle rule's state at a vertex with six or more ends (phase4b-design.md 3.8).
		 *
		 * A ray whose first arc starts away from the vertex's output position and is short has a
		 * direction that, seen from that position, is noise. Rays whose cones are pairwise
		 * separable are anchored and keep their measured order; the rest float, and the sector
		 * walk is solved on every order that places them where their cones allow.
		 */
		class Bundle
		{
		public:
			//! The anchored rays (indices into the vertex's rays), in the measured cyclic order.
			std::vector<unsigned int> anchored;

			//! The floating rays, in order of certainty (smallest cone, then largest reach).
			std::vector<unsigned int> floating;

			/**
			 * The admissible orders of the vertex's rays (each a cyclic sequence of ray indices),
			 * the measured order first. Only the measured order when no ray floats or too many do.
			 */
			std::vector<std::vector<unsigned int>> orders;

			//! More than the cap of floating rays: the measured order alone is used.
			bool capped = false;
		};


		/**
		 * An overlay vertex: the records that name a common original vertex, closed transitively.
		 *
		 * Records at one point that name no common original vertex (two crossings of a twist lying
		 * on the other polygon) are separate vertices by design.
		 */
		class Vertex
		{
		public:
			//! Its records, ascending, in the overlay's vertex-record list.
			unsigned int first_record;
			unsigned int num_records;

			//! A subject (clip) vertex it holds: the first named, in record order.
			boost::optional<RingVertex> subject_vertex;
			boost::optional<RingVertex> clip_vertex;

			//! The output position: the subject's vertex, else the clip's, else the first record's.
			PointOnSphere position;

			//! The ends of the pieces here, after the loop and spike rules (internal ones removed).
			std::vector<End> ends;

			//! The rays, in the measured anticlockwise order about 'position' (from outside).
			std::vector<Ray> rays;

			//! At six or more ends (a common ray counts two): the bundle rule's orders.
			boost::optional<Bundle> bundle;
		};


		/**
		 * A stretch of one ring between two consecutive record stops, or a whole ring with none.
		 */
		class Piece
		{
		public:
			//! Index into the overlay's rings.
			unsigned int ring;

			/**
			 * Its first and last stops (global indices). A closed ring's piece has its ring's
			 * first stop as both; a whole polyline's has the polyline's first and last stops.
			 */
			unsigned int start_stop;
			unsigned int end_stop;

			//! A ring with no record (one piece), or a polyline with none.
			bool closed;

			//! Whether an ORIGINAL_VERTEX stop lies strictly inside the piece.
			bool has_interior_vertex;

			//! The ring-local segment, when the piece is one arc.
			boost::optional<unsigned int> single_arc_segment;

			//! The overlay vertices at its ends (none at a dangling end, or for a closed piece).
			boost::optional<unsigned int> start_vertex;
			boost::optional<unsigned int> end_vertex;

			//! A loop or a spike: not classified, not a ray, not output (the loop and spike rules).
			bool internal = false;

			//! The partner piece of the other input, when the two run together.
			boost::optional<unsigned int> common;

			//! Its class (a non-common piece) or common class (a piece of a common pair).
			Class piece_class = CLASS_UNKNOWN;
			CommonClass common_class = COMMON_UNKNOWN;
			Method method = METHOD_NONE;

			bool
			is_classified() const
			{
				return common ? common_class != COMMON_UNKNOWN : piece_class != CLASS_UNKNOWN;
			}
		};


		/**
		 * The sector walk's answer at one vertex (phase4b-design.md section 3.7.1).
		 */
		class VertexSolution
		{
		public:
			//! Whether some placement of the sectors' memberships meets every class at the vertex.
			bool consistent = true;

			//! Solved by the geometry-free rule (four ends, two of each input, none common).
			bool free_rule = false;

			//! The order of the vertex's rays used (indices into its rays); empty by the free rule.
			std::vector<unsigned int> order;

			/**
			 * Per position in 'order': the memberships (in the subject, in the clip) of the sector
			 * after that ray, when every solution agrees on them.
			 */
			std::vector<boost::optional<bool>> in_subject;
			std::vector<boost::optional<bool>> in_clip;

			//! The number of orders with a consistent placement (0 if inconsistent; 1 unbundled).
			unsigned int num_consistent_orders = 0;

			//! The order used is not the measured one (the bundle rule moved a ray, or the
			//! one-passage rule built another).
			bool moved = false;

			//! Solved by the one-passage rule (six or more ends, one input passing once).
			bool one_passage = false;
		};


		//! A class the vertex rules can deduce for an unclassified piece.
		class Deduction
		{
		public:
			unsigned int piece;
			bool inside_or_same_side;  // a common piece: same side; otherwise inside
			Method method;
		};


		//! Counts of what an overlay did, for tests and failure reports.
		class Diagnostics
		{
		public:
			unsigned int internal_pieces = 0;     // loops and spikes made internal
			unsigned int spikes = 0;              // there-and-back pairs among them
			unsigned int common_pairs = 0;
			unsigned int common_ambiguous = 0;    // vertex pairs with both inputs' pieces, unpaired
			unsigned int zero_length_rays = 0;    // rays with no direction
			unsigned int max_ends = 0;            // most ends at a vertex (a common ray: two)

			// Classification.
			unsigned int vertex_tests = 0;
			unsigned int deduced = 0;
			unsigned int sector_classes = 0;      // common pieces classified by the sector walk
			unsigned int midpoint_tests = 0;
			unsigned int unresolved_pieces = 0;   // left unclassified (a common pair counts once)
			unsigned int check_failures = 0;      // vertices with no consistent placement
			unsigned int piece_side_disagreements = 0;  // debug builds: a piece's vertices disagree

			// The bundle rule, counted once per vertex by the final check.
			unsigned int bundle_vertices = 0;         // six or more ends, and a floating ray
			unsigned int bundle_floating_max = 0;     // most floating rays at one vertex
			unsigned int bundle_admissible_max = 0;   // most admissible orders at one vertex
			unsigned int bundle_placements_max = 0;   // most consistent orders at one vertex
			unsigned int bundle_ambiguous = 0;        // vertices with several consistent orders
			unsigned int bundle_capped = 0;           // vertices with too many floating rays
			unsigned int bundle_moved = 0;            // vertices not using the measured order

			// The one-passage rule, counted once per vertex by the final check.
			unsigned int one_passage_vertices = 0;
			unsigned int one_passage_moved = 0;       // its built order is not the measured one
			unsigned int both_pass_twice = 0;         // both inputs pass more than once
		};


		//! What the caller asks of an overlay.
		class Options
		{
		public:
			//! The speed of point-in-polygon tests against the clip (the partitioner's choice).
			PolygonOnSphere::PointInPolygonSpeedAndMemory clip_speed = PolygonOnSphere::ADAPTIVE;
		};


		/**
		 * One overlay of a subject with a clip polygon: one call, holding references to its two
		 * inputs, which must outlive it.
		 */
		class Overlay
		{
		public:

			Overlay(
					const PolygonOnSphere &subject,
					const PolygonOnSphere &clip,
					const Options &options = Options());

			Overlay(
					const PolylineOnSphere &subject,
					const PolygonOnSphere &clip,
					const Options &options = Options());


			//
			// The working structures, for the unit tests and diagnostics.
			//

			const GeometryIntersect::Graph &
			get_graph() const
			{
				return d_graph;
			}

			const std::vector<RingView> &
			get_rings() const
			{
				return d_rings;
			}

			const std::vector<Stop> &
			get_stops() const
			{
				return d_stops;
			}

			//! The records of a stop: indices into the graph's unordered intersections.
			std::vector<unsigned int>
			get_stop_records(
					const Stop &stop) const;

			const std::vector<Vertex> &
			get_vertices() const
			{
				return d_vertices;
			}

			//! The records of a vertex (indices into the unordered intersections), ascending.
			std::vector<unsigned int>
			get_vertex_records(
					const Vertex &vertex) const;

			//! The overlay vertex of a record.
			unsigned int
			get_vertex_of_record(
					unsigned int record_index) const
			{
				return d_vertex_of_record[record_index];
			}

			const std::vector<Piece> &
			get_pieces() const
			{
				return d_pieces;
			}

			//! The positions of a piece's points, from its start stop to its end stop inclusive.
			std::vector<PointOnSphere>
			get_piece_points(
					const Piece &piece) const;

			const Diagnostics &
			get_diagnostics() const
			{
				return d_diagnostics;
			}

			/**
			 * Write the overlay's state (vertices with their records and rays, pieces with their
			 * classes and methods) in the form of the prototype's verbose report, for debugging.
			 */
			void
			dump(
					std::ostream &output) const;


			/**
			 * Classify every piece (phase4b-design.md section 3.7), then check every vertex.
			 *
			 * The result is RESULT_INSIDE when the difference (subject minus clip) keeps no piece,
			 * RESULT_OUTSIDE when the intersection keeps none, and RESULT_INTERSECTING otherwise.
			 */
			Result
			classify();

			/**
			 * The sector walk at one vertex, on the current classes; with @a deductions, also the
			 * classes every solution agrees on for the vertex's unclassified pieces.
			 */
			VertexSolution
			solve_vertex(
					unsigned int vertex_index,
					std::vector<Deduction> *deductions = nullptr) const;

		private:

			const PolygonOnSphere *d_subject_polygon;
			const PolylineOnSphere *d_subject_polyline;
			const PolygonOnSphere &d_clip;
			Options d_options;

			GeometryIntersect::Graph d_graph;

			std::vector<RingView> d_rings;
			//! Per side: the index of its first ring in 'd_rings', and one past its last.
			unsigned int d_side_rings_begin[2];
			unsigned int d_side_rings_end[2];

			std::vector<Stop> d_stops;
			std::vector<unsigned int> d_stop_records;

			std::vector<Vertex> d_vertices;
			std::vector<unsigned int> d_vertex_records;
			std::vector<unsigned int> d_vertex_of_record;

			std::vector<Piece> d_pieces;

			Diagnostics d_diagnostics;


			void
			build();

			void
			add_polygon_rings(
					Side side,
					const PolygonOnSphere &polygon);

			void
			add_polyline_ring(
					const PolylineOnSphere &polyline);

			//! The ring of a side holding global segment (or vertex) @a segment_index.
			unsigned int
			ring_of_segment(
					Side side,
					unsigned int segment_index) const;

			void
			build_vertices();

			void
			build_stops_and_pieces(
					unsigned int ring);

			void
			apply_loop_and_spike_rules();

			void
			find_common_pairs();

			void
			build_rays();

			//! A ray's direction and cone quantities, from one piece end (false if no direction).
			bool
			measure_ray(
					const Vertex &vertex,
					const End &end,
					Ray &ray) const;

			//! The other input's polygon, against which a piece of @a side is classified.
			const PolygonOnSphere &
			other_polygon(
					Side side) const
			{
				return side == SUBJECT ? d_clip : *d_subject_polygon;
			}

			//! Point-in-polygon, threshold off, against the other input of a piece of @a side.
			bool
			is_inside_other(
					Side side,
					const PointOnSphere &point) const;

			//! Apply deductions at every vertex until nothing changes; true if anything did.
			bool
			deduce_to_fixed_point();

			bool
			solve_four_end_vertex(
					const Vertex &vertex,
					std::vector<Deduction> *deductions) const;

			//! The measured anticlockwise order of rays about a point (seen from outside).
			static
			std::vector<Ray>
			sort_rays_anticlockwise(
					const PointOnSphere &centre,
					const std::vector<Ray> &rays);

			//! The input with exactly two ends at a vertex of six or more ends (it passes once).
			static
			boost::optional<Side>
			one_passage_side(
					const Vertex &vertex);

			//! The bundle rule's anchored and floating rays and admissible orders at a vertex.
			static
			Bundle
			build_bundle(
					const Vertex &vertex);
		};
	}
}

#endif // GPLATES_MATHS_POLYGONOVERLAY_H
