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

#include "boost/optional.hpp"

#include "maths/PointOnSphere.h"
#include "HellingerDialog.h"


namespace GPlatesQtWidgets
{

	enum HellingerPickType
	{
		MOVING_PICK_TYPE = 1,
		FIXED_PICK_TYPE,
		DISABLED_MOVING_PICK_TYPE = 31,
		DISABLED_FIXED_PICK_TYPE
	};

	// NOTE: should the pick structure contain its segment number? Bear in mind that picks can be re-allocated to
	// different segment numbers. We've gone for a structure without the segment no.
	// FIXME: the "d_is_enabled" field is not strictly necessary as we encode this already in the
	// HellingerSegmentType. Having both is a little ugly actually, as we need to update both, and there's always
	// the potential for them being out of sync.
	// FIXME: we could of course use a GPlates llp here.

	struct HellingerPick{
		HellingerPick(
				const HellingerPickType &type,
				const double &lat,
				const double &lon,
				const double &uncertainty,
				const bool &enabled):
			d_segment_type(type),
			d_lat(lat),
			d_lon(lon),
			d_uncertainty(uncertainty),
			d_is_enabled(enabled){};
		HellingerPick(){};
		HellingerPickType d_segment_type;
		double d_lat;
		double d_lon;
		double d_uncertainty;
		bool d_is_enabled;
	};

	typedef std::multimap<int,HellingerPick> hellinger_model_type;

	typedef std::vector<HellingerPick> hellinger_segment_type;

	/**
	 * @brief The HellingerComFileStructure struct
	 * This structure mirrors the content of a Hellinger .com file. The Hellinger .com file contains
	 * a list of input parameters to the hellinger1 FORTRAN code.
	 *
	 *
	 * NOTE: The search radius is in RADIANS. (Although the interactive FORTRAN code asks for a
	 * value in degrees, the input value is then converted from radians to degrees).
	 *
	 * The python code behaves as per the fortran code.
	 *
	 * It's probably cleaner to use degrees here, but some of the .com files floating around provide
	 * this value in radians.
	 *
	 * So we maintain radians for now.
	 *
	 * TODO: 1) indicate somewhere on the interface that it's radians that is expected.
	 *		 2) sometime, somehow...let the user choose between the two.
	 *
	 * TODO: consider if "estimate kappa" and "output graphics" should be ditched / phased out / ignored.
	 * We should probably just take these two booleans as TRUE and go ahead and calculate stuff. It'll
	 * save a bit of space in the UI too.
	 */
	struct HellingerComFileStructure{
		HellingerComFileStructure(
				const QString &file,
				const double &lat,
				const double &lon,
				const double &rho,
				const double &radius,
				const bool &grid_search,
				const double &significance,
				const bool &kappa,
				const bool &graphics,
				const QString &file_dat,
				const QString &file_up,
				const QString &file_down):
			d_pick_file(file),
			d_lat(lat),
			d_lon(lon),
			d_rho(rho),
			d_search_radius(radius),
			d_perform_grid_search(grid_search),
			d_significance_level(significance),
			d_estimate_kappa(kappa),
			d_generate_output_files(graphics),
			d_data_filename(file_dat),
			d_up_filename(file_up),
			d_down_filename(file_down){};
		HellingerComFileStructure(){};
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
	struct HellingerFitStructure{
		HellingerFitStructure(double lat, double lon, double angle, double eps=0):
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

	/**
	 * @brief The HellingerModel class
	 *
	 * This class holds the input data for the hellinger fit (picks, initial guess etc) and
	 * the output results (the pole, and associated uncertainty/goodness-of-fit info).
	 *
	 * FIXME: wait a minute, the stats don't seem to be even stored here. I think they should be.
	 * Check where they are stored, and shift them in here.
	 */
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

		void
		add_segment(hellinger_segment_type &picks,
					const int &segment_number);

        QStringList
		get_pick_as_string(
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

		hellinger_segment_type
		get_segment(
			const int &segment) const;

        int
		num_rows_in_segment(const int &segment) const;

        void
		remove_pick(
			const int &segment,
			const int &row);

        void
        remove_segment(
			const int &segment);

        void
        reset_model();

        void
		set_fit(
			const QStringList &fields);

		void
		set_fit(
			const HellingerFitStructure &fields);

		void
		set_com_file_structure(
				const HellingerComFileStructure &com_file_structure)
		{
			d_active_com_file_struct = com_file_structure;
		}

		HellingerComFileStructure&
		get_hellinger_com_file_struct()
		{
			return d_active_com_file_struct;
		}

		boost::optional<HellingerFitStructure>
		get_fit();

        void
		read_error_ellipse_points();

		const std::vector<GPlatesMaths::LatLonPoint> &
		get_error_ellipse_points() const;

		// TODO: this seems to be redundant now. Check this.
        void
		set_initial_guess(
			const QStringList &com_list_fields);

		boost::optional<HellingerComFileStructure>
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

		/**
		 * @brief make_space_for_new_segment
		 * Shifts the segments from @param segment down by one.
		 * @param segment
		 */
        void
		make_space_for_new_segment(
			int segment);

		/**
		 * @brief renumber_segments
		 * Reorganise the model such that segments numbers (i.e. the keys in the model multimap)
		 * are contiguous from 1.
		 */
		void
		renumber_segments();

    private:

		void
		reset_com_file_struct();

		void
		reset_fit_struct();

		void
		reset_points();



		HellingerComFileStructure d_active_com_file_struct;
		boost::optional<HellingerFitStructure> d_last_result;
		hellinger_model_type d_hellinger_picks;
		std::vector<GPlatesMaths::LatLonPoint> d_error_ellipse_points;

		// TODO: check if this path is required.
		QString d_python_path;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERMODEL_H
