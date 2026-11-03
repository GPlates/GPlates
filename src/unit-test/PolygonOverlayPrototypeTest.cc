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
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <boost/optional.hpp>
#include <gtest/gtest.h>

#include "maths/GeometryDistance.h"
#include "maths/GeometryIntersect.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolygonOverlay.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


//
// The overlay against the Phase 4a prototype (phase4b-design.md section 11.2): record for record,
// piece for piece, class for class.
//
// The data file is a frozen snapshot, exported from the Phase 4a prototype of the overlay (a
// Python implementation in exact rational arithmetic, kept outside this repository) on its
// catalogue of configurations with several records at or near one point. The prototype ran the
// chosen configuration (identity vertices with the loop and spike rules, the LL rule, midpoint
// tests in order of margin, the bundle rule, option 3 for anchorless chains), and the file holds
// every tie order it ran, including the orders the C++ sort can produce: GeometryIntersect orders
// records along a segment by a cosine in doubles, so records whose cosines agree within a few ulps
// come out in either order. Format, per case: 'case <name>'; 'ring P|Q <n>' then n lines 'x y z';
// 'records <n>' then n lines '<kind> <index1> <index2>'; per tie order 'order <repairs> <check
// failures>', lines 'piece <P|Q> <ring> <closed> <internal> <common> <class> <method> <start>
// <end>' (start and end: the piece's end stops as comma-separated record indices, '-' if closed),
// and 'endorder'; then 'endcase'.
//
// The overlay must match one of the tie orders exactly in structure (which pieces exist, internal,
// common) and in every class, except a band piece in a case where a midpoint test was made: a
// single arc that is not common, whose midpoint lies within twice the thickness of the other
// input's outline. Its class is arbitrary by nature, whether midpoint-tested or deduced from
// another band piece, and the output oracle checks those. It can also differ for a reason the
// prototype does not model: point-in-polygon approximates an edge shorter than about 1e-6 radians
// (zero length to GreatCircleArc) by its end point, so near one its answer is not exact.
//
// A deliberate change to the overlay's rules that changes these answers should re-export the
// snapshot from the prototype (changed to match), or retire this test; not hand-edit the file.
//

namespace
{
	using GPlatesMaths::GeometryIntersect::Intersection;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolygonOnSphere;
	using GPlatesMaths::UnitVector3D;
	using GPlatesMaths::Vector3D;
	namespace Overlay = GPlatesMaths::PolygonOverlay;

	const double H = Intersection::get_on_segment_start_threshold_sine();


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
	 * Compare one case; returns an empty string if it matches, else what differed.
	 */
	std::string
	compare_case(
			PrototypeCase &prototype_case,
			unsigned int &num_band_differences)
	{
		const auto polygon = [](const std::vector<std::vector<PointOnSphere>> &rings)
		{
			return PolygonOnSphere::create(
					rings.front(),
					std::vector<std::vector<PointOnSphere>>(rings.begin() + 1, rings.end()));
		};
		const PolygonOnSphere::non_null_ptr_to_const_type P = polygon(prototype_case.rings[0]);
		const PolygonOnSphere::non_null_ptr_to_const_type Q = polygon(prototype_case.rings[1]);

		Overlay::Overlay overlay(*P, *Q);
		overlay.classify();

		std::stringstream report;
		if (overlay.get_diagnostics().check_failures != 0 ||
			overlay.get_diagnostics().unresolved_pieces != 0)
		{
			report << overlay.get_diagnostics().check_failures << " check failures, "
					<< overlay.get_diagnostics().unresolved_pieces << " unresolved pieces";
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
				std::vector<unsigned int> start_records, end_records;
				for (unsigned int r : parse_records(start))
				{
					start_records.push_back(overlay_record_of_prototype[r]);
				}
				for (unsigned int r : parse_records(end))
				{
					end_records.push_back(overlay_record_of_prototype[r]);
				}
				std::sort(start_records.begin(), start_records.end());
				std::sort(end_records.begin(), end_records.end());
				piece.key = piece_key_type(polygon[0], ring, start_records, end_records);
				prototype_case.orders[o].pieces.push_back(piece);
			}
		}

		// The overlay's pieces, keyed alike.
		std::vector<std::pair<piece_key_type, const Overlay::Piece *>> overlay_pieces;
		for (const Overlay::Piece &piece : overlay.get_pieces())
		{
			const Overlay::RingView &ring = overlay.get_rings()[piece.ring];
			std::vector<unsigned int> start_records, end_records;
			if (!piece.closed)
			{
				start_records = overlay.get_stop_records(overlay.get_stops()[piece.start_stop]);
				end_records = overlay.get_stop_records(overlay.get_stops()[piece.end_stop]);
				std::sort(start_records.begin(), start_records.end());
				std::sort(end_records.begin(), end_records.end());
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
			unsigned int band_differences = 0;
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

				differences << where << ": class " << overlay_class << " (method " << piece.method
						<< ") against " << prototype_piece.piece_class
						<< " (" << prototype_piece.method << "); ";
			}

			if (differences.str().empty())
			{
				num_band_differences += band_differences;
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


TEST(PolygonOverlayPrototypeTest, Catalogue)
{
	std::vector<PrototypeCase> cases =
			read_cases(GPLATES_UNIT_TEST_DATA_DIR "/polygon-overlay/prototype-catalogue.txt");
	ASSERT_GT(cases.size(), 400u);

	unsigned int num_failures = 0;
	unsigned int num_band_differences = 0;
	for (PrototypeCase &prototype_case : cases)
	{
		const unsigned int band_differences_before = num_band_differences;
		const std::string mismatch = compare_case(prototype_case, num_band_differences);
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
	std::cout << cases.size() << " prototype cases, " << num_failures << " differ; "
			<< num_band_differences << " band pieces have the other class" << std::endl;
}

