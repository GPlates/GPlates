/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
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

#include <boost/foreach.hpp>
#include <QListWidget>
#include <QListWidgetItem>

#include "ChooseFeatureTypeWidget.h"

#include "QtWidgetUtils.h"

#include "model/GPGIMInfo.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	class DefaultConstructibleFeatureType
	{
	public:

		DefaultConstructibleFeatureType()
		{  }

		DefaultConstructibleFeatureType(
				const GPlatesModel::FeatureType &feature_type) :
			d_feature_type(feature_type)
		{  }

		operator GPlatesModel::FeatureType() const
		{
			return *d_feature_type;
		}

		bool
		operator==(
				const DefaultConstructibleFeatureType &other)
		{
			return d_feature_type == other.d_feature_type;
		}

	private:

		boost::optional<GPlatesModel::FeatureType> d_feature_type;
	};


	/**
	 * Fill the list with possible feature types we can create.
	 */
	void
	populate_feature_types_list(
			GPlatesQtWidgets::SelectionWidget &selection_widget,
			bool topological)
	{
		// FIXME: For extra brownie points, filter -this- list based on features
		// which you couldn't possibly create given the digitised geometry.
		// E.g. no Cratons made from PolylineOnSphere!
		typedef GPlatesModel::GPGIMInfo::feature_set_type feature_set_type;
		const feature_set_type &list = GPlatesModel::GPGIMInfo::get_feature_set(topological);

		selection_widget.clear();

		// Add all the feature types from the finished list.
		BOOST_FOREACH(const GPlatesModel::FeatureType &feature_type, list)
		{
			selection_widget.add_item<DefaultConstructibleFeatureType>(
					GPlatesUtils::make_qstring_from_icu_string(feature_type.build_aliased_name()),
					feature_type);
		}

		if (selection_widget.get_count())
		{
			selection_widget.set_current_index(0);
		}
	}
}


Q_DECLARE_METATYPE( DefaultConstructibleFeatureType )


GPlatesQtWidgets::ChooseFeatureTypeWidget::ChooseFeatureTypeWidget(
		SelectionWidget::DisplayWidget display_widget,
		QWidget *parent_) :
	QWidget(parent_),
	d_selection_widget(new SelectionWidget(display_widget, this))
{
	QtWidgetUtils::add_widget_to_placeholder(d_selection_widget, this);

	make_signal_slot_connections();
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::populate(
		bool topological)
{
	populate_feature_types_list(*d_selection_widget, topological);
}


boost::optional<GPlatesModel::FeatureType>
GPlatesQtWidgets::ChooseFeatureTypeWidget::get_feature_type() const
{
	return boost::optional<GPlatesModel::FeatureType>(
			d_selection_widget->get_data<DefaultConstructibleFeatureType>(
				d_selection_widget->get_current_index()));
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::set_feature_type(
		const GPlatesModel::FeatureType &feature_type)
{
	int index = d_selection_widget->find_data<DefaultConstructibleFeatureType>(feature_type);
	d_selection_widget->set_current_index(index);
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::focusInEvent(
		QFocusEvent *ev)
{
	d_selection_widget->setFocus();
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::handle_item_activated(
		int index)
{
	emit item_activated();
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::handle_current_index_changed(
		int index)
{
	emit current_index_changed(
			boost::optional<GPlatesModel::FeatureType>(
				d_selection_widget->get_data<DefaultConstructibleFeatureType>(index)));
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::make_signal_slot_connections()
{
	QObject::connect(
			d_selection_widget,
			SIGNAL(item_activated(int)),
			this,
			SLOT(handle_item_activated(int)));
	QObject::connect(
			d_selection_widget,
			SIGNAL(current_index_changed(int)),
			this,
			SLOT(handle_current_index_changed(int)));
}

