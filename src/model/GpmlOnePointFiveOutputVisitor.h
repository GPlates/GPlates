/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H
#define GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H

#include "XmlOutputInterface.h"
#include "model/ConstFeatureVisitor.h"


namespace GPlatesFileIO {

	class GpmlOnePointFiveOutputVisitor: public GPlatesModel::ConstFeatureVisitor {

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
		visit_feature_revision(
				const GPlatesModel::FeatureRevision &feature_revision);

		virtual
		void
		visit_gml_line_string(
				const GPlatesModel::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesModel::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_time_instant(
				const GPlatesModel::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gml_time_period(
				const GPlatesModel::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesModel::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesModel::GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_single_valued_property_container(
				const GPlatesModel::SingleValuedPropertyContainer &single_valued_property_container);

		virtual
		void
		visit_xs_string(
				const GPlatesModel::XsString &xs_string);

	private:

		XmlOutputInterface d_output;

	};

}

#endif  // GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H
