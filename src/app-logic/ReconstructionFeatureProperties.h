/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONFEATUREPROPERTIES_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONFEATUREPROPERTIES_H

#include <boost/optional.hpp>

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/Enumeration.h"


namespace GPlatesAppLogic
{
	/**
	 * A visitor that retrieves commonly used reconstruction parameters from a feature's property values.
	 *
	 * Call 'visit_feature()' on an instance of this class to gather information on the feature.
	 */
	class ReconstructionFeatureProperties :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		/**
		 * Returns true unless a "gml:validTime" property in the feature has
		 * a time period that does not include the specified time.
		 *
		 * The return value defaults to true; it's only set to false if
		 * both: (i) a "gml:validTime" property is encountered which contains a
		 * "gml:TimePeriod" structural type; and (ii) the reconstruction time lies
		 * outside the range of the valid time.
		 */
		bool
		is_feature_defined_at_recon_time(
				const double &reconstruction_time) const;


		/**
		 * Returns optional plate id if "gpml:reconstructionPlateId" property is found.
		 */
		const boost::optional<GPlatesModel::integer_plate_id_type> &
		get_recon_plate_id() const
		{
			return d_recon_plate_id;
		}

		/**
		 * Returns optional plate id if "gpml:rightPlate" property is found.
		 */
		const boost::optional<GPlatesModel::integer_plate_id_type> &
		get_right_plate_id() const
		{
			return d_right_plate_id;
		}

		/**
		 * Returns optional plate id if "gpml:leftPlate" property is found.
		 */
		const boost::optional<GPlatesModel::integer_plate_id_type> &
		get_left_plate_id() const
		{
			return d_left_plate_id;
		}

		/**
		 * Returns optional ridge spreading asymmetry if "gpml:spreadingAsymmetry" property is found.
		 *
		 * Spreading asymmetry is in the range [-1,1] where the value 0 represents half-stage
		 * rotation, the value 1 represents full-stage rotation (right plate) and the value -1
		 * represents zero stage rotation (left plate).
		 */
		const boost::optional<double> &
		get_spreading_asymmetry() const
		{
			return d_spreading_asymmetry;
		}

		/**
		 * Returns optional reconstruction method if "gpml:reconstructionMethod" property is found.
		 */
		const boost::optional<GPlatesPropertyValues::EnumerationContent> &
		get_reconstruction_method() const
		{
			return d_recon_method;
		}


		/**
		 * Returns optional time of appearance if a "gml:validTime" property is found.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		get_time_of_appearance() const
		{
			return d_time_of_appearance;
		}


		/**
		 * Returns optional time of dissappearance if a "gml:validTime" property is found.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		get_time_of_dissappearance() const
		{
			return d_time_of_dissappearance;
		}


		virtual
		void
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double);

		void
		visit_enumeration(
				const enumeration_type &enumeration);

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

	private:

		boost::optional<GPlatesModel::integer_plate_id_type> d_recon_plate_id;
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_appearance;
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_dissappearance;

		boost::optional<GPlatesPropertyValues::EnumerationContent> d_recon_method;
		boost::optional<GPlatesModel::integer_plate_id_type> d_right_plate_id;
		boost::optional<GPlatesModel::integer_plate_id_type> d_left_plate_id;
		boost::optional<double> d_spreading_asymmetry;
	};
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONFEATUREPROPERTIES_H
