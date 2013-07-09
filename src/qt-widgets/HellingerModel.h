/* $Id: HellingerModel.h 258 2012-03-19 11:52:08Z robin.watson@ngu.no $ */

/**
 * \file 
 * $Revision: 258 $
 * $Date: 2012-03-19 12:52:08 +0100 (Mon, 19 Mar 2012) $ 
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

	enum HellingerSegmentType
	{
		MOVING_SEGMENT_TYPE = 1,
		FIXED_SEGMENT_TYPE,
		DISABLED_MOVING_SEGMENT_TYPE = 31,
		DISABLED_FIXED_SEGMENT_TYPE
	};

	// TODO: should the pick structure contain its segment number? Bear in mind that picks can be re-allocated to
	// different segment numbers.
	struct HellingerPick{
		HellingerSegmentType d_segment_type;
		double d_lat;
		double d_lon;
		double d_uncertainty;
		bool d_is_enabled;
	};

	typedef std::multimap<int,HellingerPick> hellinger_model_type;

	// Contents of a hellinger .com file.
	struct hellinger_com_file_struct{
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
	struct hellinger_fit_struct{
		hellinger_fit_struct(double lat, double lon, double angle, double eps=0):
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
			const QStringList &HellingerPick);

		void
		add_pick(const HellingerPick &pick,
				 const int &segment_number);

        QStringList
        get_line(
			int &segment,
			int &row) const;

        bool
		get_pick_state(
			const int &segment,
			const int &row) const ;

        void
		set_pick_state(
			const int &segment,
			const int &row,
			bool enabled);

		boost::optional<HellingerPick>
		get_pick(
			const int &segment, 
			const int &row) const;

        QStringList
		get_segment_as_string(
			const int &segment) const;

		std::vector<HellingerPick>
		get_segment(
			const int &segment) const;

        int
		num_rows_in_segment(const int &segment);

        void
		remove_pick(
			int &segment,
			int &row);

        void
        remove_segment(
			int &segment);

        void
        reset_model();

        void
		set_fit(
			const QStringList &fields);

		void
		set_fit(
			const hellinger_fit_struct &fields);

		void
		set_com_file_structure(
				const hellinger_com_file_struct &com_file_structure)
		{
			d_active_com_file_struct = com_file_structure;
		}

		hellinger_com_file_struct&
		get_hellinger_com_file_struct()
		{
			return d_active_com_file_struct;
		}

		boost::optional<hellinger_fit_struct>
		get_fit();

        void
		add_data_file();

        std::vector<GPlatesMaths::LatLonPoint>
		get_pick_points() const;

        void
		set_initial_guess(
			const QStringList &com_list_fields);

		boost::optional<hellinger_com_file_struct>
		get_com_file() const;

        QStringList
		get_data_as_string() const;

		hellinger_model_type::const_iterator begin() const;

		hellinger_model_type::const_iterator end() const;

		hellinger_model_type::const_iterator segment_begin(
			const int &segment) const;

		hellinger_model_type::const_iterator segment_end(
			const int &segment) const;

        bool
		segment_number_exists(
			int segment_num) const;

        void
		reorder_segment(
			int segment);

		void
		reorder_picks();

    private:

		void
		reset_com_file_struct();

		void
		reset_fit_struct();

		void
		reset_points();



		hellinger_com_file_struct d_active_com_file_struct;
		boost::optional<hellinger_fit_struct> d_last_result;
		hellinger_model_type d_hellinger_picks;
		std::vector<GPlatesMaths::LatLonPoint> d_points;

		// TODO: check if this path is required.
		QString d_python_path;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERMODEL_H
