/**
 * \file 
 * $Revision: 5534 $
 * $Date: 2009-04-20 17:17:43 -0700 (Mon, 20 Apr 2009) $ 
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
 
#ifndef GPLATES_QTWIDGETS_TOPOLOGYTOOLSWIDGET_H
#define GPLATES_QTWIDGETS_TOPOLOGYTOOLSWIDGET_H

#include <QWidget>
#include "TopologyToolsWidgetUi.h"

#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"

namespace GPlatesQtWidgets
{
	class TopologyToolsWidget:
			public QWidget, 
			protected Ui_TopologyToolsWidget
	{
		Q_OBJECT
		
	public:
		explicit
		TopologyToolsWidget(
				QWidget *parent_ = NULL);
					
	public slots:
		
		void
		clear();

		void
		display_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);
	
	private:

	};
}

#endif  // GPLATES_QTWIDGETS_TOPOLOGYTOOLSWIDGET_H

