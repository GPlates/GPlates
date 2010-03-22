/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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

#include "app-logic/Reconstruct.h"
#include "gui/FeatureFocus.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/QueryFeaturePropertiesWidgetPopulator.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/LatLonPoint.h"
#include "property-values/GpmlPlateId.h"
#include "utils/UnicodeStringUtils.h"
#include "presentation/ViewState.h"


GPlatesQtWidgets::QueryFeaturePropertiesWidget::QueryFeaturePropertiesWidget(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_reconstruct_ptr(&view_state_.get_reconstruct())
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

	QObject::connect(d_reconstruct_ptr,
			SIGNAL(reconstructed(GPlatesAppLogic::Reconstruct &, bool, bool)),
			this,
			SLOT(refresh_display()));
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
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type focused_rg)
{
	d_feature_ref = feature_ref;
	d_focused_rg = focused_rg;

	refresh_display();
}


void
GPlatesQtWidgets::QueryFeaturePropertiesWidget::refresh_display()
{
	if ( ! d_feature_ref.is_valid()) {
		// Always check your weak-refs, even if they should be valid because
		// FeaturePropertiesDialog promised it'd check them, because in this
		// one case we can also get updated directly when the reconstruction
		// time changes.
		return;
	}
	
	// Update our text fields at the top.
	lineedit_feature_id->setText(
			GPlatesUtils::make_qstring_from_icu_string(d_feature_ref->handle_data().feature_id().get()));
	lineedit_revision_id->setText(
			GPlatesUtils::make_qstring_from_icu_string(d_feature_ref->revision_id().get()));

	// These next few fields only make sense if the feature is reconstructable, ie. if it has a
	// reconstruction plate ID.
	static const GPlatesModel::PropertyName plate_id_property_name = 
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;
	if (GPlatesFeatureVisitors::get_property_value(
			d_feature_ref, plate_id_property_name, recon_plate_id))
	{
		// The feature has a reconstruction plate ID.
		set_plate_id(recon_plate_id->value());

		GPlatesModel::integer_plate_id_type root_plate_id =
				d_reconstruct_ptr->get_current_anchored_plate_id();
		set_root_plate_id(root_plate_id);
		set_reconstruction_time(
				d_reconstruct_ptr->get_current_reconstruction_time());

		// Now let's use the reconstruction plate ID of the feature to find the appropriate 
		// absolute rotation in the reconstruction tree.
		GPlatesModel::ReconstructionTree &recon_tree =
				d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree();
		std::pair<GPlatesMaths::FiniteRotation,
				GPlatesModel::ReconstructionTree::ReconstructionCircumstance>
				absolute_rotation =
						recon_tree.get_composed_absolute_rotation(recon_plate_id->value());

		// FIXME:  Do we care about the reconstruction circumstance?
		// (For example, there may have been no match for the reconstruction plate ID.)
		const GPlatesMaths::UnitQuaternion3D &uq = absolute_rotation.first.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(uq)) {
			set_euler_pole(QObject::tr("indeterminate"));
			set_angle(0.0);
		} else {
			using namespace GPlatesMaths;

			UnitQuaternion3D::RotationParams params =
					uq.get_rotation_params(absolute_rotation.first.axis_hint());

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

	//PROFILE_BLOCK("QueryFeaturePropertiesWidgetPopulator");

	GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator populator(
			property_tree());
	populator.populate(d_feature_ref, d_focused_rg);
}

