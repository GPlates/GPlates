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

#include <cmath>
#include <cstddef> // For std::size_t
#include <boost/foreach.hpp>
#include <CGAL/centroid.h>

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
#include "app-logic/DeformedFeatureGeometry.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/MultiPointVectorField.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/PropertyExtractors.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructedSmallCircle.h"
#include "app-logic/ReconstructedVirtualGeomagneticPole.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ResolvedRaster.h"
#include "app-logic/ResolvedScalarField3D.h"
#include "app-logic/ResolvedTopologicalGeometry.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTriangulationUtils.h"

#include "data-mining/DataTable.h"

#include "global/GPlatesAssert.h"

#include "gui/Colour.h"
#include "gui/DrawStyleManager.h"
#include "gui/PlateIdColourPalettes.h"

#include "maths/CalculateVelocity.h"
#include "maths/Centroid.h"
#include "maths/MathsUtils.h"

#include "utils/ComponentManager.h"
#include "utils/Profile.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"


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
	 * Returns a GPlatesGui::ColourProxy.
	 */
	GPlatesGui::ColourProxy
	get_colour(
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry,
			const boost::optional<GPlatesGui::Colour> &colour,
			boost::optional<const GPlatesGui::StyleAdapter &> style_adapter)
	{
		// If on override colour has been provided then use that.
		if (colour)
		{
			return GPlatesGui::ColourProxy(colour.get());
		}

		// If python colouring is enabled then use the python draw style.
		if (GPlatesUtils::ComponentManager::instance().is_enabled(GPlatesUtils::ComponentManager::Component::python()))
		{
			GPlatesGui::DrawStyle style;

			if (style_adapter)
			{
				boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
						GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
								reconstruction_geometry);
				if (feature_ref)
				{
					style = style_adapter->get_style(feature_ref.get());
				}
			}

			return GPlatesGui::ColourProxy(style.colour);
		}

		// Use the old method of colouring based on hard-coded (C++) colour schemes where the
		// colour is determined using feature properties.
		//
		// Note: This also used to be deferred (under actual painting) colouring but not sure
		// if that's still the case.
		return GPlatesGui::ColourProxy(reconstruction_geometry);
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
			const GPlatesGui::ColourProxy &colour_proxy,
			const boost::optional<GPlatesMaths::Rotation> &rotation = boost::none,
			const boost::optional<GPlatesGui::symbol_map_type> &feature_type_symbol_map = boost::none)
	{
		boost::optional<GPlatesGui::Symbol> symbol = get_symbol(
				feature_type_symbol_map, reconstruction_geometry);

		// Create a RenderedGeometry for drawing the reconstruction geometry.
		GPlatesViewOperations::RenderedGeometry rendered_geom =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						rotation ? rotation.get() * geometry : geometry,
						colour_proxy,
						render_params.reconstruction_point_size_hint,
						render_params.reconstruction_line_width_hint,
						render_params.fill_polygons,
						render_params.fill_polylines,
						render_params.fill_modulate_colour,
						symbol);

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
		bool fill_polylines_,
		float ratio_zoom_dependent_bin_dimension_to_globe_radius_,
		float ratio_arrow_unit_vector_direction_to_globe_radius_,
		float ratio_arrowhead_size_to_globe_radius_,
		bool show_deformed_feature_geometries_,
		bool show_strain_accumulation_,
		double strain_accumulation_scale_,
		bool show_topological_network_delaunay_triangulation_,
		bool show_topological_network_constrained_triangulation_,
		bool show_topological_network_mesh_triangulation_,
		bool show_topological_network_total_triangulation_,
		bool show_topological_network_segment_velocity_,
		int topological_network_color_index_) :
	reconstruction_line_width_hint(reconstruction_line_width_hint_),
	reconstruction_point_size_hint(reconstruction_point_size_hint_),
	fill_polygons(fill_polygons_),
	fill_polylines(fill_polylines_),
	ratio_zoom_dependent_bin_dimension_to_globe_radius(ratio_zoom_dependent_bin_dimension_to_globe_radius_),
	ratio_arrow_unit_vector_direction_to_globe_radius(ratio_arrow_unit_vector_direction_to_globe_radius_),
	ratio_arrowhead_size_to_globe_radius(ratio_arrowhead_size_to_globe_radius_),
	raster_colour_palette(GPlatesGui::RasterColourPalette::create()),
	fill_modulate_colour(1, 1, 1, 1),
	normal_map_height_field_scale_factor(1),
	vgp_draw_circular_error(true),
	show_deformed_feature_geometries(show_deformed_feature_geometries_),
	show_strain_accumulation(show_strain_accumulation_),
	strain_accumulation_scale(strain_accumulation_scale_),
	show_topological_network_delaunay_triangulation( show_topological_network_delaunay_triangulation_),
	show_topological_network_constrained_triangulation( show_topological_network_constrained_triangulation_),
	show_topological_network_mesh_triangulation( show_topological_network_mesh_triangulation_),
	show_topological_network_total_triangulation( show_topological_network_total_triangulation_),
	show_topological_network_segment_velocity( show_topological_network_segment_velocity_),
	topological_network_color_index( topological_network_color_index_),
	user_colour_palette(GPlatesGui::UserColourPalette::create())
{
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_raster_visual_layer_params(
		const RasterVisualLayerParams &params)
{
	d_render_params.raster_colour_palette = params.get_colour_palette();
	d_render_params.fill_modulate_colour = params.get_modulate_colour();
	d_render_params.normal_map_height_field_scale_factor = params.get_surface_relief_scale();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_reconstruct_visual_layer_params(
		const ReconstructVisualLayerParams &params)
{
	d_render_params.vgp_draw_circular_error = params.get_vgp_draw_circular_error();
	d_render_params.fill_polygons = params.get_fill_polygons();
	d_render_params.fill_polylines = params.get_fill_polylines();
	d_render_params.fill_modulate_colour = params.get_fill_modulate_colour();
	d_render_params.show_deformed_feature_geometries = params.get_show_deformed_feature_geometries();
	d_render_params.show_strain_accumulation = params.get_show_strain_accumulation();
	d_render_params.strain_accumulation_scale = params.get_strain_accumulation_scale();
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
	d_render_params.show_topological_network_total_triangulation = params.show_total_triangulation(); 
	d_render_params.show_topological_network_segment_velocity = params.show_segment_velocity();
	d_render_params.fill_polygons = params.show_fill();
	d_render_params.topological_network_color_index = params.color_index();
	d_render_params.raster_colour_palette = params.get_colour_palette();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_velocity_field_calculator_visual_layer_params(
		const VelocityFieldCalculatorVisualLayerParams &params)
{
	d_render_params.ratio_zoom_dependent_bin_dimension_to_globe_radius = params.get_arrow_spacing();
	d_render_params.ratio_arrow_unit_vector_direction_to_globe_radius = params.get_arrow_body_scale();
	d_render_params.ratio_arrowhead_size_to_globe_radius = params.get_arrowhead_scale();
}


GPlatesPresentation::ReconstructionGeometryRenderer::ReconstructionGeometryRenderer(
		const RenderParams &render_params,
		const boost::optional<GPlatesGui::Colour> &colour,
		const boost::optional<GPlatesMaths::Rotation> &reconstruction_adjustment,
		const boost::optional<GPlatesGui::symbol_map_type> &feature_type_symbol_map,
		boost::optional<const GPlatesGui::StyleAdapter &> style_adaptor) :
	d_render_params(render_params),
	d_colour(colour),
	d_reconstruction_adjustment(reconstruction_adjustment),
	d_feature_type_symbol_map(feature_type_symbol_map),
	d_style_adapter(style_adaptor)
{
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::begin_render(
		GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	// We've started targeting a rendered geometry layer.
	d_rendered_geometry_layer = rendered_geometry_layer;

	// Set up any limits on the screen-space density of geometries in the rendered geometry layer.
	// A value of zero will set no limits.
	d_rendered_geometry_layer->set_ratio_zoom_dependent_bin_dimension_to_globe_radius(
			d_render_params.ratio_zoom_dependent_bin_dimension_to_globe_radius);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::end_render()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	// We're not targeting a rendered geometry layer anymore.
	d_rendered_geometry_layer = boost::none;
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
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

		if (velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::InNetworkDeformingRegion) {
			// The point was in the deforming region of a deformation network.
			// This means it was inside the network but outside any interior rigid blocks on the network.
			// The arrow should be rendered black.
			const GPlatesViewOperations::RenderedGeometry rendered_arrow =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
							point,
							velocity.d_vector,
							d_render_params.ratio_arrow_unit_vector_direction_to_globe_radius,
							GPlatesGui::Colour::get_black(),
							d_render_params.ratio_arrowhead_size_to_globe_radius);
			// Render the rendered geometry.
			render(rendered_arrow);

		} else if (velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::InNetworkRigidBlock ||
			velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::InPlateBoundary ||
			velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::InStaticPolygon ||
			velocity.d_reason == GPlatesAppLogic::MultiPointVectorField::CodomainElement::ReconstructedDomainPoint) {
			// The point was either:
			//   - in a interior rigid block of a network, or
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
								d_render_params.ratio_arrow_unit_vector_direction_to_globe_radius,
								GPlatesGui::ColourProxy(rg_non_null_ptr),
								d_render_params.ratio_arrowhead_size_to_globe_radius);

				// Render the rendered geometry.
				render(rendered_arrow);
			} else {
				const GPlatesViewOperations::RenderedGeometry rendered_arrow =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
								point,
								velocity.d_vector,
								d_render_params.ratio_arrow_unit_vector_direction_to_globe_radius,
								GPlatesGui::Colour::get_olive(),
								d_render_params.ratio_arrowhead_size_to_globe_radius);

				// Render the rendered geometry.
				render(rendered_arrow);
			}
		}
		// else, don't render
	}
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<deformed_feature_geometry_type> &dfg)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesGui::ColourProxy dfg_colour_proxy =  get_colour(dfg, d_colour, d_style_adapter);

	if (d_render_params.show_deformed_feature_geometries)
	{
		GPlatesViewOperations::RenderedGeometry rendered_geometry =
			create_rendered_reconstruction_geometry(
					dfg->reconstructed_geometry(),
					dfg,
					d_render_params,
					dfg_colour_proxy,
					d_reconstruction_adjustment,
					d_feature_type_symbol_map);

		// The rendered geometry represents the reconstruction geometry so render to the spatial partition.
		render_reconstruction_geometry_on_sphere(rendered_geometry);
	}

	if (d_render_params.show_strain_accumulation)
	{
		// Convenience typedefs.
		typedef std::vector<GPlatesMaths::PointOnSphere> deformed_geometry_points_seq_type;
		typedef GPlatesAppLogic::DeformedFeatureGeometry::point_deformation_info_seq_type
				point_deformation_info_seq_type;

		// Get the deformed geometry points.
		deformed_geometry_points_seq_type deformed_geometry_points;
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*dfg->reconstructed_geometry(),
				deformed_geometry_points);

		// Get the deformation information associated with each deformed geometry point.
		const point_deformation_info_seq_type &point_deformation_information =
				dfg->get_point_deformation_information();

		// Iterate over the geometry points and generate a rendered geometry for each one.
		deformed_geometry_points_seq_type::const_iterator deformed_geometry_points_iter =
				deformed_geometry_points.begin();
		deformed_geometry_points_seq_type::const_iterator deformed_geometry_points_end =
				deformed_geometry_points.end();
		point_deformation_info_seq_type::const_iterator point_deformation_infos_iter =
				point_deformation_information.begin();
		point_deformation_info_seq_type::const_iterator point_deformation_infos_end =
				point_deformation_information.end();
		for ( ;
			deformed_geometry_points_iter != deformed_geometry_points_end &&
				point_deformation_infos_iter != point_deformation_infos_end;
			++deformed_geometry_points_iter, ++point_deformation_infos_iter)
		{
			const GPlatesMaths::PointOnSphere &deformed_geometry_point = *deformed_geometry_points_iter;
			const GPlatesAppLogic::GeometryDeformation::DeformationInfo &point_deformation_info =
					*point_deformation_infos_iter;

			// Determine the strain colour.
			boost::optional<GPlatesGui::Colour> strain_accumulation_colour = GPlatesGui::Colour::get_black();
#if 0
			// FIXME: find out how to allow gui selection of .cpt file 
			// NOTE: using this type of cmd:
			// $ makecpt -Cpolar -I -T-19/19/1 > /private/tmp/strain_test.cpt
			boost::optional<GPlatesGui::CptPalette*> cpt_opt;
			try
			{
				cpt_opt = new GPlatesGui::CptPalette("/private/tmp/strain_test.cpt");
			}
			catch (std::exception &exc)
			{
				qWarning() << "Cannot read '/private/tmp/strain_test.cpt'; Using default built-in color palette" << exc.what();
			}
			catch(...)
			{
				qWarning() << "Cannot read '/private/tmp/strain_test.cpt'; Using default built-in color palette";
			}
			
			if (cpt_opt)
			{
				if (cpt_opt.get()->get_colour( strain ) )
				{
					strain_accumulation_colour = cpt_opt.get()->get_colour( strain ).get();
				}
			}
#endif

			const GPlatesGui::ColourProxy strain_accumulation_colour_proxy = strain_accumulation_colour
					? GPlatesGui::ColourProxy(strain_accumulation_colour.get())
					: dfg_colour_proxy;

			GPlatesGui::Symbol strain_accumulation_symbol(
					GPlatesGui::Symbol::STRAIN_MARKER, // type
					1, // size
					true, // fill
					// Scale the strain accumulation...
					point_deformation_info.S1 * d_render_params.strain_accumulation_scale, // scale_x
					point_deformation_info.S2 * d_render_params.strain_accumulation_scale, // scale_y
					point_deformation_info.S_DIR); // orientation angle

			// Create a RenderedGeometry.
			GPlatesViewOperations::RenderedGeometry strain_accumulation_rendered_geom =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
							d_reconstruction_adjustment
									? d_reconstruction_adjustment.get() * deformed_geometry_point.clone_as_point()
									: deformed_geometry_point.clone_as_point(),
							strain_accumulation_colour_proxy,
							d_render_params.reconstruction_point_size_hint,
							d_render_params.reconstruction_line_width_hint,
							d_render_params.fill_polygons,
							d_render_params.fill_polylines,
							d_render_params.fill_modulate_colour,
							strain_accumulation_symbol);

			// Create a RenderedGeometry for storing the ReconstructionGeometry and
			// a RenderedGeometry associated with it.
			GPlatesViewOperations::RenderedGeometry strain_accumulation_rendered_reconstruction_geometry =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
							dfg,
							strain_accumulation_rendered_geom);

			// The rendered geometry represents the reconstruction geometry so render to the spatial partition.
			render_reconstruction_geometry_on_sphere(strain_accumulation_rendered_reconstruction_geometry);
		}
	}
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
		create_rendered_reconstruction_geometry(
				rfg->reconstructed_geometry(),
				rfg,
				d_render_params,
				get_colour(rfg, d_colour, d_style_adapter),
				d_reconstruction_adjustment,
				d_feature_type_symbol_map);

	// The rendered geometry represents the reconstruction geometry so render to the spatial partition.
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_raster_type> &rr)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	// Create a RenderedGeometry for drawing the resolved raster.
	GPlatesViewOperations::RenderedGeometry rendered_resolved_raster =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_resolved_raster(
					rr,
					d_render_params.raster_colour_palette,
					d_render_params.fill_modulate_colour,
					d_render_params.normal_map_height_field_scale_factor);


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
			d_rendered_geometry_layer,
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
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	// The RVGP feature-properties-based colour.
	const GPlatesGui::ColourProxy rvgp_colour = get_colour(rvgp, d_colour, d_style_adapter);
	
	if(rvgp->vgp_params().d_vgp_point)
	{
		GPlatesViewOperations::RenderedGeometry rendered_vgp_point =
				create_rendered_reconstruction_geometry(
						*rvgp->vgp_params().d_vgp_point,
						rvgp,
						d_render_params,
						rvgp_colour,
						d_reconstruction_adjustment,
						d_feature_type_symbol_map);

		// The rendered geometry represents a geometry-on-sphere so render to the spatial partition.
		render_reconstruction_geometry_on_sphere(rendered_vgp_point);
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
						rvgp_colour);

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
					rvgp_colour);

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
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
		create_rendered_reconstruction_geometry(
				rtg->resolved_topology_geometry(), 
				rtg, 
				d_render_params, 
				get_colour(rtg, d_colour, d_style_adapter));

	// The rendered geometry represents a geometry-on-sphere so render to the spatial partition.
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	//PROFILE_BLOCK("ReconstructionGeometryRenderer::visit(resolved_topological_network_type)");

#if 0
qDebug() << "GPlatesPresentation::ReconstructionGeometryRenderer::visit( rtn )";
qDebug() << "GPlatesPresentation::ReconstructionGeometryRenderer::visit( rtn ): d_colour.get() =" << d_colour.get();
#endif

	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	// Check for drawing mesh of constrained delaunay triangulation.
	if (d_render_params.show_topological_network_mesh_triangulation)
	{
		// Get the colour based on the feature properties of the resolved topological network.
		const GPlatesGui::ColourProxy rtn_colour = get_colour(rtn, d_colour, d_style_adapter);

		render_topological_network_constrained_delaunay_triangulation(
				rtn,
				true/*clip_to_mesh*/,
				rtn_colour);
	}

	// Check for drawing constrained delaunay triangulation.
	if (d_render_params.show_topological_network_constrained_triangulation)
	{
		bool clip_to_mesh = ! d_render_params.show_topological_network_total_triangulation;

		render_topological_network_constrained_delaunay_triangulation(
				rtn,
				clip_to_mesh,
				GPlatesGui::Colour::get_grey());
	}

	// Check for drawing delaunay triangulation.
	if (d_render_params.show_topological_network_delaunay_triangulation)
	{
		// This was previously in...
		//    ResolvedTopologicalNetwork::get_resolved_topology_geometries_from_triangulation_2()
		// ...but that method has been removed and simplified inline here so this is just to keep
		// the debugging until it's not needed anymore (at which point this method should be removed).
		rtn->report_deformation_to_file();

		bool clip_to_mesh = ! d_render_params.show_topological_network_total_triangulation;

		render_topological_network_delaunay_triangulation(rtn, clip_to_mesh);
	}

	// Check for drawing velocity vectors at vertices of delaunay triangulation.
	if (d_render_params.show_topological_network_segment_velocity)
	{
		render_topological_network_velocities(rtn);
	}

}

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
	const GPlatesUtils::non_null_intrusive_ptr<reconstructed_flowline_type> &rf)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
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
			d_rendered_geometry_layer,
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
		d_rendered_geometry_layer,
		GPLATES_ASSERTION_SOURCE);

	// Create a RenderedGeometry for drawing the reconstructed geometry.
	// Draw it in the specified colour (if specified) otherwise defer colouring to a later time
	// using ColourProxy.

	GPlatesViewOperations::RenderedGeometry rendered_geom =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_small_circle(
			GPlatesMaths::SmallCircle::create_colatitude(
				rsc->centre()->position_vector(),rsc->radius()),
			get_colour(rsc, d_colour, d_style_adapter));

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
			d_rendered_geometry_layer,
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
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_constrained_delaunay_triangulation(
		const GPlatesAppLogic::resolved_topological_network_non_null_ptr_to_const_type &rtn,
		bool clip_to_mesh,
		const GPlatesGui::ColourProxy &colour)
{
	const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network =
			rtn->get_triangulation_network();

	const GPlatesMaths::ProjectionUtils::AzimuthalEqualArea &resolved_triangulation_projection =
			resolved_triangulation_network.get_projection();

	const GPlatesAppLogic::ResolvedTriangulation::ConstrainedDelaunay_2 &
			constrained_delaunay_triangulation_2 =
					resolved_triangulation_network.get_constrained_delaunay_2();

	// Keep track of vertex indices of unique triangulation vertices referenced by the faces.
	typedef GPlatesAppLogic::ResolvedTriangulation::VertexIndices<
			GPlatesAppLogic::ResolvedTriangulation::ConstrainedDelaunay_2>
					constrained_delaunay_vertex_indices_type;
	constrained_delaunay_vertex_indices_type constrained_delaunay_vertex_indices;

	// The triangle/vertices of our rendered mesh (if fill is turned on).
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::triangle_seq_type rendered_triangle_mesh_triangles;
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::vertex_seq_type rendered_triangle_mesh_vertices;

	// The edges/vertices of our rendered mesh (if fill is turned off).
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::edge_seq_type rendered_edge_mesh_edges;
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::vertex_seq_type rendered_edge_mesh_vertices;

	// Iterate over the individual faces of the constrained triangulation.
	GPlatesAppLogic::ResolvedTriangulation::ConstrainedDelaunay_2::Finite_faces_iterator
			constrained_finite_faces_2_iter = constrained_delaunay_triangulation_2.finite_faces_begin();
	GPlatesAppLogic::ResolvedTriangulation::ConstrainedDelaunay_2::Finite_faces_iterator
			constrained_finite_faces_2_end =  constrained_delaunay_triangulation_2.finite_faces_end();
	for ( ; constrained_finite_faces_2_iter != constrained_finite_faces_2_end; ++constrained_finite_faces_2_iter)
	{
		// If clipping to mesh then only draw those triangles in the interior of the meshed region.
		// This excludes areas with seed points (also called micro blocks) and excludes regions
		// outside the envelope of the bounded area.
		if (clip_to_mesh && !constrained_finite_faces_2_iter->is_in_domain())
		{
			continue;
		}

		if (d_render_params.fill_polygons)
		{
			const GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::Triangle rendered_mesh_triangle(
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(0)),
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(1)),
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(2)),
					colour);
			rendered_triangle_mesh_triangles.push_back(rendered_mesh_triangle);
		}
		else // Render mesh as edges/lines...
		{
			// Render the three edges of the current triangle.
			//
			// NOTE: Two triangles that share an edge will both draw it - so it'll get drawn twice.
			//
			// TODO: Only emit one rendered mesh edge per triangulation edge (perhaps by iterating
			// over edges instead of triangles).

			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge1(
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(0)),
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(1)),
					colour);
			rendered_edge_mesh_edges.push_back(rendered_mesh_edge1);

			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge2(
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(1)),
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(2)),
					colour);
			rendered_edge_mesh_edges.push_back(rendered_mesh_edge2);

			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge3(
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(2)),
					constrained_delaunay_vertex_indices.add_vertex(constrained_finite_faces_2_iter->vertex(0)),
					colour);
			rendered_edge_mesh_edges.push_back(rendered_mesh_edge3);
		}
	}

	// For each triangulation vertex, referenced by the rendered mesh, project the 2D vertex
	// back onto the 3D globe.
	const constrained_delaunay_vertex_indices_type::vertex_seq_type &mesh_vertices =
			constrained_delaunay_vertex_indices.get_vertices();

	if (d_render_params.fill_polygons)
	{
		rendered_triangle_mesh_vertices.reserve(mesh_vertices.size());
	}
	else // Render mesh as edges/lines...
	{
		rendered_edge_mesh_vertices.reserve(mesh_vertices.size());
	}

	// Iterate over the (referenced) constrained triangulation vertices.
	constrained_delaunay_vertex_indices_type::vertex_seq_type::const_iterator
			mesh_vertices_iter = mesh_vertices.begin();
	constrained_delaunay_vertex_indices_type::vertex_seq_type::const_iterator
			mesh_vertices_end = mesh_vertices.end();
	for ( ; mesh_vertices_iter != mesh_vertices_end; ++mesh_vertices_iter)
	{
		constrained_delaunay_vertex_indices_type::triangulation_type::Vertex_handle
				mesh_vertex = *mesh_vertices_iter;

		// Un-project the 2D triangulation vertex position back onto the 3D sphere.
		const GPlatesMaths::PointOnSphere rendered_mesh_vertex =
				resolved_triangulation_projection.unproject_to_point_on_sphere(
						mesh_vertex->point());

		if (d_render_params.fill_polygons)
		{
			rendered_triangle_mesh_vertices.push_back(rendered_mesh_vertex);
		}
		else // Render mesh as edges/lines...
		{
			rendered_edge_mesh_vertices.push_back(rendered_mesh_vertex);
		}
	}

	// Create the rendered surface mesh.
	GPlatesViewOperations::RenderedGeometry rendered_geometry;
	if (d_render_params.fill_polygons)
	{
		rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_triangle_surface_mesh(
				rendered_triangle_mesh_triangles,
				rendered_triangle_mesh_vertices);
	}
	else // Render mesh as edges/lines...
	{
		rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_edge_surface_mesh(
				rendered_edge_mesh_edges,
				rendered_edge_mesh_vertices,
				d_render_params.reconstruction_line_width_hint);
	}

	// Create a RenderedGeometry for storing the ReconstructionGeometry and a RenderedGeometry associated with it.
	rendered_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rtn,
			rendered_geometry);

	// The rendered geometry represents a geometry on the sphere so render to the spatial partition.
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_delaunay_triangulation(
		const GPlatesAppLogic::resolved_topological_network_non_null_ptr_to_const_type &rtn,
		bool clip_to_mesh)
{
	const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network =
			rtn->get_triangulation_network();

	// NOTE: We avoid retrieving the *constrained* delaunay triangulation since we don't need it
	// and hence can improve performance by not generating it.
	const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &delaunay_triangulation_2 =
			resolved_triangulation_network.get_delaunay_2();

	// Keep track of vertex indices of unique triangulation vertices referenced by the faces.
	typedef GPlatesAppLogic::ResolvedTriangulation::VertexIndices<
			GPlatesAppLogic::ResolvedTriangulation::Delaunay_2>
					delaunay_vertex_indices_type;
	delaunay_vertex_indices_type delaunay_vertex_indices;

	// The triangle/vertices of our rendered mesh (if fill is turned on).
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::triangle_seq_type rendered_triangle_mesh_triangles;
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::vertex_seq_type rendered_triangle_mesh_vertices;

	// The edges/vertices of our rendered mesh (if fill is turned off).
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::edge_seq_type rendered_edge_mesh_edges;
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::vertex_seq_type rendered_edge_mesh_vertices;

	// Iterate over the individual faces of the delaunay triangulation.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_faces_iterator
			finite_faces_2_iter = delaunay_triangulation_2.finite_faces_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_faces_iterator
			finite_faces_2_end = delaunay_triangulation_2.finite_faces_end();
	for ( ; finite_faces_2_iter != finite_faces_2_end; ++finite_faces_2_iter)
	{
		// If clipping triangle to the mesh then only create a face polygon if its centroid is in mesh area.
		if (clip_to_mesh)
		{
			const GPlatesMaths::PointOnSphere face_points[3] =
			{
				finite_faces_2_iter->vertex(0)->get_point_on_sphere(),
				finite_faces_2_iter->vertex(1)->get_point_on_sphere(),
				finite_faces_2_iter->vertex(2)->get_point_on_sphere()
			};

			// Compute centroid of face.
			const GPlatesMaths::PointOnSphere centroid(
					GPlatesMaths::Centroid::calculate_points_centroid(
							face_points,
							face_points + 3));

			if (!resolved_triangulation_network.is_point_in_deforming_region(centroid))
			{
				continue;
			}
		}

		// Set the render value for the triangle.
		boost::optional<double> render_value;

		if (d_render_params.topological_network_color_index != 0)
		{
			const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face::DeformationInfo &
					deformation_info = finite_faces_2_iter->get_deformation_info();

			if (d_render_params.topological_network_color_index == 1)
			{
				const double dilitation = deformation_info.dilitation;
				const double sign = (dilitation < 0) ? -1 : 1;
				render_value = sign * std::fabs( std::log10( std::fabs( dilitation ) ) );
			}
			else if (d_render_params.topological_network_color_index == 2)
			{
				const double second_invariant = deformation_info.second_invariant;
				const double sign = (second_invariant < 0) ? -1 : 1;
				render_value = sign * std::fabs( std::log10( std::fabs( second_invariant ) ) );
			}
		}

		// The colour to use for this triangle panel.
		boost::optional<GPlatesGui::Colour> colour;

		// Check if a render_value was set for this panel.
		if (render_value)
		{
			// Get the colour for this value
			colour = GPlatesGui::RasterColourPaletteColour::get_colour(
				*d_render_params.raster_colour_palette,
				render_value.get());
		}
		// Check if a colour was passed to the ReconstructionGeometryRenderer constructor.
		else if (d_colour)
		{
			colour = d_colour.get();
		}
		// final else ; use Grey
		else
		{
			colour = GPlatesGui::Colour::get_grey();
		}

		if (d_render_params.fill_polygons)
		{
			const GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::Triangle rendered_mesh_triangle(
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(0)),
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(1)),
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(2)),
					colour.get());
			rendered_triangle_mesh_triangles.push_back(rendered_mesh_triangle);
		}
		else // Render mesh as edges/lines...
		{
			// Render the three edges of the current triangle.
			//
			// NOTE: Two triangles that share an edge will both draw it using their own colour.
			// The one that gets drawn on top will the one belonging to the last triangle iterated.
			//
			// TODO: Only emit one rendered mesh edge per triangulation edge (perhaps by iterating
			// over edges instead of triangles) and maybe use the average colour of adjacent triangles.

			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge1(
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(0)),
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(1)),
					colour.get());
			rendered_edge_mesh_edges.push_back(rendered_mesh_edge1);

			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge2(
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(1)),
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(2)),
					colour.get());
			rendered_edge_mesh_edges.push_back(rendered_mesh_edge2);

			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge3(
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(2)),
					delaunay_vertex_indices.add_vertex(finite_faces_2_iter->vertex(0)),
					colour.get());
			rendered_edge_mesh_edges.push_back(rendered_mesh_edge3);
		}
	}

	// For each triangulation vertex, referenced by the rendered mesh, get the un-projected
	// 2D vertex back onto the 3D globe.
	const delaunay_vertex_indices_type::vertex_seq_type &mesh_vertices =
			delaunay_vertex_indices.get_vertices();

	if (d_render_params.fill_polygons)
	{
		rendered_triangle_mesh_vertices.reserve(mesh_vertices.size());
	}
	else // Render mesh as edges/lines...
	{
		rendered_edge_mesh_vertices.reserve(mesh_vertices.size());
	}

	// Iterate over the (referenced) constrained triangulation vertices.
	delaunay_vertex_indices_type::vertex_seq_type::const_iterator
			mesh_vertices_iter = mesh_vertices.begin();
	delaunay_vertex_indices_type::vertex_seq_type::const_iterator
			mesh_vertices_end = mesh_vertices.end();
	for ( ; mesh_vertices_iter != mesh_vertices_end; ++mesh_vertices_iter)
	{
		delaunay_vertex_indices_type::triangulation_type::Vertex_handle mesh_vertex = *mesh_vertices_iter;

		// Get the un-projected 2D triangulation vertex position back onto the 3D sphere.
		const GPlatesMaths::PointOnSphere &rendered_mesh_vertex = mesh_vertex->get_point_on_sphere();

		if (d_render_params.fill_polygons)
		{
			rendered_triangle_mesh_vertices.push_back(rendered_mesh_vertex);
		}
		else // Render mesh as edges/lines...
		{
			rendered_edge_mesh_vertices.push_back(rendered_mesh_vertex);
		}
	}

	// Create the rendered surface mesh.
	GPlatesViewOperations::RenderedGeometry rendered_geometry;
	if (d_render_params.fill_polygons)
	{
		rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_triangle_surface_mesh(
				rendered_triangle_mesh_triangles,
				rendered_triangle_mesh_vertices);
	}
	else // Render mesh as edges/lines...
	{
		rendered_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_edge_surface_mesh(
				rendered_edge_mesh_edges,
				rendered_edge_mesh_vertices,
				d_render_params.reconstruction_line_width_hint);
	}

	// Create a RenderedGeometry for storing the ReconstructionGeometry and a RenderedGeometry associated with it.
	rendered_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rtn,
			rendered_geometry);

	// The rendered geometry represents a geometry on the sphere so render to the spatial partition.
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_velocities(
		const GPlatesAppLogic::resolved_topological_network_non_null_ptr_to_const_type &rtn)
{
	const GPlatesGui::Colour &velocity_colour = d_colour
			? d_colour.get()
			: GPlatesGui::Colour::get_white();

	const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network =
			rtn->get_triangulation_network();

	// NOTE: We avoid retrieving the *constrained* delaunay triangulation since we don't need it
	// and hence can improve performance by not generating it.
	const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &delaunay_triangulation_2 =
			resolved_triangulation_network.get_delaunay_2();

	// Iterate over the individual vertices of the delaunay triangulation and
	// render the velocity at each vertex in the network.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_vertices_iterator
			finite_vertices_2_iter = delaunay_triangulation_2.finite_vertices_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_vertices_iterator
			finite_vertices_2_end = delaunay_triangulation_2.finite_vertices_end();
	for ( ; finite_vertices_2_iter != finite_vertices_2_end; ++finite_vertices_2_iter)
	{
		const GPlatesMaths::PointOnSphere &point = finite_vertices_2_iter->get_point_on_sphere();
		const GPlatesMaths::Vector3D &velocity_vector = finite_vertices_2_iter->get_velocity_vector();

		// Create a RenderedGeometry using the velocity vector.
		const GPlatesViewOperations::RenderedGeometry rendered_vector =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
				point,
				velocity_vector,
				d_render_params.ratio_arrow_unit_vector_direction_to_globe_radius,
				velocity_colour,
				d_render_params.ratio_arrowhead_size_to_globe_radius);

		// Create a RenderedGeometry for storing the ReconstructionGeometry and
		// a RenderedGeometry associated with it.
		//
		// This means the resolved topological network can be selected by clicking on of
		// its velocity arrows (note: currently arrows cannot be selected so this will
		// not do anything).
		const GPlatesViewOperations::RenderedGeometry rendered_reconstruction_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
						rtn,
						rendered_vector);

		// Render the rendered geometry.
		render(rendered_reconstruction_geometry);
	}
}


// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the file because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

