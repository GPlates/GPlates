
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-13 09:30:52 -0700 (Wed, 13 Aug 2008) $
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

#include <map>
#include <list>
#include <utility>
#include <QLocale>
#include <QDebug>
#include <QList>
#include <boost/none.hpp>

#include "ReconstructedFeatureGeometryFinder.h"

#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"
#include "model/Reconstruction.h"
#include "model/ReconstructedFeatureGeometry.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"

#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/FeatureHandleToOldId.h"

namespace
{
	/**
	 * Enum used to select coordinate columns.
	 */
	namespace CoordinatePeriods
	{
		enum CoordinatePeriod { PRESENT = 1, RECONSTRUCTED = 2 };
	}
	
	/**
	 * Iterates over the vertices of the polyline, setting the coordinates in the
	 * column of each QTreeWidget corresponding to 'period'.
	 */
	void
	populate_coordinates_from_polyline(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline,
			CoordinatePeriods::CoordinatePeriod period)
	{
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline->vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline->vertex_end();
		
		for ( ; iter != end; ++iter) 
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
		}
	}

	/**
	 * Iterates over the vertices of the point, setting the coordinates in the
	 * column of each QTreeWidget corresponding to 'period'.
	 */
	void
	populate_coordinates_from_point(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
			CoordinatePeriods::CoordinatePeriod period)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*point_on_sphere);
	}
}



void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_num_features += 1;

	// Iterate over the Reconstruction and grab the reconstructed geometry that originates
	// from the given feature.
	populate_rfg_geometries_for_feature(feature_handle);

	// Now visit each of the properties in turn, populating d_property_info_vector
	visit_feature_properties(feature_handle);

	// Now add any geometric properties we were interested in (and delete the others)
	property_info_vector_const_iterator it = d_property_info_vector.begin();
	property_info_vector_const_iterator end = d_property_info_vector.end();
	for ( ; it != end; ++it) {
			if (it->is_geometric_property) {
				// This is a geometric property and we want to add it to the widget.
			}
	}

#if 0
// FIXME: save this code for use on how to populate individual maps
	// Iterate over the Reconstruction and grab 
	// the reconstructed geometry that originates from the given feature.
	populate_feature_maps(feature_handle);
#endif
}


// FIXME: I was going to implement this d_last_property_visited optional in {Const,}FeatureVisitor
// directly, since there's several visitors that currently rely on a similar member and override this
// visit_feature_properties function.
// However, I ran into problems with the forward-declaration of FeatureHandle. I am not high level
// enough to make those changes just yet.
void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::visit_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	GPlatesModel::FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			d_last_property_visited = iter;
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::visit_inline_property_container(
		GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	// Create a top-level item for this property and remember it - do not add it just yet.
	PropertyInfo info;
	info.is_geometric_property = false;
	d_property_info_vector.push_back(info);

	visit_property_values(inline_property_container);
}




void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;
	
#if 0
	// The present-day polyline.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type present_day_polyline = gml_line_string.polyline();
	populate_coordinates_from_polyline(present_day_polyline, CoordinatePeriods::PRESENT);
#endif

	// The reconstructed polyline, which may not be available. And test d_last_property_visited,
	// because someone might attempt to call us without invoking visit_feature_handle.
	if (d_last_property_visited) 
	{
		boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> recon_geometry =
				get_reconstructed_geometry_for_property(*d_last_property_visited);

		if (recon_geometry) 
		{
			// We use a dynamic cast here (despite the fact that dynamic casts are
			// generally considered bad form) because we only care about one specific
			// derivation.  There's no "if ... else if ..." chain, so I think it's not
			// super-bad form.  (The "if ... else if ..." chain would imply that we
			// should be using polymorphism -- specifically, the double-dispatch of the
			// Visitor pattern -- rather than updating the "if ... else if ..." chain
			// each time a new derivation is added.)
			const GPlatesMaths::PolylineOnSphere *recon_polyline =
					dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(recon_geometry->get());

			if (recon_polyline) 
			{
				// FROM EditFeatureGeometriesWidgetPopulator
				populate_coordinates_from_polyline(
						GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type(
								recon_polyline,
								GPlatesUtils::NullIntrusivePointerHandler()),
						CoordinatePeriods::RECONSTRUCTED);

			}
		}
	}
}


void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;

	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	// Mark that the property tree widget item we are in the middle of constructing is a
	// geometry-valued property so we can remember to add it to the QTreeWidget later.
	d_property_info_vector.back().is_geometric_property = true;

#if 0
	// The present-day point.
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type present_day_point = gml_point.point();
	populate_coordinates_from_point(present_day_point, CoordinatePeriods::PRESENT);
#endif

	// The reconstructed point, which may not be available. And test d_last_property_visited,
	// because someone might attempt to call us without invoking visit_feature_handle.
	if (d_last_property_visited) 
	{
		boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> recon_geometry =
				get_reconstructed_geometry_for_property(*d_last_property_visited);

		if (recon_geometry) 
		{
			// We use a dynamic cast here (despite the fact that dynamic casts are
			// generally considered bad form) because we only care about one specific
			// derivation.  There's no "if ... else if ..." chain, so I think it's not
			// super-bad form.  (The "if ... else if ..." chain would imply that we
			// should be using polymorphism -- specifically, the double-dispatch of the
			// Visitor pattern -- rather than updating the "if ... else if ..." chain
			// each time a new derivation is added.)
			const GPlatesMaths::PointOnSphere *recon_point =
					dynamic_cast<const GPlatesMaths::PointOnSphere *>(recon_geometry->get());
			if (recon_point) 
			{
				populate_coordinates_from_point(
						GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type(
								recon_point,
								GPlatesUtils::NullIntrusivePointerHandler()),
						CoordinatePeriods::RECONSTRUCTED);
			}
		}
	}
}



void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::populate_rfg_geometries_for_feature(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	GPlatesModel::Reconstruction::geometry_collection_type::const_iterator it =
			d_reconstruction_ptr->geometries().begin();

	GPlatesModel::Reconstruction::geometry_collection_type::const_iterator end =
			d_reconstruction_ptr->geometries().end();

	for ( ; it != end; ++it) 
	{
		GPlatesModel::ReconstructionGeometry *rg = it->get();
		// We use a dynamic cast here (despite the fact that dynamic casts are
		// generally considered bad form) because we only care about one specific
		// derivation.  There's no "if ... else if ..." chain, so I think it's not
		// super-bad form.  (The "if ... else if ..." chain would imply that we
		// should be using polymorphism -- specifically, the double-dispatch of the
		// Visitor pattern -- rather than updating the "if ... else if ..." chain
		// each time a new derivation is added.)
		GPlatesModel::ReconstructedFeatureGeometry *rfg =
				dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);

		if ( ! rfg) {
			continue;
		}

		if (rfg->feature_ref().references(feature_handle)) 
		{
			ReconstructedGeometryInfo info(rfg->property(), rfg->geometry());
			d_rfg_geometries.push_back(info);

			// Get the feature ID 
			std::string old_id = GPlatesUtils::get_old_id( feature_handle );

			// fill the RFG map
			d_old_id_to_geometry_map.insert( 
				std::make_pair(
					old_id, 
					rfg->geometry() ) );

			// fill the RFG map
			d_feature_id_to_geometry_map.insert( 
				std::make_pair(
					feature_handle.feature_id(), 
					rfg->geometry() ) );

#if 0
			// fill the feature_id to handle map
			GPlatesModel::FeatureHandle::weak_ref ref = feature_handle.reference();

			d_feature_id_to_feature_handle_map.insert(
				std::make_pair(
					feature_handle.feature_id(), 
					ref ) );
#endif

		}
	}
}


boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::get_reconstructed_geometry_for_property(
		const GPlatesModel::FeatureHandle::properties_iterator property)
{
	geometries_for_property_const_iterator it = d_rfg_geometries.begin();
	geometries_for_property_const_iterator end = d_rfg_geometries.end();

	for ( ; it != end; ++it) 
	{
		if (it->d_property == property) 
		{
			return it->d_geometry;
		}
	}
	return boost::none;
}


void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::report()
{
	std::cout << "--------------------------------------------" << std::endl;
	std::cout << "ReconstructedFeatureGeometryFinder::report()" << std::endl;
	std::cout << "number features visited = " << d_num_features << std::endl;
	std::cout << "- old_id_to_geometry_map:" << std::endl;
	old_id_to_geometry_map_type::iterator g_iter = d_old_id_to_geometry_map.begin();
	old_id_to_geometry_map_type::iterator g_end = d_old_id_to_geometry_map.end();
	for ( ; g_iter != g_end ; ++g_iter )
	{
		std::list<GPlatesMaths::PointOnSphere> list;
		get_vertex_list_from_old_id( list, g_iter->first );
		std::cout 
			<< "old_id= " << g_iter->first 
			<< "; size=" << list.size()
			<< std::endl;
	} 

	std::cout << "- feature_id_to_geometry_map:" << std::endl;
	feature_id_to_geometry_map_type::iterator f_iter = d_feature_id_to_geometry_map.begin();
	feature_id_to_geometry_map_type::iterator f_end = d_feature_id_to_geometry_map.end();
	for ( ; f_iter != f_end ; ++f_iter )
	{
		std::list<GPlatesMaths::PointOnSphere> list;
		get_vertex_list_from_feature_id( list, f_iter->first );

		qDebug() 
			<< "fid=" << GPlatesUtils::make_qstring_from_icu_string( f_iter->first.get() )
			<< "; size=" << list.size();
	} 

#if 0
	std::cout << "- feature_id_to_feature_handle_map:" << std::endl;
	feature_id_to_feature_handle_map_type::iterator h_iter = d_feature_id_to_feature_handle_map.begin();
	feature_id_to_feature_handle_map_type::iterator h_end = d_feature_id_to_feature_handle_map.end();
	for ( ; h_iter != h_end ; ++h_iter )
	{
		qDebug() 
		<< " fid=" << GPlatesUtils::make_qstring_from_icu_string( (h_iter->first).get() )
		<< " ; type=" << GPlatesUtils::make_qstring_from_icu_string( (h_iter->second)->feature_type().get_name()  );
	} 
#endif

}


// access the geometry map
boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::get_geometry_from_old_id(
	std::string id )
{
	// find the geom from the RFG map
	old_id_to_geometry_map_type::const_iterator pos = d_old_id_to_geometry_map.find( id );

	// if found, return it
	if ( pos != d_old_id_to_geometry_map.end() )
	{
		return pos->second;
	}
	else 
	{
		return boost::none;
	}
}


// get a vertex list for the feature
void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::get_vertex_list_from_old_id( 
	std::list<GPlatesMaths::PointOnSphere> &vertex_list,
	std::string id )
{
	// find the geom from the RFG map
	old_id_to_geometry_map_type::const_iterator iter = 
		d_old_id_to_geometry_map.find( id );

	// if it is found, then visit it
	if ( iter != d_old_id_to_geometry_map.end() )
	{
		iter->second->accept_visitor( *this );

		// Copy the vertices from the d_vertex_list to the argument list
		copy(
			d_vertex_list.begin(),
			d_vertex_list.end(),
			back_inserter( vertex_list )
		);
	}
	// else, no change to argument list
}


// get a vertex list for the feature
void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::get_vertex_list_from_feature_id( 
	std::list<GPlatesMaths::PointOnSphere> &vertex_list,
	GPlatesModel::FeatureId id)
{
	// find the geom from the RFG map
	feature_id_to_geometry_map_type::const_iterator iter = 
		d_feature_id_to_geometry_map.find( id );

	// if it is found, then visit it
	if ( iter != d_feature_id_to_geometry_map.end() )
	{
		iter->second->accept_visitor( *this );

		// Copy the vertices from the d_vertex_list to the argument list
		copy(
			d_vertex_list.begin(),
			d_vertex_list.end(),
			back_inserter( vertex_list )
		);
	}
	// else, no change to argument list
}


#if 0
// get a feature handle  for the feature id
void
GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder::get_feature_handle_from_feature_id( 
	GPlatesModel::FeatureHandle &handle,
	GPlatesModel::FeatureId id)
{
	// find the handle map
	feature_id_to_feature_handle_map_type::const_iterator iter = 
		d_feature_id_to_feature_handle_map.find( id );

	// if it is found, then return it 
	if ( iter != d_feature_id_to_feature_handle_map.end() )
	{
		handle = (iter->second).clone();
	}
	// else, no change to argument 
}
#endif


