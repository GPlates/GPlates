/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_FEATURESUMMARYWIDGET_H
#define GPLATES_QTWIDGETS_FEATURESUMMARYWIDGET_H

#include <QWidget>
#include "FeatureSummaryWidgetUi.h"

#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"

// An effort to reduce the dependency spaghetti currently plaguing the GUI.
namespace GPlatesGui
{
	class FeatureFocus;
}


namespace GPlatesQtWidgets
{
	class FeatureSummaryWidget:
			public QWidget, 
			protected Ui_FeatureSummaryWidget
	{
		Q_OBJECT
		
	public:
		explicit
		FeatureSummaryWidget(
				GPlatesGui::FeatureFocus &feature_focus,
				QWidget *parent_ = NULL);
					
	public slots:
		
		void
		clear();

		void
		display_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);
	
	private:

		void
		hide_plate_id_fields_as_appropriate();
	
	};
}

#endif  // GPLATES_QTWIDGETS_FEATURESUMMARYWIDGET_H

