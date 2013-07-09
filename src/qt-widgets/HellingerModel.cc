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

#include <map>
#include <vector>

#include <QDebug>
#include <QFile>
#include <QTextStream>

#include "maths/PointOnSphere.h"
#include "HellingerModel.h"

GPlatesQtWidgets::HellingerModel::HellingerModel(
		const QString &python_path):
			d_python_path(python_path)
{     
}

QStringList
GPlatesQtWidgets::HellingerModel::get_line(int &segment, int &row) const
{

	std::pair<hellinger_model_type::const_iterator,hellinger_model_type::const_iterator> pair =
			d_hellinger_picks.equal_range(segment);

	hellinger_model_type::const_iterator iter = pair.first;

	QStringList get_data_line;
	for (int n = 0;  iter != pair.second ; ++iter, ++n)
	{
		if (n == row)
		{
			HellingerPick segment_num = iter->second;
			GPlatesQtWidgets::HellingerSegmentType move_fix = segment_num.d_segment_type;
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

void
GPlatesQtWidgets::HellingerModel::set_pick_state(
		const int &segment,
		const int &row,
		bool enabled)
{
	std::pair<hellinger_model_type::iterator,hellinger_model_type::iterator> pair =
			d_hellinger_picks.equal_range(segment);

	hellinger_model_type::iterator iter = pair.first;

	for (int n = 0 ; iter != pair.second ; ++iter, ++n)
	{
		if (n == row){
			iter->second.d_is_enabled = enabled;
		}
	}
}

bool
GPlatesQtWidgets::HellingerModel::get_pick_state(const int &segment, const int &row) const
{
	std::pair<hellinger_model_type::const_iterator,hellinger_model_type::const_iterator> pair =
			d_hellinger_picks.equal_range(segment);

	hellinger_model_type::const_iterator iter = pair.first;

	for (int n = 0; iter != pair.second; ++iter, ++n)
    {
		if (n == row){
			return iter->second.d_is_enabled;
        }
        n++;
    }
	//Should we really be sending an optional<bool> back from this function, and boost::none if it gets to this point?
	return false;
}

int
GPlatesQtWidgets::HellingerModel::num_rows_in_segment(
		const int &segment)
{
	return d_hellinger_picks.count(segment);
}

QStringList
GPlatesQtWidgets::HellingerModel::get_segment_as_string(
		const int &segment) const
{
	std::pair<hellinger_model_type::const_iterator,hellinger_model_type::const_iterator> pair =
			d_hellinger_picks.equal_range(segment);

	hellinger_model_type::const_iterator iter = pair.first;
    QStringList segment_data_values;
	for (; iter != pair.second; ++iter )
    {
			HellingerPick segment_values = iter->second;

			GPlatesQtWidgets::HellingerSegmentType move_fix = segment_values.d_segment_type;
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
GPlatesQtWidgets::HellingerModel::remove_pick(int &segment, int &row)
{
	std::pair<hellinger_model_type::iterator,hellinger_model_type::iterator> pair =
			d_hellinger_picks.equal_range(segment);

	hellinger_model_type::iterator iter = pair.first;

	for (int n = 0;  iter != pair.second ; ++iter, ++n)
    {
		if (n == row)
        {
            d_hellinger_picks.erase(iter);
			return;
        }
    }
}

void
GPlatesQtWidgets::HellingerModel::remove_segment(int &segment)
{
    d_hellinger_picks.erase(segment);
}

QStringList
GPlatesQtWidgets::HellingerModel::get_data_as_string() const
{
	hellinger_model_type::const_iterator iter;
    QStringList load_data;
    for (iter=d_hellinger_picks.begin(); iter != d_hellinger_picks.end(); ++iter) {
		HellingerPick s = iter->second;
		GPlatesQtWidgets::HellingerSegmentType move_fix = s.d_segment_type;
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



void
GPlatesQtWidgets::HellingerModel::add_pick(const QStringList &pick)
{
	HellingerPick new_pick;
	if (pick.at(0).toInt() == DISABLED_MOVING_SEGMENT_TYPE)
    {
		new_pick.d_segment_type = MOVING_SEGMENT_TYPE;
    }
	else if (pick.at(0).toInt()== DISABLED_FIXED_SEGMENT_TYPE)
    {
		new_pick.d_segment_type=FIXED_SEGMENT_TYPE;
    }
    else
    {
		new_pick.d_segment_type = static_cast<GPlatesQtWidgets::HellingerSegmentType>(pick.at(0).toInt());
    }
		new_pick.d_lat = pick.at(2).toDouble();
		new_pick.d_lon = pick.at(3).toDouble();
		new_pick.d_uncertainty = pick.at(4).toDouble();
		if (pick.at(0).toInt() == DISABLED_MOVING_SEGMENT_TYPE)
        {
			new_pick.d_is_enabled = false;
        }
		else if (pick.at(0).toInt() == DISABLED_FIXED_SEGMENT_TYPE)
        {
			new_pick.d_is_enabled = false;
        }
        else
        {
			new_pick.d_is_enabled = true;
        }

		d_hellinger_picks.insert(std::pair<int, HellingerPick>(pick.at(1).toInt(), new_pick));


}

void
GPlatesQtWidgets::HellingerModel::add_pick(
		const HellingerPick &pick,
		const int &segment_number)
{
	d_hellinger_picks.insert(std::pair<int,HellingerPick>(segment_number,pick));
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

boost::optional<GPlatesQtWidgets::HellingerComFileStructure>
GPlatesQtWidgets::HellingerModel::get_com_file() const
{
    return d_active_com_file_struct;
}

void
GPlatesQtWidgets::HellingerModel::add_data_file()
{

	QString path = d_python_path+d_active_com_file_struct.d_data_filename;
    QFile data_file(path);
    std::vector<GPlatesMaths::LatLonPoint> points;
    if (data_file.open(QFile::ReadOnly))
    {

        QTextStream in(&data_file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            QStringList fields = line.split(" ",QString::SkipEmptyParts);
            GPlatesMaths::LatLonPoint llp(fields.at(1).toDouble(),fields.at(0).toDouble());
            points.push_back(llp);
        }
    }
    d_points = points;
}

std::vector <GPlatesMaths::LatLonPoint>
GPlatesQtWidgets::HellingerModel::get_pick_points() const
{
    return d_points;
}

void
GPlatesQtWidgets::HellingerModel::reset_points()
{
	if (!d_points.empty())
	{
		d_points.clear();
	}
}

void
GPlatesQtWidgets::HellingerModel::reset_model()
{
    d_hellinger_picks.clear();
    d_points.clear();
    reset_com_file_struct();
    reset_fit_struct();
}

void
GPlatesQtWidgets::HellingerModel:: reset_com_file_struct()
{
	d_active_com_file_struct.d_pick_file = "";
	d_active_com_file_struct.d_lat = 0;
	d_active_com_file_struct.d_lon = 0;
	d_active_com_file_struct.d_rho = 0;
	d_active_com_file_struct.d_search_radius = 0;
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
GPlatesQtWidgets::HellingerModel::reorder_picks()
{
	hellinger_model_type::const_iterator iter;
	hellinger_model_type new_map;
    int num_segment = 0;
    int miss_num = 0;
    for (iter=d_hellinger_picks.begin(); iter != d_hellinger_picks.end(); ++iter) {
		HellingerPick pick_part = iter->second;
        if (iter->first>num_segment)
        {
            if (iter->first == num_segment+1)
            {
                ++num_segment;
            }
            else
            {
                ++num_segment;
                ++miss_num;
            }
        }
        else
        {
			new_map.insert(std::pair<int, HellingerPick>(iter->first-miss_num, pick_part));
        }
      }

      d_hellinger_picks = new_map;
}

void
GPlatesQtWidgets::HellingerModel::reorder_segment(int segment)
{
	hellinger_model_type::const_iterator iter;
	hellinger_model_type new_map;
    for (iter=d_hellinger_picks.begin(); iter != d_hellinger_picks.end(); ++iter) {
		HellingerPick pick_part = iter->second;
        if (iter->first>=segment)
        {
			new_map.insert(std::pair<int, HellingerPick>(iter->first+1, pick_part));
        }
        else
        {
			new_map.insert(std::pair<int, HellingerPick>(iter->first, pick_part));
        }
      }

      d_hellinger_picks = new_map;

}

boost::optional<GPlatesQtWidgets::HellingerPick>
GPlatesQtWidgets::HellingerModel::get_pick(
		const int &segment,
		const int &row) const
{
	hellinger_model_type::const_iterator iter;
	int n = 0;
	for (iter = d_hellinger_picks.find(segment); iter != d_hellinger_picks.end(); ++iter )
	{
		if ((iter->first == segment) && (n == row))
		{
            return (iter->second);
		}
		n++;
	}
	return boost::none;
}

std::vector<GPlatesQtWidgets::HellingerPick>
GPlatesQtWidgets::HellingerModel::get_segment(
		const int &segment) const
{
	return std::vector<GPlatesQtWidgets::HellingerPick>();
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::begin() const
{
	return d_hellinger_picks.begin();
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::end() const
{
	return d_hellinger_picks.end();
}


bool
GPlatesQtWidgets::HellingerModel::segment_number_exists(int segment_num) const
{
	return d_hellinger_picks.count(segment_num) > 0;
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::segment_begin(
	const int &segment) const
{
	return d_hellinger_picks.equal_range(segment).first;
}

GPlatesQtWidgets::hellinger_model_type::const_iterator
GPlatesQtWidgets::HellingerModel::segment_end(
	const int &segment) const
{
	return d_hellinger_picks.equal_range(segment).second;
}

