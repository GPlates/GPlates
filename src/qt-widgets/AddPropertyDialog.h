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
 
#ifndef GPLATES_QTWIDGETS_ADDPROPERTYDIALOG_H
#define GPLATES_QTWIDGETS_ADDPROPERTYDIALOG_H

#include <vector>
#include <boost/optional.hpp>
#include <QDialog>

#include "AddPropertyDialogUi.h"
#include "EditWidgetGroupBox.h"

#include "model/FeatureHandle.h"
#include "model/FeatureType.h"

#include "property-values/StructuralType.h"


namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesModel
{
	class Gpgim;
	class GpgimProperty;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class AddPropertyDialog: 
			public QDialog,
			protected Ui_AddPropertyDialog 
	{
		Q_OBJECT
		
	public:
		/**
		 * Constructs the Add Property Dialog instance.
		 */
		explicit
		AddPropertyDialog(
				GPlatesGui::FeatureFocus &feature_focus_,
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~AddPropertyDialog()
		{  }
			
	public slots:
		
		/**
		 * Set the feature, and its feature type, that the properties are being added to.
		 */
		void
		set_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);

		/**
		 * Resets dialog components to default state.
		 */
		void
		reset();

		/**
		 * Pops up the AddPropertyDialog as a modal dialog,
		 * after resetting itself to default values.
		 */
		void
		pop_up();

	private slots:

		void
		set_appropriate_edit_widget();

		void
		check_property_name_validity();
		
		void
		add_property();
	
		void
		populate_property_name_combobox();

		void
		populate_property_type_combobox();
	
	private:
	
		void
		connect_to_combobox_add_property_name_signals(
				bool connects_signal_slot);

		void
		connect_to_combobox_add_property_type_signals(
				bool connects_signal_slot);
	
		void
		set_up_add_property_box();

		void
		set_up_edit_widgets();


		//! Default feature type to use when no available feature or invalid feature reference.
		static
		const GPlatesModel::FeatureType &
		get_default_feature_type();


		//! Determines if a property name is valid for a feature type.
		const GPlatesModel::Gpgim &d_gpgim;

		//! Announce modifications to the focused feature.
		GPlatesGui::FeatureFocus &d_feature_focus;

		//! The feature that properties are being added to.
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

		//! The type of feature that properties are being added to.
		GPlatesModel::FeatureType d_feature_type;

		EditWidgetGroupBox *d_edit_widget_group_box_ptr;
	};
}

#endif  // GPLATES_QTWIDGETS_ADDPROPERTYDIALOG_H
