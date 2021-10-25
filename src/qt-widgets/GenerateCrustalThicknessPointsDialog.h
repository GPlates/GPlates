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

#ifndef GPLATES_QTWIDGETS_GENERATECRUSTALTHICKNESSPOINTSDIALOG_H
#define GPLATES_QTWIDGETS_GENERATECRUSTALTHICKNESSPOINTSDIALOG_H

#include <boost/optional.hpp>
#include <QDialog>
#include <QString>

#include "GenerateCrustalThicknessPointsDialogUi.h"

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
}

namespace GPlatesQtWidgets
{
	class ChooseFeatureCollectionWidget;
	class EditPlateIdWidget;
	class EditTimePeriodWidget;
	class EditStringWidget;

	/**
	 * This dialog generates a distribution of points with initial crustal thicknesses at a past geological time.
	 */
	class GenerateCrustalThicknessPointsDialog :
			public GPlatesDialog,
			protected Ui_GenerateCrustalThicknessPointsDialog
	{
		Q_OBJECT

	public:

		enum StackedWidgetPage
		{
			GENERATE_POINTS_PAGE,
			COLLECTION_PAGE
		};	

		GenerateCrustalThicknessPointsDialog(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent = 0);

		/**
		 * Reset the state of the dialog for a new creation process.
		 *
		 * Returns false if there is no focused feature geometry that is a polygon boundary.
		 */	
		bool
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
		handle_crustal_scalar_type_combobox_activated(
				const QString &text);

		void
		handle_point_density_spin_box_value_changed(
				int value);

	private:

		void
		setup_pages();

		void
		make_generate_points_page_current();

		void
		make_feature_collection_page_current();

		void
		display_point_density_spacing();

		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reverse_reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geom,
				const double &reconstruction_time,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THINNING_FACTOR;
		static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THICKNESS;


		GPlatesAppLogic::ApplicationState &d_application_state;
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
		 * Either 'gpml:CrustalThinningFactor' or 'gpml:CrustalThickness'.
		 */
		GPlatesPropertyValues::ValueObjectType d_crustal_scalar_type;

		/**
		 * The polygon geometry of the focused feature (topological plate/network or static polygon).
		 */
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_focused_boundary_polygon;

		GPlatesQtWidgets::InformationDialog *d_help_scalar_type_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_point_distribution_dialog;
	};
}

#endif // GPLATES_QTWIDGETS_GENERATECRUSTALTHICKNESSPOINTSDIALOG_H
