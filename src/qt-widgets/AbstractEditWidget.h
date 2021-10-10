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
			d_dirty(false)
		{  }
		
		virtual
		~AbstractEditWidget()
		{  }
		
		/**
		 * Sets sensible default values for all line edits, spinboxes etc
		 * that belong to this edit widget.
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
		 * Checks if this edit widget is 'dirty' (user has modified fields and
		 * data is not in the model)
		 */
		bool
		is_dirty();
		
	protected:
	
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
	
	signals:
	
		/**
		 * Signal typically emitted when the user presses enter, indicating an updated value.
		 * Some widgets will emit this signal in additional situations, e.g. when the user
		 * checks one of "Distant Past" or "Distant Future" on an EditTimePeriodWidget.
		 */
		void
		commit_me();

	private:
		
		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		AbstractEditWidget &
		operator=(
				const AbstractEditWidget &);
		
		bool d_dirty;
		
	};
}

#endif  // GPLATES_QTWIDGETS_ABSTRACTEDITWIDGET_H
