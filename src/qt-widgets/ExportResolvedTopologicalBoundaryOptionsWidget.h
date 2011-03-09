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

#ifndef GPLATES_QT_WIDGETS_EXPORTRESOLVEDTOPOLOGICALBOUNDARYOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTRESOLVEDTOPOLOGICALBOUNDARYOPTIONSWIDGET_H

#include "ExportResolvedTopologicalBoundaryOptionsWidgetUi.h"

#include "ExportOptionsWidget.h"

#include "gui/ExportResolvedTopologyAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * RasterLayerOptionsWidget is used to show additional options for raster
	 * layers in the visual layers widget.
	 */
	class ExportResolvedTopologicalBoundaryOptionsWidget :
			public ExportOptionsWidget,
			protected Ui_ExportResolvedTopologicalBoundaryOptionsWidget
	{
		Q_OBJECT

	public:
		/**
		 * Creates a @a ExportResolvedTopologicalBoundaryOptionsWidget containing default export options.
		 */
		static
		ExportOptionsWidget *
		create(
				QWidget *parent,
				const GPlatesGui::ExportResolvedTopologyAnimationStrategy::const_configuration_ptr &
						default_export_configuration)
		{
			return new ExportResolvedTopologicalBoundaryOptionsWidget(parent, default_export_configuration);
		}


		/**
		 * Collects the options specified by the user and
		 * returns them as an export animation strategy configuration.
		 */
		virtual
		GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
		create_export_animation_strategy_configuration(
				const QString &filename_template);

	private slots:
		void
		react_check_box_state_changed(
				int state);

	private:
		explicit
		ExportResolvedTopologicalBoundaryOptionsWidget(
				QWidget *parent_,
				const GPlatesGui::ExportResolvedTopologyAnimationStrategy::const_configuration_ptr &
						default_export_configuration);


		void
		make_signal_slot_connections();

		GPlatesGui::ExportResolvedTopologyAnimationStrategy::Configuration d_export_configuration;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTRESOLVEDTOPOLOGICALBOUNDARYOPTIONSWIDGET_H
