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
#include <utility>
#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>

#include "ReconstructionGeometryRenderer.h"

#include "RasterVisualLayerParams.h"
#include "ReconstructScalarCoverageVisualLayerParams.h"
#include "ReconstructVisualLayerParams.h"
#include "ScalarField3DVisualLayerParams.h"
#include "TopologyGeometryVisualLayerParams.h"
#include "TopologyNetworkVisualLayerParams.h"
#include "VelocityFieldCalculatorVisualLayerParams.h"
#include "ViewState.h"
#include "VisualLayerParamsVisitor.h"


#include "app-logic/ApplicationState.h"
#include "app-logic/CoRegistrationData.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/MultiPointVectorField.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/PropertyExtractors.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructedScalarCoverage.h"
#include "app-logic/ReconstructedSmallCircle.h"
#include "app-logic/ReconstructedVirtualGeomagneticPole.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ResolvedRaster.h"
#include "app-logic/ResolvedScalarField3D.h"
#include "app-logic/ResolvedTopologicalGeometry.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTriangulationDelaunay2.h"
#include "app-logic/TopologyReconstructedFeatureGeometry.h"

#include "data-mining/DataTable.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"
#include "gui/DrawStyleManager.h"
#include "gui/PlateIdColourPalettes.h"
#include "gui/RenderSettings.h"

#include "maths/CalculateVelocity.h"
#include "maths/MathsUtils.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"

#include "property-values/Enumeration.h"
#include "property-values/EnumerationContent.h"

#include "utils/ComponentManager.h"
#include "utils/Profile.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"


namespace
{
	/**
	 * Returns true if the reconstruction geometry is used as a topological section
	 * in any topology for any reconstruction time.
	 */
	template <typename ReconstructionGeometryPointer>
	bool
	is_topological_section(
			const ReconstructionGeometryPointer &reconstruction_geometry,
			const std::set<GPlatesModel::FeatureId> &topological_sections)
	{
		GPlatesModel::FeatureHandle *feature_handle = reconstruction_geometry->feature_handle_ptr();

		return feature_handle != NULL &&
				topological_sections.find(feature_handle->feature_id()) != topological_sections.end();
	}


