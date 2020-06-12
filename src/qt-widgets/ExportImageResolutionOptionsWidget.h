/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_EXPORTIMAGERESOLUTIONOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTIMAGERESOLUTIONOPTIONSWIDGET_H

#include <boost/optional.hpp>
#include <QWidget>

#include "ui_ExportImageResolutionOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportOptionsUtils.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportImageResolutionOptionsWidget is used to show export options for
	 * exporting images of the globe/map view (including SVG export).
	 */
	class ExportImageResolutionOptionsWidget :
			public QWidget,
			protected Ui_ExportImageResolutionOptionsWidget
	{
		Q_OBJECT

	public:

		/**
		 * Creates a @a ExportImageResolutionOptionsWidget containing default export options.
		 */
		static
		ExportImageResolutionOptionsWidget *
		create(
				QWidget *parent,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportOptionsUtils::ExportImageResolutionOptions &default_export_image_resolution_options)
		{
			return new ExportImageResolutionOptionsWidget(
					parent,
					export_animation_context,
					default_export_image_resolution_options);
		}


		/**
		 * Returns the options that have (possibly) been edited by the user via the GUI.
		 */
		const GPlatesGui::ExportOptionsUtils::ExportImageResolutionOptions &
		get_export_image_resolution_options() const
		{
			return d_export_image_resolution_options;
		}

	private Q_SLOTS:

		void
		react_width_spin_box_value_changed(
				int value);

		void
		react_height_spin_box_value_changed(
				int value);

		void
		react_constrain_aspect_ratio_check_box_clicked();

		void
		handle_use_main_window_dimensions_push_button_clicked();

	private:

		GPlatesGui::ExportAnimationContext &d_export_animation_context;
		GPlatesGui::ExportOptionsUtils::ExportImageResolutionOptions d_export_image_resolution_options;

		boost::optional<double> d_constrained_aspect_ratio;


		explicit
		ExportImageResolutionOptionsWidget(
				QWidget *parent_,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportOptionsUtils::ExportImageResolutionOptions &default_export_image_resolution_options);


		void
		make_signal_slot_connections();
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTIMAGERESOLUTIONOPTIONSWIDGET_H
