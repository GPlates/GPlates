/* $Id$ */

/**
 * @file
 * Contains the main function of the GPlates no-GUI demo.
 *
 * This file used to be "model/compilation_test.cc".  It was used during the implementation of the
 * new GPGIM-1.5-based Model, to check that the new Model code compiled without error and ran
 * correctly.  The code in this file constructs some hard-coded GPGIM features (which are
 * minimalist but otherwise structurally accurate) and outputs them as GPML.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#include <vector>
#include <iostream>
#include <utility>  /* std::pair */

#include "app-logic/ReconstructParams.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGraph.h"
#include "app-logic/ReconstructionGraphBuilder.h"
#include "app-logic/ReconstructionGraphPopulator.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructUtils.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureCollectionRevision.h"
#include "model/FeatureHandle.h"
#include "model/FeatureRevision.h"
#include "model/Model.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/XmlNode.h"

#include "file-io/GpmlOutputVisitor.h"
#include "file-io/GpmlPropertyStructuralTypeReader.h"
#include "file-io/PlatesLineFormatWriter.h"

#include "file-io/GpmlReader.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/FileInfo.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"

#include "property-values/GpmlPlateId.h"
#include "property-values/XsString.h"
#include "property-values/StructuralType.h"


const GPlatesModel::FeatureHandle::weak_ref
create_isochron(
		GPlatesModel::ModelInterface &model,
		GPlatesModel::FeatureCollectionHandle::weak_ref &target_collection,
		const unsigned long &plate_id,
		const double *coords,
		unsigned num_coords,
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_begin,
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_end,
		const GPlatesUtils::UnicodeString &geographic_description,
		const GPlatesUtils::UnicodeString &name,
		const GPlatesUtils::UnicodeString &codespace_of_name)
{
	GPlatesModel::FeatureType feature_type = GPlatesModel::FeatureType::create_gpml("Isochron");
	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			GPlatesModel::FeatureHandle::create(target_collection, feature_type);

	const std::vector<double> coords_vector(coords, coords + num_coords);

	// Wrap a "gpml:plateId" in a "gpml:ConstantValue" and append it as the
	// "gpml:reconstructionPlateId" property.
	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type recon_plate_id =
			GPlatesPropertyValues::GpmlPlateId::create(plate_id);
	feature_handle->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
				GPlatesModel::ModelUtils::create_gpml_constant_value(recon_plate_id)));

	std::list<GPlatesMaths::PointOnSphere> points;
	GPlatesMaths::populate_point_on_sphere_sequence(points, coords_vector);
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
			GPlatesMaths::PolylineOnSphere::create(points);
	GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
			GPlatesPropertyValues::GmlLineString::create(polyline);
	GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
			GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(gml_orientable_curve);

	feature_handle->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("centerLineOf"),
				property_value));

	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time =
			GPlatesModel::ModelUtils::create_gml_time_period(
					geo_time_instant_begin, geo_time_instant_end);
	feature_handle->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gml("validTime"),
				gml_valid_time));

	GPlatesPropertyValues::XsString::non_null_ptr_type gml_description = 
			GPlatesPropertyValues::XsString::create(geographic_description);
	feature_handle->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gml("description"),
				gml_description));

	GPlatesPropertyValues::XsString::non_null_ptr_type gml_name =
			GPlatesPropertyValues::XsString::create(name);
	feature_handle->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gml("name"),
				gml_name, 
				"codeSpace",
				codespace_of_name));

	return feature_handle;
}


void
traverse_recon_tree_recursive(
		const GPlatesAppLogic::ReconstructionTree::Edge &edge)
{
	std::cout << " * Children of pole (fixed plate: "
			<< edge.get_fixed_plate()
			<< ", moving plate: "
			<< edge.get_moving_plate()
			<< ")\n";

	const GPlatesAppLogic::ReconstructionTree::edge_list_type &child_edges = edge.get_child_edges();
	GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator iter = child_edges.begin();
	GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator end = child_edges.end();
	for ( ; iter != end; ++iter)
	{
		std::cout << " - FiniteRotation: " << iter->get_relative_rotation() << "\n";
		std::cout << "    with absolute rotation: " << iter->get_composed_absolute_rotation() << "\n";
		std::cout << "    and fixed plate: " << iter->get_fixed_plate() << std::endl;
		std::cout << "    and moving plate: " << iter->get_moving_plate() << std::endl;
	}

	iter = child_edges.begin();
	for ( ; iter != end; ++iter)
	{
		::traverse_recon_tree_recursive(*iter);
	}
}


