/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include "ExportRasterOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportRasterAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * Raster (colour or numerical) export options.
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
				const GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr &
						export_configuration)
		{
			return new ExportRasterOptionsWidget(parent, export_configuration);
		}


		/**
		 * Collects the options specified by the user and
		 * returns them as an export animation strategy configuration.
		 */
		virtual
		GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
		create_export_animation_strategy_configuration(
				const QString &filename_template);

	private Q_SLOTS:

		void
		react_resolution_spin_box_value_changed(
				double value);

		void
		react_top_extents_spin_box_value_changed(
				double value);
		void
		react_bottom_extents_spin_box_value_changed(
				double value);
		void
		react_left_extents_spin_box_value_changed(
				double value);
		void
		react_right_extents_spin_box_value_changed(
				double value);

		void
		react_use_global_extents_button_clicked();

		void
		react_enable_compression_check_box_clicked();

	private:

		explicit
		ExportRasterOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportRasterAnimationStrategy::const_configuration_ptr &
						export_configuration);


		void
		make_signal_slot_connections();

		void
		update_raster_dimensions();


		GPlatesGui::ExportRasterAnimationStrategy::Configuration d_export_configuration;

	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTRASTEROPTIONSWIDGET_H
