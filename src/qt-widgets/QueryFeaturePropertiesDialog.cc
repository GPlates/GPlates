/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#include <QLocale>
#include "QueryFeaturePropertiesDialog.h"


GPlatesQtWidgets::QueryFeaturePropertiesDialog::QueryFeaturePropertiesDialog(
		QWidget *parent_):
	QDialog(parent_)
{
	setupUi(this);

	tree_widget_Properties->setColumnWidth(0, 230);

	field_Euler_Pole->setMinimumSize(150, 27);
	field_Euler_Pole->setMaximumSize(150, 27);
	field_Angle->setMinimumSize(75, 27);
	field_Angle->setMaximumSize(75, 27);
	field_Plate_ID->setMaximumSize(50, 27);
	field_Root_Plate_ID->setMaximumSize(50, 27);
	field_Recon_Time->setMaximumSize(50, 27);
}


void
GPlatesQtWidgets::QueryFeaturePropertiesDialog::set_feature_type(
		const QString &feature_type)
{
	field_Feature_Type->setText(feature_type);
}


void
GPlatesQtWidgets::QueryFeaturePropertiesDialog::set_euler_pole(
		const QString &point_position)
{
	field_Euler_Pole->setText(point_position);
}


void
GPlatesQtWidgets::QueryFeaturePropertiesDialog::set_angle(
		const double &angle)
{
	// Use the default locale for the floating-point-to-string conversion.
	// (We need the underscore at the end of the variable name, because apparently there is
	// already a member of QueryFeaturePropertiesDialog named 'locale'.)
	QLocale locale_;

	field_Angle->setText(locale_.toString(angle));
}


void
GPlatesQtWidgets::QueryFeaturePropertiesDialog::set_plate_id(
		unsigned long plate_id)
{
	QString s;
	s.setNum(plate_id);
	field_Plate_ID->setText(s);
}


void
GPlatesQtWidgets::QueryFeaturePropertiesDialog::set_root_plate_id(
		unsigned long plate_id)
{
	QString s;
	s.setNum(plate_id);
	field_Root_Plate_ID->setText(s);
}


void
GPlatesQtWidgets::QueryFeaturePropertiesDialog::set_reconstruction_time(
		const double &recon_time)
{
	// Use the default locale for the floating-point-to-string conversion.
	// (We need the underscore at the end of the variable name, because apparently there is
	// already a member of QueryFeaturePropertiesDialog named 'locale'.)
	QLocale locale_;

	field_Recon_Time->setText(locale_.toString(recon_time));
}


QTreeWidget &
GPlatesQtWidgets::QueryFeaturePropertiesDialog::property_tree() const
{
	return *tree_widget_Properties;
}
