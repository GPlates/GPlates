/* $Id$ */

/**
 * \file 
 * Transfers changes to focused feature geometry to the feature containing
 * the geometry.
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

#include "FocusedFeatureGeometryManipulator.h"

#include "UndoRedo.h"

#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "feature-visitors/GeometrySetter.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "model/ReconstructionTree.h"
#include "presentation/ViewState.h"


namespace GPlatesViewOperations
{
	namespace
	{
		/**
		 * Visitor a @a GeometryOnSphere and generates a new one that
		 * is reverse-reconstructed to present day.
		 */
		class Reconstruct :
			private GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:
			Reconstruct(
					const GPlatesModel::integer_plate_id_type plate_id,
					GPlatesModel::ReconstructionTree &recon_tree,
					bool reverse_reconstruct) :
			d_plate_id(plate_id),
			d_recon_tree(recon_tree),
			d_reverse_reconstruct(reverse_reconstruct)
			{
			}

			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
			reconstruct(
					GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry)
			{
				geometry->accept_visitor(*this);

				return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(
						d_reverse_reconstructed_geom.get(),
						GPlatesUtils::NullIntrusivePointerHandler());
			}

		private:
			virtual
			void
			visit_multi_point_on_sphere(
					GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_reverse_reconstructed_geom = reconstruct(multi_point_on_sphere).get();
			}

			virtual
			void
			visit_point_on_sphere(
					GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				d_reverse_reconstructed_geom = reconstruct(point_on_sphere).get();
			}

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_reverse_reconstructed_geom = reconstruct(polygon_on_sphere).get();
			}

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_reverse_reconstructed_geom = reconstruct(polyline_on_sphere).get();
			}

		private:
			const GPlatesModel::integer_plate_id_type d_plate_id;
			GPlatesModel::ReconstructionTree &d_recon_tree;
			bool d_reverse_reconstruct;
			GPlatesMaths::GeometryOnSphere::maybe_null_ptr_to_const_type d_reverse_reconstructed_geom;

			/**
			 * Apply a reverse reconstruction to the given temporary geometry, so that the
			 * coordinates are set to present-day location given the supplied plate id and
			 * current reconstruction tree.
			 *
			 * @a G should be a non_null_ptr_to_const_type of some suitable GeometryOnSphere
			 * derivation, with an implementation of FiniteRotation::operator*() available.
			 */
			template <class G>
			G
			reconstruct(
					G geometry)
			{
				// Get the composed absolute rotation needed to bring a thing on that plate
				// in the present day to this time.
				GPlatesMaths::FiniteRotation rotation =
						d_recon_tree.get_composed_absolute_rotation(d_plate_id).first;

				// Are we reversing reconstruction back to present day ?
				if (d_reverse_reconstruct)
				{
					rotation = GPlatesMaths::get_reverse(rotation);
				}
				
				// Apply the reverse rotation.
				G present_day_geometry = rotation * geometry;
				
				return present_day_geometry;
			}
		};

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
						GeometryType::MULTIPOINT,
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
						GeometryType::POINT,
						geom_point,
						geom_point + 1);
			}

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_undo_operation = d_geom_builder->set_geometry(
						GeometryType::POLYGON,
						polygon_on_sphere->vertex_begin(),
						polygon_on_sphere->vertex_end());
			}

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_undo_operation = d_geom_builder->set_geometry(
						GeometryType::POLYLINE,
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
d_reconstruct(&view_state.get_reconstruct()),
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
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)),
		this,
		SLOT(set_focus(
				GPlatesModel::FeatureHandle::weak_ref,
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type)));
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
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type focused_geometry)
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
	d_feature = feature_ref;

	// We're only interested in ReconstructedFeatureGeometry's (ResolvedTopologicalGeometry's,
	// for instance, reference regular feature geometries).
	GPlatesModel::ReconstructedFeatureGeometry *focused_rfg = NULL;
	if (focused_geometry &&
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type(
				focused_geometry, focused_rfg))
	{
		d_focused_geometry = focused_rfg;
	}
	else
	{
		d_focused_geometry = NULL;
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
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere =
				d_focused_geometry->geometry();

		// Initialise our GeometryBuilder with this geometry.
		// Various canvas tools will then make changes to the geometry through
		// this builder. We'll listen for those changes via the GeometryBuilder
		// signals and make changes to the feature containing the original
		// geometry property.
		// TODO: we currently ignore the returned undo operation because we're not
		// allowing undo/redo across a feature focus change boundary.
		SetGeometryInBuilder(
				d_focused_feature_geom_builder).set_geometry_in_builder(geometry_on_sphere);
	}
	else
	{
		// TODO: we currently ignore the returned undo operation because we're not
		// allowing undo/redo across a feature focus change boundary.
		d_focused_feature_geom_builder->clear_all_geometries();
	}
}

void
GPlatesViewOperations::FocusedFeatureGeometryManipulator::convert_geom_from_builder_to_feature()
{
	GeometryBuilder::geometry_opt_ptr_type opt_geometry_on_sphere =
		d_focused_feature_geom_builder->get_geometry_on_sphere();

	if (opt_geometry_on_sphere)
	{
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere =
				*opt_geometry_on_sphere;

		// Reconstruct back to present day.
		geometry_on_sphere = reconstruct(geometry_on_sphere, true/*reverse_reconstruct*/);

		// Set the actual geometry in the geometry property of the focused geometry.
		GPlatesFeatureVisitors::GeometrySetter geometry_setter(geometry_on_sphere);

		// Since we can have multiple geometry properties per feature we make sure we
		// set the geometry that the user actually clicked on.
		GPlatesModel::TopLevelProperty &geom_top_level_prop = **d_focused_geometry->property();
		geometry_setter.set_geometry(&geom_top_level_prop);

		// Announce that we've modified the focused feature.
		d_feature_focus->announce_modification_of_focused_feature();
	}
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesViewOperations::FocusedFeatureGeometryManipulator::reconstruct(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere,
		bool reverse_reconstruct)
{
	// If feature is reconstructable then need to convert geometry to present day
	// coordinates first. This is because the geometry is currently reconstructed
	// geometry at the current reconstruction time.
	GPlatesModel::integer_plate_id_type plate_id;
	if (get_plate_id_from_feature(plate_id))
	{
		// Get current reconstruction tree.
		GPlatesModel::ReconstructionTree &recon_tree =
				d_reconstruct->get_current_reconstruction().reconstruction_tree();

		geometry_on_sphere = Reconstruct(
			plate_id, recon_tree, reverse_reconstruct).reconstruct(geometry_on_sphere);
	}

	// If feature wasn't reconstructed then we'll be returning the geometry passed in.
	return geometry_on_sphere;
}

bool
GPlatesViewOperations::FocusedFeatureGeometryManipulator::get_plate_id_from_feature(
		GPlatesModel::integer_plate_id_type &plate_id)
{
	if (d_focused_geometry && d_focused_geometry->reconstruction_plate_id())
	{
		plate_id = *d_focused_geometry->reconstruction_plate_id();

		return true;
	}

	return false;
}