	/**
	 * Returns a GPlatesGui::Symbol for the feature type of the @a reconstruction_geometry, if
	 * an appropriate entry in the @a feature_type_symbol_map exists.
	 */
	boost::optional<GPlatesGui::Symbol>
	get_symbol(
	    boost::optional<const GPlatesGui::symbol_map_type &> symbol_map,
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
			boost::optional<const GPlatesGui::symbol_map_type &> feature_type_symbol_map = boost::none,
			float line_width_and_point_size_multiplier = 1.0f)
	{
		boost::optional<GPlatesGui::Symbol> symbol = get_symbol(
				feature_type_symbol_map, reconstruction_geometry);

		// Create a RenderedGeometry for drawing the reconstruction geometry.
		GPlatesViewOperations::RenderedGeometry rendered_geom =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						rotation ? rotation.get() * geometry : geometry,
						colour_proxy,
						render_params.reconstruction_point_size_hint * line_width_and_point_size_multiplier,
						render_params.reconstruction_line_width_hint * line_width_and_point_size_multiplier,
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


	// Threshold used when subdividing a topological network delaunay face to visualise 'smoothed' strain rates.
	// Natural neighbour tends to need more subdivision than barycentric.
	const double SUBDIVIDE_TOPOLOGICAL_NETWORK_DELAUNAY_BARYCENTRIC_SMOOTHED_ANGLE = GPlatesMaths::convert_deg_to_rad(0.5);
	const double SUBDIVIDE_TOPOLOGICAL_NETWORK_DELAUNAY_NATURAL_NEIGHBOUR_SMOOTHED_ANGLE = GPlatesMaths::convert_deg_to_rad(0.5);
}


GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams::RenderParams(
		const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters_,
		bool fill_polygons_,
		bool fill_polylines_) :
	reconstruction_line_width_hint(rendered_geometry_parameters_.get_reconstruction_layer_line_width_hint()),
	reconstruction_point_size_hint(rendered_geometry_parameters_.get_reconstruction_layer_point_size_hint()),
	reconstruction_topology_size_multiplier(rendered_geometry_parameters_.get_reconstruction_layer_topology_size_multiplier()),
	fill_polygons(fill_polygons_),
	fill_polylines(fill_polylines_),
	fill_modulate_colour(1, 1, 1, 1),
	ratio_zoom_dependent_bin_dimension_to_globe_radius(0),
	ratio_arrow_unit_vector_direction_to_globe_radius(
			rendered_geometry_parameters_.get_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius()),
	ratio_arrowhead_size_to_globe_radius(
			rendered_geometry_parameters_.get_reconstruction_layer_ratio_arrowhead_size_to_globe_radius()),
	scalar_coverage_colour_palette(GPlatesGui::RasterColourPalette::create()),
	raster_colour_palette(GPlatesGui::RasterColourPalette::create()),
	normal_map_height_field_scale_factor(1),
	show_vgp([] (double, boost::optional<double>) { return true; }),  // Return true by default to always show VGP.
	vgp_draw_circular_error(true),
	show_topology_reconstructed_feature_geometries(true),
	show_strain_accumulation(false),
	strain_accumulation_scale(1.0),
	fill_topological_network_rigid_blocks(false),
	show_topological_network_segment_velocity(false),
	topological_network_triangulation_colour_mode(TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DRAW_STYLE),
	topological_network_triangulation_draw_mode(TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_NONE)
{
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_raster_visual_layer_params(
		const RasterVisualLayerParams &params)
{
	d_render_params.raster_colour_palette = params.get_colour_palette_parameters().get_colour_palette();
	d_render_params.fill_modulate_colour = params.get_modulate_colour();
	d_render_params.normal_map_height_field_scale_factor = params.get_surface_relief_scale();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_reconstruct_scalar_coverage_visual_layer_params(
		const ReconstructScalarCoverageVisualLayerParams &params)
{
	d_render_params.scalar_coverage_colour_palette = params.get_current_colour_palette_parameters().get_colour_palette();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_reconstruct_visual_layer_params(
		const ReconstructVisualLayerParams &params)
{
	using namespace boost::placeholders;  // For _1, _2, etc

	d_render_params.show_vgp = boost::bind(&ReconstructVisualLayerParams::show_vgp, &params, _1, _2);
	d_render_params.vgp_draw_circular_error = params.get_vgp_draw_circular_error();
	d_render_params.fill_polygons = params.get_fill_polygons();
	d_render_params.fill_polylines = params.get_fill_polylines();
	d_render_params.fill_modulate_colour = params.get_fill_modulate_colour();
	d_render_params.show_topology_reconstructed_feature_geometries = params.get_show_topology_reconstructed_feature_geometries();
	d_render_params.show_strain_accumulation = params.get_show_strain_accumulation();
	d_render_params.strain_accumulation_scale = params.get_strain_accumulation_scale();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_scalar_field_3d_visual_layer_params(
		const ScalarField3DVisualLayerParams &params)
{
	d_render_params.scalar_field_render_parameters = params.get_scalar_field_3d_render_parameters();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_topology_geometry_visual_layer_params(
		const TopologyGeometryVisualLayerParams &params)
{
	d_render_params.fill_polygons = params.get_fill_polygons();
	d_render_params.fill_modulate_colour = params.get_fill_modulate_colour();
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator::visit_topology_network_visual_layer_params(
		const TopologyNetworkVisualLayerParams &params)
{
	d_render_params.show_topological_network_segment_velocity = params.show_segment_velocity();
	d_render_params.fill_topological_network_rigid_blocks = params.get_fill_rigid_blocks();
	d_render_params.fill_modulate_colour = params.get_fill_modulate_colour();
	d_render_params.topological_network_triangulation_colour_mode = params.get_triangulation_colour_mode();
	d_render_params.topological_network_triangulation_draw_mode = params.get_triangulation_draw_mode();

	switch (params.get_triangulation_colour_mode())
	{
	case TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE:
		d_render_params.topological_network_triangulation_colour_palette = params.get_dilatation_colour_palette();
		break;

	case TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE:
		d_render_params.topological_network_triangulation_colour_palette = params.get_second_invariant_colour_palette();
		break;

	case TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE:
		d_render_params.topological_network_triangulation_colour_palette = params.get_strain_rate_style_colour_palette();
		break;

	case TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DRAW_STYLE:
	default:
		// When using the draw style we don't use a colour palette.
		d_render_params.topological_network_triangulation_colour_palette = boost::none;
		break;
	}
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
		const GPlatesGui::RenderSettings &render_settings,
		const std::set<GPlatesModel::FeatureId> &topological_sections,
		const GPlatesAppLogic::TopologyUtils::resolved_topological_boundaries_networks_to_shared_sub_segments_map_type &all_resolved_topological_shared_sub_segments,
		const boost::optional<GPlatesGui::Colour> &colour,
		const boost::optional<GPlatesMaths::Rotation> &reconstruction_adjustment,
		boost::optional<const GPlatesGui::symbol_map_type &> feature_type_symbol_map,
		boost::optional<const GPlatesGui::StyleAdapter &> style_adaptor) :
	d_render_params(render_params),
	d_render_settings(render_settings),
	d_topological_sections(topological_sections),
	d_all_resolved_topological_shared_sub_segments(all_resolved_topological_shared_sub_segments),
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

	// Clear shared sub-segments of any resolved topological boundaries and networks
	// that were rendered in the previous rendered geometry layer.
	d_resolved_topological_shared_sub_segments_map.clear();

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

	// Render shared sub-segments of any resolved topological boundaries and networks
	// that were rendered in the current rendered geometry layer.
	render_topological_shared_sub_segments();

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

	// Return early if hiding velocity arrows.
	if (!d_render_settings.show_velocity_arrows())
	{
		return;
	}

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
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_tangential_arrow(
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
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_tangential_arrow(
								point,
								velocity.d_vector,
								d_render_params.ratio_arrow_unit_vector_direction_to_globe_radius,
								GPlatesGui::ColourProxy(rg_non_null_ptr),
								d_render_params.ratio_arrowhead_size_to_globe_radius);

				// Render the rendered geometry.
				render(rendered_arrow);
			} else {
				const GPlatesViewOperations::RenderedGeometry rendered_arrow =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_tangential_arrow(
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
		const GPlatesUtils::non_null_intrusive_ptr<topology_reconstructed_feature_geometry_type> &trfg)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type trfg_geometry = trfg->reconstructed_geometry();

	// Return early if hiding the geometry type of the (topology) reconstructed feature geometry.
	if (!d_render_settings.show_static_points() ||
		!d_render_settings.show_static_multipoints() ||
		!d_render_settings.show_static_lines() ||
		!d_render_settings.show_static_polygons())
	{
		const GPlatesMaths::GeometryType::Value trfg_geometry_type = GPlatesAppLogic::GeometryUtils::get_geometry_type(*trfg_geometry);
		if ((!d_render_settings.show_static_points() && (trfg_geometry_type == GPlatesMaths::GeometryType::POINT)) ||
			(!d_render_settings.show_static_multipoints() && (trfg_geometry_type == GPlatesMaths::GeometryType::MULTIPOINT)) ||
			(!d_render_settings.show_static_lines() && (trfg_geometry_type == GPlatesMaths::GeometryType::POLYLINE)) ||
			(!d_render_settings.show_static_polygons() && (trfg_geometry_type == GPlatesMaths::GeometryType::POLYGON)))
		{
			return;
		}
	}

	const GPlatesGui::ColourProxy dfg_colour_proxy =  get_colour(trfg, d_colour, d_style_adapter);

	if (d_render_params.show_topology_reconstructed_feature_geometries)
	{
		GPlatesViewOperations::RenderedGeometry rendered_geometry =
			create_rendered_reconstruction_geometry(
					trfg_geometry,
					trfg,
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
		typedef std::vector<GPlatesMaths::PointOnSphere> topology_reconstructed_geometry_points_seq_type;
		typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_total_strain_seq_type
				point_deformation_total_strain_seq_type;

		// Get the geometry point and deformation (total) strains.
		topology_reconstructed_geometry_points_seq_type geometry_points;
		point_deformation_total_strain_seq_type point_deformation_total_strains;
		trfg->get_geometry_data(
				geometry_points,
				boost::none/*geometry_point_locations*/,
				boost::none/*strain_rates*/,
				point_deformation_total_strains/*strains*/);

		// Iterate over the geometry points and generate a rendered geometry for each one.
		topology_reconstructed_geometry_points_seq_type::const_iterator geometry_points_iter = geometry_points.begin();
		topology_reconstructed_geometry_points_seq_type::const_iterator geometry_points_end = geometry_points.end();
		point_deformation_total_strain_seq_type::const_iterator point_deformation_total_strains_iter = point_deformation_total_strains.begin();
		point_deformation_total_strain_seq_type::const_iterator point_deformation_total_strains_end = point_deformation_total_strains.end();
		for ( ;
			geometry_points_iter != geometry_points_end &&
				point_deformation_total_strains_iter != point_deformation_total_strains_end;
			++geometry_points_iter, ++point_deformation_total_strains_iter)
		{
			const GPlatesMaths::PointOnSphere &geometry_point = *geometry_points_iter;
			const GPlatesAppLogic::DeformationStrain &point_deformation_total_strain = *point_deformation_total_strains_iter;

			const GPlatesMaths::PointOnSphere strain_marker_centre = d_reconstruction_adjustment
					? d_reconstruction_adjustment.get() * geometry_point
					: geometry_point;

			const GPlatesAppLogic::DeformationStrain::StrainPrincipal point_deformation_total_strain_principle =
					point_deformation_total_strain.get_strain_principal();

			// Create a RenderedGeometry.
			GPlatesViewOperations::RenderedGeometry strain_accumulation_rendered_geom =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_strain_marker_symbol(
							strain_marker_centre,
							1, // size
							// Scale the strain accumulation...
							d_render_params.strain_accumulation_scale * point_deformation_total_strain_principle.principal1, // scale_x
							d_render_params.strain_accumulation_scale * point_deformation_total_strain_principle.principal2, // scale_y
							point_deformation_total_strain_principle.angle); // orientation angle

			// Create a RenderedGeometry for storing the ReconstructionGeometry and
			// a RenderedGeometry associated with it.
			GPlatesViewOperations::RenderedGeometry strain_accumulation_rendered_reconstruction_geometry =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
							trfg,
							strain_accumulation_rendered_geom);

			// Render the rendered geometry.
			render(strain_accumulation_rendered_reconstruction_geometry);
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

	// If hiding topological sections then avoid rendering features referenced by topologies.
	if (!d_render_settings.show_topological_sections())
	{
		if (is_topological_section(rfg, d_topological_sections))
		{
			return;
		}
	}

	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type rfg_geometry = rfg->reconstructed_geometry();

	// Return early if hiding the geometry type of the reconstructed feature geometry.
	if (!d_render_settings.show_static_points() ||
		!d_render_settings.show_static_multipoints() ||
		!d_render_settings.show_static_lines() ||
		!d_render_settings.show_static_polygons())
	{
		const GPlatesMaths::GeometryType::Value rfg_geometry_type = GPlatesAppLogic::GeometryUtils::get_geometry_type(*rfg_geometry);
		if ((!d_render_settings.show_static_points() && (rfg_geometry_type == GPlatesMaths::GeometryType::POINT)) ||
			(!d_render_settings.show_static_multipoints() && (rfg_geometry_type == GPlatesMaths::GeometryType::MULTIPOINT)) ||
			(!d_render_settings.show_static_lines() && (rfg_geometry_type == GPlatesMaths::GeometryType::POLYLINE)) ||
			(!d_render_settings.show_static_polygons() && (rfg_geometry_type == GPlatesMaths::GeometryType::POLYGON)))
		{
			return;
		}
	}

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
		create_rendered_reconstruction_geometry(
				rfg_geometry,
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

	// Return early if hiding rasters.
	if (!d_render_settings.show_rasters())
	{
		return;
	}

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

	// Return early if hiding 3D scalar fields.
	if (!d_render_settings.show_3d_scalar_fields())
	{
		return;
	}

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

	// Return early if hiding static points.
	//
	// We consider a VGP as a point geometry (even though it also has a circular or elliptical error geometry).
	if (!d_render_settings.show_static_points())
	{
		return;
	}

	// See if should show the current VGP based on comparisons between its age and the current time.
	if (!d_render_params.show_vgp(rvgp->get_reconstruction_time(), rvgp->vgp_params().d_age))
	{
		return;
	}

	// The RVGP feature-properties-based colour.
	const GPlatesGui::ColourProxy rvgp_colour = get_colour(rvgp, d_colour, d_style_adapter);

	// Get (and render) the site point.
	boost::optional<GPlatesMaths::PointOnSphere> site_point;
	if (rvgp->vgp_params().d_site_point)
	{
		site_point = d_reconstruction_adjustment
				? d_reconstruction_adjustment.get() * rvgp->vgp_params().d_site_point.get()
				: rvgp->vgp_params().d_site_point.get();

		GPlatesViewOperations::RenderedGeometry rendered_site_point = 
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
					site_point.get(),
					rvgp_colour,
					d_render_params.reconstruction_point_size_hint);

		GPlatesViewOperations::RenderedGeometry site_point_rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
					rvgp,
					rendered_site_point);

		// Render the rendered geometry.
		render(site_point_rendered_geometry);
	}
	
	// Get (and render) the VGP (pole) point and its circle/ellipse.
	if (rvgp->vgp_params().d_vgp_point)
	{
		GPlatesViewOperations::RenderedGeometry rendered_vgp_point =
				create_rendered_reconstruction_geometry(
						rvgp->vgp_params().d_vgp_point->get_geometry_on_sphere(),
						rvgp,
						d_render_params,
						rvgp_colour,
						d_reconstruction_adjustment,
						d_feature_type_symbol_map);

		// The rendered geometry represents a geometry-on-sphere so render to the spatial partition.
		// In other words the VGP point is the reconstructed feature geometry (as opposed to the site point).
		render_reconstruction_geometry_on_sphere(rendered_vgp_point);

		const GPlatesMaths::PointOnSphere pole_point = d_reconstruction_adjustment
				? d_reconstruction_adjustment.get() * rvgp->vgp_params().d_vgp_point.get()
				: rvgp->vgp_params().d_vgp_point.get();

		if (d_render_params.vgp_draw_circular_error &&
			rvgp->vgp_params().d_a95 )
		{
			GPlatesViewOperations::RenderedGeometry rendered_small_circle = 
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_small_circle(
							GPlatesMaths::SmallCircle::create_colatitude(
									pole_point.position_vector(),
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
				site_point)
		{
			GPlatesMaths::GreatCircle great_circle(site_point.get(), pole_point);
				
			GPlatesViewOperations::RenderedGeometry rendered_ellipse = 
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_ellipse(
						pole_point,
						GPlatesMaths::convert_deg_to_rad(rvgp->vgp_params().d_dp.get()),
						GPlatesMaths::convert_deg_to_rad(rvgp->vgp_params().d_dm.get()),
						great_circle,
						rvgp_colour);

			// The circle/ellipse geometries are not (currently) queryable, so we
			// just add the rendered geometry to the layer.

			// Render the rendered geometry.
			render(rendered_ellipse);
		}
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

	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type rtg_geometry = rtg->resolved_topology_geometry();
	const GPlatesMaths::GeometryType::Value rtg_geometry_type = GPlatesAppLogic::GeometryUtils::get_geometry_type(*rtg_geometry);

	if (!d_render_settings.show_topological_sections() ||
		!d_render_settings.show_topological_lines() ||
		!d_render_settings.show_topological_polygons())
	{
		// If hiding topological sections then avoid rendering resolved topological lines referenced by topologies.
		if (!d_render_settings.show_topological_sections())
		{
			// Of resolved topological geometries, only resolved topological lines can be topological sections.
			if (rtg_geometry_type == GPlatesMaths::GeometryType::POLYLINE)
			{
				if (is_topological_section(rtg, d_topological_sections))
				{
					return;
				}
			}
		}

		// Return early if hiding the resolved geometry type of the resolved topological geometry.
		if ((!d_render_settings.show_topological_lines() && (rtg_geometry_type == GPlatesMaths::GeometryType::POLYLINE)) ||
			(!d_render_settings.show_topological_polygons() && (rtg_geometry_type == GPlatesMaths::GeometryType::POLYGON)))
		{
			return;
		}
	}

	// Resolved topological *polygons* are handled differently than resolved topological *lines*.
	if (rtg_geometry_type == GPlatesMaths::GeometryType::POLYGON)
	{
		// Add all shared boundary sub-segments of the resolved topological boundary.
		// Only resolved topological polygons will have boundary sub-segments (resolved topological lines do not).
		// These boundary sub-segments will get rendered at the end of the current rendered geometry layer (in 'end_render()').
		add_topological_shared_sub_segments(rtg, rtg->property());

		// Return early if NOT filling topological polygons.
		//
		// Note: The boundary segments of topological polygons are no longer rendered as a polygon *outline* (as they were in GPlates <= 2.4).
		//       Instead they're now rendered separately as sub-segments shared by topological polygons and networks in a separate post-layer render pass
		//       (that renders on top of the *filled* polygon, if drawn).
		if (!d_render_params.fill_polygons)
		{
			return;
		}
	}

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
		create_rendered_reconstruction_geometry(
				rtg_geometry, 
				rtg, 
				d_render_params, 
				get_colour(rtg, d_colour, d_style_adapter),
				d_reconstruction_adjustment,
				d_feature_type_symbol_map,
				// Topological boundary outlines and topological lines get rendered with a different thickness...
				d_render_params.reconstruction_topology_size_multiplier/*line_width_and_point_size_multiplier*/);

	// The rendered geometry represents a geometry-on-sphere so render to the spatial partition.
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	//PROFILE_BLOCK("ReconstructionGeometryRenderer::visit(resolved_topological_network_type)");

	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	// Return early if hiding topological networks.
	if (!d_render_settings.show_topological_networks())
	{
		return;
	}

	//
	// Render the network's triangulation.
	//
	if (d_render_params.topological_network_triangulation_colour_mode ==
		TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DRAW_STYLE)
	{
		if (d_render_params.topological_network_triangulation_draw_mode ==
			TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_MESH)
		{
			// Draw triangulation *edges* using draw style.
			render_topological_network_delaunay_edges_using_draw_style(rtn);
		}
		else if (d_render_params.topological_network_triangulation_draw_mode ==
			TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL)
		{
			// Draw triangulation *fill* using draw style.
			render_topological_network_fill_using_draw_style(rtn);
		}
	}
	else // not colouring by draw style (ie, colour by dilatation, second invariant strain rate, etc)...
	{
		// If we're using smoothed strain rates then each vertex of a face/edge will have a different colour.
		// In contrast, a non-smoothed strain rate is constant across each face/edge.
		const GPlatesAppLogic::TopologyNetworkParams::StrainRateSmoothing strain_rate_smoothing =
				rtn->get_triangulation_network().get_strain_rate_smoothing();
		if (strain_rate_smoothing == GPlatesAppLogic::TopologyNetworkParams::NO_SMOOTHING)
		{
			if (d_render_params.topological_network_triangulation_draw_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_MESH)
			{
				render_topological_network_delaunay_edges_unsmoothed_strain_rates(rtn);
			}
			else if (d_render_params.topological_network_triangulation_draw_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL)
			{
				render_topological_network_delaunay_faces_unsmoothed_strain_rates(rtn);
			}
		}
		else // smoothed strain rates...
		{
			const double subdivide_threshold_angle =
					(strain_rate_smoothing == GPlatesAppLogic::TopologyNetworkParams::NATURAL_NEIGHBOUR_SMOOTHING)
							? SUBDIVIDE_TOPOLOGICAL_NETWORK_DELAUNAY_NATURAL_NEIGHBOUR_SMOOTHED_ANGLE
							: SUBDIVIDE_TOPOLOGICAL_NETWORK_DELAUNAY_BARYCENTRIC_SMOOTHED_ANGLE;

			if (d_render_params.topological_network_triangulation_draw_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_MESH)
			{
				render_topological_network_delaunay_edges_smoothed_strain_rates(rtn, subdivide_threshold_angle);
			}
			else if (d_render_params.topological_network_triangulation_draw_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL)
			{
				render_topological_network_delaunay_faces_smoothed_strain_rates(rtn, std::cos(subdivide_threshold_angle));
			}
		}
	}

	// Add all shared boundary sub-segments of the resolved topological network.
	// These boundary sub-segments will get rendered at the end of the current rendered geometry layer (in 'end_render()').
	//
	// Note: The exterior boundary segments of topological networks are no longer rendered as a network outline (as they were in GPlates <= 2.4).
	//       Instead they're now rendered separately as sub-segments shared by topological polygons and networks in a separate post-layer render pass
	//       (that renders on top of the network triangulation, if drawn).
	add_topological_shared_sub_segments(rtn, rtn->property());

	// Render rigid interior blocks.
	//
	// This is the *interior* boundary equivalent of the *exterior* boundary.
	// Except we're rendering the *interior* boundary as part of this network
	// (whereas the *exterior* boundary is drawn in a separate render pass as mentioned above).
	//
	// Note: We now *always* render the interior blocks (whether filled or unfilled).
	//       Previously (in GPlates <= 2.4) we only rendered interior blocks when they were filled or the deforming region was coloured by draw style.
	//       This prevented the dilatation or second invariant strain rate colouring from being overwritten around the boundary of the interior blocks.
	//       However it's better to always render interior blocks since this is more consistent with the exterior boundary (that was, and is, *always* rendered).
	render_topological_network_rigid_blocks(rtn);

	// Check for drawing velocity vectors at vertices of delaunay triangulation.
	if (d_render_params.show_topological_network_segment_velocity)
	{
		// But only if showing velocity arrows in general.
		if (d_render_settings.show_velocity_arrows())
		{
			render_topological_network_velocities(rtn);
		}
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

	// Return early if hiding static points.
	//
	// We consider a flowline as a (seed) point geometry (even though the history of that point are two left/right lines).
	if (!d_render_settings.show_static_points())
	{
		return;
	}

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

	// Return early if hiding static points.
	//
	// We consider a motion path as a (seed) point geometry (even though the history of that point is a line).
	if (!d_render_settings.show_static_points())
	{
		return;
	}

	// Create a RenderedGeometry for drawing the reconstructed geometry.
	// Draw it in the specified colour (if specified) otherwise defer colouring to a later time
	// using ColourProxy.
	GPlatesViewOperations::RenderedGeometry rendered_geom =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_arrowed_polyline(
			rmp->motion_path_points(),
			get_colour(rmp, d_colour, d_style_adapter));

	GPlatesViewOperations::RenderedGeometry rendered_geometry = 
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rmp,
			rendered_geom);

	// Render the rendered geometry.
	render(rendered_geometry);

}

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_scalar_coverage_type> &rsc)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_rendered_geometry_layer,
			GPLATES_ASSERTION_SOURCE);

	// Return early if hiding scalar coverages.
	if (!d_render_settings.show_scalar_coverages())
	{
		return;
	}

	// Get the domain geometry.
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type domain_geometry = rsc->get_reconstructed_geometry();

	// Get the scalar values.
	GPlatesAppLogic::ReconstructedScalarCoverage::point_scalar_value_seq_type scalar_values;
	rsc->get_reconstructed_point_scalar_values(scalar_values);

	// We should have a real-valued colour palette.
	boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> scalar_colour_palette =
			GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(
					*d_render_params.scalar_coverage_colour_palette);
	if (!scalar_colour_palette)
	{
		return;
	}

	unsigned int num_points = scalar_values.size();

	// Convert the scalars to colours.
	std::vector<GPlatesGui::ColourProxy> point_colours;
	point_colours.reserve(num_points);
	for (unsigned int point_index = 0; point_index < num_points; ++point_index)
	{
		const double scalar = scalar_values[point_index];

		// Look up the scalar value in the colour palette.
		boost::optional<GPlatesGui::Colour> colour = scalar_colour_palette.get()->get_colour(scalar);
		point_colours.push_back(GPlatesGui::ColourProxy(colour));
	}

	boost::optional<GPlatesGui::Symbol> symbol = get_symbol(d_feature_type_symbol_map, rsc);

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_geometry_on_sphere(
					d_reconstruction_adjustment
							? d_reconstruction_adjustment.get() * domain_geometry
							: domain_geometry,
					point_colours,
					d_render_params.reconstruction_point_size_hint,
					d_render_params.reconstruction_line_width_hint,
					symbol);

	// Create a RenderedGeometry for storing the ReconstructionGeometry and a RenderedGeometry associated with it.
	rendered_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rsc,
			rendered_geometry);

	// The rendered geometry represents a geometry-on-sphere so render to the spatial partition.
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
        const GPlatesUtils::non_null_intrusive_ptr<reconstructed_small_circle_type> &rsc)
{
	// Must be between 'begin_render' and 'end_render'.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
		d_rendered_geometry_layer,
		GPLATES_ASSERTION_SOURCE);

	// Return early if hiding static points.
	//
	// We consider a small circle as a point geometry (defined by the centre of the small circle).
	if (!d_render_settings.show_static_points())
	{
		return;
	}

	// Create a RenderedGeometry for drawing the reconstructed geometry.
	// Draw it in the specified colour (if specified) otherwise defer colouring to a later time
	// using ColourProxy.

	GPlatesViewOperations::RenderedGeometry rendered_geom =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_small_circle(
			GPlatesMaths::SmallCircle::create_colatitude(
				rsc->centre().position_vector(), rsc->radius()),
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
PUSH_GCC_WARNINGS
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
POP_GCC_WARNINGS


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_delaunay_face_smoothed_strain_rate(
		const GPlatesMaths::PointOnSphere &point1,
		const GPlatesMaths::PointOnSphere &point2,
		const GPlatesMaths::PointOnSphere &point3,
		smoothed_vertex_indices_type &vertex_indices,
		GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::triangle_seq_type &rendered_triangle_mesh_triangles,
		const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network,
		GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle delaunay_face,
		const double &subdivide_face_threshold)
{
	const double dot12 = dot(point1.position_vector(), point2.position_vector()).dval();
	const double dot13 = dot(point1.position_vector(), point3.position_vector()).dval();
	const double dot23 = dot(point2.position_vector(), point3.position_vector()).dval();

	// If no need to further subdivide then add triangle and return.
	if (dot12 > subdivide_face_threshold &&
		dot13 > subdivide_face_threshold &&
		dot23 > subdivide_face_threshold)
	{
		// Add the triangle.
		const GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::Triangle rendered_mesh_triangle(
				vertex_indices.add_vertex(smoothed_vertex_type(point1, delaunay_face)),
				vertex_indices.add_vertex(smoothed_vertex_type(point2, delaunay_face)),
				vertex_indices.add_vertex(smoothed_vertex_type(point3, delaunay_face)));
		rendered_triangle_mesh_triangles.push_back(rendered_mesh_triangle);

		return;
	}

	// Subdivide into two triangles by splitting the longest edge of current triangle.
	if (dot12 < dot13)
	{
		if (dot12 < dot23)
		{
			const GPlatesMaths::PointOnSphere point12((GPlatesMaths::Vector3D(point1.position_vector()) +
					GPlatesMaths::Vector3D(point2.position_vector())).get_normalisation());

			render_topological_network_delaunay_face_smoothed_strain_rate(
					point1, point12, point3,
					vertex_indices, rendered_triangle_mesh_triangles,
					resolved_triangulation_network, delaunay_face, subdivide_face_threshold);
			render_topological_network_delaunay_face_smoothed_strain_rate(
					point12, point2, point3,
					vertex_indices, rendered_triangle_mesh_triangles,
					resolved_triangulation_network, delaunay_face, subdivide_face_threshold);
		}
		else
		{
			const GPlatesMaths::PointOnSphere point23((GPlatesMaths::Vector3D(point2.position_vector()) +
					GPlatesMaths::Vector3D(point3.position_vector())).get_normalisation());

			render_topological_network_delaunay_face_smoothed_strain_rate(
					point1, point2, point23,
					vertex_indices, rendered_triangle_mesh_triangles,
					resolved_triangulation_network, delaunay_face, subdivide_face_threshold);
			render_topological_network_delaunay_face_smoothed_strain_rate(
					point1, point23, point3,
					vertex_indices, rendered_triangle_mesh_triangles,
					resolved_triangulation_network, delaunay_face, subdivide_face_threshold);
		}
	}
	else
	{
		if (dot13 < dot23)
		{
			const GPlatesMaths::PointOnSphere point13((GPlatesMaths::Vector3D(point1.position_vector()) +
					GPlatesMaths::Vector3D(point3.position_vector())).get_normalisation());

			render_topological_network_delaunay_face_smoothed_strain_rate(
					point1, point2, point13,
					vertex_indices, rendered_triangle_mesh_triangles,
					resolved_triangulation_network, delaunay_face, subdivide_face_threshold);
			render_topological_network_delaunay_face_smoothed_strain_rate(
					point13, point2, point3,
					vertex_indices, rendered_triangle_mesh_triangles,
					resolved_triangulation_network, delaunay_face, subdivide_face_threshold);
		}
		else
		{
			const GPlatesMaths::PointOnSphere point23((GPlatesMaths::Vector3D(point2.position_vector()) +
					GPlatesMaths::Vector3D(point3.position_vector())).get_normalisation());

			render_topological_network_delaunay_face_smoothed_strain_rate(
					point1, point2, point23,
					vertex_indices, rendered_triangle_mesh_triangles,
					resolved_triangulation_network, delaunay_face, subdivide_face_threshold);
			render_topological_network_delaunay_face_smoothed_strain_rate(
					point1, point23, point3,
					vertex_indices, rendered_triangle_mesh_triangles,
					resolved_triangulation_network, delaunay_face, subdivide_face_threshold);
		}
	}
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_delaunay_faces_smoothed_strain_rates(
		const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn,
		const double &subdivide_face_threshold)
{
	const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network =
			rtn->get_triangulation_network();

	// NOTE: We avoid retrieving the *constrained* delaunay triangulation since we don't need it
	// and hence can improve performance by not generating it.
	const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &delaunay_triangulation_2 =
			resolved_triangulation_network.get_delaunay_2();

	//
	// Render mesh as faces/triangles.
	//

	// The triangle/vertices/colours of our rendered mesh (fill is turned on).
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::triangle_seq_type rendered_triangle_mesh_triangles;
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::vertex_seq_type rendered_triangle_mesh_vertices;
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::colour_seq_type rendered_triangle_mesh_colours;

	// Keep track of vertex indices of unique triangulation vertices referenced by the faces.
	smoothed_vertex_indices_type vertex_indices;

	// Iterate over the individual faces of the delaunay triangulation.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_faces_iterator
			finite_faces_2_iter = delaunay_triangulation_2.finite_faces_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_faces_iterator
			finite_faces_2_end = delaunay_triangulation_2.finite_faces_end();
	for ( ; finite_faces_2_iter != finite_faces_2_end; ++finite_faces_2_iter)
	{
		if (!finite_faces_2_iter->is_in_deforming_region())
		{
			// Face centroid is outside deforming region.
			continue;
		}

		render_topological_network_delaunay_face_smoothed_strain_rate(
				finite_faces_2_iter->vertex(0)->get_point_on_sphere(),
				finite_faces_2_iter->vertex(1)->get_point_on_sphere(),
				finite_faces_2_iter->vertex(2)->get_point_on_sphere(),
				vertex_indices,
				rendered_triangle_mesh_triangles,
				resolved_triangulation_network,
				finite_faces_2_iter,
				subdivide_face_threshold);
	}

	const smoothed_vertex_indices_type::vertex_seq_type &mesh_vertices = vertex_indices.get_vertices();
	rendered_triangle_mesh_vertices.reserve(mesh_vertices.size());
	rendered_triangle_mesh_colours.reserve(mesh_vertices.size());

	// Iterate over the smoothed vertices.
	smoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_iter = mesh_vertices.begin();
	smoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_end = mesh_vertices.end();
	for ( ; mesh_vertices_iter != mesh_vertices_end; ++mesh_vertices_iter)
	{
		const smoothed_vertex_type &vertex = *mesh_vertices_iter;
		const GPlatesMaths::PointOnSphere &point = vertex.first;
		GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle delaunay_face = vertex.second;

		boost::optional<GPlatesGui::ColourProxy> vertex_colour;

		if (d_colour) // Check if a colour was passed to the ReconstructionGeometryRenderer constructor.
		{
			vertex_colour = GPlatesGui::ColourProxy(d_colour.get());
		}
		else if (d_render_params.topological_network_triangulation_colour_palette)
		{
			// We've only generated vertices from tessellated triangles that are in the deforming region
			// so we can assume the tessellated vertices are in, or at least very close to, the deforming region.
			const GPlatesAppLogic::ResolvedTriangulation::DeformationInfo deformation_info =
					// The returned deformation will be barycentric or natural neighbour interpolated...
					resolved_triangulation_network.calculate_deformation_in_deforming_region(point, delaunay_face);

			if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE)
			{
				vertex_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_dilatation()));
			}
			else if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE)
			{
				vertex_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_second_invariant()));
			}
			else if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE)
			{
				vertex_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_style()));
			}
		}

		if (!vertex_colour)
		{
			// Should always get a valid vertex colour - if not then return without rendering mesh.
			return;
		}

		// Add the vertex colour.
		rendered_triangle_mesh_colours.push_back(vertex_colour.get());

		// Add the vertex position.
		rendered_triangle_mesh_vertices.push_back(point);
	}

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_triangle_surface_mesh(
			rendered_triangle_mesh_triangles,
			rendered_triangle_mesh_vertices,
			rendered_triangle_mesh_colours,
			true/*use_vertex_colouring*/,
			d_render_params.fill_modulate_colour);

	// Create a RenderedGeometry for storing the ReconstructionGeometry and a RenderedGeometry associated with it.
	rendered_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rtn,
			rendered_geometry);

	// The rendered geometry is the triangulation, which is on the sphere and within the bounds of the
	// resolved topological network, so we can render it to the spatial partition (to get view-frustum culling).
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_delaunay_faces_unsmoothed_strain_rates(
		const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn)
{
	const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network =
			rtn->get_triangulation_network();

	// NOTE: We avoid retrieving the *constrained* delaunay triangulation since we don't need it
	// and hence can improve performance by not generating it.
	const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &delaunay_triangulation_2 =
			resolved_triangulation_network.get_delaunay_2();

	//
	// Render mesh as faces/triangles.
	//

	// The triangle/vertices/colours of our rendered mesh (fill is turned on).
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::triangle_seq_type rendered_triangle_mesh_triangles;
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::vertex_seq_type rendered_triangle_mesh_vertices;
	GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::colour_seq_type rendered_triangle_mesh_colours;

	// Keep track of vertex indices of unique triangulation vertices referenced by the faces.
	unsmoothed_vertex_indices_type vertex_indices;

	// Iterate over the individual faces of the delaunay triangulation.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_faces_iterator
			finite_faces_2_iter = delaunay_triangulation_2.finite_faces_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_faces_iterator
			finite_faces_2_end = delaunay_triangulation_2.finite_faces_end();
	for ( ; finite_faces_2_iter != finite_faces_2_end; ++finite_faces_2_iter)
	{
		if (!finite_faces_2_iter->is_in_deforming_region())
		{
			// Face centroid is outside deforming region.
			continue;
		}

		boost::optional<GPlatesGui::ColourProxy> face_colour;

		if (d_colour) // Check if a colour was passed to the ReconstructionGeometryRenderer constructor.
		{
			face_colour = GPlatesGui::ColourProxy(d_colour.get());
		}
		else if (d_render_params.topological_network_triangulation_colour_palette)
		{
			const GPlatesAppLogic::ResolvedTriangulation::DeformationInfo &
					deformation_info = finite_faces_2_iter->get_deformation_info();

			if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE)
			{
				face_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_dilatation()));
			}
			else if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE)
			{
				face_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_second_invariant()));
			}
			else if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE)
			{
				face_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_style()));
			}
		}

		if (!face_colour)
		{
			// Should always get a valid face colour - if not then return without rendering mesh.
			return;
		}

		// Add the face colour.
		rendered_triangle_mesh_colours.push_back(face_colour.get());

		// Add the triangle.
		const GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::Triangle rendered_mesh_triangle(
				vertex_indices.add_vertex(finite_faces_2_iter->vertex(0)),
				vertex_indices.add_vertex(finite_faces_2_iter->vertex(1)),
				vertex_indices.add_vertex(finite_faces_2_iter->vertex(2)));
		rendered_triangle_mesh_triangles.push_back(rendered_mesh_triangle);
	}

	// For each triangulation vertex, referenced by the rendered mesh, get the un-projected
	// 2D vertex back onto the 3D globe.
	const unsmoothed_vertex_indices_type::vertex_seq_type &mesh_vertices = vertex_indices.get_vertices();

	rendered_triangle_mesh_vertices.reserve(mesh_vertices.size());

	// Iterate over the (referenced) triangulation vertices.
	unsmoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_iter = mesh_vertices.begin();
	unsmoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_end = mesh_vertices.end();
	for ( ; mesh_vertices_iter != mesh_vertices_end; ++mesh_vertices_iter)
	{
		// Get the un-projected 2D triangulation vertex position back onto the 3D sphere.
		GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle mesh_vertex = *mesh_vertices_iter;

		// Add the vertex.
		rendered_triangle_mesh_vertices.push_back(mesh_vertex->get_point_on_sphere());
	}

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_triangle_surface_mesh(
			rendered_triangle_mesh_triangles,
			rendered_triangle_mesh_vertices,
			rendered_triangle_mesh_colours,
			false/*use_vertex_colouring*/,
			d_render_params.fill_modulate_colour);

	// Create a RenderedGeometry for storing the ReconstructionGeometry and a RenderedGeometry associated with it.
	rendered_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rtn,
			rendered_geometry);

	// The rendered geometry is the triangulation, which is on the sphere and within the bounds of the
	// resolved topological network, so we can render it to the spatial partition (to get view-frustum culling).
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_delaunay_edges_smoothed_strain_rates(
		const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn,
		const double &subdivide_edge_threshold_angle)
{
	const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network =
			rtn->get_triangulation_network();

	// NOTE: We avoid retrieving the *constrained* delaunay triangulation since we don't need it
	// and hence can improve performance by not generating it.
	const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &delaunay_triangulation_2 =
			resolved_triangulation_network.get_delaunay_2();

	//
	// Render mesh as edges/lines.
	//

	// The edges/vertices of our rendered mesh (fill is turned off).
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::edge_seq_type rendered_edge_mesh_edges;
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::vertex_seq_type rendered_edge_mesh_vertices;
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::colour_seq_type rendered_edge_mesh_colours;

	// Keep track of vertex indices of unique triangulation vertices referenced by the edges.
	smoothed_vertex_indices_type vertex_indices;

	// Iterate over the edges of the delaunay triangulation.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_edges_iterator
			finite_edges_2_iter = delaunay_triangulation_2.finite_edges_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_edges_iterator
			finite_edges_2_end = delaunay_triangulation_2.finite_edges_end();
	for ( ; finite_edges_2_iter != finite_edges_2_end; ++finite_edges_2_iter)
	{
		// Get the two faces adjoining the current edge.
		const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle face_handle[2] =
		{
			finite_edges_2_iter->first,
			finite_edges_2_iter->first->neighbor(finite_edges_2_iter->second)
		};

		// Iterate over both faces adjoining the current edge.
		unsigned int num_valid_faces = 0;
		for (unsigned int t = 0; t < 2; ++t)
		{
			// Skip infinite faces. These occur on convex hull edges.
			// Also skip faces with centroid outside deforming region.
			if (delaunay_triangulation_2.is_infinite(face_handle[t]) ||
				!face_handle[t]->is_in_deforming_region())
			{
				continue;
			}

			++num_valid_faces;
		}

		// If both triangles adjoining current edge are outside deforming region or are
		// infinite faces for some reason then the skip the current edge.
		if (num_valid_faces == 0)
		{
			continue;
		}

		// Get the edge vertices.
		const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle edge_vertex_handles[2] =
		{
			finite_edges_2_iter->first->vertex(delaunay_triangulation_2.cw(finite_edges_2_iter->second)),
			finite_edges_2_iter->first->vertex(delaunay_triangulation_2.ccw(finite_edges_2_iter->second))
		};

		// Get the edge vertex positions.
		const GPlatesMaths::PointOnSphere edge_vertex_points[2] =
		{
			edge_vertex_handles[0]->get_point_on_sphere(),
			edge_vertex_handles[1]->get_point_on_sphere()
		};

		// Subdivide the edge (great circle arc).
		std::vector<GPlatesMaths::PointOnSphere> tessellation_points;
		tessellate(
				tessellation_points,
				GPlatesMaths::GreatCircleArc::create(edge_vertex_points[0], edge_vertex_points[1]),
				subdivide_edge_threshold_angle);

		// Iterate over the subdivided edges.
		const unsigned int num_subdivided_edges = tessellation_points.size() - 1;
		for (unsigned int e = 0; e < num_subdivided_edges; ++e)
		{
			// Add the edge.
			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge(
					// Pick either adjacent triangle - we just need a nearby face as an optimisation...
					vertex_indices.add_vertex(smoothed_vertex_type(tessellation_points[e], face_handle[0])),
					vertex_indices.add_vertex(smoothed_vertex_type(tessellation_points[e + 1], face_handle[0])));
			rendered_edge_mesh_edges.push_back(rendered_mesh_edge);
		}
	}

	const smoothed_vertex_indices_type::vertex_seq_type &mesh_vertices = vertex_indices.get_vertices();
	rendered_edge_mesh_vertices.reserve(mesh_vertices.size());
	rendered_edge_mesh_colours.reserve(mesh_vertices.size());

	// Iterate over the (referenced) triangulation vertices.
	smoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_iter = mesh_vertices.begin();
	smoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_end = mesh_vertices.end();
	for ( ; mesh_vertices_iter != mesh_vertices_end; ++mesh_vertices_iter)
	{
		const smoothed_vertex_type &vertex = *mesh_vertices_iter;
		const GPlatesMaths::PointOnSphere &point = vertex.first;
		GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle delaunay_face = vertex.second;

		boost::optional<GPlatesGui::ColourProxy> vertex_colour;

		if (d_colour) // Check if a colour was passed to the ReconstructionGeometryRenderer constructor.
		{
			vertex_colour = GPlatesGui::ColourProxy(d_colour.get());
		}
		else if (d_render_params.topological_network_triangulation_colour_palette)
		{
			// We've only generated vertices from tessellated edges that are in, or on the boundary of,
			// the deforming region so we can assume the tessellated vertices are in, or at least
			// very close to, the deforming region.
			const GPlatesAppLogic::ResolvedTriangulation::DeformationInfo deformation_info =
					// The returned deformation will be barycentric or natural neighbour interpolated...
					resolved_triangulation_network.calculate_deformation_in_deforming_region(point, delaunay_face);

			if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE)
			{
				vertex_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_dilatation()));
			}
			else if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE)
			{
				vertex_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_second_invariant()));
			}
			else if (d_render_params.topological_network_triangulation_colour_mode ==
				TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE)
			{
				vertex_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(deformation_info.get_strain_rate().get_strain_rate_style()));
			}
		}

		if (!vertex_colour)
		{
			// Should always get a valid vertex colour - if not then return without rendering mesh.
			return;
		}

		// Add the vertex colour.
		rendered_edge_mesh_colours.push_back(vertex_colour.get());

		// Add the vertex position.
		rendered_edge_mesh_vertices.push_back(point);
	}

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_edge_surface_mesh(
			rendered_edge_mesh_edges,
			rendered_edge_mesh_vertices,
			rendered_edge_mesh_colours,
			true/*use_vertex_colours*/,
			d_render_params.reconstruction_line_width_hint);

	// Create a RenderedGeometry for storing the ReconstructionGeometry and a RenderedGeometry associated with it.
	rendered_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rtn,
			rendered_geometry);

	// The rendered geometry is the triangulation, which is on the sphere and within the bounds of the
	// resolved topological network, so we can render it to the spatial partition (to get view-frustum culling).
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_delaunay_edges_unsmoothed_strain_rates(
		const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn)
{
	const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network =
			rtn->get_triangulation_network();

	// NOTE: We avoid retrieving the *constrained* delaunay triangulation since we don't need it
	// and hence can improve performance by not generating it.
	const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &delaunay_triangulation_2 =
			resolved_triangulation_network.get_delaunay_2();

	//
	// Render mesh as edges/lines.
	//

	// The edges/vertices of our rendered mesh (fill is turned off).
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::edge_seq_type rendered_edge_mesh_edges;
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::vertex_seq_type rendered_edge_mesh_vertices;
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::colour_seq_type rendered_edge_mesh_colours;

	// Keep track of vertex indices of unique triangulation vertices referenced by the edges.
	unsmoothed_vertex_indices_type vertex_indices;

	// Iterate over the edges of the delaunay triangulation.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_edges_iterator
			finite_edges_2_iter = delaunay_triangulation_2.finite_edges_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_edges_iterator
			finite_edges_2_end = delaunay_triangulation_2.finite_edges_end();
	for ( ; finite_edges_2_iter != finite_edges_2_end; ++finite_edges_2_iter)
	{
		// Get the two faces adjoining the current edge.
		const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle face_handle[2] =
		{
			finite_edges_2_iter->first,
			finite_edges_2_iter->first->neighbor(finite_edges_2_iter->second)
		};

		boost::optional<double> face_strain_rate[2];
		GPlatesMaths::Real face_area[2];
		unsigned int num_valid_faces = 0;

		// Iterate over both faces adjoining the current edge.
		for (unsigned int t = 0; t < 2; ++t)
		{
			// Skip infinite faces. These occur on convex hull edges.
			// Also skip faces with centroid outside deforming region.
			if (delaunay_triangulation_2.is_infinite(face_handle[t]) ||
				!face_handle[t]->is_in_deforming_region())
			{
				continue;
			}

			// Get the area of the face triangle.
			face_area[t] = std::fabs(CGAL::to_double(delaunay_triangulation_2.triangle(face_handle[t]).area()));

			if (!d_colour && // no override colour
				d_render_params.topological_network_triangulation_colour_palette)
			{
				const GPlatesAppLogic::ResolvedTriangulation::DeformationInfo &
						deformation_info = face_handle[t]->get_deformation_info();

				if (d_render_params.topological_network_triangulation_colour_mode ==
					TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE)
				{
					face_strain_rate[t] = deformation_info.get_strain_rate().get_strain_rate_dilatation();
				}
				else if (d_render_params.topological_network_triangulation_colour_mode ==
					TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE)
				{
					face_strain_rate[t] = deformation_info.get_strain_rate().get_strain_rate_second_invariant();
				}
				else if (d_render_params.topological_network_triangulation_colour_mode ==
					TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE)
				{
					face_strain_rate[t] = deformation_info.get_strain_rate().get_strain_rate_style();
				}
			}

			++num_valid_faces;
		}

		// If both triangles adjoining current edge are outside deforming region or are
		// infinite faces for some reason then the skip the current edge.
		if (num_valid_faces == 0)
		{
			continue;
		}

		boost::optional<GPlatesGui::ColourProxy> edge_colour;

		if (d_colour) // Check if a colour was passed to the ReconstructionGeometryRenderer constructor.
		{
			edge_colour = GPlatesGui::ColourProxy(d_colour.get());
		}
		else if (face_strain_rate[0] || face_strain_rate[1])
		{
			// If both triangles share the current edge then average their strain rates (weighted by their areas).
			if (face_strain_rate[0] && face_strain_rate[1])
			{
				const double total_area = face_area[0].dval() + face_area[1].dval();
				if (total_area > 0)
				{
					const double edge_strain_rate = (face_area[0].dval() * face_strain_rate[0].get() +
							face_area[1].dval() * face_strain_rate[1].get()) / total_area;

					edge_colour = GPlatesGui::ColourProxy(
							d_render_params.topological_network_triangulation_colour_palette.get()
									->get_colour(edge_strain_rate));
				}
			}
			else // only one triangle adjacent to edge ...
			{
				edge_colour = GPlatesGui::ColourProxy(
						d_render_params.topological_network_triangulation_colour_palette.get()
								->get_colour(face_strain_rate[0] ? face_strain_rate[0].get() : face_strain_rate[1].get()));
			}
		}

		if (edge_colour)
		{
			// Get the edge vertices.
			const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle edge_vertex_handles[2] =
			{
				finite_edges_2_iter->first->vertex(delaunay_triangulation_2.cw(finite_edges_2_iter->second)),
				finite_edges_2_iter->first->vertex(delaunay_triangulation_2.ccw(finite_edges_2_iter->second))
			};

			// Add the edge colour.
			rendered_edge_mesh_colours.push_back(edge_colour.get());

			// Add the edge.
			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge(
					vertex_indices.add_vertex(edge_vertex_handles[0]),
					vertex_indices.add_vertex(edge_vertex_handles[1]));
			rendered_edge_mesh_edges.push_back(rendered_mesh_edge);
		}
	}

	// For each triangulation vertex, referenced by the rendered mesh, get the un-projected
	// 2D vertex back onto the 3D globe.
	const unsmoothed_vertex_indices_type::vertex_seq_type &mesh_vertices = vertex_indices.get_vertices();

	rendered_edge_mesh_vertices.reserve(mesh_vertices.size());

	// Iterate over the (referenced) triangulation vertices.
	unsmoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_iter = mesh_vertices.begin();
	unsmoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_end = mesh_vertices.end();
	for ( ; mesh_vertices_iter != mesh_vertices_end; ++mesh_vertices_iter)
	{
		// Get the un-projected 2D triangulation vertex position back onto the 3D sphere.
		GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle mesh_vertex = *mesh_vertices_iter;
		rendered_edge_mesh_vertices.push_back(mesh_vertex->get_point_on_sphere());
	}

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_edge_surface_mesh(
			rendered_edge_mesh_edges,
			rendered_edge_mesh_vertices,
			rendered_edge_mesh_colours,
			false/*use_vertex_colours*/,
			d_render_params.reconstruction_line_width_hint);

	// Create a RenderedGeometry for storing the ReconstructionGeometry and a RenderedGeometry associated with it.
	rendered_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rtn,
			rendered_geometry);

	// The rendered geometry is the triangulation, which is on the sphere and within the bounds of the
	// resolved topological network, so we can render it to the spatial partition (to get view-frustum culling).
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_delaunay_edges_using_draw_style(
		const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn)
{
	// The entire mesh has the same colour (determined by draw style).
	const GPlatesGui::ColourProxy colour = get_colour(rtn, d_colour, d_style_adapter);

	const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network =
			rtn->get_triangulation_network();

	// NOTE: We avoid retrieving the *constrained* delaunay triangulation since we don't need it
	// and hence can improve performance by not generating it.
	const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2 &delaunay_triangulation_2 =
			resolved_triangulation_network.get_delaunay_2();

	//
	// Render mesh as edges/lines.
	//

	// The edges/vertices of our rendered mesh (fill is turned off).
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::edge_seq_type rendered_edge_mesh_edges;
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::vertex_seq_type rendered_edge_mesh_vertices;
	GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::colour_seq_type rendered_edge_mesh_colours;

	// Keep track of vertex indices of unique triangulation vertices referenced by the edges.
	unsmoothed_vertex_indices_type vertex_indices;

	// Iterate over the edges of the delaunay triangulation.
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_edges_iterator
			finite_edges_2_iter = delaunay_triangulation_2.finite_edges_begin();
	GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Finite_edges_iterator
			finite_edges_2_end = delaunay_triangulation_2.finite_edges_end();
	for ( ; finite_edges_2_iter != finite_edges_2_end; ++finite_edges_2_iter)
	{
		// Get the two faces adjoining the current edge.
		const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle face_handle[2] =
		{
			finite_edges_2_iter->first,
			finite_edges_2_iter->first->neighbor(finite_edges_2_iter->second)
		};

		bool valid_edge = false;

		// Iterate over both faces adjoining the current edge.
		for (unsigned int t = 0; t < 2; ++t)
		{
			// Skip infinite faces. These occur on convex hull edges.
			// Also skip faces with centroid outside deforming region.
			if (!delaunay_triangulation_2.is_infinite(face_handle[t]) &&
				face_handle[t]->is_in_deforming_region())
			{
				valid_edge = true;
				break;
			}
		}

		// If both triangles adjoining current edge are outside deforming region or are
		// infinite faces for some reason then the skip the current edge.
		if (!valid_edge)
		{
			continue;
		}

		// Get the edge vertices.
		const GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle edge_vertex_handles[2] =
		{
			finite_edges_2_iter->first->vertex(delaunay_triangulation_2.cw(finite_edges_2_iter->second)),
			finite_edges_2_iter->first->vertex(delaunay_triangulation_2.ccw(finite_edges_2_iter->second))
		};

		// Add the edge colour.
		rendered_edge_mesh_colours.push_back(colour);

		// Add the edge.
		const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh::Edge rendered_mesh_edge(
				vertex_indices.add_vertex(edge_vertex_handles[0]),
				vertex_indices.add_vertex(edge_vertex_handles[1]));
		rendered_edge_mesh_edges.push_back(rendered_mesh_edge);
	}

	// For each triangulation vertex, referenced by the rendered mesh, get the un-projected
	// 2D vertex back onto the 3D globe.
	const unsmoothed_vertex_indices_type::vertex_seq_type &mesh_vertices = vertex_indices.get_vertices();

	rendered_edge_mesh_vertices.reserve(mesh_vertices.size());

	// Iterate over the (referenced) triangulation vertices.
	unsmoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_iter = mesh_vertices.begin();
	unsmoothed_vertex_indices_type::vertex_seq_type::const_iterator mesh_vertices_end = mesh_vertices.end();
	for ( ; mesh_vertices_iter != mesh_vertices_end; ++mesh_vertices_iter)
	{
		// Get the un-projected 2D triangulation vertex position back onto the 3D sphere.
		GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle mesh_vertex = *mesh_vertices_iter;
		rendered_edge_mesh_vertices.push_back(mesh_vertex->get_point_on_sphere());
	}

	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_edge_surface_mesh(
			rendered_edge_mesh_edges,
			rendered_edge_mesh_vertices,
			rendered_edge_mesh_colours,
			false/*use_vertex_colours*/,
			d_render_params.reconstruction_line_width_hint);

	// Create a RenderedGeometry for storing the ReconstructionGeometry and a RenderedGeometry associated with it.
	rendered_geometry = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rtn,
			rendered_geometry);

	// The rendered geometry is the triangulation, which is on the sphere and within the bounds of the
	// resolved topological network, so we can render it to the spatial partition (to get view-frustum culling).
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_fill_using_draw_style(
		const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn)
{
	// Make a copy of the render params, but with fill polygons turned on since our network boundary is a polygon.
	RenderParams render_params(d_render_params);
	render_params.fill_polygons = true;

	// Use the network boundary polygon with rigid block interior holes since the triangulation
	// (ie, deforming region) is filled and we don't want to fill the interior holes (rigid blocks).
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type network_boundary_with_rigid_block_holes =
			rtn->get_triangulation_network().get_boundary_polygon_with_rigid_block_holes();

	// Create a RenderedGeometry for drawing the network boundary polygon.
	GPlatesViewOperations::RenderedGeometry rendered_geometry =
		create_rendered_reconstruction_geometry(
				network_boundary_with_rigid_block_holes,
				rtn,
				render_params,
				get_colour(rtn, d_colour, d_style_adapter),
				d_reconstruction_adjustment,
				d_feature_type_symbol_map,
				d_render_params.reconstruction_topology_size_multiplier/*line_width_and_point_size_multiplier*/);

	// The rendered geometry is the network boundary, which is on the sphere and is the bounds of the
	// resolved topological network, so we can render it to the spatial partition (to get view-frustum culling).
	render_reconstruction_geometry_on_sphere(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_rigid_blocks(
		const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn)
{
	RenderParams render_params(d_render_params);
	if (d_render_params.fill_topological_network_rigid_blocks)
	{
		// Make a copy of the render params, but with fill polygons turned on since our rigid blocks will be polygons.
		render_params.fill_polygons = true;
	}

	// Iterate over the interior rigid blocks, if any, of the current topological network.
	const GPlatesAppLogic::ResolvedTriangulation::Network::rigid_block_seq_type &rigid_blocks =
			rtn->get_triangulation_network().get_rigid_blocks();
	GPlatesAppLogic::ResolvedTriangulation::Network::rigid_block_seq_type::const_iterator rigid_blocks_iter = rigid_blocks.begin();
	GPlatesAppLogic::ResolvedTriangulation::Network::rigid_block_seq_type::const_iterator rigid_blocks_end = rigid_blocks.end();
	for ( ; rigid_blocks_iter != rigid_blocks_end; ++rigid_blocks_iter)
	{
		const GPlatesAppLogic::ResolvedTriangulation::Network::RigidBlock &rigid_block = *rigid_blocks_iter;

		const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type rigid_block_rfg =
				rigid_block.get_reconstructed_feature_geometry();

		GPlatesViewOperations::RenderedGeometry rendered_geometry =
			create_rendered_reconstruction_geometry(
					rigid_block_rfg->reconstructed_geometry(),
					// Use the resolved topological network (instead of rigid block RFG) so when the user
					// clicks on this rigid block rendered geometry then the resolved network shows up.
					// Also means the RFG export won't output rigid blocks when a resolved network
					// layer is visible (will only export if the actual RFG layer containing the
					// rigid blocks is visible)...
					rtn,
					render_params,
					get_colour(rigid_block_rfg, d_colour, d_style_adapter),
					d_reconstruction_adjustment,
					d_feature_type_symbol_map,
					// Topological network boundaries get rendered with a different thickness...
					d_render_params.reconstruction_topology_size_multiplier/*line_width_and_point_size_multiplier*/);

		// The rendered geometry is a rigid interior block, which is on the sphere and within the bounds of the
		// resolved topological network, so we can render it to the spatial partition (to get view-frustum culling).
		render_reconstruction_geometry_on_sphere(rendered_geometry);
	}
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_network_velocities(
		const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn)
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
		const GPlatesMaths::Vector3D velocity_vector = finite_vertices_2_iter->calc_velocity_vector();

		// Create a RenderedGeometry using the velocity vector.
		const GPlatesViewOperations::RenderedGeometry rendered_vector =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_tangential_arrow(
				point,
				velocity_vector,
				d_render_params.ratio_arrow_unit_vector_direction_to_globe_radius,
				velocity_colour,
				d_render_params.ratio_arrowhead_size_to_globe_radius);

		// Create a RenderedGeometry for storing the ReconstructionGeometry and
		// a RenderedGeometry associated with it.
		//
		// This means the resolved topological network can be selected by clicking one of
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


void
GPlatesPresentation::ReconstructionGeometryRenderer::add_topological_shared_sub_segments(
		const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &resolved_topology,
		const GPlatesModel::FeatureHandle::iterator &resolved_topology_feature_property)
{
	// Find the shared sub-segments associated with the specified resolved topology.
	auto shared_sub_segment_infos_iter = d_all_resolved_topological_shared_sub_segments.find(resolved_topology_feature_property);
	if (shared_sub_segment_infos_iter == d_all_resolved_topological_shared_sub_segments.end())
	{
		// Resolved topology not found in the map.
		// This shouldn't happen if the resolved topology is a resolved topological polygon or network.
		// If it happens then we'll just ignore the resolved topology.
		return;
	}

	// Add the shared sub-segments associated with the specified resolved topology.
	// These will get rendered later (at the end of the current rendered geometry layer).
	for (const auto &shared_sub_segment_info : shared_sub_segment_infos_iter->second)
	{
		const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::non_null_ptr_type shared_sub_segment = shared_sub_segment_info.first;

		d_resolved_topological_shared_sub_segments_map[shared_sub_segment].push_back(resolved_topology);
	}
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::render_topological_shared_sub_segments()
{
	for (const auto &shared_sub_segment_map_entry : d_resolved_topological_shared_sub_segments_map)
	{
		const GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::non_null_ptr_type shared_sub_segment =
				shared_sub_segment_map_entry.first;
		const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> &resolved_topologies =
				shared_sub_segment_map_entry.second;

		// Shared sub-segment polyline.
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type shared_sub_segment_polyline =
				shared_sub_segment->get_shared_sub_segment_geometry();
		if (d_reconstruction_adjustment)
		{
			shared_sub_segment_polyline = d_reconstruction_adjustment.get() * shared_sub_segment_polyline;
		}

		// Shared sub-segment reconstruction geometry.
		//
		// Note: This is the full reconstruction geometry (not just the shared sub-segment part of it).
		const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type shared_sub_segment_reconstruction_geometry =
				shared_sub_segment->get_reconstruction_geometry();

		// The colour is determined by the shared sub-segment (ie, the topological section feature it came from).
		const GPlatesGui::ColourProxy shared_sub_segment_colour =
				get_colour(shared_sub_segment_reconstruction_geometry, d_colour, d_style_adapter);

		const boost::optional<SubductionPolarity> subduction_polarity =
				get_subduction_polarity(shared_sub_segment_reconstruction_geometry);

		// Create a RenderedGeometry for drawing the shared sub-segment.
		GPlatesViewOperations::RenderedGeometry shared_sub_segment_rendered_geom;
		if (subduction_polarity)
		{
			// Create a polyline with subduction teeth.
			shared_sub_segment_rendered_geom = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_subduction_teeth_polyline(
					shared_sub_segment_polyline,
					subduction_polarity.get() == SubductionPolarity::LEFT,  // subduction_polarity_is_left
					shared_sub_segment_colour,
					// Topological plate/network boundaries get rendered with a different thickness...
					d_render_params.reconstruction_line_width_hint * d_render_params.reconstruction_topology_size_multiplier);
		}
		else
		{
			// Create an ordinary polyline.
			shared_sub_segment_rendered_geom = GPlatesViewOperations::RenderedGeometryFactory::create_rendered_polyline_on_sphere(
					shared_sub_segment_polyline,
					shared_sub_segment_colour,
					// Topological plate/network boundaries get rendered with a different thickness...
					d_render_params.reconstruction_line_width_hint * d_render_params.reconstruction_topology_size_multiplier,
					d_render_params.fill_polylines,
					d_render_params.fill_modulate_colour);
		}

		// Create a RenderedGeometry for storing the sharing resolved topologies (ReconstructionGeometry's) and
		// a RenderedGeometry associated with them (for the shared sub-segment geometry).
		//
		// By storing the resolved topologies (that share the shared sub-segment) instead of the reconstruction geometry
		// of the shared sub-segment, when the user clicks on the shared sub-segment the resolved topologies will
		// get added to the selected features list (which is what we want). The user can still click on the
		// (topological section) feature of the shared sub-segment (provided visibility of topological sections is enabled),
		// in which case the full topological section feature will get highlighted (including the dangling bits at the ends,
		// if it's a line, and not just the sub-segments that actually contribute to the resolved topologies).
		const GPlatesViewOperations::RenderedGeometry rendered_multi_reconstruction_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_multi_reconstruction_geometry(
						resolved_topologies,
						shared_sub_segment_rendered_geom);

		// Render the rendered geometry.
		//
		// Note: We don't render using 'render_reconstruction_geometry_on_sphere()' since that requires
		//       that the rendered geometry match the reconstruction geometry. In our case this is not
		//       true since the rendered geometry is the shared sub-segment and there are multiple
		//       reconstruction geometries corresponding to the sharing resolved topologies.
		render(rendered_multi_reconstruction_geometry);
	}
}


boost::optional<GPlatesPresentation::ReconstructionGeometryRenderer::SubductionPolarity>
GPlatesPresentation::ReconstructionGeometryRenderer::get_subduction_polarity(
		const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &resolved_topological_section) const
{
	// Get the feature.
	boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(resolved_topological_section);
	if (feature_ref)
	{
		// See if feature is a subduction zone.
		static const GPlatesModel::FeatureType subduction_zone_type = GPlatesModel::FeatureType::create_gpml("SubductionZone");
		if (feature_ref.get()->feature_type() == subduction_zone_type)
		{
			// See if has a 'gpml:subductionPolarity' property.
			static const GPlatesModel::PropertyName subduction_polarity_property_name = GPlatesModel::PropertyName::create_gpml("subductionPolarity");
			boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> subduction_polarity_property_value =
					GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::Enumeration>(feature_ref.get(), subduction_polarity_property_name);
			if (subduction_polarity_property_value)
			{
				// See if property is a 'gpml:SubductionPolarityEnumeration' enumeration.
				static const GPlatesPropertyValues::EnumerationType subduction_polarity_enumeration_type =
						GPlatesPropertyValues::EnumerationType::create_gpml("SubductionPolarityEnumeration");
				if (subduction_polarity_enumeration_type.is_equal_to(subduction_polarity_property_value.get()->type()))
				{
					// See if polarity is 'Left' or 'Right'.
					static const GPlatesPropertyValues::EnumerationContent subduction_polarity_enumeration_value_left("Left");
					static const GPlatesPropertyValues::EnumerationContent subduction_polarity_enumeration_value_right("Right");
					if (subduction_polarity_enumeration_value_left.is_equal_to(subduction_polarity_property_value.get()->value()))
					{
						return SubductionPolarity::LEFT;
					}
					if (subduction_polarity_enumeration_value_right.is_equal_to(subduction_polarity_property_value.get()->value()))
					{
						return SubductionPolarity::RIGHT;
					}
				}
			}
		}
	}

	return boost::none;
}

// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the file because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")
