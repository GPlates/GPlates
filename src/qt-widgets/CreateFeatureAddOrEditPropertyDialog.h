/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_CREATEFEATUREADDOREDITPROPERTYDIALOG_H
#define GPLATES_QTWIDGETS_CREATEFEATUREADDOREDITPROPERTYDIALOG_H

#include <boost/optional.hpp>
#include <QDialog>

#include "ui_CreateFeatureAddOrEditPropertyDialogUi.h"

#include "model/GpgimProperty.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"
#include "model/TopLevelProperty.h"

#include "property-values/StructuralType.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class EditWidgetGroupBox;

	class CreateFeatureAddOrEditPropertyDialog: 
			public QDialog,
			protected Ui_CreateFeatureAddOrEditPropertyDialog
	{
		Q_OBJECT
		
	public:

		explicit
		CreateFeatureAddOrEditPropertyDialog(
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~CreateFeatureAddOrEditPropertyDialog()
		{  }
			
		
		/**
		 * Pops up the CreateFeatureAddOrEditPropertyDialog as a modal dialog and
		 * allows user to create a feature property identified by the specified GPGIM property.
		 *
		 * Returns boost::none if the user canceled dialog or there was an error.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
		add_property(
				const GPlatesModel::GpgimProperty &gpgim_feature_property);

		/**
		 * Pops up the CreateFeatureAddOrEditPropertyDialog as a modal dialog and
		 * allows user to edit the specified feature property.
		 */
		void
		edit_property(
				const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property);

		/**
		 * Returns true if the specified GPGIM property has at least one structural type that is
		 * supported by an edit widget.
		 */
		bool
		is_property_supported(
				const GPlatesModel::GpgimProperty &gpgim_property) const;

	private Q_SLOTS:

		void
		set_appropriate_edit_widget_by_property_value_type();

		void
		create_property_from_edit_widget();

		void
		update_property_from_edit_widget();

	private:

		//! Associates a GPGIM property to the created feature property when adding a new property.
		struct AddProperty
		{
			explicit
			AddProperty(
					const GPlatesModel::GpgimProperty &gpgim_property_) :
				gpgim_property(&gpgim_property_)
			{  }

			const GPlatesModel::GpgimProperty *gpgim_property;
			boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> feature_property;
		};


		void
		set_up_edit_widgets();

		void
		connect_to_combobox_property_type_signals(
				bool connects_signal_slot);

		void
		populate_add_property_type_combobox(
				const GPlatesModel::GpgimProperty &gpgim_property);

		void
		populate_edit_property_type_combobox(
				const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property);


		//! Used to add or edit a feature property.
		EditWidgetGroupBox *d_edit_widget_group_box;

		//! Only used to store feature property when 'adding' a property (as opposed to editing).
		boost::optional<AddProperty> d_add_property;
	};
}

#endif  // GPLATES_QTWIDGETS_CREATEFEATUREADDOREDITPROPERTYDIALOG_H
