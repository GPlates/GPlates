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

#include <QListWidget>
#include <QListWidgetItem>

#include "ChooseFeatureTypeWidget.h"

#include "model/GPGIMInfo.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	/**
	 * Subclass of QListWidgetItem so that we can display QualifiedXmlNames in the QListWidget
	 * without converting them to a QString (and thus forgetting that we had a QualifiedXmlName
	 * in the first place).
	 */
	class FeatureTypeItem :
			public QListWidgetItem
	{
	public:
		FeatureTypeItem(
				const GPlatesModel::FeatureType type_):
			QListWidgetItem(GPlatesUtils::make_qstring_from_icu_string(type_.build_aliased_name())),
			d_type(type_)
		{  }
		
		const GPlatesModel::FeatureType
		get_type()
		{
			return d_type;
		}
			
	private:
		const GPlatesModel::FeatureType d_type;
	};


	/**
	 * Fill the list with possible feature types we can create.
	 */
	void
	populate_feature_types_list(
			QListWidget &list_widget,
			bool topological)
	{
		// FIXME: For extra brownie points, filter -this- list based on features
		// which you couldn't possibly create given the digitised geometry.
		// E.g. no Cratons made from PolylineOnSphere!
		typedef const GPlatesModel::GPGIMInfo::feature_set_type feature_set_type;
		const feature_set_type &list = GPlatesModel::GPGIMInfo::get_feature_set(topological);

		list_widget.clear();

		// Add all the feature types from the finished list.
		feature_set_type::const_iterator list_it = list.begin();
		feature_set_type::const_iterator list_end = list.end();
		for ( ; list_it != list_end; ++list_it) {
			list_widget.addItem(new FeatureTypeItem(*list_it));
		}

		// Set the default field to UnclassifiedFeature. 
		QList<QListWidgetItem*> unclassified_items = list_widget.findItems(
			QString("gpml:UnclassifiedFeature"),Qt::MatchFixedString);
		if (unclassified_items.isEmpty())
		{
			list_widget.setCurrentRow(0);
		}
		else
		{
			list_widget.setCurrentItem(unclassified_items.first());
		}
	}

	/**
	 * Get the FeatureType the user has selected.
	 */
	boost::optional<const GPlatesModel::FeatureType>
	currently_selected_feature_type(
			const QListWidget *listwidget_feature_types)
	{
		FeatureTypeItem *type_item = dynamic_cast<FeatureTypeItem *>(
				listwidget_feature_types->currentItem());
		if (type_item == NULL) {
			return boost::none;
		}
		return type_item->get_type();
	}

}


GPlatesQtWidgets::ChooseFeatureTypeWidget::ChooseFeatureTypeWidget(
		QWidget *parent_) :
	QGroupBox(parent_)
{
	setupUi(this);

#if 0
	listwidget_feature_collections->setFocus();

	// Emit signal if the user pushes Enter or double-clicks on the list.
	QObject::connect(
			listwidget_feature_collections,
			SIGNAL(itemActivated(QListWidgetItem *)),
			this,
			SLOT(handle_listwidget_item_activated(QListWidgetItem *)));
#endif
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::initialise(
		bool topological)
{
	populate_feature_types_list(*listwidget_feature_types, topological);
}


void
GPlatesQtWidgets::ChooseFeatureTypeWidget::handle_listwidget_item_activated(
		QListWidgetItem *)
{
	emit item_activated();
}


boost::optional<const GPlatesModel::FeatureType>
GPlatesQtWidgets::ChooseFeatureTypeWidget::get_feature_type() const
{
	return currently_selected_feature_type(listwidget_feature_types);
}

