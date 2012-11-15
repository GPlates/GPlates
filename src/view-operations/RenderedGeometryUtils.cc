/* $Id$ */

/**
 * \file 
 * Various helper functions, classes that use @a RenderedGeometryCollection.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "app-logic/ReconstructionGeometryUtils.h"

#include "RenderedGeometryUtils.h"

#include "RenderedReconstructionGeometry.h"
#include "RenderedGeometryVisitor.h"


#include <boost/foreach.hpp>
namespace GPlatesViewOperations
{
	namespace RenderedGeometryUtils
	{
		namespace
		{
			/**
			 * Increments count for every non-empty @a RenderedGeometryLayer.
			 */
			class CountNonEmptyRenderedGeometries
			{
			public:
				CountNonEmptyRenderedGeometries() :
				d_count(0)
				{  }

				void
				operator()(
						const RenderedGeometryLayer &rendered_geom_layer)
				{
					if (rendered_geom_layer.is_active() && !rendered_geom_layer.is_empty())
					{
						++d_count;
					}
				}

				unsigned int
				get_count() const
				{
					return d_count;
				}

			private:
				unsigned int d_count;
			};


			/**
			 * Retrieves any @a ReconstructionGeometry objects from @a RenderedGeometryLayer.
			 */
			class CollectReconstructionGeometries :
				public ConstRenderedGeometryVisitor
			{
			public:
				CollectReconstructionGeometries(
						reconstruction_geom_seq_type &reconstruction_geom_seq) :
					d_reconstruction_geom_seq(reconstruction_geom_seq)
				{  }

				void
				operator()(
						const RenderedGeometryLayer &rendered_geom_layer)
				{
					if (!rendered_geom_layer.is_active())
					{
						return;
					}

					// Visit each RenderedGeometry in the layer to collect its
					// ReconstructionGeometry if it has one.
					std::for_each(
							rendered_geom_layer.rendered_geometry_begin(),
							rendered_geom_layer.rendered_geometry_end(),
							boost::bind(&RenderedGeometry::accept_visitor, _1, boost::ref(*this)));
				}


				virtual
				void
				visit_rendered_reconstruction_geometry(
						const RenderedReconstructionGeometry &rendered_recon_geom)
				{
					d_reconstruction_geom_seq.push_back(
							rendered_recon_geom.get_reconstruction_geometry());
				}


			private:
				reconstruction_geom_seq_type &d_reconstruction_geom_seq;
			};


			/**
			 * Removes duplicate @a ReconstructionGeometry objects from an unsorted sequence.
			 *
			 * NOTE: This keeps the original sort order which is important if the (rendered) geometries
			 * are sorted by mouse click proximity - we don't want to lose that sort order.
			 */
			void
			remove_duplicates(
					reconstruction_geom_seq_type &reconstruction_geom_seq)
			{
				// Instead of using std::sort, std::unique and erase we keep the reconstruction geometry
				// sequence in its original order.
				const reconstruction_geom_seq_type original_reconstruction_geom_seq(reconstruction_geom_seq);
				reconstruction_geom_seq.clear();

				BOOST_FOREACH(
					const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom,
					original_reconstruction_geom_seq)
				{
					// Add reconstruction geometry is it isn't already in the sequence.
					if (std::find(reconstruction_geom_seq.begin(), reconstruction_geom_seq.end(), recon_geom)
						== reconstruction_geom_seq.end())
					{
						reconstruction_geom_seq.push_back(recon_geom);
					}
				}
			}
		}
	}
}

unsigned int
GPlatesViewOperations::RenderedGeometryUtils::get_num_active_non_empty_layers(
		const RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		bool only_if_main_layer_active)
{
	RenderedGeometryCollection::main_layers_update_type main_layers;
	main_layers.set(main_layer_type);

	return get_num_active_non_empty_layers(
			rendered_geom_collection, main_layers, only_if_main_layer_active);
}

unsigned int
GPlatesViewOperations::RenderedGeometryUtils::get_num_active_non_empty_layers(
		const RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active)
{
	CountNonEmptyRenderedGeometries count;

	ConstVisitFunctionOnRenderedGeometryLayers get_count(
			boost::ref(count),
			main_layers,
			only_if_main_layer_active);

	get_count.call_function(rendered_geom_collection);

	return count.get_count();
}

