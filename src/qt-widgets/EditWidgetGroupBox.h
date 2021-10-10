/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_EDITWIDGETGROUPBOX_H
#define GPLATES_QTWIDGETS_EDITWIDGETGROUPBOX_H

#include <QGroupBox>

#include "model/PropertyValue.h"
#include "model/FeatureHandle.h"
#include "qt-widgets/AbstractEditWidget.h"
#include "qt-widgets/EditTimeInstantWidget.h"
#include "qt-widgets/EditTimePeriodWidget.h"
#include "qt-widgets/EditOldPlatesHeaderWidget.h"
#include "qt-widgets/EditDoubleWidget.h"
#include "qt-widgets/EditEnumerationWidget.h"
#include "qt-widgets/EditIntegerWidget.h"
#include "qt-widgets/EditPlateIdWidget.h"
#include "qt-widgets/EditPolarityChronIdWidget.h"
#include "qt-widgets/EditAngleWidget.h"
#include "qt-widgets/EditStringWidget.h"
#include "qt-widgets/EditBooleanWidget.h"


namespace GPlatesQtWidgets
{
	/**
	 * A collection of pre-allocated property edit widgets, which are hidden/shown
	 * depending on which edit widget needs to be displayed.
	 */
	class EditWidgetGroupBox: 
			public QGroupBox
	{
		Q_OBJECT
		
	public:
		
		/**
		 * Map type used to activate appropriate edit widget given a property value name.
		 */
		typedef std::map<QString, AbstractEditWidget *> widget_map_type;
		typedef widget_map_type::const_iterator widget_map_const_iterator;
		
		/**
		 * List of property type names that are handled by this EditWidgetGroupBox.
		 * Used by AddPropertyDialog.
		 */
		typedef std::list<QString> property_types_list_type;
		typedef property_types_list_type::const_iterator property_types_list_const_iterator;
		
		explicit
		EditWidgetGroupBox(
				QWidget *parent_ = NULL);
		
		virtual
		~EditWidgetGroupBox()
		{  }
		
		/**
		 * Changes the verb used as the title of the GroupBox.
		 */
		void
		set_edit_verb(
				const QString &verb)
		{
			d_edit_verb = verb;
		}
		
		/**
		 * List of property type names that are handled by this EditWidgetGroupBox.
		 * Used by AddPropertyDialog.
		 */
		property_types_list_type
		get_handled_property_types_list() const;

		/**
		 * Uses EditWidgetChooser to activate the editing widget most appropriate
		 * for the given property iterator. Used by EditFeaturePropertiesWidget.
		 */
		void
		activate_appropriate_edit_widget(
				GPlatesModel::FeatureHandle::properties_iterator it);

		/**
		 * Uses EditWidgetChooser to update the editing widget to the latest value of
		 * the property being edited.
		 * Note that this does not change which widget is being displayed (otherwise
		 * the interface would appear to 'flicker') - it is used by EditFeaturePropertiesWidget
		 * to handle a case where a user has edited a value via the QTableView and the
		 * currently selected edit widget needs to be updated.
		 */
		void
		refresh_edit_widget(
				GPlatesModel::FeatureHandle::properties_iterator it);
		 
		/**
		 * Uses a dispatch table to activate the editing widget for a given
		 * PropertyValue type name. Used by AddPropertyDialog.
		 */
		void
		activate_widget_by_property_value_name(
				const QString &property_value_name);
		
		/**
		 * Call this function before you call create_property_value_from_widget()
		 * to determine if any edit widget is active.
		 */
		bool
		is_edit_widget_active();
		
		/**
		 * Creates an appropriate property value for the currently active edit widget.
		 */
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget();
		
		/**
		 * Checks if the current edit widget is 'dirty' (user has modified fields and
		 * data is not in the model)
		 *
		 * If no edit widget is active, this function always returns false.
		 */
		bool
		is_dirty();
		
		/**
		 * Informs the EditWidgetGroupBox that the data from the current widget has been
		 * committed safely.
		 */
		void
		set_clean();

		/**
		 * Informs the EditWidgetGroupBox that the data from the current widget does
		 * not match the Model.
		 *
		 * Client code should not need this, as the only way a widget becomes 'dirty'
		 * is currently through user interaction. However it is conceivable that other
		 * applications of EditWidgetGroupBox may need it, so it is provided for completeness.
		 */
		void
		set_dirty();

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_time_instant_widget(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_time_period_widget(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_old_plates_header_widget(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_double_widget(
				const GPlatesPropertyValues::XsDouble &xs_double);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_enumeration_widget(
				const GPlatesPropertyValues::Enumeration &enumeration);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_integer_widget(
				const GPlatesPropertyValues::XsInteger &xs_integer);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_plate_id_widget(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_polarity_chron_id_widget(
				const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_angle_widget(
				const GPlatesPropertyValues::GpmlMeasure &gpml_measure);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_string_widget(
				const GPlatesPropertyValues::XsString &xs_string);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_boolean_widget(
				const GPlatesPropertyValues::XsBoolean &xs_boolean);
	
	signals:
		
		void
		commit_me();

	public slots:
	
		void
		deactivate_edit_widgets();

		void
		edit_widget_wants_committing();
	
	private:
		
		/**
		 * Builds a map of QString to AbstractEditWidget *, to activate edit widgets
		 * based on their property values' names.
		 */
		const widget_map_type &
		build_widget_map() const;

		/**
		 * Given the name of a property value type, returns a pointer to the widget
		 * responsible for editing it.
		 * Returns NULL in the event that no such value type is registered.
		 */
		GPlatesQtWidgets::AbstractEditWidget *
		get_widget_by_name(
				const QString &property_value_type_name);
		

		/**
		 * This pointer always refers to the one edit widget which is currently active
		 * and visible. In the event of no widget being active, it is NULL.
		 */
		GPlatesQtWidgets::AbstractEditWidget *d_active_widget_ptr;
		
		GPlatesQtWidgets::EditTimeInstantWidget *d_edit_time_instant_widget_ptr;
		GPlatesQtWidgets::EditTimePeriodWidget *d_edit_time_period_widget_ptr;
		GPlatesQtWidgets::EditOldPlatesHeaderWidget *d_edit_old_plates_header_widget_ptr;
		GPlatesQtWidgets::EditDoubleWidget *d_edit_double_widget_ptr;
		GPlatesQtWidgets::EditEnumerationWidget *d_edit_enumeration_widget_ptr;
		GPlatesQtWidgets::EditIntegerWidget *d_edit_integer_widget_ptr;
		GPlatesQtWidgets::EditPlateIdWidget *d_edit_plate_id_widget_ptr;
		GPlatesQtWidgets::EditPolarityChronIdWidget *d_edit_polarity_chron_id_widget_ptr;
		GPlatesQtWidgets::EditAngleWidget *d_edit_angle_widget_ptr;
		GPlatesQtWidgets::EditStringWidget *d_edit_string_widget_ptr;
		GPlatesQtWidgets::EditBooleanWidget *d_edit_boolean_widget_ptr;
		
		/**
		 * The verb in front of the title of the groupbox, prepended to the PropertyValue
		 * name. This defaults to "Edit" - for the AddPropertyDialog, this can be changed to
		 * "Add".
		 */
		QString d_edit_verb;
	};
}

#endif	// GPLATES_QTWIDGETS_EDITWIDGETGROUPBOX_H

