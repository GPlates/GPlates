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

#include <algorithm>
#include <functional>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QMetaType>

#include "ChoosePropertyWidget.h"

#include "QtWidgetUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/Gpgim.h"
#include "model/GpgimProperty.h"
#include "model/PropertyName.h"

namespace
{
	bool
	feature_has_property_name(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const GPlatesModel::PropertyName &property_name)
	{
		if (!feature_ref.is_valid())
		{
			return false;
		}

		// Iterate over all feature properties.
		GPlatesModel::FeatureHandle::const_iterator iter = feature_ref->begin();
		GPlatesModel::FeatureHandle::const_iterator end = feature_ref->end();
		for ( ; iter != end; ++iter)
		{
			if ((*iter)->get_property_name() == property_name)
			{
				// Found a matching property name.
				return true;
			}
		}

		return false;
	}

	class DefaultConstructiblePropertyName
	{
	public:

		DefaultConstructiblePropertyName()
		{  }

		DefaultConstructiblePropertyName(
				const GPlatesModel::PropertyName &property_name) :
			d_property_name(property_name)
		{  }

		operator GPlatesModel::PropertyName() const
		{
			return *d_property_name;
		}

		bool
		operator==(
				const DefaultConstructiblePropertyName &other)
		{
			return d_property_name == other.d_property_name;
		}

	private:

		boost::optional<GPlatesModel::PropertyName> d_property_name;
	};


	/**
	 * Used to sort GPGIM properties by the unqualified part of their property names.
	 */
	class SortByUnqualifiedPropertyName :
			public std::binary_function<
					GPlatesModel::GpgimProperty::non_null_ptr_to_const_type,
					GPlatesModel::GpgimProperty::non_null_ptr_to_const_type,
					bool>
	{
	public:
		bool
		operator()(
				const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &lhs,
				const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &rhs) const
		{
			return lhs->get_property_name().get_name() < rhs->get_property_name().get_name();
		}
	};
}


Q_DECLARE_METATYPE( DefaultConstructiblePropertyName )


bool
GPlatesQtWidgets::ChoosePropertyWidget::get_properties_to_populate(
		std::vector<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &gpgim_properties,
		const GPlatesModel::FeatureType &target_feature_type,
		const GPlatesPropertyValues::StructuralType &target_property_type,
		const GPlatesModel::FeatureHandle::weak_ref &source_feature_ref)
{
	// Get the GPGIM feature properties for the target feature type and target property type.
	GPlatesModel::Gpgim::property_seq_type gpgim_feature_properties;
	if (!GPlatesModel::Gpgim::instance().get_feature_properties(
			target_feature_type,
			target_property_type,
			gpgim_feature_properties))
	{
		return false;
	}

	// Iterate over the matching feature properties.
	BOOST_FOREACH(
			const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_feature_property,
			gpgim_feature_properties)
	{
		// If the current property is allowed to occur at most once per feature then only allow
		// changing to the property if it doesn't already exist in the feature (if feature specified).
		if (gpgim_feature_property->get_multiplicity() == GPlatesModel::GpgimProperty::ZERO_OR_ONE ||
			gpgim_feature_property->get_multiplicity() == GPlatesModel::GpgimProperty::ONE)
		{
			if (feature_has_property_name(source_feature_ref, gpgim_feature_property->get_property_name()))
			{
				continue;
			}
		}

		gpgim_properties.push_back(gpgim_feature_property);
	}

	return !gpgim_properties.empty();
}


GPlatesQtWidgets::ChoosePropertyWidget::ChoosePropertyWidget(
		SelectionWidget::DisplayWidget display_widget,
		QWidget *parent_) :
	QWidget(parent_),
	d_selection_widget(new SelectionWidget(display_widget, this))
{
	QtWidgetUtils::add_widget_to_placeholder(d_selection_widget, this);

	setFocusProxy(d_selection_widget);

	QObject::connect(
			d_selection_widget,
			SIGNAL(item_activated(int)),
			this,
			SLOT(handle_item_activated(int)));
}


void
GPlatesQtWidgets::ChoosePropertyWidget::populate(
		const GPlatesModel::FeatureType &target_feature_type,
		const GPlatesPropertyValues::StructuralType &target_property_type,
		const GPlatesModel::FeatureHandle::weak_ref &source_feature_ref)
{
	// Remember the current selection so we can re-select if it exists after re-populating.
	boost::optional<GPlatesModel::PropertyName> previously_selected_property_name = get_property_name();
	boost::optional<GPlatesModel::PropertyName> selected_property_name;

	d_selection_widget->clear();

	//
	// Add the names of all properties, allowed by the GPGIM, of the target feature type that
	// match the target property type.
	//

	std::vector<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> gpgim_feature_properties;
	get_properties_to_populate(
			gpgim_feature_properties,
			target_feature_type,
			target_property_type,
			source_feature_ref);

	// Sort GPGIM properties by the unqualified part of their property names.
	std::sort(gpgim_feature_properties.begin(), gpgim_feature_properties.end(), SortByUnqualifiedPropertyName());

	// Iterate over the matching feature properties and add them to the selection.
	BOOST_FOREACH(
			const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_feature_property,
			gpgim_feature_properties)
	{
		// Add the current property for display.
		d_selection_widget->add_item<DefaultConstructiblePropertyName>(
				gpgim_feature_property->get_user_friendly_name(),
				gpgim_feature_property->get_property_name());

		// Set the newly selected property name if it matches previous selection (if there was any)
		// and the previous selection exists in the new list (ie, if we get here).
		if (!selected_property_name &&
			previously_selected_property_name &&
			previously_selected_property_name.get() == gpgim_feature_property->get_property_name())
		{
			selected_property_name = previously_selected_property_name.get();
		}
	}

	if (d_selection_widget->get_count())
	{
		if (selected_property_name)
		{
			set_property_name(selected_property_name.get());
		}
		else
		{
			d_selection_widget->set_current_index(0);
		}
	}
}


boost::optional<GPlatesModel::PropertyName>
GPlatesQtWidgets::ChoosePropertyWidget::get_property_name() const
{
	return boost::optional<GPlatesModel::PropertyName>(
			d_selection_widget->get_data<DefaultConstructiblePropertyName>(
				d_selection_widget->get_current_index()));
}


void
GPlatesQtWidgets::ChoosePropertyWidget::set_property_name(
		const GPlatesModel::PropertyName &property_name)
{
	int index = d_selection_widget->find_data<DefaultConstructiblePropertyName>(property_name);
	d_selection_widget->set_current_index(index);
}


void
GPlatesQtWidgets::ChoosePropertyWidget::handle_item_activated(
		int index)
{
	Q_EMIT item_activated();
}

