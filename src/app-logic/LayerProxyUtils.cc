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
#include <boost/foreach.hpp>

#include "LayerProxyUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedFeatureGeometryFinder.h"
#include "Reconstruction.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedTopologicalGeometry.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"

#include "scribe/Scribe.h"
#include "scribe/ScribeExceptions.h"


void
GPlatesAppLogic::LayerProxyUtils::get_reconstructed_feature_geometries(
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
		std::vector<ReconstructHandle::type> &reconstruct_handles,
		const Reconstruction &reconstruction)
{
	// Get the reconstruct layer outputs.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_proxies;
	reconstruction.get_active_layer_outputs<ReconstructLayerProxy>(reconstruct_layer_proxies);

	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstruct_layer_proxy,
			reconstruct_layer_proxies)
	{
		// Get the reconstructed feature geometries from the current layer for the
		// current reconstruction time and anchor plate id.
		const ReconstructHandle::type reconstruct_handle =
				reconstruct_layer_proxy->get_reconstructed_feature_geometries(reconstructed_feature_geometries);

		// Add the reconstruct handle to the list.
		reconstruct_handles.push_back(reconstruct_handle);
	}
}


void
GPlatesAppLogic::LayerProxyUtils::get_resolved_topological_lines(
		std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_lines,
		std::vector<ReconstructHandle::type> &reconstruct_handles,
		const Reconstruction &reconstruction)
{
	// Get the resolved geometry layer outputs.
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> topology_geometry_resolver_layer_proxies;
	reconstruction.get_active_layer_outputs<TopologyGeometryResolverLayerProxy>(topology_geometry_resolver_layer_proxies);

	BOOST_FOREACH(
			const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topology_geometry_resolver_layer_proxy,
			topology_geometry_resolver_layer_proxies)
	{
		// Get the resolved topological lines from the current layer.
		const ReconstructHandle::type reconstruct_handle =
				topology_geometry_resolver_layer_proxy->get_resolved_topological_lines(resolved_topological_lines);

		// Add the reconstruct handle to the list.
		reconstruct_handles.push_back(reconstruct_handle);
	}
}


void
GPlatesAppLogic::LayerProxyUtils::find_reconstructed_feature_geometries_of_feature(
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		const Reconstruction &reconstruction)
{
	// Get the RFGs (and generate if not already) for all active ReconstructLayer's.
	// This is needed because we are about to search the feature for its RFGs and
	// if we don't generate the RFGs (and no one else does either) then they might not be found.
	//
	// The RFGs - we'll keep them active until we've finished searching for RFGs below.
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> rfgs_in_reconstruction;
	std::vector<ReconstructHandle::type> reconstruct_handles;
	get_reconstructed_feature_geometries(rfgs_in_reconstruction, reconstruct_handles, reconstruction);

	// Iterate through all RFGs observing 'feature_ref' and that were reconstructed just now (above).
	ReconstructedFeatureGeometryFinder rfg_finder(
			boost::none/*reconstruction_tree_to_match*/,
			reconstruct_handles);
	rfg_finder.find_rfgs_of_feature(feature_ref);

	// Add the found RFGs to the caller's sequence.
	reconstructed_feature_geometries.insert(
			reconstructed_feature_geometries.end(),
			rfg_finder.found_rfgs_begin(),
			rfg_finder.found_rfgs_end());
}


namespace GPlatesAppLogic
{
	namespace LayerProxyUtils
	{
		template <class LayerProxyType>
		class InputLayerProxy<LayerProxyType>::TranscribeSubjectTokenMethod
		{
		public:

			TranscribeSubjectTokenMethod() :
				d_subject_token_method(NULL)
			{  }

			void
			set_subject_token_method(
					const subject_token_method_type &subject_token_method)
			{
				d_subject_token_method = subject_token_method;
			}

			subject_token_method_type
			get_subject_token_method() const
			{
				return d_subject_token_method;
			}


			typedef std::vector<
					std::pair<std::string, subject_token_method_type> >
							get_token_method_mapping_type;

			/**
			 * This should get initialised with all methods in all 'LayerProxyType' classes.
			 *
			 * This is done in "LayerProxyUtils.cc".
			 */
			static
			const get_token_method_mapping_type &
			get_registered_subject_token_methods();

		private:

