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

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <boost/optional.hpp>
#include <gtest/gtest.h>

#include "maths/GeometryDistance.h"
#include "maths/GeometryIntersect.h"
#include "maths/GreatCircleArc.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOverlay.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "unit-test/PolygonOverlayOracle.h"


//
// The overlay against the Phase 4a prototype (phase4b-design.md section 11.2): record for record,
// piece for piece, class for class.
//
// The data file is a frozen snapshot, exported from the Phase 4a prototype of the overlay (a
// Python implementation in exact rational arithmetic, kept outside this repository) on its
// catalogue of configurations with several records at or near one point. The prototype ran the
// chosen configuration ('r5c': identity vertices with the loop rule and the spike rule where the
// other input passes the tip at most once, self-touch cuts, the LL rule, the one-passage rule,
// midpoint tests in order of margin, the bundle rule where both inputs pass more than once,
// option 3 for anchorless chains), and the file holds every tie order it ran, including the orders
// the C++ sort can produce: GeometryIntersect orders records along a segment by a cosine in
// doubles, so records whose cosines agree within a few ulps come out in either order. Format, per
// case: 'case <name>'; 'ring P|Q <n>' then n lines 'x y z'; 'records <n>' then n lines '<kind>
// <index1> <index2>'; per tie order 'order <repairs> <check failures>', lines 'piece <P|Q> <ring>
// <closed> <internal> <common> <class> <method> <start> <end>' (start and end: the piece's end
// stops as comma-separated record indices, '-' if closed, and '~' then the vertex's records for a
// self-touch cut, which no record of the ring names), 'output <op> <n>' and n lines 'ringout
// <point> ...' (or 'output same'), and 'endorder'; then 'endcase'.
//
// The overlay must match one of the tie orders exactly in structure (which pieces exist, internal,
// common) and in every class, except a band piece in a case where a midpoint test was made: a
// single arc that is not common, whose midpoint lies within twice the thickness of the other
// input's outline. Its class is arbitrary by nature, whether midpoint-tested or deduced from
// another band piece, and the output oracle checks those. A common pair's class may differ only in
// a tie order where a band piece's does, since the sector walk can carry a band class into a short
// common chain (counted with the band pieces). It can also differ for a reason the
// prototype does not model: point-in-polygon approximates an edge shorter than about 1e-6 radians
// (zero length to GreatCircleArc) by its end point, so near one its answer is not exact.
//
// A deliberate change to the overlay's rules that changes these answers should re-export the
// snapshot from the prototype (changed to match), or retire this test; not hand-edit the file.
//

namespace
{
	using GPlatesMaths::GreatCircleArc;
	using GPlatesMaths::GeometryIntersect::Intersection;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	using GPlatesMaths::UnitVector3D;
	using GPlatesMaths::Vector3D;
	namespace Overlay = GPlatesMaths::PolygonOverlay;

	const double H = Intersection::get_on_segment_start_threshold_sine();

	//! Marks a stop key as a self-touch cut's (a stop no record of its ring names).
	const unsigned int SELF_TOUCH_CUT = 0xffffffff;


	//! A record as the prototype names it: (kind, index1, index2).
	typedef std::tuple<std::string, unsigned int, unsigned int> record_key_type;

	//! A piece as compared: (polygon, ring, start records, end records), records as C++ indices.
	typedef std::tuple<char, unsigned int, std::vector<unsigned int>, std::vector<unsigned int>>
			piece_key_type;

	class PrototypePiece
	{
	public:
		piece_key_type key;
		bool internal;
		bool common;
		std::string piece_class;
		std::string method;
	};

	class PrototypeOrder
	{
	public:
		unsigned int repairs;
		unsigned int check_failures;
		std::vector<PrototypePiece> pieces;

		//! The output rings of 'and', 'diff' and 'or' (intersection, difference, union), each ring
		//! a cyclic sequence of named points (a record index, or P|Q<ring>.<vertex>).
		std::vector<std::vector<std::string>> outputs[3];
	};

	class PrototypeCase
	{
	public:
		std::string name;
		std::vector<std::vector<PointOnSphere>> rings[2];  // P's rings, Q's rings
		std::vector<record_key_type> records;
		std::vector<std::vector<std::string>> raw_pieces;  // per order, the unparsed 'piece' lines
		std::vector<PrototypeOrder> orders;
	};


	std::vector<unsigned int>
	parse_records(
			const std::string &field)
	{
		std::vector<unsigned int> records;
		if (field == "-")
		{
			return records;
		}
		std::stringstream stream(field);
		std::string item;
		while (std::getline(stream, item, ','))
		{
			records.push_back(std::stoul(item));
		}
		return records;
	}


