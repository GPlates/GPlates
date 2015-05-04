/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GPMLPROPERTYSTRUCTURALTYPEREADERUTILS_H
#define GPLATES_FILEIO_GPMLPROPERTYSTRUCTURALTYPEREADERUTILS_H

#include "ReadErrors.h"

#include "model/PropertyValue.h"
#include "model/XmlNode.h"

// Please keep these ordered alphabetically...
#include "property-values/Enumeration.h"
#include "property-values/GmlDataBlock.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlRectifiedGrid.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlMetadata.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlScalarField3DFile.h"
#include "property-values/GpmlStringList.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"


namespace GPlatesModel
{
	class GpgimEnumerationType;
	class GpgimVersion;
}	

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;
	class GpmlPropertyStructuralTypeReader;

	/**
	 * This namespace contains utilities for reading structural types from GPML files.
	 *
	 * NOTE: It only includes structural types that can be feature properties.
	 * Utilities for 'non-property' structural types can be found in 'GpmlStructuralTypeReaderUtils'.
	 */
	namespace GpmlPropertyStructuralTypeReaderUtils
	{
		//
		// NOTE: In the following functions, 'gpml_version' represents the GPGIM version
		// used when the GPML file (currently being read) was written.
		//
		// This allows us to modify the structural types in newer GPGIM revisions while still
		// allowing older GPML files to be read (into the current/latest GPGIM format).
		//


		//
		// XSI namespace.
		//
		// Any additions here also need to be added to GpmlPropertyStructuralTypeReader.
		//
		// Please keep these ordered alphabetically...


		GPlatesPropertyValues::XsBoolean::non_null_ptr_type
		create_xs_boolean(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::XsDouble::non_null_ptr_type
		create_xs_double(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::XsInteger::non_null_ptr_type
		create_xs_integer(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::XsString::non_null_ptr_type
		create_xs_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		//
		// GML namespace.
		//
		// Any additions here also need to be added to GpmlPropertyStructuralTypeReader.
		//
		// Please keep these ordered alphabetically...


		GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type
		create_gml_data_block(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlFile::non_null_ptr_type
		create_gml_file(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlLineString::non_null_ptr_type
		create_gml_line_string(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type
		create_gml_multi_point(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
		create_gml_orientable_curve(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlPoint::non_null_ptr_type
		create_gml_point(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlPolygon::non_null_ptr_type
		create_gml_polygon(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlRectifiedGrid::non_null_ptr_type
		create_gml_rectified_grid(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		create_gml_time_instant(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
		create_gml_time_period(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		//
		// GPML namespace.
		//
		// Any additions here also need to be added to GpmlPropertyStructuralTypeReader.
		//
		// Please keep these ordered alphabetically...


		GPlatesPropertyValues::GpmlArray::non_null_ptr_type
		create_gpml_array(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
		create_gpml_constant_value(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		/**
		 * Reads the enumeration of the type specified by @a gpgim_property_enumeration_type.
		 *
		 * The GPGIM enumeration type also dictates whether the enumeration value read is allowed or not.
		 */
		GPlatesPropertyValues::Enumeration::non_null_ptr_type
		create_gpml_enumeration(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimEnumerationType &gpgim_property_enumeration_type,
				const GPlatesModel::GpgimVersion &gpml_version,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlFeatureReference::non_null_ptr_type
		create_gpml_feature_reference(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlFeatureSnapshotReference::non_null_ptr_type
		create_gpml_feature_snapshot_reference(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
		create_gpml_finite_rotation(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type
		create_gpml_hot_spot_trail_mark(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
		create_gpml_irregular_sampling(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
		create_gpml_key_value_dictionary(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type
		create_gpml_measure(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlMetadata::non_null_ptr_type
		create_gpml_metadata(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
		create_gpml_old_plates_header(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
		create_gpml_piecewise_aggregation(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GpmlPropertyStructuralTypeReader &structural_type_reader,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type
		create_gpml_plate_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlPolarityChronId::non_null_ptr_type
		create_gpml_polarity_chron_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlRasterBandNames::non_null_ptr_type
		create_gpml_raster_band_names(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlRevisionId::non_null_ptr_type
		create_gpml_revision_id(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlScalarField3DFile::non_null_ptr_type
		create_gpml_scalar_field_3d_file(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlStringList::non_null_ptr_type
		create_gpml_string_list(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type
		create_gpml_topological_line(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTopologicalNetwork::non_null_ptr_type
		create_gpml_topological_network(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);


		GPlatesPropertyValues::GpmlTopologicalPolygon::non_null_ptr_type
		create_gpml_topological_polygon(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &elem,
				const GPlatesModel::GpgimVersion &gpml_version,
				ReadErrorAccumulation &read_errors);
	}
}

#endif // GPLATES_FILEIO_GPMLPROPERTYSTRUCTURALTYPEREADERUTILS_H
