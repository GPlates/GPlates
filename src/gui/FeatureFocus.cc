/* $Id$ */

/**
 * \file 
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

#include <boost/none.hpp>
#include <QDebug>

#include "FeatureFocus.h"
#include "PythonManager.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructGraph.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/ReconstructionGeometryFinder.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/GeometryUtils.h"

#include "feature-visitors/GeometryFinder.h"

#include "model/FeatureHandle.h"
#include "model/WeakReferenceCallback.h"

#include "presentation/Application.h"
#include "presentation/ViewState.h"

#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/SceneView.h"

#include "utils/FeatureUtils.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryUtils.h"


namespace
{
	/**
	 * Feature handle weak ref callback to unset the focused feature
	 * if it gets deactivated in the model.
	 */
	class FocusedFeatureDeactivatedCallback :
			public GPlatesModel::WeakReferenceCallback<GPlatesModel::FeatureHandle>
	{
	public:

		explicit
		FocusedFeatureDeactivatedCallback(
				GPlatesGui::FeatureFocus &feature_focus) :
			d_feature_focus(feature_focus)
		{
		}

		void
		publisher_deactivated(
				const deactivated_event_type &)
		{
			d_feature_focus.unset_focus();
		}

	private:

		GPlatesGui::FeatureFocus &d_feature_focus;
	};
}


GPlatesGui::FeatureFocus::FeatureFocus(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection):
	d_rendered_geometry_collection(rendered_geometry_collection)
{
	// Get notified whenever the rendered geometry collection gets updated.
	QObject::connect(
			&d_rendered_geometry_collection,
			SIGNAL(collection_was_updated(
					GPlatesViewOperations::RenderedGeometryCollection &,
					GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type)),
			this,
			SLOT(handle_rendered_geometry_collection_update()));
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type new_associated_rg)
{
	if ( ! new_feature_ref.is_valid())
	{
		unset_focus();
		return;
	}

	if (d_focused_feature == new_feature_ref &&
		d_associated_reconstruction_geometry == new_associated_rg)
	{
		// Avoid infinite signal/slot loops like the plague!
		return;
	}

	d_focused_feature = d_callback_focused_feature = new_feature_ref;
	// Attach callback to feature handle weak-ref so we can unset the focus when
	// the feature is deactivated in the model.
	// NOTE: See data member comment for 'd_callback_focused_feature' for an explanation of
	// why there's a separate callback weak ref.
	d_callback_focused_feature.attach_callback(new FocusedFeatureDeactivatedCallback(*this));

	d_associated_reconstruction_geometry = new_associated_rg.get();

	// See if the new_associated_rg has a geometry property.
	boost::optional<GPlatesModel::FeatureHandle::iterator> new_geometry_property =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_geometry_property_iterator(
					new_associated_rg);

	// Either way we set the properties iterator - it'll either get set to
	// the default value (invalid) or to the found properties iterator.
	d_associated_geometry_property = new_geometry_property
			? new_geometry_property.get() : GPlatesModel::FeatureHandle::iterator();

	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref,
		GPlatesModel::FeatureHandle::iterator new_associated_property)
{
	if ( ! new_feature_ref.is_valid())
	{
		unset_focus();
		return;
	}

	if (d_focused_feature == new_feature_ref &&
		d_associated_geometry_property == new_associated_property)
	{
		// Avoid infinite signal/slot loops like the plague!
		return;
	}

	d_focused_feature = d_callback_focused_feature = new_feature_ref;
	// Attach callback to feature handle weak-ref so we can unset the focus when
	// the feature is deactivated in the model.
	// NOTE: See data member comment for 'd_callback_focused_feature' for an explanation of
	// why there's a separate callback weak ref.
	d_callback_focused_feature.attach_callback(new FocusedFeatureDeactivatedCallback(*this));

	d_associated_reconstruction_geometry = NULL;
	d_associated_geometry_property = new_associated_property;

	// Find the ReconstructionGeometry associated with the geometry property.
	find_new_associated_reconstruction_geometry();

	// tell the rest of the application about the new focus
	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::set_focus(
		GPlatesModel::FeatureHandle::weak_ref new_feature_ref)
{
	if ( ! new_feature_ref.is_valid())
	{
		unset_focus();
		return;
	}
	
	// Locate a geometry property within the feature.
	// Note that there could be multiple geometry properties in which case we'll
	// choose the first since the caller hasn't specified a particular property.
	GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geometries_observing_feature;
	if (!GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_observing_feature(
		reconstruction_geometries_observing_feature,
		d_rendered_geometry_collection,
		new_feature_ref))
	{
		// None found, we cannot focus this.
		unset_focus();
		return; // I guess we need a return here?
	}

	// Found something, just focus the first one.
	set_focus(new_feature_ref, reconstruction_geometries_observing_feature.front());
}


void
GPlatesGui::FeatureFocus::unset_focus()
{
	d_focused_feature = d_callback_focused_feature = GPlatesModel::FeatureHandle::weak_ref();
	d_associated_reconstruction_geometry = NULL;
	d_associated_geometry_property = GPlatesModel::FeatureHandle::iterator();

	emit focus_changed(*this);
}


void
GPlatesGui::FeatureFocus::find_new_associated_reconstruction_geometry()
{
	if ( !d_focused_feature.is_valid() ||
		!d_associated_geometry_property.is_still_valid())
	{
		// There is either no focused feature, or no geometry property
		// associated with the most recent ReconstructionGeometry of the focused feature.
		// Either way, there's nothing for us to do here.
		return;
	}

	// Get any ReconstructionGeometry objects (observing the focused feature) that are visible in
	// all active layers of the RenderedGeometryCollection - this is the output of the current
	// reconstruction and provides a more convenient means to get the visible geometries.
	// If we used the current Reconstruction object and iterated over the layer outputs it wouldn't
	// be as easy due to the varied richness of the interface provided by the layer outputs (layer proxies) -
	// in other words for each layer we would need to know how to generate reconstruction geometries
	// amongst all the other interface methods provided by each layer.
	GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geometries_observing_feature;
	if (!GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_observing_feature(
		reconstruction_geometries_observing_feature,
		d_rendered_geometry_collection,
		d_focused_feature,
		d_associated_geometry_property))
	{
		// We looked at the relevant reconstruction geometries in the new reconstruction, without
		// finding a match.  Thus, it appears that there is no RG in the new reconstruction which
		// corresponds to the current associated geometry property.
		//
		// When there is no RG found, we lose the associated RG.  This will be apparent to the user
		// if the reconstruction time is incremented to a time when there is no RG (meaning that the
		// associated RG will become NULL). However the geometry property used by the RG will still
		// be non-null so when the user then steps back one increment, a new RG will be found that
		// uses the same geometry property and so the RG will be non-null once again.
		d_associated_reconstruction_geometry = NULL;

		// NOTE: We don't change the associated geometry property since
		// the focused feature hasn't changed and hence it's still applicable.
		// We'll be using the geometry property to find the associated RG when/if one comes
		// back into existence.
		return;
	}

	// Actually we don't want to generate a debug message because it'll get output for every
	// reconstruction time change while the current feature is focused.
#if 0
	if (reconstruction_geometries_observing_feature.size() > 1)
	{
		qDebug() <<
			"WARNING: More than one reconstruction geometry for focused feature "
			"geometry property - choosing the first one found.";
	}
#endif

	// Assign the new associated reconstruction geometry.
	d_associated_reconstruction_geometry = reconstruction_geometries_observing_feature.front().get();
}


void
GPlatesGui::FeatureFocus::announce_modification_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have modified it, nothing is focused!
		return;
	}

	if (!associated_geometry_property().is_still_valid()) {
		// There is no geometry property - it must have been removed
		// during the feature modification.
		// We'll need to unset the focused feature.
		unset_focus();
	}

	emit focused_feature_modified(*this);
}


void
GPlatesGui::FeatureFocus::announce_deletion_of_focused_feature()
{
	if ( ! d_focused_feature.is_valid()) {
		// You can't have deleted it, nothing is focused!
		return;
	}
	emit focused_feature_deleted(*this);
	unset_focus();
}


void
GPlatesGui::FeatureFocus::handle_rendered_geometry_collection_update()
{
	const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type
			old_associated_reconstruction_geometry = d_associated_reconstruction_geometry;

	find_new_associated_reconstruction_geometry();

	if (d_associated_reconstruction_geometry != old_associated_reconstruction_geometry)
	{
		// A new ReconstructionGeometry has been found so we should
		// emit a signal in case clients need to know this.
		emit focus_changed(*this);
	}
}

class ReconstructionGeometryLocator :
	public GPlatesAppLogic::ConstReconstructionGeometryVisitor
{
public:
	boost::optional<const GPlatesMaths::LatLonPoint>
	location()
	{
		return *d_location;
	}

protected:
		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf)
		{ }

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
		{
			std::vector<GPlatesMaths::PointOnSphere> points;
			GPlatesAppLogic::GeometryUtils::get_geometry_points(*rfg->reconstructed_geometry(),points);
			if(points.size())
				d_location =  GPlatesMaths::make_lat_lon_point(points[0]);
		}

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<reconstructed_flowline_type> &rf)
		{ }

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<reconstructed_motion_path_type> &rmp)
		{ }

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<reconstructed_virtual_geomagnetic_pole_type> &rvgp)
		{ }

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<resolved_raster_type> &rr)
		{ }

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<resolved_scalar_field_3d_type> &rsf)
		{ }

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg)
		{
			// We want the first vertex.
			std::vector<GPlatesMaths::PointOnSphere> points;
			GPlatesAppLogic::GeometryUtils::get_geometry_points(*rtg->resolved_topology_geometry(), points);

			d_location = GPlatesMaths::make_lat_lon_point(points.front());
		}

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
		{
			std::vector<GPlatesMaths::PointOnSphere> points;
			GPlatesAppLogic::GeometryUtils::get_geometry_points(*rtn->nodes_begin()->get_geometry(),points);
			if(points.size())
			{
				d_location =  GPlatesMaths::make_lat_lon_point(points[0]);
			}
		}

		void
		visit(const GPlatesUtils::non_null_intrusive_ptr<co_registration_data_type> &crr)
		{ }

        void
        visit(const GPlatesUtils::non_null_intrusive_ptr<reconstructed_small_circle_type> &rsc)
        { }

private:
	boost::optional<GPlatesMaths::LatLonPoint> d_location;
};


boost::optional<const GPlatesMaths::LatLonPoint>
GPlatesGui::locate_focus()
{
	GPlatesPresentation::Application &app = GPlatesPresentation::Application::instance();
	FeatureFocus& focus = app.get_view_state().get_feature_focus();
			
	ReconstructionGeometryLocator locator;
	GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type geo = focus.associated_reconstruction_geometry();
	if(geo)
	{
		geo->accept_visitor(locator);
		return locator.location();
	}
	else
	{
		return boost::none;
	}
}