void
traverse_recon_tree(
		const GPlatesAppLogic::ReconstructionTree &recon_tree)
{
	std::cout << " * Root-most poles:\n";

	const GPlatesAppLogic::ReconstructionTree::edge_list_type &anchor_plate_edges =
			recon_tree.get_anchor_plate_edges();
	GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator iter = anchor_plate_edges.begin();
	GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator end = anchor_plate_edges.end();
	for ( ; iter != end; ++iter)
	{
		std::cout << " - FiniteRotation: " << iter->get_relative_rotation() << "\n";
		std::cout << "    with absolute rotation: " << iter->get_composed_absolute_rotation() << "\n";
		std::cout << "    and fixed plate: " << iter->get_fixed_plate() << std::endl;
		std::cout << "    and moving plate: " << iter->get_moving_plate() << std::endl;
	}
	iter = anchor_plate_edges.begin();
	for ( ; iter != end; ++iter)
	{
		::traverse_recon_tree_recursive(*iter);
	}
}


const std::pair<GPlatesModel::FeatureCollectionHandle::weak_ref,
		GPlatesModel::FeatureCollectionHandle::weak_ref>
populate_feature_store(
		GPlatesModel::ModelInterface &model)
{
#if 0
	// The following code is no longer used, but I'm retaining it here because I think it's
	// important that this question gets a definite answer.  (If I cut the code (and comment), I
	// know the question will be forgotten.)

	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type isochrons =
			GPlatesModel::FeatureCollectionHandle::create();

	// FIXME:  Should the next four operations occur in any particular order?  Is there any
	// problem in "committing" features to a feature collection, when the feature collection is
	// not yet in the feature store root?  Or any problem committing modifications to a
	// feature, when the feature is not yet in a feature collection?  Any problems to do with
	// dangling pointers-to-handles, for example, if the handle is destroyed (since it is only
	// referenced by an intrusive-ptr on the stack) after it is supposedly committed (and there
	// is thus a TransactionItem which holds a pointer to it)?
	//
	// Further thoughts, 2007-04-10:  If the TransactionItem were to hold an *intrusive_ptr*
	// to a *Handle (rather than a "raw" pointer to a *Handle), then the *Handle could never be
	// accidentally destroyed before the TransactionItem.  The ref-count of the *Handle
	// instance could become quite high (if a feature-collection is having many features added
	// and removed, and is thus referenced by many TransactionItem instances), but hopefully,
	// since we're using a 'long' to contain the count (a max of at least 2147483647, right?),
	// there won't be a danger of overflow.
	//
	// So now, the next question:  Should the 'create' functions of FeatureCollectionHandle and
	// FeatureHandle instances require TransactionHandle references to be passed?  And should
	// functions be added to FeatureStoreRootHandle and FeatureCollectionHandle, to create
	// empty feature-collections and features (respectively) which are already inside the
	// FeatureStoreRootHandle or FeatureCollectionHandle instance?  And should client code be
	// encouraged (or even forced) to use these functions, rather than instantiating
	// feature-collections and features (respectively) on their own?

	GPlatesModel::FeatureStoreRootHandle::collections_iterator isochrons_iter =
			feature_store->root()->append_feature_collection(isochrons, transaction_iso_coll);

	isochrons->append_feature(isochron1, transaction_iso1);
	isochrons->append_feature(isochron2, transaction_iso2);
	isochrons->append_feature(isochron3, transaction_iso3);
#endif

	GPlatesModel::FeatureCollectionHandle::weak_ref isochrons =
			GPlatesModel::FeatureCollectionHandle::create(model->root());
	GPlatesModel::FeatureCollectionHandle::weak_ref total_recon_seqs =
			GPlatesModel::FeatureCollectionHandle::create(model->root());

	static const unsigned long plate_id1 = 501;
	// lon, lat, lon, lat... is how GML likes it.
	static const double coords1[] = {
		69.2877,
		-5.5765,
		69.1323,
		-4.8556,
		69.6092,
		-4.3841,
		69.2748,
		-3.9554,
		69.7079,
		-3.3680,
		69.4119,
		-3.0486,
		69.5999,
		-2.6304,
		68.9400,
		-1.8446,
	};
	static const unsigned num_coords1 = sizeof(coords1) / sizeof(coords1[0]);
	GPlatesPropertyValues::GeoTimeInstant geo_time_instant_begin1(10.9);
	GPlatesPropertyValues::GeoTimeInstant geo_time_instant_end1 =
			GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	GPlatesUtils::UnicodeString description1("CARLSBERG RIDGE, INDIA-AFRICA ANOMALY 5 ISOCHRON");
	GPlatesUtils::UnicodeString name1("Izzy the Isochron");
	GPlatesUtils::UnicodeString codespace_of_name1("EarthByte");

	GPlatesModel::FeatureHandle::weak_ref isochron1 =
			create_isochron(model, isochrons, plate_id1, coords1, num_coords1,
					geo_time_instant_begin1, geo_time_instant_end1,
					description1, name1, codespace_of_name1);

	static const unsigned long plate_id2 = 702;
	// lon, lat, lon, lat... is how GML likes it.
	static const double coords2[] = {
		41.9242,
		-34.9340,
		42.7035,
		-33.4482,
		44.8065,
		-33.5645,
		44.9613,
		-33.0805,
		45.6552,
		-33.2601,
		46.3758,
		-31.6947,
	};
	static const unsigned num_coords2 = sizeof(coords2) / sizeof(coords2[0]);
	GPlatesPropertyValues::GeoTimeInstant geo_time_instant_begin2(83.5);
	GPlatesPropertyValues::GeoTimeInstant geo_time_instant_end2 =
			GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	GPlatesUtils::UnicodeString description2("SOUTHWEST INDIAN RIDGE, MADAGASCAR-ANTARCTICA ANOMALY 34 ISOCHRON");
	GPlatesUtils::UnicodeString name2("Ozzy the Isochron");
	GPlatesUtils::UnicodeString codespace_of_name2("EarthByte");

	GPlatesModel::FeatureHandle::weak_ref isochron2 =
			create_isochron(model, isochrons, plate_id2, coords2, num_coords2,
					geo_time_instant_begin2, geo_time_instant_end2,
					description2, name2, codespace_of_name2);

	static const unsigned long plate_id3 = 511;
	// lon, lat, lon, lat... is how GML likes it.
	static const double coords3[] = {
		76.6320,
		-18.1374,
		77.9538,
		-19.1216,
		77.7709,
		-19.4055,
		80.1582,
		-20.6289,
		80.3237,
		-20.3765,
		81.1422,
		-20.7506,
		80.9199,
		-21.2669,
		81.8522,
		-21.9828,
	};
	static const unsigned num_coords3 = sizeof(coords3) / sizeof(coords3[0]);
	GPlatesPropertyValues::GeoTimeInstant geo_time_instant_begin3(40.1);
	GPlatesPropertyValues::GeoTimeInstant geo_time_instant_end3 =
			GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	GPlatesUtils::UnicodeString description3("SEIR CROZET AND CIB, CENTRAL INDIAN BASIN-ANTARCTICA ANOMALY 18 ISOCHRON");
	GPlatesUtils::UnicodeString name3("Uzi the Isochron");
	GPlatesUtils::UnicodeString codespace_of_name3("EarthByte");

	GPlatesModel::FeatureHandle::weak_ref isochron3 =
			create_isochron(model, isochrons, plate_id3, coords3, num_coords3,
					geo_time_instant_begin3, geo_time_instant_end3,
					description3, name3, codespace_of_name3);

	static const unsigned long fixed_plate_id1 = 511;
	static const unsigned long moving_plate_id1 = 501;
	static const GPlatesModel::ModelUtils::TotalReconstructionPole five_tuples1[] = {
		//	time	e.lat	e.lon	angle	comment
		{	0.0,	90.0,	0.0,	0.0,	"IND-CIB India-Central Indian Basin"	},
		{	9.9,	-8.7,	76.9,	2.75,	"IND-CIB AN 5 JYR 7/4/89"	},
		{	20.2,	-5.2,	74.3,	5.93,	"IND-CIB Royer & Chang 1991"	},
		{	83.5,	-5.2,	74.3,	5.93,	"IND-CIB switchover"	},
	};
	static const unsigned num_five_tuples1 = sizeof(five_tuples1) / sizeof(five_tuples1[0]);
	static const std::vector<GPlatesModel::ModelUtils::TotalReconstructionPole> five_tuples_vec1(
				five_tuples1, five_tuples1 + num_five_tuples1);

	GPlatesModel::FeatureHandle::weak_ref total_recon_seq1 =
			create_total_recon_seq(model, total_recon_seqs, fixed_plate_id1,
					moving_plate_id1, five_tuples_vec1);

	static const unsigned long fixed_plate_id2 = 702;
	static const unsigned long moving_plate_id2 = 501;
	static const GPlatesModel::ModelUtils::TotalReconstructionPole five_tuples2[] = {
		//	time	e.lat	e.lon	angle	comment
		{	83.5,	22.8,	19.1,	-51.28,	"IND-MAD"	},
		{	88.0,	19.8,	27.2,	-59.16,	" RDM/chris 30/11/2001"	},
		{	120.4,	24.02,	32.04,	-53.01,	"IND-MAD M0 RDM 21/01/02"	},
	};
	static const unsigned num_five_tuples2 = sizeof(five_tuples2) / sizeof(five_tuples2[0]);
	static const std::vector<GPlatesModel::ModelUtils::TotalReconstructionPole> five_tuples_vec2(
				five_tuples2, five_tuples2 + num_five_tuples2);

	GPlatesModel::FeatureHandle::weak_ref total_recon_seq2 =
			create_total_recon_seq(model, total_recon_seqs, fixed_plate_id2,
					moving_plate_id2, five_tuples_vec2);

	static const unsigned long fixed_plate_id3 = 501;
	static const unsigned long moving_plate_id3 = 502;
	static const GPlatesModel::ModelUtils::TotalReconstructionPole five_tuples3[] = {
		//	time	e.lat	e.lon	angle	comment
		{	0.0,	0.0,	0.0,	0.0,	"SLK-IND Sri Lanka-India"	},
		{	75.0,	0.0,	0.0,	0.0,	"SLK-ANT Sri Lanka-Ant"	},
		{	90.0,	21.97,	72.79,	-10.13,	"SLK-IND M9 FIT CG01/04-"	},
		{	129.5,	21.97,	72.79,	-10.13,	"SLK-IND M9 FIT CG01/04-for sfs in Enderby"	},
	};
	static const unsigned num_five_tuples3 = sizeof(five_tuples3) / sizeof(five_tuples3[0]);
	static const std::vector<GPlatesModel::ModelUtils::TotalReconstructionPole> five_tuples_vec3(
				five_tuples3, five_tuples3 + num_five_tuples3);

	GPlatesModel::FeatureHandle::weak_ref total_recon_seq3 =
			create_total_recon_seq(model, total_recon_seqs, fixed_plate_id3,
					moving_plate_id3, five_tuples_vec3);

	return std::make_pair(isochrons, total_recon_seqs);
}


