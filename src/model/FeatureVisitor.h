/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATUREVISITOR_H
#define GPLATES_MODEL_FEATUREVISITOR_H

#include "FeatureHandle.h"
#include "FeatureCollectionHandle.h"


namespace GPlatesPropertyValues
{
	// Forward declarations for the member functions.
	// Please keep these ordered alphabetically.
	class Enumeration;
	class GmlDataBlock;
	class GmlLineString;
	class GmlMultiPoint;
	class GmlOrientableCurve;
	class GmlPoint;
	class GmlPolygon;
	class GmlTimeInstant;
	class GmlTimePeriod;
	class GpmlConstantValue;
	class GpmlFeatureReference;
	class GpmlFeatureSnapshotReference;
	class GpmlFiniteRotation;
	class GpmlFiniteRotationSlerp;
	class GpmlHotSpotTrailMark;
	class GpmlIrregularSampling;
	class GpmlKeyValueDictionary;
	class GpmlMeasure;
	class GpmlOldPlatesHeader;
	class GpmlPiecewiseAggregation;
	class GpmlPlateId;
	class GpmlPolarityChronId;
	class GpmlPropertyDelegate;
	class GpmlRevisionId;
	class GpmlTopologicalPolygon;
	class GpmlTopologicalLineSection;
	class GpmlTopologicalIntersection;
	class GpmlTopologicalPoint;
	class UninterpretedPropertyValue;
	class XsBoolean;
	class XsDouble;
	class XsInteger;
	class XsString;
}

namespace GPlatesModel
{
	// Forward declarations for the member functions.
	class TopLevelPropertyInline;

	/**
	 * This class defines an abstract interface for a Visitor to visit non-const features.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 *  - The "visit" functions explicitly include the name of the target type in the function
	 *    name, to avoid the problem of name hiding in derived classes.  (If you declare *any*
	 *    functions named 'f' in a derived class, the declaration will hide *all* functions
	 *    named 'f' in the base class; if all the "visit" functions were simply called 'visit'
	 *    and you wanted to override *any* of them in a derived class, you would have to
	 *    override them all.)
	 */
	class FeatureVisitor
	{
	public:

		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~FeatureVisitor() = 0;

		/**
		 * Visit the feature referenced by @a feature_weak_ref.
		 *
		 * Return true if @a feature_weak_ref was valid (and thus, the feature was
		 * visited), false otherwise.
		 */
		bool
		visit_feature(
				const FeatureHandle::weak_ref &feature_weak_ref)
		{
			if (feature_weak_ref.is_valid()) {
				visit_feature_handle(*feature_weak_ref);
				return true;
			} else {
				log_invalid_weak_ref(feature_weak_ref);
				return false;
			}
		}

		/**
		 * Visit the feature indicated by @a iterator.
		 *
		 * Return true if @a iterator was valid (and thus, the feature was visited), false
		 * otherwise.
		 */
		bool
		visit_feature(
				const FeatureCollectionHandle::children_iterator &iterator)
		{
			if (iterator.is_valid()) {
				visit_feature_handle(**iterator);
				return true;
			} else {
				log_invalid_iterator(iterator);
				return false;
			}
		}

	protected:

		/**
		 * Visit a feature handle.
		 *
		 * In general, it shouldn't be necessary to override this function in your own
		 * derived class.  Instead, override @a initialise_pre_feature_properties and/or
		 * @a finalise_post_feature_properties.
		 *
		 * If you do override this function in your own derived class, don't forget to
		 * invoke @a visit_feature_properties in the function body, to visit each of the
		 * properties in turn.
		 */
		virtual
		void
		visit_feature_handle(
				FeatureHandle &feature_handle)
		{
			if ( ! initialise_pre_feature_properties(feature_handle)) {
				return;
			}

			// Visit each of the properties in turn.
			visit_feature_properties(feature_handle);

			finalise_post_feature_properties(feature_handle);
		}

		/**
		 * Initialise the visitor before visiting the feature properties.
		 *
		 * Return true if the visitor should continue and visit the feature properties;
		 * false if the visitor should skip the rest of this feature.  Note that if this
		 * function returns false, @a finalise_post_feature_properties will not be invoked.
		 *
		 * This is a template method function.  Override this function in your own derived
		 * class.
		 */
		virtual
		bool
		initialise_pre_feature_properties(
				FeatureHandle &feature_handle)
		{
			return true;
		}

		/**
		 * Finalise the visitor after visiting the feature properties.
		 *
		 * This is a template method function.  Override this function in your own derived
		 * class.
		 */
		virtual
		void
		finalise_post_feature_properties(
				FeatureHandle &feature_handle)
		{  }

		/**
		 * Invoke this function in @a visit_feature_handle to visit each of the the feature
		 * properties in turn.
		 *
		 * This function should not be overridden except in emergency (such as in
		 * ReconstructedFeatureGeometryPopulator, QueryFeaturePropertiesWidgetPopulator and
		 * ViewFeatureGeometriesWidgetPopulator).
		 */
		virtual
		void
		visit_feature_properties(
				FeatureHandle &feature_handle);

		/**
		 * Access the iterator of the top-level property which we're currently visiting.
		 */
		const boost::optional<FeatureHandle::children_iterator> &
		current_top_level_propiter() const
		{
			return d_current_top_level_propiter;
		}

		/**
		 * Access the name of the top-level property which we're currently visiting.
		 */
		const boost::optional<PropertyName> &
		current_top_level_propname() const
		{
			return d_current_top_level_propname;
		}

	// These need to be public so that the TopLevelProperty derivations can access them.
	public:

		/**
		 * Visit the inline top-level properties of a feature.
		 *
		 * In general, it shouldn't be necessary to override this function in your own
		 * derived class.  Instead, override @a initialise_pre_property_values and/or
		 * @a finalise_post_property_values.
		 *
		 * If you do override this function in your own derived class, don't forget to
		 * invoke @a visit_property_values in the function body, to visit each of the
		 * properties in turn.
		 */
		virtual
		void
		visit_top_level_property_inline(
				TopLevelPropertyInline &top_level_property_inline)
		{
			if ( ! initialise_pre_property_values(top_level_property_inline)) {
				return;
			}

			// Visit each of the properties in turn.
			visit_property_values(top_level_property_inline);

			finalise_post_property_values(top_level_property_inline);
		}

	protected:

		/**
		 * Initialise the visitor before visiting the property values.
		 *
		 * Return true if the visitor should continue and visit the property values;
		 * false if the visitor should skip the rest of this top-level property.  Note that
		 * if this function returns false, @a finalise_post_property_values will not be
		 * invoked.
		 *
		 * This is a template method function.  Override this function in your own derived
		 * class.
		 */
		virtual
		bool
		initialise_pre_property_values(
				TopLevelPropertyInline &top_level_property_inline)
		{
			return true;
		}

		/**
		 * Finalise the visitor after visiting the property values.
		 *
		 * This is a template method function.  Override this function in your own derived
		 * class.
		 */
		virtual
		void
		finalise_post_property_values(
				TopLevelPropertyInline &top_level_property_inline)
		{  }

		/**
		 * Invoke this function in @a visit_top_level_property_inline to visit each of the
		 * property-values in turn.
		 *
		 * Note that this function is not virtual.  This function should not be overridden.
		 */
		void
		visit_property_values(
				TopLevelPropertyInline &top_level_property_inline);

	// These need to be public so that the PropertyValue derivations can access them.
	public:

		// Please keep these property-value types ordered alphabetically.

		virtual
		void
		visit_enumeration(
				GPlatesPropertyValues::Enumeration &enumeration)
		{  }

		virtual
		void
		visit_gml_data_block(
				GPlatesPropertyValues::GmlDataBlock &gml_data_block)
		{  }

		virtual
		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string)
		{  }

		virtual
		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{  }

		virtual
		void
		visit_gml_orientable_curve(
				GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
		{  }

		virtual
		void
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point)
		{  }

		virtual
		void
		visit_gml_polygon(
				GPlatesPropertyValues::GmlPolygon &gml_polygon)
		{  }

		virtual
		void
		visit_gml_time_instant(
				GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
		{  }

		virtual
		void
		visit_gml_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
		{  }

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{  }

		virtual
		void
		visit_gpml_feature_reference(
				GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference)
		{  }

		virtual
		void
		visit_gpml_feature_snapshot_reference(
				GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference)
		{  }

		virtual
		void
		visit_gpml_finite_rotation(
				GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation)
		{  }

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
		{  }

		virtual
		void
		visit_gpml_hot_spot_trail_mark(
				GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark)
		{  }

		virtual
		void
		visit_gpml_irregular_sampling(
				GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
		{  }

		virtual
		void
		visit_gpml_key_value_dictionary(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
		{  }

		virtual
		void
		visit_gpml_measure(
				GPlatesPropertyValues::GpmlMeasure &gpml_measure)
		{  }

		virtual
		void
		visit_gpml_old_plates_header(
				GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header) 
		{  }

		virtual
		void
		visit_gpml_piecewise_aggregation(
				GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{  }

		virtual
		void
		visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
		{  }

		virtual
		void
		visit_gpml_polarity_chron_id(
				GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id)
		{  }

		virtual
		void
		visit_gpml_property_delegate(
				GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate)
		{  }

		virtual
		void
		visit_gpml_revision_id(
				GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id) 
		{  }

		virtual
		void
		visit_gpml_topological_polygon(
				GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon)
		{  }

		virtual
		void
		visit_gpml_topological_line_section(
				GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
		{  }

		virtual
		void
		visit_gpml_topological_intersection(
				GPlatesPropertyValues::GpmlTopologicalIntersection &gpml_toplogical_intersection)
		{  }

		virtual
		void
		visit_gpml_topological_point(
				GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
		{  }

		virtual
		void
		visit_uninterpreted_property_value(
				GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val) 
		{  }

		virtual
		void
		visit_xs_boolean(
				GPlatesPropertyValues::XsBoolean &xs_boolean)
		{  }

		virtual
		void
		visit_xs_double(
				GPlatesPropertyValues::XsDouble &xs_double)
		{  }

		virtual
		void
		visit_xs_integer(
				GPlatesPropertyValues::XsInteger &xs_integer)
		{  }

		virtual
		void
		visit_xs_string(
				GPlatesPropertyValues::XsString &xs_string)
		{  }

	private:

		void
		log_invalid_weak_ref(
				const FeatureHandle::weak_ref &feature_weak_ref);

		void
		log_invalid_iterator(
				const FeatureCollectionHandle::children_iterator &iterator);

		/**
		 * Tracks the iterator of the most-recently read top-level property.
		 */
		boost::optional<FeatureHandle::children_iterator> d_current_top_level_propiter;

		/**
		 * Tracks the name of the most-recently read top-level property.
		 */
		boost::optional<PropertyName> d_current_top_level_propname;

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		FeatureVisitor &
		operator=(
				const FeatureVisitor &);
	};

}

#endif  // GPLATES_MODEL_FEATUREVISITOR_H
