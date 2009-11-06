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

#include "RenderedGeometryUtils.h"

#include "RenderedReconstructionGeometry.h"
#include "RenderedGeometryVisitor.h"


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
GPlatesViewOperations::RenderedGeometryUtils::get_reconstruction_geometries(
		reconstruction_geom_seq_type &reconstruction_geom_seq,
		const RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		bool only_if_main_layer_active)
{
	RenderedGeometryCollection::main_layers_update_type main_layers;
	main_layers.set(main_layer_type);

	return get_reconstruction_geometries(
			reconstruction_geom_seq, rendered_geom_collection,
			main_layers, only_if_main_layer_active);
}


bool
GPlatesViewOperations::RenderedGeometryUtils::get_reconstruction_geometries(
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

	using boost::lambda::_1;
	using boost::lambda::_2;

	// Remove any duplicate reconstruction geometries.
	reconstruction_geom_seq.erase(
			std::unique(
					reconstruction_geom_seq.begin(),
					reconstruction_geom_seq.end(),
					boost::lambda::bind(&GPlatesModel::ReconstructionGeometry::non_null_ptr_type::get, _1) ==
							boost::lambda::bind(&GPlatesModel::ReconstructionGeometry::non_null_ptr_type::get, _1)),
			reconstruction_geom_seq.end());

	return !reconstruction_geom_seq.empty();
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
