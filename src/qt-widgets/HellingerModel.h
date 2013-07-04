/* $Id: HellingerModel.h 258 2012-03-19 11:52:08Z robin.watson@ngu.no $ */

/**
 * \file 
 * $Revision: 258 $
 * $Date: 2012-03-19 12:52:08 +0100 (Mon, 19 Mar 2012) $ 
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
 
#ifndef GPLATES_QTWIDGETS_HELLINGERMODEL_H
#define GPLATES_QTWIDGETS_HELLINGERMODEL_H

#include <vector>


#include <QItemDelegate>
#include <QModelIndex>
#include <QSize>

#include <QTextStream>

#include "boost/optional.hpp"

#include "maths/PointOnSphere.h"
#include "HellingerDialog.h"


namespace GPlatesQtWidgets
{
    enum SegmentType
    {
        MOVING_SEGMENT_TYPE = 1,
		FIXED_SEGMENT_TYPE,
		DISABLED_MOVING_SEGMENT_TYPE = 31,
		DISABLED_FIXED_SEGMENT_TYPE
    };

	struct Pick{
		SegmentType d_segment_type;
		double d_lat;
		double d_lon;
		double d_uncertainty;
		bool d_is_enabled;
	};

    typedef std::multimap<int,Pick> model_type;

	// Contents of a hellinger .com file.
    struct com_file_struct{
		QString d_pick_file;
		double d_lat;	// initial estimate
		double d_lon; // initial estimate
		double d_rho; // initial estimate
		double d_search_radius; //km
		bool d_perform_grid_search;
		double d_significance_level;
		bool d_estimate_kappa;
		bool d_generate_output_files;
		QString d_data_filename;
		QString d_up_filename;
		QString d_down_filename;

    };

	// The result of the fit.
	struct fit_struct{
		fit_struct(double lat, double lon, double angle, double eps=0):
			d_lat(lat),
			d_lon(lon),
			d_angle(angle),
			d_eps(eps)
		{};

		double d_lat;
		double d_lon;
		double d_angle;
		double d_eps;
    };

    class HellingerModel
	{

	public:
        HellingerModel(
			const QString &python_path);

        void
        add_pick(
			const QStringList &Pick);

        void
        reset();

        QStringList
        get_line(
			int &segment,
            int &row);

        bool
		get_pick_state(
			int &segment,
			int &row);

        void
        set_state(
			int &segment,
			int &row);

		boost::optional<Pick>
		get_line(
			const int &segment, 
			const int &row);

        QStringList
        get_segment(
			int &segment);

        int
        segment_number_row(
			int &segment);

        void
        remove_line(
			int &segment,
			int &row);

        void
        remove_segment(
			int &segment);

        void
        reset_model();

        void
        add_results(
			const QStringList &fields);

        boost::optional<fit_struct>
        get_results();

        void
		add_data_file();

        std::vector<GPlatesMaths::LatLonPoint>
		get_points();

        void
        set_initialization_guess(
			const QStringList &com_list_fields);

        void
        reset_com_file_struct();

        void
        reset_fit_struct();

        void
		reset_points();

        void
		reorder_picks();

        void
        set_error_order(
			bool error_order);

        bool
        get_error_order();

        void
        set_error_lat_lon_rho(
			bool error_lat_lon_rho);

        bool
        get_error_lat_lon_rho();

        boost::optional<com_file_struct>
		get_com_file();

        QStringList
        get_data();

        model_type::const_iterator begin() const;

        model_type::const_iterator end() const;

		model_type::const_iterator segment_begin(
			const int &segment) const;

		model_type::const_iterator segment_end(
			const int &segment) const;


        bool
		segment_number_exists(
			int segment_num);

        void
		reorder_segment(
			int segment);

    private:

		com_file_struct d_active_com_file_struct;
		boost::optional<fit_struct> d_fit_struct;
		model_type d_hellinger_picks;                
		std::vector<GPlatesMaths::LatLonPoint> d_points;

		// TODO: these two booleans, and their getter/setters, don't appear
		// to be used. Check if we (will) need these.
		bool d_error_order;
		bool d_error_lat_lon_rho;

		QString d_python_path;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERMODEL_H
