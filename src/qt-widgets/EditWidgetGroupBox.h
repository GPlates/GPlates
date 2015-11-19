/* $Id$ */

/**
 * \file 
 * Contains the definition of class EditWidgetGroupBox.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010, 2015 The University of Sydney, Australia
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

#include <list>
#include <map>
#include <boost/optional.hpp>
#include <QGroupBox>

#include "AbstractEditWidget.h"

#include "model/FeatureHandle.h"
#include "model/GpgimStructuralType.h"
#include "model/PropertyValue.h"

#include "property-values/StructuralType.h"


namespace GPlatesModel
{
	class GpgimProperty;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesPropertyValues
{
	class Enumeration;
	class GmlLineString;
	class GmlMultiPoint;
	class GmlPoint;
	class GmlPolygon;
	class GmlTimeInstant;
	class GmlTimePeriod;
	class GpmlAge;
	class GpmlArray; 
	class GpmlKeyValueDictionary;
	class GpmlMeasure;
	class GpmlOldPlatesHeader;
	class GpmlPlateId;
	class GpmlPolarityChronId;
	class GpmlStringList;
	class XsBoolean;
	class XsDouble;
	class XsInteger;
	class XsString;
}

namespace GPlatesQtWidgets
{
	class EditAgeWidget;
	class EditAngleWidget;
	class EditBooleanWidget;
	class EditDoubleWidget;
	class EditEnumerationWidget;
	class EditGeometryWidget;
	class EditIntegerWidget;
	class EditOldPlatesHeaderWidget;
	class EditPlateIdWidget;
	class EditPolarityChronIdWidget;
	class EditShapefileAttributesWidget;
	class EditStringListWidget;
	class EditStringWidget;
	class EditTimeInstantWidget;
	class EditTimePeriodWidget;
	class EditTimeSequenceWidget;


	/**
	 * A collection of pre-allocated property edit widgets, which are hidden/shown
	 * depending on which edit widget needs to be displayed.
	 *
	 * Attention! If you want to add a new type of EditWidget, see instructions in
	 * AbstractEditWidget.h.
	 */
	class EditWidgetGroupBox: 
			public QGroupBox
	{
		Q_OBJECT
		
	public:
		
		/**
		 * A property type is the structural type of the property and an optional
		 * value type (only used if property value type is a template such as 'gpml:Array').
		 */
		typedef GPlatesModel::GpgimStructuralType::instantiation_type property_value_type;

		/**
		 * List of property types that are handled by this EditWidgetGroupBox.
		 * Used by AddPropertyDialog.
		 */
		typedef std::list<property_value_type> property_types_list_type;
		typedef property_types_list_type::const_iterator property_types_list_const_iterator;

		
		explicit
		EditWidgetGroupBox(
				GPlatesPresentation::ViewState &view_state_,
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
		 * List of property types that are handled by this EditWidgetGroupBox.
		 * Used by AddPropertyDialog.
		 */
		property_types_list_type
		get_handled_property_types_list() const;

		/**
		 * Returns true if the specified GPGIM property has at least one structural type that is
		 * supported by an edit widget.
		 *
		 * Optionally returns the list of property types, of the GPGIM property, that are supported.
		 */
		bool
		get_handled_property_types(
				const GPlatesModel::GpgimProperty &gpgim_property,
				boost::optional<property_types_list_type &> property_types = boost::none) const;

		/**
		 * Uses EditWidgetChooser to activate the editing widget most appropriate
		 * for the given top-level property.
		 */
		void
		activate_appropriate_edit_widget(
				const GPlatesModel::TopLevelProperty::non_null_ptr_type &top_level_property);

		/**
		 * Uses EditWidgetChooser to activate the editing widget most appropriate
		 * for the given property iterator @a it. Used by EditFeaturePropertiesWidget.
		 */
		void
		activate_appropriate_edit_widget(
				GPlatesModel::FeatureHandle::iterator it);

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
				GPlatesModel::FeatureHandle::iterator it);
		 
		/**
		 * Uses a dispatch table to activate the editing widget for a given property type.
		 * Used by AddPropertyDialog.
		 */
		void
		activate_widget_by_property_type(
				const property_value_type &type_of_property);
		
		/**
		 * Call this function before you call create_property_value_from_widget()
		 * to determine if any edit widget is active.
		 */
		bool
		is_edit_widget_active();
		
		/**
		 * Creates an appropriate property value for the currently active edit widget.
		 * It is the caller's responsibility to insert this into the model, or
		 * insert it wherever else the caller wishes.
		 *
		 * Can throw NoActiveEditWidgetException.
		 */
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget();
		
		/**
		 * Tells the current edit widget (if any) that it should modify the last
		 * PropertyValue that it loaded data from to match what the user has
		 * entered. This will update the model directly.
		 *
		 * Note that this means (once we have revisioning 100% implemented) calling
		 * this method will cause a new revision to be propagated up from the current
		 * PropertyValue being edited; If the caller is displaying other data from
		 * the same feature (I'm looking at you, EditFeaturePropertiesWidget!), then
		 * any cached data must be purged and re-populated from the most current
		 * revision of the Feature.
		 *
		 * You cannot use this method without first calling
		 * activate_appropriate_edit_widget() and providing a properties_iterator;
		 * otherwise how would the edit widget know what PropertyValue it should
		 * be modifying?
		 *
		 * Can throw NoActiveEditWidgetException, and UninitialisedEditWidgetException.
		 *
		 * Returns true only if the edit widget was dirty and the model was altered;
		 * you should pay attention to this if you plan on calling the FeatureFocus
		 * method announce_modfication_of_focused_feature, because otherwise you'll
		 * likely end up with infinite Signal/Slot loops.
		 */
		bool
		update_property_value_from_widget();
		
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
		 * #### FIXME: Do we still need these? It's quite a bit of copypasta and we already have
		 *      e.g. activate_widget_by_property_value_type() doing some nice polymorphism.
		 *      What uses these, and is there some nice way to configure it to use d_widget_map?
		 *      -- jclark 20150224
		 */
		void
		activate_edit_age_widget(
				GPlatesPropertyValues::GpmlAge &gpml_age);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_time_instant_widget(
				GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_time_period_widget(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_old_plates_header_widget(
				GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_double_widget(
				GPlatesPropertyValues::XsDouble &xs_double);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_enumeration_widget(
				GPlatesPropertyValues::Enumeration &enumeration);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_line_string_widget(
				GPlatesPropertyValues::GmlLineString &gml_line_string);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_multi_point_widget(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_point_widget(
				GPlatesPropertyValues::GmlPoint &gml_point);
		
		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_polygon_widget(
				GPlatesPropertyValues::GmlPolygon &gml_polygon);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_integer_widget(
				GPlatesPropertyValues::XsInteger &xs_integer);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_plate_id_widget(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_polarity_chron_id_widget(
				GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_angle_widget(
				GPlatesPropertyValues::GpmlMeasure &gpml_measure);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_string_list_widget(
				GPlatesPropertyValues::GpmlStringList &gpml_string_list);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_string_widget(
				GPlatesPropertyValues::XsString &xs_string);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_boolean_widget(
				GPlatesPropertyValues::XsBoolean &xs_boolean);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_shapefile_attributes_widget(
				GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

		/**
		 * Called by EditWidgetChooser to select the appropriate editing widget.
		 */
		void
		activate_edit_time_sequence_widget(
				GPlatesPropertyValues::GpmlArray &gpml_array);

		/**
		 * Accessor for the EditGeometryWidget, to support the extra functionality
		 * available (e.g. set_reconstruction_plate_id())
		 */
		GPlatesQtWidgets::EditGeometryWidget &
		geometry_widget()
		{
			return *d_edit_geometry_widget_ptr;
		}

		/**
		 * Accessor for the EditTimePeriodWidget, to allow the EditFeaturePropertiesWidget
		 * to change the accelerator mneumonics on the labels.
		 *
		 * TODO: These accessors could probably be extended to all of the widgets.
		 */
		GPlatesQtWidgets::EditTimePeriodWidget &
		time_period_widget()
		{
			return *d_edit_time_period_widget_ptr;
		}

		/**
		 * The various edit widgets make changes to what is just a clone of the property.
		 * This method commits those changes back into the model.
		 */
		void
		commit_property_to_model();
	
	Q_SIGNALS:
		
		void
		commit_me();

	public Q_SLOTS:
	
		void
		deactivate_edit_widgets();

		void
		edit_widget_wants_committing();
	
	private:

		/**
		 * Map type used to activate appropriate edit widget given a property value type and
		 * optional value type (only used if property type is a template).
		 */
		typedef std::map<
				property_value_type,
				AbstractEditWidget *>
						widget_map_type;
		typedef widget_map_type::const_iterator widget_map_const_iterator;


		/**
		 * Builds a map of QString to AbstractEditWidget *, to activate edit widgets
		 * based on their property values' types.
		 */
		void
		build_widget_map();

		/**
		 * Given a property type, returns a pointer to the widget responsible for editing it.
		 *
		 * Returns NULL in the event that no such value type is registered.
		 */
		GPlatesQtWidgets::AbstractEditWidget *
		get_widget_by_property_type(
				const property_value_type &type_of_property);


		/**
		 * This pointer always refers to the one edit widget which is currently active
		 * and visible. In the event of no widget being active, it is NULL.
		 */
		GPlatesQtWidgets::AbstractEditWidget *d_active_widget_ptr;
		
		// Please keep these members and their initialisers sorted in alphabetical order.
		GPlatesQtWidgets::EditAgeWidget *d_edit_age_widget_ptr;
		GPlatesQtWidgets::EditAngleWidget *d_edit_angle_widget_ptr;
		GPlatesQtWidgets::EditBooleanWidget *d_edit_boolean_widget_ptr;
		GPlatesQtWidgets::EditDoubleWidget *d_edit_double_widget_ptr;
		GPlatesQtWidgets::EditEnumerationWidget *d_edit_enumeration_widget_ptr;
		GPlatesQtWidgets::EditGeometryWidget *d_edit_geometry_widget_ptr;
		GPlatesQtWidgets::EditIntegerWidget *d_edit_integer_widget_ptr;
		GPlatesQtWidgets::EditOldPlatesHeaderWidget *d_edit_old_plates_header_widget_ptr;
		GPlatesQtWidgets::EditPlateIdWidget *d_edit_plate_id_widget_ptr;
		GPlatesQtWidgets::EditPolarityChronIdWidget *d_edit_polarity_chron_id_widget_ptr;
		GPlatesQtWidgets::EditShapefileAttributesWidget *d_edit_shapefile_attributes_widget_ptr;
		GPlatesQtWidgets::EditStringListWidget *d_edit_string_list_widget_ptr;
		GPlatesQtWidgets::EditStringWidget *d_edit_string_widget_ptr;
		GPlatesQtWidgets::EditTimeInstantWidget *d_edit_time_instant_widget_ptr;
		GPlatesQtWidgets::EditTimePeriodWidget *d_edit_time_period_widget_ptr;
		GPlatesQtWidgets::EditTimeSequenceWidget *d_edit_time_sequence_widget_ptr;

		/**
		 * Map of property types to edit widgets.
		 */
		widget_map_type d_widget_map;
		
		/**
		 * The verb in front of the title of the groupbox, prepended to the PropertyValue
		 * name. This defaults to "Edit" - for the AddPropertyDialog, this can be changed to
		 * "Add".
		 */
		QString d_edit_verb;

		/**
		 * The TopLevelProperty that we're currently editing using an edit widget.
		 * Because, for a feature iterator, we cannot directly edit the TopLevelProperty object
		 * stored in the model, the edit widgets must work with a clone, which is later
		 * committed back into the model.
		 */
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> d_current_property;

		/**
		 * The iterator to the TopLevelProperty that we're currently editing using an edit widget.
		 *
		 * We need to keep the iterator so that we can commit the clone back into the model
		 * after the edit widget is done with it.
		 *
		 * Note that if we're editing a standalone top-level property (that's not part of a feature)
		 * then this iterator will be boost::none.
		 */
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_current_property_iterator;

	};
}

#endif	// GPLATES_QTWIDGETS_EDITWIDGETGROUPBOX_H

