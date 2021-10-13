/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2010, 2011 The University of Sydney, Australia
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

#include <map>
#include <QAction>
#include <QString>

#include "SpecifyAnchoredPlateIdDialog.h"

#include "QtWidgetUtils.h"

#include "model/FeatureVisitor.h"

#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlConstantValue.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	class ExtractPlateIds :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		typedef std::map<QString, GPlatesModel::integer_plate_id_type> result_type;

		const result_type &
		get_plate_ids() const
		{
			return d_plate_ids;
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
		{
			if (current_top_level_propname())
			{
				QString property_name = GPlatesUtils::make_qstring_from_icu_string(
						current_top_level_propname()->build_aliased_name());
				d_plate_ids.insert(std::make_pair(property_name, gpml_plate_id.value()));
			}
		}

	private:

		result_type d_plate_ids;
	};
}


GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::SpecifyAnchoredPlateIdDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_fill_menu(new QMenu(this))
{
	setupUi(this);

	// Set up fill button's menu.
	fill_toolbutton->setMenu(d_fill_menu);
	QObject::connect(
			d_fill_menu,
			SIGNAL(triggered(QAction *)),
			this,
			SLOT(handle_action_triggered(QAction *)));

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

	// Notify listeners about a change of plate ID when the user clicks OK.
	QObject::connect(
			this,
			SIGNAL(accepted()),
			this,
			SLOT(propagate_value()));

	// Reset button.
	QObject::connect(
			reset_button,
			SIGNAL(clicked()),
			this,
			SLOT(reset_to_zero()));

	QtWidgetUtils::resize_based_on_size_hint(this);
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::populate(
		GPlatesModel::integer_plate_id_type plate_id,
		const GPlatesModel::FeatureHandle::weak_ref &focused_feature)
{
	populate_spinbox(plate_id);
	populate_menu(focused_feature);
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::showEvent(
		QShowEvent *ev)
{
	fixed_plate_spinbox->setFocus();
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::propagate_value()
{
	emit value_changed(
			static_cast<GPlatesModel::integer_plate_id_type>(
				fixed_plate_spinbox->value()));
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::handle_action_triggered(
		QAction *action)
{
	QVariant qv = action->data();
	bool ok;
	uint plate_id = qv.toUInt(&ok);
	if (ok)
	{
		populate_spinbox(static_cast<GPlatesModel::integer_plate_id_type>(plate_id));
	}
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::reset_to_zero()
{
	fixed_plate_spinbox->setValue(0);
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::populate_spinbox(
		GPlatesModel::integer_plate_id_type plate_id)
{
	// Update the spinbox's value. Note that the range of the spinbox is narrower
	// than the range of possible plate IDs.
	if (plate_id >= static_cast<GPlatesModel::integer_plate_id_type>(fixed_plate_spinbox->minimum()) &&
			plate_id <= static_cast<GPlatesModel::integer_plate_id_type>(fixed_plate_spinbox->maximum()))
	{
		fixed_plate_spinbox->setValue(static_cast<int>(plate_id));
	}
	else
	{
		fixed_plate_spinbox->setValue(fixed_plate_spinbox->minimum());
	}

	fixed_plate_spinbox->selectAll();
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::populate_menu(
		const GPlatesModel::FeatureHandle::weak_ref &focused_feature)
{
	// Find out what plate IDs are in the given feature.
	ExtractPlateIds visitor;
	if (focused_feature.is_valid())
	{
		visitor.visit_feature(focused_feature);
	}

	// Clear the menu and repopulate it.
	d_fill_menu->clear();
	const ExtractPlateIds::result_type &plate_ids = visitor.get_plate_ids();
	for (ExtractPlateIds::result_type::const_iterator iter = plate_ids.begin();
			iter != plate_ids.end(); ++iter)
	{
		QAction *action = new QAction(iter->first, d_fill_menu);
		action->setData(static_cast<uint>(iter->second));
		d_fill_menu->addAction(action);
	}

	fill_toolbutton->setEnabled(plate_ids.size() > 0);
}

