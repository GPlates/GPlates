/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#include <algorithm>
#include <vector>
#include <typeinfo>
#include <boost/optional.hpp>
#include <boost/foreach.hpp>
#include <QString>
#include <QStringList>
#include <QValidator>
#include <QMessageBox>

#include "FeaturePropertiesDialog.h"

#include "ChangeGeometryPropertyDialog.h"
#include "GeometryDestinationsWidget.h"

#include "model/FeatureType.h"
#include "model/FeatureId.h"
#include "model/FeatureRevision.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/TopLevelPropertyInline.h"
#include "model/GPGIMInfo.h"

#include "presentation/ViewState.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/TemplateTypeParameterType.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	class FeatureTypeValidator :
			public QValidator
	{
	public:

		FeatureTypeValidator(
				QObject *parent_ = NULL) :
			QValidator(parent_)
		{  }

		virtual
		QValidator::State
		validate(
				QString &input,
				int &pos) const
		{
			QStringList tokens = input.split(":");
			if (tokens.count() > 2)
			{
				return QValidator::Invalid;
			}

			if (tokens.count() == 2)
			{
				// We'll only let the user enter gpml, gml and xsi namespaces.
				const QString &namespace_alias = tokens.at(0);
				if (namespace_alias == "gpml" ||
					namespace_alias == "gml" ||
					namespace_alias == "xsi")
				{
					if (tokens.at(1).length() > 0)
					{
						return QValidator::Acceptable;
					}
					else
					{
						return QValidator::Intermediate;
					}
				}
				else
				{
					return QValidator::Intermediate;
				}
			}
			else
			{
				return QValidator::Intermediate;
			}
		}
	};
}


GPlatesQtWidgets::FeaturePropertiesDialog::FeaturePropertiesDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_query_feature_properties_widget(new QueryFeaturePropertiesWidget(
			view_state_, this)),
	d_edit_feature_properties_widget(new EditFeaturePropertiesWidget(
			view_state_, this)),
	d_view_feature_geometries_widget(new ViewFeatureGeometriesWidget(
			view_state_, this)),
	d_change_geometry_property_dialog(new ChangeGeometryPropertyDialog(this))
{
	setupUi(this);
	
	// Set up the tab widget. Note we have to delete the 'dummy' tab set up by the Designer.
	tabwidget_query_edit->clear();
	tabwidget_query_edit->addTab(d_query_feature_properties_widget,
			QIcon(":/gnome_edit_find_16.png"), tr("&Query Properties"));
	tabwidget_query_edit->addTab(d_edit_feature_properties_widget,
			QIcon(":/gnome_gtk_edit_16.png"), tr("&Edit Properties"));
	tabwidget_query_edit->addTab(d_view_feature_geometries_widget,
			QIcon(":/gnome_stock_edit_points_16.png"), tr("View &Coordinates"));
	tabwidget_query_edit->setCurrentIndex(0);

	// Handle tab changes.
	QObject::connect(tabwidget_query_edit, SIGNAL(currentChanged(int)),
			this, SLOT(handle_tab_change(int)));
	
	// Handle focus changes.
	QObject::connect(&view_state_.get_feature_focus(), 
			SIGNAL(focus_changed(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(display_feature(GPlatesGui::FeatureFocus &)));
	QObject::connect(&view_state_.get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(display_feature(GPlatesGui::FeatureFocus &)));

	// Handle feature type changes.
	QObject::connect(
			combobox_feature_type->lineEdit(),
			SIGNAL(editingFinished()),
			this,
			SLOT(handle_feature_type_changed()));

	populate_feature_types();
	combobox_feature_type->setValidator(new FeatureTypeValidator(this));
	
	// Refresh display - since the feature ref is invalid at this point,
	// the dialog should lock everything down that might otherwise cause problems.
	refresh_display();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::display_feature(
		GPlatesGui::FeatureFocus &feature_focus)
{
	d_feature_ref = feature_focus.focused_feature();
	d_focused_rg = feature_focus.associated_reconstruction_geometry();

	refresh_display();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::refresh_display()
{
	if ( ! d_feature_ref.is_valid()) {
		// Disable everything except the Close button.
		combobox_feature_type->setEnabled(false);
		tabwidget_query_edit->setEnabled(false);
		combobox_feature_type->lineEdit()->clear();
		return;
	}

	//PROFILE_FUNC();

	// Ensure everything is enabled.
	combobox_feature_type->setEnabled(true);
	tabwidget_query_edit->setEnabled(true);
	
	// Update our text fields at the top.
	combobox_feature_type->lineEdit()->setText(
			GPlatesUtils::make_qstring_from_icu_string(
				d_feature_ref->feature_type().build_aliased_name()));
	
	// Update our tabbed sub-widgets.
	d_query_feature_properties_widget->display_feature(d_feature_ref, d_focused_rg);
	d_edit_feature_properties_widget->edit_feature(d_feature_ref);
	d_view_feature_geometries_widget->edit_feature(d_feature_ref, d_focused_rg);
}

		
void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_query_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_query_feature_properties_widget);
	d_query_feature_properties_widget->load_data_if_necessary();
	pop_up();
}

void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_edit_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_edit_feature_properties_widget);
	pop_up();
}

