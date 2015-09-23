/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_CREATEFEATUREPROPERTIESPAGE_H
#define GPLATES_QTWIDGETS_CREATEFEATUREPROPERTIESPAGE_H

#include <vector>
#include <boost/optional.hpp>
#include <QFocusEvent>
#include <QWidget>

#include "CreateFeaturePropertiesPageUi.h"

#include "model/GpgimProperty.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"
#include "model/TopLevelProperty.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class CreateFeatureAddOrEditPropertyDialog;
	class EditWidgetGroupBox;
	class ResizeToContentsTextEdit;

	class CreateFeaturePropertiesPage :
			public QWidget, 
			protected Ui_CreateFeaturePropertiesPage
	{
		Q_OBJECT
		
	public:

		//! Typedef for a sequence of property names.
		typedef std::vector<GPlatesModel::PropertyName> property_name_seq_type;

		//! Typedef for a sequence of top-level feature properties.
		typedef std::vector<GPlatesModel::TopLevelProperty::non_null_ptr_type> property_seq_type;


		explicit
		CreateFeaturePropertiesPage(
				const GPlatesModel::Gpgim &gpgim,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);


		/**
		 * Set the feature type and the initial set of feature properties.
		 *
		 * The user can use this page to add more properties supported by the specified feature type.
		 *
		 * @a reserved_feature_properties contains the names (if any) of any feature properties that
		 * will later be added (and hence are equivalent to existing properties in that they are not
		 * available for the user to add). These are typically properties that the caller wishes to
		 * add themselves rather than have us add them.
		 */
		void
		initialise(
				const GPlatesModel::FeatureType &feature_type,
				const property_seq_type &feature_properties,
				const property_name_seq_type &reserved_feature_properties = property_name_seq_type(),
		        const QString &adjective = "");


		/**
		 * Returns true if the user has added all feature properties that are required
		 * (that have a minimum GPGIM multiplicity of one).
		 */
		bool
		is_finished() const;


		/**
		 * Returns the current list of feature properties.
		 *
		 * While this page is active the user could have added more properties to those from @a initialise.
		 * The user could also have edited and removed properties.
		 */
		void
		get_feature_properties(
				property_seq_type &feature_properties) const;

	Q_SIGNALS:

		/**
		 * Emitted when there are no remaining *required* feature properties for the user to add.
		 *
		 * This is primarily intended to be used to change the focus to the "Next" page button
		 * of the Create Feature dialog.
		 */
		void
		finished();

	protected:

		virtual
		void
		focusInEvent(
				QFocusEvent *event);

	private Q_SLOTS:

		void
		handle_available_properties_selection_changed();

		void
		handle_existing_properties_selection_changed();

		void
		handle_add_property_button_clicked();

		void
		handle_remove_property_button_clicked();

		void
		handle_edit_property_button_clicked();

	private:

		/**
		 * These should match the 'available properties' table columns set up in the UI designer.
		 */
		enum AvailablePropertiesColumnName
		{
			AVAILABLE_PROPERTIES_COLUMN_PROPERTY,
			AVAILABLE_PROPERTIES_COLUMN_MULTIPLICITY
		};

		/**
		 * These should match the 'existing properties' table columns set up in the UI designer.
		 */
		enum ExistingPropertiesColumnName
		{
			EXISTING_PROPERTIES_COLUMN_PROPERTY,
			EXISTING_PROPERTIES_COLUMN_VALUE
		};

		//! Typedef for a sequence of GPGIM feature properties.
		typedef std::vector<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> gpgim_property_seq_type;


		/**
		 * The GPGIM contains information about the feature types and their properties.
		 */
		const GPlatesModel::Gpgim &d_gpgim;

		//! The type of feature that the properties will be added to.
		GPlatesModel::FeatureType d_feature_type;

		/**
		 * The names of any feature properties that will later be added (and hence are equivalent
		 * to existing properties in that they are not available for the user to add).
		 */
		property_name_seq_type d_reserved_feature_properties;

		//! A property description QTextEdit that resizes to its contents.
		ResizeToContentsTextEdit *d_property_description_widget;

		//! Dialog used to add and edit feature properties.
		CreateFeatureAddOrEditPropertyDialog *d_add_or_edit_property_dialog;


		void
		initialise_existing_properties_table(
				const property_seq_type &feature_propertiesfeature_properties);

		void
		add_to_existing_properties(
				const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property);

		void
		update_available_properties_table();

		boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type>
		get_available_property(
				int row) const;

		void
		get_available_properties(
				gpgim_property_seq_type &gpgim_feature_properties) const;

		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
		get_existing_property(
				int row) const;

		void
		update_focus();
	};
}

#endif // GPLATES_QTWIDGETS_CREATEFEATUREPROPERTIESPAGE_H
