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
 
#ifndef GPLATES_QTWIDGETS_CHOOSEPROPERTYWIDGET_H
#define GPLATES_QTWIDGETS_CHOOSEPROPERTYWIDGET_H

#include <vector>
#include <QWidget>

#include "SelectionWidget.h"

#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/GpgimProperty.h"
#include "model/PropertyName.h"

#include "property-values/StructuralType.h"


namespace GPlatesModel
{
	class Gpgim;
}


namespace GPlatesQtWidgets
{
	/**
	 * ChoosePropertyWidget encapsulates a widget that offers the user a selection of
	 * property names that can be used with a particular feature type and property structural type.
	 *
	 * It is used, for example, by the CreateFeatureDialog.
	 */
	class ChoosePropertyWidget :
			public QWidget
	{
		Q_OBJECT

	public:

		/**
		 * Returns the list of GPGIM properties that @a populate will show to the user.
		 *
		 * Returns false if no properties will be shown.
		 */
		static
		bool
		get_properties_to_populate(
				std::vector<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &gpgim_properties,
				const GPlatesModel::Gpgim &gpgim,
				const GPlatesModel::FeatureType &target_feature_type,
				const GPlatesPropertyValues::StructuralType &target_property_type,
				const GPlatesModel::FeatureHandle::weak_ref &source_feature_ref =
						GPlatesModel::FeatureHandle::weak_ref());


		explicit
		ChoosePropertyWidget(
				const GPlatesModel::Gpgim &gpgim,
				SelectionWidget::DisplayWidget display_widget,
				QWidget *parent_ = NULL);

		/**
		 * Causes this widget to show properties appropriate for the specified
		 * feature type and property type.
		 *
		 * If the optional source feature is specified then properties that can occur at most once
		 * will not be shown if they already exist in the feature.
		 *
		 * If the previously selected property name (if any) is in the new list of property names
		 * then the selection is retained.
		 */
		void
		populate(
				const GPlatesModel::FeatureType &target_feature_type,
				const GPlatesPropertyValues::StructuralType &target_property_type,
				const GPlatesModel::FeatureHandle::weak_ref &source_feature_ref =
						GPlatesModel::FeatureHandle::weak_ref());

		/**
		 * Returns the currently selected property name.
		 */
		boost::optional<GPlatesModel::PropertyName>
		get_property_name() const;

		/**
		 * Changes the currently selected property name to @a property_name.
		 */
		void
		set_property_name(
				const GPlatesModel::PropertyName &property_name);

	Q_SIGNALS:

		void
		item_activated();

	private Q_SLOTS:

		void
		handle_item_activated(
				int index);

	private:

		const GPlatesModel::Gpgim &d_gpgim;

		SelectionWidget *d_selection_widget;

	};
}


#endif	// GPLATES_QTWIDGETS_CHOOSEPROPERTYWIDGET_H