void
GPlatesViewOperations::RenderedGeometryUtils::activate_rendered_geometry_layers(
		RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		bool only_if_main_layer_active)
{
	RenderedGeometryCollection::main_layers_update_type main_layers;
	main_layers.set(main_layer_type);

	activate_rendered_geometry_layers(
			rendered_geom_collection, main_layers, only_if_main_layer_active);
}

void
GPlatesViewOperations::RenderedGeometryUtils::activate_rendered_geometry_layers(
		RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active)
{
	VisitFunctionOnRenderedGeometryLayers activate(
			boost::bind(&RenderedGeometryLayer::set_active, _1, true),
			main_layers,
			only_if_main_layer_active);

	activate.call_function(rendered_geom_collection);
}

void
GPlatesViewOperations::RenderedGeometryUtils::deactivate_rendered_geometry_layers(
		RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		bool only_if_main_layer_active)
{
	RenderedGeometryCollection::main_layers_update_type main_layers;
	main_layers.set(main_layer_type);

	deactivate_rendered_geometry_layers(
			rendered_geom_collection, main_layers, only_if_main_layer_active);
}

void
GPlatesViewOperations::RenderedGeometryUtils::deactivate_rendered_geometry_layers(
		RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active)
{
	VisitFunctionOnRenderedGeometryLayers deactivate(
			boost::bind(&RenderedGeometryLayer::set_active, _1, false),
			main_layers,
			only_if_main_layer_active);

	deactivate.call_function(rendered_geom_collection);
}


bool
GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq_type &reconstruction_geom_seq,
		const RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		bool only_if_main_layer_active)
{
	RenderedGeometryCollection::main_layers_update_type main_layers;
	main_layers.set(main_layer_type);

	return get_unique_reconstruction_geometries(
			reconstruction_geom_seq, rendered_geom_collection,
			main_layers, only_if_main_layer_active);
}


bool
GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq_type &reconstruction_geom_seq,
		const RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active)
{
	CollectReconstructionGeometries collect_recon_geoms(reconstruction_geom_seq);

	ConstVisitFunctionOnRenderedGeometryLayers collect_recon_geoms_visitor(
			boost::ref(collect_recon_geoms),
			main_layers,
			only_if_main_layer_active);

	collect_recon_geoms_visitor.call_function(rendered_geom_collection);

	// Remove any duplicate reconstruction geometries.
	remove_duplicates(reconstruction_geom_seq);

	return !reconstruction_geom_seq.empty();
}


bool
GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq_type &reconstruction_geom_seq,
		const GPlatesViewOperations::sorted_rendered_geometry_proximity_hits_type &
				sorted_rendered_geometry_hits)
{
	CollectReconstructionGeometries collect_recon_geoms(reconstruction_geom_seq);

	GPlatesViewOperations::sorted_rendered_geometry_proximity_hits_type::const_iterator sorted_iter;
	for (sorted_iter = sorted_rendered_geometry_hits.begin();
		sorted_iter != sorted_rendered_geometry_hits.end();
		++sorted_iter)
	{
		GPlatesViewOperations::RenderedGeometry rendered_geom =
			sorted_iter->d_rendered_geom_layer->get_rendered_geometry(
			sorted_iter->d_rendered_geom_index);

		// If rendered geometry contains a reconstruction geometry then it'll be added
		// 'reconstruction_geom_seq'.
		rendered_geom.accept_visitor(collect_recon_geoms);
	}

	// Remove any duplicate reconstruction geometries.
	remove_duplicates(reconstruction_geom_seq);

	return !reconstruction_geom_seq.empty();
}


bool
GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry,
		boost::optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree,
		bool only_if_reconstruction_layer_active)
{
	// Get all reconstruction geometries from the rendered geometry collection RECONSTRUCTION layer.
	reconstruction_geom_seq_type all_reconstruction_geoms_in_reconstruction_layer;
	if (!get_unique_reconstruction_geometries(
			all_reconstruction_geoms_in_reconstruction_layer,
			rendered_geom_collection,
			RenderedGeometryCollection::RECONSTRUCTION_LAYER,
			only_if_reconstruction_layer_active))
	{
		return false;
	}

	return GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
			reconstruction_geometries_observing_feature,
			all_reconstruction_geoms_in_reconstruction_layer,
			reconstruction_geometry,
			reconstruction_tree);
}


