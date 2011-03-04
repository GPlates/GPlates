/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "ResolvedTopologicalBoundaryExportImpl.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"

#include "property-values/Enumeration.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/XsString.h"


namespace GPlatesFileIO
{
	namespace ResolvedTopologicalBoundaryExportImpl
	{
		/**
		 * Determines feature type of subsegment source feature referenced by platepolygon
		 * at a specific reconstruction time.
		 */
		class DetermineSubSegmentFeatureType :
				private GPlatesModel::ConstFeatureVisitor
		{
		public:
			DetermineSubSegmentFeatureType(
					const double &recon_time) :
				d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time))
			{  }

			SubSegmentType
			get_sub_segment_feature_type(
					const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment)
			{
				d_sub_segment_type = SUB_SEGMENT_TYPE_OTHER;

				const GPlatesModel::FeatureHandle::const_weak_ref &feature =
						sub_segment.get_feature_ref();

				
				visit_feature(feature);

				// We just visited 'feature' looking for:
				// - a feature type of "SubductionZone",
				// - a property named "subductionPolarity",
				// - a property type of "gpml:SubductionPolarityEnumeration".
				// - an enumeration value other than "Unknown".
				//
				// If we didn't find this information then look for the "sL" and "sR"
				// data type codes in an old plates header if we can find one.
				//
				if (d_sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN)
				{
					get_sub_segment_feature_type_from_old_plates_header(feature);
				}

# if 0
				// NOTE: do not call this function;
				// The sL or sR property is set by the feature, and should not change
				// for any sub-segment
				//
				// Check if the sub_segment is being used in reverse in the polygon boundary
				if (sub_segment.get_use_reverse())
				{
					reverse_orientation();
				}
#endif

				return d_sub_segment_type;
			}

		private:
			GPlatesPropertyValues::GeoTimeInstant d_recon_time;
			SubSegmentType d_sub_segment_type;


			virtual
			bool
			initialise_pre_feature_properties(
					const GPlatesModel::FeatureHandle &feature_handle)
			{
				static const GPlatesModel::FeatureType subduction_zone_type =
						GPlatesModel::FeatureType::create_gpml("SubductionZone");

				// Only interested in "SubductionZone" features.
				// If something is not a subduction zone then it is considering a ridge/transform.
				if (feature_handle.feature_type() != subduction_zone_type)
				{
					return false;
				}

				// We know it's a subduction zone but need to look at properties to
				// see if a left or right subduction zone.
				d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN;

				return true;
			}


			virtual
			bool
			initialise_pre_property_values(
					const GPlatesModel::TopLevelPropertyInline &)
			{
				static const GPlatesModel::PropertyName subduction_polarity_property_name =
						GPlatesModel::PropertyName::create_gpml("subductionPolarity");

				// Only interested in detecting the "subductionPolarity" property.
				// If something is not a subduction zone then it is considering a ridge/transform.
				return current_top_level_propname() == subduction_polarity_property_name;
			}


			// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
			virtual
			void
			visit_gpml_constant_value(
					const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}


			// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
			virtual
			void
			visit_gpml_irregular_sampling(
					const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
			{
				std::vector< GPlatesPropertyValues::GpmlTimeSample >::const_iterator 
					iter = gpml_irregular_sampling.time_samples().begin(),
					end = gpml_irregular_sampling.time_samples().end();
				for ( ; iter != end; ++iter)
				{
					// If time of time sample matches our reconstruction time then visit.
					if (d_recon_time.is_coincident_with(iter->valid_time()->time_position()))
					{
						iter->value()->accept_visitor(*this);
					}
				}
			}


			// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
			virtual
			void
			visit_gpml_piecewise_aggregation(
					const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation) 
			{
				std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
					gpml_piecewise_aggregation.time_windows().begin();
				std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
					gpml_piecewise_aggregation.time_windows().end();
				for ( ; iter != end; ++iter)
				{
					// If the time window covers our reconstruction time then visit.
					if (iter->valid_time()->contains(d_recon_time))
					{
						iter->time_dependent_value()->accept_visitor(*this);
					}
				}
			}


			virtual
			void
			visit_enumeration(
					const GPlatesPropertyValues::Enumeration &enumeration)
			{
				static const GPlatesPropertyValues::EnumerationType subduction_polarity_enumeration_type(
						"gpml:SubductionPolarityEnumeration");

				if (!subduction_polarity_enumeration_type.is_equal_to(enumeration.type()))
				{
					return;
				}

				static const GPlatesPropertyValues::EnumerationContent unknown("Unknown");
				if (unknown.is_equal_to(enumeration.value()))
				{
					d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN;
					return;
				}

				static const GPlatesPropertyValues::EnumerationContent left("Left");
				d_sub_segment_type = left.is_equal_to(enumeration.value())
						? SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT
						: SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT;
			}


			void
			get_sub_segment_feature_type_from_old_plates_header(
					const GPlatesModel::FeatureHandle::const_weak_ref &feature)
			{
				static const GPlatesModel::PropertyName old_plates_header_property_name =
					GPlatesModel::PropertyName::create_gpml("oldPlatesHeader");

				const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header;

				if ( GPlatesFeatureVisitors::get_property_value(
						feature,
						old_plates_header_property_name,
						old_plates_header ) )
				{
					if ( old_plates_header->data_type_code() == "sL" )
					{
						// set the type
						d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT;
					}

					if ( old_plates_header->data_type_code() == "sR" )
					{
						// set the type
						d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT;
					}
				}
			}


			void
			reverse_orientation()
			{
				if (d_sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT) 
				{
					// flip the orientation flag
					d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT;
				}
				else if (d_sub_segment_type == SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT) 
				{
					// flip the orientation flag
					d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT;
				}
			}

		};


