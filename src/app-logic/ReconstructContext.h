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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTCONTEXT_H
#define GPLATES_APP_LOGIC_RECONSTRUCTCONTEXT_H

#include <set>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "MultiPointVectorField.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructHandle.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"
#include "ReconstructParams.h"
#include "ReconstructMethodInterface.h"
#include "ReconstructMethodType.h"
#include "TimeSpanUtils.h"
#include "VelocityDeltaTime.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureId.h"


namespace GPlatesAppLogic
{
	class ReconstructMethodRegistry;

	/**
	 * Used to reconstruct regular features into @a ReconstructedFeatureGeometry objects
	 * at various reconstruction times.
	 *
	 * This class keeps a mapping of features to reconstruct methods internally.
	 * This is because:
	 * - Avoids unnecessarily detecting the reconstruct method at each reconstruction time,
	 * - Makes it easier to extract a mapping of present-day geometries knowing that the
	 *   features (and hence present day geometries) have not changed. This is useful when
	 *   things like static polygon raster reconstruction that map a present day polygon geometry
	 *   to an OpenGL polygon mesh that persists as long as the feature remains unchanged.
	 *
	 * NOTE: This only reconstructs features which can be reconstructed
	 * as @a ReconstructedFeatureGeometry objects (eg, does not handle topological features).
	 */
	class ReconstructContext
	{
	public:
		//! Typedef for a geometry type.
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_type;

		/**
		 * Typedef for a handle to a geometry feature property.
		 *
		 * It's referred to as 'resolved' instead of present day because it could be a
		 * time-dependent property. In either case it is *not* the reconstructed geometry.
		 *
		 * This property handle, along with methods @a get_present_day ,@a get_reconstructed_feature_geometries, etc,
		 * can be used by clients to efficiently map any reconstructed feature geometry, across
		 * all features, to its present day, or even resolved, geometry. This is useful when
		 * the client needs to associate an object with each present day geometry such as
		 * an OpenGL polygon mesh - the geometry property handle can then be used to quickly
		 * locate the OpenGL polygon mesh.
		 */
		typedef unsigned int geometry_property_handle_type;


		/**
		 * Used to associate a reconstructed feature geometry with its resolved geometry (ie, *unreconstructed*).
		 */
		class Reconstruction
		{
		public:
			Reconstruction(
					geometry_property_handle_type geometry_property_handle,
					const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_feature_geometry) :
				d_geometry_property_handle(geometry_property_handle),
				d_reconstructed_feature_geometry(reconstructed_feature_geometry)
			{  }

			//! Returns the geometry property handle.
			geometry_property_handle_type
			get_geometry_property_handle() const
			{
				return d_geometry_property_handle;
			}

			//! Returns the reconstructed feature geometry.
			const ReconstructedFeatureGeometry::non_null_ptr_type &
			get_reconstructed_feature_geometry() const
			{
				return d_reconstructed_feature_geometry;
			}

		private:
			geometry_property_handle_type d_geometry_property_handle;
			ReconstructedFeatureGeometry::non_null_ptr_type d_reconstructed_feature_geometry;
		};


		/**
		 * Used to associate a feature with its reconstructed feature geometry(s).
		 */
		class ReconstructedFeature
		{
		public:
			//! Typedef for a sequence of @a Reconstruction objects.
			typedef std::vector<Reconstruction> reconstruction_seq_type;


			explicit
			ReconstructedFeature(
					const GPlatesModel::FeatureHandle::weak_ref &feature) :
				d_feature(feature)
			{  }

			ReconstructedFeature(
					const GPlatesModel::FeatureHandle::weak_ref &feature,
					const reconstruction_seq_type &reconstructions) :
				d_feature(feature),
				d_reconstructions(reconstructions)
			{  }

			/**
			 * Returns the feature.
			 */
			const GPlatesModel::FeatureHandle::weak_ref &
			get_feature() const
			{
				return d_feature;
			}

			/**
			 * Returns the reconstructed feature geometries of 'this' feature.
			 *
			 * NOTE: The returned sequence can be empty if the feature is inactive at the
			 * reconstruction time for example.
			 */
			const reconstruction_seq_type &
			get_reconstructions() const
			{
				return d_reconstructions;
			}

		private:
			GPlatesModel::FeatureHandle::weak_ref d_feature;

			reconstruction_seq_type d_reconstructions;

			friend class ReconstructContext;
		};


		/**
		 * Similar to @a Reconstruction but for a span of times rather than a single time.
		 */
		class ReconstructionTimeSpan
		{
		public:

			/**
			 * A time span of RFGs over the range [begin_time, end_time] (passed into constructor).
			 */
			typedef TimeSpanUtils::TimeSampleSpan<ReconstructedFeatureGeometry::non_null_ptr_type>
					rfg_time_sample_span_type;


			ReconstructionTimeSpan(
					geometry_property_handle_type geometry_property_handle,
					GPlatesModel::FeatureHandle::iterator geometry_property_iterator,
					const TimeSpanUtils::TimeRange &time_range) :
				d_geometry_property_handle(geometry_property_handle),
				d_geometry_property_iterator(geometry_property_iterator),
				d_rfg_time_sample_span(rfg_time_sample_span_type::create(time_range))
			{  }

			ReconstructionTimeSpan(
					geometry_property_handle_type geometry_property_handle,
					GPlatesModel::FeatureHandle::iterator geometry_property_iterator,
					const rfg_time_sample_span_type::non_null_ptr_type &rfg_time_sample_span) :
				d_geometry_property_handle(geometry_property_handle),
				d_geometry_property_iterator(geometry_property_iterator),
				d_rfg_time_sample_span(rfg_time_sample_span)
			{  }


			//! Returns the geometry property handle.
			geometry_property_handle_type
			get_geometry_property_handle() const
			{
				return d_geometry_property_handle;
			}

			/**
			 * Returns the geometry property iterator.
			 *
			 * Note: This is also available from each RFG in the time span.
			 */
			const GPlatesModel::FeatureHandle::iterator
			get_geometry_property_iterator() const
			{
				return d_geometry_property_iterator;
			}

			//! Returns the time range that the RFGs span.
			TimeSpanUtils::TimeRange
			get_time_range() const
			{
				return d_rfg_time_sample_span->get_time_range();
			}

			/**
			 * Alternatively can directly use TimeSpanUtils::TimeSampleSpan interface to extract RFGs.
			 */
			rfg_time_sample_span_type::non_null_ptr_to_const_type
			get_reconstructed_feature_geometry_time_span() const
			{
				return d_rfg_time_sample_span;
			}

		private:
			geometry_property_handle_type d_geometry_property_handle;
			GPlatesModel::FeatureHandle::iterator d_geometry_property_iterator;
			rfg_time_sample_span_type::non_null_ptr_type d_rfg_time_sample_span;

			friend class ReconstructContext;
		};


		/**
		 * Similar to @a ReconstructedFeature but for a span of times rather than a single time.
		 */
		class ReconstructedFeatureTimeSpan
		{
		public:
			//! Typedef for a sequence of @a ReconstructionTimeSpan objects.
			typedef std::vector<ReconstructionTimeSpan> reconstruction_time_span_seq_type;


			ReconstructedFeatureTimeSpan(
					const GPlatesModel::FeatureHandle::weak_ref &feature,
					const TimeSpanUtils::TimeRange &time_range) :
				d_feature(feature),
				d_time_range(time_range)
			{  }

			ReconstructedFeatureTimeSpan(
					const GPlatesModel::FeatureHandle::weak_ref &feature,
					const TimeSpanUtils::TimeRange &time_range,
					const reconstruction_time_span_seq_type &reconstruction_time_spans) :
				d_feature(feature),
				d_time_range(time_range),
				d_reconstruction_time_spans(reconstruction_time_spans)
			{  }

			/**
			 * Returns the feature.
			 */
			const GPlatesModel::FeatureHandle::weak_ref &
			get_feature() const
			{
				return d_feature;
			}

			//! Returns the time range that the reconstructions span.
			const TimeSpanUtils::TimeRange &
			get_time_range() const
			{
				return d_time_range;
			}

			/**
			 * Returns the time spans of reconstructed feature geometries of 'this' feature.
			 */
			const reconstruction_time_span_seq_type &
			get_reconstruction_time_spans() const
			{
				return d_reconstruction_time_spans;
			}

		private:
			GPlatesModel::FeatureHandle::weak_ref d_feature;
			TimeSpanUtils::TimeRange d_time_range;
			reconstruction_time_span_seq_type d_reconstruction_time_spans;

			friend class ReconstructContext;
		};


		/**
		 * Similar to @a ReconstructedFeatureTimeSpan but specific to feature's reconstructed using topologies and
		 * returns a TopologyReconstruct::GeometryTimeSpan instead of a @a TopologyReconstructedFeatureGeometry.
		 */
		class TopologyReconstructedFeatureTimeSpan
		{
		public:

			/**
			 * Association of geometry time span with geometry property.
			 */
			class GeometryTimeSpan
			{
			public:

				GeometryTimeSpan(
						GPlatesModel::FeatureHandle::iterator geometry_property_iterator,
						const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span) :
					d_geometry_property_iterator(geometry_property_iterator),
					d_geometry_time_span(geometry_time_span)
				{  }

				/**
				 * Returns the geometry property iterator.
				 */
				const GPlatesModel::FeatureHandle::iterator
				get_geometry_property_iterator() const
				{
					return d_geometry_property_iterator;
				}

				/**
				 * The geometry time span associated with this geometry property.
				 */
				TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type
				get_geometry_time_span() const
				{
					return d_geometry_time_span;
				}

			private:
				GPlatesModel::FeatureHandle::iterator d_geometry_property_iterator;
				TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type d_geometry_time_span;
			};

			//! Typedef for a sequence of @a GeometryTimeSpan objects.
			typedef std::vector<GeometryTimeSpan> geometry_time_span_seq_type;


			TopologyReconstructedFeatureTimeSpan(
					const GPlatesModel::FeatureHandle::weak_ref &feature) :
				d_feature(feature)
			{  }

			TopologyReconstructedFeatureTimeSpan(
					const GPlatesModel::FeatureHandle::weak_ref &feature,
					const geometry_time_span_seq_type &geometry_time_spans) :
				d_feature(feature),
				d_geometry_time_spans(geometry_time_spans)
			{  }

			/**
			 * Returns the feature.
			 */
			const GPlatesModel::FeatureHandle::weak_ref &
			get_feature() const
			{
				return d_feature;
			}

			/**
			 * Returns the geometry time spans of 'this' feature.
			 */
			const geometry_time_span_seq_type &
			get_geometry_time_spans() const
			{
				return d_geometry_time_spans;
			}

		private:
			GPlatesModel::FeatureHandle::weak_ref d_feature;
			geometry_time_span_seq_type d_geometry_time_spans;

			friend class ReconstructContext;
		};


