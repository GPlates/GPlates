/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <sstream>
#include <boost/noncopyable.hpp>

#include "PyGPlatesModule.h"
#include "PythonConverterUtils.h"

#include "global/python.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"

#include "property-values/EnumerationType.h"
#include "property-values/StructuralType.h"
#include "property-values/ValueObjectType.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	// Only allow the single argument version of the creation function.
	// There's no need for the user to also specify the namespace alias.
	template <class QualifiedXmlNameType>
	const QualifiedXmlNameType
	qualified_xml_name_create_gpgim(
			const QString &name)
	{
		// Use the single argument overload.
		return QualifiedXmlNameType::create_gpgim(name);
	}

	// Only allow the single argument version of the creation function.
	// There's no need for the user to also specify the namespace alias.
	template <class QualifiedXmlNameType>
	const QualifiedXmlNameType
	qualified_xml_name_create_gpml(
			const QString &name)
	{
		// Use the single argument overload.
		return QualifiedXmlNameType::create_gpml(name);
	}

	// Only allow the single argument version of the creation function.
	// There's no need for the user to also specify the namespace alias.
	template <class QualifiedXmlNameType>
	const QualifiedXmlNameType
	qualified_xml_name_create_gml(
			const QString &name)
	{
		// Use the single argument overload.
		return QualifiedXmlNameType::create_gml(name);
	}

	// Only allow the single argument version of the creation function.
	// There's no need for the user to also specify the namespace alias.
	template <class QualifiedXmlNameType>
	const QualifiedXmlNameType
	qualified_xml_name_create_xsi(
			const QString &name)
	{
		// Use the single argument overload.
		return QualifiedXmlNameType::create_xsi(name);
	}

	template <class QualifiedXmlNameType>
	bp::object
	qualified_xml_name_hash(
			const QualifiedXmlNameType &qualified_xml_name)
	{
		// Use python builtin hash() function to hash the fully-qualified string.
		return get_builtin_hash()(
				GPlatesModel::convert_qualified_xml_name_to_qstring<QualifiedXmlNameType>(
						qualified_xml_name));
	}
}


template <class PythonClassType/* the boost::python::class_ type*/>
void
export_qualified_xml_name(
		PythonClassType &qualified_xml_name_class,
		const char *class_name,
		const char *instance_name,
		const char *example_qualified_name,
		const char *example_unqualified_name)
{
	// The GPlatesModel::QualifiedXmlName<> type.
	typedef typename PythonClassType::wrapped_type qualified_xml_name_type;

	//
	// The common GPlatesModel::QualifiedXmlName part of each qualified name to avoid repetition.
	//
	qualified_xml_name_class
		.def("get_namespace",
				&qualified_xml_name_type::get_namespace,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_namespace()\n"
				"  Returns the namespace URI.\n"
				"\n"
				"  :rtype: string\n"
				"\n"
				"  For example, the ``gpml`` namespace alias has the namespace "
				"``http://www.gplates.org/gplates``.\n")
		.def("get_namespace_alias",
				&qualified_xml_name_type::get_namespace_alias,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_namespace_alias()\n"
				"  Returns the namespace alias.\n"
				"\n"
				"  :rtype: string\n"
				"\n"
				"  For example, ``gpml`` (if created with *create_gpml()*).\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__' is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.def("__hash__", &GPlatesApi::qualified_xml_name_hash<qualified_xml_name_type>)
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		.def(bp::self < bp::self)
		.def(bp::self <= bp::self)
		.def(bp::self > bp::self)
		.def(bp::self >= bp::self)
		// For '__str__' convert to a qualified XML string...
		.def("__str__", &GPlatesModel::convert_qualified_xml_name_to_qstring<qualified_xml_name_type>)
	;


	std::stringstream get_name_docstring_stream;
	get_name_docstring_stream <<
			"get_name()\n"
			"  Returns the unqualified name.\n"
			"\n"
			"  :rtype: string\n"
			"\n"
			"  This is the fully qualified name minus the "
			"``gpml:`` prefix (if created with *create_gpml()*).\n"
			"\n"
			"  For example, ``" << example_unqualified_name << "``.\n";

	// Member to-QString conversion function.
	qualified_xml_name_class.def("get_name",
			&qualified_xml_name_type::get_name,
			bp::return_value_policy<bp::copy_const_reference>(),
			get_name_docstring_stream.str().c_str());


	std::stringstream to_qualified_string_docstring_stream;
	to_qualified_string_docstring_stream <<
			"to_qualified_string()\n"
			"  Returns the fully qualified name.\n"
			"\n"
			"  :rtype: string\n"
			"\n"
			"  For example, ``" << example_qualified_name << "``.\n";

	// Member to-QString conversion function.
	qualified_xml_name_class.def("to_qualified_string",
			&GPlatesModel::convert_qualified_xml_name_to_qstring<qualified_xml_name_type>,
			to_qualified_string_docstring_stream.str().c_str());


	std::stringstream from_qualified_string_docstring_stream;
	from_qualified_string_docstring_stream <<
			"create_from_qualified_string(name)\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Creates a :class:`" << class_name << "` instance from a fully qualified name string.\n"
			"\n"
			"  :param name: qualified name\n"
			"  :type name: string\n"
			"  :rtype: :class:`" << class_name << "` or None\n"
			"\n"
			"  The name string should have a ``:`` character separating the namespace alias from the unqualified name, "
			"for example ``" << example_qualified_name
			<< "``. If the namespace alias is not recognised (as ``gpml``, ``gml`` or ``xsi``) then ``gpml`` is assumed.\n"
			"\n"
			"  An over-qualified name string (eg, containing two or more ``:`` characters) will result "
			"in ``None`` being returned.\n"
			"  ::\n"
			"\n"
			"    " << instance_name << " = pygplates." << class_name
			<< ".create_from_qualified_string('" << example_qualified_name << "')\n";

	// Static-member from-QString conversion function.
	qualified_xml_name_class.def("create_from_qualified_string",
			&GPlatesModel::convert_qstring_to_qualified_xml_name<qualified_xml_name_type>,
			from_qualified_string_docstring_stream.str().c_str());
	qualified_xml_name_class.staticmethod("create_from_qualified_string");


	// Enable boost::optional<GPlatesModel::QualifiedXmlName<> > to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<qualified_xml_name_type>();
}


void
export_enumeration_type()
{
	//
	// EnumerationType
	//
	bp::class_<GPlatesPropertyValues::EnumerationType> enumeration_type_class(
			"EnumerationType",
			"The namespace-qualified type of an enumeration.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported. EnumerationType is "
			"hashable (can be used as a key in a ``dict``).\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	enumeration_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesPropertyValues::EnumerationType>,
			"create_gpml(name)\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Create an enumeration type qualified with the ``gpml:`` prefix (``gpml:`` + ``name``).\n"
			"\n"
			"  :param name: unqualified name\n"
			"  :type name: string\n"
			"  :rtype: :class:`EnumerationType`\n"
			"\n"
			"  ::\n"
			"\n"
			"    gpml_subduction_polarity_enumeration_type = pygplates.EnumerationType.create_gpml('SubductionPolarityEnumeration')\n");
	enumeration_type_class.staticmethod("create_gpml");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			enumeration_type_class,
			"EnumerationType",
			"enumeration_type",
			"gpml:SubductionPolarityEnumeration",
			"SubductionPolarityEnumeration");
}


namespace GPlatesApi
{
	//
	// Some common feature types....
	//
	const GPlatesModel::FeatureType gpml_aseismic_ridge = GPlatesModel::FeatureType::create_gpml("AseismicRidge");
	const GPlatesModel::FeatureType gpml_basic_rock_unit = GPlatesModel::FeatureType::create_gpml("BasicRockUnit");
	const GPlatesModel::FeatureType gpml_basin = GPlatesModel::FeatureType::create_gpml("Basin");
	const GPlatesModel::FeatureType gpml_closed_continental_boundary = GPlatesModel::FeatureType::create_gpml("ClosedContinentalBoundary");
	const GPlatesModel::FeatureType gpml_closed_plate_boundary = GPlatesModel::FeatureType::create_gpml("ClosedPlateBoundary");
	const GPlatesModel::FeatureType gpml_coastline = GPlatesModel::FeatureType::create_gpml("Coastline");
	const GPlatesModel::FeatureType gpml_continental_crust = GPlatesModel::FeatureType::create_gpml("ContinentalCrust");
	const GPlatesModel::FeatureType gpml_continental_fragment = GPlatesModel::FeatureType::create_gpml("ContinentalFragment");
	const GPlatesModel::FeatureType gpml_continental_rift = GPlatesModel::FeatureType::create_gpml("ContinentalRift");
	const GPlatesModel::FeatureType gpml_craton = GPlatesModel::FeatureType::create_gpml("Craton");
	const GPlatesModel::FeatureType gpml_extended_continental_crust = GPlatesModel::FeatureType::create_gpml("ExtendedContinentalCrust");
	const GPlatesModel::FeatureType gpml_fault = GPlatesModel::FeatureType::create_gpml("Fault");
	const GPlatesModel::FeatureType gpml_flowline = GPlatesModel::FeatureType::create_gpml("Flowline");
	const GPlatesModel::FeatureType gpml_fold_plane = GPlatesModel::FeatureType::create_gpml("FoldPlane");
	const GPlatesModel::FeatureType gpml_fracture_zone = GPlatesModel::FeatureType::create_gpml("FractureZone");
	const GPlatesModel::FeatureType gpml_fracture_zone_identification = GPlatesModel::FeatureType::create_gpml("FractureZoneIdentification");
	const GPlatesModel::FeatureType gpml_geological_lineation = GPlatesModel::FeatureType::create_gpml("GeologicalLineation");
	const GPlatesModel::FeatureType gpml_geological_plane = GPlatesModel::FeatureType::create_gpml("GeologicalPlane");
	const GPlatesModel::FeatureType gpml_hot_spot = GPlatesModel::FeatureType::create_gpml("HotSpot");
	const GPlatesModel::FeatureType gpml_hot_spot_trail = GPlatesModel::FeatureType::create_gpml("HotSpotTrail");
	const GPlatesModel::FeatureType gpml_inferred_paleo_boundary = GPlatesModel::FeatureType::create_gpml("InferredPaleoBoundary");
	const GPlatesModel::FeatureType gpml_island_arc = GPlatesModel::FeatureType::create_gpml("IslandArc");
	const GPlatesModel::FeatureType gpml_isochron = GPlatesModel::FeatureType::create_gpml("Isochron");
	const GPlatesModel::FeatureType gpml_large_igneous_province = GPlatesModel::FeatureType::create_gpml("LargeIgneousProvince");
	const GPlatesModel::FeatureType gpml_magnetic_anomaly_identification = GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIdentification");
	const GPlatesModel::FeatureType gpml_magnetic_anomaly_lineation = GPlatesModel::FeatureType::create_gpml("MagneticAnomalyLineation");
	const GPlatesModel::FeatureType gpml_magnetic_anomaly_ship_track = GPlatesModel::FeatureType::create_gpml("MagneticAnomalyShipTrack");
	const GPlatesModel::FeatureType gpml_mid_ocean_ridge = GPlatesModel::FeatureType::create_gpml("MidOceanRidge");
	const GPlatesModel::FeatureType gpml_motion_path = GPlatesModel::FeatureType::create_gpml("MotionPath");
	const GPlatesModel::FeatureType gpml_oceanic_crust = GPlatesModel::FeatureType::create_gpml("OceanicCrust");
	const GPlatesModel::FeatureType gpml_ophiolite = GPlatesModel::FeatureType::create_gpml("Ophiolite");
	const GPlatesModel::FeatureType gpml_orogenic_belt = GPlatesModel::FeatureType::create_gpml("OrogenicBelt");
	const GPlatesModel::FeatureType gpml_passive_continental_boundary = GPlatesModel::FeatureType::create_gpml("PassiveContinentalBoundary");
	const GPlatesModel::FeatureType gpml_pluton = GPlatesModel::FeatureType::create_gpml("Pluton");
	const GPlatesModel::FeatureType gpml_pseudo_fault = GPlatesModel::FeatureType::create_gpml("PseudoFault");
	const GPlatesModel::FeatureType gpml_seamount = GPlatesModel::FeatureType::create_gpml("Seamount");
	const GPlatesModel::FeatureType gpml_slab_edge = GPlatesModel::FeatureType::create_gpml("SlabEdge");
	const GPlatesModel::FeatureType gpml_subduction_zone = GPlatesModel::FeatureType::create_gpml("SubductionZone");
	const GPlatesModel::FeatureType gpml_suture = GPlatesModel::FeatureType::create_gpml("Suture");
	const GPlatesModel::FeatureType gpml_terrane_boundary = GPlatesModel::FeatureType::create_gpml("TerraneBoundary");
	const GPlatesModel::FeatureType gpml_topological_closed_plate_boundary = GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary");
	const GPlatesModel::FeatureType gpml_topological_network = GPlatesModel::FeatureType::create_gpml("TopologicalNetwork");
	const GPlatesModel::FeatureType gpml_topological_slab_boundary = GPlatesModel::FeatureType::create_gpml("TopologicalSlabBoundary");
	const GPlatesModel::FeatureType gpml_total_reconstruction_sequence = GPlatesModel::FeatureType::create_gpml("TotalReconstructionSequence");
	const GPlatesModel::FeatureType gpml_transform = GPlatesModel::FeatureType::create_gpml("Transform");
	const GPlatesModel::FeatureType gpml_transitional_crust = GPlatesModel::FeatureType::create_gpml("TransitionalCrust");
	const GPlatesModel::FeatureType gpml_unconformity = GPlatesModel::FeatureType::create_gpml("Unconformity");
	const GPlatesModel::FeatureType gpml_unknown_contact = GPlatesModel::FeatureType::create_gpml("UnknownContact");
	const GPlatesModel::FeatureType gpml_virtual_geomagnetic_pole = GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");
	const GPlatesModel::FeatureType gpml_volcano = GPlatesModel::FeatureType::create_gpml("Volcano");
}

void
export_feature_type()
{
	//
	// FeatureType
	//
	bp::class_<GPlatesModel::FeatureType> feature_type_class(
			"FeatureType",
			"The namespace-qualified type of a feature.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported. FeatureType is "
			"hashable (can be used as a key in a ``dict``).\n"
			"\n"
			"As a convenience the following common feature types are available as class attributes:\n"
			"\n"
			"* ``pygplates.FeatureType.gpml_aseismic_ridge`` = pygplates.FeatureType.create_gpml('AseismicRidge')\n"
			"* ``pygplates.FeatureType.gpml_basic_rock_unit`` = pygplates.FeatureType.create_gpml('BasicRockUnit')\n"
			"* ``pygplates.FeatureType.gpml_basin`` = pygplates.FeatureType.create_gpml('Basin')\n"
			"* ``pygplates.FeatureType.gpml_closed_continental_boundary`` = pygplates.FeatureType.create_gpml('ClosedContinentalBoundary')\n"
			"* ``pygplates.FeatureType.gpml_closed_plate_boundary`` = pygplates.FeatureType.create_gpml('ClosedPlateBoundary')\n"
			"* ``pygplates.FeatureType.gpml_coastline`` = pygplates.FeatureType.create_gpml('Coastline')\n"
			"* ``pygplates.FeatureType.gpml_continental_crust`` = pygplates.FeatureType.create_gpml('ContinentalCrust')\n"
			"* ``pygplates.FeatureType.gpml_continental_fragment`` = pygplates.FeatureType.create_gpml('ContinentalFragment')\n"
			"* ``pygplates.FeatureType.gpml_continental_rift`` = pygplates.FeatureType.create_gpml('ContinentalRift')\n"
			"* ``pygplates.FeatureType.gpml_craton`` = pygplates.FeatureType.create_gpml('Craton')\n"
			"* ``pygplates.FeatureType.gpml_extended_continental_crust`` = pygplates.FeatureType.create_gpml('ExtendedContinentalCrust')\n"
			"* ``pygplates.FeatureType.gpml_fault`` = pygplates.FeatureType.create_gpml('Fault')\n"
			"* ``pygplates.FeatureType.gpml_flowline`` = pygplates.FeatureType.create_gpml('Flowline')\n"
			"* ``pygplates.FeatureType.gpml_fold_plane`` = pygplates.FeatureType.create_gpml('FoldPlane')\n"
			"* ``pygplates.FeatureType.gpml_fracture_zone`` = pygplates.FeatureType.create_gpml('FractureZone')\n"
			"* ``pygplates.FeatureType.gpml_fracture_zone_identification`` = pygplates.FeatureType.create_gpml('FractureZoneIdentification')\n"
			"* ``pygplates.FeatureType.gpml_geological_lineation`` = pygplates.FeatureType.create_gpml('GeologicalLineation')\n"
			"* ``pygplates.FeatureType.gpml_geological_plane`` = pygplates.FeatureType.create_gpml('GeologicalPlane')\n"
			"* ``pygplates.FeatureType.gpml_hot_spot`` = pygplates.FeatureType.create_gpml('HotSpot')\n"
			"* ``pygplates.FeatureType.gpml_hot_spot_trail`` = pygplates.FeatureType.create_gpml('HotSpotTrail')\n"
			"* ``pygplates.FeatureType.gpml_inferred_paleo_boundary`` = pygplates.FeatureType.create_gpml('InferredPaleoBoundary')\n"
			"* ``pygplates.FeatureType.gpml_island_arc`` = pygplates.FeatureType.create_gpml('IslandArc')\n"
			"* ``pygplates.FeatureType.gpml_isochron`` = pygplates.FeatureType.create_gpml('Isochron')\n"
			"* ``pygplates.FeatureType.gpml_large_igneous_province`` = pygplates.FeatureType.create_gpml('LargeIgneousProvince')\n"
			"* ``pygplates.FeatureType.gpml_magnetic_anomaly_identification`` = pygplates.FeatureType.create_gpml('MagneticAnomalyIdentification')\n"
			"* ``pygplates.FeatureType.gpml_magnetic_anomaly_lineation`` = pygplates.FeatureType.create_gpml('MagneticAnomalyLineation')\n"
			"* ``pygplates.FeatureType.gpml_magnetic_anomaly_ship_track`` = pygplates.FeatureType.create_gpml('MagneticAnomalyShipTrack')\n"
			"* ``pygplates.FeatureType.gpml_mid_ocean_ridge`` = pygplates.FeatureType.create_gpml('MidOceanRidge')\n"
			"* ``pygplates.FeatureType.gpml_motion_path`` = pygplates.FeatureType.create_gpml('MotionPath')\n"
			"* ``pygplates.FeatureType.gpml_oceanic_crust`` = pygplates.FeatureType.create_gpml('OceanicCrust')\n"
			"* ``pygplates.FeatureType.gpml_ophiolite`` = pygplates.FeatureType.create_gpml('Ophiolite')\n"
			"* ``pygplates.FeatureType.gpml_orogenic_belt`` = pygplates.FeatureType.create_gpml('OrogenicBelt')\n"
			"* ``pygplates.FeatureType.gpml_passive_continental_boundary`` = pygplates.FeatureType.create_gpml('PassiveContinentalBoundary')\n"
			"* ``pygplates.FeatureType.gpml_pluton`` = pygplates.FeatureType.create_gpml('Pluton')\n"
			"* ``pygplates.FeatureType.gpml_pseudo_fault`` = pygplates.FeatureType.create_gpml('PseudoFault')\n"
			"* ``pygplates.FeatureType.gpml_seamount`` = pygplates.FeatureType.create_gpml('Seamount')\n"
			"* ``pygplates.FeatureType.gpml_slab_edge`` = pygplates.FeatureType.create_gpml('SlabEdge')\n"
			"* ``pygplates.FeatureType.gpml_subduction_zone`` = pygplates.FeatureType.create_gpml('SubductionZone')\n"
			"* ``pygplates.FeatureType.gpml_suture`` = pygplates.FeatureType.create_gpml('Suture')\n"
			"* ``pygplates.FeatureType.gpml_terrane_boundary`` = pygplates.FeatureType.create_gpml('TerraneBoundary')\n"
			"* ``pygplates.FeatureType.gpml_topological_closed_plate_boundary`` = pygplates.FeatureType.create_gpml('TopologicalClosedPlateBoundary')\n"
			"* ``pygplates.FeatureType.gpml_topological_network`` = pygplates.FeatureType.create_gpml('TopologicalNetwork')\n"
			"* ``pygplates.FeatureType.gpml_topological_slab_boundary`` = pygplates.FeatureType.create_gpml('TopologicalSlabBoundary')\n"
			"* ``pygplates.FeatureType.gpml_total_reconstruction_sequence`` = pygplates.FeatureType.create_gpml('TotalReconstructionSequence')\n"
			"* ``pygplates.FeatureType.gpml_transform`` = pygplates.FeatureType.create_gpml('Transform')\n"
			"* ``pygplates.FeatureType.gpml_transitional_crust`` = pygplates.FeatureType.create_gpml('TransitionalCrust')\n"
			"* ``pygplates.FeatureType.gpml_unconformity`` = pygplates.FeatureType.create_gpml('Unconformity')\n"
			"* ``pygplates.FeatureType.gpml_unknown_contact`` = pygplates.FeatureType.create_gpml('UnknownContact')\n"
			"* ``pygplates.FeatureType.gpml_virtual_geomagnetic_pole`` = pygplates.FeatureType.create_gpml('VirtualGeomagneticPole')\n"
			"* ``pygplates.FeatureType.gpml_volcano`` = pygplates.FeatureType.create_gpml('Volcano')\n",
			bp::no_init/*force usage of create functions*/);

	// Some common feature types...
	feature_type_class.def_readonly("gpml_aseismic_ridge", GPlatesApi::gpml_aseismic_ridge);
	feature_type_class.def_readonly("gpml_basic_rock_unit", GPlatesApi::gpml_basic_rock_unit);
	feature_type_class.def_readonly("gpml_basin", GPlatesApi::gpml_basin);
	feature_type_class.def_readonly("gpml_closed_continental_boundary", GPlatesApi::gpml_closed_continental_boundary);
	feature_type_class.def_readonly("gpml_closed_plate_boundary", GPlatesApi::gpml_closed_plate_boundary);
	feature_type_class.def_readonly("gpml_coastline", GPlatesApi::gpml_coastline);
	feature_type_class.def_readonly("gpml_continental_crust", GPlatesApi::gpml_continental_crust);
	feature_type_class.def_readonly("gpml_continental_fragment", GPlatesApi::gpml_continental_fragment);
	feature_type_class.def_readonly("gpml_continental_rift", GPlatesApi::gpml_continental_rift);
	feature_type_class.def_readonly("gpml_craton", GPlatesApi::gpml_craton);
	feature_type_class.def_readonly("gpml_extended_continental_crust", GPlatesApi::gpml_extended_continental_crust);
	feature_type_class.def_readonly("gpml_fault", GPlatesApi::gpml_fault);
	feature_type_class.def_readonly("gpml_flowline", GPlatesApi::gpml_flowline);
	feature_type_class.def_readonly("gpml_fold_plane", GPlatesApi::gpml_fold_plane);
	feature_type_class.def_readonly("gpml_fracture_zone", GPlatesApi::gpml_fracture_zone);
	feature_type_class.def_readonly("gpml_fracture_zone_identification", GPlatesApi::gpml_fracture_zone_identification);
	feature_type_class.def_readonly("gpml_geological_lineation", GPlatesApi::gpml_geological_lineation);
	feature_type_class.def_readonly("gpml_geological_plane", GPlatesApi::gpml_geological_plane);
	feature_type_class.def_readonly("gpml_hot_spot", GPlatesApi::gpml_hot_spot);
	feature_type_class.def_readonly("gpml_hot_spot_trail", GPlatesApi::gpml_hot_spot_trail);
	feature_type_class.def_readonly("gpml_inferred_paleo_boundary", GPlatesApi::gpml_inferred_paleo_boundary);
	feature_type_class.def_readonly("gpml_island_arc", GPlatesApi::gpml_island_arc);
	feature_type_class.def_readonly("gpml_isochron", GPlatesApi::gpml_isochron);
	feature_type_class.def_readonly("gpml_large_igneous_province", GPlatesApi::gpml_large_igneous_province);
	feature_type_class.def_readonly("gpml_magnetic_anomaly_identification", GPlatesApi::gpml_magnetic_anomaly_identification);
	feature_type_class.def_readonly("gpml_magnetic_anomaly_lineation", GPlatesApi::gpml_magnetic_anomaly_lineation);
	feature_type_class.def_readonly("gpml_magnetic_anomaly_ship_track", GPlatesApi::gpml_magnetic_anomaly_ship_track);
	feature_type_class.def_readonly("gpml_mid_ocean_ridge", GPlatesApi::gpml_mid_ocean_ridge);
	feature_type_class.def_readonly("gpml_motion_path", GPlatesApi::gpml_motion_path);
	feature_type_class.def_readonly("gpml_oceanic_crust", GPlatesApi::gpml_oceanic_crust);
	feature_type_class.def_readonly("gpml_ophiolite", GPlatesApi::gpml_ophiolite);
	feature_type_class.def_readonly("gpml_orogenic_belt", GPlatesApi::gpml_orogenic_belt);
	feature_type_class.def_readonly("gpml_passive_continental_boundary", GPlatesApi::gpml_passive_continental_boundary);
	feature_type_class.def_readonly("gpml_pluton", GPlatesApi::gpml_pluton);
	feature_type_class.def_readonly("gpml_pseudo_fault", GPlatesApi::gpml_pseudo_fault);
	feature_type_class.def_readonly("gpml_seamount", GPlatesApi::gpml_seamount);
	feature_type_class.def_readonly("gpml_slab_edge", GPlatesApi::gpml_slab_edge);
	feature_type_class.def_readonly("gpml_subduction_zone", GPlatesApi::gpml_subduction_zone);
	feature_type_class.def_readonly("gpml_suture", GPlatesApi::gpml_suture);
	feature_type_class.def_readonly("gpml_terrane_boundary", GPlatesApi::gpml_terrane_boundary);
	feature_type_class.def_readonly("gpml_topological_closed_plate_boundary", GPlatesApi::gpml_topological_closed_plate_boundary);
	feature_type_class.def_readonly("gpml_topological_network", GPlatesApi::gpml_topological_network);
	feature_type_class.def_readonly("gpml_topological_slab_boundary", GPlatesApi::gpml_topological_slab_boundary);
	feature_type_class.def_readonly("gpml_total_reconstruction_sequence", GPlatesApi::gpml_total_reconstruction_sequence);
	feature_type_class.def_readonly("gpml_transform", GPlatesApi::gpml_transform);
	feature_type_class.def_readonly("gpml_transitional_crust", GPlatesApi::gpml_transitional_crust);
	feature_type_class.def_readonly("gpml_unconformity", GPlatesApi::gpml_unconformity);
	feature_type_class.def_readonly("gpml_unknown_contact", GPlatesApi::gpml_unknown_contact);
	feature_type_class.def_readonly("gpml_virtual_geomagnetic_pole", GPlatesApi::gpml_virtual_geomagnetic_pole);
	feature_type_class.def_readonly("gpml_volcano", GPlatesApi::gpml_volcano);

	// Select the create functions appropriate for this QualifiedXmlName type...
	feature_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesModel::FeatureType>,
			"create_gpml(name)\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Create a feature type qualified with the ``gpml:`` prefix (``gpml:`` + ``name``).\n"
			"\n"
			"  :param name: unqualified name\n"
			"  :type name: string\n"
			"  :rtype: :class:`FeatureType`\n"
			"\n"
			"  ::\n"
			"\n"
			"    gpml_coastline_feature_type = pygplates.FeatureType.create_gpml('Coastline')\n");
	feature_type_class.staticmethod("create_gpml");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			feature_type_class,
			"FeatureType",
			"feature_type",
			"gpml:Coastline",
			"Coastline");
}


namespace GPlatesApi
{
	//
	// Some common property names....
	//
	const GPlatesModel::PropertyName gml_description = GPlatesModel::PropertyName::create_gml("description");
	const GPlatesModel::PropertyName gml_name = GPlatesModel::PropertyName::create_gml("name");
	const GPlatesModel::PropertyName gml_valid_time = GPlatesModel::PropertyName::create_gml("validTime");
	const GPlatesModel::PropertyName gpml_average_age = GPlatesModel::PropertyName::create_gpml("averageAge");
	const GPlatesModel::PropertyName gpml_average_declination = GPlatesModel::PropertyName::create_gpml("averageDeclination");
	const GPlatesModel::PropertyName gpml_average_inclination = GPlatesModel::PropertyName::create_gpml("averageInclination");
	const GPlatesModel::PropertyName gpml_conjugate_plate_id = GPlatesModel::PropertyName::create_gpml("conjugatePlateId");
	const GPlatesModel::PropertyName gpml_fixed_reference_frame = GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame");
	const GPlatesModel::PropertyName gpml_left_plate = GPlatesModel::PropertyName::create_gpml("leftPlate");
	const GPlatesModel::PropertyName gpml_moving_reference_frame = GPlatesModel::PropertyName::create_gpml("movingReferenceFrame");
	const GPlatesModel::PropertyName gpml_polarity_chron_id = GPlatesModel::PropertyName::create_gpml("polarityChronId");
	const GPlatesModel::PropertyName gpml_polarity_chron_offset = GPlatesModel::PropertyName::create_gpml("polarityChronOffset");
	const GPlatesModel::PropertyName gpml_pole_a95 = GPlatesModel::PropertyName::create_gpml("poleA95");
	const GPlatesModel::PropertyName gpml_pole_dm = GPlatesModel::PropertyName::create_gpml("poleDm");
	const GPlatesModel::PropertyName gpml_pole_dp = GPlatesModel::PropertyName::create_gpml("poleDp");
	const GPlatesModel::PropertyName gpml_reconstruction_method = GPlatesModel::PropertyName::create_gpml("reconstructionMethod");
	const GPlatesModel::PropertyName gpml_reconstruction_plate_id = GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	const GPlatesModel::PropertyName gpml_relative_plate = GPlatesModel::PropertyName::create_gpml("relativePlate");
	const GPlatesModel::PropertyName gpml_right_plate = GPlatesModel::PropertyName::create_gpml("rightPlate");
	const GPlatesModel::PropertyName gpml_shapefile_attributes = GPlatesModel::PropertyName::create_gpml("shapefileAttributes");
	const GPlatesModel::PropertyName gpml_times = GPlatesModel::PropertyName::create_gpml("times");
	const GPlatesModel::PropertyName gpml_total_reconstruction_pole = GPlatesModel::PropertyName::create_gpml("totalReconstructionPole");

	//
	// Some common enumeration property names....
	//
	const GPlatesModel::PropertyName gpml_absolute_reference_frame = GPlatesModel::PropertyName::create_gpml("absoluteReferenceFrame");
	const GPlatesModel::PropertyName gpml_crust = GPlatesModel::PropertyName::create_gpml("crust");
	const GPlatesModel::PropertyName gpml_dip_side = GPlatesModel::PropertyName::create_gpml("dipSide");
	const GPlatesModel::PropertyName gpml_dip_slip = GPlatesModel::PropertyName::create_gpml("dipSlip");
	const GPlatesModel::PropertyName gpml_edge = GPlatesModel::PropertyName::create_gpml("edge");
	const GPlatesModel::PropertyName gpml_fold_annotation = GPlatesModel::PropertyName::create_gpml("foldAnnotation");
	const GPlatesModel::PropertyName gpml_motion = GPlatesModel::PropertyName::create_gpml("motion");
	const GPlatesModel::PropertyName gpml_polarity_chron_orientation = GPlatesModel::PropertyName::create_gpml("polarityChronOrientation");
	const GPlatesModel::PropertyName gpml_primary_slip_component = GPlatesModel::PropertyName::create_gpml("primarySlipComponent");
	const GPlatesModel::PropertyName gpml_quality = GPlatesModel::PropertyName::create_gpml("quality");
	const GPlatesModel::PropertyName gpml_side = GPlatesModel::PropertyName::create_gpml("side");
	const GPlatesModel::PropertyName gpml_strike_slip = GPlatesModel::PropertyName::create_gpml("strikeSlip");
	const GPlatesModel::PropertyName gpml_subduction_polarity = GPlatesModel::PropertyName::create_gpml("subductionPolarity");

