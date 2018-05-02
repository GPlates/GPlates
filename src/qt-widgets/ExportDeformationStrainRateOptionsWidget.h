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

#ifndef GPLATES_QT_WIDGETS_EXPORTDEFORMATIONSTRAINRATEOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTDEFORMATIONSTRAINRATEOPTIONSWIDGET_H

#include "ExportDeformationStrainRateOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportDeformationStrainRateAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	class ExportFileOptionsWidget;

	/**
	 * ExportDeformationStrainRateOptionsWidget is used to show export options for exporting deformation strain rates.
	 */
	class ExportDeformationStrainRateOptionsWidget :
			public ExportOptionsWidget,
			protected Ui_ExportDeformationStrainRateOptionsWidget
	{
		Q_OBJECT

	public:
		/**
		 * Creates a @a ExportDeformationStrainRateOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportDeformationStrainRateAnimationStrategy::const_configuration_ptr &export_configuration)
		{
			return new ExportDeformationStrainRateOptionsWidget(parent, export_configuration);
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
		react_gmt_domain_point_format_radio_button_toggled(
				bool checked);

		void
		react_include_dilatation_strain_rate_check_box_clicked();

		void
		react_include_second_invariant_check_box_clicked();

	private:

		explicit
		ExportDeformationStrainRateOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportDeformationStrainRateAnimationStrategy::const_configuration_ptr &export_configuration);

		void
		make_signal_slot_connections();

		void
		update_output_description_label();


		GPlatesGui::ExportDeformationStrainRateAnimationStrategy::configuration_ptr d_export_configuration;

		ExportFileOptionsWidget *d_export_file_options_widget;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTDEFORMATIONSTRAINRATEOPTIONSWIDGET_H
