/* $Id: HellingerModel.cc 257 2012-03-01 14:12:46Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 257 $
 * $Date: 2012-03-01 15:12:46 +0100 (Thu, 01 Mar 2012) $
 *
 * Copyright (C) 2011, 2012, 2013 Geological Survey of Norway
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

#include <set>

#include "boost/foreach.hpp"

#include <QDebug>

#include "maths/PointOnSphere.h"
#include "HellingerModel.h"



namespace{

	int
	unique_keys(
			const GPlatesQtWidgets::hellinger_model_type &model)
	{
		std::set<int> set;
		BOOST_FOREACH(GPlatesQtWidgets::hellinger_model_pair_type pair,model)
		{
			set.insert(pair.first);
		}
		//qDebug() << set.size() << " unique keys in map";

		return static_cast<int>(set.size());
	}

	/**
	 * @brief determine_fit_type_from_model - determine the fit type (i.e. two-way or
	 * three-way) of the model. Returns three-way if it finds any PLATE_THREE pick types
	 * (disabled or not), otherwise returns two-way - i.e. does not explicitly check for
	 * presence of PLATE_ONE or PLATE_TWO picks.
	 * @param model_data
	 * @return
	 */
	GPlatesQtWidgets::HellingerFitType
	determine_fit_type_from_model(
			const GPlatesQtWidgets::hellinger_model_type &model_data)
	{
		// TODO: consider making a similar std::set part of the HellingerModel and updating
		// it after each pick addition.
		std::set<GPlatesQtWidgets::HellingerPlateIndex> set;
		BOOST_FOREACH(GPlatesQtWidgets::hellinger_model_pair_type pair, model_data)
		{
			set.insert(pair.second.d_segment_type);
		}
		if ((set.find(GPlatesQtWidgets::PLATE_THREE_PICK_TYPE) != set.end() ||
			 (set.find(GPlatesQtWidgets::DISABLED_PLATE_THREE_PICK_TYPE) != set.end())))
		{
			return GPlatesQtWidgets::THREE_PLATE_FIT_TYPE;
		}
		return GPlatesQtWidgets::TWO_PLATE_FIT_TYPE;
	}
}

GPlatesQtWidgets::HellingerModel::HellingerModel():
	d_fit_type(GPlatesQtWidgets::TWO_PLATE_FIT_TYPE)
{
}

void
GPlatesQtWidgets::HellingerModel::set_pick_state(
		const unsigned int &segment,
		const unsigned int &row,
		bool enabled)
{
	hellinger_model_range_type pair =
			d_model_data.equal_range(segment);

	hellinger_model_type::iterator iter = pair.first;

	for (unsigned int n = 0 ; iter != pair.second ; ++iter, ++n)
	{
		if (n == row){
			iter->second.d_is_enabled = enabled;
		}
	}
}

bool
GPlatesQtWidgets::HellingerModel::pick_is_enabled(
		const unsigned int &segment, const unsigned int &row) const
{
	hellinger_model_const_range_type pair =
			d_model_data.equal_range(segment);

	hellinger_model_type::const_iterator iter = pair.first;

	for (unsigned int n = 0; iter != pair.second; ++iter, ++n)
	{
		if (n == row){
			return iter->second.d_is_enabled;
		}
	}
	//Should we really be sending an optional<bool> back from this function, and boost::none if it gets to this point?
	return false;
}

int
GPlatesQtWidgets::HellingerModel::num_rows_in_segment(
		const unsigned int &segment) const
{
	return d_model_data.count(segment);
}

void
GPlatesQtWidgets::HellingerModel::remove_pick(
		unsigned const int &segment, unsigned const int &row)
{
	hellinger_model_range_type pair =
			d_model_data.equal_range(segment);

	hellinger_model_type::iterator iter = pair.first;

	for (unsigned int n = 0;  iter != pair.second ; ++iter, ++n)
	{
		if (n == row)
		{
			d_model_data.erase(iter);
			return;
		}
	}
}

