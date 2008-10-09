/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
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
#include <sstream>
#include <iostream>

#include <QLocale>
#include <QDebug>
#include <QList>
#include <QString>
#include <boost/none.hpp>

#include "PlatepolygonResolver.h"

#include "feature-visitors/ValueFinder.h"
#include "feature-visitors/ReconstructedFeatureGeometryFinder.h"

#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlOldPlatesHeader.h"

#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/ProximityCriteria.h"
#include "maths/PolylineIntersections.h"

#include "gui/ProximityTests.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/FeatureHandleToOldId.h"

GPlatesFeatureVisitors::PlatepolygonResolver::PlatepolygonResolver(
			const double &recon_time,
			unsigned long root_plate_id,
			GPlatesModel::Reconstruction &recon,
			GPlatesModel::ReconstructionTree &recon_tree,
			GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder &finder,
			reconstruction_geometries_type &reconstructed_geometries,
			bool should_keep_features_without_recon_plate_id):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_root_plate_id(GPlatesModel::integer_plate_id_type(root_plate_id)),
	d_recon_ptr(&recon),
	d_recon_tree_ptr(&recon_tree),
	d_recon_finder_ptr(&finder),
	d_reconstruction_geometries_to_populate(&reconstructed_geometries),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id)
{  
	d_num_features = 0;
	d_num_platepolygons = 0;
}



void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_num_features += 1;

	//
	// This feature property value is what determines if the feature is a Platepolygon 
	//

	// Create a visitor to check for boundarList property 
	static const GPlatesModel::PropertyName prop_name = 
		GPlatesModel::PropertyName::create_gpml("boundaryList");

//	static const GPlatesModel::PropertyName prop_name = 
//		GPlatesModel::PropertyName::create_gpml("boundary");


	GPlatesFeatureVisitors::ValueFinder finder(prop_name);
	finder.visit_feature_handle(feature_handle);

#if 0
	// super short-cut for features without boundary list properties
	if (finder.found_values_begin() == finder.found_values_end()) 
	{ 
		return; 
	}
#endif

	// else process this feature:
	// create an accumulator struct 
	// visit the properties once to check times and rot ids
	// visit the properties once to reconstruct ?
	// parse the boundary string
	// resolve the boundary vertex_list

	d_accumulator = ReconstructedFeatureGeometryAccumulator();

	// Now visit each of the properties in turn, twice -- firstly, to find a reconstruction
	// plate ID and to determine whether the feature is defined at this reconstruction time;
	// after that, to perform the reconstructions (if appropriate) using the plate ID.

	// The first time through, we're not reconstructing, just gathering information.
	d_accumulator->d_perform_reconstructions = false;

	visit_feature_properties(feature_handle);


	// So now we've visited the properties of this feature.  Let's find out if we were able
	// to obtain all the information we need.
	if ( ! d_accumulator->d_feature_is_defined_at_recon_time) {
		// Quick-out: No need to continue.
		d_accumulator = boost::none;
		return;
	}


	if ( ! d_accumulator->d_recon_plate_id) 
	{
		// We couldn't obtain the reconstruction plate ID.
		// So, how do we react to this situation?  Do we ignore features which don't have a
		// reconsruction plate ID, or do we "reconstruct" their geometries using the
		// identity rotation, so the features simply sit still on the globe?  Fortunately,
		// the client code has already told us how it wants us to behave...
		if ( ! d_should_keep_features_without_recon_plate_id) {
			d_accumulator = boost::none;
			return;
		}
	}
	else
	{
		// We obtained the reconstruction plate ID.  We now have all the information we
		// need to reconstruct according to the reconstruction plate ID.
		d_accumulator->d_recon_rotation =
			d_recon_tree_ptr->get_composed_absolute_rotation(*(d_accumulator->d_recon_plate_id)).first;
	}


	// Now for the second pass through the properties of the feature:  
	// This time we reconstruct any geometries we find.
	d_accumulator->d_perform_reconstructions = true;
	visit_feature_properties(feature_handle);

	// parse the list of boundary features into nodes of m_boundary_list 
	parse_boundary_string( feature_handle );

	// iterate over m_boundary_list
	resolve_boundary();

	d_accumulator = boost::none;
}



void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::visit_feature_properties() " << std::endl;

	GPlatesModel::FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			// FIXME: This d_current_property thing could go in the {Const,}FeatureVisitor base.
			d_accumulator->d_current_property = iter;
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_inline_property_container(
		GPlatesModel::InlinePropertyContainer &inline_property_container)
{
// std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::visit_inline_property_container() " << std::endl;

	visit_property_values(inline_property_container);
}

#if 0

void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	using namespace GPlatesMaths;

	if (d_accumulator->d_perform_reconstructions) {
		GPlatesModel::FeatureHandle::properties_iterator property = *(d_accumulator->d_current_property);

		if (d_accumulator->d_recon_plate_id) {
			const FiniteRotation &r = *d_accumulator->d_recon_rotation;
			PolylineOnSphere::non_null_ptr_to_const_type reconstructed_polyline =
					r * gml_line_string.polyline();

			ReconstructedFeatureGeometry rfg(reconstructed_polyline,
					*d_accumulator->d_current_property->collection_handle_ptr(),
					*d_accumulator->d_current_property,
					*d_accumulator->d_recon_plate_id);

			d_reconstructed_geometries_to_populate->push_back(rfg);
			d_reconstructed_geometries_to_populate->back().set_reconstruction_ptr(d_recon_ptr);

		} else {
			// We must be reconstructing using the identity rotation.
			ReconstructedFeatureGeometry rfg(gml_line_string.polyline(),
					*d_accumulator->d_current_property->collection_handle_ptr(),
					*d_accumulator->d_current_property);

			d_reconstructed_geometries_to_populate->push_back(rfg);
			d_reconstructed_geometries_to_populate->back().set_reconstruction_ptr(d_recon_ptr);
		}
	}
}


void
GGPlatesFeatureVisitors::PlatepolygonResolver::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	using namespace GPlatesMaths;

	if (d_accumulator->d_perform_reconstructions) {
		GPlatesModel::FeatureHandle::properties_iterator property = *(d_accumulator->d_current_property);

		if (d_accumulator->d_recon_plate_id) {
			const FiniteRotation &r = *d_accumulator->d_recon_rotation;
			PointOnSphere::non_null_ptr_to_const_type reconstructed_point =
					r * gml_point.point();

			ReconstructedFeatureGeometry rfg(reconstructed_point,
					*d_accumulator->d_current_property->collection_handle_ptr(),
					*d_accumulator->d_current_property,
					*d_accumulator->d_recon_plate_id);
			d_reconstructed_geometries_to_populate->push_back(rfg);
			d_reconstructed_geometries_to_populate->back().set_reconstruction_ptr(d_recon_ptr);
		} else {
			// We must be reconstructing using the identity rotation.
			ReconstructedFeatureGeometry rfg(gml_point.point(),
					*d_accumulator->d_current_property->collection_handle_ptr(),
					*d_accumulator->d_current_property);
			d_reconstructed_geometries_to_populate->push_back(rfg);
			d_reconstructed_geometries_to_populate->back().set_reconstruction_ptr(d_recon_ptr);
		}
	}
}
#endif


void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gml_time_period(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	// std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::visit_gml_time_period() " << std::endl;
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	if ( ! d_accumulator->d_perform_reconstructions) {
		// We're gathering information, not performing reconstructions.

		// Note that we're going to assume that we're in a property...
		if (d_accumulator->current_property_name() == valid_time_property_name) {
			// This time period is the "valid time" time period.
			if ( ! gml_time_period.contains(d_recon_time)) {
				// Oh no!  This feature instance is not defined at the recon time!
				d_accumulator->d_feature_is_defined_at_recon_time = false;
			}
		}
	}
}


void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	// std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_constant_value() " << std::endl;
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	// std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_plate_id() " << std::endl;
	static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	if ( ! d_accumulator->d_perform_reconstructions) {
		// We're gathering information, not performing reconstructions.

		// Note that we're going to assume that we're in a property...
		if (d_accumulator->current_property_name() == reconstruction_plate_id_property_name) {
			// This plate ID is the reconstruction plate ID.
			d_accumulator->d_recon_plate_id = gpml_plate_id.value();
		}
	}
}




void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_feature_reference(
	GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference)
{
#if 0
	d_output.writeStartGpmlElement("FeatureReference");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_feature_reference.feature_id().get());
		d_output.writeEndElement();


		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_feature_reference.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
#endif
}

