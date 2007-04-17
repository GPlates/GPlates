/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <boost/intrusive_ptr.hpp>
#include <boost/python.hpp>

#include "Model.h"
#include "DummyTransactionHandle.h"
#include "FeatureHandle.h"
#include "FeatureRevision.h"
#include "GmlLineString.h"
#include "GmlOrientableCurve.h"
#include "GmlTimePeriod.h"
#include "GpmlConstantValue.h"
#include "GpmlFiniteRotationSlerp.h"
#include "GpmlFiniteRotation.h"
#include "GpmlIrregularSampling.h"
#include "GpmlPlateId.h"
#include "GpmlTimeSample.h"
#include "InlinePropertyContainer.h"
#include "XsString.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreePopulator.h"
#include "ReconstructedFeatureGeometryPopulator.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"


const boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_reconstruction_plate_id(
		const unsigned long &plate_id)
{
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_plate_id =
			GPlatesModel::GpmlPlateId::create(plate_id);

	UnicodeString template_type_parameter_type_string("gpml:plateId");
	GPlatesModel::TemplateTypeParameterType template_type_parameter_type(template_type_parameter_type_string);
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_plate_id_constant_value =
			GPlatesModel::GpmlConstantValue::create(gpml_plate_id, template_type_parameter_type);

	UnicodeString property_name_string("gpml:reconstructionPlateId");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> inline_property_container =
			GPlatesModel::InlinePropertyContainer::create(property_name,
			gpml_plate_id_constant_value, xml_attributes);

	return inline_property_container;
}


const boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_reference_frame_plate_id(
		const unsigned long &plate_id,
		const char *which_reference_frame)
{
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_plate_id =
			GPlatesModel::GpmlPlateId::create(plate_id);

	UnicodeString property_name_string(which_reference_frame);
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> inline_property_container =
			GPlatesModel::InlinePropertyContainer::create(property_name,
			gpml_plate_id, xml_attributes);

	return inline_property_container;
}


const boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_centre_line_of(
		const double *points,
		unsigned num_points)
{
	std::vector<double> gml_pos_list(points, points + num_points);
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_line_string =
			GPlatesModel::GmlLineString::create(gml_pos_list);

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	GPlatesModel::XmlAttributeName xml_attribute_name("orientation");
	GPlatesModel::XmlAttributeValue xml_attribute_value("+");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_orientable_curve =
			GPlatesModel::GmlOrientableCurve::create(gml_line_string, xml_attributes);

	UnicodeString template_type_parameter_type_string("gml:OrientableCurve");
	GPlatesModel::TemplateTypeParameterType template_type_parameter_type(template_type_parameter_type_string);
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_orientable_curve_constant_value =
			GPlatesModel::GpmlConstantValue::create(gml_orientable_curve, template_type_parameter_type);

	UnicodeString property_name_string("gpml:centreLineOf");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes2;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> inline_property_container =
			GPlatesModel::InlinePropertyContainer::create(property_name,
			gml_orientable_curve_constant_value, xml_attributes2);

	return inline_property_container;
}


const boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_valid_time(
		const GPlatesModel::GeoTimeInstant &geo_time_instant_begin,
		const GPlatesModel::GeoTimeInstant &geo_time_instant_end)
{
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	GPlatesModel::XmlAttributeName xml_attribute_name("frame");
	GPlatesModel::XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	boost::intrusive_ptr<GPlatesModel::GmlTimeInstant> gml_time_instant_begin =
			GPlatesModel::GmlTimeInstant::create(geo_time_instant_begin, xml_attributes);

	boost::intrusive_ptr<GPlatesModel::GmlTimeInstant> gml_time_instant_end =
			GPlatesModel::GmlTimeInstant::create(geo_time_instant_end, xml_attributes);

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_time_period =
			GPlatesModel::GmlTimePeriod::create(gml_time_instant_begin, gml_time_instant_end);

	UnicodeString property_name_string("gml:validTime");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes2;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> inline_property_container =
			GPlatesModel::InlinePropertyContainer::create(property_name,
			gml_time_period, xml_attributes2);

	return inline_property_container;
}


const boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_description(
		const UnicodeString &description)
{
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_description = GPlatesModel::XsString::create(description);

	UnicodeString property_name_string("gml:description");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> inline_property_container =
			GPlatesModel::InlinePropertyContainer::create(property_name,
			gml_description, xml_attributes);

	return inline_property_container;
}


const boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_name(
		const UnicodeString &name,
		const UnicodeString &codespace)
{
	boost::intrusive_ptr<GPlatesModel::PropertyValue> gml_name = GPlatesModel::XsString::create(name);

	UnicodeString property_name_string("gml:name");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	GPlatesModel::XmlAttributeName xml_attribute_name("codeSpace");
	GPlatesModel::XmlAttributeValue xml_attribute_value(codespace);
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> inline_property_container =
			GPlatesModel::InlinePropertyContainer::create(property_name,
			gml_name, xml_attributes);

	return inline_property_container;
}


const GPlatesContrib::non_null_intrusive_ptr<GPlatesModel::FeatureHandle>
create_isochron(
		const unsigned long &plate_id,
		const double *points,
		unsigned num_points,
		const GPlatesModel::GeoTimeInstant &geo_time_instant_begin,
		const GPlatesModel::GeoTimeInstant &geo_time_instant_end,
		const UnicodeString &description,
		const UnicodeString &name,
		const UnicodeString &codespace_of_name)
{
	GPlatesModel::FeatureId feature_id;
	UnicodeString feature_type_string("gpml:Isochron");
	GPlatesModel::FeatureType feature_type(feature_type_string);
	GPlatesContrib::non_null_intrusive_ptr<GPlatesModel::FeatureHandle> feature_handle =
			GPlatesModel::FeatureHandle::create(feature_type, feature_id);

	boost::intrusive_ptr<GPlatesModel::PropertyContainer> reconstruction_plate_id_container =
			create_reconstruction_plate_id(plate_id);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> centre_line_of_container =
			create_centre_line_of(points, num_points);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> valid_time_container =
			create_valid_time(geo_time_instant_begin, geo_time_instant_end);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> description_container =
			create_description(description);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> name_container =
			create_name(name, codespace_of_name);

	feature_handle->properties().push_back(reconstruction_plate_id_container);
	feature_handle->properties().push_back(centre_line_of_container);
	feature_handle->properties().push_back(valid_time_container);
	feature_handle->properties().push_back(description_container);
	feature_handle->properties().push_back(name_container);

	return feature_handle;
}


struct RotationFileFiveTuple {
	double time;
	double lat_of_euler_pole;
	double lon_of_euler_pole;
	double rotation_angle;
	const char *comment;
};


const boost::intrusive_ptr<GPlatesModel::PropertyContainer>
create_total_reconstruction_pole(
		const RotationFileFiveTuple *five_tuples,
		unsigned num_five_tuples)
{
	std::vector<GPlatesModel::GpmlTimeSample> time_samples;
	GPlatesModel::TemplateTypeParameterType value_type("gpml:FiniteRotation");

	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes;
	GPlatesModel::XmlAttributeName xml_attribute_name("frame");
	GPlatesModel::XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	for (unsigned i = 0; i < num_five_tuples; ++i) {
		std::pair<double, double> gpml_euler_pole =
				std::make_pair(five_tuples[i].lon_of_euler_pole, five_tuples[i].lat_of_euler_pole);
		boost::intrusive_ptr<GPlatesModel::GpmlFiniteRotation> gpml_finite_rotation =
				GPlatesModel::GpmlFiniteRotation::create(gpml_euler_pole,
				five_tuples[i].rotation_angle);

		GPlatesModel::GeoTimeInstant geo_time_instant(five_tuples[i].time);
		boost::intrusive_ptr<GPlatesModel::GmlTimeInstant> gml_time_instant =
				GPlatesModel::GmlTimeInstant::create(geo_time_instant, xml_attributes);

		UnicodeString comment_string(five_tuples[i].comment);
		boost::intrusive_ptr<GPlatesModel::XsString> gml_description =
				GPlatesModel::XsString::create(comment_string);

		time_samples.push_back(GPlatesModel::GpmlTimeSample(gpml_finite_rotation, gml_time_instant,
				gml_description, value_type));
	}

	boost::intrusive_ptr<GPlatesModel::GpmlInterpolationFunction> gpml_finite_rotation_slerp =
			GPlatesModel::GpmlFiniteRotationSlerp::create(value_type);

	boost::intrusive_ptr<GPlatesModel::PropertyValue> gpml_irregular_sampling =
			GPlatesModel::GpmlIrregularSampling::create(time_samples,
			gpml_finite_rotation_slerp, value_type);

	UnicodeString property_name_string("gpml:totalReconstructionPole");
	GPlatesModel::PropertyName property_name(property_name_string);
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes2;
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> inline_property_container =
			GPlatesModel::InlinePropertyContainer::create(property_name,
			gpml_irregular_sampling, xml_attributes2);

	return inline_property_container;
}


const GPlatesContrib::non_null_intrusive_ptr<GPlatesModel::FeatureHandle>
create_total_recon_seq(
		const unsigned long &fixed_plate_id,
		const unsigned long &moving_plate_id,
		const RotationFileFiveTuple *five_tuples,
		unsigned num_five_tuples)
{
	GPlatesModel::FeatureId feature_id;
	UnicodeString feature_type_string("gpml:TotalReconstructionSequence");
	GPlatesModel::FeatureType feature_type(feature_type_string);
	GPlatesContrib::non_null_intrusive_ptr<GPlatesModel::FeatureHandle> feature_handle =
			GPlatesModel::FeatureHandle::create(feature_type, feature_id);

	boost::intrusive_ptr<GPlatesModel::PropertyContainer> total_reconstruction_pole_container =
			create_total_reconstruction_pole(five_tuples, num_five_tuples);
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> fixed_reference_frame_container =
			create_reference_frame_plate_id(fixed_plate_id, "gpml:fixedReferenceFrame");
	boost::intrusive_ptr<GPlatesModel::PropertyContainer> moving_reference_frame_container =
			create_reference_frame_plate_id(moving_plate_id, "gpml:movingReferenceFrame");

	feature_handle->properties().push_back(total_reconstruction_pole_container);
	feature_handle->properties().push_back(fixed_reference_frame_container);
	feature_handle->properties().push_back(moving_reference_frame_container);

	return feature_handle;
}


GPlatesModel::FeatureStoreRootHandle::iterator
create_feature_collection(
		boost::intrusive_ptr<GPlatesModel::FeatureStoreRootHandle> feature_store_root)
{
	GPlatesModel::DummyTransactionHandle transaction;
	GPlatesContrib::non_null_intrusive_ptr<GPlatesModel::FeatureCollectionHandle> feature_collection =
			GPlatesModel::FeatureCollectionHandle::create();
	return feature_store_root->append_feature_collection(feature_collection, transaction);
}