	std::vector<PrototypeCase>
	read_cases(
			const std::string &filename)
	{
		std::vector<PrototypeCase> cases;
		std::ifstream file(filename);
		std::string line;
		while (std::getline(file, line))
		{
			std::stringstream stream(line);
			std::string keyword;
			stream >> keyword;
			if (keyword == "case")
			{
				cases.push_back(PrototypeCase());
				cases.back().name = line.substr(5);
			}
			else if (keyword == "ring")
			{
				std::string polygon;
				unsigned int num_points;
				stream >> polygon >> num_points;
				std::vector<PointOnSphere> ring;
				for (unsigned int i = 0; i < num_points; ++i)
				{
					std::getline(file, line);
					std::stringstream point_stream(line);
					double x, y, z;
					point_stream >> x >> y >> z;
					ring.push_back(PointOnSphere(UnitVector3D(x, y, z)));
				}
				cases.back().rings[polygon == "P" ? 0 : 1].push_back(ring);
			}
			else if (keyword == "records")
			{
				unsigned int num_records;
				stream >> num_records;
				for (unsigned int i = 0; i < num_records; ++i)
				{
					std::getline(file, line);
					std::stringstream record_stream(line);
					std::string kind;
					unsigned int index1, index2;
					record_stream >> kind >> index1 >> index2;
					cases.back().records.push_back(record_key_type(kind, index1, index2));
				}
			}
			else if (keyword == "order")
			{
				PrototypeOrder order;
				stream >> order.repairs >> order.check_failures;
				cases.back().orders.push_back(order);
				cases.back().raw_pieces.push_back(std::vector<std::string>());
			}
			else if (keyword == "piece")
			{
				cases.back().raw_pieces.back().push_back(line);
			}
			else if (keyword == "output")
			{
				std::string operation;
				stream >> operation;
				PrototypeOrder &order = cases.back().orders.back();
				if (operation == "same")
				{
					const std::vector<PrototypeOrder> &orders = cases.back().orders;
					const PrototypeOrder &previous = orders[orders.size() - 2];
					for (unsigned int o = 0; o < 3; ++o)
					{
						order.outputs[o] = previous.outputs[o];
					}
					continue;
				}
				const unsigned int o = (operation == "and") ? 0 : (operation == "diff" ? 1 : 2);
				unsigned int num_rings;
				stream >> num_rings;
				for (unsigned int i = 0; i < num_rings; ++i)
				{
					std::getline(file, line);
					std::stringstream ring_stream(line);
					std::string token;
					ring_stream >> token;  // 'ringout'
					std::vector<std::string> ring;
					while (ring_stream >> token)
					{
						ring.push_back(token);
					}
					order.outputs[o].push_back(ring);
				}
			}
		}
		return cases;
	}


	std::string
	record_kind(
			const Intersection &record)
	{
		switch (record.type)
		{
		case Intersection::SEGMENTS_CROSS:
			return "LL";
		case Intersection::SEGMENT_START_ON_SEGMENT_START:
			return "SS";
		case Intersection::SEGMENT1_START_ON_SEGMENT2:
			return "SL";
		case Intersection::SEGMENT2_START_ON_SEGMENT1:
			return "LS";
		}
		return "?";
	}


	std::string
	piece_class(
			const Overlay::Piece &piece)
	{
		if (piece.common)
		{
			return piece.common_class == Overlay::COMMON_SAME_SIDE ? "same" :
					(piece.common_class == Overlay::COMMON_OPPOSITE_SIDES ? "opposite" : "none");
		}
		return piece.piece_class == Overlay::CLASS_INSIDE ? "in" :
				(piece.piece_class == Overlay::CLASS_OUTSIDE ? "out" : "none");
	}


	/**
	 * Whether two ring points are the same for the comparison: within 1e-9 radians (under 1e-3 h).
	 * Two distinct overlay vertices can lie within the rounding of a computed crossing of each
	 * other (a crossing beside a touch, or two crossings of nearly parallel segments, joined by a
	 * piece 1e-9 h to 1e-6 h long), and the prototype's float positions for its exact ones can then
	 * be equal, so that its clean-up drops one as a repeat where the C++ keeps both (measured:
	 * 6e-16 and 2e-12 radians apart); see 'without_near_repeats'.
	 */
	bool
	same_point(
			const PointOnSphere &a,
			const PointOnSphere &b)
	{
		return PolygonOverlayOracle::angle_between(a.position_vector(), b.position_vector()) < 1e-9;
	}


	//! A ring without points within 1e-9 radians of the one before (cyclically).
	std::vector<PointOnSphere>
	without_near_repeats(
			const std::vector<PointOnSphere> &ring)
	{
		std::vector<PointOnSphere> out;
		for (const PointOnSphere &point : ring)
		{
			if (out.empty() || !same_point(out.back(), point))
			{
				out.push_back(point);
			}
		}
		while (out.size() > 1 && same_point(out.front(), out.back()))
		{
			out.pop_back();
		}
		return out;
	}


	//! Whether two rings are the same cyclic sequence of points, in either direction.
	bool
	same_ring(
			const std::vector<PointOnSphere> &a,
			const std::vector<PointOnSphere> &b)
	{
		const unsigned int n = a.size();
		if (b.size() != n)
		{
			return false;
		}
		for (unsigned int start = 0; start < n; ++start)
		{
			for (int direction : { 1, -1 })
			{
				bool same = true;
				for (unsigned int i = 0; same && i < n; ++i)
				{
					const unsigned int j = (start + n + direction * static_cast<int>(i)) % n;
					same = same_point(a[i], b[j]);
				}
				if (same)
				{
					return true;
				}
			}
		}
		return false;
	}


