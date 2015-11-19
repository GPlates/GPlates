/* $Id$ */

/**
 * \file 
 * Contains the definition of the FeatureVisitor and ConstFeatureVisitor classes.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2015 The University of Sydney, Australia
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

#include <iostream>
#include <boost/optional.hpp>
#include <QDebug>

#include "FeatureHandle.h"
#include "FeatureCollectionHandle.h"
#include "HandleTraits.h"
#include "PropertyName.h"
#include "RevisionAwareIterator.h"
#include "TopLevelPropertyInline.h"

#include "utils/CopyConst.h"


namespace GPlatesPropertyValues
{
	// Forward declarations for the member functions.
	// Please keep these ordered alphabetically.
	class Enumeration;
	class GmlDataBlock;
	class GmlFile;
	class GmlGridEnvelope;
	class GmlLineString;
	class GmlMultiPoint;
	class GmlOrientableCurve;
	class GmlPoint;
	class GmlPolygon;
	class GmlRectifiedGrid;
	class GmlTimeInstant;
	class GmlTimePeriod;
	class GpmlAge;
	class GpmlArray;
	class GpmlConstantValue;
	class GpmlFeatureReference;
	class GpmlFeatureSnapshotReference;
	class GpmlFiniteRotation;
	class GpmlFiniteRotationSlerp;
	class GpmlHotSpotTrailMark;
	class GpmlIrregularSampling;
	class GpmlKeyValueDictionary;
	class GpmlMeasure;
	class GpmlMetadata;
	class GpmlOldPlatesHeader;
	class GpmlPiecewiseAggregation;
	class GpmlPlateId;
	class GpmlPolarityChronId;
	class GpmlPropertyDelegate;
	class GpmlRasterBandNames;
	class GpmlRevisionId;
	class GpmlScalarField3DFile;
	class GpmlStringList;
	class GpmlTotalReconstructionPole;
	class GpmlTopologicalNetwork;
	class GpmlTopologicalInterior;
	class GpmlTopologicalPolygon;
	class GpmlTopologicalLine;
	class GpmlTopologicalLineSection;
	class GpmlTopologicalPoint;
	class OldVersionPropertyValue;
	class UninterpretedPropertyValue;
	class XsBoolean;
	class XsDouble;
	class XsInteger;
	class XsString;
}

namespace GPlatesModel
{
	namespace FeatureVisitorInternals
	{
		/**
		 * A helper traits class to differentiate between const and non-const FeatureHandles.
		 */
		template<class FeatureHandleType>
		struct Traits
		{
			typedef typename HandleTraits<FeatureHandle>::weak_ref feature_weak_ref_type;
			typedef typename HandleTraits<FeatureHandle>::iterator feature_iterator_type;
			typedef typename HandleTraits<FeatureCollectionHandle>::iterator feature_collection_iterator_type;
			typedef TopLevelPropertyInline top_level_property_inline_type;
			typedef TopLevelPropertyInline::iterator top_level_property_inline_iterator_type;
		};

		template<class FeatureHandleType>
		struct Traits<const FeatureHandleType>
		{
			typedef typename HandleTraits<FeatureHandle>::const_weak_ref feature_weak_ref_type;
			typedef typename HandleTraits<FeatureHandle>::const_iterator feature_iterator_type;
			typedef typename HandleTraits<FeatureCollectionHandle>::const_iterator feature_collection_iterator_type;
			typedef const TopLevelPropertyInline top_level_property_inline_type;
			typedef TopLevelPropertyInline::const_iterator top_level_property_inline_iterator_type;
		};
	}

	/**
	 * This class defines an abstract interface for a Visitor to visit features.
	 * The visitor is non-const if the template parameter FeatureHandleType is
	 * (non-const) FeatureHandle, and the visitor is const if the template parameter
	 * is const FeatureHandle.
	 *
	 * For convenience, FeatureVisitor and ConstFeatureVisitor typedefs are defined
	 * at the bottom of this header file.
	 *
	 * IMPORTANT: For performance reasons, you are strongly advised to inherit from
	 * ConstFeatureVisitor if you do not need to modify the objects you are visiting.
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
	template<class FeatureHandleType> // Could be either const or non-const FeatureHandle.
	class FeatureVisitorBase
	{

	public:

		/**
		 * Convenience typedef for the template parameter, which is either const or non-const FeatureHandle.
		 */
		typedef FeatureHandleType feature_handle_type;

		/**
		 * Convenience typedef for a weak-ref to a feature, with appropriate const-ness.
		 */
		typedef typename FeatureVisitorInternals::Traits<feature_handle_type>::feature_weak_ref_type feature_weak_ref_type;

		/**
		 * Convenience typedef for a feature's children iterator, with appropriate const-ness.
		 */
		typedef typename FeatureVisitorInternals::Traits<feature_handle_type>::feature_iterator_type feature_iterator_type;

		/**
		 * Convenience typedef for a feature collection's children iterator (which points to a feature), with appropriate const-ness.
		 */
		typedef typename FeatureVisitorInternals::Traits<feature_handle_type>::feature_collection_iterator_type feature_collection_iterator_type;

		/**
		 * Convenience typedef for a feature's child type, with appropriate const-ness.
		 */
		typedef typename FeatureVisitorInternals::Traits<feature_handle_type>::top_level_property_inline_type top_level_property_inline_type;

		/**
		 * Convenience typedef for a TopLevelProperty's iterator type, with appropriate const-ness.
		 */
		typedef typename FeatureVisitorInternals::Traits<feature_handle_type>::top_level_property_inline_iterator_type top_level_property_inline_iterator_type;

		// Typedefs to give the property values appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::Enumeration>::type enumeration_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlDataBlock>::type gml_data_block_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlFile>::type gml_file_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlGridEnvelope>::type gml_grid_envelope_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlLineString>::type gml_line_string_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlMultiPoint>::type gml_multi_point_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlOrientableCurve>::type gml_orientable_curve_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlPoint>::type gml_point_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlPolygon>::type gml_polygon_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlRectifiedGrid>::type gml_rectified_grid_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlTimeInstant>::type gml_time_instant_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GmlTimePeriod>::type gml_time_period_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlAge>::type gpml_age_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlArray>::type gpml_array_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlConstantValue>::type gpml_constant_value_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlFeatureReference>::type gpml_feature_reference_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlFeatureSnapshotReference>::type gpml_feature_snapshot_reference_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlFiniteRotation>::type gpml_finite_rotation_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlFiniteRotationSlerp>::type gpml_finite_rotation_slerp_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlHotSpotTrailMark>::type gpml_hot_spot_trail_mark_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlIrregularSampling>::type gpml_irregular_sampling_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlKeyValueDictionary>::type gpml_key_value_dictionary_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlMeasure>::type gpml_measure_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlOldPlatesHeader>::type gpml_old_plates_header_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlPiecewiseAggregation>::type gpml_piecewise_aggregation_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlPlateId>::type gpml_plate_id_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlPolarityChronId>::type gpml_polarity_chron_id_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlPropertyDelegate>::type gpml_property_delegate_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlRasterBandNames>::type gpml_raster_band_names_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlRevisionId>::type gpml_revision_id_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlScalarField3DFile>::type gpml_scalar_field_3d_file_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlStringList>::type gpml_string_list_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlTopologicalNetwork>::type gpml_topological_network_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlTopologicalPolygon>::type gpml_topological_polygon_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlTopologicalLine>::type gpml_topological_line_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlTopologicalLineSection>::type gpml_topological_line_section_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlTopologicalPoint>::type gpml_topological_point_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::OldVersionPropertyValue>::type old_version_property_value_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::UninterpretedPropertyValue>::type uninterpreted_property_value_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::XsBoolean>::type xs_boolean_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::XsDouble>::type xs_double_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::XsInteger>::type xs_integer_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::XsString>::type xs_string_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlTotalReconstructionPole>::type gpml_total_reconstruction_pole_type;
		typedef typename GPlatesUtils::CopyConst<feature_handle_type, GPlatesPropertyValues::GpmlMetadata>::type gpml_metadata_type;
		/**
		 * Destructor.
		 */
		virtual
		~FeatureVisitorBase();

		/**
		 * Visit the feature referenced by @a feature_weak_ref.
		 *
		 * Return true if @a feature_weak_ref was valid (and thus, the feature was
		 * visited), false otherwise.
		 */
		bool
		visit_feature(
				const feature_weak_ref_type &feature_weak_ref);

		/**
		 * Visit the feature indicated by @a iterator.
		 *
		 * Return true if @a iterator was valid (and thus, the feature was visited), false
		 * otherwise.
		 */
		bool
		visit_feature(
				const feature_collection_iterator_type &iterator);

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
				feature_handle_type &feature_handle);

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
				feature_handle_type &feature_handle);

		/**
		 * Finalise the visitor after visiting the feature properties.
		 *
		 * This is a template method function.  Override this function in your own derived
		 * class.
		 */
		virtual
		void
		finalise_post_feature_properties(
				feature_handle_type &feature_handle);

		/**
		 * Invoke this function in @a visit_feature_properties to visit a feature properties.
		 *
		 * This function should not be overridden except in emergency.
		 */
		virtual
		void
		visit_feature_property(
				const feature_iterator_type &feature_iterator);

		/**
		 * Invoke this function in @a visit_feature_handle to visit each of the the feature
		 * properties in turn.
		 *
		 * Note that this function is not virtual.  This function should not be overridden.
		 */
		void
		visit_feature_properties(
				feature_handle_type &feature_handle);

		/**
		 * Access the iterator of the top-level property which we're currently visiting.
		 */
		const boost::optional<feature_iterator_type> &
		current_top_level_propiter() const;

		/**
		 * Access the name of the top-level property which we're currently visiting.
		 */
		const boost::optional<PropertyName> &
		current_top_level_propname() const;

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
				top_level_property_inline_type &top_level_property_inline);

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
				top_level_property_inline_type &top_level_property_inline);

		/**
		 * Finalise the visitor after visiting the property values.
		 *
		 * This is a template method function.  Override this function in your own derived
		 * class.
		 */
		virtual
		void
		finalise_post_property_values(
				top_level_property_inline_type &top_level_property_inline);

		/**
		 * Invoke this function in @a visit_top_level_property_inline to visit each of the
		 * property-values in turn.
		 *
		 * Note that this function is not virtual.  This function should not be overridden.
		 */
		void
		visit_property_values(
				top_level_property_inline_type &top_level_property_inline);

	// These need to be public so that the PropertyValue derivations can access them.
	public:

		// Please keep these property-value types ordered alphabetically.

		virtual
		void
		visit_enumeration(
				enumeration_type &enumeration)
		{  }

		virtual
		void
		visit_gml_data_block(
				gml_data_block_type &gml_data_block)
		{  }

		virtual
		void
		visit_gml_file(
				gml_file_type &gml_file)
		{  }

		virtual
		void
		visit_gml_grid_envelope(
				gml_grid_envelope_type &gml_grid_envelope)
		{  }

		virtual
		void
		visit_gml_line_string(
				gml_line_string_type &gml_line_string)
		{  }

		virtual
		void
		visit_gml_multi_point(
				gml_multi_point_type &gml_multi_point)
		{  }

		virtual
		void
		visit_gml_orientable_curve(
				gml_orientable_curve_type &gml_orientable_curve)
		{  }

		virtual
		void
		visit_gml_point(
				gml_point_type &gml_point)
		{  }

		virtual
		void
		visit_gml_polygon(
				gml_polygon_type &gml_polygon)
		{  }

		virtual
		void
		visit_gml_rectified_grid(
				gml_rectified_grid_type &gml_rectified_grid)
		{  }

		virtual
		void
		visit_gml_time_instant(
				gml_time_instant_type &gml_time_instant)
		{  }

		virtual
		void
		visit_gml_time_period(
				gml_time_period_type &gml_time_period)
		{  }


		virtual
		void
		visit_gpml_age(
				gpml_age_type &gpml_age)
		{  }


		virtual
		void
		visit_gpml_array(
				gpml_array_type &gpml_array)
		{  }


		virtual
		void
		visit_gpml_constant_value(
				gpml_constant_value_type &gpml_constant_value)
		{  }

		virtual
		void
		visit_gpml_feature_reference(
				gpml_feature_reference_type &gpml_feature_reference)
		{  }

		virtual
		void
		visit_gpml_feature_snapshot_reference(
				gpml_feature_snapshot_reference_type &gpml_feature_snapshot_reference)
		{  }

		virtual
		void
		visit_gpml_finite_rotation(
				gpml_finite_rotation_type &gpml_finite_rotation)
		{  }

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				gpml_finite_rotation_slerp_type &gpml_finite_rotation_slerp)
		{  }

		virtual
		void
		visit_gpml_hot_spot_trail_mark(
				gpml_hot_spot_trail_mark_type &gpml_hot_spot_trail_mark)
		{  }

		virtual
		void
		visit_gpml_irregular_sampling(
				gpml_irregular_sampling_type &gpml_irregular_sampling)
		{  }

		virtual
		void
		visit_gpml_key_value_dictionary(
				gpml_key_value_dictionary_type &gpml_key_value_dictionary)
		{  }

		virtual
		void
		visit_gpml_measure(
				gpml_measure_type &gpml_measure)
		{  }

		virtual
		void
		visit_gpml_metadata(
				gpml_metadata_type &gpml_metadata)
		{  }

		virtual
		void
		visit_gpml_old_plates_header(
				gpml_old_plates_header_type &gpml_old_plates_header) 
		{  }

		virtual
		void
		visit_gpml_piecewise_aggregation(
				gpml_piecewise_aggregation_type &gpml_piecewise_aggregation)
		{  }

		virtual
		void
		visit_gpml_plate_id(
				gpml_plate_id_type &gpml_plate_id)
		{  }

		virtual
		void
		visit_gpml_polarity_chron_id(
				gpml_polarity_chron_id_type &gpml_polarity_chron_id)
		{  }

		virtual
		void
		visit_gpml_property_delegate(
				gpml_property_delegate_type &gpml_property_delegate)
		{  }

		virtual
		void
		visit_gpml_raster_band_names(
				gpml_raster_band_names_type &gpml_raster_band_names)
		{  }

		virtual
		void
		visit_gpml_revision_id(
				gpml_revision_id_type &gpml_revision_id) 
		{  }

		virtual
		void
		visit_gpml_scalar_field_3d_file(
				gpml_scalar_field_3d_file_type &gpml_scalar_field_3d_file)
		{  }

		virtual
		void
		visit_gpml_string_list(
				gpml_string_list_type &gpml_string_list)
		{  }

		virtual
		void
		visit_gpml_topological_network(
				gpml_topological_network_type &gpml_toplogical_network)
		{  }

		virtual
		void
		visit_gpml_topological_polygon(
				gpml_topological_polygon_type &gpml_toplogical_polygon)
		{  }

		virtual
		void
		visit_gpml_topological_line(
				gpml_topological_line_type &gpml_toplogical_line)
		{  }

		virtual
		void
		visit_gpml_topological_line_section(
				gpml_topological_line_section_type &gpml_toplogical_line_section)
		{  }

		virtual
		void
		visit_gpml_topological_point(
				gpml_topological_point_type &gpml_toplogical_point)
		{  }

		virtual
		void
		visit_gpml_total_reconstruction_pole(
				gpml_total_reconstruction_pole_type &gpml_total_reconstruction_pole);

		virtual
		void
		visit_old_version_property_value(
				old_version_property_value_type &old_version_prop_val) 
		{  }

		virtual
		void
		visit_uninterpreted_property_value(
				uninterpreted_property_value_type &uninterpreted_prop_val) 
		{  }

		virtual
		void
		visit_xs_boolean(
				xs_boolean_type &xs_boolean)
		{  }

		virtual
		void
		visit_xs_double(
				xs_double_type &xs_double)
		{  }

		virtual
		void
		visit_xs_integer(
				xs_integer_type &xs_integer)
		{  }

		virtual
		void
		visit_xs_string(
				xs_string_type &xs_string)
		{  }

	private:

		void
		log_invalid_weak_ref(
				const feature_weak_ref_type &feature_weak_ref);

		void
		log_invalid_iterator(
				const feature_collection_iterator_type &iterator);

		/**
		 * Tracks the iterator of the most-recently read top-level property.
		 */
		boost::optional<feature_iterator_type> d_current_top_level_propiter;

		/**
		 * Tracks the name of the most-recently read top-level property.
		 */
		boost::optional<PropertyName> d_current_top_level_propname;

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		FeatureVisitorBase &
		operator=(
				const FeatureVisitorBase &);
	};


	// Public methods ////////////////////////////////////////////////////////////

	template<class FeatureHandleType>
	FeatureVisitorBase<FeatureHandleType>::~FeatureVisitorBase()
	{
	};


	template<class FeatureHandleType>
	bool
	FeatureVisitorBase<FeatureHandleType>::visit_feature(
			const feature_weak_ref_type &feature_weak_ref)
	{
		if (feature_weak_ref.is_valid())
		{
			visit_feature_handle(*feature_weak_ref);
			return true;
		}
		else
		{
			log_invalid_weak_ref(feature_weak_ref);
			return false;
		}
	}


	template<class FeatureHandleType>
	bool
	FeatureVisitorBase<FeatureHandleType>::visit_feature(
			const feature_collection_iterator_type &iterator)
	{
		if (iterator.is_still_valid())
		{
			visit_feature_handle(**iterator);
			return true;
		}
		else
		{
			log_invalid_iterator(iterator);
			return false;
		}
	}


	template<class FeatureHandleType>
	void
	FeatureVisitorBase<FeatureHandleType>::visit_top_level_property_inline(
			top_level_property_inline_type &top_level_property_inline)
	{
		if (!initialise_pre_property_values(top_level_property_inline))
		{
			return;
		}

		// Visit each of the properties in turn.
		visit_property_values(top_level_property_inline);

		finalise_post_property_values(top_level_property_inline);
	}


	// Protected methods /////////////////////////////////////////////////////////
	
	template<class FeatureHandleType>
	void
	FeatureVisitorBase<FeatureHandleType>::visit_feature_handle(
			feature_handle_type &feature_handle)
	{
		if (!initialise_pre_feature_properties(feature_handle))
		{
			return;
		}

		// Visit each of the properties in turn.
		visit_feature_properties(feature_handle);

		finalise_post_feature_properties(feature_handle);
	}


	template<class FeatureHandleType>
	bool
	FeatureVisitorBase<FeatureHandleType>::initialise_pre_feature_properties(
			feature_handle_type &feature_handle)
	{
		return true;
	}


	template<class FeatureHandleType>
	void
	FeatureVisitorBase<FeatureHandleType>::finalise_post_feature_properties(
			feature_handle_type &feature_handle)
	{
	}


	template<class FeatureHandleType>
	void
	FeatureVisitorBase<FeatureHandleType>::visit_feature_properties(
			feature_handle_type &feature_handle)
	{
		feature_iterator_type iter = feature_handle.begin();
		feature_iterator_type end = feature_handle.end();
		for ( ; iter != end; ++iter)
		{
			d_current_top_level_propiter = iter;
			d_current_top_level_propname = (*iter)->property_name();

			visit_feature_property(iter);

			d_current_top_level_propiter = boost::none;
			d_current_top_level_propname = boost::none;
		}
	}


	// Template specialisations are in .cc file.
	template<>
	void
	FeatureVisitorBase<FeatureHandle>::visit_feature_property(
			const feature_iterator_type &feature_iterator);


	// Template specialisations are in .cc file.
	template<>
	void
	FeatureVisitorBase<const FeatureHandle>::visit_feature_property(
			const feature_iterator_type &feature_iterator);


	template<class FeatureHandleType>
	const boost::optional<typename FeatureVisitorBase<FeatureHandleType>::feature_iterator_type> &
	FeatureVisitorBase<FeatureHandleType>::current_top_level_propiter() const
	{
		return d_current_top_level_propiter;
	}


	template<class FeatureHandleType>
	const boost::optional<PropertyName> &
	FeatureVisitorBase<FeatureHandleType>::current_top_level_propname() const
	{
		return d_current_top_level_propname;
	}


	template<class FeatureHandleType>
	bool
	FeatureVisitorBase<FeatureHandleType>::initialise_pre_property_values(
			top_level_property_inline_type &top_level_property_inline)
	{
		return true;
	}


	template<class FeatureHandleType>
	void
	FeatureVisitorBase<FeatureHandleType>::finalise_post_property_values(
			top_level_property_inline_type &top_level_property_inline)
	{
	}


	template<class FeatureHandleType>
	void
	FeatureVisitorBase<FeatureHandleType>::visit_property_values(
			top_level_property_inline_type &top_level_property_inline)
	{
		top_level_property_inline_iterator_type iter = top_level_property_inline.begin();
		top_level_property_inline_iterator_type end = top_level_property_inline.end();
		for ( ; iter != end; ++iter)
		{
			(*iter)->accept_visitor(*this);
		}
	}


	// Template specialisations are in .cc file.
	//
	// Note: Only reason for specialising is to avoid needing to include "GpmlTotalReconstructionPole.h"
	template<>
	void
	FeatureVisitorBase<FeatureHandle>::visit_gpml_total_reconstruction_pole(
			gpml_total_reconstruction_pole_type &gpml_total_reconstruction_pole);


	// Template specialisations are in .cc file.
	//
	// Note: Only reason for specialising is to avoid needing to include "GpmlTotalReconstructionPole.h"
	template<>
	void
	FeatureVisitorBase<const FeatureHandle>::visit_gpml_total_reconstruction_pole(
			gpml_total_reconstruction_pole_type &gpml_total_reconstruction_pole);


	// Private methods ///////////////////////////////////////////////////////////
	
	template<class FeatureHandleType>
	void
	FeatureVisitorBase<FeatureHandleType>::log_invalid_weak_ref(
			const feature_weak_ref_type &feature_weak_ref)
	{
		qWarning() << "Invalid weak-ref not dereferenced.";
	}


	template<class FeatureHandleType>
	void
	FeatureVisitorBase<FeatureHandleType>::log_invalid_iterator(
			const feature_collection_iterator_type &iterator)
	{
		qWarning() << "Invalid iterator not dereferenced.";
	}


	typedef FeatureVisitorBase<FeatureHandle> FeatureVisitor;
	typedef FeatureVisitorBase<const FeatureHandle> ConstFeatureVisitor;
}

#endif  // GPLATES_MODEL_FEATUREVISITOR_H
