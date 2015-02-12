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

#include <algorithm> // copy
#include <map>
#include <set>
#include <vector>

#include "boost/foreach.hpp"

#include <QDebug>
#include <QDir> // TODO: remove this when file-io moved out of this class.
#include <QFile>
#include <QTextStream>

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
}

GPlatesQtWidgets::HellingerModel::HellingerModel(
        const QString &temporary_file_path):
            d_temporary_file_path(temporary_file_path)
{     
}

QStringList
GPlatesQtWidgets::HellingerModel::get_pick_as_string(
		const unsigned int &segment, const unsigned int &row) const
{

	hellinger_model_const_range_type pair =
			d_model.equal_range(segment);

	hellinger_model_type::const_iterator iter = pair.first;

	QStringList get_data_line;
	for (unsigned int n = 0;  iter != pair.second ; ++iter, ++n)
	{
		if (n == row)
		{
			HellingerPick segment_num = iter->second;
			GPlatesQtWidgets::HellingerPickType move_fix = segment_num.d_segment_type;
			double lat = segment_num.d_lat;
			double lon = segment_num.d_lon;
			double uncert = segment_num.d_uncertainty;
			QString segment_str = QString("%1").arg(iter->first);
			QString move_fix_str = QString("%1").arg(move_fix);
			QString lat_str = QString("%1").arg(lat);
			QString lon_str = QString("%1").arg(lon);
			QString uncert_str = QString("%1").arg(uncert);
			get_data_line << segment_str << move_fix_str << lat_str << lon_str << uncert_str;
		}
	}
	return get_data_line;
}

//boost::optional<const GPlatesQtWidgets::HellingerPick &>
//GPlatesQtWidgets::HellingerModel::get_pick(
//		const unsigned int &index) const
//{
//	if (index >= d_model.size())
//	{
//		return boost::none;
//	}
//	hellinger_model_type::const_iterator it = d_model.begin();
//	std::advance(it, index);
//	return it->second;
//}

void
GPlatesQtWidgets::HellingerModel::set_pick_state(
		const unsigned int &segment,
		const unsigned int &row,
		bool enabled)
{
	hellinger_model_range_type pair =
			d_model.equal_range(segment);

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
			d_model.equal_range(segment);

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
	return d_model.count(segment);
}

QStringList
GPlatesQtWidgets::HellingerModel::get_segment_as_string(
		const unsigned int &segment) const
{
	hellinger_model_const_range_type pair =
			d_model.equal_range(segment);

	hellinger_model_type::const_iterator iter = pair.first;
    QStringList segment_data_values;
	for (; iter != pair.second; ++iter )
    {
			HellingerPick segment_values = iter->second;

			GPlatesQtWidgets::HellingerPickType move_fix = segment_values.d_segment_type;
			double lat = segment_values.d_lat;
			double lon = segment_values.d_lon;
			double uncert = segment_values.d_uncertainty;
			bool is_activated = segment_values.d_is_enabled;
            QString move_fix_str = QString("%1").arg(move_fix);
            QString lat_str = QString("%1").arg(lat);
            QString lon_str = QString("%1").arg(lon);
            QString uncert_str = QString("%1").arg(uncert);
            QString is_activated_str = QString("%1").arg(is_activated);

			segment_data_values << move_fix_str << lat_str << lon_str << uncert_str<< is_activated_str;
    }
    return segment_data_values;
}

void
GPlatesQtWidgets::HellingerModel::remove_pick(
		unsigned const int &segment, unsigned const int &row)
{
	hellinger_model_range_type pair =
			d_model.equal_range(segment);

	hellinger_model_type::iterator iter = pair.first;

	for (unsigned int n = 0;  iter != pair.second ; ++iter, ++n)
    {
		if (n == row)
        {
			d_model.erase(iter);
			return;
        }
    }
}

void
GPlatesQtWidgets::HellingerModel::remove_segment(
		const unsigned int &segment)
{
	d_model.erase(segment);
}

QStringList
GPlatesQtWidgets::HellingerModel::get_data_as_string() const
{
	hellinger_model_type::const_iterator iter;
    QStringList load_data;
	for (iter=d_model.begin(); iter != d_model.end(); ++iter) {
		HellingerPick s = iter->second;
		GPlatesQtWidgets::HellingerPickType move_fix = s.d_segment_type;
		double lat = s.d_lat;
		double lon = s.d_lon;
		double uncert = s.d_uncertainty;
		bool is_enabled = s.d_is_enabled;
        QString segment_str = QString("%1").arg(iter->first);
        QString move_fix_str = QString("%1").arg(move_fix);
        QString lat_str = QString("%1").arg(lat);
        QString lon_str = QString("%1").arg(lon);
        QString uncert_str = QString("%1").arg(uncert);
        QString is_enabled_str = QString("%1").arg(is_enabled);
		load_data << segment_str << move_fix_str << lat_str << lon_str << uncert_str << is_enabled_str;
	}

    return load_data;
}



GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::add_pick(const QStringList &pick)
{
	HellingerPick new_pick;
	if (pick.at(0).toInt() == DISABLED_MOVING_PICK_TYPE)
    {
		new_pick.d_segment_type = MOVING_PICK_TYPE;
    }
	else if (pick.at(0).toInt()== DISABLED_FIXED_PICK_TYPE)
    {
		new_pick.d_segment_type=FIXED_PICK_TYPE;
    }
    else
    {
		new_pick.d_segment_type = static_cast<GPlatesQtWidgets::HellingerPickType>(pick.at(0).toInt());
    }
		new_pick.d_lat = pick.at(2).toDouble();
		new_pick.d_lon = pick.at(3).toDouble();
		new_pick.d_uncertainty = pick.at(4).toDouble();
		if (pick.at(0).toInt() == DISABLED_MOVING_PICK_TYPE)
        {
			new_pick.d_is_enabled = false;
        }
		else if (pick.at(0).toInt() == DISABLED_FIXED_PICK_TYPE)
        {
			new_pick.d_is_enabled = false;
        }
        else
        {
			new_pick.d_is_enabled = true;
        }

		return d_model.insert(hellinger_model_pair_type(pick.at(1).toInt(), new_pick));


}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::add_pick(
		const HellingerPick &pick,
		const unsigned int &segment_number)
{
	return d_model.insert(hellinger_model_pair_type(segment_number,pick));
}

GPlatesMaths::LatLonPoint
GPlatesQtWidgets::HellingerModel::get_initial_pole_llp()
{
	double lat = d_active_com_file_struct.d_lat;
	double lon = d_active_com_file_struct.d_lon;
	return GPlatesMaths::LatLonPoint(lat,lon);
}

double
GPlatesQtWidgets::HellingerModel::get_initial_pole_angle()
{
	return d_active_com_file_struct.d_rho;
}

void
GPlatesQtWidgets::HellingerModel::set_fit(const QStringList &fields)
{	
	d_last_result.reset(HellingerFitStructure(fields.at(0).toDouble(),
											 fields.at(1).toDouble(),
											 fields.at(2).toDouble()));
}

void
GPlatesQtWidgets::HellingerModel::set_fit(
		const HellingerFitStructure &fit)
{
	d_last_result.reset(fit);
}

boost::optional<GPlatesQtWidgets::HellingerFitStructure>
GPlatesQtWidgets::HellingerModel::get_fit()
{
	return d_last_result;
}

void
GPlatesQtWidgets::HellingerModel::set_initial_guess(const QStringList &com_list_fields)
{
	d_active_com_file_struct.d_pick_file = com_list_fields.at(0);
	d_active_com_file_struct.d_lat = com_list_fields.at(1).toDouble();
	d_active_com_file_struct.d_lon = com_list_fields.at(2).toDouble();
	d_active_com_file_struct.d_rho = com_list_fields.at(3).toDouble();
	d_active_com_file_struct.d_search_radius = com_list_fields.at(4).toDouble();
    if (com_list_fields.at(5).toStdString()=="y")
    {
		d_active_com_file_struct.d_perform_grid_search = true;
	}
    else if (com_list_fields.at(5).toStdString()=="n")
    {
		d_active_com_file_struct.d_perform_grid_search = false;
    }
	d_active_com_file_struct.d_significance_level = com_list_fields.at(6).toDouble();
    if (com_list_fields.at(7).toStdString()=="y")
    {
		d_active_com_file_struct.d_estimate_kappa = true;
    }
    else if (com_list_fields.at(7).toStdString()=="n")
    {
		d_active_com_file_struct.d_estimate_kappa = false;
    }
    if (com_list_fields.at(8)=="y")
    {
		d_active_com_file_struct.d_generate_output_files = true;
    }
    else if (com_list_fields.at(8)=="n")
    {
		d_active_com_file_struct.d_generate_output_files = false;
    }
	d_active_com_file_struct.d_data_filename = com_list_fields.at(9);
	d_active_com_file_struct.d_up_filename = com_list_fields.at(10);
	d_active_com_file_struct.d_down_filename = com_list_fields.at(11);
}

void
GPlatesQtWidgets::HellingerModel::set_initial_guess(
		const double &lat,
		const double &lon,
		const double &rho,
		const double &radius)
{
	d_active_com_file_struct.d_lat = lat;
	d_active_com_file_struct.d_lon = lon;
	d_active_com_file_struct.d_rho = rho;
	d_active_com_file_struct.d_search_radius = radius;
}

