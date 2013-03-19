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

#ifndef GPLATES_QT_WIDGETS_EXPORTRASTEROPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTRASTEROPTIONSWIDGET_H

#include <boost/optional.hpp>
#include <QObject>

#include "ExportRasterOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportRasterAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportRasterOptionsWidget is used to show export options for
	 * exporting screen shots of the globe/map view.
	 */
	class ExportRasterOptionsWidget :
			public ExportOptionsWidget,
			protected Ui_ExportRasterOptionsWidget
	{
		Q_OBJECT

	public:

		/**
		 * Creates a @a ExportRasterOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr &export_configuration)
		{
			return new ExportRasterOptionsWidget(parent, export_animation_context, export_configuration);
		}


		/**
		 * Collects the options specified by the user and returns them as an
		 * export animation strategy configuration.
		 */
		virtual
		GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
		create_export_animation_strategy_configuration(
				const QString &filename_template);

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
		GPlatesGui::ExportRasterAnimationStrategy::Configuration d_export_configuration;

		boost::optional<double> d_constrained_aspect_ratio;


		explicit
		ExportRasterOptionsWidget(
				QWidget *parent_,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr &export_configuration);

		void
		make_signal_slot_connections();
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTRASTEROPTIONSWIDGET_H
