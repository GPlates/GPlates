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

#ifndef GPLATES_MODEL_GPGIMFEATURECLASS_H
#define GPLATES_MODEL_GPGIMFEATURECLASS_H

#include <vector>
#include <boost/optional.hpp>
#include <QString>

#include "FeatureType.h"
#include "GpgimProperty.h"
#include "PropertyName.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * Represents the class of feature in the GPlates Geological Information Model (GPGIM).
	 *
	 * The feature classes follow a (single) inheritance hierarchy.
	 * The leaf nodes of the inheritance graph are associated with concrete feature types
	 * that can be instantiated in the GPlates model (as opposed to abstract types).
	 */
	class GpgimFeatureClass :
			public GPlatesUtils::ReferenceCount<GpgimFeatureClass>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GpgimFeatureClass.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpgimFeatureClass> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpgimFeatureClass.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpgimFeatureClass> non_null_ptr_to_const_type;

		//! Typedef for a sequence of GPGIM properties (definitions).
		typedef std::vector<GpgimProperty::non_null_ptr_to_const_type> gpgim_property_seq_type;


		/**
		 * Creates a @a GpgimFeatureClass that (optionally) inherits from the specified parent feature class.
		 *
		 * @param feature_type the name associated with this feature class.
		 * @param feature_description short description of the feature type.
		 * @param gpgim_properties_begin begin iterator over the GPGIM properties associated with this
		 *        feature class (but not its ancestor classes).
		 * @param gpgim_properties_end end iterator over the GPGIM properties associated with this
		 *        feature class (but not its ancestor classes).
		 * @param default_geometry_property the default geometry property (if there is one).
		 * @param parent_feature_class the parent feature class (if there is a parent).
		 *
		 * @throws PreconditionViolationError if @a default_geometry_property is specified but
		 * is not one of the GPGIM property instances in the range
		 * [ @a gpgim_properties_begin, @a gpgim_properties_end ].
		 */
		template <typename GpgimPropertyForwardIter>
		static
		non_null_ptr_type
		create(
				const FeatureType &feature_type,
				const QString &feature_description,
				GpgimPropertyForwardIter gpgim_properties_begin,
				GpgimPropertyForwardIter gpgim_properties_end,
				boost::optional<GpgimProperty::non_null_ptr_to_const_type> default_geometry_property = boost::none,
				boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type> parent_feature_class = boost::none)
		{
			return non_null_ptr_type(
					new GpgimFeatureClass(
							feature_type,
							feature_description,
							gpgim_properties_begin,
							gpgim_properties_end,
							default_geometry_property,
							parent_feature_class));
		}


		/**
		 * Returns the feature type (string) of this GPGIM feature class.
		 *
		 * Note that only 'concrete' feature types are instantiated in the GPlates model.
		 * See 'Gpgim::get_concrete_feature_types()'.
		 */
		const FeatureType &
		get_feature_type() const
		{
			return d_feature_type;
		}


		/**
		 * Returns the feature description for this GPGIM feature class.
		 */
		const QString &
		get_feature_description() const
		{
			return d_feature_description;
		}


		/**
		 * Returns the GPGIM properties of this feature class (including ancestor feature classes).
		 *
		 * This includes properties from the parent class (if exists) and any of its ancestors
		 * (back to the root class).
		 */
		void
		get_feature_properties(
				gpgim_property_seq_type &feature_properties) const;


		/**
		 * Convenience method returns the GPGIM property(s) of the specified property type.
		 *
		 * Returns false if the specified property type is not recognised for any properties
		 * of this feature class (or any ancestor/inherited classes).
		 *
		 * Optionally returns the matching feature properties in @a feature_properties.
		 */
		bool
		get_feature_properties(
				const GPlatesPropertyValues::StructuralType &property_type,
				boost::optional<gpgim_property_seq_type &> feature_properties = boost::none) const;


		/**
		 * Returns the GPGIM properties of only this feature class (excluding ancestor feature classes).
		 *
		 * Only properties from this feature class (superclass) are included.
		 */
		const gpgim_property_seq_type &
		get_feature_properties_excluding_ancestor_classes() const
		{
			return d_feature_properties;
		}


		/**
		 * Convenience method returns the GPGIM property of the specified property name.
		 *
		 * Returns boost::none if the specified property name is not recognised for this feature class
		 * (or any ancestor/inherited classes).
		 */
		boost::optional<GpgimProperty::non_null_ptr_to_const_type>
		get_feature_property(
				const GPlatesModel::PropertyName &property_name) const;


		/**
		 * Returns the default GPGIM property that represents a *geometry* property for this feature class.
		 *
		 * Returns boost::none if this feature class (and its ancestor/inherited classes) do not
		 * have a default geometry property. This can happen if this feature class is an abstract
		 * feature class (since a descendent/derived class will likely contain a geometry property).
		 *
		 * If both an ancestor feature class (or multiple ancestor classes) and this feature class
		 * provide a default GPGIM property then this feature class overrides the ancestors.
		 * Note that typically this won't happen if the GPGIM is designed/edited properly.
		 */
		boost::optional<GpgimProperty::non_null_ptr_to_const_type>
		get_default_geometry_feature_property() const;


		/**
		 * Same as @a get_default_geometry_feature_property but excludes ancestor feature classes.
		 *
		 * Only if this feature class (ie, not superclasses) has a default geometry property
		 * will one be returned. This is useful when converting one feature class into another.
		 */
		boost::optional<GpgimProperty::non_null_ptr_to_const_type>
		get_default_geometry_feature_property_excluding_ancestor_classes() const
		{
			return d_default_geometry_property;
		}


		/**
		 * Returns the parent feature class that this feature class inherits from, or
		 * returns boost::none if this is the root class (ie, has no parent).
		 */
		boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type>
		get_parent_feature_class() const
		{
			return d_parent_feature_class;
		}

	private:

		//! The GPGIM feature type (string) of this feature class.
		FeatureType d_feature_type;

		//! A short description of the feature type.
		QString d_feature_description;

		//! The GPGIM properties of this feature class.
		gpgim_property_seq_type d_feature_properties;

		//! The optional default geometry property.
		boost::optional<GpgimProperty::non_null_ptr_to_const_type> d_default_geometry_property;

		//! Optional parent feature class that 'this' feature class inherits from.
		boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type> d_parent_feature_class;


		template <typename GpgimPropertyForwardIter>
		GpgimFeatureClass(
				const FeatureType &feature_type,
				const QString &feature_description,
				GpgimPropertyForwardIter gpgim_properties_begin,
				GpgimPropertyForwardIter gpgim_properties_end,
				boost::optional<GpgimProperty::non_null_ptr_to_const_type> default_geometry_property,
				boost::optional<GpgimFeatureClass::non_null_ptr_to_const_type> parent_feature_class) :
			d_feature_type(feature_type),
			d_feature_description(feature_description),
			d_feature_properties(gpgim_properties_begin, gpgim_properties_end),
			d_default_geometry_property(default_geometry_property),
			d_parent_feature_class(parent_feature_class)
		{
			// If default geometry property specified then it must be one of the list feature properties.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					!d_default_geometry_property ||
					std::find(d_feature_properties.begin(), d_feature_properties.end(),
							d_default_geometry_property.get()) != d_feature_properties.end(),
					GPLATES_ASSERTION_SOURCE);
		}

	};
}

#endif // GPLATES_MODEL_GPGIMFEATURECLASS_H
