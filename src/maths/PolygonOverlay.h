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
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "GeometryIntersect.h"
#include "GreatCircleArc.h"
#include "MathematicalException.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "Vector3D.h"

// Windows' WinUser.h defines DIFFERENCE (a region-combining flag nothing here uses), which turns
// the Operation enumerator below into a number wherever <windows.h> came first. The files that
// name the enumerator include this header after any Windows header, so undefining it here is
// enough.
#ifdef DIFFERENCE
#	undef DIFFERENCE
#endif


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

		/**
		 * The Boolean operations, each a function f(in subject, in clip) of a point's memberships
		 * (phase4b-design.md section 5.1). Partitioning's inside is the intersection, its outside
		 * the difference.
		 */
		enum Operation
		{
			INTERSECTION,          // p and q
			DIFFERENCE,            // p and not q (subject minus clip)
			REVERSE_DIFFERENCE,    // q and not p (clip minus subject)
			UNION,                 // p or q
			SYMMETRIC_DIFFERENCE   // p != q
		};

		//! Whether a point with these memberships is in the output of @a operation.
		inline
		bool
		is_in_output(
				Operation operation,
				bool in_subject,
				bool in_clip)
		{
			switch (operation)
			{
			case INTERSECTION:
				return in_subject && in_clip;
			case DIFFERENCE:
				return in_subject && !in_clip;
			case REVERSE_DIFFERENCE:
				return in_clip && !in_subject;
			case UNION:
				return in_subject || in_clip;
			case SYMMETRIC_DIFFERENCE:
			default:
				return in_subject != in_clip;
			}
		}


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
			unsigned int self_touch_cuts = 0;     // pieces cut where the input touches itself
			unsigned int zero_length_rays = 0;    // rays with no direction
			unsigned int max_ends = 0;            // most ends at a vertex (a common ray: two)

			// Classification.
			unsigned int vertex_tests = 0;
			unsigned int deduced = 0;
			unsigned int sector_classes = 0;      // common pieces classified by the sector walk
			unsigned int midpoint_tests = 0;
			unsigned int unresolved_pieces = 0;   // left unclassified (a common pair counts once)
			unsigned int anchorless_resolved = 0; // common chains classified by option 3
			unsigned int anchorless_arcs_max = 0; // most arcs in one such chain
			unsigned int along_pieces = 0;        // a polyline's pieces along the clip's outline
			unsigned int ll_disagreements = 0;    // a polyline's tested pieces at a crossing agree
			unsigned int check_failures = 0;      // vertices still inconsistent after repair
			unsigned int check_failures_before_repair = 0;
			unsigned int repairs = 0;             // successful flips (a whole chain counts once)
			unsigned int whole_chain_repairs = 0; // of which, a common chain flipped at once
			unsigned int trusted_flipped = 0;     // repairs that flipped a vertex-tested class
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

			// Linking, accumulated over every extract.
			unsigned int pairing_failures = 0;        // vertices whose kept ends did not pair
			unsigned int unresolved_vertices = 0;     // a vertex with an unclassified piece end
			unsigned int keep_mismatches = 0;         // kept by the sectors, not by the classes
			unsigned int rings_split = 0;             // cuts of a ring passing a vertex twice
			unsigned int repeated_positions_removed = 0;
			unsigned int collapsed_rings_dropped = 0; // fewer than three distinct positions

			// Grouping and the hemisphere check, accumulated over every extract.
			unsigned int coincident_rings = 0;        // ring pairs with no vertex off the other
			unsigned int grouping_failures = 0;       // a hole whose parent is not an exterior
			unsigned int hemisphere_check_failures = 0;  // the output read as its complement
			unsigned int hemisphere_check_skipped = 0;   // no reference point clear of both

			// Provenance, accumulated over every extract: a piece's segment whose ends lie on no
			// one segment of the original subject (a subject location lost) or, both being cut
			// points, of an earlier clip (nothing lost: the point's clip location is this clip's).
			unsigned int provenance_subject_mismatches = 0;
			unsigned int provenance_clip_mismatches = 0;
		};


		/**
		 * Thrown when the overlay's classes describe no consistent arrangement, after repair
		 * (phase4b-design.md section 9.2): never a silent wrong answer.
		 *
		 * Carries the diagnostics and the first failing vertex, for a report.
		 */
		class InconsistentOverlayException :
				public MathematicalException
		{
		public:
			InconsistentOverlayException(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::string &message,
					const Diagnostics &diagnostics,
					const boost::optional<PointOnSphere> &vertex_position) :
				MathematicalException(exception_source),
				d_message(message),
				d_diagnostics(diagnostics),
				d_vertex_position(vertex_position)
			{  }

			~InconsistentOverlayException() throw()
			{  }

			const Diagnostics &
			get_diagnostics() const
			{
				return d_diagnostics;
			}

			//! The first failing vertex's output position (none if only pieces were unresolved).
			const boost::optional<PointOnSphere> &
			get_vertex_position() const
			{
				return d_vertex_position;
			}

		protected:

			const char *
			exception_name() const override
			{
				return "InconsistentOverlayException";
			}

			void
			write_message(
					std::ostream &os) const override
			{
				write_string_message(os, d_message);
			}

		private:
			std::string d_message;
			Diagnostics d_diagnostics;
			boost::optional<PointOnSphere> d_vertex_position;
		};


		/**
		 * Where an output point lies on an input (phase4b-design.md section 2.8): a vertex, or a
		 * segment and the angle fraction along it.
		 */
		class Location
		{
		public:
			//! 0 for the exterior ring (or a polyline), 'i + 1' for interior ring 'i'.
			unsigned int ring;

			//! A vertex index, or a segment index when 'ratio' is set.
			unsigned int index;

			//! None: the vertex itself; else the angle fraction along segment 'index'
			//! (GreatCircleArc::point_on_arc's parameter).
			boost::optional<double> ratio;
		};


		/**
		 * Where an output point came from (phase4b-design.md section 7): its location on the
		 * subject (relative to the original subject when composed through a cascade, section 7.2),
		 * and on the clip that made it, when it lies on that clip's outline.
		 */
		class Provenance
		{
		public:
			//! None: a point of the clip's outline inside the subject (a cut).
			boost::optional<Location> subject;

			//! Set for every point on the outline of clip 'clip_index'.
			boost::optional<Location> clip;

			//! Which clip of a cascade 'clip' is on (Options::clip_index; 0 outside a cascade).
			unsigned int clip_index = 0;
		};

		//! One per point of a geometry, in GeometryUtils::get_geometry_points order.
		typedef std::vector<Provenance> provenance_seq_type;


		/**
		 * One output ring (phase4b-design.md section 2.8). In an OutputPolygon it has the output on
		 * its left: an exterior runs anticlockwise seen from outside the sphere, a hole clockwise.
		 */
		class OutputRing
		{
		public:
			std::vector<PointOnSphere> points;

			//! One per point, when Options::want_provenance (else empty).
			provenance_seq_type provenances;
		};


		/**
		 * One output polyline of a polyline subject (phase4b-design.md section 6.1): a run of the
		 * subject's consecutive pieces of one class, in the subject's direction.
		 */
		class OutputPolyline
		{
		public:
			std::vector<PointOnSphere> points;
			bool inside;

			//! One per point, when Options::want_provenance (else empty).
			provenance_seq_type provenances;
		};


		//! One output polygon: an exterior ring and its holes (phase4b-design.md section 4.2).
		class OutputPolygon
		{
		public:
			OutputRing exterior;
			std::vector<OutputRing> interiors;
		};


		//! What the caller asks of an overlay.
		class Options
		{
		public:
			//! The speed of point-in-polygon tests against the clip (PolygonPartitioner passes its
			//! partition_point_speed_and_memory).
			PolygonOnSphere::PointInPolygonSpeedAndMemory clip_speed = PolygonOnSphere::ADAPTIVE;

			//! The most rounds of repair (phase4b-design.md section 3.9).
			unsigned int max_repair_rounds = 20;

			/**
			 * Whether an inconsistency left after repair throws InconsistentOverlayException (the
			 * default); if false it is only counted in the diagnostics (for hunts and tests).
			 */
			bool throw_if_inconsistent = true;

			/**
			 * The class of a polyline subject's piece along the clip's outline (a common pair):
			 * inside (requirements Q2, decided), so that partitioning puts it in the inside list
			 * only; false for a caller that wants such pieces outside.
			 */
			bool polyline_along_is_inside = true;

			//! Whether the outputs carry a Provenance per point (phase4b-design.md section 7).
			bool want_provenance = false;

			/**
			 * The subject's own provenance, one per point (get_geometry_points order), when the
			 * subject is a piece of an earlier overlay in a cascade: the outputs' provenance is
			 * then composed with it, relative to the original (section 7.2). Must outlive the
			 * overlay.
			 */
			const provenance_seq_type *subject_provenance = nullptr;

			/**
			 * With @a subject_provenance: the original subject (the geometry the cascade started
			 * from, to which the table refers), and the cascade's clips by clip index, so that
			 * composing can tell a ring's last segment (vertex 0 ends it). Without them a location
			 * at vertex 0 is taken for the start of segment 0 only, and an edge needing the
			 * wrap-around is counted as a mismatch (Diagnostics::provenance_subject_mismatches or
			 * provenance_clip_mismatches): never a wrong location. Both must outlive the overlay.
			 *
			 * Only the subject's locations reach a caller (a coverage's values, section 7.3):
			 * every output point at an overlay vertex lies on this overlay's clip, so its clip
			 * location is this clip's. An earlier clip's locations are composed only to tell an
			 * edge along an earlier cut (both ends on that clip, perhaps also both on the
			 * subject's outline) from a mismatch.
			 */
			const GeometryOnSphere *original_subject = nullptr;
			const std::vector<const PolygonOnSphere *> *cascade_clips = nullptr;

			//! The clip's index in a cascade, recorded in every clip location it makes.
			unsigned int clip_index = 0;
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
			 * Classify every piece (phase4b-design.md section 3.7), then check every vertex and
			 * repair (@a check_and_repair). For a polyline subject, its pieces by its own rules
			 * (section 6.1: no sector walk, no checks but one counted, no repair); the result is
			 * RESULT_INSIDE when no piece is outside, RESULT_OUTSIDE when none is inside.
			 *
			 * The result is RESULT_INSIDE when the difference (subject minus clip) keeps no piece,
			 * RESULT_OUTSIDE when the intersection keeps none, and RESULT_INTERSECTING otherwise.
			 *
			 * The result counts regions that clean-up would collapse, so it can be
			 * RESULT_INTERSECTING where the outputs leave one of them empty (a caller wanting
			 * phase4b-design.md section 5.3's result extracts and looks, as PolygonPartitioner
			 * does).
			 *
			 * @throws InconsistentOverlayException if a vertex is still inconsistent after repair,
			 * or a piece is left unclassified (unless Options::throw_if_inconsistent is false).
			 */
			Result
			classify();

			/**
			 * Check every vertex, repair what fails (section 3.9) and return the result; the second
			 * half of @a classify, public so that a test can set a class by hand and run it again.
			 *
			 * @throws InconsistentOverlayException as @a classify does.
			 */
			Result
			check_and_repair();

			/**
			 * Set a piece's class (a common pair's, on both its pieces) and the method recorded for
			 * it; for tests that provoke an inconsistency, and for diagnostics.
			 */
			void
			set_piece_class(
					unsigned int piece_index,
					bool inside_or_same_side,
					Method method);

			/**
			 * The sector walk at one vertex, on the current classes; with @a deductions, also the
			 * classes every solution agrees on for the vertex's unclassified pieces.
			 */
			VertexSolution
			solve_vertex(
					unsigned int vertex_index,
					std::vector<Deduction> *deductions = nullptr,
					bool use_free_rule = true) const;


			/**
			 * The output of one operation as polygons with holes (phase4b-design.md sections 3.10
			 * to 4.3): the rings of @a extract_rings grouped by containment with parity depth (an
			 * even depth is an exterior, an odd depth a hole of the ring containing it, and an
			 * island in a hole an exterior of its own), each oriented with the output on its left.
			 * Appended to @a polygons.
			 *
			 * Call after @a classify. @throws InconsistentOverlayException if the rings would be
			 * read as the output's complement (section 5.2): the union or the symmetric difference
			 * exceeds a hemisphere, or the classes are wrong (the only cause for the intersection
			 * and the differences, which lie inside an input). Unless
			 * Options::throw_if_inconsistent is false: then counted
			 * (Diagnostics::hemisphere_check_failures) and the rings returned as read, which a
			 * caller must not take for the output. Also throws as @a extract_rings does.
			 */
			void
			extract(
					Operation operation,
					std::vector<OutputPolygon> &polygons) const;

			/**
			 * The rings of one operation's output (phase4b-design.md sections 3.10 and 3.11): the
			 * kept ends paired at every vertex and walked into rings, rings passing a vertex twice
			 * cut there, repeated positions removed and collapsed rings dropped. Appended to
			 * @a rings. Not grouped into polygons, and in the direction walked (arbitrary).
			 *
			 * Call after @a classify. @throws InconsistentOverlayException if a vertex's ends
			 * cannot be paired (which consistent classes rule out), unless
			 * Options::throw_if_inconsistent is false (then counted in the diagnostics, and the
			 * affected rings left missing).
			 */
			void
			extract_rings(
					Operation operation,
					std::vector<OutputRing> &rings) const;

			/**
			 * A polyline subject's output (phase4b-design.md section 6.1): its consecutive pieces
			 * of one class joined into polylines, in its direction, a repeated position at a join
			 * dropped; an internal piece (a loop or spike within a vertex) takes the class of the
			 * piece before it (or after it, at the start), so that the runs cover the polyline.
			 * A record's point is its position on the polyline (not its vertex's output position,
			 * which a dense run of touches can put far off it). Appended to @a polylines.
			 *
			 * Call after @a classify.
			 */
			void
			extract_polylines(
					std::vector<OutputPolyline> &polylines) const;

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

			//! Mutable: extract_rings accumulates the linking counters.
			mutable Diagnostics d_diagnostics;

			/**
			 * Where a ring is cut at an overlay vertex that no record of its own names: the input
			 * touches itself there, on a stretch it shares with the other input (@a
			 * find_self_touch_cuts). Keyed by (ring, ring-local segment, the vertex whose stop the
			 * cuts follow); each cut is (ratio along the segment, vertex), in order along it.
			 */
			std::map<std::tuple<unsigned int, unsigned int, unsigned int>,
					std::vector<std::pair<double, unsigned int>>> d_self_touch_cuts;


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

			//! The cuts of @a d_self_touch_cuts the pieces need; false if none.
			bool
			find_self_touch_cuts();

			//! Every ring's stops, pieces, loops, spikes and common pairs (after the vertices).
			void
			build_pieces();

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

			//! The pieces of one ring: [first, end) in the piece list.
			std::pair<unsigned int, unsigned int>
			ring_piece_range(
					unsigned int ring) const;

			//! Whether a piece bounds the output of an operation (none while unclassified).
			boost::optional<bool>
			is_kept(
					const Piece &piece,
					Operation operation) const;

			//! Steps (a) to (e) of classification (phase4b-design.md section 3.7).
			void
			classify_pieces();

			//! A polyline subject's classification (phase4b-design.md section 6.1).
			void
			classify_polyline_pieces();

			//! An output point's provenance at an overlay vertex, walked from @a arriving_stop to
			//! @a leaving_stop (the walked pieces' stops at the vertex; either may be none), at
			//! @a position.
			Provenance
			get_vertex_provenance(
					unsigned int vertex_index,
					const PointOnSphere &position,
					boost::optional<unsigned int> arriving_stop,
					boost::optional<unsigned int> leaving_stop) const;

			//! The ring a stop belongs to.
			unsigned int
			ring_of_stop(
					unsigned int stop_index) const;

			//! An output point's provenance at a stop that is an original vertex of its ring.
			Provenance
			get_original_vertex_provenance(
					unsigned int stop_index) const;

			//! A polyline subject's output point's provenance at one of its stops.
			Provenance
			get_polyline_stop_provenance(
					unsigned int stop_index) const;

			/**
			 * A location on this overlay's subject, composed with Options::subject_provenance
			 * (section 7.2) into @a provenance's subject (and an earlier clip's location, for a
			 * point of an earlier cut); unchanged without a table.
			 */
			void
			compose_subject_location(
					const RingView &ring,
					const Location &location,
					Provenance &provenance) const;

			//! Up to Options::max_repair_rounds of repair at the first failing vertex.
			void
			repair();

			//! The result from the classes (what the intersection and the difference keep).
			Result
			get_result_from_classes() const;

			//! The first vertex with no consistent placement, if any.
			boost::optional<unsigned int>
			find_failing_vertex() const;

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

			//! The common subject pieces in @a start_piece's chain (through all-common vertices).
			std::vector<unsigned int>
			get_common_chain(
					unsigned int start_piece) const;

			//! Classify an arc of a chain no other piece reaches (option 3); false if none can be.
			bool
			classify_anchorless_chain(
					const std::vector<unsigned int> &chain);

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
