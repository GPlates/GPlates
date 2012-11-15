/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <boost/bind.hpp>
#include <QDebug>

#include "GpmlPropertyStructuralTypeReader.h"

#include "GpmlPropertyStructuralTypeReaderUtils.h"

#include "model/Gpgim.h"
#include "model/GpgimEnumerationType.h"
#include "model/GpgimStructuralType.h"

#include "utils/UnicodeStringUtils.h"

#include <boost/foreach.hpp>

using GPlatesPropertyValues::StructuralType;


GPlatesFileIO::GpmlPropertyStructuralTypeReader::non_null_ptr_type
GPlatesFileIO::GpmlPropertyStructuralTypeReader::create(
		const GPlatesModel::Gpgim &gpgim)
{
	non_null_ptr_type gpml_property_structural_type_reader = create_empty(gpgim);

	// Add all property structural types.
	gpml_property_structural_type_reader->add_all_structural_types();

	return gpml_property_structural_type_reader;
}


GPlatesFileIO::GpmlPropertyStructuralTypeReader::GpmlPropertyStructuralTypeReader(
		const GPlatesModel::Gpgim &gpgim) :
	d_gpgim(&gpgim)
{
	// In ".cc" file because constructor of GPlatesUtils::non_null_intrusive_ptr requires complete type.
}


GPlatesFileIO::GpmlPropertyStructuralTypeReader::~GpmlPropertyStructuralTypeReader()
{
	// In ".cc" file because destructor of GPlatesUtils::non_null_intrusive_ptr requires complete type.
}


void
GPlatesFileIO::GpmlPropertyStructuralTypeReader::add_all_structural_types()
{
	//
	// Add the 'time-dependent wrapper' property structural types.
	//

	add_time_dependent_wrapper_structural_types();

	//
	// Add the 'native' property structural types.
	//

	add_native_structural_types();

	//
	// Add the 'enumeration' property structural types.
	//
	// The enumeration properties differ from the above native property types in that their
	// definitions are not hard coded but instead declared in the GPGIM XML file.

	add_enumeration_structural_types();

	//
	// Make sure we have handled all property structural types specified by the GPGIM.
	//

	// This actually includes the enumerations defined by the GPGIM so, strictly speaking, we don't
	// need to test them against the GPGIM but it's easier just to loop over all structural types.
	const GPlatesModel::Gpgim::property_structural_type_seq_type &gpgim_property_structural_types =
			d_gpgim->get_property_structural_types();

	BOOST_FOREACH(
		const GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type &gpgim_property_structural_type,
		gpgim_property_structural_types)
	{
		// The structural type should be in our map.
		if (d_structural_type_reader_map.find(gpgim_property_structural_type->get_structural_type()) ==
			d_structural_type_reader_map.end())
		{
			// Perhaps should throw an exception - but for now will just log a warning.
			qWarning() << "Encountered GPGIM property structural type '"
				<< convert_qualified_xml_name_to_qstring(gpgim_property_structural_type->get_structural_type())
				<< "' that the GPML file reader does not recognise.";
		}
	}
}


boost::optional<GPlatesFileIO::GpmlPropertyStructuralTypeReader::structural_type_reader_function_type>
GPlatesFileIO::GpmlPropertyStructuralTypeReader::get_structural_type_reader_function(
		const GPlatesPropertyValues::StructuralType &structural_type) const
{
	structural_type_reader_map_type::const_iterator structural_type_iter =
			d_structural_type_reader_map.find(structural_type);
	if (structural_type_iter == d_structural_type_reader_map.end())
	{
		// Did not find the specified structural type.
		return boost::none;
	}

	return structural_type_iter->second;
}


void
GPlatesFileIO::GpmlPropertyStructuralTypeReader::add_time_dependent_wrapper_structural_types()
{
	d_structural_type_reader_map[StructuralType::create_gpml("ConstantValue")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_constant_value, _1, boost::cref(*this), _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("IrregularSampling")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_irregular_sampling, _1, boost::cref(*this), _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("PiecewiseAggregation")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_piecewise_aggregation, _1, boost::cref(*this), _2, _3);
}


void
GPlatesFileIO::GpmlPropertyStructuralTypeReader::add_native_structural_types()
{
	//
	// XSI namespace.
	//
	// Please keep these ordered alphabetically (by structural type name)...

	d_structural_type_reader_map[StructuralType::create_xsi("boolean")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_xs_boolean, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_xsi("double")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_xs_double, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_xsi("integer")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_xs_integer, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_xsi("string")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_xs_string, _1, _2, _3);

	//
	// GML namespace.
	//
	// Please keep these ordered alphabetically (by structural type name)...

	d_structural_type_reader_map[StructuralType::create_gml("File")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_file, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gml("LineString")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_line_string, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gml("MultiPoint")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_multi_point, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gml("OrientableCurve")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_orientable_curve, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gml("Point")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_point, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gml("Polygon")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_polygon, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gml("RectifiedGrid")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_rectified_grid, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gml("TimeInstant")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_time_instant, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gml("TimePeriod")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gml_time_period, _1, _2, _3);

	//
	// GPML namespace (non-enumeration types).
	//
	// Please keep these ordered alphabetically (by structural type name)...

	d_structural_type_reader_map[StructuralType::create_gpml("Array")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_array, _1, boost::cref(*this), _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("FeatureReference")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_feature_reference, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("FeatureSnapshotReference")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_feature_snapshot_reference, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("FiniteRotation")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_finite_rotation, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("HotSpotTrailMark")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_hot_spot_trail_mark, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("KeyValueDictionary")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_key_value_dictionary, _1, boost::cref(*this), _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("measure")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_measure, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("OldPlatesHeader")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_old_plates_header, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("plateId")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_plate_id, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("PolarityChronId")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_polarity_chron_id, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("RasterBandNames")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_raster_band_names, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("revisionId")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_revision_id, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("ScalarField3DFile")] =
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_scalar_field_3d_file, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("StringList")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_string_list, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("TopologicalLine")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_topological_line, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("TopologicalNetwork")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_topological_network, _1, _2, _3);

	d_structural_type_reader_map[StructuralType::create_gpml("TopologicalPolygon")] = 
			boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
					&GpmlPropertyStructuralTypeReaderUtils::create_gpml_topological_polygon, _1, _2, _3);
}


void
GPlatesFileIO::GpmlPropertyStructuralTypeReader::add_enumeration_structural_types()
{
	const GPlatesModel::Gpgim::property_enumeration_type_seq_type &gpgim_property_enumeration_types =
			d_gpgim->get_property_enumeration_types();

	BOOST_FOREACH(
			const GPlatesModel::GpgimEnumerationType::non_null_ptr_to_const_type &gpgim_property_enumeration_type,
			gpgim_property_enumeration_types)
	{
		d_structural_type_reader_map[gpgim_property_enumeration_type->get_structural_type()] = 
				boost::bind<GPlatesModel::PropertyValue::non_null_ptr_type>(
						&GpmlPropertyStructuralTypeReaderUtils::create_gpml_enumeration,
						_1,
						boost::cref(*gpgim_property_enumeration_type),
						_2,
						_3);
	}
}


void
GPlatesFileIO::GpmlPropertyStructuralTypeReader::add_structural_type(
		const GPlatesPropertyValues::StructuralType &structural_type,
		const structural_type_reader_function_type &reader_function)
{
	d_structural_type_reader_map[structural_type] = reader_function;
}