		/**
		 * Determines feature type of subsegment source feature referenced by Slab Polygon
		 * at a specific reconstruction time.
		 */
		class DetermineSlabSubSegmentFeatureType :
				private GPlatesModel::ConstFeatureVisitor
		{
		public:
			DetermineSlabSubSegmentFeatureType(
					const double &recon_time) :
				d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time))
			{  }

			SubSegmentType
			get_slab_sub_segment_feature_type(
					const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment)
			{
				d_sub_segment_type = SUB_SEGMENT_TYPE_OTHER;

				const GPlatesModel::FeatureHandle::const_weak_ref &feature =
						sub_segment.get_feature_ref();

				
				visit_feature(feature);

				// We just visited 'feature' looking for:
				// - a property named "subductionPolarity",
				// - a property type of "gpml:SubductionPolarityEnumeration".
				// - an enumeration value other than "Unknown".
				//
				// If we didn't find this information then look for the "sL" and "sR"
				// data type codes in an old plates header if we can find one.
				//

				return d_sub_segment_type;
			}

		private:
			GPlatesPropertyValues::GeoTimeInstant d_recon_time;
			SubSegmentType d_sub_segment_type;


			virtual
			bool
			initialise_pre_feature_properties(
					const GPlatesModel::FeatureHandle &feature_handle)
			{
#if 0
				static const GPlatesModel::FeatureType subduction_zone_type =
						GPlatesModel::FeatureType::create_gpml("SubductionZone");

				// Only interested in "SubductionZone" features.
				// If something is not a subduction zone then it is considering a ridge/transform.
				if (feature_handle.feature_type() != subduction_zone_type)
				{
					return false;
				}

				// We know it's a subduction zone but need to look at properties to
				// see if a left or right subduction zone.
				d_sub_segment_type = SUB_SEGMENT_TYPE_OTHER;
#endif

				return true;
			}


			virtual
			bool
			initialise_pre_property_values(
					const GPlatesModel::TopLevelPropertyInline &)
			{
				static const GPlatesModel::PropertyName subduction_polarity_property_name =
						GPlatesModel::PropertyName::create_gpml("subductionPolarity");

				// Only interested in detecting the "subductionPolarity" property.
				// If something is not a subduction zone then it is considering a ridge/transform.
				return current_top_level_propname() == subduction_polarity_property_name;
			}


			// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
			virtual
			void
			visit_gpml_constant_value(
					const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}


			// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
			virtual
			void
			visit_gpml_irregular_sampling(
					const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
			{
				std::vector< GPlatesPropertyValues::GpmlTimeSample >::const_iterator 
					iter = gpml_irregular_sampling.time_samples().begin(),
					end = gpml_irregular_sampling.time_samples().end();
				for ( ; iter != end; ++iter)
				{
					// If time of time sample matches our reconstruction time then visit.
					if (d_recon_time.is_coincident_with(iter->valid_time()->time_position()))
					{
						iter->value()->accept_visitor(*this);
					}
				}
			}


			// Need this since "SubductionPolarityEnumeration" is in a time-dependent property value.
			virtual
			void
			visit_gpml_piecewise_aggregation(
					const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation) 
			{
				std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
					gpml_piecewise_aggregation.time_windows().begin();
				std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
					gpml_piecewise_aggregation.time_windows().end();
				for ( ; iter != end; ++iter)
				{
					// If the time window covers our reconstruction time then visit.
					if (iter->valid_time()->contains(d_recon_time))
					{
						iter->time_dependent_value()->accept_visitor(*this);
					}
				}
			}


			virtual
			void
			visit_enumeration(
					const GPlatesPropertyValues::Enumeration &enumeration)
			{
				static const GPlatesPropertyValues::EnumerationType subduction_polarity_enumeration_type(
						"gpml:SubductionPolarityEnumeration");

				if (!subduction_polarity_enumeration_type.is_equal_to(enumeration.type()))
				{
					return;
				}

				static const GPlatesPropertyValues::EnumerationContent unknown("Unknown");
				if (unknown.is_equal_to(enumeration.value()))
				{
					d_sub_segment_type = SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN;
					return;
				}

				static const GPlatesPropertyValues::EnumerationContent left("Left");
				d_sub_segment_type = left.is_equal_to(enumeration.value())
						? SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_LEFT
						: SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_RIGHT;
			}
		};
	}
}


GPlatesFileIO::ResolvedTopologicalBoundaryExportImpl::SubSegmentType
GPlatesFileIO::ResolvedTopologicalBoundaryExportImpl::get_sub_segment_type(
		const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
		const double &recon_time)
{
	return DetermineSubSegmentFeatureType(recon_time).get_sub_segment_feature_type(sub_segment);
}


GPlatesFileIO::ResolvedTopologicalBoundaryExportImpl::SubSegmentType
GPlatesFileIO::ResolvedTopologicalBoundaryExportImpl::get_slab_sub_segment_type(
		const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
		const double &recon_time)
{
	SubSegmentType d_sub_segment_type = SUB_SEGMENT_TYPE_OTHER;

	const GPlatesModel::FeatureHandle::const_weak_ref &feature =
		sub_segment.get_feature_ref();

	QString slabEdgeType;
	static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("slabEdgeType");
	const GPlatesPropertyValues::XsString *property_value = NULL;

	if ( GPlatesFeatureVisitors::get_property_value( feature, property_name, property_value) )
	{
		slabEdgeType = GPlatesUtils::make_qstring_from_icu_string( property_value->value().get() );

		if (slabEdgeType == QString("Leading") ) 
		{
			return DetermineSlabSubSegmentFeatureType(recon_time).get_slab_sub_segment_feature_type(sub_segment);
		}
		else if (slabEdgeType == QString("Trench") )  
		{ 
			return SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH; 
		}
		else if (slabEdgeType == QString("Side") )    
		{
			return SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE; 
		}
	}

	return d_sub_segment_type;
}
