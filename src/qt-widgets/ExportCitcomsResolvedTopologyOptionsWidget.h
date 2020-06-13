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

#ifndef GPLATES_QT_WIDGETS_EXPORTCITCOMSRESOLVEDTOPOLOGYOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTCITCOMSRESOLVEDTOPOLOGYOPTIONSWIDGET_H

#include "ui_ExportCitcomsResolvedTopologyOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportCitcomsResolvedTopologyAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	class DatelineWrapOptionsWidget;

	/**
	 * CitcomS-specific resolved topology export options.
	 */
	class ExportCitcomsResolvedTopologyOptionsWidget :
			public ExportOptionsWidget,
			protected Ui_ExportCitcomsResolvedTopologyOptionsWidget
	{
		Q_OBJECT

	public:
		/**
		 * Creates a @a ExportCitcomsResolvedTopologyOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::const_configuration_ptr &
						export_configuration,
				bool configure_dateline_wrapping)
		{
			return new ExportCitcomsResolvedTopologyOptionsWidget(
					parent, export_configuration, configure_dateline_wrapping);
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
		react_check_box_state_changed(
				int state);

	private:
		explicit
		ExportCitcomsResolvedTopologyOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::const_configuration_ptr &
						export_configuration,
				bool configure_dateline_wrapping);


		void
		make_signal_slot_connections();

		GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::Configuration d_export_configuration;

		DatelineWrapOptionsWidget *d_dateline_wrap_options_widget;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTCITCOMSRESOLVEDTOPOLOGYOPTIONSWIDGET_H