	//
	// Some common geometry property names....
	//
	const GPlatesModel::PropertyName gpml_average_sample_site_position = GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition");
	const GPlatesModel::PropertyName gpml_boundary = GPlatesModel::PropertyName::create_gpml("boundary");
	const GPlatesModel::PropertyName gpml_center_line_of = GPlatesModel::PropertyName::create_gpml("centerLineOf");
	const GPlatesModel::PropertyName gpml_error_bounds = GPlatesModel::PropertyName::create_gpml("errorBounds");
	const GPlatesModel::PropertyName gpml_multi_position = GPlatesModel::PropertyName::create_gpml("multiPosition");
	const GPlatesModel::PropertyName gpml_outline_of = GPlatesModel::PropertyName::create_gpml("outlineOf");
	const GPlatesModel::PropertyName gpml_pole_position = GPlatesModel::PropertyName::create_gpml("polePosition");
	const GPlatesModel::PropertyName gpml_position = GPlatesModel::PropertyName::create_gpml("position");
	const GPlatesModel::PropertyName gpml_seed_points = GPlatesModel::PropertyName::create_gpml("seedPoints");
	const GPlatesModel::PropertyName gpml_unclassified_geometry = GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry");
}

void
export_property_name()
{
	//
	// PropertyName - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesModel::PropertyName> property_name_class(
			"PropertyName",
			"The namespace-qualified name of a property.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported. PropertyName is "
			"hashable (can be used as a key in a ``dict``).\n"
			"\n"
			"As a convenience the following common property names are available as class attributes:\n"
			"\n"
			"* ``pygplates.PropertyName.gml_description`` = pygplates.PropertyName.create_gml('description')\n"
			"* ``pygplates.PropertyName.gml_name`` = pygplates.PropertyName.create_gml('name')\n"
			"* ``pygplates.PropertyName.gml_valid_time`` = pygplates.PropertyName.create_gml('validTime')\n"
			"* ``pygplates.PropertyName.gpml_average_age`` = pygplates.PropertyName.create_gpml('averageAge')\n"
			"* ``pygplates.PropertyName.gpml_average_declination`` = pygplates.PropertyName.create_gpml('averageDeclination')\n"
			"* ``pygplates.PropertyName.gpml_average_inclination`` = pygplates.PropertyName.create_gpml('averageInclination')\n"
			"* ``pygplates.PropertyName.gpml_conjugate_plate_id`` = pygplates.PropertyName.create_gpml('conjugatePlateId')\n"
			"* ``pygplates.PropertyName.gpml_fixed_reference_frame`` = pygplates.PropertyName.create_gpml('fixedReferenceFrame')\n"
			"* ``pygplates.PropertyName.gpml_left_plate`` = pygplates.PropertyName.create_gpml('leftPlate')\n"
			"* ``pygplates.PropertyName.gpml_moving_reference_frame`` = pygplates.PropertyName.create_gpml('movingReferenceFrame')\n"
			"* ``pygplates.PropertyName.gpml_polarity_chron_id`` = pygplates.PropertyName.create_gpml('polarityChronId')\n"
			"* ``pygplates.PropertyName.gpml_polarity_chron_offset`` = pygplates.PropertyName.create_gpml('polarityChronOffset')\n"
			"* ``pygplates.PropertyName.gpml_pole_a95`` = pygplates.PropertyName.create_gpml('poleA95')\n"
			"* ``pygplates.PropertyName.gpml_pole_dm`` = pygplates.PropertyName.create_gpml('poleDm')\n"
			"* ``pygplates.PropertyName.gpml_pole_dp`` = pygplates.PropertyName.create_gpml('poleDp')\n"
			"* ``pygplates.PropertyName.gpml_reconstruction_method`` = pygplates.PropertyName.create_gpml('reconstructionMethod')\n"
			"* ``pygplates.PropertyName.gpml_reconstruction_plate_id`` = pygplates.PropertyName.create_gpml('reconstructionPlateId')\n"
			"* ``pygplates.PropertyName.gpml_relative_plate`` = pygplates.PropertyName.create_gpml('relativePlate')\n"
			"* ``pygplates.PropertyName.gpml_right_plate`` = pygplates.PropertyName.create_gpml('rightPlate')\n"
			"* ``pygplates.PropertyName.gpml_shapefile_attributes`` = pygplates.PropertyName.create_gpml('shapefileAttributes')\n"
			"* ``pygplates.PropertyName.gpml_times`` = pygplates.PropertyName.create_gpml('times')\n"
			"* ``pygplates.PropertyName.gpml_total_reconstruction_pole`` = pygplates.PropertyName.create_gpml('totalReconstructionPole')\n"
			"\n"
			"As a convenience the following common :class:`enumeration<Enumeration>` property names are available as class attributes:\n"
			"\n"
			"* ``pygplates.PropertyName.gpml_absolute_reference_frame`` = pygplates.PropertyName.create_gpml('absoluteReferenceFrame')\n"
			"* ``pygplates.PropertyName.gpml_crust`` = pygplates.PropertyName.create_gpml('crust')\n"
			"* ``pygplates.PropertyName.gpml_dip_side`` = pygplates.PropertyName.create_gpml('dipSide')\n"
			"* ``pygplates.PropertyName.gpml_dip_slip`` = pygplates.PropertyName.create_gpml('dipSlip')\n"
			"* ``pygplates.PropertyName.gpml_edge`` = pygplates.PropertyName.create_gpml('edge')\n"
			"* ``pygplates.PropertyName.gpml_fold_annotation`` = pygplates.PropertyName.create_gpml('foldAnnotation')\n"
			"* ``pygplates.PropertyName.gpml_motion`` = pygplates.PropertyName.create_gpml('motion')\n"
			"* ``pygplates.PropertyName.gpml_polarity_chron_orientation`` = pygplates.PropertyName.create_gpml('polarityChronOrientation')\n"
			"* ``pygplates.PropertyName.gpml_primary_slip_component`` = pygplates.PropertyName.create_gpml('primarySlipComponent')\n"
			"* ``pygplates.PropertyName.gpml_quality`` = pygplates.PropertyName.create_gpml('quality')\n"
			"* ``pygplates.PropertyName.gpml_side`` = pygplates.PropertyName.create_gpml('side')\n"
			"* ``pygplates.PropertyName.gpml_strike_slip`` = pygplates.PropertyName.create_gpml('strikeSlip')\n"
			"* ``pygplates.PropertyName.gpml_subduction_polarity`` = pygplates.PropertyName.create_gpml('subductionPolarity')\n"
			"\n"
			"As a convenience the following common *geometry* property names are available as class attributes:\n"
			"\n"
			"* ``pygplates.PropertyName.gpml_average_sample_site_position`` = pygplates.PropertyName.create_gpml('averageSampleSitePosition')\n"
			"* ``pygplates.PropertyName.gpml_boundary`` = pygplates.PropertyName.create_gpml('boundary')\n"
			"* ``pygplates.PropertyName.gpml_center_line_of`` = pygplates.PropertyName.create_gpml('centerLineOf')\n"
			"* ``pygplates.PropertyName.gpml_error_bounds`` = pygplates.PropertyName.create_gpml('errorBounds')\n"
			"* ``pygplates.PropertyName.gpml_multi_position`` = pygplates.PropertyName.create_gpml('multiPosition')\n"
			"* ``pygplates.PropertyName.gpml_outline_of`` = pygplates.PropertyName.create_gpml('outlineOf')\n"
			"* ``pygplates.PropertyName.gpml_pole_position`` = pygplates.PropertyName.create_gpml('polePosition')\n"
			"* ``pygplates.PropertyName.gpml_position`` = pygplates.PropertyName.create_gpml('position')\n"
			"* ``pygplates.PropertyName.gpml_seed_points`` = pygplates.PropertyName.create_gpml('seedPoints')\n"
			"* ``pygplates.PropertyName.gpml_unclassified_geometry`` = pygplates.PropertyName.create_gpml('unclassifiedGeometry')\n",
			bp::no_init/*force usage of create functions*/);

	// Some common property names...
	property_name_class.def_readonly("gml_description", GPlatesApi::gml_description);
	property_name_class.def_readonly("gml_name", GPlatesApi::gml_name);
	property_name_class.def_readonly("gml_valid_time", GPlatesApi::gml_valid_time);
	property_name_class.def_readonly("gpml_average_age", GPlatesApi::gpml_average_age);
	property_name_class.def_readonly("gpml_average_declination", GPlatesApi::gpml_average_declination);
	property_name_class.def_readonly("gpml_average_inclination", GPlatesApi::gpml_average_inclination);
	property_name_class.def_readonly("gpml_conjugate_plate_id", GPlatesApi::gpml_conjugate_plate_id);
	property_name_class.def_readonly("gpml_fixed_reference_frame", GPlatesApi::gpml_fixed_reference_frame);
	property_name_class.def_readonly("gpml_left_plate", GPlatesApi::gpml_left_plate);
	property_name_class.def_readonly("gpml_moving_reference_frame", GPlatesApi::gpml_moving_reference_frame);
	property_name_class.def_readonly("gpml_polarity_chron_id", GPlatesApi::gpml_polarity_chron_id);
	property_name_class.def_readonly("gpml_polarity_chron_offset", GPlatesApi::gpml_polarity_chron_offset);
	property_name_class.def_readonly("gpml_pole_a95", GPlatesApi::gpml_pole_a95);
	property_name_class.def_readonly("gpml_pole_dm", GPlatesApi::gpml_pole_dm);
	property_name_class.def_readonly("gpml_pole_dp", GPlatesApi::gpml_pole_dp);
	property_name_class.def_readonly("gpml_reconstruction_method", GPlatesApi::gpml_reconstruction_method);
	property_name_class.def_readonly("gpml_reconstruction_plate_id", GPlatesApi::gpml_reconstruction_plate_id);
	property_name_class.def_readonly("gpml_right_plate", GPlatesApi::gpml_right_plate);
	property_name_class.def_readonly("gpml_relative_plate", GPlatesApi::gpml_relative_plate);
	property_name_class.def_readonly("gpml_shapefile_attributes", GPlatesApi::gpml_shapefile_attributes);
	property_name_class.def_readonly("gpml_times", GPlatesApi::gpml_times);
	property_name_class.def_readonly("gpml_total_reconstruction_pole", GPlatesApi::gpml_total_reconstruction_pole);

	// Some common enumeration property names...
	property_name_class.def_readonly("gpml_absolute_reference_frame", GPlatesApi::gpml_absolute_reference_frame);
	property_name_class.def_readonly("gpml_crust", GPlatesApi::gpml_crust);
	property_name_class.def_readonly("gpml_dip_side", GPlatesApi::gpml_dip_side);
	property_name_class.def_readonly("gpml_dip_slip", GPlatesApi::gpml_dip_slip);
	property_name_class.def_readonly("gpml_edge", GPlatesApi::gpml_edge);
	property_name_class.def_readonly("gpml_fold_annotation", GPlatesApi::gpml_fold_annotation);
	property_name_class.def_readonly("gpml_motion", GPlatesApi::gpml_motion);
	property_name_class.def_readonly("gpml_polarity_chron_orientation", GPlatesApi::gpml_polarity_chron_orientation);
	property_name_class.def_readonly("gpml_primary_slip_component", GPlatesApi::gpml_primary_slip_component);
	property_name_class.def_readonly("gpml_quality", GPlatesApi::gpml_quality);
	property_name_class.def_readonly("gpml_side", GPlatesApi::gpml_side);
	property_name_class.def_readonly("gpml_strike_slip", GPlatesApi::gpml_strike_slip);
	property_name_class.def_readonly("gpml_subduction_polarity", GPlatesApi::gpml_subduction_polarity);

	// Some common geometry property names...
	property_name_class.def_readonly("gpml_average_sample_site_position", GPlatesApi::gpml_average_sample_site_position);
	property_name_class.def_readonly("gpml_boundary", GPlatesApi::gpml_boundary);
	property_name_class.def_readonly("gpml_center_line_of", GPlatesApi::gpml_center_line_of);
	property_name_class.def_readonly("gpml_error_bounds", GPlatesApi::gpml_error_bounds);
	property_name_class.def_readonly("gpml_multi_position", GPlatesApi::gpml_multi_position);
	property_name_class.def_readonly("gpml_outline_of", GPlatesApi::gpml_outline_of);
	property_name_class.def_readonly("gpml_pole_position", GPlatesApi::gpml_pole_position);
	property_name_class.def_readonly("gpml_position", GPlatesApi::gpml_position);
	property_name_class.def_readonly("gpml_seed_points", GPlatesApi::gpml_seed_points);
	property_name_class.def_readonly("gpml_unclassified_geometry", GPlatesApi::gpml_unclassified_geometry);

	// Select the create functions appropriate for this QualifiedXmlName type...
	property_name_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesModel::PropertyName>,
				"create_gpml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a property name qualified with the ``gpml:`` prefix (``gpml:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`PropertyName`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gpml_reconstruction_plate_id_property_name = pygplates.PropertyName.create_gpml('reconstructionPlateId')\n");
	property_name_class.staticmethod("create_gpml");
	property_name_class.def("create_gml",
			&GPlatesApi::qualified_xml_name_create_gml<GPlatesModel::PropertyName>,
				"create_gml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a property name qualified with the ``gml:`` prefix (``gml:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`PropertyName`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gml_valid_time_property_name = pygplates.PropertyName.create_gml('validTime')\n");
	property_name_class.staticmethod("create_gml");
	property_name_class.def("create_xsi",
			&GPlatesApi::qualified_xml_name_create_xsi<GPlatesModel::PropertyName>,
				"create_xsi(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a property name qualified with the ``xsi:`` prefix (``xsi:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`PropertyName`\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_name = pygplates.PropertyName.create_xsi(name)\n");
	property_name_class.staticmethod("create_xsi");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			property_name_class,
			"PropertyName",
			"property_name",
			"gpml:reconstructionPlateId",
			"reconstructionPlateId");
}


