/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H
#define GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H

#include "XmlOutputInterface.h"
#include "model/ConstFeatureVisitor.h"


namespace GPlatesPropertyValues
{
	class GpmlTimeSample;
}

namespace GPlatesFileIO
{
	class GpmlOnePointFiveOutputVisitor:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		explicit
		GpmlOnePointFiveOutputVisitor(
				const XmlOutputInterface &xoi):
			d_output(xoi) {  }

		virtual
		~GpmlOnePointFiveOutputVisitor() {  }

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_top_level_property_inline(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

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
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);

	private:

		XmlOutputInterface d_output;

		void
		write_gpml_time_sample(
				const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample);

	};
}

#endif  // GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H