			subject_token_method_type d_subject_token_method;


			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data)
			{
				const get_token_method_mapping_type &token_mapping = get_registered_subject_token_methods();

				std::string method_name;

				if (scribe.is_saving())
				{
					typename get_token_method_mapping_type::const_iterator iter = token_mapping.begin();
					typename get_token_method_mapping_type::const_iterator end = token_mapping.end();
					for ( ; iter != end; ++iter)
					{
						if (iter->second == d_subject_token_method)
						{
							method_name = iter->first;
							break;
						}
					}

					// Throw exception in save path (should only happen due to programmer error).
					//
					// If this assertion is triggered then it means:
					//   * the transcribed method was not found in the list of methods
					//     registered in s_token_mapping.
					//
					GPlatesGlobal::Assert<GPlatesScribe::Exceptions::ScribeUserError>(
							iter != end,
							GPLATES_ASSERTION_SOURCE,
							"Subject token method not registered.");
				}

				// Transcribe the method name.
				if (!scribe.transcribe(TRANSCRIBE_SOURCE, method_name, "method_name", GPlatesScribe::DONT_TRACK))
				{
					return scribe.get_transcribe_result();
				}

				if (scribe.is_loading())
				{
					typename get_token_method_mapping_type::const_iterator iter = token_mapping.begin();
					typename get_token_method_mapping_type::const_iterator end = token_mapping.end();
					for ( ; iter != end; ++iter)
					{
						if (iter->first == method_name)
						{
							d_subject_token_method = iter->second;
							break;
						}
					}

					// If method name not found in load path then it means the version of GPlates that
					// generated the transcription (archive) saved a method that we don't know.
					if (iter == end)
					{
						return GPlatesScribe::TRANSCRIBE_UNKNOWN_TYPE;
					}
				}

				return GPlatesScribe::TRANSCRIBE_SUCCESS;
			}

			// Only the scribe system should be able to transcribe.
			friend class GPlatesScribe::Access;
		};


		//
		// Register layer proxy get-subject-token methods for the various layer proxy types.
		//

		//
		// ReconstructionLayerProxy
		//

		template <>
		const InputLayerProxy<ReconstructionLayerProxy>::TranscribeSubjectTokenMethod::get_token_method_mapping_type &
		InputLayerProxy<ReconstructionLayerProxy>::TranscribeSubjectTokenMethod::get_registered_subject_token_methods()
		{
			static bool initialised = false;
			static get_token_method_mapping_type s_token_mapping;

			if (!initialised)
			{
				// WARNING: Changing the string ids will break backward/forward compatibility.
				s_token_mapping.push_back(std::make_pair(
						"get_subject_token",
						&ReconstructionLayerProxy::get_subject_token));

				initialised = true;
			}

			return s_token_mapping;
		}

		//
		// ReconstructLayerProxy
		//

		template <>
		const InputLayerProxy<ReconstructLayerProxy>::TranscribeSubjectTokenMethod::get_token_method_mapping_type &
		InputLayerProxy<ReconstructLayerProxy>::TranscribeSubjectTokenMethod::get_registered_subject_token_methods()
		{
			static bool initialised = false;
			static get_token_method_mapping_type s_token_mapping;

			if (!initialised)
			{
				// WARNING: Changing the string ids will break backward/forward compatibility.
				s_token_mapping.push_back(std::make_pair(
						"get_subject_token",
						&ReconstructLayerProxy::get_subject_token));
				s_token_mapping.push_back(std::make_pair(
						"get_reconstructable_feature_collections_subject_token",
						&ReconstructLayerProxy::get_reconstructable_feature_collections_subject_token));

				initialised = true;
			}

			return s_token_mapping;
		}

		//
		// TopologyNetworkResolverLayerProxy
		//

		template <>
		const InputLayerProxy<TopologyNetworkResolverLayerProxy>::TranscribeSubjectTokenMethod::get_token_method_mapping_type &
		InputLayerProxy<TopologyNetworkResolverLayerProxy>::TranscribeSubjectTokenMethod::get_registered_subject_token_methods()
		{
			static bool initialised = false;
			static get_token_method_mapping_type s_token_mapping;

			if (!initialised)
			{
				// WARNING: Changing the string ids will break backward/forward compatibility.
				s_token_mapping.push_back(std::make_pair(
						"get_subject_token",
						&TopologyNetworkResolverLayerProxy::get_subject_token));

				initialised = true;
			}

			return s_token_mapping;
		}



		template <class LayerProxyType>
		GPlatesScribe::TranscribeResult
		InputLayerProxy<LayerProxyType>::transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data)
		{
			if (transcribed_construct_data)
			{
				return GPlatesScribe::TRANSCRIBE_SUCCESS;
			}

			TranscribeSubjectTokenMethod subject_token_method;

			if (scribe.is_saving())
			{
				subject_token_method.set_subject_token_method(d_subject_token_method);
			}

			if (!scribe.transcribe(TRANSCRIBE_SOURCE, subject_token_method, "subject_token_method", GPlatesScribe::DONT_TRACK) ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, d_input_layer_proxy, "d_input_layer_proxy"))
			{
				return scribe.get_transcribe_result();
			}

			if (scribe.is_loading())
			{
				d_subject_token_method = subject_token_method.get_subject_token_method();
			}

			d_input_layer_proxy_observer_token.reset();

			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		}


		template <class LayerProxyType>
		GPlatesScribe::TranscribeResult
		InputLayerProxy<LayerProxyType>::transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject< InputLayerProxy<LayerProxyType> > &input_layer_proxy)
		{
			TranscribeSubjectTokenMethod subject_token_method;

			if (scribe.is_saving())
			{
				scribe.save(TRANSCRIBE_SOURCE, input_layer_proxy->d_input_layer_proxy, "d_input_layer_proxy");

				subject_token_method.set_subject_token_method(
						input_layer_proxy->d_subject_token_method);

			}
			
			if (!scribe.transcribe(TRANSCRIBE_SOURCE,
					subject_token_method, "subject_token_method", GPlatesScribe::DONT_TRACK))
			{
				return scribe.get_transcribe_result();
			}

			if (scribe.is_loading())
			{
				GPlatesScribe::LoadRef<layer_proxy_non_null_ptr_type> layer_proxy =
						scribe.load<layer_proxy_non_null_ptr_type>(
								TRANSCRIBE_SOURCE, "d_input_layer_proxy");
				if (!layer_proxy.is_valid())
				{
					return scribe.get_transcribe_result();
				}

				input_layer_proxy.construct_object(layer_proxy, subject_token_method.get_subject_token_method());

				scribe.relocated(TRANSCRIBE_SOURCE, input_layer_proxy->d_input_layer_proxy, layer_proxy);
			}

			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		}


		template <class LayerProxyType>
		GPlatesScribe::TranscribeResult
		OptionalInputLayerProxy<LayerProxyType>::transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data)
		{
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_optional_input_layer_proxy, "d_optional_input_layer_proxy"))
			{
				return scribe.get_transcribe_result();
			}

			d_is_none_and_up_to_date = !d_optional_input_layer_proxy;

			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		}


		template <class LayerProxyType>
		GPlatesScribe::TranscribeResult
		InputLayerProxySequence<LayerProxyType>::transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data)
		{
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_seq, "d_seq"))
			{
				return scribe.get_transcribe_result();
			}

			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		}
	}
}


// Explicitly instantiate the transcribe methods, etc, above so we don't have to include
// heavyweight "Scribe.h" in the header file.
// We only need to instantate the 'InputLayerProxySequence' and 'OptionalInputLayerProxy'
// transcribe methods since they'll instantiate the rest.

template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::LayerProxyUtils::InputLayerProxySequence<
		GPlatesAppLogic::ReconstructionLayerProxy>::transcribe(
				GPlatesScribe::Scribe &,
				bool);
template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::LayerProxyUtils::OptionalInputLayerProxy<
		GPlatesAppLogic::ReconstructionLayerProxy>::transcribe(
				GPlatesScribe::Scribe &,
				bool);

template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::LayerProxyUtils::InputLayerProxySequence<
		GPlatesAppLogic::ReconstructLayerProxy>::transcribe(
				GPlatesScribe::Scribe &,
				bool);
template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::LayerProxyUtils::OptionalInputLayerProxy<
		GPlatesAppLogic::ReconstructLayerProxy>::transcribe(
				GPlatesScribe::Scribe &,
				bool);

template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::LayerProxyUtils::InputLayerProxySequence<
		GPlatesAppLogic::TopologyNetworkResolverLayerProxy>::transcribe(
				GPlatesScribe::Scribe &,
				bool);
template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::LayerProxyUtils::OptionalInputLayerProxy<
		GPlatesAppLogic::TopologyNetworkResolverLayerProxy>::transcribe(
				GPlatesScribe::Scribe &,
				bool);