	/**
	 * Compare the overlay's rings of the intersection, difference and union with the prototype's
	 * (by the points they name); an empty string if they are the same rings.
	 */
	//! Ring sets not compared because their decomposition is ambiguous (see compare_rings).
	unsigned int num_ambiguous_ring_sets = 0;

	//! Cases whose prototype rings depend on the tie order, whose rings are not compared.
	unsigned int num_tie_dependent_ring_sets = 0;

	//! Cases whose input rings cross, not checked as polygons (PolygonOverlayOracle::rings_cross).
	unsigned int num_crossing_inputs = 0;


	std::string
	compare_rings(
			const PrototypeCase &prototype_case,
			const PrototypeOrder &order,
			const Overlay::Overlay &overlay,
			const std::vector<unsigned int> &overlay_record_of_prototype)
	{
		const auto point_of = [&](const std::string &name)
		{
			if (name[0] == 'P' || name[0] == 'Q')
			{
				const Overlay::Side side = (name[0] == 'P') ? Overlay::SUBJECT : Overlay::CLIP;
				const std::size_t dot = name.find('.');
				const unsigned int ring_index = std::stoul(name.substr(1, dot - 1));
				const unsigned int vertex = std::stoul(name.substr(dot + 1));
				for (const Overlay::RingView &ring : overlay.get_rings())
				{
					if (ring.side == side && ring.ring_index == ring_index)
					{
						return ring.vertex(vertex);
					}
				}
			}
			const unsigned int record = overlay_record_of_prototype[std::stoul(name)];
			return overlay.get_vertices()[overlay.get_vertex_of_record(record)].position;
		};

		const Overlay::Operation operations[3] = {
				Overlay::INTERSECTION, Overlay::DIFFERENCE, Overlay::UNION };
		const char *const names[3] = { "intersection", "difference", "union" };
		std::stringstream report;
		for (unsigned int o = 0; o < 3; ++o)
		{
			// Rings that touch each other at two vertices (or one ring twice) decompose in more
			// than one valid way, depending on which repeated vertex the split meets first, and the
			// prototype starts its walks at arbitrary points: compare only where no two of the
			// prototype's rings share two points. (The oracle checks the point set regardless.)
			const std::vector<std::vector<std::string>> &prototype_rings = order.outputs[o];
			std::vector<std::vector<std::string>> sorted_names;
			for (const std::vector<std::string> &prototype_ring : prototype_rings)
			{
				sorted_names.push_back(prototype_ring);
				std::sort(sorted_names.back().begin(), sorted_names.back().end());
			}
			bool ambiguous = false;
			for (unsigned int a = 0; !ambiguous && a < sorted_names.size(); ++a)
			{
				// A point twice on one ring, or two points shared by two rings.
				ambiguous = std::adjacent_find(sorted_names[a].begin(), sorted_names[a].end()) !=
						sorted_names[a].end();
				for (unsigned int b = a + 1; !ambiguous && b < sorted_names.size(); ++b)
				{
					std::vector<std::string> shared;
					std::set_intersection(
							sorted_names[a].begin(), sorted_names[a].end(),
							sorted_names[b].begin(), sorted_names[b].end(),
							std::back_inserter(shared));
					ambiguous = shared.size() >= 2;
				}
			}
			if (ambiguous)
			{
				++num_ambiguous_ring_sets;
				continue;
			}

			std::vector<Overlay::OutputRing> rings;
			overlay.extract_rings(operations[o], rings);

			std::vector<bool> used(rings.size(), false);
			unsigned int num_unmatched = 0;
			for (const std::vector<std::string> &prototype_ring : order.outputs[o])
			{
				std::vector<PointOnSphere> points;
				for (const std::string &name : prototype_ring)
				{
					points.push_back(point_of(name));
				}
				bool matched = false;
				points = without_near_repeats(points);
				for (unsigned int r = 0; !matched && r < rings.size(); ++r)
				{
					if (!used[r] && same_ring(without_near_repeats(rings[r].points), points))
					{
						used[r] = matched = true;
					}
				}
				num_unmatched += matched ? 0 : 1;
			}
			if (num_unmatched != 0 || rings.size() != order.outputs[o].size())
			{
				report << names[o] << ": " << rings.size() << " rings against the prototype's "
						<< order.outputs[o].size() << ", " << num_unmatched
						<< " of its unmatched; ";
			}
		}
		return report.str();
	}


	/**
	 * Compare one case; returns an empty string if it matches, else what differed.
	 */
	std::string
	compare_case(
			PrototypeCase &prototype_case,
			unsigned int &num_band_differences,
			unsigned int &num_oracle_failures)
	{
		const auto polygon = [](const std::vector<std::vector<PointOnSphere>> &rings)
		{
			return PolygonOnSphere::create(
					rings.front(),
					std::vector<std::vector<PointOnSphere>>(rings.begin() + 1, rings.end()));
		};
		const PolygonOnSphere::non_null_ptr_to_const_type P = polygon(prototype_case.rings[0]);
		const PolygonOnSphere::non_null_ptr_to_const_type Q = polygon(prototype_case.rings[1]);

		// Count an inconsistency rather than throw, so that the case's report is complete.
		Overlay::Options options;
		options.throw_if_inconsistent = false;
		Overlay::Overlay overlay(*P, *Q, options);
		overlay.classify();

		// The design requires no repair on generated data (phase4b-design.md section 12, step 15).
		std::stringstream report;
		if (overlay.get_diagnostics().check_failures != 0 ||
			overlay.get_diagnostics().unresolved_pieces != 0 ||
			overlay.get_diagnostics().repairs != 0)
		{
			report << overlay.get_diagnostics().check_failures << " check failures, "
					<< overlay.get_diagnostics().unresolved_pieces << " unresolved pieces, "
					<< overlay.get_diagnostics().repairs << " repairs";
			return report.str();
		}

		// Records: the same set, and the map from the prototype's indices to the overlay's.
		const std::vector<Intersection> &records = overlay.get_graph().unordered_intersections;
		std::map<record_key_type, unsigned int> overlay_record_of_key;
		for (unsigned int r = 0; r < records.size(); ++r)
		{
			const record_key_type key(
					record_kind(records[r]),
					records[r].segment_index1,
					records[r].segment_index2);
			overlay_record_of_key[key] = r;
		}
		std::vector<unsigned int> overlay_record_of_prototype;
		for (const record_key_type &key : prototype_case.records)
		{
			auto found = overlay_record_of_key.find(key);
			if (found == overlay_record_of_key.end())
			{
				report << "records differ (prototype has " << std::get<0>(key) << "("
						<< std::get<1>(key) << "," << std::get<2>(key) << "))";
				return report.str();
			}
			overlay_record_of_prototype.push_back(found->second);
		}
		if (records.size() != prototype_case.records.size())
		{
			report << "records differ (" << records.size() << " against "
					<< prototype_case.records.size() << ")";
			return report.str();
		}

		// Parse the prototype's pieces, with their records as the overlay's indices.
		for (unsigned int o = 0; o < prototype_case.orders.size(); ++o)
		{
			for (const std::string &line : prototype_case.raw_pieces[o])
			{
				std::stringstream stream(line);
				std::string keyword, polygon, start, end;
				PrototypePiece piece;
				unsigned int ring;
				int closed, internal, common;
				stream >> keyword >> polygon >> ring >> closed >> internal >> common
						>> piece.piece_class >> piece.method >> start >> end;
				piece.internal = internal != 0;
				piece.common = common != 0;
				const auto stop_key = [&](std::string field)
				{
					// '~' and the vertex's records: a self-touch cut, which no record names.
					const bool cut = !field.empty() && field[0] == '~';
					std::vector<unsigned int> key;
					for (unsigned int r : parse_records(cut ? field.substr(1) : field))
					{
						key.push_back(overlay_record_of_prototype[r]);
					}
					std::sort(key.begin(), key.end());
					if (cut)
					{
						key.push_back(SELF_TOUCH_CUT);
					}
					return key;
				};
				const std::vector<unsigned int> start_records = stop_key(start);
				const std::vector<unsigned int> end_records = stop_key(end);
				piece.key = piece_key_type(polygon[0], ring, start_records, end_records);
				prototype_case.orders[o].pieces.push_back(piece);
			}
		}

		// The overlay's pieces, keyed alike.
		std::vector<std::pair<piece_key_type, const Overlay::Piece *>> overlay_pieces;
		for (const Overlay::Piece &piece : overlay.get_pieces())
		{
			const Overlay::RingView &ring = overlay.get_rings()[piece.ring];
			const auto stop_key = [&](const Overlay::Stop &stop)
			{
				std::vector<unsigned int> key = overlay.get_stop_records(stop);
				const bool cut = stop.kind == Overlay::Stop::RECORD && key.empty();
				if (cut)
				{
					key = overlay.get_vertex_records(overlay.get_vertices()[stop.vertex]);
				}
				std::sort(key.begin(), key.end());
				if (cut)
				{
					key.push_back(SELF_TOUCH_CUT);
				}
				return key;
			};
			std::vector<unsigned int> start_records, end_records;
			if (!piece.closed)
			{
				start_records = stop_key(overlay.get_stops()[piece.start_stop]);
				end_records = stop_key(overlay.get_stops()[piece.end_stop]);
			}
			overlay_pieces.push_back(std::make_pair(
					piece_key_type(
							ring.side == Overlay::SUBJECT ? 'P' : 'Q',
							ring.ring_index,
							start_records,
							end_records),
					&piece));
		}

		// Match the overlay's structure against each tie order; compare the classes of the first
		// that matches.
		std::string first_mismatch;
		for (const PrototypeOrder &order : prototype_case.orders)
		{
			if (order.repairs != 0 || order.check_failures != 0)
			{
				continue;
			}

			std::vector<bool> used(order.pieces.size(), false);
			std::vector<const PrototypePiece *> match(overlay_pieces.size(), nullptr);
			bool structure_matches = order.pieces.size() == overlay_pieces.size();
			for (unsigned int i = 0; structure_matches && i < overlay_pieces.size(); ++i)
			{
				for (unsigned int j = 0; j < order.pieces.size(); ++j)
				{
					if (!used[j] && order.pieces[j].key == overlay_pieces[i].first)
					{
						used[j] = true;
						match[i] = &order.pieces[j];
						break;
					}
				}
				structure_matches = match[i] != nullptr;
			}
			if (!structure_matches)
			{
				if (first_mismatch.empty())
				{
					first_mismatch = "pieces differ in every tie order";
				}
				continue;
			}

			// Band pieces are exempt only where some piece's class came from a midpoint test.
			bool any_midpoint_test = overlay.get_diagnostics().midpoint_tests != 0;
			for (const PrototypePiece &prototype_piece : order.pieces)
			{
				any_midpoint_test = any_midpoint_test || prototype_piece.method == "midpoint";
			}

			std::stringstream differences;
			std::stringstream common_differences;  // allowed only downstream of a band difference
			unsigned int band_differences = 0;
			unsigned int num_common_differences = 0;
			for (unsigned int i = 0; i < overlay_pieces.size(); ++i)
			{
				const Overlay::Piece &piece = *overlay_pieces[i].second;
				const PrototypePiece &prototype_piece = *match[i];
				const std::string where =
						std::string(1, std::get<0>(overlay_pieces[i].first)) +
						std::to_string(std::get<1>(overlay_pieces[i].first)) +
						" piece " + std::to_string(i);
				if (piece.internal != prototype_piece.internal)
				{
					differences << where << ": internal " << piece.internal
							<< " against " << prototype_piece.internal << "; ";
					continue;
				}
				if (bool(piece.common) != prototype_piece.common)
				{
					differences << where << ": common " << bool(piece.common)
							<< " against " << prototype_piece.common << "; ";
					continue;
				}
				if (piece.internal)
				{
					continue;
				}

				const std::string overlay_class = piece_class(piece);
				if (overlay_class == prototype_piece.piece_class)
				{
					continue;
				}

				// A band piece may have either class.
				if (any_midpoint_test &&
					!piece.common &&
					!piece.has_interior_vertex &&
					!piece.closed)
				{
					const std::vector<PointOnSphere> points = overlay.get_piece_points(piece);
					const Vector3D sum =
							Vector3D(points.front().position_vector()) +
							Vector3D(points.back().position_vector());
					const PointOnSphere midpoint(sum.get_normalisation());
					const PolygonOnSphere &other =
							(overlay.get_rings()[piece.ring].side == Overlay::SUBJECT) ? *Q : *P;
					const boost::optional<GPlatesMaths::AngularDistance> distance =
							GPlatesMaths::minimum_distance(midpoint, other);
					if (distance && distance->calculate_angle().dval() < 2 * H)
					{
						++band_differences;
						continue;
					}
				}

				std::stringstream &out = piece.common ? common_differences : differences;
				out << where << ": class " << overlay_class << " (method " << piece.method
						<< ") against " << prototype_piece.piece_class
						<< " (" << prototype_piece.method << "); ";
				num_common_differences += piece.common ? 1 : 0;
			}

			// A common chain's class can follow a band piece's (deduced from it through the
			// sector walk), so a common difference is accepted only where a band piece differs.
			if (num_common_differences != 0 && band_differences == 0)
			{
				differences << common_differences.str();
			}

			if (differences.str().empty())
			{
				num_band_differences += band_differences + num_common_differences;

				// The output of every operation, by the oracle (phase4b-design.md section 11.3).
				const std::vector<PointOnSphere> samples =
						PolygonOverlayOracle::get_samples(*P, *Q, overlay);
				// Grouping and orientation only where no input rings cross: otherwise the output
				// crosses itself where the input does, and its rings have no consistent side.
				const bool input_rings_cross = PolygonOverlayOracle::rings_cross(*P) ||
						PolygonOverlayOracle::rings_cross(*Q);
				if (input_rings_cross)
				{
					++num_crossing_inputs;
				}
				std::stringstream oracle_report;
				for (Overlay::Operation operation : {
						Overlay::INTERSECTION, Overlay::DIFFERENCE, Overlay::REVERSE_DIFFERENCE,
						Overlay::UNION, Overlay::SYMMETRIC_DIFFERENCE })
				{
					std::vector<Overlay::OutputRing> rings;
					overlay.extract_rings(operation, rings);
					const unsigned int failures =
							PolygonOverlayOracle::count_failures(*P, *Q, operation, rings, samples);
					if (failures != 0)
					{
						oracle_report << "operation " << operation << ": " << failures << " of "
								<< samples.size() << " samples wrong; ";
					}

					// Grouped into polygons, with the output on the left of every ring. (Where
					// input rings cross, only that no hemisphere failure is found: the output rings
					// cross where the input's do, and their grouping into polygons means nothing.)
					std::vector<Overlay::OutputPolygon> polygons;
					try
					{
						overlay.extract(operation, polygons);
					}
					catch (const Overlay::InconsistentOverlayException &)
					{
						oracle_report << "operation " << operation << ": over a hemisphere; ";
						continue;
					}
					if (input_rings_cross)
					{
						continue;
					}
					const unsigned int polygon_failures = PolygonOverlayOracle::count_failures(
							*P, *Q, operation, polygons, samples);
					const unsigned int misoriented =
							PolygonOverlayOracle::count_misoriented_rings(polygons);
					if (polygon_failures != 0 || misoriented != 0)
					{
						oracle_report << "operation " << operation << ": " << polygon_failures
								<< " samples wrong as polygons, " << misoriented
								<< " rings misoriented; ";
					}
				}
				if (PolygonOverlayOracle::is_area_sum_applicable(*P, *Q))
				{
					const double area_error = PolygonOverlayOracle::area_sum_error(*P, overlay);
					const double tolerance = PolygonOverlayOracle::area_sum_tolerance(*P, *Q);
					if (area_error > tolerance)
					{
						oracle_report << "the intersection's and the difference's areas sum to the "
								<< "subject's within " << area_error << " of it (allowed "
								<< tolerance << "); ";
					}
				}
				const Overlay::Diagnostics &diagnostics = overlay.get_diagnostics();
				if (diagnostics.pairing_failures != 0 || diagnostics.keep_mismatches != 0)
				{
					oracle_report << diagnostics.pairing_failures << " pairing failures, "
							<< diagnostics.keep_mismatches << " keep mismatches; ";
				}
				if (diagnostics.hemisphere_check_failures != 0 ||
					diagnostics.hemisphere_check_skipped != 0 ||
					(!input_rings_cross &&
						(diagnostics.coincident_rings != 0 || diagnostics.grouping_failures != 0)))
				{
					oracle_report << diagnostics.hemisphere_check_failures
							<< " hemisphere check failures, "
							<< diagnostics.hemisphere_check_skipped << " skipped, "
							<< diagnostics.coincident_rings << " coincident rings, "
							<< diagnostics.grouping_failures << " grouping failures; ";
				}
				if (!oracle_report.str().empty())
				{
					++num_oracle_failures;
					return "classes agree, but the output does not: " + oracle_report.str();
				}

				// The ring structure (which the oracle cannot see: it reads the output by parity),
				// against the prototype's, where no band piece differs and the prototype's rings
				// are the same in every clean tie order (design 11.2 item 4: rings that touch can
				// decompose either way, and its walk's start depends on the order).
				const auto ring_set = [](const std::vector<std::vector<std::string>> &rings)
				{
					std::vector<std::vector<std::string>> set;
					for (std::vector<std::string> ring : rings)
					{
						std::sort(ring.begin(), ring.end());
						set.push_back(ring);
					}
					std::sort(set.begin(), set.end());
					return set;
				};
				bool tie_dependent = false;
				for (const PrototypeOrder &other : prototype_case.orders)
				{
					if (other.repairs != 0 || other.check_failures != 0)
					{
						continue;
					}
					for (unsigned int o = 0; o < 3; ++o)
					{
						tie_dependent = tie_dependent ||
								ring_set(other.outputs[o]) != ring_set(order.outputs[o]);
					}
				}
				if (tie_dependent)
				{
					++num_tie_dependent_ring_sets;
				}
				if (band_differences + num_common_differences == 0 && !tie_dependent)
				{
					const std::string ring_report = compare_rings(
							prototype_case, order, overlay, overlay_record_of_prototype);
					if (!ring_report.empty())
					{
						++num_oracle_failures;
						return "classes agree, but the rings do not: " + ring_report;
					}
				}
				return std::string();
			}
			if (first_mismatch.empty() || first_mismatch == "pieces differ in every tie order")
			{
				first_mismatch = differences.str();
			}
		}

		if (first_mismatch.empty())
		{
			// Every tie order of the prototype needed repair or failed a check.
			return "no tie order of the prototype is free of repairs and check failures";
		}

		std::stringstream dump;
		overlay.dump(dump);
		return first_mismatch + "\n" + dump.str();
	}
}


namespace
{
	/**
	 * Compare every case of one exported file, reporting each that differs as a test failure.
	 */
	void
	compare_file(
			const std::string &filename,
			unsigned int minimum_num_cases,
			bool allow_prototype_failures = false)
	{
		std::vector<PrototypeCase> cases = read_cases(filename);
		ASSERT_GE(cases.size(), minimum_num_cases) << filename;

		unsigned int num_failures = 0;
		unsigned int num_band_differences = 0;
		unsigned int num_prototype_failures = 0;
		unsigned int num_oracle_failures = 0;
		num_ambiguous_ring_sets = 0;
		num_crossing_inputs = 0;
		num_tie_dependent_ring_sets = 0;
		for (PrototypeCase &prototype_case : cases)
		{
			// A case the prototype itself fails in every tie order (a check failure, or repair) is
			// not a difference; it is reported, for the hunt to investigate.
			bool prototype_clean = false;
			for (const PrototypeOrder &order : prototype_case.orders)
			{
				prototype_clean = prototype_clean ||
						(order.repairs == 0 && order.check_failures == 0);
			}
			if (!prototype_clean)
			{
				++num_prototype_failures;
				std::cout << "the prototype fails too: " << prototype_case.name << std::endl;
				continue;
			}

			const unsigned int band_differences_before = num_band_differences;
			const std::string mismatch =
					compare_case(prototype_case, num_band_differences, num_oracle_failures);
			if (num_band_differences != band_differences_before)
			{
				std::cout << "band pieces with the other class: " << prototype_case.name << " ("
						<< num_band_differences - band_differences_before << ")" << std::endl;
			}
			if (!mismatch.empty())
			{
				++num_failures;
				ADD_FAILURE() << prototype_case.name << ": " << mismatch;
			}
		}
		// A committed file holds only cases the prototype is clean on.
		if (!allow_prototype_failures)
		{
			EXPECT_EQ(num_prototype_failures, 0u) << filename;
		}
		std::cout << cases.size() << " prototype cases, " << num_failures << " differ ("
				<< num_oracle_failures << " in output only; " << num_ambiguous_ring_sets
				<< " ring sets not compared, touching at two vertices; "
				<< num_tie_dependent_ring_sets
				<< " cases whose prototype rings depend on the tie order; "
				<< num_crossing_inputs
				<< " with crossing input rings not checked as polygons); "
				<< num_band_differences << " band pieces have the other class; "
				<< num_prototype_failures << " skipped (the prototype fails)" << std::endl;
	}
}


// The Phase 4a catalogue: configurations with several records at or near one point.
TEST(PolygonOverlayPrototypeTest, Catalogue)
{
	compare_file(GPLATES_UNIT_TEST_DATA_DIR "/polygon-overlay/prototype-catalogue.txt", 400);
}


// Generated cases from the Phase 4a hunt: run 2's residue, which the bundle rule clears
// (phase4a-run3.md section 4.1), a sample of every generator, two 'chain' cases with an output
// ring all of whose vertices lie on another ring (grouping decides by an edge midpoint), the four
// cases of review 3 that alone show the sector walk's phase and the four-kept bounce pairing
// (six#27, #207, near#48, #277), 20 of the 'holes' generator (holes and islands in both inputs,
// no ring crossing another: the grouping's parent choice), and chain#146 seed 2, whose chain vertex
// at a segment's end needs the self-touch cuts' rounding margin.
TEST(PolygonOverlayPrototypeTest, Hunt)
{
	compare_file(GPLATES_UNIT_TEST_DATA_DIR "/polygon-overlay/prototype-hunt.txt", 100);
}


// Common chains that no other piece reaches (Phase 4a run 2 section 7's kinds, three seeds):
// identical and jittered rings, either way round, holes equal to rings, thin faces, two rings.
TEST(PolygonOverlayPrototypeTest, Anchorless)
{
	compare_file(GPLATES_UNIT_TEST_DATA_DIR "/polygon-overlay/prototype-anchorless.txt", 40);
}