boost::optional<GPlatesQtWidgets::HellingerComFileStructure>
GPlatesQtWidgets::HellingerModel::get_com_file() const
{
    return d_active_com_file_struct;
}

void
GPlatesQtWidgets::HellingerModel::read_error_ellipse_points()
{
	// TODO:The file-io aspect of this should probably be moved out to
	// the HellingerReader.
    QString path = d_temporary_file_path + QDir::separator() + d_active_com_file_struct.d_data_filename;
    QFile data_file(path);

    if (data_file.open(QFile::ReadOnly))
    {
        QTextStream in(&data_file);
		in.readLine();
        while (!in.atEnd())
        {
            QString line = in.readLine();
            QStringList fields = line.split(" ",QString::SkipEmptyParts);
            GPlatesMaths::LatLonPoint llp(fields.at(1).toDouble(),fields.at(0).toDouble());
			d_error_ellipse_points.push_back(llp);
        }
    }

}

const std::vector <GPlatesMaths::LatLonPoint> &
GPlatesQtWidgets::HellingerModel::get_error_ellipse_points() const
{
	return d_error_ellipse_points;
}

void
GPlatesQtWidgets::HellingerModel::reset_error_ellipse_points()
{
	if (!d_error_ellipse_points.empty())
	{
		d_error_ellipse_points.clear();
	}
}

void
GPlatesQtWidgets::HellingerModel::reset_model()
{
	d_model.clear();
	d_error_ellipse_points.clear();
    reset_com_file_struct();
	reset_fit_struct();
}

void GPlatesQtWidgets::HellingerModel::clear_all_picks()
{
	d_model.clear();
}

void
GPlatesQtWidgets::HellingerModel::reset_com_file_struct()
{
	d_active_com_file_struct.d_pick_file = "";
	d_active_com_file_struct.d_lat = 0;
	d_active_com_file_struct.d_lon = 0;
	d_active_com_file_struct.d_rho = 5.;


	d_active_com_file_struct.d_search_radius = 0.2;
	d_active_com_file_struct.d_perform_grid_search = false;
	d_active_com_file_struct.d_significance_level = 0.95;
	d_active_com_file_struct.d_estimate_kappa = true;
	d_active_com_file_struct.d_generate_output_files = false;
	d_active_com_file_struct.d_data_filename = "";
	d_active_com_file_struct.d_up_filename = "";
	d_active_com_file_struct.d_down_filename = "";
}

void
GPlatesQtWidgets::HellingerModel::reset_fit_struct()
{
	d_last_result.reset();
}

void
GPlatesQtWidgets::HellingerModel::renumber_segments()
{
	hellinger_model_type result;

	int last_segment_number = 0;
	int new_segment_number = 0;
	BOOST_FOREACH(GPlatesQtWidgets::hellinger_model_pair_type pair, d_model)
	{
		if (pair.first != last_segment_number)
		{
			last_segment_number = pair.first;
			new_segment_number ++;
		}
		result.insert(hellinger_model_pair_type(new_segment_number,pair.second));
	}

	d_model = result;
}

bool GPlatesQtWidgets::HellingerModel::segments_are_ordered() const
{

	for (int i = 1; i <= unique_keys(d_model) ; ++i)
	{
		if (d_model.find(i) == d_model.end())
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
	hellinger_model_range_type range = d_model.equal_range(segment-1);

	hellinger_model_type::const_iterator
			iter = d_model.begin(),
			iter_end = range.second;
	for (; iter != iter_end; ++iter)
	{
		result.insert(*iter);
	}

	iter = d_model.find(segment);

	for (; iter != d_model.end(); ++iter)
	{
		result.insert(hellinger_model_pair_type(iter->first+1, iter->second));
	}

	d_model = result;
}


GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::get_pick(
		const unsigned int &segment,
		const unsigned int &row) const
{
	hellinger_model_const_range_type range =
			d_model.equal_range(segment);

	hellinger_model_type::const_iterator iter = range.first;
	for (unsigned int n = 0; iter != range.second; ++iter, ++n)
	{
		if (n == row)
		{
			return iter;
		}
	}
	return d_model.end();
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
	return d_model.equal_range(segment);
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::begin() const
{
	return d_model.begin();
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::end() const
{
	return d_model.end();
}


bool
GPlatesQtWidgets::HellingerModel::segment_number_exists(int segment_num) const
{
	return d_model.count(segment_num) > 0;
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::segment_begin(
	const int &segment) const
{
	if (d_model.count(segment) > 0)
	{
		return d_model.equal_range(segment).first;
	}
	else
	{
		return d_model.end();
	}
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::segment_end(
	const int &segment) const
{
	if (d_model.count(segment) > 0)
	{
		return d_model.equal_range(segment).second;
	}
	else
	{
		return d_model.end();
	}
}

