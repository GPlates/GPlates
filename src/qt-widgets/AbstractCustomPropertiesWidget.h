/* $Id: AbstractCustomPropertiesWidget.h 8461 2010-05-20 14:18:01Z rwatson $ */

/**
* \file 
* $Revision: 8461 $
* $Date: 2010-05-20 16:18:01 +0200 (to, 20 mai 2010) $ 
* 
* Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_ABSTRACTCUSTOMPROPERTIESWIDGET_H
#define GPLATES_QTWIDGETS_ABSTRACTCUSTOMPROPERTIESWIDGET_H

#include <QWidget>

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/PropertyValue.h"


namespace GPlatesQtWidgets
{
	/**
	 *  An abstract base class for special handling of feature properties.
	 */
	class AbstractCustomPropertiesWidget:
		public QWidget
	{
		
	public:

		explicit
		AbstractCustomPropertiesWidget(
			QWidget *parent_ = NULL):
			QWidget(parent_)
		{  }
		
		virtual
		~AbstractCustomPropertiesWidget()
		{  }

        virtual
        GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
        do_geometry_tasks(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstruction_time_geometry_,
				const GPlatesModel::FeatureHandle::weak_ref &feature_handle)
        {
            return reconstruction_time_geometry_;
        }

	};
}

#endif // GPLATES_QTWIDGETS_ABSTRACTCUSTOMPROPERTIESWIDGET_H