// Vertices where both inputs pass more than once, the only place the bundle rule applies after
// run 4 (about 1 vertex in 900 dense shared-edge cases; Phase 5 review 2 section 1.1).
TEST(PolygonOverlayPrototypeTest, Bundle)
{
	compare_file(GPLATES_UNIT_TEST_DATA_DIR "/polygon-overlay/prototype-bundle.txt", 15);
}


// A larger export, not committed (a hunt): set GPLATES_POLYGON_OVERLAY_PROTOTYPE_CASES to its path.
TEST(PolygonOverlayPrototypeTest, File)
{
	const char *const filename = std::getenv("GPLATES_POLYGON_OVERLAY_PROTOTYPE_CASES");
	if (!filename)
	{
		GTEST_SKIP() << "GPLATES_POLYGON_OVERLAY_PROTOTYPE_CASES is not set";
	}
	compare_file(filename, 1, true/*allow_prototype_failures*/);
}


// The one-passage rule (phase4a-run4.md) on run 2's residue and run 4's chain failures: each has a
// vertex of six or more ends where one input passes once, which the sector walk on the measured
// order got wrong (run 2's residue was cleared by the bundle rule, the chain cases by nothing
// before run 4). Each is now consistent, and the counts are the prototype's, which are the same in
// every tie order: one-passage vertices, those whose built order is not the measured one, and no
// bundle (both inputs pass more than once nowhere here).
TEST(PolygonOverlayPrototypeTest, OnePassageRule)
{
	class Expected
	{
	public:
		const char *name;
		unsigned int one_passage_vertices;
		unsigned int one_passage_moved;
	};
	const Expected expected[] = {
			{ "shared#28 seed 1", 3, 3 },
			{ "shared#169 seed 1", 5, 2 },  // 3 before the self-touch cuts
			{ "shared#234 seed 1", 1, 1 },
			{ "shared#247 seed 1", 2, 1 },
			{ "six#5 seed 7", 2, 2 },
			{ "six#188 seed 7", 2, 2 },
			{ "chain#41 seed 1", 2, 2 },
			{ "chain#182 seed 1", 1, 1 } };

	const std::vector<PrototypeCase> cases =
			read_cases(GPLATES_UNIT_TEST_DATA_DIR "/polygon-overlay/prototype-hunt.txt");
	for (const Expected &e : expected)
	{
		SCOPED_TRACE(e.name);
		const auto found = std::find_if(
				cases.begin(),
				cases.end(),
				[&e](const PrototypeCase &c) { return c.name == e.name; });
		ASSERT_TRUE(found != cases.end());

		const auto polygon = [](const std::vector<std::vector<PointOnSphere>> &rings)
		{
			return PolygonOnSphere::create(
					rings.front(),
					std::vector<std::vector<PointOnSphere>>(rings.begin() + 1, rings.end()));
		};
		const auto P = polygon(found->rings[0]);
		const auto Q = polygon(found->rings[1]);
		Overlay::Overlay overlay(*P, *Q);
		overlay.classify();

		const Overlay::Diagnostics &diagnostics = overlay.get_diagnostics();
		EXPECT_EQ(diagnostics.check_failures, 0u);
		EXPECT_EQ(diagnostics.check_failures_before_repair, 0u);
		EXPECT_EQ(diagnostics.unresolved_pieces, 0u);
		EXPECT_EQ(diagnostics.one_passage_vertices, e.one_passage_vertices);
		EXPECT_EQ(diagnostics.one_passage_moved, e.one_passage_moved);
		EXPECT_EQ(diagnostics.both_pass_twice, 0u);
		EXPECT_EQ(diagnostics.bundle_vertices, 0u);
	}
}



