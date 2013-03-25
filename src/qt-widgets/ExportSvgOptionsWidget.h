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

#ifndef GPLATES_QT_WIDGETS_EXPORTSVGOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTSVGOPTIONSWIDGET_H

#include <QObject>

#include "ExportOptionsWidget.h"
#include "ExportImageResolutionOptionsWidget.h"

#include "gui/ExportSvgAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * ExportSvgOptionsWidget is used to show export options for exporting the globe/map view to SVG.
	 */
	class ExportSvgOptionsWidget :
			public ExportOptionsWidget
	{
		Q_OBJECT

	public:

		/**
		 * Creates a @a ExportSvgOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportSvgAnimationStrategy::const_configuration_ptr &export_configuration)
		{
			return new ExportSvgOptionsWidget(parent, export_animation_context, export_configuration);
		}


		/**
		 * Collects the options specified by the user and returns them as an
		 * export animation strategy configuration.
		 */
		virtual
		GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
		create_export_animation_strategy_configuration(
				const QString &filename_template);

	private:

		ExportImageResolutionOptionsWidget *d_export_image_resolution_options_widget;
		GPlatesGui::ExportSvgAnimationStrategy::Configuration d_export_configuration;


		explicit
		ExportSvgOptionsWidget(
				QWidget *parent_,
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const GPlatesGui::ExportSvgAnimationStrategy::const_configuration_ptr &export_configuration);

	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTSVGOPTIONSWIDGET_H
