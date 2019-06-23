/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_GENERATEDEFORMINGMESHPOINTSDIALOG_H
#define GPLATES_QTWIDGETS_GENERATEDEFORMINGMESHPOINTSDIALOG_H

#include <boost/optional.hpp>
#include <boost/weak_ptr.hpp>
#include <QDialog>
#include <QString>

#include "GenerateDeformingMeshPointsDialogUi.h"

#include "GPlatesDialog.h"
#include "InformationDialog.h"

#include "maths/GeometryOnSphere.h"
#include "maths/PolygonOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"

#include "property-values/ValueObjectType.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	class ChooseFeatureCollectionWidget;
	class EditPlateIdWidget;
	class EditTimePeriodWidget;
	class EditStringWidget;
	class SetTopologyReconstructionParametersDialog;

	/**
	 * This dialog generates a distribution of points with initial crustal thicknesses at a past geological time.
	 */
	class GenerateDeformingMeshPointsDialog :
			public GPlatesDialog,
			protected Ui_GenerateDeformingMeshPointsDialog
	{
		Q_OBJECT

	public:

		enum StackedWidgetPage
		{
			GENERATE_POINTS_PAGE,
			PROPERTIES_PAGE,
			COLLECTION_PAGE
		};	

		GenerateDeformingMeshPointsDialog(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent = 0);

		/**
		 * Reset the state of the dialog for a new creation process.
		 *
		 * If there is no focused feature geometry that is a polygon boundary then the user can still
		 * choose a lat/lon extent.
		 */	
		void
		initialise();

	Q_SIGNALS:

		void
		feature_created(
				GPlatesModel::FeatureHandle::weak_ref feature);

	private Q_SLOTS:

		void
		handle_create();

		void
		handle_cancel();

		void
		handle_previous();

		void
		handle_next();

		void
		handle_points_region_mode_button(
				bool checked);

		void
		handle_left_extents_spin_box_value_changed(
				double value);

		void
		handle_right_extents_spin_box_value_changed(
				double value);

		void
		handle_use_global_extents_button_clicked();

		void
		handle_point_density_spin_box_value_changed(
				int value);

		void
		handle_visual_layer_added(
				boost::weak_ptr<GPlatesPresentation::VisualLayer>);

	private:

		/**
		 * RAII class keeps track of whether currently creating a feature or not.
		 */
		class CurrentlyCreatingFeatureGuard
		{
		public:
			CurrentlyCreatingFeatureGuard(
					GenerateDeformingMeshPointsDialog &dialog) :
				d_dialog(dialog)
			{
				d_dialog.d_currently_creating_feature = true;
			}

			~CurrentlyCreatingFeatureGuard()
			{
				d_dialog.d_currently_creating_feature = false;
			}

		private:
			GenerateDeformingMeshPointsDialog &d_dialog;
		};


		void
		initialise_widgets();

		void
		setup_pages();

		void
		make_generate_points_page_current();

		void
		make_properties_page_current();

		void
		make_feature_collection_page_current();

		void
		display_point_density_spacing();

		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reverse_reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geom,
				const double &reconstruction_time,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		void
		open_topology_reconstruction_parameters_dialog(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> reconstruct_visual_layer);


		static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THICKNESS;
		static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_STRETCHING_FACTOR;
		static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THINNING_FACTOR;


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		GPlatesGui::FeatureFocus &d_feature_focus;
		
		/**
		 * The custom edit widget for reconstruction. Memory managed by Qt.
		 */
		EditPlateIdWidget *d_plate_id_widget;

		/**
		 * The custom edit widget for GmlTimePeriod. Memory managed by Qt.
		 */
		EditTimePeriodWidget *d_time_period_widget;

		/**
		 * The custom edit widget for XsString which we are using for the gml:name property.
		 * Memory managed by Qt.
		 */
		EditStringWidget *d_name_widget;

		/**
		 * The widget for choosing the feature collection.
		 */
		GPlatesQtWidgets::ChooseFeatureCollectionWidget *d_choose_feature_collection_widget;

		/**
		 * Used to initialise topological reconstruction for newly created reconstruct layers.
		 */
		SetTopologyReconstructionParametersDialog *d_set_topology_reconstruction_parameters_dialog;

		/**
		 * The polygon geometry of the focused feature (topological plate/network or static polygon).
		 */
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_focused_boundary_polygon;
		/**
		 * Same as @a d_focused_boundary_polygon but including rigid block holes as interiors.
		 *
		 * This actually means that points will *not* be generated inside rigid blocks because they are actually
		 * outside the *filled* polygon (they are not filled).
		 */
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_focused_boundary_polygon_with_rigid_block_holes;

		/**
		 * Is true when inside @a handle_create, so we know when a new layer is created as a result of creating a new feature.
		 */
		bool d_currently_creating_feature;

		GPlatesQtWidgets::InformationDialog *d_help_scalar_type_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_point_region_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_point_distribution_dialog;
	};
}

#endif // GPLATES_QTWIDGETS_GENERATEDEFORMINGMESHPOINTSDIALOG_H
