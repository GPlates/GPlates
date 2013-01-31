/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_EXPORTVELOCITYOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTVELOCITYOPTIONSWIDGET_H

#include "ExportVelocityOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportVelocityAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	class ExportFileOptionsWidget;

	/**
	 * General (non-CitcomS-specific) resolved topology export options.
	 */
	class ExportVelocityOptionsWidget :
			public ExportOptionsWidget,
			protected Ui_ExportVelocityOptionsWidget
	{
		Q_OBJECT

	public:
		/**
		 * Creates a @a ExportVelocityOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportVelocityAnimationStrategy::const_configuration_ptr &default_export_configuration)
		{
			return new ExportVelocityOptionsWidget(parent, default_export_configuration);
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
		react_radio_button_toggled(
				bool checked);

	private:

		explicit
		ExportVelocityOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportVelocityAnimationStrategy::const_configuration_ptr &default_export_configuration);


		void
		make_signal_slot_connections();

		void
		update_output_description_label();


		GPlatesGui::ExportVelocityAnimationStrategy::Configuration d_export_configuration;

		ExportFileOptionsWidget *d_export_file_options_widget;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTVELOCITYOPTIONSWIDGET_H
