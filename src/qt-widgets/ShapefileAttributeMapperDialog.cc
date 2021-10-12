/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2009 Geological Survey of Norway
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
#include <algorithm> // std::find
#include <iostream>

#include <QMap>
#include <QString>

#include "model/FeatureCollectionHandle.h"
#include "model/TopLevelProperty.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"
#include "utils/UnicodeStringUtils.h"

#include "ShapefilePropertyMapper.h"
#include "ShapefileAttributeWidget.h"
#include "ShapefileAttributeMapperDialog.h"



GPlatesQtWidgets::ShapefileAttributeMapperDialog::ShapefileAttributeMapperDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setupUi(this);

	QObject::connect(button_reset,SIGNAL(clicked()),this,SLOT(reset_fields()));
}

void
GPlatesQtWidgets::ShapefileAttributeMapperDialog::setup(
		QString &filename,
		QStringList &field_names,
		QMap< QString,QString > &model_to_attribute_map)
{
	d_shapefile_attribute_widget = new ShapefileAttributeWidget(this,filename,field_names,model_to_attribute_map,false);
	gridLayout->addWidget(d_shapefile_attribute_widget,0,0);

}


void
GPlatesQtWidgets::ShapefileAttributeMapperDialog::accept()
{
	d_shapefile_attribute_widget->accept_fields();	
	done(QDialog::Accepted);
}

void
GPlatesQtWidgets::ShapefileAttributeMapperDialog::reset_fields()
{
	d_shapefile_attribute_widget->reset_fields();
}

