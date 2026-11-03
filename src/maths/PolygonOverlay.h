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

#include <vector>
#include <boost/optional.hpp>

#include "GeometryIntersect.h"
#include "GreatCircleArc.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"


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

			//! The piece ends here.
			std::vector<End> ends;
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
		};


		//! What the caller asks of an overlay.
		class Options
		{
		public:
			Options()
			{  }
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
		};
	}
}

#endif // GPLATES_MATHS_POLYGONOVERLAY_H
