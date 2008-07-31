/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_QTWIDGETS_ABSTRACTEDITWIDGET_H
#define GPLATES_QTWIDGETS_ABSTRACTEDITWIDGET_H

#include <QWidget>
#include <QLabel>

#include "model/PropertyValue.h"
#include "qt-widgets/PropertyValueNotSupportedException.h"


namespace GPlatesQtWidgets
{
	/**
	 * Abstract base of all GPlatesQtWidgets::Edit*Widget.
	 * 
	 * If you need to add a new edit widget, you may wish to refer to changeset:2682.
	 * You will need to design the Ui.ui file, create a derivation of this class,
	 * and add references to the new edit widget in the following places:-
	 *
	 * EditWidgetGroupBox.h:
	 *  - add a member data pointer to the edit widget.
	 *  - define an activate_edit_*_widget() function.
	 * EditWidgetGroupBox.cc:
	 *  - add initialiser to constructor.
	 *  - add widget to edit_layout in constructor.
	 *  - connect widget's commit_me() signal in constructor.
	 *  - add map entry in EditWidgetGroupBox::build_widget_map().
	 *  - add an activate_edit_*_widget() function.
	 *  - hide the widget in deactivate_edit_widgets() function.
	 * EditWidgetChooser.cc:
	 *  - call the activate_edit_*_widget() function you defined earlier.
	 */
	class AbstractEditWidget:
			public QWidget
	{
		Q_OBJECT
		
	public:
		
		explicit
		AbstractEditWidget(
				QWidget *parent_ = NULL):
			QWidget(parent_),
			d_default_label_ptr(NULL),
			d_dirty(false),
			d_handle_enter_key(true)
		{  }
		
		virtual
		~AbstractEditWidget()
		{  }
		
		/**
		 * Sets sensible default values for all line edits, spinboxes etc
		 * that belong to this edit widget.
		 *
		 * This should also cause the edit widget to forget about any PropertyValue
		 * that it may have been initialised with; calling
		 * update_property_value_from_widget() immediately after a reset should
		 * fail with a UninitialisedEditWidgetException.
		 */
		virtual
		void
		reset_widget_to_default_values() = 0;
		
		/**
		 * Informs the edit widget of the specific property value type (by name)
		 * that we are requesting this edit widget handle. Most edit widgets will
		 * not need to bother reimplementing this function, as they are designed
		 * for a single PropertyValue e.g. gml:TimePeriod or gpml:OldPlatesHeader.
		 *
		 * However, some edit widgets will need to handle multiple property values
		 * such as gml:_Geometry, gml:LineString, gml:MultiPoint etc being handled
		 * by a single EditGeometryWidget, or multiple enumeration properties being
		 * handled by a single EditEnumerationWidget. These widgets will benefit
		 * from reimplementing this function.
		 *
		 * The default implementation does nothing. Widgets which are able to
		 * support multiple similar PropertyValue types should throw a
		 * PropertyValueNotSupportedException for property value types they cannot
		 * handle.
		 *
		 * Calling this function may cause the widget to hide or show buttons,
		 * change combobox contents, etc as necessary. It might also do nothing.
		 *
		 * It is called by EditWidgetGroupBox, as part of the
		 * activate_widget_by_property_value_name() function, which is used by
		 * AddPropertyDialog.
		 */
		virtual
		void
		configure_for_property_value_type(
				const QString &property_value_name)
		{  }
		
		/**
		 * Requests that the edit widget convert its fields into a new PropertyValue
		 * and return it, ready for insertion into the Model.
		 */
		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const = 0;
		
		/**
		 * Requests that the edit widget should use setter methods to update
		 * whichever PropertyValue the widget last read values from.
		 *
		 * For example, the EditPlateIdWidget has a method
		 * update_widget_from_plate_id(). When this is called, the widget should
		 * remember the GpmlPlateId::non_null_ptr_type it is given, so
		 * that it can modify the PropertyValue when this method is invoked.
		 *
		 * Obviously, this may not work for two reasons:
		 *  1) The caller is an idiot and has called this method without
		 *     first calling the appropriate update_widget_from_xxxx() function
		 *     to get the data into the fields in the first place,
		 *  2) The EditWidget is being used by the Add Properties dialog to
		 *     create brand new PropertyValues out of thin air, and the caller
		 *     is still an idiot.
		 * In these cases, this method will throw a UninitialisedEditWidgetException.
		 *
		 * Returns true only if the edit widget was dirty and the model was altered;
		 * the caller should pay attention to this if they plan on calling something
		 * like the FeatureFocus method announce_modfication_of_focused_feature, because
		 * otherwise you'll likely end up with infinite Signal/Slot loops.
		 */
		virtual
		bool
		update_property_value_from_widget() = 0;
		
		
		/**
		 * Checks if this edit widget is 'dirty' (user has modified fields and
		 * data is not in the model)
		 */
		bool
		is_dirty();
		
		/**
		 * Returns whether the edit widget will process the Enter key and emit
		 * the commit_me() signal when it is pressed.
		 */
		bool
		will_handle_enter_key();

		/**
		 * Some derivations of AbstractEditWidget may declare one of their (presumably
		 * Qt-Designer made) labels as the 'default' label. This may not be applicable in
		 * all cases, but for those widgets that typically set up a single label with a single
		 * control, this allows the owner of the edit widget some control over the QLabel in
		 * question.
		 * 
		 * For example, depending on the environment the edit widget is to be used in,
		 * the label may not be appropriate and should be hidden. In others, it may be useful
		 * to show the label, but set a different mneumonic key so that it will not conflict with
		 * any other accelerators.
		 *
		 * Note that this accessor may return NULL if the widget in question has no suitable
		 * label.
		 */
		QLabel *
		label();
		
	protected:
	
		/**
		 * Derivations of AbstractEditWidget can call this member in their constructor
		 * to set a label as the 'default' for this edit widget; This allows the owner
		 * of the edit widget to hide, show, or change default mneumonic keys of the
		 * label as appropriate for the parent environment, using the accessor @a label().
		 */
		void
		declare_default_label(
				QLabel *label_);

		void
		keyPressEvent(
				QKeyEvent *ev);
	
	public slots:
		
		/**
		 * set_dirty() should be called whenever a widget is modified by a user
		 * (not programatically!) to keep track of whether this edit widget should
		 * have it's data committed by the EditFeaturePropertiesWidget.
		 */
		void
		set_dirty();
		
		/**
		 * set_clean() will be called by EditWidgetGroupBox::set_clean(), which should
		 * be called whenever a PropertyValue has been constructed and committed into
		 * the model from this widget.
		 */
		void
		set_clean();
		
		
		/**
		 * Controls whether the edit widget will process the Enter key and emit
		 * the commit_me() and enter_pressed() signals when it is pressed.
		 *
		 * The default is true - the Enter key will be trapped and processed by
		 * this edit widget.
		 */
		void
		set_handle_enter_key(
				bool should_handle);
	
	signals:
	
		/**
		 * Signal typically emitted when the user presses enter, indicating an updated value.
		 * Some widgets will emit this signal in additional situations, e.g. when the user
		 * checks one of "Distant Past" or "Distant Future" on an EditTimePeriodWidget.
		 */
		void
		commit_me();

		/**
		 * Signal emitted when the user presses enter.
		 * Note that this is different to the commit_me() signal, and may be useful for
		 * focus handling situations where you (the owner of this widget) wish to call
		 * create_property_value_from_widget() when you're good and ready.
		 */
		void
		enter_pressed();

	private:
				
		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		AbstractEditWidget &
		operator=(
				const AbstractEditWidget &);
		
		/**
		 * The 'default' label for this edit widget. This may not be applicable for all
		 * edit widgets, so this pointer may be NULL.
		 */
		QLabel *d_default_label_ptr;
		
		bool d_dirty;
		
		bool d_handle_enter_key;
		
	};
}

#endif  // GPLATES_QTWIDGETS_ABSTRACTEDITWIDGET_H
