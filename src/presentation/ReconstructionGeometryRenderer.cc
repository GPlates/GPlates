/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <cstddef> // For std::size_t


#include "global/CompilerWarnings.h"

#include "ReconstructionGeometryRenderer.h"

#include "RasterVisualLayerParams.h"
#include "ReconstructVisualLayerParams.h"
#include "ScalarField3DVisualLayerParams.h"
#include "TopologyBoundaryVisualLayerParams.h"
#include "TopologyNetworkVisualLayerParams.h"
#include "VelocityFieldCalculatorVisualLayerParams.h"
#include "ViewState.h"
#include "VisualLayerParamsVisitor.h"


#include "app-logic/ApplicationState.h"
#include "app-logic/CoRegistrationData.h"
#include "app-logic/PropertyExtractors.h"
#include "app-logic/MultiPointVectorField.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructedSmallCircle.h"
#include "app-logic/ReconstructedVirtualGeomagneticPole.h"
#include "app-logic/ResolvedRaster.h"
#include "app-logic/ResolvedScalarField3D.h"
#include "app-logic/ResolvedTopologicalGeometry.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/PlateVelocityUtils.h"

#include "data-mining/DataTable.h"

#include "global/GPlatesAssert.h"

#include "gui/Colour.h"
#include "gui/DrawStyleManager.h"
#include "gui/PlateIdColourPalettes.h"

#include "maths/CalculateVelocity.h"
#include "maths/MathsUtils.h"

#include "utils/Profile.h"
#include "utils/ComponentManager.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"

#include <boost/foreach.hpp>

namespace
{
	/**
	 * Returns a GPlatesGui::Symbol for the feature type of the @a reconstruction_geometry, if
	 * an appropriate entry in the @a feature_type_symbol_map exists.
	 */
	boost::optional<GPlatesGui::Symbol>
	get_symbol(
	    const boost::optional<GPlatesGui::symbol_map_type> &symbol_map,
	    const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry)
	{
	    if (symbol_map)
	    {
		GPlatesAppLogic::FeatureTypePropertyExtractor extractor;
		const boost::optional<GPlatesAppLogic::FeatureTypePropertyExtractor::return_type> feature_type =
			    extractor(*reconstruction_geometry);

		if (feature_type)
		{
		    GPlatesGui::symbol_map_type::const_iterator iter = symbol_map->find(*feature_type);

		    if (iter != symbol_map->end())
		    {
			return iter->second;
		    }
		}
	    }
	    return boost::none;
	}


	/**
	 * Creates a @a RenderedGeometry from @a geometry and wraps it in another @a RenderedGeometry
	 * that references @a reconstruction_geometry.
	 */
	GPlatesViewOperations::RenderedGeometry
	create_rendered_reconstruction_geometry(
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry,
			const GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams &render_params,
			const boost::optional<GPlatesGui::Colour> &colour,
			const boost::optional<GPlatesMaths::Rotation> &rotation = boost::none,
			const boost::optional<GPlatesGui::symbol_map_type> &feature_type_symbol_map = boost::none,
			const GPlatesGui::DrawStyle style = GPlatesGui::DrawStyle())
	{

		boost::optional<GPlatesGui::Symbol> symbol = get_symbol(
				feature_type_symbol_map, reconstruction_geometry);
		//symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, 10, true);

		// Create a RenderedGeometry for drawing the reconstructed geometry.
		// Draw it in the specified colour (if specified) otherwise defer colouring to a later time
		// using ColourProxy.
		GPlatesViewOperations::RenderedGeometry rendered_geom;
		if(GPlatesUtils::ComponentManager::instance().is_enabled(GPlatesUtils::ComponentManager::Component::python()))
		{
			rendered_geom =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					rotation ? rotation.get() * geometry : geometry,
					colour ? colour : style.colour,
					render_params.reconstruction_point_size_hint,
					render_params.reconstruction_line_width_hint,
					render_params.fill_polygons,
					symbol);
		}
		else
		{
			rendered_geom =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					rotation ? rotation.get() * geometry : geometry,
					colour ? colour.get() : GPlatesGui::ColourProxy(reconstruction_geometry),
					render_params.reconstruction_point_size_hint,
					render_params.reconstruction_line_width_hint,
					render_params.fill_polygons,
					symbol);
		}
		

		// Create a RenderedGeometry for storing the ReconstructionGeometry and
		// a RenderedGeometry associated with it.
		return GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
				reconstruction_geometry,
				rendered_geom);
	}
}



GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams::RenderParams(
		float reconstruction_line_width_hint_,
		float reconstruction_point_size_hint_,
		bool fill_polygons_,
		float velocity_ratio_unit_vector_direction_to_globe_radius_,
		bool show_topological_network_delaunay_triangulation_,
		bool show_topological_network_constrained_triangulation_,
		bool show_topological_network_mesh_triangulation_,
		bool show_topological_network_segment_velocity_,
		bool show_velocity_field_delaunay_vectors_,
		bool show_velocity_field_constrained_vectors_) :
	reconstruction_line_width_hint(reconstruction_line_width_hint_),
	reconstruction_point_size_hint(reconstruction_point_size_hint_),
	fill_polygons(fill_polygons_),
	velocity_ratio_unit_vector_direction_to_globe_radius(
			velocity_ratio_unit_vector_direction_to_globe_radius_),
	raster_colour_palette(GPlatesGui::RasterColourPalette::create()),
	vgp_draw_circular_error(true),
	show_topological_network_mesh_triangulation( show_topological_network_mesh_triangulation_),
	show_topological_network_delaunay_triangulation( show_topological_network_delaunay_triangulation_),
	show_topological_network_constrained_triangulation( show_topological_network_constrained_triangulation_),
	show_topological_network_segment_velocity( show_topological_network_segment_velocity_),
	show_velocity_field_delaunay_vectors( show_velocity_field_delaunay_vectors_),
	show_velocity_field_constrained_vectors( show_velocity_field_constrained_vectors_)
{
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_raster_visual_layer_params(
		const RasterVisualLayerParams &params)
{
	d_render_params.raster_colour_palette = params.get_colour_palette();
	d_render_params.raster_modulate_colour = params.get_modulate_colour();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_reconstruct_visual_layer_params(
		const ReconstructVisualLayerParams &params)
{
	d_render_params.vgp_draw_circular_error = params.get_vgp_draw_circular_error();
	d_render_params.fill_polygons = params.get_fill_polygons();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_scalar_field_3d_visual_layer_params(
		const ScalarField3DVisualLayerParams &params)
{
	const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &isovalue_parameters =
			params.get_isovalue_parameters()
			? params.get_isovalue_parameters().get()
			: GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters();

	const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction =
			params.get_depth_restriction()
			? params.get_depth_restriction().get()
			: GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction();

	d_render_params.scalar_field_render_parameters =
			GPlatesViewOperations::ScalarField3DRenderParameters(
					params.get_render_mode(),
					params.get_colour_mode(),
					params.get_colour_palette(),
					isovalue_parameters,
					params.get_deviation_window_render_options(),
					params.get_surface_polygons_mask(),
					depth_restriction,
					params.get_quality_performance(),
					params.get_shader_test_variables());
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_topology_boundary_visual_layer_params(
		topology_boundary_visual_layer_params_type &params)
{
	d_render_params.fill_polygons = params.get_fill_polygons();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_topology_network_visual_layer_params(
		const TopologyNetworkVisualLayerParams &params)
{
	d_render_params.show_topological_network_delaunay_triangulation = params.show_delaunay_triangulation();
	d_render_params.show_topological_network_constrained_triangulation = params.show_constrained_triangulation();
	d_render_params.show_topological_network_mesh_triangulation = params.show_mesh_triangulation(); 
	d_render_params.show_topological_network_segment_velocity = params.show_segment_velocity();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_velocity_field_calculator_visual_layer_params(
		const VelocityFieldCalculatorVisualLayerParams &params)
{
	d_render_params.show_velocity_field_delaunay_vectors = params.show_delaunay_vectors();
	d_render_params.show_velocity_field_constrained_vectors = params.show_constrained_vectors();
}


GPlatesPresentation::ReconstructionGeometryRenderer::ReconstructionGeometryRenderer(
		const RenderParams &render_params,
		const boost::optional<GPlatesGui::Colour> &colour,
		const boost::optional<GPlatesMaths::Rotation> &reconstruction_adjustment,
		const boost::optional<GPlatesGui::symbol_map_type> &feature_type_symbol_map,
		const GPlatesGui::StyleAdapter* sa) :
	d_render_params(render_params),
	d_colour(colour),
	d_reconstruction_adjustment(reconstruction_adjustment),
	d_feature_type_symbol_map(feature_type_symbol_map),
	d_style_adapter(sa)
{
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::begin_render()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	// Create a new rendered geometries spatial partition.
	d_rendered_geometries_spatial_partition =
			rendered_geometries_spatial_partition_type::create(DEFAULT_SPATIAL_PARTITION_DEPTH);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::end_render(
		GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	// Now transfer ownership of the rendered geometries spatial partition to
	// the rendered geometry layer.
	//
	// NOTE: It's important to add rendered geometries as a spatial partition because it gives
	// spatial locality to each geometry which makes some types of rendering faster such as
	// filled polygons (which are actually rendered as multi-resolution cube rasters).
	rendered_geometry_layer.add_rendered_geometries(d_rendered_geometries_spatial_partition.get());

	// Release our shared reference to the spatial partition.
	d_rendered_geometries_spatial_partition = boost::none;
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter = mpvf->multi_point()->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator domain_end = mpvf->multi_point()->end();

	GPlatesAppLogic::MultiPointVectorField::codomain_type::const_iterator codomain_iter = mpvf->begin();

	for ( ; domain_iter != domain_end; ++domain_iter, ++codomain_iter)
	{
		if ( ! *codomain_iter) {
			// There is no codomain element at this position.
			continue;
		}
		const GPlatesMaths::PointOnSphere &point = *domain_iter;
		const GPlatesAppLogic::MultiPointVectorField::CodomainElement &velocity = **codomain_iter;

		if (velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::InDeformationNetwork) {
			// The point was in a deformation network.
			// The arrow should be rendered black.
			const GPlatesViewOperations::RenderedGeometry rendered_arrow =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
							point,
							velocity.d_vector,
							d_render_params.velocity_ratio_unit_vector_direction_to_globe_radius,
							GPlatesGui::Colour::get_black());
			// Render the rendered geometry.
			render(rendered_arrow);

		} else if (velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::InPlateBoundary ||
			velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::InStaticPolygon ||
			velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::ReconstructedDomainPoint) {
			// The point was either:
			//   - in a plate boundary (or a reconstructed static polygon), or
			//   - a reconstructed domain point that used the domain features' plate ID.
			// Colour the arrow according to the plate ID.
			//
			// The ReconstructionGeometry passed into the ColourProxy is either that of the plate
			// boundary or that of the originating/domain feature depending on the 'reason'.
			// But situations are handled the same though.

			const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type &plate_id_recon_geom =
					velocity.d_plate_id_reconstruction_geometry;
			if (plate_id_recon_geom) {
				GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type rg_non_null_ptr =
						plate_id_recon_geom.get();

				const GPlatesViewOperations::RenderedGeometry rendered_arrow =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
								point,
								velocity.d_vector,
								d_render_params.velocity_ratio_unit_vector_direction_to_globe_radius,
								GPlatesGui::ColourProxy(rg_non_null_ptr));

				// Render the rendered geometry.
				render(rendered_arrow);
			} else {
				const GPlatesViewOperations::RenderedGeometry rendered_arrow =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
								point,
								velocity.d_vector,
								d_render_params.velocity_ratio_unit_vector_direction_to_globe_radius,
								GPlatesGui::Colour::get_olive());

				// Render the rendered geometry.
				render(rendered_arrow);
			}
		}
		// else, don't render
	}
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);
	
	GPlatesGui::DrawStyle ds = d_style_adapter ? 
		d_style_adapter->get_style(rfg->get_feature_ref()) : 
		GPlatesGui::DrawStyle();

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			create_rendered_reconstruction_geometry(
					rfg->reconstructed_geometry(),
					rfg,
					d_render_params,
					d_colour,
					d_reconstruction_adjustment,
					d_feature_type_symbol_map,
					ds);

	// The rendered geometry represents the reconstruction geometry so render to the spatial partition.
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_raster_type> &rr)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	// Create a RenderedGeometry for drawing the resolved raster.
	GPlatesViewOperations::RenderedGeometry rendered_resolved_raster =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_resolved_raster(
					rr,
					d_render_params.raster_colour_palette,
					d_render_params.raster_modulate_colour);


	// Create a RenderedGeometry for storing the ReconstructionGeometry and
	// a RenderedGeometry associated with it.
	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
					rr,
					rendered_resolved_raster);

	// Render the rendered geometry.
	render(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_scalar_field_3d_type> &rsf)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	// Create a RenderedGeometry for drawing the scalar field.
	GPlatesViewOperations::RenderedGeometry rendered_resolved_scalar_field =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_resolved_scalar_field_3d(
					rsf,
					d_render_params.scalar_field_render_parameters);

	// Create a RenderedGeometry for storing the ReconstructionGeometry and
	// a RenderedGeometry associated with it.
	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
					rsf,
					rendered_resolved_scalar_field);

	// Render the rendered geometry.
	render(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_virtual_geomagnetic_pole_type> &rvgp)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	if(rvgp->vgp_params().d_vgp_point)
	{
		GPlatesViewOperations::RenderedGeometry rendered_vgp_point =
				create_rendered_reconstruction_geometry(
						*rvgp->vgp_params().d_vgp_point,
						rvgp,
						d_render_params,
						d_colour,
						d_reconstruction_adjustment);

		// Render the rendered geometry.
		render(rendered_vgp_point);
	}

	boost::optional<GPlatesMaths::PointOnSphere> pole_point = boost::none;
	boost::optional<GPlatesMaths::PointOnSphere> site_point = boost::none;
	if(d_reconstruction_adjustment)
	{
		pole_point = (*d_reconstruction_adjustment) * (**rvgp->vgp_params().d_vgp_point);
		if(rvgp->vgp_params().d_site_point)
		{
			site_point = (*d_reconstruction_adjustment) * (**rvgp->vgp_params().d_site_point);
		}
	}
	else
	{
		pole_point = (**rvgp->vgp_params().d_vgp_point);
		if(rvgp->vgp_params().d_site_point)
		{
			site_point = (**rvgp->vgp_params().d_site_point);
		}
	}
	if (d_render_params.vgp_draw_circular_error &&
		rvgp->vgp_params().d_a95 )
	{
		GPlatesViewOperations::RenderedGeometry rendered_small_circle = 
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_small_circle(
						GPlatesMaths::SmallCircle::create_colatitude(
								pole_point->position_vector(),
								GPlatesMaths::convert_deg_to_rad(*rvgp->vgp_params().d_a95)),
						GPlatesGui::ColourProxy(rvgp));

		// The circle/ellipse geometries are not (currently) queryable, so we
		// just add the rendered geometry to the layer.

		// Render the rendered geometry.
		render(rendered_small_circle);
	}
	// We can only draw an ellipse if we have dm and dp defined, and if we have
	// a site point. We need the site point so that we can align the ellipse axes
	// appropriately. 
	else if (
			!d_render_params.vgp_draw_circular_error && 
			rvgp->vgp_params().d_dm  && 
			rvgp->vgp_params().d_dp  && 
			rvgp->vgp_params().d_site_point )
	{
		GPlatesMaths::GreatCircle great_circle(
				*site_point,
				*pole_point);
				
		GPlatesViewOperations::RenderedGeometry rendered_ellipse = 
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_ellipse(
					*pole_point,
					GPlatesMaths::convert_deg_to_rad(*rvgp->vgp_params().d_dp),
					GPlatesMaths::convert_deg_to_rad(*rvgp->vgp_params().d_dm),
					great_circle,
					GPlatesGui::ColourProxy(rvgp));

		// The circle/ellipse geometries are not (currently) queryable, so we
		// just add the rendered geometry to the layer.

		// Render the rendered geometry.
		render(rendered_ellipse);
	}
}

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	GPlatesGui::DrawStyle ds = d_style_adapter ? 
			d_style_adapter->get_style(rtg->get_feature_ref()) : 
			GPlatesGui::DrawStyle();

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
		create_rendered_reconstruction_geometry(
				rtg->resolved_topology_geometry(), 
				rtg, 
				d_render_params, 
				d_colour,
				boost::none,
				boost::none,
				ds);

	// Render the rendered geometry.
	render(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	GPlatesGui::DrawStyle ds = d_style_adapter ? 
		d_style_adapter->get_style(rtn->get_feature_ref()) : 
		GPlatesGui::DrawStyle();

	// Check for Mesh Triangulation
	if (d_render_params.show_topological_network_mesh_triangulation)
	{
		const std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>& 
			mesh_geometries = rtn->resolved_topology_geometries_from_mesh();

		std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>::const_iterator 
			mesh_it = mesh_geometries.begin();
		std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>::const_iterator 
		mesh_it_end = mesh_geometries.end();
		for(; mesh_it !=mesh_it_end; mesh_it++)
		{
			GPlatesViewOperations::RenderedGeometry rendered_geometry =
				create_rendered_reconstruction_geometry(
						*mesh_it, 
						rtn, 
						d_render_params, 
						d_colour,
						boost::none,
						boost::none,
						ds);

			
			// Render the rendered geometry.
			render(rendered_geometry);
		}
	}
		
	// check for drawing of 2D + C Triangulation 
	if (d_render_params.show_topological_network_constrained_triangulation)
	{
		// FIXME: at some point we might want to propagate control up to something like 
		// d_render_params.clip_constrained ... ?  for now hard code to true ...
		bool clip_to_mesh = true; 
		const std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>& 
			geometries = rtn->resolved_topology_geometries_from_constrained( clip_to_mesh );
		std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>::const_iterator 
			it = geometries.begin();
		std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>::const_iterator 
			it_end = geometries.end();
		for(; it !=it_end; it++)
		{
			GPlatesViewOperations::RenderedGeometry rendered_geometry =
				create_rendered_reconstruction_geometry(
						*it, 
						rtn, 
						d_render_params, 
						GPlatesGui::Colour::get_grey(),
						boost::none,
						boost::none);

			// Add to the rendered geometry layer.
			render(rendered_geometry);
		}
	}

	// check for drawing of 2D total triangulation
	if (d_render_params.show_topological_network_delaunay_triangulation)
	{
		// FIXME: at some point we might want to propagate control up to something like 
		// d_render_params.clip_constrained ... ?  for now hard code to true ...
		bool clip_to_mesh = true; 

		const std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>& 
			geometries = rtn->resolved_topology_geometries_from_triangulation_2( clip_to_mesh );

		std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>::const_iterator 
			it = geometries.begin();
		std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>::const_iterator 
			it_end = geometries.end();
		for(; it !=it_end; it++)
		{
			GPlatesViewOperations::RenderedGeometry rendered_geometry =
				create_rendered_reconstruction_geometry(
						*it, 
						rtn, 
						d_render_params, 
						GPlatesGui::Colour::get_black(),
						boost::none,
						boost::none);

			// Add to the rendered geometry layer.
			render(rendered_geometry);
		}
	}

	// check for drawing of network segment velocity vectors 
	if (d_render_params.show_topological_network_segment_velocity)
	{
		render_topological_network_velocities(rtn, d_render_params, d_colour);
	}

}

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
	const GPlatesUtils::non_null_intrusive_ptr<reconstructed_flowline_type> &rf)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	GPlatesGui::DefaultPlateIdColourPalette::non_null_ptr_to_const_type palette =
		GPlatesGui::DefaultPlateIdColourPalette::create();

	// Reconstructed seed point - we can use the first point of either of the lines as an extra point for reference.
	// FIXME: Could(perhaps should?) also do this for the end points of each line....
	GPlatesViewOperations::RenderedGeometry rendered_seed_point =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
			rf->left_flowline_points()->start_point(),
			d_colour ? d_colour.get() : palette->get_colour(0),
			3); // experiment with a bigger point, so that it stands out in relation to the left/right lines. 

	GPlatesViewOperations::RenderedGeometry seed_point_rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rf,
			rendered_seed_point);


	// Render the rendered geometry.
	render(seed_point_rendered_geometry);

	// Left-plate flowline
	GPlatesViewOperations::RenderedGeometry left_rendered_geom =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_arrowed_polyline(
			rf->left_flowline_points(),
			d_colour ? d_colour.get() : palette->get_colour(rf->left_plate_id()));

	GPlatesViewOperations::RenderedGeometry left_rendered_geometry = 
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rf,
			left_rendered_geom);

	// Render the rendered geometry.
	render(left_rendered_geometry);

	// Downstream
	GPlatesViewOperations::RenderedGeometry right_rendered_geom =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_arrowed_polyline(
			rf->right_flowline_points(),
			d_colour ? d_colour.get() : palette->get_colour(rf->right_plate_id()));

	GPlatesViewOperations::RenderedGeometry right_rendered_geometry = 
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rf,
			right_rendered_geom);

	// Render the rendered geometry.
	render(right_rendered_geometry);

}

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
	const GPlatesUtils::non_null_intrusive_ptr<reconstructed_motion_path_type> &rmp)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	// Create a RenderedGeometry for drawing the reconstructed geometry.
	// Draw it in the specified colour (if specified) otherwise defer colouring to a later time
	// using ColourProxy.
	GPlatesViewOperations::RenderedGeometry rendered_geom =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_arrowed_polyline(
			rmp->motion_path_points(),
			d_colour ? d_colour.get() : GPlatesGui::ColourProxy(rmp));

	GPlatesViewOperations::RenderedGeometry rendered_geometry = 
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rmp,
			rendered_geom);

	// Render the rendered geometry.
	render(rendered_geometry);

}

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
        const GPlatesUtils::non_null_intrusive_ptr<reconstructed_small_circle_type> &rsc)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		d_rendered_geometries_spatial_partition,
		GPLATES_ASSERTION_SOURCE);

	// Create a RenderedGeometry for drawing the reconstructed geometry.
	// Draw it in the specified colour (if specified) otherwise defer colouring to a later time
	// using ColourProxy.
	GPlatesViewOperations::RenderedGeometry rendered_geom =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_small_circle(
			GPlatesMaths::SmallCircle::create_colatitude(
				rsc->centre()->position_vector(),rsc->radius()),
			d_colour ? d_colour.get() : GPlatesGui::ColourProxy(rsc));

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
		rsc,
		rendered_geom);

	render(rendered_geometry);

}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<co_registration_data_type> &crr)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometries_spatial_partition,
			GPLATES_ASSERTION_SOURCE);

	//TODO: 
	// Add to the rendered geometry layer.
	/*
	using namespace GPlatesDataMining;
	using namespace GPlatesAppLogic;
	using namespace GPlatesMaths;

	const DataTable& table = crr->data_table();
	BOOST_FOREACH(const DataTable::value_type& row, table )
	{
		const std::vector< const ReconstructedFeatureGeometry* >& rfgs = row->seed_rfgs();
		BOOST_FOREACH(const ReconstructedFeatureGeometry* rfg, rfgs )
		{
			GPlatesViewOperations::RenderedGeometry rendered_geometry =
				create_rendered_reconstruction_geometry(
						rfg->reconstructed_geometry(), crr, d_render_params, GPlatesGui::Colour::get_red());

			// Render the rendered geometry.
			render(rendered_geometry);
			
			const PointOnSphere* point = dynamic_cast<const PointOnSphere*>(rfg->reconstructed_geometry().get());
			if(point)
			{
				GPlatesViewOperations::RenderedGeometry rendered_small_circle = 
					GPlatesViewOperations::create_rendered_small_circle(
							*point,
							GPlatesMaths::convert_deg_to_rad(12),
							GPlatesGui::Colour::get_red());

				// Render the rendered geometry.
				render(rendered_small_circle);	
			}
		}
	}*/
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_velocities(
		const GPlatesAppLogic::resolved_topological_network_non_null_ptr_to_const_type &topological_network,
		const ReconstructionGeometryRenderer::RenderParams &render_params,
		const boost::optional<GPlatesGui::Colour> &colour)
{
	const GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities &
			topological_network_velocities = topological_network->get_network_velocities();

	if (!topological_network_velocities.contains_velocities())
	{
		return;
	}

	const GPlatesGui::Colour &velocity_colour = colour
			? colour.get()
			: GPlatesGui::Colour::get_white();

	// Get the velocities at the network points.
	std::vector<GPlatesMaths::PointOnSphere> network_points;
	std::vector<GPlatesMaths::VectorColatitudeLongitude> network_velocities;
	topological_network_velocities.get_network_velocities(
			network_points, network_velocities);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			network_points.size() == network_velocities.size(),
			GPLATES_ASSERTION_SOURCE);

	// Render each velocity in the current network.
	for (std::size_t velocity_index = 0;
		velocity_index < network_velocities.size();
		++velocity_index)
	{
		const GPlatesMaths::PointOnSphere &point = network_points[velocity_index];
		const GPlatesMaths::Vector3D velocity_vector =
				GPlatesMaths::convert_vector_from_colat_lon_to_xyz(
						point, network_velocities[velocity_index]);

		// Create a RenderedGeometry using the velocity vector.
		const GPlatesViewOperations::RenderedGeometry rendered_vector =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
				point,
				velocity_vector,
				render_params.velocity_ratio_unit_vector_direction_to_globe_radius,
				velocity_colour);

		// Create a RenderedGeometry for storing the ReconstructionGeometry and
		// a RenderedGeometry associated with it.
		//
		// This means the resolved topological network can be selected by clicking on of
		// its velocity arrows (note: currently arrows cannot be selected so this will
		// not do anything).
		const GPlatesViewOperations::RenderedGeometry rendered_reconstruction_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
						topological_network,
						rendered_vector);

		// Render the rendered geometry.
		render(rendered_reconstruction_geometry);
	}
}


// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the file because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