void
output_as_gpml(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &features)
{
	QFile standard_output;
	standard_output.open(stdout, QIODevice::WriteOnly);
	GPlatesFileIO::GpmlOutputVisitor v(&standard_output, features);

	GPlatesModel::FeatureCollectionHandle::iterator begin = features->begin();
	GPlatesModel::FeatureCollectionHandle::iterator end = features->end();
	for ( ; begin != end; ++begin) {
		v.visit_feature(begin);
	}
}


void
output_reconstructions(
		GPlatesModel::FeatureCollectionHandle::weak_ref isochrons,
		GPlatesModel::FeatureCollectionHandle::weak_ref total_recon_seqs)
{
	static const double recon_times_to_test[] = {
		0.0,
		10.0,
		20.0,
		80.0,
		83.5,
		85.0,
		90.0
	};
	static const unsigned num_recon_times_to_test =
			sizeof(recon_times_to_test) / sizeof(recon_times_to_test[0]);

	GPlatesAppLogic::ReconstructionGraphBuilder graph_builder;
	GPlatesAppLogic::ReconstructionGraphPopulator rgp(graph_builder);

	GPlatesModel::FeatureCollectionHandle::iterator iter1 = total_recon_seqs->begin();
	for ( ; iter1 != total_recon_seqs->end(); ++iter1)
	{
		rgp.visit_feature(iter1);
	}

	std::cout << "\n--> Building graph\n";
	GPlatesAppLogic::ReconstructionGraph::non_null_ptr_to_const_type graph = graph_builder.build_graph();

	for (unsigned i = 0; i < num_recon_times_to_test; ++i) {
		double recon_time = recon_times_to_test[i];

		const GPlatesModel::integer_plate_id_type anchor_plate_id = 501;

		std::cout << "\n===> Reconstruction time: " << recon_time << std::endl;

		std::cout << "\n--> Building tree, root node: " << anchor_plate_id << " \n";
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree =
				GPlatesAppLogic::ReconstructionTree::create(
						graph,
						recon_time,
						anchor_plate_id);

		traverse_recon_tree(*tree);

		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstructable_features_collection(
				1, isochrons);

		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstruction_features_collection(
				1, total_recon_seqs);

		typedef std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> rfg_seq_type;
		rfg_seq_type reconstructed_feature_geometries;
		GPlatesAppLogic::ReconstructUtils::reconstruct(
				reconstructed_feature_geometries,
				recon_time,
				anchor_plate_id,
				reconstructable_features_collection,
				reconstruction_features_collection);

		std::cout << "<> After feature geometry reconstructions, there are\n   "
				<< reconstructed_feature_geometries.size()
				<< " reconstructed geometries."
				<< std::endl;

		std::cout << " > The reconstructed polylines are:\n";
		rfg_seq_type::const_iterator iter3 = reconstructed_feature_geometries.begin();
		const rfg_seq_type::const_iterator end3 = reconstructed_feature_geometries.end();
		for ( ; iter3 != end3; ++iter3) {
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type rfg = *iter3;

			// We only care about polylines (since all the geometries in this function
			// *should* be polylines), so let's just use a dynamic cast.
			const GPlatesMaths::PolylineOnSphere *polyline =
					dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(rfg->reconstructed_geometry().get());
			if ( ! polyline) {
				// Why wasn't it a polyline?
				std::cerr << "Why wasn't it a polyline?" << std::endl;
				std::exit(1);
			}

			GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter4 =
					polyline->vertex_begin();
			GPlatesMaths::PolylineOnSphere::vertex_const_iterator end4 =
					polyline->vertex_end();

			{
				GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter4);
				std::cout << "  - Polyline: (" << llp.latitude() << ", " << llp.longitude() << ")";
			}
			for (++iter4 ; iter4 != end4; ++iter4) {
				GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter4);
				std::cout << ", (" << llp.latitude() << ", " << llp.longitude() << ")";
			}
			std::cout << std::endl;
		}

