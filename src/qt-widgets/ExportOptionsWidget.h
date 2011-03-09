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

#ifndef GPLATES_QT_WIDGETS_EXPORTOPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_EXPORTOPTIONSWIDGET_H

#include <QWidget>
#include <QString>

#include "gui/ExportAnimationStrategy.h"


namespace GPlatesQtWidgets
{
	/**
	 * This is the abstract base class of widgets used to display export options
	 * particular to different export animation types.
	 */
	class ExportOptionsWidget :
			public QWidget
	{
	public:

		ExportOptionsWidget(
				QWidget *parent_) :
			QWidget(parent_)
		{  }

		virtual
		~ExportOptionsWidget()
		{  }


		/**
		 * Collects the options specified by the user and
		 * returns them as an export animation strategy configuration.
		 */
		virtual
		GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
		create_export_animation_strategy_configuration(
				const QString &filename_template) const = 0;
	};
}

#endif // GPLATES_QT_WIDGETS_EXPORTOPTIONSWIDGET_H