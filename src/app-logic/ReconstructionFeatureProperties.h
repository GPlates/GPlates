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

#include "app-logic/ReconstructMethodType.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


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
		 * Constructs a visitor that retrieves reconstruction parameters from a
		 * feature's property values at the given @a reconstruction_time.
		 */
		ReconstructionFeatureProperties(
				const double &reconstruction_time);

		/**
		 * Constructs a visitor that retrieves reconstruction parameters from a
		 * feature's property values that are independent of reconstruction time.
		 */
		ReconstructionFeatureProperties();

		/**
		 * Returns true unless a "gml:validTime" property in the feature has
		 * a time period that does not include the time passed into constructor
		 * taking a reconstruction time.
		 *
		 * If this visitor was constructed without a reconstruction time, this
		 * function always returns true.
		 */
		bool
		is_feature_defined_at_recon_time() const
		{
			return d_feature_is_defined_at_recon_time;
		}


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
		 * Returns optional reconstruction method if "gpml:reonstructionMethod" property is found.
		 */
		const boost::optional<GPlatesAppLogic::ReconstructMethod::Type> &
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
		visit_enumeration(
				const enumeration_type &enumeration);

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);

	private:

		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_recon_time;

		/**
		 * Whether or not the current feature is defined at this reconstruction
		 * time.
		 *
		 * The value of this member defaults to true; it's only set to false if
		 * both: (i) a "gml:validTime" property is encountered which contains a
		 * "gml:TimePeriod" structural type; and (ii) the reconstruction time lies
		 * outside the range of the valid time.
		 */
		bool d_feature_is_defined_at_recon_time;

		boost::optional<GPlatesModel::integer_plate_id_type> d_recon_plate_id;
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_appearance;
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_dissappearance;

		boost::optional<GPlatesAppLogic::ReconstructMethod::Type> d_recon_method;
		boost::optional<GPlatesModel::integer_plate_id_type> d_right_plate_id;
		boost::optional<GPlatesModel::integer_plate_id_type> d_left_plate_id;
	};
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONFEATUREPROPERTIES_H
