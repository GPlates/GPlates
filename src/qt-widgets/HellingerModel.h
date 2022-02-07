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


const QString DEFAULT_OUTPUT_FILE_EXTENSION(".dat");
const double INITIAL_AMOEBA_TWO_WAY_RESIDUAL = 1e-10;
const double INITIAL_AMOEBA_THREE_WAY_RESIDUAL = 0.005;

namespace GPlatesQtWidgets
{
	enum HellingerFitType
	{
		TWO_PLATE_FIT_TYPE,
		THREE_PLATE_FIT_TYPE
	};

	enum HellingerPlateIndex
	{
		PLATE_ONE_PICK_TYPE = 1,
		PLATE_TWO_PICK_TYPE,
		PLATE_THREE_PICK_TYPE,
		DISABLED_PLATE_ONE_PICK_TYPE = 31,
		DISABLED_PLATE_TWO_PICK_TYPE,
		DISABLED_PLATE_THREE_PICK_TYPE
	};

	enum HellingerPlatePairType
	{
		PLATES_1_2_PAIR_TYPE,
		PLATES_1_3_PAIR_TYPE,
		PLATES_2_3_PAIR_TYPE,
	};

	// NOTE: should the pick structure contain its segment number? Bear in mind that picks can be re-allocated to
	// different segment numbers. We've gone for a structure without the segment no.
	// FIXME: the "d_is_enabled" field is not strictly necessary as we encode this already in the
	// HellingerSegmentType. Having both is a little ugly actually, as we need to update both, and there's always
	// the potential for them being out of sync.

	struct HellingerPick{
		HellingerPick(
				const HellingerPlateIndex &type,
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
		HellingerPlateIndex d_segment_type;
		double d_lat;
		double d_lon;
		double d_uncertainty;
		bool d_is_enabled;
	};

	struct HellingerPoleEstimate{
		HellingerPoleEstimate():
			d_lat(0.),
			d_lon(0.),
			d_angle(5.)
		{}
		HellingerPoleEstimate(
				const double &lat,
				const double &lon,
				const double &angle):
			d_lat(lat),
			d_lon(lon),
			d_angle(angle)
		{}
		double d_lat;
		double d_lon;
		double d_angle;
	};

	typedef std::multimap<int,HellingerPick> hellinger_model_type;

	typedef std::pair<int,HellingerPick> hellinger_model_pair_type;

	typedef std::pair<hellinger_model_type::const_iterator,hellinger_model_type::const_iterator> hellinger_model_const_range_type;

	typedef std::pair<hellinger_model_type::iterator,hellinger_model_type::iterator> hellinger_model_range_type;

	typedef std::vector<HellingerPick> hellinger_segment_type;

	/**
	 * @brief The HellingerComFileStructure struct
	 * This structure mirrors the content of a Hellinger .com file. The Hellinger .com file contains
	 * a list of input parameters to the hellinger1 or hellinger3 FORTRAN code.
	 *
	 * The "estimate kappa" and "output graphics" fields are read into the com structure, but in effect
	 * are not used - the python routines always estimate kappa and generate output graphics.
	 * ("Output graphics" here means the generation of text files containing lat-lon cords of
	 * for example error ellipses)
	 */
	struct HellingerComFileStructure{

		HellingerComFileStructure():
			d_perform_grid_search(false),
			d_use_amoeba_iteration_limit(false),
			d_use_amoeba_tolerance(false),
			d_amoeba_two_way_tolerance(INITIAL_AMOEBA_TWO_WAY_RESIDUAL),
			d_amoeba_three_way_tolerance(INITIAL_AMOEBA_THREE_WAY_RESIDUAL),
			d_estimate_kappa(true),
			d_generate_output_files(true)
		{}
		QString d_pick_file;
		HellingerPoleEstimate d_estimate_12;
		HellingerPoleEstimate d_estimate_13;
		double d_search_radius_degrees;
		bool d_perform_grid_search;
		unsigned int d_number_of_grid_iterations;
		bool d_use_amoeba_iteration_limit;
		unsigned int d_number_amoeba_iterations;
		bool d_use_amoeba_tolerance;
		double d_amoeba_two_way_tolerance;
		double d_amoeba_three_way_tolerance;
		double d_significance_level;
		bool d_estimate_kappa;
		bool d_generate_output_files;

		// NOTE: for three-way fitting results, we have the 3 combinations of
		// plate-pairs (12,13,23) and for each pair we have both simultaneous and individual
		// results. And for each of these combinations we have 3 types of output: ellipse,
		// upper surface and lower surface. That makes 18 files in total. Rather than keep track
		// of 18 user-provided output filenames,it is simpler to take a file root name
		// (based on the input pick file name for example, or provided by the user in the UI)
		// and add suitable extensions to differentiate the various output forms.
		QString d_error_ellipse_filename_12;
		QString d_upper_surface_filename_12;
		QString d_lower_surface_filename_12;
		QString d_error_ellipse_filename_13;
		QString d_upper_surface_filename_13;
		QString d_lower_surface_filename_13;
		QString d_error_ellipse_filename_23;
		QString d_upper_surface_filename_23;
		QString d_lower_surface_filename_23;
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

		HellingerModel();

		hellinger_model_type::const_iterator
		add_pick(const HellingerPick &pick,
				 const unsigned int &segment_number);

		void
		add_segment(hellinger_segment_type &picks,
					const unsigned int &segment_number);

		//		boost::optional<const HellingerPick &> get_pick(
		//			const unsigned int &index) const;

		hellinger_model_type::const_iterator
		get_pick(
				const unsigned int &segment,
				const unsigned int &row) const;

		bool
		pick_is_enabled(
				const unsigned int &segment,
				const unsigned int &row) const ;

		void
		set_pick_state(
				const unsigned int &segment,
				const unsigned int &row,
				bool enabled);

		hellinger_segment_type
		get_segment(
				const unsigned int &segment) const;

		hellinger_model_const_range_type
		get_segment_as_range(
				const unsigned int &segment) const;

		int
		num_rows_in_segment(
				const unsigned int &segment) const;

		void
		remove_pick(
				const unsigned int &segment,
				const unsigned int &row);

		void
		remove_segment(
				const unsigned int &segment);

		void
		reset_model();

		void
		clear_all_picks();

		void
		clear_fit_results();

		void
		clear_uncertainty_results();

		void
		set_fit_12(
				const HellingerFitStructure &fit_12);

		void
		set_fit_13(
				const HellingerFitStructure &fit_12);

		void
		set_fit_23(
				const HellingerFitStructure &fit_12);

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
		get_fit_12();

		boost::optional<HellingerFitStructure>
		get_fit_13();

		boost::optional<HellingerFitStructure>
		get_fit_23();

		std::vector<GPlatesMaths::LatLonPoint> &
		error_ellipse_points(
				const HellingerPlatePairType &type = PLATES_1_2_PAIR_TYPE);

		HellingerPoleEstimate
		get_initial_guess_12() const;

		HellingerPoleEstimate
		get_initial_guess_13() const;

		void
		set_initial_guess_12(
				const HellingerPoleEstimate &estimate);

		void
		set_initial_guess_13(
				const HellingerPoleEstimate &estimate);


		void
		set_initial_guess_12(
				const double &lat,
				const double &lon,
				const double &rho);

		void
		set_initial_guess_13(
				const double &lat,
				const double &lon,
				const double &rho);

		void
		set_search_radius(
				const double &radius);

		double
		get_search_radius() const
		{
			return d_active_com_file_struct.d_search_radius_degrees;
		}



		void
		set_confidence_level(const double &conf)
		{
			d_active_com_file_struct.d_significance_level = conf;
		}

		double
		get_confidence_level() const
		{
			return d_active_com_file_struct.d_significance_level;
		}

		int
		get_grid_iterations() const
		{
			return d_active_com_file_struct.d_number_of_grid_iterations;
		}

		bool
		get_grid_search() const
		{
			return d_active_com_file_struct.d_perform_grid_search;
		}

		void
		set_number_of_amoeba_iterations(
				const unsigned int &iterations)
		{
			d_active_com_file_struct.d_number_amoeba_iterations = iterations;
		}

		unsigned int
		get_amoeba_iterations() const
		{
			return d_active_com_file_struct.d_number_amoeba_iterations;
		}

		double
		get_amoeba_tolerance() const
		{
			return (d_fit_type == TWO_PLATE_FIT_TYPE) ?
						d_active_com_file_struct.d_amoeba_two_way_tolerance :
						d_active_com_file_struct.d_amoeba_three_way_tolerance;
		}

		double
		get_amoeba_two_way_tolerance() const
		{
			return d_active_com_file_struct.d_amoeba_two_way_tolerance;
		}

		double
		get_amoeba_three_way_tolerance() const
		{
			return d_active_com_file_struct.d_amoeba_three_way_tolerance;
		}

		void
		set_amoeba_two_way_tolerance(
				const double &tolerance)
		{
			d_active_com_file_struct.d_amoeba_two_way_tolerance = tolerance;
		}

		void
		set_amoeba_three_way_tolerance(
				const double &tolerance)
		{
			d_active_com_file_struct.d_amoeba_three_way_tolerance = tolerance;
		}

		void
		set_amoeba_tolerance(
				const double &tolerance)
		{
			switch(d_fit_type)
			{
			case TWO_PLATE_FIT_TYPE:
				d_active_com_file_struct.d_amoeba_two_way_tolerance = tolerance;
				break;
			case THREE_PLATE_FIT_TYPE:
				d_active_com_file_struct.d_amoeba_three_way_tolerance = tolerance;
				break;
			default:
				d_active_com_file_struct.d_amoeba_two_way_tolerance = tolerance;
			}
		}

		void
		set_amoeba_tolerance(
				const double &tolerance,
				const HellingerFitType &type)
		{
			switch(type)
			{
			case TWO_PLATE_FIT_TYPE:
				d_active_com_file_struct.d_amoeba_two_way_tolerance = tolerance;
				break;
			case THREE_PLATE_FIT_TYPE:
				d_active_com_file_struct.d_amoeba_three_way_tolerance = tolerance;
				break;
			default:
				d_active_com_file_struct.d_amoeba_two_way_tolerance = tolerance;
			}
		}

		bool
		get_use_amoeba_iterations() const
		{
			return d_active_com_file_struct.d_use_amoeba_iteration_limit;
		}

		void
		set_use_amoeba_iterations(
				bool use)
		{
			d_active_com_file_struct.d_use_amoeba_iteration_limit = use;
		}

		bool
		get_use_amoeba_tolerance() const
		{
			return d_active_com_file_struct.d_use_amoeba_tolerance;
		}

		void
		set_use_amoeba_tolerance(
				bool use)
		{
			d_active_com_file_struct.d_use_amoeba_tolerance = use;
		}


		void
		set_estimate_kappa(bool estimate)
		{
			d_active_com_file_struct.d_estimate_kappa = estimate;
		}

		void
		set_input_pick_filename(
				const QString &input_filename);

		void
		set_fit_type(
				const HellingerFitType &type);


		const HellingerFitType &
		get_fit_type(
				bool update = false);

		// TODO: don't think we need this as optional.... check.
		boost::optional<HellingerComFileStructure>
		get_com_file() const;

		QString
		get_pick_filename() const
		{
			return d_active_com_file_struct.d_pick_file;
		}

		QString
		get_chron_string() const
		{
			return d_chron_string;
		}

		void
		set_chron_string(
				const QString &chron_string)
		{
			d_chron_string = chron_string;
		}

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
		 * are contiguous from 1. Assumes that segment numbers are >= 1.
		 */
		void
		renumber_segments();

		int
		number_of_segments() const;

		bool
		segments_are_ordered() const;

		void
		clear_error_ellipse(
				const HellingerPlatePairType &type = PLATES_1_2_PAIR_TYPE);

		void clear_error_ellipses();

		QString
		error_ellipse_filename() const;

		QString
		error_ellipse_filename(
				const HellingerPlatePairType &type) const;

		bool
		picks_are_valid() const;

		void
		set_output_file_root(
				const QString &root);

		QString
		output_file_root() const;

		const hellinger_model_type &
		model_data() const
		{
			return d_model_data;
		}

		void
		set_model_data(
				const hellinger_model_type &model_data_)
		{
			d_model_data = model_data_;
		}

	private:

		void
		clear_com_file_struct();

		HellingerComFileStructure d_active_com_file_struct;

		boost::optional<HellingerFitStructure> d_last_fit_12_result;
		boost::optional<HellingerFitStructure> d_last_fit_13_result;
		boost::optional<HellingerFitStructure> d_last_fit_23_result;
		hellinger_model_type d_model_data;
		std::vector<GPlatesMaths::LatLonPoint> d_error_ellipse_points;
		std::vector<GPlatesMaths::LatLonPoint> d_error_ellipse_12_points;
		std::vector<GPlatesMaths::LatLonPoint> d_error_ellipse_13_points;
		std::vector<GPlatesMaths::LatLonPoint> d_error_ellipse_23_points;

		QString d_chron_string;

		/**
		 * @brief d_fit_type. The desired type of fit - 2-plate or 3-plate.
		 */
		HellingerFitType d_fit_type;

		QString d_output_file_root;

	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERMODEL_H