GPlatesModel::Model::Model():
	d_feature_store(FeatureStore::create()),
	d_isochrons(::create_feature_collection(d_feature_store->root())),
	d_total_recon_seqs(::create_feature_collection(d_feature_store->root()))
{
	static const unsigned long plate_id1 = 501;
	// lon, lat, lon, lat... is how GML likes it.
	static const double points1[] = {
		69.2877, -5.5765,
		69.1323, -4.8556,
		69.6092, -4.3841,
		69.2748, -3.9554,
		69.7079, -3.3680,
		69.4119, -3.0486,
		69.5999, -2.6304,
		68.9400, -1.8446,
	};
	static const unsigned num_points1 = sizeof(points1) / sizeof(points1[0]);
	GeoTimeInstant geo_time_instant_begin1(10.9);
	GeoTimeInstant geo_time_instant_end1 =
			GeoTimeInstant::create_distant_future();
	UnicodeString description1("CARLSBERG RIDGE, INDIA-AFRICA ANOMALY 5 ISOCHRON");
	UnicodeString name1("Izzy the Isochron");
	UnicodeString codespace_of_name1("EarthByte");

	GPlatesContrib::non_null_intrusive_ptr<FeatureHandle> isochron1 =
			create_isochron(plate_id1, points1, num_points1, geo_time_instant_begin1,
			geo_time_instant_end1, description1, name1, codespace_of_name1);

	static const unsigned long plate_id2 = 702;
	// lon, lat, lon, lat... is how GML likes it.
	static const double points2[] = {
		41.9242, -34.9340,
		42.7035, -33.4482,
		44.8065, -33.5645,
		44.9613, -33.0805,
		45.6552, -33.2601,
		46.3758, -31.6947,
	};
	static const unsigned num_points2 = sizeof(points2) / sizeof(points2[0]);
	GeoTimeInstant geo_time_instant_begin2(83.5);
	GeoTimeInstant geo_time_instant_end2 =
			GeoTimeInstant::create_distant_future();
	UnicodeString description2("SOUTHWEST INDIAN RIDGE, MADAGASCAR-ANTARCTICA ANOMALY 34 ISOCHRON");
	UnicodeString name2("Ozzy the Isochron");
	UnicodeString codespace_of_name2("EarthByte");

	GPlatesContrib::non_null_intrusive_ptr<FeatureHandle> isochron2 =
			create_isochron(plate_id2, points2, num_points2, geo_time_instant_begin2,
			geo_time_instant_end2, description2, name2, codespace_of_name2);

	static const unsigned long plate_id3 = 511;
	// lon, lat, lon, lat... is how GML likes it.
	static const double points3[] = {
		76.6320, -18.1374,
		77.9538, -19.1216,
		77.7709, -19.4055,
		80.1582, -20.6289,
		80.3237, -20.3765,
		81.1422, -20.7506,
		80.9199, -21.2669,
		81.8522, -21.9828,
	};
	static const unsigned num_points3 = sizeof(points3) / sizeof(points3[0]);
	GeoTimeInstant geo_time_instant_begin3(40.1);
	GeoTimeInstant geo_time_instant_end3 =
			GeoTimeInstant::create_distant_future();
	UnicodeString description3("SEIR CROZET AND CIB, CENTRAL INDIAN BASIN-ANTARCTICA ANOMALY 18 ISOCHRON");
	UnicodeString name3("Uzi the Isochron");
	UnicodeString codespace_of_name3("EarthByte");

	GPlatesContrib::non_null_intrusive_ptr<FeatureHandle> isochron3 =
			create_isochron(plate_id3, points3, num_points3, geo_time_instant_begin3,
			geo_time_instant_end3, description3, name3, codespace_of_name3);

	DummyTransactionHandle transaction_iso1;
	(*d_isochrons)->append_feature(isochron1, transaction_iso1);
	transaction_iso1.commit();

	DummyTransactionHandle transaction_iso2;
	(*d_isochrons)->append_feature(isochron2, transaction_iso2);
	transaction_iso2.commit();

	DummyTransactionHandle transaction_iso3;
	(*d_isochrons)->append_feature(isochron3, transaction_iso3);
	transaction_iso3.commit();

	static const unsigned long fixed_plate_id1 = 511;
	static const unsigned long moving_plate_id1 = 501;
	static const RotationFileFiveTuple five_tuples1[] = {
		//	time	e.lat	e.lon	angle	comment
		{	0.0,	90.0,	0.0,	0.0,	"IND-CIB India-Central Indian Basin"	},
		{	9.9,	-8.7,	76.9,	2.75,	"IND-CIB AN 5 JYR 7/4/89"	},
		{	20.2,	-5.2,	74.3,	5.93,	"IND-CIB Royer & Chang 1991"	},
		{	83.5,	-5.2,	74.3,	5.93,	"IND-CIB switchover"	},
	};
	static const unsigned num_five_tuples1 = sizeof(five_tuples1) / sizeof(five_tuples1[0]);

	GPlatesContrib::non_null_intrusive_ptr<FeatureHandle> total_recon_seq1 =
			create_total_recon_seq(fixed_plate_id1, moving_plate_id1, five_tuples1,
			num_five_tuples1);

	static const unsigned long fixed_plate_id2 = 702;
	static const unsigned long moving_plate_id2 = 501;
	static const RotationFileFiveTuple five_tuples2[] = {
		//	time	e.lat	e.lon	angle	comment
		{	83.5,	22.8,	19.1,	-51.28,	"IND-MAD"	},
		{	88.0,	19.8,	27.2,	-59.16,	" RDM/chris 30/11/2001"	},
		{	120.4,	24.02,	32.04,	-53.01,	"IND-MAD M0 RDM 21/01/02"	},
	};
	static const unsigned num_five_tuples2 = sizeof(five_tuples2) / sizeof(five_tuples2[0]);

	GPlatesContrib::non_null_intrusive_ptr<FeatureHandle> total_recon_seq2 =
			create_total_recon_seq(fixed_plate_id2, moving_plate_id2, five_tuples2,
			num_five_tuples2);

	static const unsigned long fixed_plate_id3 = 501;
	static const unsigned long moving_plate_id3 = 502;
	static const RotationFileFiveTuple five_tuples3[] = {
		//	time	e.lat	e.lon	angle	comment
		{	0.0,	0.0,	0.0,	0.0,	"SLK-IND Sri Lanka-India"	},
		{	75.0,	0.0,	0.0,	0.0,	"SLK-ANT Sri Lanka-Ant"	},
		{	90.0,	21.97,	72.79,	-10.13,	"SLK-IND M9 FIT CG01/04-"	},
		{	129.5,	21.97,	72.79,	-10.13,	"SLK-IND M9 FIT CG01/04-for sfs in Enderby"	},
	};
	static const unsigned num_five_tuples3 = sizeof(five_tuples3) / sizeof(five_tuples3[0]);

	GPlatesContrib::non_null_intrusive_ptr<FeatureHandle> total_recon_seq3 =
			create_total_recon_seq(fixed_plate_id3, moving_plate_id3, five_tuples3,
			num_five_tuples3);

	DummyTransactionHandle transaction_trs1;
	(*d_total_recon_seqs)->append_feature(total_recon_seq1, transaction_trs1);
	transaction_trs1.commit();

	DummyTransactionHandle transaction_trs2;
	(*d_total_recon_seqs)->append_feature(total_recon_seq2, transaction_trs2);
	transaction_trs2.commit();

	DummyTransactionHandle transaction_trs3;
	(*d_total_recon_seqs)->append_feature(total_recon_seq3, transaction_trs3);
	transaction_trs3.commit();
}


void
GPlatesModel::Model::create_reconstruction(
		std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> > &
				point_reconstructions,
		std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> > &
				polyline_reconstructions,
		const double &time,
		unsigned long root)
{
	ReconstructionTree recon_tree;
	ReconstructionTreePopulator rtp(time, recon_tree);

	// Populate the reconstruction tree with our total recon seqs.
	FeatureCollectionHandle::iterator iter2 = (*d_total_recon_seqs)->begin();
	FeatureCollectionHandle::iterator end2 = (*d_total_recon_seqs)->end();
	for ( ; iter2 != end2; ++iter2) {
		(*iter2)->accept_visitor(rtp);
	}

	// Build the reconstruction tree, using 'root' as the root of the tree.
	recon_tree.build_tree(root);

	ReconstructedFeatureGeometryPopulator rfgp(time, root,
			recon_tree, point_reconstructions, polyline_reconstructions);

	// Populate the vectors with reconstructed feature geometries from our isochrons.
	FeatureCollectionHandle::iterator iter3 = (*d_isochrons)->begin();
	FeatureCollectionHandle::iterator end3 = (*d_isochrons)->end();
	for ( ; iter3 != end3; ++iter3) {
		(*iter3)->accept_visitor(rfgp);
	}
}

using namespace boost::python;

void
GPlatesModel::export_Model()
{
	class_<GPlatesModel::Model>("Model", init<>())
		.def("create_reconstruction", &GPlatesModel::Model::create_reconstruction)
	;
}

