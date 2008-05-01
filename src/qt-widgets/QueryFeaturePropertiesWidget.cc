/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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
#include "QueryFeaturePropertiesWidget.h"

#include "gui/FeatureFocus.h"
#include "qt-widgets/ViewportWindow.h"
#include "feature-visitors/QueryFeaturePropertiesWidgetPopulator.h"
#include "feature-visitors/PlateIdFinder.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/LatLonPointConversions.h"
#include "utils/UnicodeStringUtils.h"


GPlatesQtWidgets::QueryFeaturePropertiesWidget::QueryFeaturePropertiesWidget(
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesGui::FeatureFocus &feature_focus,
		QWidget *parent_):
	QWidget(parent_),
	d_view_state_ptr(&view_state_)
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
GPlatesQtWidgets::QueryFeaturePropertiesWidget::set_euler_pole(
		const QString &point_position)
{
	field_Euler_Pole->setText(point_position);
}


void
GPlatesQtWidgets::QueryFeaturePropertiesWidget::set_angle(
		const double &angle)
{
	// Use the default locale for the floating-point-to-string conversion.
	// (We need the underscore at the end of the variable name, because apparently there is
	// already a member of QueryFeaturePropertiesWidget named 'locale'.)
	QLocale locale_;

	field_Angle->setText(locale_.toString(angle));
}


void
GPlatesQtWidgets::QueryFeaturePropertiesWidget::set_plate_id(
		unsigned long plate_id)
{
	QString s;
	s.setNum(plate_id);
	field_Plate_ID->setText(s);
}


void
GPlatesQtWidgets::QueryFeaturePropertiesWidget::set_root_plate_id(
		unsigned long plate_id)
{
	QString s;
	s.setNum(plate_id);
	field_Root_Plate_ID->setText(s);
}


void
GPlatesQtWidgets::QueryFeaturePropertiesWidget::set_reconstruction_time(
		const double &recon_time)
{
	// Use the default locale for the floating-point-to-string conversion.
	// (We need the underscore at the end of the variable name, because apparently there is
	// already a member of QueryFeaturePropertiesWidget named 'locale'.)
	QLocale locale_;

	field_Recon_Time->setText(locale_.toString(recon_time));
}


QTreeWidget &
GPlatesQtWidgets::QueryFeaturePropertiesWidget::property_tree() const
{
	return *tree_widget_Properties;
}



void
GPlatesQtWidgets::QueryFeaturePropertiesWidget::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	// These next few fields only make sense if the feature is reconstructable, ie. if it has a
	// reconstruction plate ID.
	static const GPlatesModel::PropertyName plate_id_property_name = 
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	GPlatesFeatureVisitors::PlateIdFinder plate_id_finder(plate_id_property_name);
	plate_id_finder.visit_feature_handle(*feature_ref);
	if (plate_id_finder.found_plate_ids_begin() != plate_id_finder.found_plate_ids_end()) {
		// The feature has a reconstruction plate ID.
		GPlatesModel::integer_plate_id_type recon_plate_id =
				*plate_id_finder.found_plate_ids_begin();
		set_plate_id(recon_plate_id);

		GPlatesModel::integer_plate_id_type root_plate_id =
				view_state().reconstruction_root();
		set_root_plate_id(root_plate_id);
		set_reconstruction_time(
				view_state().reconstruction_time());

		// Now let's use the reconstruction plate ID of the feature to find the appropriate 
		// absolute rotation in the reconstruction tree.
		GPlatesModel::ReconstructionTree &recon_tree =
				view_state().reconstruction().reconstruction_tree();
		std::pair<GPlatesMaths::FiniteRotation,
				GPlatesModel::ReconstructionTree::ReconstructionCircumstance>
				absolute_rotation =
						recon_tree.get_composed_absolute_rotation(recon_plate_id);

		// FIXME:  Do we care about the reconstruction circumstance?
		// (For example, there may have been no match for the reconstruction plate ID.)
		GPlatesMaths::UnitQuaternion3D uq = absolute_rotation.first.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(uq)) {
			set_euler_pole(QObject::tr("indeterminate"));
			set_angle(0.0);
		} else {
			using namespace GPlatesMaths;

			UnitQuaternion3D::RotationParams params = uq.get_rotation_params();

			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);

			// Use the default locale for the floating-point-to-string conversion.
			// (We need the underscore at the end of the variable name, because
			// apparently there is already a member of QueryFeature named 'locale'.)
			QLocale locale_;
			QString euler_pole_lat = locale_.toString(llp.latitude());
			QString euler_pole_lon = locale_.toString(llp.longitude());
			QString euler_pole_as_string;
			euler_pole_as_string.append(euler_pole_lat);
			euler_pole_as_string.append(QObject::tr(" ; "));
			euler_pole_as_string.append(euler_pole_lon);

			set_euler_pole(euler_pole_as_string);

			const double &angle = radiansToDegrees(params.angle).dval();
			set_angle(angle);
		}
	}

	GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator populator(
			property_tree());
	populator.visit_feature_handle(*feature_ref);
}


