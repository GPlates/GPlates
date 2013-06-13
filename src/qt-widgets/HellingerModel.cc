/* $Id: HellingerModel.cc 257 2012-03-01 14:12:46Z robin.watson@ngu.no $ */

/**
 * \file 
 * $Revision: 257 $
 * $Date: 2012-03-01 15:12:46 +0100 (Thu, 01 Mar 2012) $ 
 * 
 * Copyright (C) 2011, 2012 Geological Survey of Norway
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
            d_error_order(true),
            d_error_lat_lon_rho(true),
			d_python_path(python_path)
{     
}

QStringList
GPlatesQtWidgets::HellingerModel::get_line(int &segment, int &row)
{
	model_type::const_iterator iter;
	int n = 0;
	QStringList get_data_line;
	for (iter = d_hellinger_picks.find(segment); iter != d_hellinger_picks.end(); ++iter )
	{
		if (iter->first == segment && n == row)
		{
			Pick segment_num = iter->second;
			GPlatesQtWidgets::SegmentType move_fix = segment_num.segment_type;
			double lat = segment_num.lat;
			double lon = segment_num.lon;
			double uncert = segment_num.uncert;
			QString segment_str = QString("%1").arg(iter->first);
			QString move_fix_str = QString("%1").arg(move_fix);
			QString lat_str = QString("%1").arg(lat);
			QString lon_str = QString("%1").arg(lon);
			QString uncert_str = QString("%1").arg(uncert);
			get_data_line << segment_str<<move_fix_str<<lat_str<<lon_str<<uncert_str;
		}
		n++;
	}
	return get_data_line;
}

void
GPlatesQtWidgets::HellingerModel::set_state(int &segment, int &row)
{

}

bool
GPlatesQtWidgets::HellingerModel::get_state(int &segment,int &row)
{
    model_type::const_iterator iter;
    int n = 0;
    bool is_enabled = false;
    for (iter = d_hellinger_picks.find(segment); iter != d_hellinger_picks.end(); ++iter )
    {
        if (iter->first == segment && n == row)
        {
         Pick segment_num = iter->second;
         is_enabled = segment_num.is_enabled;

        }
        n++;
    }
    return is_enabled;
}

int
GPlatesQtWidgets::HellingerModel::segment_number_row(int &segment)
{
    int n = 0;
    model_type::const_iterator iter;
    for (iter = d_hellinger_picks.find(segment); iter != d_hellinger_picks.end(); ++iter )
    {
        if (iter->first == segment)
        {
            n++;
        }
     }
    return n;
}


QStringList
GPlatesQtWidgets::HellingerModel::get_segment(int &segment)
{
    model_type::const_iterator iter;
    QStringList segment_data_values;
    for (iter = d_hellinger_picks.find(segment); iter != d_hellinger_picks.end(); ++iter )
    {
        if (iter->first == segment)
        {
            Pick segment_values = iter->second;

            GPlatesQtWidgets::SegmentType move_fix = segment_values.segment_type;
            double lat = segment_values.lat;
            double lon = segment_values.lon;
            double uncert = segment_values.uncert;
            bool is_activated = segment_values.is_enabled;
            QString move_fix_str = QString("%1").arg(move_fix);
            QString lat_str = QString("%1").arg(lat);
            QString lon_str = QString("%1").arg(lon);
            QString uncert_str = QString("%1").arg(uncert);
            QString is_activated_str = QString("%1").arg(is_activated);

            segment_data_values << move_fix_str << lat_str << lon_str << uncert_str<<is_activated_str;
        }
    }
    return segment_data_values;
}

void
GPlatesQtWidgets::HellingerModel::remove_line(int &segment, int &row)
{
    model_type::iterator iter;
    int n = 0;

    for (iter = d_hellinger_picks.find(segment); iter != d_hellinger_picks.end(); ++iter )
    {
        if (iter->first == segment && n == row)
        {
            d_hellinger_picks.erase(iter);
        }
        n++;
    }
}

void
GPlatesQtWidgets::HellingerModel::remove_segment(int &segment)
{
    d_hellinger_picks.erase(segment);
}

QStringList
GPlatesQtWidgets::HellingerModel::get_data()
{
    model_type::const_iterator iter;
    QStringList load_data;
    for (iter=d_hellinger_picks.begin(); iter != d_hellinger_picks.end(); ++iter) {
        Pick s = iter->second;
        GPlatesQtWidgets::SegmentType move_fix = s.segment_type;
        double lat = s.lat;
        double lon = s.lon;
        double uncert = s.uncert;
        bool is_enabled = s.is_enabled;
        QString segment_str = QString("%1").arg(iter->first);
        QString move_fix_str = QString("%1").arg(move_fix);
        QString lat_str = QString("%1").arg(lat);
        QString lon_str = QString("%1").arg(lon);
        QString uncert_str = QString("%1").arg(uncert);
        QString is_enabled_str = QString("%1").arg(is_enabled);
        load_data << segment_str<<move_fix_str<<lat_str<<lon_str<<uncert_str<<is_enabled_str;
           }

    return load_data;
}



void
GPlatesQtWidgets::HellingerModel::add_pick(QStringList &pick)
{
	Pick new_pick;
    if (pick.at(0).toInt() == COMMENT_MOVING_SEGMENT_TYPE)
    {
        new_pick.segment_type = MOVING_SEGMENT_TYPE;
    }
    else if (pick.at(0).toInt()==COMMENT_FIXED_SEGMENT_TYPE)
    {
        new_pick.segment_type=FIXED_SEGMENT_TYPE;
    }
    else
    {
        new_pick.segment_type = static_cast<GPlatesQtWidgets::SegmentType>(pick.at(0).toInt());
    }
        new_pick.lat = pick.at(2).toDouble();
        new_pick.lon = pick.at(3).toDouble();
        new_pick.uncert = pick.at(4).toDouble();
        if (pick.at(0).toInt() == COMMENT_MOVING_SEGMENT_TYPE)
        {
            new_pick.is_enabled = false;
        }
        else if (pick.at(0).toInt() == COMMENT_FIXED_SEGMENT_TYPE)
        {
            new_pick.is_enabled = false;
        }
        else
        {
            new_pick.is_enabled = true;
        }

		d_hellinger_picks.insert(std::pair<int, Pick>(pick.at(1).toInt(), new_pick));


}

void
GPlatesQtWidgets::HellingerModel::add_results(QStringList &fields)
{

      d_fit_struct.lat = fields.at(0).toDouble();
      d_fit_struct.lon = fields.at(1).toDouble();
      d_fit_struct.angle = fields.at(2).toDouble();
//      d_fit_struct.eps = fields.at(3);

}

void
GPlatesQtWidgets::HellingerModel::set_error_order(bool error_order)
{
    d_error_order = error_order;
}

bool
GPlatesQtWidgets::HellingerModel::get_error_order()
{
    return d_error_order;
}

void
GPlatesQtWidgets::HellingerModel::set_error_lat_lon_rho(bool error_lat_lon_rho)
{
    d_error_lat_lon_rho = error_lat_lon_rho;
}

bool
GPlatesQtWidgets::HellingerModel::get_error_lat_lon_rho()
{
    return d_error_lat_lon_rho;
}

boost::optional<GPlatesQtWidgets::fit_struct>
GPlatesQtWidgets::HellingerModel::get_results()
{
	// FIXME: return optional<> as appropriate.
    return d_fit_struct;
}

void
GPlatesQtWidgets::HellingerModel::set_initialization_guess(QStringList &com_list_fields)
{
    d_active_com_file_struct.pick_file = com_list_fields.at(0);
    d_active_com_file_struct.lat = com_list_fields.at(1).toDouble();
    d_active_com_file_struct.lon = com_list_fields.at(2).toDouble();
    d_active_com_file_struct.rho = com_list_fields.at(3).toDouble();
    d_active_com_file_struct.search_radius = com_list_fields.at(4).toDouble();
    if (com_list_fields.at(5).toStdString()=="y")
    {
        d_active_com_file_struct.grid_search = true;
    }
    else if (com_list_fields.at(5).toStdString()=="n")
    {
        d_active_com_file_struct.grid_search = false;
    }
    d_active_com_file_struct.significance_level = com_list_fields.at(6).toDouble();
    if (com_list_fields.at(7).toStdString()=="y")
    {
        d_active_com_file_struct.estimated_kappa = true;
    }
    else if (com_list_fields.at(7).toStdString()=="n")
    {
        d_active_com_file_struct.estimated_kappa = false;
    }
    if (com_list_fields.at(8)=="y")
    {
        d_active_com_file_struct.output_files = true;
    }
    else if (com_list_fields.at(8)=="n")
    {
        d_active_com_file_struct.output_files = false;
    }
    d_active_com_file_struct.dat_file = com_list_fields.at(9);
    d_active_com_file_struct.up_file = com_list_fields.at(10);
    d_active_com_file_struct.do_file = com_list_fields.at(11);
}

boost::optional<GPlatesQtWidgets::com_file_struct>
GPlatesQtWidgets::HellingerModel::get_initialization_guess()
{
    return d_active_com_file_struct;
}

void
GPlatesQtWidgets::HellingerModel::add_dat_file()
{

    QString path = d_python_path+d_active_com_file_struct.dat_file;
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
GPlatesQtWidgets::HellingerModel::get_data_file()
{
    return d_points;
}

void
GPlatesQtWidgets::HellingerModel::reset_data_file()
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
GPlatesQtWidgets::HellingerModel::reset_com_file_struct()
{
    d_active_com_file_struct.pick_file = "";
    d_active_com_file_struct.lat = 0;
    d_active_com_file_struct.lon = 0;
    d_active_com_file_struct.rho = 0;
    d_active_com_file_struct.search_radius = 0;
    d_active_com_file_struct.grid_search = false;
    d_active_com_file_struct.significance_level = 0.95;
    d_active_com_file_struct.estimated_kappa = true;
    d_active_com_file_struct.output_files = false;
    d_active_com_file_struct.dat_file = "";
    d_active_com_file_struct.up_file = "";
    d_active_com_file_struct.do_file = "";
}

void
GPlatesQtWidgets::HellingerModel::reset_fit_struct()
{
    d_fit_struct.lat = 0;
    d_fit_struct.lon = 0;
    d_fit_struct.angle = 0;
    d_fit_struct.eps = 0;
}

void
GPlatesQtWidgets::HellingerModel::reorder_picks()
{
    model_type::const_iterator iter;
    model_type new_map;
    int num_segment = 0;
    int miss_num = 0;
    for (iter=d_hellinger_picks.begin(); iter != d_hellinger_picks.end(); ++iter) {
        Pick pick_part = iter->second;
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
			new_map.insert(std::pair<int, Pick>(iter->first-miss_num, pick_part));
        }
      }
      d_hellinger_picks.clear();
      d_hellinger_picks = new_map;
      new_map.clear();
}

void
GPlatesQtWidgets::HellingerModel::reordering_segments(int &segment)
{
    model_type::const_iterator iter;
    model_type new_map;
    for (iter=d_hellinger_picks.begin(); iter != d_hellinger_picks.end(); ++iter) {
        Pick pick_part = iter->second;
        if (iter->first>=segment)
        {
			new_map.insert(std::pair<int, Pick>(iter->first+1, pick_part));
        }
        else
        {
			new_map.insert(std::pair<int, Pick>(iter->first, pick_part));
        }
      }
      d_hellinger_picks.clear();
      d_hellinger_picks = new_map;
      new_map.clear();
}

boost::optional<GPlatesQtWidgets::Pick>
GPlatesQtWidgets::HellingerModel::get_line(
		const int &segment,
		const int &row)
{
    model_type::const_iterator iter;
	int n = 0;
	for (iter = d_hellinger_picks.find(segment); iter != d_hellinger_picks.end(); ++iter )
	{
		if (iter->first == segment && n == row)
		{
            return (iter->second);
		}
		n++;
	}
	return boost::none;
}

GPlatesQtWidgets::model_type::const_iterator
GPlatesQtWidgets::HellingerModel::begin() const
{
	return d_hellinger_picks.begin();
}

GPlatesQtWidgets::model_type::const_iterator
GPlatesQtWidgets::HellingerModel::end() const
{
	return d_hellinger_picks.end();
}


bool
GPlatesQtWidgets::HellingerModel::segment_number_exists(int segment_num)
{
    model_type::const_iterator iter;
    int n = 0;
    for (iter = d_hellinger_picks.begin(); iter != d_hellinger_picks.end(); ++iter )
    {
        if (iter->first == segment_num)
        {
            return true;
        }
        n++;
    }
    return false;
}

GPlatesQtWidgets::model_type::const_iterator
GPlatesQtWidgets::HellingerModel::segment_begin(
	const int &segment) const
{
	return d_hellinger_picks.equal_range(segment).first;
}

GPlatesQtWidgets::model_type::const_iterator
GPlatesQtWidgets::HellingerModel::segment_end(
	const int &segment) const
{
	return d_hellinger_picks.equal_range(segment).second;
}

ENABLE_GCC_WARNING("-Wwrite-strings")