void
export_scalar_type()
{
	//
	// ScalarType - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	// In the C++ code it's really a 'ValueObjectType' (the type that goes in the 'gml:valueComponent'
	// slots of a 'gml:DataBlock' property type) but it only really gets used to differentiate the
	// different scalar types (packed in the 'gml:DataBlock' and elsewhere in the code) so we'll call it
	// 'ScalarType' in the Python code (also 'ValueObjectType' is a bit meaningless in a general context).
	//
	bp::class_<GPlatesPropertyValues::ValueObjectType> scalar_type_class(
			"ScalarType",
			"The namespace-qualified type of scalar values.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported. ScalarType is "
			"hashable (can be used as a key in a ``dict``).\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	scalar_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesPropertyValues::ValueObjectType>,
				"create_gpml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a scalar type qualified with the ``gpml:`` prefix (``gpml:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`ScalarType`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gpml_velocity_colat_scalar_type = pygplates.ScalarType.create_gpml('VelocityColat')\n");
	scalar_type_class.staticmethod("create_gpml");
	scalar_type_class.def("create_gml",
			&GPlatesApi::qualified_xml_name_create_gml<GPlatesPropertyValues::ValueObjectType>,
				"create_gml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a scalar type qualified with the ``gml:`` prefix (``gml:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`ScalarType`\n");
	scalar_type_class.staticmethod("create_gml");
	scalar_type_class.def("create_xsi",
			&GPlatesApi::qualified_xml_name_create_xsi<GPlatesPropertyValues::ValueObjectType>,
				"create_xsi(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a scalar type qualified with the ``xsi:`` prefix (``xsi:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`ScalarType`\n");
	scalar_type_class.staticmethod("create_xsi");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			scalar_type_class,
			"ScalarType",
			"gpml_velocity_colat_scalar_type",
			"gpml:VelocityColat",
			"VelocityColat");
}