		/**
		 * Extrinsic reconstruction state that features are reconstructed with.
		 *
		 * The intrinsic state is the properties of the features being reconstructed.
		 *
		 * Both types of state are needed to reconstruct features.
		 * Keeping the reconstruct context state extrinsic allows us to use a single
		 * @a ReconstructContext instance with multiple context states and hence re-use the
		 * common feature-to-reconstruct-method-type mapping across all context states.
		 */
		class ContextState :
				private boost::noncopyable
		{
		public:

			//
			// Limited public interface - does not return internal reconstruct methods.
			// This is essentially the Memento pattern.
			//

			const ReconstructMethodInterface::Context &
			get_reconstruction_method_context() const
			{
				return d_reconstruct_method_context;
			}

		private:

			//! Typedef for a sequence of reconstruct methods.
			typedef std::vector<ReconstructMethodInterface::non_null_ptr_type> reconstruct_method_seq_type;

			ReconstructMethodInterface::Context d_reconstruct_method_context;
			reconstruct_method_seq_type d_reconstruct_methods;

			explicit
			ContextState(
					const ReconstructMethodInterface::Context &reconstruct_method_context) :
				d_reconstruct_method_context(reconstruct_method_context)
			{  }

			//! Only parent class can access.
			friend class ReconstructContext;
		};

		//! Typedef for a reference to a context state.
		typedef boost::shared_ptr<ContextState> context_state_reference_type;

		//! Typedef for a weak reference to a context state.
		typedef boost::weak_ptr<ContextState> context_state_weak_reference_type;


		/**
		 * Constructor defaults to no features.
		 *
		 * Features can be subsequently added using @a set_features.
		 *
		 * Note that @a reconstruct_method_registry must exist for this life of 'this' instance.
		 */
		explicit
		ReconstructContext(
				const ReconstructMethodRegistry &reconstruct_method_registry);


		/**
		 * Adds the specified features after removing any features added in a previous call to
		 * @a set_features and for each feature in each feature collection determines which
		 * reconstruct method to use.
		 *
		 * Calls to @a get_reconstructed_feature_geometries, etc, will then use that mapping of features to
		 * reconstruct methods (and the context state passed in) when carrying out reconstructions.
		 *
		 * If @a reconstructable_features is specified then the subset of features that are reconstructable are returned.
		 *
		 * Note: If the features change you should call @a set_features again.
		 * This is because each feature might now require a different reconstruct method.
		 */
		void
		set_features(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections,
				boost::optional<std::vector<GPlatesModel::FeatureHandle::weak_ref> &> reconstructable_features = boost::none);

		/**
		 * Overload accepting a sequence of features instead of feature collections.
		 */
		void
		set_features(
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &features,
				boost::optional<std::vector<GPlatesModel::FeatureHandle::weak_ref> &> reconstructable_features = boost::none);


		/**
		 * Creates a context state associated with the specified reconstruct context state.
		 *
		 * The returned shared reference can be passed to @a get_reconstructed_feature_geometries, etc,
		 * in order to reconstruct the features with a particular reconstruct context state.
		 */
		context_state_reference_type
		create_context_state(
				const ReconstructMethodInterface::Context &reconstruct_method_context);


		/**
		 * The same as @a get_resolved_feature_geometries with a reconstruction time of zero
		 * except the returned sequence contains geometries instead of optional geometries -
		 * this is because the value of the geometry property (at time zero) is obtained
		 * regardless of whether its active at present day or not - in the majority of cases
		 * it will be active at present day.
		 *
		 * TODO: May need to revisit the notion of disregarding active state at present day.
		 *
		 * The returned sequence can be indexed using @a geometry_property_handle_type.
		 *
		 * The returned reference is valid until @a set_features is called.
		 */
		const std::vector<geometry_type> &
		get_present_day_feature_geometries();


#if 0
		/**
		 * Returns the resolved geometries for the geometry properties of the features specified
		 * in the constructor, or the most recent call to @a set_features,
		 * at the specified reconstruction time.
		 *
		 * The returned sequence can be indexed using @a geometry_property_handle_type.
		 *
		 * If a geometry property is time-dependent and is not active at @a reconstruction_time
		 * then the corresponding resolved geometry will be boost::none.
		 */
		std::vector<boost::optional<geometry_type> >
		get_resolved_feature_geometries(
				const double &reconstruction_time);
#endif