// The bundle rule's measurements on the bundle cases (Phase 5 review 2, section 1.1): the most
// floating rays, admissible orders and consistent placements at one vertex, the vertices with more
// than one consistent placement and those whose order used is not the measured one. The values are
// the prototype's ('r5c'), so that the enumeration's size is pinned, not only the classes.
// (shared#2893 seed 11 is no longer here: its bundle vertex was a self-touch on a shared edge,
// which the self-touch cuts turn into common pairs.)
TEST(PolygonOverlayPrototypeTest, BundleRule)
{
	class Expected
	{
	public:
		const char *name;
		unsigned int floating_max;
		unsigned int admissible_max;
		unsigned int placements_max;
		unsigned int ambiguous;
		unsigned int moved;
	};
	const Expected expected[] = {
			{ "shared#107 seed 11", 3, 20, 4, 1, 2 },
			{ "shared#2100 seed 11", 4, 360, 16, 1, 0 },
			{ "shared#1925 seed 12", 6, 720, 32, 2, 2 },
			{ "shared#2240 seed 12", 6, 5040, 144, 2, 2 },
			{ "shared#366 seed 13", 4, 60, 3, 1, 1 },
			{ "shared#2729 seed 13", 1, 3, 1, 0, 0 },
			{ "shared#2859 seed 13", 4, 360, 16, 1, 1 },
			{ "shared#3176 seed 13", 4, 48, 2, 1, 1 },
			{ "shared#3890 seed 13", 6, 3360, 60, 2, 2 },
			{ "shared#1265 seed 14", 2, 6, 2, 1, 1 },
			{ "shared#1373 seed 14", 4, 68, 8, 1, 1 },
			{ "shared#1707 seed 14", 4, 70, 8, 1, 1 },
			{ "shared#3346 seed 14", 3, 40, 4, 1, 1 },
			{ "shared#3653 seed 14", 1, 2, 1, 0, 0 } };

	const std::vector<PrototypeCase> cases =
			read_cases(GPLATES_UNIT_TEST_DATA_DIR "/polygon-overlay/prototype-bundle.txt");
	for (const Expected &e : expected)
	{
		SCOPED_TRACE(e.name);
		const auto found = std::find_if(
				cases.begin(),
				cases.end(),
				[&e](const PrototypeCase &c) { return c.name == e.name; });
		ASSERT_TRUE(found != cases.end());

		const auto polygon = [](const std::vector<std::vector<PointOnSphere>> &rings)
		{
			return PolygonOnSphere::create(
					rings.front(),
					std::vector<std::vector<PointOnSphere>>(rings.begin() + 1, rings.end()));
		};
		const auto P = polygon(found->rings[0]);
		const auto Q = polygon(found->rings[1]);
		Overlay::Overlay overlay(*P, *Q);
		overlay.classify();

		const Overlay::Diagnostics &diagnostics = overlay.get_diagnostics();
		EXPECT_EQ(diagnostics.check_failures_before_repair, 0u);
		EXPECT_GE(diagnostics.bundle_vertices, 1u);
		EXPECT_EQ(diagnostics.bundle_floating_max, e.floating_max);
		EXPECT_EQ(diagnostics.bundle_admissible_max, e.admissible_max);
		EXPECT_EQ(diagnostics.bundle_placements_max, e.placements_max);
		EXPECT_EQ(diagnostics.bundle_ambiguous, e.ambiguous);
		EXPECT_EQ(diagnostics.bundle_moved, e.moved);
		EXPECT_EQ(diagnostics.bundle_capped, 0u);
	}
}
