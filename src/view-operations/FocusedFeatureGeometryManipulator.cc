/* $Id$ */

/**
 * \file 
 * Transfers changes to focused feature geometry to the feature containing
 * the geometry.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include "FocusedFeatureGeometryManipulator.h"

#include "UndoRedo.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FlowlineUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionFeatureProperties.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/ResolvedTopologicalGeometry.h"
#include "app-logic/ResolvedTopologicalNetwork.h"

#include "feature-visitors/GeometrySetter.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "model/FeatureHandle.h"
#include "presentation/ViewState.h"


namespace GPlatesViewOperations
{
	namespace
	{

		/**
		 * Visitor gets a sequence of @a PointOnSphere objects from a @a GeometryOnSphere
		 * derived object and sets the geometry in a @a GeometryBuilder.
		 */
		class SetGeometryInBuilder :
				private GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:
			SetGeometryInBuilder(
					GeometryBuilder *geom_builder) :
			d_geom_builder(geom_builder)
			{
			}

			GeometryBuilder::UndoOperation
			set_geometry_in_builder(
					GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry)
			{
				geometry->accept_visitor(*this);

				return d_undo_operation;
			}

		private:
			virtual
			void
			visit_multi_point_on_sphere(
					GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_undo_operation = d_geom_builder->set_geometry(
						GPlatesMaths::GeometryType::MULTIPOINT,
						multi_point_on_sphere->begin(),
						multi_point_on_sphere->end());
			}

			virtual
			void
			visit_point_on_sphere(
					GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				const GPlatesMaths::PointOnSphere geom_point[1] = { *point_on_sphere };

				d_undo_operation = d_geom_builder->set_geometry(
						GPlatesMaths::GeometryType::POINT,
						geom_point,
						geom_point + 1);
			}

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_undo_operation = d_geom_builder->set_geometry(
						GPlatesMaths::GeometryType::POLYGON,
						polygon_on_sphere->vertex_begin(),
						polygon_on_sphere->vertex_end());
			}

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_undo_operation = d_geom_builder->set_geometry(
						GPlatesMaths::GeometryType::POLYLINE,
						polyline_on_sphere->vertex_begin(),
						polyline_on_sphere->vertex_end());
			}

		private:
			GeometryBuilder *d_geom_builder;
			GeometryBuilder::UndoOperation d_undo_operation;
		};
	}
}

GPlatesViewOperations::FocusedFeatureGeometryManipulator::FocusedFeatureGeometryManipulator(
		GeometryBuilder &focused_feature_geom_builder,
		GPlatesPresentation::ViewState &view_state) :
d_focused_feature_geom_builder(&focused_feature_geom_builder),
d_feature_focus(&view_state.get_feature_focus()),
d_application_state(&view_state.get_application_state()),
d_ignore_geom_builder_update(false),
d_block_infinite_signal_slot_loop_depth(0)
{
	connect_to_geometry_builder();

	connect_to_feature_focus();
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::connect_to_geometry_builder()
{
	// Connect to the current geometry builder's signals.

	// GeometryBuilder has just finished updating geometry.
	QObject::connect(
			d_focused_feature_geom_builder,
			SIGNAL(stopped_updating_geometry()),
			this,
			SLOT(geometry_builder_stopped_updating_geometry()));

	// GeometryBuilder has just moved a vertex.
	// We're interested only in whether it is an intermediate move or not.
	// If it is then we're not going to waste time reconstructing.
	QObject::connect(
			d_focused_feature_geom_builder,
			SIGNAL(moved_point_in_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesMaths::PointOnSphere &,
					bool)),
			this,
			SLOT(move_point_in_current_geometry(
					GPlatesViewOperations::GeometryBuilder::PointIndex,
					const GPlatesMaths::PointOnSphere &,
					bool)));
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::connect_to_feature_focus()
{
	QObject::connect(
		d_feature_focus,
		SIGNAL(focus_changed(
				GPlatesGui::FeatureFocus &)),
		this,
		SLOT(set_focus(
				GPlatesGui::FeatureFocus &)));
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::geometry_builder_stopped_updating_geometry()
{
	// Stop infinite loop from happening where we update feature with geometry builder
	// which updates geometry builder with feature in a continuous loop.
	if (is_infinite_signal_slot_loop_blocked())
	{
		return;
	}

	// Stop infinite loop from happening where we update feature with geometry builder
	// which updates geometry builder with feature in a continuous loop.
	BlockInfiniteSignalSlotLoop block_infinite_loop(this);

	// The geometry builder has just potentially finished a group of
	// geometry modifications and is now notifying us that it's finished.

	// Get the new geometry from the builder and set it in the currently focused
	// feature's geometry property.
	if (d_focused_geometry)
	{
		// Only set geometry in focused feature if we're not ignoring the
		// GeometryBuilder update to the geometry (an example of an update we
		// ignore is the Move Vertex operations that occur during a mouse drag -
		// we're only interested in the Move Vertex operation when the mouse
		// button is released).
		if (!d_ignore_geom_builder_update)
		{
			if (d_feature.is_valid())
			{
				convert_geom_from_builder_to_feature();
			}
		}
	}

	d_ignore_geom_builder_update = false;
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::move_point_in_current_geometry(
		GPlatesViewOperations::GeometryBuilder::PointIndex /*point_index*/,
		const GPlatesMaths::PointOnSphere &/*new_oriented_pos_on_globe*/,
		bool is_intermediate_move)
{
	// We're interested only in whether it is an intermediate move or not.
	// If it is then we're not going to waste time reconstructing.
	d_ignore_geom_builder_update = is_intermediate_move;
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::set_focus(
		GPlatesGui::FeatureFocus &feature_focus)
{
	// Stop infinite loop from happening where we update geometry builder with feature
	// which updates feature with geometry builder in a continuous loop.
	if (is_infinite_signal_slot_loop_blocked())
	{
		return;
	}

	// Stop infinite loop from happening where we update geometry builder with feature
	// which updates feature with geometry builder in a continuous loop.
	BlockInfiniteSignalSlotLoop block_infinite_loop(this);

	// FIXME: The reconstruction time needs to be taken into account when handling undo.
	// Currently just clear the undo stack and avoid this issue until we get a better
	// handle on undo/redo in the model and across reconstruction times.
	GPlatesViewOperations::UndoRedo::instance().get_active_undo_stack().clear();

	// Set these data member variables first because when we call operations
	// on GeometryBuilder then 'this' object will receive signals from it
	// and use these variables.
	d_feature = feature_focus.focused_feature();

	// Accept any type of ReconstructionGeometry derivation (not just ReconstructedFeatureGeometry's)
	// because then the CloneGeometry tool, for example, can copy a ResolvedTopologicalGeometry's geometry.
	// Other operations on ResolvedTopologicalGeometry's don't make sense though, such as MoveVertex,
	// and so the appropriate canvas tools will need to be disabled in these situations.
	d_focused_geometry = NULL;
	if (feature_focus.associated_reconstruction_geometry())
	{
		d_focused_geometry = feature_focus.associated_reconstruction_geometry();
	}

	convert_geom_from_feature_to_builder();
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::convert_geom_from_feature_to_builder()
{
	// If we've got a focused feature geometry at a reconstruction time then
	// copy the geometry to the geometry builder, otherwise clear the geometry
	// in the geometry builder.
	if (d_focused_geometry)
	{
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_on_sphere =
				get_geometry_from_feature();
		if (!geometry_on_sphere)
		{
			// Clear the geometry builder and return early.
			d_focused_feature_geom_builder->clear_all_geometries();
			return;
		}

		// Initialise our GeometryBuilder with this geometry.
		// Various canvas tools will then make changes to the geometry through
		// this builder. We'll listen for those changes via the GeometryBuilder
		// signals and make changes to the feature containing the original
		// geometry property.
		// TODO: we currently ignore the returned undo operation because we're not
		// allowing undo/redo across a feature focus change boundary.
		SetGeometryInBuilder(
				d_focused_feature_geom_builder).set_geometry_in_builder(geometry_on_sphere.get());
	}
	else
	{
		// TODO: we currently ignore the returned undo operation because we're not
		// allowing undo/redo across a feature focus change boundary.
		d_focused_feature_geom_builder->clear_all_geometries();
	}
}

boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
GPlatesViewOperations::FocusedFeatureGeometryManipulator::get_geometry_from_feature()
{
	// See if the focused reconstruction geometry is an RFG.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> focused_rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry *>(d_focused_geometry);
	if (focused_rfg)
	{
		return focused_rfg.get()->reconstructed_geometry();
	}

	// See if the focused reconstruction geometry is a resolved topological boundary.
	boost::optional<const GPlatesAppLogic::ResolvedTopologicalGeometry *> focused_rtb =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ResolvedTopologicalGeometry *>(d_focused_geometry);
	if (focused_rtb)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type resolved_geom =
				focused_rtb.get()->resolved_topology_geometry();
		return resolved_geom;
	}

	// See if the focused reconstruction geometry is a resolved topological network.
	// If so then we'll use it's boundary polygon as the geometry and ignore the interior nodes, etc.
	boost::optional<const GPlatesAppLogic::ResolvedTopologicalNetwork *> focused_rtn =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ResolvedTopologicalNetwork *>(d_focused_geometry);
	if (focused_rtn)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type resolved_geom =
				focused_rtn.get()->boundary_polygon();
		return resolved_geom;
	}

	return boost::none;
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::convert_geom_from_builder_to_feature()
{
	// We're only interested in setting geometry for non-topological features because topological
	// features resolve their geometry using other features so it doesn't make sense to modify
	// its resolved geometry using one of the canvas tools.
	//
	// So return early if the ReconstructionGeometry is not an RFG.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> focused_rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry *>(d_focused_geometry);
	if (!focused_rfg)
	{
		return;
	}

	GeometryBuilder::geometry_opt_ptr_type opt_geometry_on_sphere =
		d_focused_feature_geom_builder->get_geometry_on_sphere();

	if (opt_geometry_on_sphere)
	{
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere =
				*opt_geometry_on_sphere;

		// Reconstruct back to present day.
		geometry_on_sphere = reconstruct(
				geometry_on_sphere,
				*focused_rfg.get()->get_reconstruction_tree(),
				// FIXME: Using default reconstruct parameters, but will probably need to
				// get this from the layer that created the focused feature...
				GPlatesAppLogic::ReconstructParams(),
				true/*reverse_reconstruct*/);

		// Set the actual geometry in the geometry property of the focused geometry.
		GPlatesFeatureVisitors::GeometrySetter geometry_setter(geometry_on_sphere);

		// Since we can have multiple geometry properties per feature we make sure we
		// set the geometry that the user actually clicked on.
		GPlatesModel::FeatureHandle::iterator iter = focused_rfg.get()->property();
		GPlatesModel::TopLevelProperty::non_null_ptr_type geom_top_level_prop_clone = (*iter)->clone();
		geometry_setter.set_geometry(geom_top_level_prop_clone.get());
		*iter = geom_top_level_prop_clone;
		
		convert_secondary_geometries_to_features();

		// Announce that we've modified the focused feature.
		d_feature_focus->announce_modification_of_focused_feature();
	}
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::convert_secondary_geometries_to_features()
{
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geom = 
		d_focused_feature_geom_builder->get_secondary_geometry();
		
	boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type> rfg = 
		d_focused_feature_geom_builder->get_secondary_rfg();
	
	if (!geom || !rfg)
	{
		return;
	}	
	
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere =
		*geom;

	// Reconstruct back to present day.
	geometry_on_sphere = reconstruct(
			geometry_on_sphere,
			*rfg.get()->get_reconstruction_tree(),
			// FIXME: Using default reconstruct parameters, but will probably need to
			// get this from the layer that created the focused feature...
			GPlatesAppLogic::ReconstructParams(),
			true/*reverse_reconstruct*/);

	// Set the actual geometry in the geometry property of the focused geometry.
	GPlatesFeatureVisitors::GeometrySetter geometry_setter(geometry_on_sphere);

	// Since we can have multiple geometry properties per feature we make sure we
	// set the geometry that the user actually clicked on.
	GPlatesModel::FeatureHandle::iterator iter = (*rfg)->property();
	GPlatesModel::TopLevelProperty::non_null_ptr_type geom_top_level_prop_clone = (*iter)->clone();
	geometry_setter.set_geometry(geom_top_level_prop_clone.get());
	*iter = geom_top_level_prop_clone;
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesViewOperations::FocusedFeatureGeometryManipulator::reconstruct(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere,
		const GPlatesAppLogic::ReconstructionTree &reconstruction_tree,
		const GPlatesAppLogic::ReconstructParams &reconstruct_params,
		bool reverse_reconstruct)
{
	// We need to convert geometry to present day coordinates. This is because the geometry is
	// currently reconstructed geometry at the current reconstruction time.
	return GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
			geometry_on_sphere,
			d_feature,
			reconstruction_tree,
			reconstruct_params,
			reverse_reconstruct);
}