void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_geometries_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_view_feature_geometries_widget);
	pop_up();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::pop_up()
{
	show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	raise();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::populate_feature_types()
{
	for (int i = 0; i != 2; ++i)
	{
		typedef GPlatesModel::GPGIMInfo::feature_set_type feature_set_type;
		const feature_set_type &list = GPlatesModel::GPGIMInfo::get_feature_set(
				static_cast<bool>(i)); // first time, non-topological; second time, topological.

		for (feature_set_type::const_iterator iter = list.begin();
				iter != list.end(); ++iter)
		{
			combobox_feature_type->addItem(
					GPlatesUtils::make_qstring_from_icu_string(
						iter->build_aliased_name()));
		}
	}
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::setVisible(
		bool visible)
{
	if ( ! visible) {
		// We are closing. Ensure things are left tidy.
		d_edit_feature_properties_widget->commit_edit_widget_data();
		d_edit_feature_properties_widget->clean_up();
	}
	QDialog::setVisible(visible);
}



void
GPlatesQtWidgets::FeaturePropertiesDialog::handle_tab_change(
		int index)
{
	const QIcon icon = tabwidget_query_edit->tabIcon(index);
	if (index == 0)
	{
		d_query_feature_properties_widget->load_data_if_necessary();
	}
	setWindowIcon(icon);
}


namespace
{
	class ExtractGeometryPropertyVisitor :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		ExtractGeometryPropertyVisitor(
				bool time_dependent) :
			d_time_dependent(time_dependent),
			d_is_property_value_constant_value(false),
			d_is_property_value_irregular_sampling_or_piecewise_aggregation(false)
		{  }

		// Note: We only visit geometry types and time-dependent types.

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string)
		{
			store_property(gml_line_string);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_LINE_STRING =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString");
			d_template_type_parameter_type = GML_LINE_STRING;
		}

		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			store_property(gml_multi_point);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_MULTI_POINT =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("MultiPoint");
			d_template_type_parameter_type = GML_MULTI_POINT;
		}

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
		{
			store_property(gml_orientable_curve);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_ORIENTABLE_CURVE =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("OrientableCurve");
			d_template_type_parameter_type = GML_ORIENTABLE_CURVE;
		}

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point)
		{
			store_property(gml_point);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_POINT =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point");
			d_template_type_parameter_type = GML_POINT;
		}

		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon)
		{
			store_property(gml_polygon);

			static const GPlatesPropertyValues::TemplateTypeParameterType GML_POLYGON =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Polygon");
			d_template_type_parameter_type = GML_POLYGON;
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			// If the new property value needs a time-dependent wrapper, then we just use
			// the existing time-dependent wrapper. Otherwise, unwrap the time-dependent wrapper.
			if (d_time_dependent)
			{
				store_property(gpml_constant_value);
				d_is_property_value_constant_value = true;
			}
			else
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}
		}

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
		{
			// Note that even if the new property value doesn't need a time-dependent
			// wrapper, there's nothing we can do to unwrap the irregular sampling into a
			// single value.
			store_property(gpml_irregular_sampling);
			d_is_property_value_irregular_sampling_or_piecewise_aggregation = true;
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			// Note that even if the new property value doesn't need a time-dependent
			// wrapper, there's nothing we can do to unwrap the piecewise aggregation
			// into a single value.
			store_property(gpml_piecewise_aggregation);
			d_is_property_value_irregular_sampling_or_piecewise_aggregation = true;
		}

		virtual
		void
		visit_gpml_topological_polygon(
				const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
		{
			store_property(gpml_topological_polygon);

			static const GPlatesPropertyValues::TemplateTypeParameterType GPML_TOPOLOGICAL_POLYGON =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPolygon");
			d_template_type_parameter_type = GPML_TOPOLOGICAL_POLYGON;
		}

		virtual
		void
		visit_gpml_topological_line_section(
				const GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_topological_line_section)
		{
			store_property(gpml_topological_line_section);

			static const GPlatesPropertyValues::TemplateTypeParameterType GPML_TOPOLOGICAL_LINE_SECTION =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalLineSection");
			d_template_type_parameter_type = GPML_TOPOLOGICAL_LINE_SECTION;
		}

		virtual
		void
		visit_gpml_topological_point(
				const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_topological_point)
		{
			store_property(gpml_topological_point);

			static const GPlatesPropertyValues::TemplateTypeParameterType GPML_TOPOLOGICAL_POINT =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPoint");
			d_template_type_parameter_type = GPML_TOPOLOGICAL_POINT;
		}

		const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> &
		get_property_value() const
		{
			return d_property_value;
		}

		bool
		is_property_value_constant_value() const
		{
			return d_is_property_value_constant_value;
		}

		bool
		is_property_value_irregular_sampling_or_piecewise_aggregation() const
		{
			return d_is_property_value_irregular_sampling_or_piecewise_aggregation;
		}

		const boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> &
		get_template_type_parameter_type() const
		{
			return d_template_type_parameter_type;
		}

	private:

		void
		store_property(
				const GPlatesModel::PropertyValue &prop)
		{
			d_property_value = prop.deep_clone_as_prop_val();
		}

		/**
		 * Whether the new property expects a time dependent wrapper.
		 */
		bool d_time_dependent;

		bool d_is_property_value_constant_value;
		bool d_is_property_value_irregular_sampling_or_piecewise_aggregation;
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_property_value;
		boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> d_template_type_parameter_type;
	};

	boost::optional<GPlatesModel::TopLevelPropertyInline::non_null_ptr_type>
	create_new_geometry_property(
			const GPlatesModel::TopLevelProperty &top_level_property,
			const GPlatesModel::PropertyName &new_property_name,
			bool time_dependent,
			QString &error_message)
	{
		try
		{
			const GPlatesModel::TopLevelPropertyInline &tlpi =
				dynamic_cast<const GPlatesModel::TopLevelPropertyInline &>(top_level_property);
			if (tlpi.size() != 1)
			{
				static const QString NOT_ONE_PROPERTY_VALUE_MESSAGE =
					"GPlates cannot change the property name of a top-level property that does not have exactly one property value.";
				error_message = NOT_ONE_PROPERTY_VALUE_MESSAGE;
				return boost::none;
			}

			ExtractGeometryPropertyVisitor visitor(time_dependent);
			tlpi.accept_visitor(visitor);

			const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> &property_value =
				visitor.get_property_value();
			if (!property_value)
			{
				static const QString NOT_GEOMETRY_MESSAGE =
					"The property could not be identified as a geometry.";
				error_message = NOT_GEOMETRY_MESSAGE;
				return boost::none;
			}

			if (time_dependent)
			{
				if (visitor.is_property_value_constant_value() ||
						visitor.is_property_value_irregular_sampling_or_piecewise_aggregation())
				{
					// Since the property_value is already a time-dependent property, we can
					// create the new geometry property directly.
					return GPlatesModel::TopLevelPropertyInline::create(
							new_property_name, *property_value, tlpi.xml_attributes());
				}
				else
				{
					// If the property_value is not already a time-dependent property, we will
					// need to wrap it up in a constant value.
					GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value =
						GPlatesPropertyValues::GpmlConstantValue::create(
								*property_value, *visitor.get_template_type_parameter_type());
					return GPlatesModel::TopLevelPropertyInline::create(
							new_property_name, constant_value, tlpi.xml_attributes());
				}
			}
			else
			{
				if (visitor.is_property_value_irregular_sampling_or_piecewise_aggregation())
				{
					// We need to unwrap the time-dependent property, but was unable to.
					static const QString COULD_NOT_UNWRAP_TIME_DEPENDENT_PROPERTY_MESSAGE =
						"GPlates was unable to unwrap the time-dependent property.";
					error_message = COULD_NOT_UNWRAP_TIME_DEPENDENT_PROPERTY_MESSAGE;
					return boost::none;
				}
				else
				{
					return GPlatesModel::TopLevelPropertyInline::create(
							new_property_name, *property_value, tlpi.xml_attributes());
				}
			}
		}
		catch (const std::bad_cast &)
		{
			static const QString NOT_TOP_LEVEL_PROPERTY_INLINE_MESSAGE =
				"GPlates cannot change the property name of a top-level property that is not inline.";
			error_message = NOT_TOP_LEVEL_PROPERTY_INLINE_MESSAGE;
			return boost::none;
		}
	}
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::handle_feature_type_changed()
{
	if (d_feature_ref.is_valid())
	{
		try
		{
			GPlatesUtils::Parse<GPlatesModel::FeatureType> parse;
			const QString &feature_type_string = combobox_feature_type->currentText();
			GPlatesModel::FeatureType feature_type = parse(feature_type_string);
			GPlatesModel::FeatureType old_feature_type = d_feature_ref->feature_type();
			if (feature_type == old_feature_type)
			{
				return;
			}
			d_feature_ref->set_feature_type(feature_type);

			// We need to go through the feature's properties and see if there are any
			// geometry properties that are not valid geometry properties with the new
			// feature type.
			typedef GPlatesModel::GPGIMInfo::feature_geometric_prop_map_type feature_geometric_prop_map_type;
			const feature_geometric_prop_map_type &map =
				GPlatesModel::GPGIMInfo::get_feature_geometric_prop_map();
			std::vector<GPlatesModel::PropertyName> allowed_property_names;
			for (feature_geometric_prop_map_type::const_iterator map_iter = map.lower_bound(feature_type);
					map_iter != map.upper_bound(feature_type); ++map_iter)
			{
				allowed_property_names.push_back(map_iter->second);
			}
			std::vector<GPlatesModel::PropertyName> old_allowed_property_names;
			for (feature_geometric_prop_map_type::const_iterator map_iter = map.lower_bound(old_feature_type);
					map_iter != map.upper_bound(old_feature_type); ++map_iter)
			{
				old_allowed_property_names.push_back(map_iter->second);
			}

			std::vector<GPlatesModel::TopLevelPropertyInline::non_null_ptr_type> new_properties;
			for (GPlatesModel::FeatureHandle::const_iterator iter = d_feature_ref->begin();
					iter != d_feature_ref->end(); ++iter)
			{
				const GPlatesModel::TopLevelProperty &curr_top_level_property = **iter;
				const GPlatesModel::PropertyName &curr_property_name =
					curr_top_level_property.property_name();
				
				// The property name is of interest if it is a geometric property name for
				// the old feature type, but not a geometric property name for the new
				// feature type.
				if (std::find(old_allowed_property_names.begin(), old_allowed_property_names.end(),
							curr_property_name) != old_allowed_property_names.end() &&
						std::find(allowed_property_names.begin(), allowed_property_names.end(),
							curr_property_name) == allowed_property_names.end())
				{
					d_change_geometry_property_dialog->populate(feature_type, curr_property_name);
					if (d_change_geometry_property_dialog->exec() == QDialog::Accepted)
					{
						typedef std::pair<GPlatesModel::PropertyName, bool> pair_type;
						boost::optional<pair_type> item = d_change_geometry_property_dialog->get_property_name();
						if (!item)
						{
							continue;
						}

						const GPlatesModel::PropertyName &new_property_name = item->first;
						bool time_dependent = item->second;
						QString error_message;

						boost::optional<GPlatesModel::TopLevelPropertyInline::non_null_ptr_type> new_property =
							create_new_geometry_property(
									curr_top_level_property,
									new_property_name,
									time_dependent,
									error_message);
						if (new_property)
						{
							// Successful in converting the property.
							d_feature_ref->remove(iter);
							new_properties.push_back(*new_property);
						}
						else
						{
							// Not successful in converting the property; show error message.
							static const QString ERROR_MESSAGE_APPEND = " Please modify the geometry manually.";
							QMessageBox::warning(this, "Change Geometry Property",
									error_message + ERROR_MESSAGE_APPEND);
						}
					}
				}
			} // end for

			// Add all the new properties to the feature.
			BOOST_FOREACH(const GPlatesModel::TopLevelPropertyInline::non_null_ptr_type &new_property, new_properties)
			{
				d_feature_ref->add(new_property);
			}

			refresh_display();
		}
		catch (const GPlatesUtils::ParseError &)
		{
			// ignore
		}
	}
}