		/**
		 * Reconstructs the features specified in the constructor, or the most recent call
		 * to @a set_features, to the specified reconstruction time
		 * using the specified reconstruct context state.
		 *
		 * This method will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 */
		ReconstructHandle::type
		get_reconstructed_feature_geometries(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const context_state_reference_type &context_state_ref,
				const double &reconstruction_time);


		/**
		 * Reconstructs the features specified in the constructor, or the most recent call
		 * to @a set_features, to the specified reconstruction time
		 * using the specified reconstruct context state.
		 *
		 * This method will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 *
		 * This differs @a get_reconstructed_feature_geometries in that this method also
		 * associates each reconstructed feature geometry with the feature geometry property
		 * it was reconstructed from.
		 */
		ReconstructHandle::type
		get_reconstructions(
				std::vector<Reconstruction> &reconstructed_feature_geometries,
				const context_state_reference_type &context_state_ref,
				const double &reconstruction_time);


		/**
		 * Reconstructs the features specified in the constructor, or the most recent call
		 * to @a set_features, to the specified reconstruction time
		 * using the specified reconstruct context state.
		 *
		 * This method will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 *
		 * This differs @a get_reconstructed_feature_geometries in that this method returns
		 * reconstructions grouped by *feature*.
		 * Note that even if a feature is not active (or generates no reconstructions for some reason)
		 * it will still be returned (it just won't have any reconstructions in it) - this is useful
		 * for co-registration, for example, which correlates by feature over several frames (times).
		 */
		ReconstructHandle::type
		get_reconstructed_features(
				std::vector<ReconstructedFeature> &reconstructed_features,
				const context_state_reference_type &context_state_ref,
				const double &reconstruction_time);


		/**
		 * This is similar to @a get_reconstructions but reconstructs over a time range of
		 * reconstruction times instead of a single reconstruction time.
		 */
		ReconstructHandle::type
		get_reconstruction_time_spans(
				std::vector<ReconstructionTimeSpan> &reconstruction_time_spans,
				const context_state_reference_type &context_state_ref,
				const TimeSpanUtils::TimeRange &time_range);


		/**
		 * This is similar to @a get_reconstructed_features but reconstructs over a time range of
		 * reconstruction times instead of a single reconstruction time.
		 */
		ReconstructHandle::type
		get_reconstructed_feature_time_spans(
				std::vector<ReconstructedFeatureTimeSpan> &reconstructed_feature_time_spans,
				const context_state_reference_type &context_state_ref,
				const TimeSpanUtils::TimeRange &time_range);


		/**
		 * Returns any topology-reconstructed feature time spans.
		 *
		 * These are only used when features are reconstructed using *topologies*.
		 * They store the results of incrementally reconstructing using resolved topological plates/networks.
		 * If the features are *not* reconstructed using topologies then no geometry time spans will be returned.
		 */
		void
		get_topology_reconstructed_feature_time_spans(
				std::vector<TopologyReconstructedFeatureTimeSpan> &topology_reconstructed_feature_time_spans,
				const context_state_reference_type &context_state_ref);


		/**
		 * Reconstructs the features specified in the constructor, or the most recent call
		 * to @a set_features, to the specified reconstruction time using the specified
		 * reconstruct context state and limited to features matching the specified feature IDs.
		 *
		 * This method is similar to @a get_reconstructed_feature_geometries, except it limits
		 * features to those matching the specified feature IDs.
		 *
		 * This method is meant as an optimisation to avoid unnecessary reconstructions.
		 *
		 * This method will get the next (incremented) global reconstruct handle and store it
		 * in each @a ReconstructedFeatureGeometry instance created (and return the handle).
		 */
		ReconstructHandle::type
		get_reconstructed_topological_sections(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_topological_sections,
				const std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
				const context_state_reference_type &context_state_ref,
				const double &reconstruction_time);


		/**
		 * Calculate velocities at the geometry reconstruction positions of the features specified
		 * in the constructor, or the most recent call to @a set_features, at the specified
		 * reconstruction time using the specified reconstruct context state.
		 *
		 * This method will get the next (incremented) global reconstruct handle and store it
		 * in the @a MultiPointVectorField velocity objects.
		 */
		ReconstructHandle::type
		reconstruct_feature_velocities(
				std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
				const context_state_reference_type &context_state_ref,
				const double &reconstruction_time,
				const double &velocity_delta_time = 1.0,
				VelocityDeltaTime::Type velocity_delta_time_type = VelocityDeltaTime::T_PLUS_MINUS_HALF_DELTA_T);

	private:

		/**
		 * Groups a feature with its geometry properties.
		 */
		struct ReconstructMethodFeature
		{
			struct GeometryPropertyToHandle
			{
				GPlatesModel::FeatureHandle::iterator property_iterator;
				geometry_property_handle_type geometry_property_handle;
			};

			//! Typedef for a sequence mapping geometry properties to geometry property handles.
			typedef std::vector<GeometryPropertyToHandle> geometry_property_to_handle_seq_type;


			ReconstructMethodFeature(
					const GPlatesModel::FeatureHandle::weak_ref &feature_ref_,
					ReconstructMethod::Type reconstruction_method_type_) :
				feature_ref(feature_ref_),
				reconstruction_method_type(reconstruction_method_type_)
			{  }


			GPlatesModel::FeatureHandle::weak_ref feature_ref;

			/**
			 * The default reconstruct method associated with the feature.
			 */
			ReconstructMethod::Type reconstruction_method_type;

			//! Each reconstructable geometry property in the feature maps to a geometry property handle.
			geometry_property_to_handle_seq_type geometry_property_to_handle_seq;
		};

		//! Typedef for a sequence of reconstruct methods and their associated features.
		typedef std::vector<ReconstructMethodFeature> reconstruct_method_feature_seq_type;


		//! Typedef for a sequence of context states.
		typedef std::vector<context_state_weak_reference_type> context_state_weak_ref_seq_type;


		/**
		 * Used to assign reconstruct methods to features.
		 */
		const ReconstructMethodRegistry &d_reconstruct_method_registry;

		//! A sequence of features associated with their reconstruct method.
		reconstruct_method_feature_seq_type d_reconstruct_method_feature_seq;

		/**
		 * The context states that the client has created.
		 *
		 * These contain *weak* references which expire when the referenced context states
		 * (owned by the client) are destroyed.
		 * When they expire we can re-use their slots when the client creates new context states.
		 */
		context_state_weak_ref_seq_type d_context_states;

		/**
		 * The present day geometries of all reconstructable geometry properties of all features.
		 */
		boost::optional<std::vector<geometry_type> > d_cached_present_day_geometries;


		/**
		 * Converts the reconstructed feature geometries, of the specified feature, to reconstructions.
		 */
		void
		get_feature_reconstructions(
				std::vector<Reconstruction> &reconstructions,
				const ReconstructMethodFeature::geometry_property_to_handle_seq_type &feature_geometry_property_handles,
				const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries);

		/**
		 * Add the reconstructed feature geometries, of the specified feature, to reconstruction time spans.
		 */
		void
		build_feature_reconstruction_time_spans(
				std::vector<ReconstructionTimeSpan> &reconstruction_time_spans,
				const ReconstructMethodFeature::geometry_property_to_handle_seq_type &feature_geometry_property_handles,
				const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const TimeSpanUtils::TimeRange &time_range,
				unsigned int time_slot);

		/**
		 * Returns true if the geometry property handles have been assigned and are up-to-date
		 * with the current set of features.
		 */
		bool
		have_assigned_geometry_property_handles() const
		{
			return static_cast<bool>(d_cached_present_day_geometries);
		}

		/**
		 * Iterates over the assigned features and assigns geometry property handles.
		 */
		void
		assign_geometry_property_handles();

		void
		initialise_context_states();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTCONTEXT_H