void
GPlatesQtWidgets::HellingerModel::remove_segment(
		const unsigned int &segment)
{
	d_model_data.erase(segment);
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::add_pick(
		const HellingerPick &pick,
		const unsigned int &segment_number)
{
	return d_model_data.insert(hellinger_model_pair_type(segment_number,pick));
}

GPlatesQtWidgets::HellingerPoleEstimate
GPlatesQtWidgets::HellingerModel::get_initial_guess_12() const
{
	return d_active_com_file_struct.d_estimate_12;
}

GPlatesQtWidgets::HellingerPoleEstimate
GPlatesQtWidgets::HellingerModel::get_initial_guess_13() const
{
	return d_active_com_file_struct.d_estimate_13;
}

void GPlatesQtWidgets::HellingerModel::set_initial_guess_12(
		const GPlatesQtWidgets::HellingerPoleEstimate &estimate)
{
	d_active_com_file_struct.d_estimate_12 = estimate;
}

void GPlatesQtWidgets::HellingerModel::set_initial_guess_13(
		const GPlatesQtWidgets::HellingerPoleEstimate &estimate)
{
	d_active_com_file_struct.d_estimate_13 = estimate;
}


void
GPlatesQtWidgets::HellingerModel::set_fit_12(
		const GPlatesQtWidgets::HellingerFitStructure &fit_12)
{
	d_last_fit_12_result.reset(fit_12);
}

void
GPlatesQtWidgets::HellingerModel::set_fit_13(
		const GPlatesQtWidgets::HellingerFitStructure &fit_13)
{
	d_last_fit_13_result.reset(fit_13);
}

void
GPlatesQtWidgets::HellingerModel::set_fit_23(
		const GPlatesQtWidgets::HellingerFitStructure &fit_23)
{
	d_last_fit_23_result.reset(fit_23);
}


boost::optional<GPlatesQtWidgets::HellingerFitStructure>
GPlatesQtWidgets::HellingerModel::get_fit_12()
{
	return d_last_fit_12_result;
}

boost::optional<GPlatesQtWidgets::HellingerFitStructure>
GPlatesQtWidgets::HellingerModel::get_fit_13()
{
	return d_last_fit_13_result;
}

boost::optional<GPlatesQtWidgets::HellingerFitStructure>
GPlatesQtWidgets::HellingerModel::get_fit_23()
{
	return d_last_fit_23_result;
}

void
GPlatesQtWidgets::HellingerModel::set_initial_guess_12(
		const double &lat,
		const double &lon,
		const double &rho)
{
	d_active_com_file_struct.d_estimate_12.d_lat = lat;
	d_active_com_file_struct.d_estimate_12.d_lon = lon;
	d_active_com_file_struct.d_estimate_12.d_angle = rho;
}

void
GPlatesQtWidgets::HellingerModel::set_initial_guess_13(
		const double &lat,
		const double &lon,
		const double &rho)
{
	d_active_com_file_struct.d_estimate_13.d_lat = lat;
	d_active_com_file_struct.d_estimate_13.d_lon = lon;
	d_active_com_file_struct.d_estimate_13.d_angle = rho;
}

void
GPlatesQtWidgets::HellingerModel::set_search_radius(
		const double &radius)
{
	d_active_com_file_struct.d_search_radius_degrees = radius;
}

void
GPlatesQtWidgets::HellingerModel::set_input_pick_filename(
		const QString &input_filename)
{
	d_active_com_file_struct.d_pick_file = input_filename;
}

void
GPlatesQtWidgets::HellingerModel::set_fit_type(
		const GPlatesQtWidgets::HellingerFitType &type)
{
	d_fit_type = type;
}

const GPlatesQtWidgets::HellingerFitType &
GPlatesQtWidgets::HellingerModel::get_fit_type(
		bool update)
{
	if (update){
		d_fit_type = determine_fit_type_from_model(d_model_data);
	}
	return d_fit_type;
}

boost::optional<GPlatesQtWidgets::HellingerComFileStructure>
GPlatesQtWidgets::HellingerModel::get_com_file() const
{
	return d_active_com_file_struct;
}

std::vector<GPlatesMaths::LatLonPoint> &
GPlatesQtWidgets::HellingerModel::error_ellipse_points(
		const GPlatesQtWidgets::HellingerPlatePairType &type)
{
	switch(type)
	{
	case PLATES_1_2_PAIR_TYPE:
		return d_error_ellipse_12_points;
		break;
	case PLATES_1_3_PAIR_TYPE:
		return d_error_ellipse_13_points;
		break;
	case PLATES_2_3_PAIR_TYPE:
		return d_error_ellipse_23_points;
		break;
	default:
		return d_error_ellipse_12_points;
	}
}

void
GPlatesQtWidgets::HellingerModel::clear_error_ellipse(
		const GPlatesQtWidgets::HellingerPlatePairType &type)
{
	switch(type)
	{
	case PLATES_1_2_PAIR_TYPE:
		d_error_ellipse_12_points.clear();
		break;
	case PLATES_1_3_PAIR_TYPE:
		d_error_ellipse_13_points.clear();
		break;
	case PLATES_2_3_PAIR_TYPE:
		d_error_ellipse_23_points.clear();
		break;
	}
}

void
GPlatesQtWidgets::HellingerModel::clear_error_ellipses()
{
	d_error_ellipse_12_points.clear();
	d_error_ellipse_13_points.clear();
	d_error_ellipse_23_points.clear();
}

QString
GPlatesQtWidgets::HellingerModel::error_ellipse_filename() const
{
	return QString(d_output_file_root + "_ellipse"+DEFAULT_OUTPUT_FILE_EXTENSION);
}

QString
GPlatesQtWidgets::HellingerModel::error_ellipse_filename(
		const HellingerPlatePairType &type) const
{
	switch(type)
	{
	case PLATES_1_2_PAIR_TYPE:
		return QString(d_output_file_root + "_ellipse_12_sim" + DEFAULT_OUTPUT_FILE_EXTENSION);
		break;
	case PLATES_1_3_PAIR_TYPE:
		return QString(d_output_file_root + "_ellipse_13_sim" + DEFAULT_OUTPUT_FILE_EXTENSION);
		break;
	case PLATES_2_3_PAIR_TYPE:
		return QString(d_output_file_root + "_ellipse_23_sim" + DEFAULT_OUTPUT_FILE_EXTENSION);
		break;
	}
	return QString();
}

bool
GPlatesQtWidgets::HellingerModel::picks_are_valid() const
{
	//TODO: apply more stringent conditions here, e.g.
	// min no. of segments, picks-per-segment etc...
	return !d_model_data.empty();
}

void
GPlatesQtWidgets::HellingerModel::set_output_file_root(
		const QString &root)
{
	d_output_file_root = root;
}

QString
GPlatesQtWidgets::HellingerModel::output_file_root() const
{
	return d_output_file_root;
}

void
GPlatesQtWidgets::HellingerModel::reset_model()
{
	d_model_data.clear();
	clear_fit_results();
	clear_uncertainty_results();
}

void GPlatesQtWidgets::HellingerModel::clear_all_picks()
{
	d_model_data.clear();
}

void
GPlatesQtWidgets::HellingerModel::clear_fit_results()
{
	d_last_fit_12_result.reset();
	d_last_fit_13_result.reset();
	d_last_fit_23_result.reset();
}

void
GPlatesQtWidgets::HellingerModel::clear_uncertainty_results()
{
	clear_error_ellipses();
}

void
GPlatesQtWidgets::HellingerModel::clear_com_file_struct()
{
	d_active_com_file_struct.d_pick_file = "";


	d_active_com_file_struct.d_estimate_12.d_lat = 0;
	d_active_com_file_struct.d_estimate_12.d_lon = 0;
	d_active_com_file_struct.d_estimate_12.d_angle = 5.;

	d_active_com_file_struct.d_estimate_13.d_lat = 0;
	d_active_com_file_struct.d_estimate_13.d_lon = 0;
	d_active_com_file_struct.d_estimate_13.d_angle = 5.;


	d_active_com_file_struct.d_search_radius_degrees = 0.2;
	d_active_com_file_struct.d_perform_grid_search = false;
	d_active_com_file_struct.d_significance_level = 0.95;
	d_active_com_file_struct.d_estimate_kappa = true;
	d_active_com_file_struct.d_generate_output_files = false;
}

void
GPlatesQtWidgets::HellingerModel::renumber_segments()
{
	hellinger_model_type result;

	int last_segment_number = 0;
	int new_segment_number = 0;
	BOOST_FOREACH(GPlatesQtWidgets::hellinger_model_pair_type pair, d_model_data)
	{
		if (pair.first != last_segment_number)
		{
			last_segment_number = pair.first;
			new_segment_number ++;
		}
		result.insert(hellinger_model_pair_type(new_segment_number,pair.second));
	}

	d_model_data = result;
}

int
GPlatesQtWidgets::HellingerModel::number_of_segments() const
{
	return	unique_keys(d_model_data);
}

bool GPlatesQtWidgets::HellingerModel::segments_are_ordered() const
{

	for (int i = 1; i <= unique_keys(d_model_data) ; ++i)
	{
		if (d_model_data.find(i) == d_model_data.end())
		{
			return false;
		}
	}
	return true;
}

void
GPlatesQtWidgets::HellingerModel::make_space_for_new_segment(int segment)
{
	hellinger_model_type result;
	hellinger_model_range_type range = d_model_data.equal_range(segment-1);

	hellinger_model_type::const_iterator
			iter = d_model_data.begin(),
			iter_end = range.second;
	for (; iter != iter_end; ++iter)
	{
		result.insert(*iter);
	}

	iter = d_model_data.find(segment);

	for (; iter != d_model_data.end(); ++iter)
	{
		result.insert(hellinger_model_pair_type(iter->first+1, iter->second));
	}

	d_model_data = result;
}


GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::get_pick(
		const unsigned int &segment,
		const unsigned int &row) const
{
	hellinger_model_const_range_type range =
			d_model_data.equal_range(segment);

	hellinger_model_type::const_iterator iter = range.first;
	for (unsigned int n = 0; iter != range.second; ++iter, ++n)
	{
		if (n == row)
		{
			return iter;
		}
	}
	return d_model_data.end();
}

GPlatesQtWidgets::hellinger_segment_type
GPlatesQtWidgets::HellingerModel::get_segment(
		const unsigned int &segment_num) const
{
	hellinger_model_type::const_iterator
			iter = segment_begin(segment_num),
			iter_end = segment_end(segment_num);
	hellinger_segment_type segment;
	for (; iter != iter_end; ++iter)
	{
		segment.push_back(iter->second);
	}
	return segment;
}

GPlatesQtWidgets::hellinger_model_const_range_type
GPlatesQtWidgets::HellingerModel::get_segment_as_range(const unsigned int &segment) const
{
	return d_model_data.equal_range(segment);
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::begin() const
{
	return d_model_data.begin();
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::end() const
{
	return d_model_data.end();
}


bool
GPlatesQtWidgets::HellingerModel::segment_number_exists(int segment_num) const
{
	return d_model_data.count(segment_num) > 0;
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::segment_begin(
		const int &segment) const
{
	if (d_model_data.count(segment) > 0)
	{
		return d_model_data.equal_range(segment).first;
	}
	else
	{
		return d_model_data.end();
	}
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::segment_end(
		const int &segment) const
{
	if (d_model_data.count(segment) > 0)
	{
		return d_model_data.equal_range(segment).second;
	}
	else
	{
		return d_model_data.end();
	}
}