#if 0
		std::cout << "\n--> Building tree, root node: 511\n";
		reconstruction->reconstruction_tree().build_tree(511);
		traverse_recon_tree(reconstruction->reconstruction_tree());

		std::cout << "\n--> Building tree, root node: 702\n";
		reconstruction->reconstruction_tree().build_tree(702);
		traverse_recon_tree(reconstruction->reconstruction_tree());

		std::cout << "\n--> Building tree, root node: 502\n";
		reconstruction->reconstruction_tree().build_tree(502);
		traverse_recon_tree(reconstruction->reconstruction_tree());
#endif

		std::cout << std::endl;
	}
}


#include <QtXml/QDomDocument>

int
main(int argc, char *argv[])
{
	GPlatesMaths::assert_has_infinity_and_nan();

	GPlatesModel::ModelInterface model;

	// Used to read structural types from a GPML file.
	GPlatesFileIO::GpmlPropertyStructuralTypeReader::non_null_ptr_to_const_type gpml_structural_type_reader =
			GPlatesFileIO::GpmlPropertyStructuralTypeReader::create();


	std::pair<GPlatesModel::FeatureCollectionHandle::weak_ref,
			GPlatesModel::FeatureCollectionHandle::weak_ref>
			isochrons_and_total_recon_seqs =
					::populate_feature_store(model);

	GPlatesModel::FeatureCollectionHandle::weak_ref isochrons =
			isochrons_and_total_recon_seqs.first;
	GPlatesModel::FeatureCollectionHandle::weak_ref total_recon_seqs =
			isochrons_and_total_recon_seqs.second;

	::output_as_gpml(isochrons);
	//::output_reconstructions(isochrons, total_recon_seqs);

	// Test GPML 1.6 reader.
	if (argc > 1) {

		std::cout << "Attempting to read \"" << argv[1] << "\"." << std::endl;
		QString filename = argv[1];

		GPlatesFileIO::FileInfo fileinfo(filename);
		GPlatesModel::ModelInterface new_model;
		GPlatesFileIO::ReadErrorAccumulation accum;
		bool contains_unsaved_changes;


		// Create a file with an empty feature collection.
		GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(fileinfo);

		// Read new features from the file into the empty feature collection.
		GPlatesFileIO::GpmlReader::read_file(
				file->get_reference(),
				gpml_structural_type_reader,
				accum,
				contains_unsaved_changes);

		GPlatesModel::FeatureCollectionHandle::weak_ref features =
				file->get_reference().get_feature_collection();
		::output_as_gpml(features);

#if 0
		QFile file(filename);
		file.open(QIODevice::ReadOnly | QIODevice::Text);
		QXmlStreamReader reader(&file);
		while ( ! reader.atEnd()) {
			reader.readNext();
		   	if (reader.isStartElement())
				break;
		}
		if ( ! reader.atEnd()) {
			GPlatesModel::XmlNode::non_null_ptr_type xml = 
				GPlatesModel::XmlElementNode::create(reader);

			QFile out;
			out.open(1, QIODevice::WriteOnly | QIODevice::Text);
			QXmlStreamWriter writer(&out);
			writer.setAutoFormatting(true);
			writer.writeStartDocument("1.0");
			xml->write_to(writer);
		}
#endif
	}
	
	return 0;
}