void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_old_plates_header(
	GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
 		
// qDebug() << GPlatesUtils::make_qstring_from_icu_string( gpml_old_plates_header.geographic_description() );

#if 0
	d_output.writeStartGpmlElement("OldPlatesHeader");

		d_output.writeStartGpmlElement("regionNumber");
 		d_output.writeInteger(gpml_old_plates_header.region_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("referenceNumber");
 		d_output.writeInteger(gpml_old_plates_header.reference_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("stringNumber");
 		d_output.writeInteger(gpml_old_plates_header.string_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("geographicDescription");
 		d_output.writeText(gpml_old_plates_header.geographic_description());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("plateIdNumber");
 		d_output.writeInteger(gpml_old_plates_header.plate_id_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("ageOfAppearance");
 		d_output.writeDecimal(gpml_old_plates_header.age_of_appearance());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("ageOfDisappearance");
 		d_output.writeDecimal(gpml_old_plates_header.age_of_disappearance());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCode");
 		d_output.writeText(gpml_old_plates_header.data_type_code());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCodeNumber");
 		d_output.writeInteger(gpml_old_plates_header.data_type_code_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCodeNumberAdditional");
 		d_output.writeText(gpml_old_plates_header.data_type_code_number_additional());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("conjugatePlateIdNumber");
 		d_output.writeInteger(gpml_old_plates_header.conjugate_plate_id_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("colourCode");
 		d_output.writeInteger(gpml_old_plates_header.colour_code());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("numberOfPoints");
 		d_output.writeInteger(gpml_old_plates_header.number_of_points());
		d_output.writeEndElement();

	d_output.writeEndElement();  // </gpml:OldPlatesHeader>
#endif
}


void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_piecewise_aggregation(
		GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
#if 0
	d_output.writeStartGpmlElement("PiecewiseAggregation");
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_piecewise_aggregation.value_type());
		d_output.writeEndElement();

		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
				gpml_piecewise_aggregation.time_windows().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
				gpml_piecewise_aggregation.time_windows().end();
		for ( ; iter != end; ++iter) 
		{
			d_output.writeStartGpmlElement("timeWindow");
				write_gpml_time_window(*iter);
			d_output.writeEndElement();
		}
	d_output.writeEndElement();  // </gpml:IrregularSampling>
#endif
}

void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_property_delegate(
	GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate)
{
#if 0
	d_output.writeStartGpmlElement("PropertyDelegate");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_property_delegate.feature_id().get());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("targetProperty");
			writeTemplateTypeParameterType(d_output, gpml_property_delegate.target_property());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_property_delegate.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
#endif
}

#if 0
void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_revision_id(
	GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id)
{
	d_output.writeText(gpml_revision_id.value().get());
}
#endif

#if 0
void
GPlatesFeatureVisitors::PlatepolygonResolver::write_gpml_time_window(
	GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	d_output.writeStartGpmlElement("TimeWindow");
		d_output.writeStartGpmlElement("timeDependentPropertyValue");
			gpml_time_window.time_dependent_value()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("validTime");
			gpml_time_window.valid_time()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_time_window.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement(); // </gpml:TimeWindow>
}
#endif


void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_topological_polygon(
	GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon)
{
#if 0
	d_output.writeStartGpmlElement("TopologicalPolygon");
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::const_iterator iter;
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::const_iterator end;
	iter = gpml_toplogical_polygon.sections().begin();
	end = gpml_toplogical_polygon.sections().end();

	for ( ; iter != end; ++iter) 
	{
		d_output.writeStartGpmlElement("section");
		(*iter)->accept_visitor(*this);
		d_output.writeEndElement();
	}
	d_output.writeEndElement();  // </gpml:TopologicalPolygon>
#endif
}

void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_topological_line_section(
	GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
#if 0
std::cerr << ("visit_gpml_topological_line_section") << std::endl;
	d_output.writeStartGpmlElement("TopologicalLineSection");

		d_output.writeStartGpmlElement("sourceGeometry");
			( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); // delgate 
		d_output.writeEndElement();

		// boost::optional<GpmlTopologicalIntersection>
		if ( gpml_toplogical_line_section.get_start_intersection() )
		{
			d_output.writeStartGpmlElement("startIntersection");
				gpml_toplogical_line_section.get_start_intersection()->accept_visitor(*this);
			d_output.writeEndElement();
		}

		// check for endInterse
		if ( gpml_toplogical_line_section.get_end_intersection() )
		{
			d_output.writeStartGpmlElement("endIntersection");
				gpml_toplogical_line_section.get_end_intersection()->accept_visitor(*this);
			d_output.writeEndElement();
		}
		
		d_output.writeStartGpmlElement("reverseOrder");
			d_output.writeBoolean( gpml_toplogical_line_section.get_reverse_order() );
		d_output.writeEndElement();

	d_output.writeEndElement();  // </gpml:TopologicalPolygon>
#endif
}


void
GPlatesFeatureVisitors::PlatepolygonResolver::visit_gpml_topological_intersection(
	GPlatesPropertyValues::GpmlTopologicalIntersection &gpml_toplogical_intersection)
{
#if 0
	d_output.writeStartGpmlElement("TopologicalIntersection");

		d_output.writeStartGpmlElement("intersectionGeometry");
			// visit the delegate
			gpml_toplogical_intersection.intersection_geometry()->accept_visitor(*this); 
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("referencePoint");
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point =
					gpml_toplogical_intersection.reference_point();
			visit_gml_point(*gml_point);
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("referencePointPlateId");
			// visit the delegate
			gpml_toplogical_intersection.reference_point_plate_id()->accept_visitor(*this);
		d_output.writeEndElement();

	d_output.writeEndElement();
#endif
}




// ZZZ 

void
GPlatesFeatureVisitors::PlatepolygonResolver::parse_boundary_string(
		GPlatesModel::FeatureHandle &feature_handle)
{

	// just in case we hit errors during parsing 
	std::stringstream err_msg; 

	//
	// This feature property value is what determines if the feature is a Platepolygon 
	//
	// Create a visitor to check for boundarList property 
	static const GPlatesModel::PropertyName prop_name = 
		GPlatesModel::PropertyName::create_gpml("boundaryList");

	GPlatesFeatureVisitors::ValueFinder finder(prop_name);
	finder.visit_feature_handle(feature_handle);

	// super short-cut for features without boundary list properties
	if (finder.found_values_begin() == finder.found_values_end()) 
	{ 
		return; 
	}


	// else parse the string into nodes for the m_boundary_list
	d_num_platepolygons += 1;

	// The value from boundaryList property
	std::string full_string = *finder.found_values_begin();
	// std::cout << "full_string = " << full_string << std::endl;

	// parse the boundaryList string into a list 
	
	// first clear the list
	m_boundary_list.clear();

	// tmp place to hold sting tokens
	std::vector<std::string> node_tokens;

	// loop over the full_list string finding new line delimiters
	while (full_string.find("$", 0) != std::string::npos)
	{ 
		// store the position of the delimiter
		size_t pos = full_string.find("$", 0);

		// get the token
		std::string temp = full_string.substr(0, pos);

		// erase it from the source 
		full_string.erase(0, pos + 1);

		// and put it into the array
		node_tokens.push_back(temp);
	}

	// loop over each boundary feature item and create a struct 
	std::vector<std::string>::iterator iter = node_tokens.begin();

	for ( ; iter != node_tokens.end() ; ++iter)
	{
			// std::cout << "iter = " << *iter << std::endl; 

			std::string node_str = *iter;

			// tmp place to hold sting tokens
			std::vector<std::string> tokens;

			// NOTE:
			// The hash is used to delimit boundary node fields
			// if it it changed here, be sure to also change
			// GPlatesGeo::operator<< for PlatePolygon::PlatePolygon 
			// in geo/PlatePolygon.cc
			//

			// loop over the string finding hash delimiters
			while (node_str.find("#", 0) != std::string::npos)
			{ 
				// store the position of the delimiter
				size_t pos = node_str.find("#", 0);

				// get the token
				std::string temp = node_str.substr(0, pos);

				// erase it from the source 
				node_str.erase(0, pos + 1);

				// and put it into the array
				tokens.push_back(temp);
			}

			// the last token is all alone,
			// but has delimiter at end, remove it
			tokens.push_back(node_str);

			// Error checking on number of tokens found
			if ( tokens.size() != 11 )
			{
				std::ostringstream oss;
				oss << "Cannot parse boundary feature node line: "
					<< "expected 10 comma delimited tokens, "
					<< "got: " << tokens.size() << " tokens."
					<< std::endl;
				std::cout << "ERROR: " << oss.str() << std::endl;
				continue;
			}

			// convert token strings into boudary feature node data
			// via direct string copy and via string streams

			int type;
			float lat;
			float lon;
			float closeness;
			bool use_reverse;
			bool use_head_prev;
			bool use_tail_prev;
			bool use_head_next;
			bool use_tail_next;

			std::string fid = tokens[0];

			std::istringstream iss_type(tokens[1]);
			iss_type >> type;

			std::istringstream iss_lat( tokens[2] );
			iss_lat >> lat;

			std::istringstream iss_lon( tokens[3] );
			iss_lon >> lon;

			std::istringstream iss_clo( tokens[4] );
			iss_clo >> closeness;

			std::istringstream iss_rev( tokens[5] );
			iss_rev >> use_reverse;

			std::istringstream iss_uhp( tokens[6] );
			iss_uhp >> use_head_prev;

			std::istringstream iss_utp( tokens[7] );
			iss_utp >> use_tail_prev;

			std::istringstream iss_uhn( tokens[8] );
			iss_uhn >> use_head_next;

			std::istringstream iss_utn( tokens[9] );
			iss_utn >> use_tail_next;

			// Check if feature exisits in reconstuction
			if ( d_recon_finder_ptr->get_geometry_from_feature_id(fid) == boost::none )
			{
				err_msg << "  MISSING feature ref.: " << fid << std::endl;
				continue; // to next boundary node
			}

// FIXME: enable to show verbose checks
#if 0
			else
			{
				err_msg << "  FOUND   feature ref.: " << fid << std::endl;
			}
#endif 

#if 0
std::cout << "GetPlatePolygonFromPolyLine: fid = " << fid << std::endl;
std::cout << "GetPlatePolygonFromPolyLine: lat = " << lat << std::endl;
std::cout << "GetPlatePolygonFromPolyLine: lon = " << lon << std::endl;
std::cout << "GetPlatePolygonFromPolyLine: use_reverse = " << use_reverse << std::endl;
std::cout << "GetPlatePolygonFromPolyLine: use_head_prev = " << use_head_prev << std::endl;
std::cout << "GetPlatePolygonFromPolyLine: use_tail_prev = " << use_tail_prev << std::endl;
std::cout << "GetPlatePolygonFromPolyLine: use_head_next = " << use_head_next << std::endl;
std::cout << "GetPlatePolygonFromPolyLine: use_tail_next = " << use_tail_next << std::endl;
#endif

			bool test_ok = true;

			if ( use_tail_next == 0 && use_head_next == 0 )
			{
				err_msg 
				<< "  in NODE: " << fid << std::endl;
				err_msg
				<< "    BOTH use TAIL && use HEAD from NEXT intersection flags == 0"
				<< std::endl;
				test_ok = false;
			}

            if ( use_tail_next == 1 && use_head_next == 1)
			{
				err_msg 
				<< "  in NODE: " << fid << std::endl;
				err_msg
				<< "    BOTH use TAIL && use HEAD from NEXT intersection flags == 1"
				<< std::endl;
				test_ok = false;
			}

			if ( use_tail_prev == 0 && use_head_prev == 0 )
			{
				err_msg 
				<< "  in NODE: " << fid << std::endl;
				err_msg
				<< "    BOTH use TAIL && use HEAD from PREV intersection flags == 0"
				<< std::endl;
				test_ok = false;
			}

            if ( use_tail_prev == 1 && use_head_prev == 1)
			{
				err_msg 
				<< "  in NODE: " << fid << std::endl;
				err_msg
				<< "    BOTH use TAIL && use HEAD from PREV intersection flags == 1"
				<< std::endl;
				test_ok = false;
			}

// FIX enable this code to add OK messages 
#if 0
			if (test_ok)
			{
				err_msg << "  in NODE: " << fid << std::endl;
				err_msg << "    OK on all intersection flags tests " 
					<< std::endl;
			}
#endif

			// convert type
			GPlatesGlobal::FeatureTypes feature_type(GPlatesGlobal::UNKNOWN_FEATURE);

			switch ( type )
			{
				case GPlatesGlobal::POINT_FEATURE:
					feature_type = GPlatesGlobal::POINT_FEATURE;
					break;

				case GPlatesGlobal::LINE_FEATURE:
					feature_type = GPlatesGlobal::LINE_FEATURE;
					break;

				default :
					std::ostringstream oss;
					oss << "UNKNOWN boundary feature node type! "
						<< std::endl;
					// throw FileFormatException(oss.str().c_str());
					break;
			}
			

			// convert coordinates
			GPlatesMaths::LatLonPoint llp( lat, lon);
			GPlatesMaths::PointOnSphere click_point = GPlatesMaths::make_point_on_sphere(llp);

			// empty list place holder
 			std::list<GPlatesMaths::PointOnSphere> empty_vert_list;
 			empty_vert_list.clear();

			// create a boundary feature struct
			BoundaryFeature bf(
				feature_handle,
				fid,
				feature_type,
				empty_vert_list,
				click_point,
				closeness,
				use_reverse,
				0,
				0,
				use_head_prev,
				use_tail_prev,
				use_head_next,
				use_tail_next
			);

			// append this feature to the 
			// platepolygon's list
			m_boundary_list.push_back( bf );

		} // end of loop over boundary feature nodes


		// report any errors

#if 0 // VERBOSE 
		if ( !err_msg.str().empty() )
		{
			std::string msg;
			msg.append( "WARNING from polygon:");
			// msg.append( polygon->GetFeatureId() );
			msg.append( "\n" );
			msg.append( err_msg.str() );
			std::cout << msg << std::endl;
		}
#endif 

}

void
GPlatesFeatureVisitors::PlatepolygonResolver::resolve_boundary()
{
	BoundaryFeatureList_type::iterator iter = m_boundary_list.begin();

	// clear the working list
	m_vertex_list.clear();

	// iterate over the list of boundary features to get the list of vertices 
	m_vertex_list = get_vertex_list( m_boundary_list.begin(), m_boundary_list.end() );

// FIXME: diagnostic output
	std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::resolve_boundary() " << std::endl;
	std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver:: m_vertex_list.size() " << m_vertex_list.size() << std::endl;

	for ( ; iter != m_boundary_list.end() ; ++iter)
	{
		std::cout << "node id = " << iter->m_feature_id << std::endl;
	}

	if (m_vertex_list.size() == 0 ) {
		return;
	}

	// create a polygon on sphere
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type reconstructed_geom = 
		GPlatesMaths::PolygonOnSphere::create_on_heap( m_vertex_list );
		
	// create a new RFG
	GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
		GPlatesModel::ReconstructedFeatureGeometry::create(
			reconstructed_geom,
			*d_accumulator->d_current_property->collection_handle_ptr(),
			*d_accumulator->d_current_property,
			d_accumulator->d_recon_plate_id,
			d_accumulator->d_time_of_appearance);

	d_reconstruction_geometries_to_populate->push_back(rfg_ptr);
 	d_reconstruction_geometries_to_populate->back()->set_reconstruction_ptr(d_recon_ptr);
}


void
GPlatesFeatureVisitors::PlatepolygonResolver::report()
{
	std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::report() " << std::endl;
	std::cout << "number features visited = " << d_num_features << std::endl;
	std::cout << "number platepolygons visited = " << d_num_platepolygons << std::endl;
}



bool
GPlatesFeatureVisitors::PlatepolygonResolver::is_feature_id_in_boundary( std::string fid )
{
	// short cut for empty list: 
	if ( m_boundary_list.empty() )
	{
		return false;
	}

	BoundaryFeatureList_type::iterator iter = m_boundary_list.begin();

	for ( ; iter != m_boundary_list.end(); ++iter)
	{
		if (iter->m_feature_id == fid)
		{
			return true;
		}
	}
	return false;
}


//
// Insert and remove operations
// NOTE: see also .h 
//



// insert at position pos a feature selection
// set neighbor relations
//
void
GPlatesFeatureVisitors::PlatepolygonResolver::insert( 
	BoundaryFeatureList_type::iterator pos, 
	BoundaryFeature feature )
{
	// get the feature id of the feature to be inserted
	// always good to know who you are inserting
	// std::string fid = feature.m_feature->GetFeatureId();
	std::string fid = GPlatesUtils::get_old_id( feature.m_feature );

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::insert: " << "fid=" << fid;
std::cout << std::endl;
#endif

	// short cut for empty list: 
	if ( m_boundary_list.empty() )
	{
		// just insert at pos the feature
		m_boundary_list.insert( pos, feature );
		return;
	}


	// short cut for inserting a POINT_FEATURE: 
	// POINT_FEATURE do not use neighbor relations
	// no need to test or set
	if ( feature.m_feature_type == GPlatesGlobal::POINT_FEATURE )
	{
		// just insert at pos the feature
		m_boundary_list.insert( pos, feature );
		return;
	}


	// short cut for inserting onto a list with only one node
	// set relations with previous node 
	if ( m_boundary_list.size() == 1 )
	{
		// get the PREV node
		BoundaryFeature &prev = m_boundary_list.front();

		std::string prev_fid = GPlatesUtils::get_old_id( prev.m_feature );

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::insert: " << "m_boundary_list.size()==1; " << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::insert: " << "prev_fid=" << prev_fid;
std::cout << std::endl;
#endif

		if (prev.m_feature_type == GPlatesGlobal::POINT_FEATURE)
		{
			// only one PREV boundary feature and it is a point

			// no need to test or set neighbor relations 
			// short cut 
			// just insert at pos the feature
			m_boundary_list.insert( pos, feature );
			return;
		} 
		else if ( prev.m_feature_type == GPlatesGlobal::LINE_FEATURE )
		{
			// only one boundary feature node and it is a line

			//
			// Set relations for this feature
			//

			// adjust this feature node's NEXT relation with prev
			set_node_relation( 
				GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT, 
				feature, 
				prev );

			// adjust this feature node's PREV relation with prev
			set_node_relation( 
				GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV, 
				feature, 
				prev );


			//
			// Set relations for neighbor
			//

			// adjust prev node's NEXT relation with feature
			set_node_relation(
				GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT,
				prev,
				feature );

			// adjust prev node's PREV relation with feature
			set_node_relation(
				GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV,
				prev,
				feature );

			//
			// relations have been set, insert the feature
			//
			m_boundary_list.insert( pos, feature );

		}

		return;
	} // end of if (m_boundary_list.size() == 1)

	//
	// else there are 2 or more features on the list
	//

	// Iterators to the boundary feature list
	BoundaryFeatureList_type::iterator prev;
	BoundaryFeatureList_type::iterator next;

	// iterator math to close the loop
	if ( pos == m_boundary_list.end() )
	{
		// insert just before end:
		next = m_boundary_list.begin(); // close the loop back to beg
		prev = pos;
		--prev; // locate the prev node
	}
	else if ( pos == m_boundary_list.begin() )
	{
		// insert just before begin
		next = pos;
		prev = --m_boundary_list.end();
	}
	else
	{
		// insert just before pos
		next = pos;
		prev = pos;
		--prev; // locate the prev node
	}

	// get a feature ids for iters
	std::string prev_fid = GPlatesUtils::get_old_id( prev->m_feature);
	std::string next_fid = GPlatesUtils::get_old_id( next->m_feature);

// FIX : remove this diagnostic output
#if 0
std::cout << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::insert: " << "PREV_fid=" << prev_fid << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::insert: " << "NEXT_fid=" << next_fid << std::endl;
std::cout << std::endl;
#endif

	// if prev is a LINE set relations
	if ( prev->m_feature_type == GPlatesGlobal::LINE_FEATURE )
	{
		//
		// Set relations for this feature
		//

		// adjust this feature node's PREV relation with prev
		set_node_relation( 
			GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV, 
			feature, 
			*prev );

		//
		// Set relations for PREV neighbor
		//

		// adjust prev node's NEXT relation with feature
		set_node_relation(
			GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT,
			*prev,
			feature );
	}

	// if next is a LINE set relations
	if ( next->m_feature_type == GPlatesGlobal::LINE_FEATURE )
	{
			//
			// Set relations for this feature
			//

			// adjust this feature node's NEXT relation with next
			set_node_relation( 
				GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT, 
				feature, 
				*next );

			//
			// Set relations for NEXT neighbor
			//

			// adjust next node's PREV relation with feature
			set_node_relation(
				GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV,
				*next,
				feature );
	}

	//
	// insert the feature
	//
	m_boundary_list.insert( pos, feature );
}



// set_node_relation is a general purpose function 
// that tests for and sets a boundary feature node's flags.
//
// relation is the specific relation to set
// node 1 is the node to change
// node 2 is a neighbor node already existing on list
void
GPlatesFeatureVisitors::PlatepolygonResolver::set_node_relation( 
	GPlatesFeatureVisitors::PlatepolygonResolver::NeighborRelation relation,
	BoundaryFeature &node1,
	BoundaryFeature &node2)
{

	// "alwys good to know who you are dealing with"
	std::string node1_fid = GPlatesUtils::get_old_id( node1.m_feature);
	std::string node2_fid = GPlatesUtils::get_old_id( node2.m_feature);


#if 0
// FIX : remove this diagnostic output
std::cout << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "relation=" << relation << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "node1_fid=" << node1_fid << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "node2_fid=" << node2_fid << std::endl;
std::cout << std::endl;
#endif


	// short cuts for POINT_FEATURE : no change to node1
	if ( node1.m_feature_type == GPlatesGlobal::POINT_FEATURE ) { return; }
	if ( node2.m_feature_type == GPlatesGlobal::POINT_FEATURE ) { return; }

	//
	// nodes must be GPlatesGlobal::LINE_FEATURE test for intersection
	//
	
	// tmp variables to hold results of processing node1 vs node2
	std::list<GPlatesMaths::PointOnSphere> node1_vertex_list;
	std::list<GPlatesMaths::PointOnSphere> node2_vertex_list;
	std::list<GPlatesMaths::PointOnSphere> test_vertex_list;

	// get verts for node1 from Layout 
	d_recon_finder_ptr->get_vertex_list_from_feature_id(
		node1_vertex_list,
		node1_fid );

	// re-populate the node list, just in case:
	if ( node1.m_vertex_list.empty() )
	{
		node1.m_vertex_list = node1_vertex_list;
	}

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "node1_vertex_list.size=" << node1_vertex_list.size() << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "node1.m_vert_list.size=" << node1.m_vertex_list.size() << std::endl;
#endif

	
	
	// get verts for node2 from Layout 
	d_recon_finder_ptr->get_vertex_list_from_feature_id(
		node2_vertex_list,
		node2_fid );

	// re-populate the node lis,  just in case:
	if ( node2.m_vertex_list.empty() )
	{
		node2.m_vertex_list = node2_vertex_list;
	}

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "node2_vertex_list.size=" << node2_vertex_list.size() << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "node2.m_vert_list.size=" << node2.m_vertex_list.size() << std::endl;
#endif

	// skip features not found, or missing from Layout
	if ( node2_vertex_list.empty() or node1_vertex_list.empty() )
	{
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "node2_vertex_list.empty()" << std::endl;
#endif
		// no need to check further
		// no change to node1
		return;
	}

	//
	// create polylines for each boundary feature node
	//
//REMOVE:	GPlatesMaths::PolylineOnSphere node1_polyline =
//REMOVE:		GPlatesMaths::PolylineOnSphere::create( node1.m_vertex_list );


	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type node1_polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap( node1.m_vertex_list );

//REMOVE:	GPlatesMaths::PolylineOnSphere node2_polyline = 
	//REMOVE:	GPlatesMaths::PolylineOnSphere::create( node2.m_vertex_list );

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type node2_polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap( node2.m_vertex_list );
	//
	// variables to save results of intersection
	//
	std::list<GPlatesMaths::PointOnSphere> intersection_points;
	std::list<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> partitioned_lines;

// FIXME
// PROXIMITY
	//
	// closeness vars used in is_close_to tests below
	//
	GPlatesMaths::real_t closeness_inclusion_threshold = 0.9;
		 //GPlatesControls::GuiCalls::GetCloseThreshold();

	const GPlatesMaths::real_t cit_sqrd =
		closeness_inclusion_threshold * closeness_inclusion_threshold;
		
	const GPlatesMaths::real_t latitude_exclusion_threshold = sqrt(1.0 - cit_sqrd);

	// this one gets filled by calls to is_close_to()
	GPlatesMaths::real_t closeness;

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: closeness_inclusion_threshold=" << closeness_inclusion_threshold << std::endl;
#endif

	//
	// test for intersection
	//
	int num_intersect = 0;
	num_intersect = GPlatesMaths::PolylineIntersections::partition_intersecting_polylines(
		*node1_polyline,
		*node2_polyline,
		intersection_points,
		partitioned_lines);

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "num_intersect = " << num_intersect << std::endl;
#endif

	// switch on relation enum to set node1's member data
	switch ( relation )
	{
		case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV :
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: INTERSECT_PREV: " << std::endl;
#endif
			node1.m_num_intersections_with_prev = num_intersect;
			break;

		case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT:
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: INTERSECT_NEXT: " << std::endl;
#endif
			node1.m_num_intersections_with_next = num_intersect;
			break;

		case GPlatesFeatureVisitors::PlatepolygonResolver::NONE :
		case GPlatesFeatureVisitors::PlatepolygonResolver::OTHER :
		default :
			// somthing bad happened
			// freak out
			break;
	}

	// 

	if ( num_intersect == 0 )
	{
		// no change to node1
		return;
	}
	else if ( num_intersect == 1)
	{
		// pair of polyline lists from intersection
		std::pair<
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		> parts;

		// unambiguously identify partitioned lines:
		// parts.first.front is the head of node1_polyline
		// parts.first.back is the tail of node1_polyline
		// parts.second.front is the head of node2_polyline
		// parts.second.back is the tail of node2_polyline
		parts = GPlatesMaths::PolylineIntersections::identify_partitioned_polylines(
			*node1_polyline,
			*node2_polyline,
			intersection_points,
			partitioned_lines);

		// now check which element of parts.first
		// is close to node1's click_point:

		// check head
		if ( parts.first.front()->is_close_to(
				node1.m_click_point,
				closeness_inclusion_threshold,
				latitude_exclusion_threshold,
				closeness) )
		{

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "use HEAD of: " << node1_fid << std::endl;
#endif

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV :
					node1.m_use_head_from_intersect_prev = true;
					node1.m_use_tail_from_intersect_prev = false;
					break;
	
				case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT:
					node1.m_use_head_from_intersect_next = true;
					node1.m_use_tail_from_intersect_next = false;
					break;

				default:
					break;
			}

			// node1's relation has been set
			return;
		} 

// PROXIMITY

		// check tail
		if ( parts.first.back()->is_close_to(
				node1.m_click_point,
				closeness_inclusion_threshold,
				latitude_exclusion_threshold,
				closeness) )
		{
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << "use TAIL segment of   : " << node1_fid << std::endl;
#endif

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV :
					node1.m_use_tail_from_intersect_prev = true;
					node1.m_use_head_from_intersect_prev = false;
					break;
	
				case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT:
					node1.m_use_tail_from_intersect_next = true;
					node1.m_use_head_from_intersect_next = false;
					break;

				default:
					break;
			}

			// node1's relation has been set
			return;
		} 

		else
		{
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << std::endl
<< "WARN: click point not close to anything!" << std::endl
<< "WARN: Unable to set boundary feature intersection flags!" 
<< std::endl << std::endl;
		}

	} // end of else if ( num_intersect == 1 )
	else 
	{
		// num_intersect must be 2 or greater
		// oh no!
		// check for overlap ...
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::s_n_r: " << std::endl
<< "WARN: num_intersect=" << num_intersect
<< "WARN: Unable to set boundary feature intersection relations!" << std::endl
<< "WARN: Make sure boundary feature's only intersect once." 
<< std::endl << std::endl;
	}
}



//
// erase a boundary feature by look up on feature id
//
void
GPlatesFeatureVisitors::PlatepolygonResolver::erase( std::string fid )
{
	// iterator to list
	BoundaryFeatureList_type::iterator iter;

	// Iterate over boundary list
	for (iter = boundary_begin(); iter != boundary_end(); ++iter)
	{
		if ( GPlatesUtils::get_old_id( iter->m_feature) == fid )
		{
			// erase the item 
			m_boundary_list.erase( iter );
			return;
		}
	}
}



//
// Modifying operations
//

#if 0 // KILL
bool
GPlatesFeatureVisitors::PlatepolygonResolver::update_feature( std::string fid )
{
	// iterator to list
	BoundaryFeatureList_type::iterator iter;

	// Iterate over boundary list
	for (iter = boundary_begin(); iter != boundary_end(); ++iter)
	{
		if ( GPlatesUtils::get_old_id( iter->m_feature) == fid )
		{
			// Update reverse flag
			iter->m_use_reverse = iter->m_feature->GetReverse();
			return true;
		}
	}

	// not found;
	return false;
}
#endif // KILL

bool
GPlatesFeatureVisitors::PlatepolygonResolver::reselect_feature( BoundaryFeature feature )
{
	// iterators into list
	BoundaryFeatureList_type::iterator iter;
	BoundaryFeatureList_type::iterator prev;
	BoundaryFeatureList_type::iterator next;

	// fid of selection
	std::string fid = feature.m_feature_id;

	// Iterate over boundary list
	for (iter = boundary_begin(); iter != boundary_end(); ++iter)
	{
		// find selection
		if ( GPlatesUtils::get_old_id( iter->m_feature) == fid )
		{

			// Double check reverse state from old feature
			bool use_reverse = iter->m_use_reverse;
			feature.m_use_reverse = use_reverse;

			// re-assign iterators
			prev = iter;
			if (prev == boundary_begin() ) { prev = boundary_end(); }
			else { --prev; }
    		std::string prev_fid = prev->m_feature_id;

			next = iter;
			if (next == --boundary_end() ) { next = boundary_begin(); }
			else { ++next; }
    		std::string next_fid = next->m_feature_id;

#if 0
// FIX : remove this diagnostic output
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::reselect_feature: " << "PREV_fid=" 
<< prev_fid << "; list.size=" << prev->m_vertex_list.size() 
<< std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::reselect_feature: " << "ITER_fid=" 
<< fid << "; list.size=" << iter->m_vertex_list.size() 
<< std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::reselect_feature: " << "NEXT_fid=" 
<< next_fid << "; list.size=" << next->m_vertex_list.size() 
<< std::endl;
std::cout << std::endl;
#endif
			// if prev is a LINE set relations
			if ( prev->m_feature_type == GPlatesGlobal::LINE_FEATURE )
			{
				// Set relations for this feature
				// adjust this feature node's PREV relation with prev
				set_node_relation( 
					GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV,
					feature, 
					*prev );
			}

			// if next is a LINE set relations
			if ( next->m_feature_type == GPlatesGlobal::LINE_FEATURE )
			{
			    // Set relations for this feature
				// adjust this feature node's NEXT relation with next
			    set_node_relation(
			        GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT,
			        feature,
			        *next );
			}

			// insert new feature 
			m_boundary_list.insert( iter, feature );

			// erase old feature
			m_boundary_list.erase( iter );

			return true;
		}
	}

	// not found;
	return false;
}


bool
GPlatesFeatureVisitors::PlatepolygonResolver::insert_feature( 
	BoundaryFeature feature, std::string pos_fid)
{
	// iterators into list
	BoundaryFeatureList_type::iterator iter;
	BoundaryFeatureList_type::iterator prev;
	BoundaryFeatureList_type::iterator next;

	// fid of selection
	std::string fid = feature.m_feature_id;

	int counter = 0;
	// Iterate over boundary list
	for (iter = boundary_begin(); iter != boundary_end(); ++iter)
	{
		std::string test_fid = iter->m_feature_id;

		// find insert spot 
		if ( test_fid == pos_fid )
		{
			insert( iter, feature ); // INSERT
			return true;
		}
		counter = counter + 1;
	}
	// not found;
	return false;
}


bool
GPlatesFeatureVisitors::PlatepolygonResolver::remove_feature( BoundaryFeature feature )
{
	// iterators into list
	BoundaryFeatureList_type::iterator iter;
	BoundaryFeatureList_type::iterator prev;
	BoundaryFeatureList_type::iterator next;

	// fid of selection
	std::string fid = feature.m_feature_id;

	// Iterate over boundary list
	for (iter = boundary_begin(); iter != boundary_end(); ++iter)
	{
		// find selection
		if ( GPlatesUtils::get_old_id( iter->m_feature) == fid )
		{
			// re-assign iterators
			prev = iter;
			if (prev == boundary_begin() ) { prev = boundary_end(); }
			else { --prev; }
    		std::string prev_fid = prev->m_feature_id;

			next = iter;
			if (next == --boundary_end() ) { next = boundary_begin(); }
			else { ++next; }
    		std::string next_fid = next->m_feature_id;

			// make copies of prev and next
			BoundaryFeature new_prev = *prev;
			BoundaryFeature new_next = *next;

#if 0
// FIX : remove this diagnostic output
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::remove_feature: " 
<< "PREV_fid=" << new_prev.m_feature_id 
<< "; list.size=" << new_prev.m_vertex_list.size()
<< std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::remove_feature: " 
<< "NEXT_fid=" << new_next.m_feature_id 
<< "; list.size=" << new_next.m_vertex_list.size()
<< std::endl;
#endif
			// if prev is a LINE set relations for new_next
			if ( prev->m_feature_type == GPlatesGlobal::LINE_FEATURE )
			{
				// Set relations for this feature
				// adjust this feature node's PREV relation with prev
				set_node_relation( 
					GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV,
					new_next, 
					*prev );
			}

			// if next is a LINE set relations for new_prev
			if ( next->m_feature_type == GPlatesGlobal::LINE_FEATURE )
			{
			    // Set relations for this feature
				// adjust this feature node's NEXT relation with next
			    set_node_relation(
			        GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT,
			        new_prev,
			        *next );
			}

			// replace PREV
			m_boundary_list.insert( prev, new_prev );
			m_boundary_list.erase( prev );

			// replace NEXT
			m_boundary_list.insert( next, new_next );
			m_boundary_list.erase( next );

			// remove iter
			m_boundary_list.erase( iter );

			report();

			return true;
		}
	}
	// not found;
	return false;
}



// Traverse the boundary feature list and return the list of vertices
// found from processing boundary list nodes and their neighbor relations
std::list<GPlatesMaths::PointOnSphere>
GPlatesFeatureVisitors::PlatepolygonResolver::get_vertex_list(
	BoundaryFeatureList_type::iterator pos1,
	BoundaryFeatureList_type::iterator pos2)
{

	// Clear working variable
	std::list<GPlatesMaths::PointOnSphere> work_vertex_list;
	work_vertex_list.clear();

	// Clear subduction boundary component lists
	m_subduction_list.clear();
	m_subduction_sR_list.clear();
	m_subduction_sL_list.clear();
	m_ridge_transform_list.clear();

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l: " << "size=" << m_boundary_list.size() << std::endl;
#endif

	// 
	// super short cut for empty list:
	//
	if ( m_boundary_list.empty() )
	{
		return work_vertex_list;
	}

	//
	// super short cut for single feature on list
	//
	if ( m_boundary_list.size() == 1 )
	{
		BoundaryFeature node = m_boundary_list.front();

		// FIX : OLD std::string fid = node.m_feature);
		std::string fid = node.m_feature_id;
		
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l: " << "m_boundary_list.size()==1; " << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l: " << "fid=" << fid;
std::cout << std::endl;
#endif

		if (node.m_feature_type == GPlatesGlobal::POINT_FEATURE)
		{
			// only one boundary feature and it is a point
			// probably the case of user starting new platepolyon

			// find vertex of rotated point in layout
			// put direcly into work list
			d_recon_finder_ptr->get_vertex_list_from_feature_id( 
				work_vertex_list, 
				fid );

			// no boundary feature list neighbor processing needed
		}
		else if (node.m_feature_type == GPlatesGlobal::LINE_FEATURE)
		{
			// only one boundary feature and it is a line
			// probably the case of user starting new platepolyon

			// find vertex list from rotated polyline in layout
			// put direcly into work list
			d_recon_finder_ptr->get_vertex_list_from_feature_id( 
				work_vertex_list, 
				fid );

			// no boundary feature list neighbor processing needed
		}
		else
		{
			// FIX : boundary features must be POINT_FEATURE or LINE only
			// FIX : for now, send back an empty list 
			work_vertex_list.clear();
		}

		return work_vertex_list;
	}

	//
	// else the list size is > 1
	//

	// Iterators to the boundary feature list
	BoundaryFeatureList_type::iterator prev;
	BoundaryFeatureList_type::iterator iter; 
	BoundaryFeatureList_type::iterator next;

	//
	// iterate over the boundary list from pos1 to --pos2
	//
	for (iter = pos1; iter != pos2; ++iter)
	{
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l() step 1: iterator math" << std::endl;
#endif
		// 
		// Step 1: Iterator math 
		//

		// iterator math for prev
		if (iter == pos1) 
		{
			prev = pos2;
			--prev;
		}
		else
		{
			prev = iter;
			--prev;
		}

		// iterator math for next 
		next = iter;
		++next;
		if ( next == pos2 )
		{
			next = pos1;
		}

		//
		// Step 2: get feature ids for iters
		//
		std::string prev_fid = prev->m_feature_id;
		std::string iter_fid = iter->m_feature_id;
		std::string next_fid = next->m_feature_id;

#if 0
// FIX : remove this diagnostic output
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 2: " << "PREV_fid=" << prev_fid << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 2: " << "ITER_fid=" << iter_fid << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 2: " << "NEXT_fid=" << next_fid << std::endl;
std::cout << std::endl;
#endif

		// short cut for iter type == POINT_FEATURE
		if ( iter->m_feature_type == GPlatesGlobal::POINT_FEATURE )
		{
			// find verts for iter
			// put directly into work_vertex_list
			d_recon_finder_ptr->get_vertex_list_from_feature_id( 
				work_vertex_list, 
				iter_fid );

			continue; // to next iter to boundary feature in list
		}

		//
		// Double check that iter is a LINE
		//
		if ( iter->m_feature_type != GPlatesGlobal::LINE_FEATURE )
		{
			// FIXME: post error ?
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 2: != GPlatesGlobal::LINE_FEATURE " << "ITER_fid=" << iter_fid << std::endl;
std::cout << std::endl;
#endif
			continue; // to next iter 
		}

		//
		// Step 3 : Get iter vertex list from feature in the Layout
		//
		std::list<GPlatesMaths::PointOnSphere> iter_vertex_list;

		d_recon_finder_ptr->get_vertex_list_from_feature_id(
			iter_vertex_list,
			iter_fid );

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 3 gvl_fid: ";
std::cout << "iter_vertex_list.size()=" << iter_vertex_list.size() << "; ";
std::cout << std::endl;
#endif


		//
		// Step 4: Process test vertex list with neighbor relations
		//

		//
		// test with NEXT ; modify iter_vertex_list as needed
		//
		get_vertex_list_from_node_relation(
			GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT,
			*iter,
			*next,
			iter_vertex_list);

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 4 post-NEXT: "
<< "iter_vertex_list.size()=" << iter_vertex_list.size() << "; "
<< std::endl;
#endif

		//
		// test with PREV ; modify iter_vertex_list as needed
		//
		get_vertex_list_from_node_relation(
			GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV,
			*iter,
			*prev,
			iter_vertex_list);

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 4 post-PREV: "
<< "iter_vertex_list.size()=" << iter_vertex_list.size() << "; "
<< std::endl;
#endif

		// 
		// Step 5: test for reverse flag on node 
		//
		// check for reverse flag of boundary feature node 
		// NOT on original feature
		//
		if ( iter->m_use_reverse )
		{
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 5 rev?: " << "iter->m_use_reverse=true" << std::endl;
#endif
			iter_vertex_list.reverse();
		}


		//
		// Step 6: copy final processed vertex list to working list
		//
		copy(
			iter_vertex_list.begin(),
			iter_vertex_list.end(),
			back_inserter( work_vertex_list )
		);

#if 0 // TYPE 
		//
		// Step 6.1: Splice out specific boundary types:
		// copy final processed vertex list to sub_boundary 
		//
		std::string type_code = iter->m_feature->GetTypeCode();

		if ( ( type_code == "sR" ) || ( type_code == "sL" ) ) 
		{
			std::string use_reverse = "no";

			// Adjust the type code to match the orientation 
			// of how the vertices are used in the polygon boundary
			if ( iter->m_use_reverse )
			{
				use_reverse = "yes"; 

				if (type_code == "sR") 
				{ 
					type_code = "sL";  
//std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 6: sR -> sL" << std::endl; 
				}
				else if (type_code == "sL") 
				{ 
					type_code = "sR";  
//std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 6: sL -> sR" << std::endl; 
				}
			}

			std::string feature_tag = 
				">" + type_code + 
				"    # name: " + iter->m_feature->GetName() +
				"    # polygon: " + GetName() +
				"    # use_reverse: " + use_reverse +
				"\n";

// std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 6: tag=" << feature_tag;


			// Get the vertices from this segment
			std::list<GPlatesMaths::PointOnSphere> tmp_vertex_list;
			tmp_vertex_list.clear();

			copy(
				iter_vertex_list.begin(),
				iter_vertex_list.end(),
				back_inserter( tmp_vertex_list )
			);

			// Append this subduction boundary to the list
			m_subduction_list.push_back( 
				SubductionBoundaryFeature(
					GPlatesUtils::get_old_id( iter->m_feature),
					feature_tag,
					tmp_vertex_list)
			);

			if (type_code == "sL") 
			{
//std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 6: sL_list.push_back: "
//<< "tmp_vertex_list.size()=" << tmp_vertex_list.size() << "; "
//<< std::endl;
				m_subduction_sL_list.push_back( 
					SubductionBoundaryFeature(
						GPlatesUtils::get_old_id( iter->m_feature),
						feature_tag,
						tmp_vertex_list)
				);
			}

			if (type_code == "sR") 
			{
//std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 6: sR_list.push_back: "
//<< "tmp_vertex_list.size()=" << tmp_vertex_list.size() << "; "
//<< std::endl;
				m_subduction_sR_list.push_back( 
					SubductionBoundaryFeature(
						GPlatesUtils::get_old_id( iter->m_feature),
						feature_tag,
						tmp_vertex_list)
				);
			}
		}
		else
		{
			std::string use_reverse = "no";
			if ( iter->m_use_reverse ) { use_reverse = "yes"; }

			std::string feature_tag = 
				">" + type_code + 
				"    # name: " + iter->m_feature->GetName() +
				"    # polygon: " + GetName() +
				"    # use_reverse: " + use_reverse +
				"\n";
			// Get the vertices from this segment
			std::list<GPlatesMaths::PointOnSphere> tmp_vertex_list;
			tmp_vertex_list.clear();

			copy(
				iter_vertex_list.begin(),
				iter_vertex_list.end(),
				back_inserter( tmp_vertex_list )
			);

// std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 6: tag=" << feature_tag;
// std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 6: r_t_list.push_back: "
// << "tmp_vertex_list.size()=" << tmp_vertex_list.size() << "; "
// << std::endl;
			// Append this boundary to the list
			m_ridge_transform_list.push_back( 
				SubductionBoundaryFeature(
					GPlatesUtils::get_old_id( iter->m_feature),
					feature_tag,
					tmp_vertex_list)
			);
		}

#endif // TYPE

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 6 copy: "
<< "iter_vertex_list.size()=" << iter_vertex_list.size() << "; "
<< std::endl;
#endif

	} // End of interation over boundary feature list

	//
	// Step 7: adjust member copy of working vertex list
	//
	m_vertex_list = work_vertex_list;

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l step 7: "
<< "work_vertex_list.size()=" << work_vertex_list.size() << "; "
<< std::endl;
#endif

	// report();

	return work_vertex_list;
}





//
// get_vertex_list_from_node_relation
//
void
GPlatesFeatureVisitors::PlatepolygonResolver::get_vertex_list_from_node_relation( 
	GPlatesFeatureVisitors::PlatepolygonResolver::NeighborRelation relation,
	BoundaryFeature &node1,
	BoundaryFeature &node2,
	std::list<GPlatesMaths::PointOnSphere> &vertex_list)
{

	// double check on empty vertex_list
	if ( vertex_list.empty() )
	{
		// no change to node1 or vertex_list
		return;
	}

	std::string node1_fid = node1.m_feature_id;
	std::string node2_fid = node2.m_feature_id;

#if 0
// FIX : remove this diagnostic output
std::cout << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l_f_n_r: " 
<< "relation=" << relation << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l_f_n_r: "
<< "node1_fid=" << node1_fid << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l_f_n_r: "
<< "node2_fid=" << node2_fid << std::endl;
std::cout << std::endl;
#endif

	// short cut for node2 is a POINT_FEATURE
	if ( node2.m_feature_type == GPlatesGlobal::POINT_FEATURE)
	{
		// no change to node1 or vertex_list
		return;
	}

	//
	// node2 must be a GPlatesGlobal::LINE_FEATURE , so test for intersection
	//
	
	//
	// tmp variables to hold results of processing node1 vs node2
	//
	std::list<GPlatesMaths::PointOnSphere> node1_vertex_list;
	std::list<GPlatesMaths::PointOnSphere> node2_vertex_list;

	//
	// short cut for empty node2
	//

	// get verts for node2 from Layout 
	d_recon_finder_ptr->get_vertex_list_from_feature_id(
		node2_vertex_list,
		node2_fid );

	//
	// skip features not found, or missing from Layout
	//
	if ( node2_vertex_list.empty() )
	{
		// no change to node1 or vertex_list
		return;
	}

	//
	// create polylines for each boundary feature node
	//

	// use the argument vertex_list
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type node1_polyline = 
		GPlatesMaths::PolylineOnSphere::create_on_heap( vertex_list );

#if 0 // REMOVE
	const GPlatesMaths::PolylineOnSphere& pl1 = *node1_polyline;
	GPlatesMaths::PolylineOnSphere::VertexConstIterator itr1 = pl1.vertex_begin();
	GPlatesMaths::PolylineOnSphere::VertexConstIterator end1 = pl1.vertex_end();
	for ( ; itr1 != end1 ; ++itr1)
	{
		std::cout << "TEST: llp( *itr1 ) = " << GPlatesMaths::make_lat_lon_point(*itr1) << std::endl;
	}
std::cout << std::endl;
#endif
	

	// use the feature's vertex list
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type node2_polyline = 
		GPlatesMaths::PolylineOnSphere::create_on_heap( node2_vertex_list );

#if 0 // REMOVE
	const GPlatesMaths::PolylineOnSphere& pl2 = *node2_polyline;
	GPlatesMaths::PolylineOnSphere::VertexConstIterator itr2 = pl2.vertex_begin();
	GPlatesMaths::PolylineOnSphere::VertexConstIterator end2 = pl2.vertex_end();

	for ( ; itr2 != end2 ; ++itr2)
	{
		std::cout << "TEST: llp( *itr2 ) = " << GPlatesMaths::make_lat_lon_point(*itr2) << std::endl;
	}
std::cout << std::endl;
#endif
	

	//
	// variables to save results of intersection tests
	//
	std::list< GPlatesMaths::PointOnSphere > intersection_points;
	std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type > partitioned_lines;

	//
	// test for intersection between node1 and node2
	//
	int num_intersect = 0;

	num_intersect = GPlatesMaths::PolylineIntersections::partition_intersecting_polylines(
		*node1_polyline,
		*node2_polyline,
		intersection_points,
		partitioned_lines);

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l_f_n_r: "
<< "num_intersect = " << num_intersect << std::endl;
#endif

	//
	// switch on relation enum to update node1's member data
	//
	switch ( relation )
	{
		case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV:
			node1.m_num_intersections_with_prev = num_intersect;
			break;

		case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT:
			node1.m_num_intersections_with_next = num_intersect;
			break;

		case GPlatesFeatureVisitors::PlatepolygonResolver::NONE :
		case GPlatesFeatureVisitors::PlatepolygonResolver::OTHER :
		default :
			// somthing bad happened , freak out
			break;
	}


	// 

	if ( num_intersect == 0 )
	{
		// no further change to node1 or vertex_list
		return;
	}
	else if ( num_intersect == 1 )
	{
		// pair of polyline lists from intersection
		std::pair<
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type >,
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type >
		> parts;

		// unambiguously identify partitioned lines:
		//
		// parts.first.front() is a ref to the head of node1_polyline
		// parts.first.back() is a ref to the tail of node1_polyline
		// parts.second.front() is a ref to the head of node2_polyline
		// parts.second.back() is a ref to the tail of node2_polyline
		//
		parts = GPlatesMaths::PolylineIntersections::identify_partitioned_polylines(
			*node1_polyline,
			*node2_polyline,
			intersection_points,
			partitioned_lines);


		// now check which element of parts.first
		// to use based upon node1's neighbor relations
		//
		// parts.first holds the sub-parts of node1's polyline

		//
		// switch on the relation to check
		//
		switch ( relation )
		{
			case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_PREV :

				if ( node1.m_use_head_from_intersect_prev )
				{
					vertex_list.clear();
					copy( 
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( vertex_list )
					);
					return;
				}
				
				if ( node1.m_use_tail_from_intersect_prev )
				{
					vertex_list.clear();
					copy( 
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( vertex_list )
					);
					return;
				}

				break;

			case GPlatesFeatureVisitors::PlatepolygonResolver::INTERSECT_NEXT:

				if ( node1.m_use_head_from_intersect_next )
				{
					vertex_list.clear();
					copy( 
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( vertex_list )
					);
					return;
				}
				
				if ( node1.m_use_tail_from_intersect_next )
				{
					vertex_list.clear();
					copy( 
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( vertex_list )
					);
					return;
				}

				break;

			default:
				break;
		}
	} 
	// end of else if ( num_intersect == 1 )
	else 
	{
		// num_intersect must be 2 or greater
		// oh no!
		// check for overlap ...
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::g_v_l_f_n_r: "
<< "WARN: num_intersect=" << num_intersect
<< std::endl;
#endif

	}

}


void
GPlatesFeatureVisitors::PlatepolygonResolver::compute_bounds()
{
	// tmp vars
	double dlon = 0;
	double lon_sum = 0.0;

	// re-set initial default values to opposite extreame value
	m_max_lat = -91;
	m_min_lat = 91;
	m_max_lon = -181;
	m_min_lon = 181;

	// re-set polar value to default : 
	// 0 = no pole contained in polygon
	m_pole = 0;

	// loop over rotated vertices
	std::list<GPlatesMaths::PointOnSphere>::const_iterator iter;
	std::list<GPlatesMaths::PointOnSphere>::const_iterator next;

	for ( iter = VertexBegin(); iter != VertexEnd(); ++iter )
	{
		// get coords for iter vertex
		GPlatesMaths::LatLonPoint v1 = GPlatesMaths::make_lat_lon_point(*iter);

		// coords for next vertex in list
		next = iter;
		++next;
		if ( next == VertexEnd() ) { next = VertexBegin(); }

		// coords for next vextex
		GPlatesMaths::LatLonPoint v2 = GPlatesMaths::make_lat_lon_point(*next);

		double v1lat = v1.latitude();
		double v1lon = v1.longitude();

		// double v2lat = v2.latitude(); // not used
		double v2lon = v2.longitude();

		m_min_lon = std::min( v1lon, m_min_lon );
		m_max_lon = std::max( v1lon, m_max_lon );

		m_min_lat = std::min( v1lat, m_min_lat );
		m_max_lat = std::max( v1lat, m_max_lat );

		dlon = v1lon - v2lon;

		if (fabs (dlon) > 180.0) 
		{
			dlon = copysign (360.0 - fabs (dlon), -dlon);
		}

		lon_sum += dlon;

	}

#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::compute_bounds: " 
<< "max_lat = " << m_max_lat << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::compute_bounds: " 
<< "min_lat = " << m_min_lat << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::compute_bounds: " 
<< "max_lon = " << m_max_lon << std::endl;
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::compute_bounds: " 
<< "min_lon = " << m_min_lon << std::endl;

std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::compute_bounds: " 
<< "lon_sum = " << lon_sum << std::endl;
#endif

	//
	// dermine if the platepolygon contains the pole
	//
	if ( fabs(fabs(lon_sum) - 360.0) < 1.0e-8 )
	{
		if ( fabs(m_max_lat) > fabs(m_min_lat) ) 
		{
			m_pole = static_cast<int>(copysign(1.0, m_max_lat));
		}
		else 
		{
			m_pole = static_cast<int>(copysign(1.0, m_min_lat));
		}
	}
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::compute_bounds: " 
<< "m_pole = " << m_pole << std::endl;
#endif

}



/* This function is used to see if some point p is located 
 * inside, outside, or on the boundary of the plate polygon
 * Returns the following values:
 *	0:	p is outside of S
 *	1:	p is inside of S
 *	2:	p is on boundary of S
 */
int 
GPlatesFeatureVisitors::PlatepolygonResolver::is_point_in_on_out ( 
	const GPlatesMaths::PointOnSphere &test_point ) const
{
	/* Algorithm:
	 *
	 * Case 1: The polygon S contains a geographical pole
	 *	   a) if P is beyond the far latitude then P is outside
	 *	   b) Compute meridian through P and count intersections:
	 *		odd: P is outside; even: P is inside
	 *
	 * Case 2: S does not contain a pole
	 *	   a) If P is outside range of latitudes then P is outside
	 *	   c) Compute meridian through P and count intersections:
	 *		odd: P is inside; even: P is outside
	 *
	 * In all cases, we check if P is on the outline of S
	 *
	 */
	
	// counters for the number of crossings of a meridian 
	// through p and and the segments of this polygon
	int count_north = 0;
	int count_south = 0;


	// coords of test point p
	GPlatesMaths::LatLonPoint p = GPlatesMaths::make_lat_lon_point(test_point);

	// test point's plat
	double plat = p.latitude();
		
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::is_point_in_on_out: " << std::endl;
std::cout << "llp" << p << std::endl;
std::cout << "pos" << test_point << std::endl;
#endif

	if (m_pole) 
	{	
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::is_point_in_on_out: pole" << std::endl;
#endif
		/* Case 1 of an enclosed polar cap */

		/* N polar cap */
		if (m_pole == 1) 
		{	
			/* South of a N polar cap */
			if (plat < m_min_lat) return (POINT_OUTSIDE_POLYGON);	

			/* Clearly inside of a N polar cap */
			if (plat > m_max_lat) return (POINT_INSIDE_POLYGON);	
		}

		/* S polar cap */
		if (m_pole == -1) 
		{	
			/* North of a S polar cap */
			if (plat > m_max_lat) return (POINT_OUTSIDE_POLYGON);

			/* North of a S polar cap */
			if (plat < m_min_lat) return (POINT_INSIDE_POLYGON);	
		}
	
		// Tally up number of intersections between polygon 
		// and meridian through p 
		
		if ( is_point_in_on_out_counter(test_point, count_north, count_south) ) 
		{
			/* Found P is on S */
			return (POINT_ON_POLYGON);	
		}
	
		if (m_pole == 1 && count_north % 2 == 0) 
		{
			return (POINT_INSIDE_POLYGON);
		}

		if (m_pole == -1 && count_south % 2 == 0) 
		{
			return (POINT_INSIDE_POLYGON);
		}
	
		return (POINT_OUTSIDE_POLYGON);
	}
	
	/* Here is Case 2.  */

	// First check latitude range 
	if (plat < m_min_lat || plat > m_max_lat) 
	{
		return (POINT_OUTSIDE_POLYGON);
	}
	
	// Longitudes are tricker and are tested with the 
	// tallying of intersections 
	
	if ( is_point_in_on_out_counter( test_point, count_north, count_south ) ) 
	{
		/* Found P is on S */
		return (POINT_ON_POLYGON);	
	}

	if (count_north % 2) return (POINT_INSIDE_POLYGON);
	
	/* Nothing triggered the tests; we are outside */
	return (POINT_OUTSIDE_POLYGON);	
}


int 
GPlatesFeatureVisitors::PlatepolygonResolver::is_point_in_on_out_counter ( 
	const GPlatesMaths::PointOnSphere &test_point,
	int &count_north,
	int &count_south) 
const
{
#if 0
std::cout << "GPlatesFeatureVisitors::PlatepolygonResolver::is_point_in_on_out_counter: " << std::endl;
#endif

	// local tmp vars
	// using GPlatesMaths::Real lets us use operator== 
	GPlatesMaths::Real W, E, S, N, x_lat;
	GPlatesMaths::Real dlon;
	GPlatesMaths::Real lon, lon1, lon2;

	// coords of test test_point p
	GPlatesMaths::LatLonPoint p = GPlatesMaths::make_lat_lon_point(test_point);

	// test_point's coords are plon, plat
	double plon = p.longitude();
	double plat = p.latitude();

	// re-set number of crossings
	count_south = 0;
	count_north = 0;

	// Compute meridian through P and count all the crossings with 
	// segments of polygon boundary 

	// loop over plate polygon vertices
	// form segments for each vertex pair
	std::list<GPlatesMaths::PointOnSphere>::const_iterator iter;
	std::list<GPlatesMaths::PointOnSphere>::const_iterator next;

	// loop over rotated vertices using access iterators
	for ( iter = VertexBegin(); iter != VertexEnd(); ++iter )
	{
		// get coords for iter vertex
		GPlatesMaths::LatLonPoint v1 = GPlatesMaths::make_lat_lon_point(*iter);

		// identifiy next vertex 
		next = iter; 
		++next;
		if ( next == VertexEnd() ) { next = VertexBegin(); }

		// coords for next vextex
		GPlatesMaths::LatLonPoint v2 = GPlatesMaths::make_lat_lon_point(*next);

		double v1lat = v1.latitude();
		double v1lon = v1.longitude();

		double v2lat = v2.latitude();
		double v2lon = v2.longitude();

#if 0
std::cout << "is_point_in_on_out_counter: v1 = " 
<< v1lat << ", " << v1lon << std::endl;
std::cout << "is_point_in_on_out_counter: v2 = " 
<< v2lat << ", " << v2lon << std::endl; 
#endif
		// Copy the two vertex longitudes 
		// since we need to mess with them
		lon1 = v1lon;
		lon2 = v2lon;

		// delta in lon
		dlon = lon2 - lon1;

		if ( dlon > 180.0 )
		{
			// Jumped across Greenwhich going westward
			lon2 -= 360.0;
		} 
		else if ( dlon < -180.0 )
		{
			// Jumped across Greenwhich going eastward
			lon1 -= 360.0;
		}

		// set lon limits for this segment
		if (lon1 <= lon2) 
		{	
			/* segment goes W to E (or N-S) */
			W = lon1;
			E = lon2;
		}
		else 
		{			
			/* segment goes E to W */
			W = lon2;
			E = lon1;
		}
		
		/* Local copy of plon, adjusted given the segment lon range */
		lon = plon;	

		/* Make sure we rewind way west for starters */
		while (lon > W) lon -= 360.0;	

		/* Then make sure we wind to inside the lon range or way east */
		while (lon < W) lon += 360.0;	

		/* Not crossing this segment */
		if (lon > E) continue; // to next vertex

		/* Special case of N-S segment: does P lie on it? */
		if (dlon == 0.0) 
		{	
			if ( v2lat < v1lat ) 
			{	
				/* Get N and S limits for segment */
				S = v2lat;
				N = v1lat;
			}
			else 
			{
				N = v2lat;
				S = v1lat;
			}

			/* P is not on this segment */
			if ( plat < S || plat > N ) continue; // to next vertex

			/* P is on segment boundary; we are done */
			return (1);	
		}

		/* Calculate latitude at intersection */
		x_lat = v1lat + (( v2lat - v1lat) / (lon2 - lon1)) * (lon - lon1);

		/* P is on S boundary */
		if ( x_lat == plat ) return (1);	

		// Only allow cutting a vertex at end of a segment 
		// to avoid duplicates 
		if (lon == lon1) continue;	

		if (x_lat > plat)	
		{
			/* Cut is north of P */
			++count_north;
		}
		else
		{
			/* Cut is south of P */
			++count_south;
		}

	} // end of loop over vertices

	return (0);	
}

//
// END
//

