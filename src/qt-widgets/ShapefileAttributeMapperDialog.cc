/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2009 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "ShapefileAttributeMapperDialog.h"

#include "ShapefilePropertyMapper.h"
#include "ShapefileAttributeWidget.h"
#include "QtWidgetUtils.h"

#include "model/FeatureCollectionHandle.h"
#include "model/TopLevelProperty.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"
#include "utils/UnicodeStringUtils.h"


GPlatesQtWidgets::ShapefileAttributeMapperDialog::ShapefileAttributeMapperDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setupUi(this);

	// Button box signals.
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(accept()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(clicked(QAbstractButton *)),
			this,
			SLOT(handle_buttonbox_clicked(QAbstractButton *)));
}


void
GPlatesQtWidgets::ShapefileAttributeMapperDialog::setup(
		QString &filename,
		QStringList &field_names,
		QMap< QString,QString > &model_to_attribute_map)
{
	d_shapefile_attribute_widget = new ShapefileAttributeWidget(this,filename,field_names,model_to_attribute_map,false);
	QtWidgetUtils::add_widget_to_placeholder(
			d_shapefile_attribute_widget,
			widget_shapefile_attribute);
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


void
GPlatesQtWidgets::ShapefileAttributeMapperDialog::handle_buttonbox_clicked(
		QAbstractButton *button)
{
	if (main_buttonbox->buttonRole(button) == QDialogButtonBox::ResetRole)
	{
		reset_fields();
	}
}

