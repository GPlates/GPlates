/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_EXTRACTSCALARFIELD3DFEATUREPROPERTIES_H
#define GPLATES_APP_LOGIC_EXTRACTSCALARFIELD3DFEATUREPROPERTIES_H

#include <boost/optional.hpp>

#include "model/FeatureVisitor.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlScalarField3DFile.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/TextContent.h"
#include "property-values/XsString.h"


namespace GPlatesAppLogic
{
	/**
	 * Returns true if the specified feature is a scalar field feature.
	 */
	bool
	is_scalar_field_3d_feature(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature);

	/**
	 * Returns true if the specified feature collection contains a scalar field feature.
	 */
	bool
	contains_scalar_field_3d_feature(
			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


	/**
	 * Visits a scalar field feature and extracts the following properties from it:
	 *  - GmlFile inside a GpmlConstantValue or a GpmlPiecewiseAggregation
	 *    inside a gpml:filename top-level property.
	 *
	 * NOTE: The properties are extracted at the specified reconstruction time.
	 */
	class ExtractScalarField3DFeatureProperties :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		explicit
		ExtractScalarField3DFeatureProperties(
				const double &reconstruction_time = 0);


		const boost::optional<GPlatesPropertyValues::TextContent> &
		get_scalar_field_filename() const;


		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle);


		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);


		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);


		virtual
		void
		visit_gpml_scalar_field_3d_file(
				const GPlatesPropertyValues::GpmlScalarField3DFile &gpml_scalar_field_3d_file);

	private:
		/**
		 * The reconstruction time at which properties are extracted.
		 */
		GPlatesPropertyValues::GeoTimeInstant d_reconstruction_time;

		/**
		 * The filename.
		 */
		boost::optional<GPlatesPropertyValues::TextContent> d_filename;

		bool d_inside_constant_value;
		bool d_inside_piecewise_aggregation;
	};
}

#endif // GPLATES_APP_LOGIC_EXTRACTSCALARFIELD3DFEATUREPROPERTIES_H