bool
GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		boost::optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree,
		bool only_if_reconstruction_layer_active)
{
	// Get all reconstruction geometries from the rendered geometry collection RECONSTRUCTION layer.
	reconstruction_geom_seq_type all_reconstruction_geoms_in_reconstruction_layer;
	if (!get_unique_reconstruction_geometries(
			all_reconstruction_geoms_in_reconstruction_layer,
			rendered_geom_collection,
			RenderedGeometryCollection::RECONSTRUCTION_LAYER,
			only_if_reconstruction_layer_active))
	{
		return false;
	}

	return GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
			reconstruction_geometries_observing_feature,
			all_reconstruction_geoms_in_reconstruction_layer,
			feature_ref,
			reconstruction_tree);
}


bool
GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator,
		boost::optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree,
		bool only_if_reconstruction_layer_active)
{
	// Get all reconstruction geometries from the rendered geometry collection RECONSTRUCTION layer.
	reconstruction_geom_seq_type all_reconstruction_geoms_in_reconstruction_layer;
	if (!get_unique_reconstruction_geometries(
			all_reconstruction_geoms_in_reconstruction_layer,
			rendered_geom_collection,
			RenderedGeometryCollection::RECONSTRUCTION_LAYER,
			only_if_reconstruction_layer_active))
	{
		return false;
	}

	return GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
			reconstruction_geometries_observing_feature,
			all_reconstruction_geoms_in_reconstruction_layer,
			feature_ref,
			geometry_property_iterator,
			reconstruction_tree);
}


GPlatesViewOperations::RenderedGeometryUtils::VisitFunctionOnRenderedGeometryLayers::VisitFunctionOnRenderedGeometryLayers(
		rendered_geometry_layer_function_type rendered_geom_layer_function,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active) :
d_rendered_geom_layer_function(rendered_geom_layer_function),
d_main_layers(main_layers),
d_only_if_main_layer_active(only_if_main_layer_active)
{
}

void
GPlatesViewOperations::RenderedGeometryUtils::VisitFunctionOnRenderedGeometryLayers::call_function(
		RenderedGeometryCollection &rendered_geom_collection)
{
	rendered_geom_collection.accept_visitor(*this);
}

bool
GPlatesViewOperations::RenderedGeometryUtils::VisitFunctionOnRenderedGeometryLayers::visit_main_rendered_layer(
		RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type)
{
	if (d_only_if_main_layer_active &&
		!rendered_geometry_collection.is_main_layer_active(main_layer_type))
	{
		return false;
	}

	// Only visit if current main layer is one of the layers we're interested in.
	return d_main_layers.test(main_layer_type);
}

bool
GPlatesViewOperations::RenderedGeometryUtils::VisitFunctionOnRenderedGeometryLayers::visit_rendered_geometry_layer(
		RenderedGeometryLayer &rendered_geometry_layer)
{
	// If we get here then we've been approved for calling user-specified
	// function on this rendered geometry layer.
	d_rendered_geom_layer_function(rendered_geometry_layer);

	// Not interesting in visiting RenderedGeometry objects.
	return false;
}


GPlatesViewOperations::RenderedGeometryUtils::ConstVisitFunctionOnRenderedGeometryLayers::ConstVisitFunctionOnRenderedGeometryLayers(
		rendered_geometry_layer_function_type rendered_geom_layer_function,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active) :
d_rendered_geom_layer_function(rendered_geom_layer_function),
d_main_layers(main_layers),
d_only_if_main_layer_active(only_if_main_layer_active)
{
}

void
GPlatesViewOperations::RenderedGeometryUtils::ConstVisitFunctionOnRenderedGeometryLayers::call_function(
		const RenderedGeometryCollection &rendered_geom_collection)
{
	rendered_geom_collection.accept_visitor(*this);
}

bool
GPlatesViewOperations::RenderedGeometryUtils::ConstVisitFunctionOnRenderedGeometryLayers::visit_main_rendered_layer(
		const RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type)
{
	if (d_only_if_main_layer_active &&
		!rendered_geometry_collection.is_main_layer_active(main_layer_type))
	{
		return false;
	}

	// Only visit if current main layer is one of the layers we're interested in.
	return d_main_layers.test(main_layer_type);
}

bool
GPlatesViewOperations::RenderedGeometryUtils::ConstVisitFunctionOnRenderedGeometryLayers::visit_rendered_geometry_layer(
		const RenderedGeometryLayer &rendered_geometry_layer)
{
	// If we get here then we've been approved for calling user-specified
	// function on this rendered geometry layer.
	d_rendered_geom_layer_function(rendered_geometry_layer);

	// Not interesting in visiting RenderedGeometry objects.
	return false;
}
