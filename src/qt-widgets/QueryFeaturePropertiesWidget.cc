/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/QueryFeaturePropertiesWidgetPopulator.h"

#include "gui/FeatureFocus.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"

#include "presentation/ViewState.h"

#include "property-values/GpmlPlateId.h"

#include "utils/UnicodeStringUtils.h"


GPlatesQtWidgets::QueryFeaturePropertiesWidget::QueryFeaturePropertiesWidget(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_application_state_ptr(&view_state_.get_application_state()),
	d_populate_property_tree_when_visible(false)
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

	QObject::connect(d_application_state_ptr,
			SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
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
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_rg)
{
	d_feature_ref = feature_ref;
	d_focused_rg = focused_rg;

	refresh_display();
}


void
GPlatesQtWidgets::QueryFeaturePropertiesWidget::refresh_display()
{
	if ( ! d_feature_ref.is_valid() || ! d_focused_rg) {
		// Always check your weak-refs, even if they should be valid because
		// FeaturePropertiesDialog promised it'd check them, because in this
		// one case we can also get updated directly when the reconstruction
		// time changes.
		return;
	}
	
	// Update our text fields at the top.
	lineedit_feature_id->setText(
			GPlatesUtils::make_qstring_from_icu_string(d_feature_ref->feature_id().get()));
	lineedit_revision_id->setText(
			GPlatesUtils::make_qstring_from_icu_string(d_feature_ref->revision_id().get()));

	const GPlatesModel::integer_plate_id_type root_plate_id =
				d_application_state_ptr->get_current_anchored_plate_id();
	const double reconstruction_time = d_application_state_ptr->get_current_reconstruction_time();

	// These next few fields only make sense if the feature is reconstructable, ie. if it has a
	// reconstruction plate ID.
	QString euler_pole_as_string = QObject::tr("indeterminate");
	double angle = 0;
	GPlatesModel::integer_plate_id_type plate_id = 0;

	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> focused_rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry *>(d_focused_rg);
	if (focused_rfg &&
		// We explicitly calculate finite rotation by plate id (so not interested in half-stage, etc)...
		focused_rfg.get()->get_reconstruct_method_type() == GPlatesAppLogic::ReconstructMethod::BY_PLATE_ID &&
		focused_rfg.get()->reconstruction_plate_id())
	{
		// The feature has a reconstruction plate ID.
		plate_id = focused_rfg.get()->reconstruction_plate_id().get();

		// Now let's use the reconstruction plate ID of the feature to find the appropriate 
		// absolute rotation in the reconstruction tree.
		const GPlatesAppLogic::ReconstructionTree &recon_tree = *focused_rfg.get()->get_reconstruction_tree();
		std::pair<GPlatesMaths::FiniteRotation,
				GPlatesAppLogic::ReconstructionTree::ReconstructionCircumstance>
				absolute_rotation =
						recon_tree.get_composed_absolute_rotation(plate_id);

		// FIXME:  Do we care about the reconstruction circumstance?
		// (For example, there may have been no match for the reconstruction plate ID.)
		const GPlatesMaths::UnitQuaternion3D &uq = absolute_rotation.first.unit_quat();
		if (!GPlatesMaths::represents_identity_rotation(uq))
		{
			using namespace GPlatesMaths;

			GPlatesMaths::UnitQuaternion3D::RotationParams params =
					uq.get_rotation_params(absolute_rotation.first.axis_hint());

			GPlatesMaths::PointOnSphere euler_pole(params.axis);
			GPlatesMaths::LatLonPoint llp = make_lat_lon_point(euler_pole);

			// Use the default locale for the floating-point-to-string conversion.
			// (We need the underscore at the end of the variable name, because
			// apparently there is already a member of QueryFeature named 'locale'.)
			QLocale locale_;
			QString euler_pole_lat = locale_.toString(llp.latitude());
			QString euler_pole_lon = locale_.toString(llp.longitude());
			euler_pole_as_string.clear();
			euler_pole_as_string.append(euler_pole_lat);
			euler_pole_as_string.append(QObject::tr(" ; "));
			euler_pole_as_string.append(euler_pole_lon);

			angle = GPlatesMaths::convert_rad_to_deg(params.angle).dval();
		}
	}

	set_reconstruction_time(reconstruction_time);
	set_root_plate_id(root_plate_id);
	set_plate_id(plate_id);
	set_euler_pole(euler_pole_as_string);
	set_angle(angle);

	if (isVisible())
	{
		GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator populator(property_tree());
		GPlatesModel::FeatureHandle::const_weak_ref const_feature = d_feature_ref;
		populator.populate(const_feature, d_focused_rg);

		d_populate_property_tree_when_visible = false;
	}
	else
	{
		// Delay populating the property tree widget until it is actually visible.
		d_populate_property_tree_when_visible = true;
	}
}


void
GPlatesQtWidgets::QueryFeaturePropertiesWidget::showEvent(
		QShowEvent *event_)
{
	if (d_populate_property_tree_when_visible)
	{
		GPlatesFeatureVisitors::QueryFeaturePropertiesWidgetPopulator populator(property_tree());
		GPlatesModel::FeatureHandle::const_weak_ref const_feature = d_feature_ref;
		populator.populate(const_feature, d_focused_rg);

		d_populate_property_tree_when_visible = false;
	}
}	