void
export_structural_type()
{
	//
	// StructuralType - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesPropertyValues::StructuralType> structural_type_class(
			"StructuralType",
			"The namespace-qualified structural type.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported. StructuralType is "
			"hashable (can be used as a key in a ``dict``).\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	structural_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesPropertyValues::StructuralType>,
				"create_gpml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a structural type qualified with the ``gpml:`` prefix (``gpml:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`StructuralType`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gpml_finite_rotation_structural_type = pygplates.StructuralType.create_gpml('FiniteRotation')\n");
	structural_type_class.staticmethod("create_gpml");
	structural_type_class.def("create_gml",
			&GPlatesApi::qualified_xml_name_create_gml<GPlatesPropertyValues::StructuralType>,
				"create_gml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a structural type qualified with the ``gml:`` prefix (``gml:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`StructuralType`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gml_time_period_structural_type = pygplates.StructuralType.create_gml('TimePeriod')\n");
	structural_type_class.staticmethod("create_gml");
	structural_type_class.def("create_xsi",
			&GPlatesApi::qualified_xml_name_create_xsi<GPlatesPropertyValues::StructuralType>,
				"create_xsi(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a structural type qualified with the ``xsi:`` prefix (``xsi:`` + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`StructuralType`\n"
				"\n"
				"  ::\n"
				"\n"
				"    xsi_double_structural_type = pygplates.StructuralType.create_xsi('double')\n");
	structural_type_class.staticmethod("create_xsi");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			structural_type_class,
			"StructuralType",
			"structural_type",
			"gml:TimePeriod",
			"TimePeriod");
}


void
export_qualified_xml_names()
{
	export_enumeration_type();
	export_feature_type();
	export_property_name();
	export_scalar_type();
#if 0 // There's no need to expose 'StructuralType' (yet)...
	export_structural_type();
#endif
}

#endif // GPLATES_NO_PYTHON
