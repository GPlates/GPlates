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

#include <algorithm>
#include <boost/foreach.hpp>
#include <QDebug>
#include <QFileInfo>

#include "ScalarField3DLayerTask.h"

#include "ExtractScalarField3DFeatureProperties.h"
#include "ScalarField3DLayerProxy.h"

#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/ScalarField3DFileFormatReader.h"

#include "model/FeatureVisitor.h"

#include "utils/UnicodeStringUtils.h"

const QString GPlatesAppLogic::ScalarField3DLayerTask::SCALAR_FIELD_FEATURE_CHANNEL_NAME = "Scalar field feature";
const QString GPlatesAppLogic::ScalarField3DLayerTask::SURFACE_GEOMETRIES_CHANNEL_NAME = "Surface geometries";


bool
GPlatesAppLogic::ScalarField3DLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return contains_scalar_field_3d_feature(feature_collection);
}


GPlatesAppLogic::ScalarField3DLayerTask::ScalarField3DLayerTask() :
	d_scalar_field_layer_proxy(ScalarField3DLayerProxy::create())
{
	// Defined in ".cc" file because non_null_ptr_type requires complete type for its destructor.
	// Data member destructors can get called if exception thrown in this constructor.
}


GPlatesAppLogic::ScalarField3DLayerTask::~ScalarField3DLayerTask()
{
	// Defined in ".cc" file because non_null_ptr_type requires complete type for its destructor.
}


std::vector<GPlatesAppLogic::LayerInputChannelType>
GPlatesAppLogic::ScalarField3DLayerTask::get_input_channel_types() const
{
	std::vector<LayerInputChannelType> input_channel_types;

	// Channel definition for the raster feature.
	input_channel_types.push_back(
			LayerInputChannelType(
					SCALAR_FIELD_FEATURE_CHANNEL_NAME,
					LayerInputChannelType::ONE_DATA_IN_CHANNEL));

	// Channel definition for the surface geometries:
	// - reconstructed geometries, or
	// - resolved topological dynamic polygons, or
	// - resolved topological networks.
	std::vector<LayerTaskType::Type> surface_geometries_input_channel_types;
	surface_geometries_input_channel_types.push_back(LayerTaskType::RECONSTRUCT);
	surface_geometries_input_channel_types.push_back(LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER);
	surface_geometries_input_channel_types.push_back(LayerTaskType::TOPOLOGY_NETWORK_RESOLVER);
	input_channel_types.push_back(
			LayerInputChannelType(
					SURFACE_GEOMETRIES_CHANNEL_NAME,
					LayerInputChannelType::MULTIPLE_DATAS_IN_CHANNEL,
					surface_geometries_input_channel_types));

	return input_channel_types;
}


QString
GPlatesAppLogic::ScalarField3DLayerTask::get_main_input_feature_collection_channel() const
{
	return SCALAR_FIELD_FEATURE_CHANNEL_NAME;
}


void
GPlatesAppLogic::ScalarField3DLayerTask::add_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == SCALAR_FIELD_FEATURE_CHANNEL_NAME)
	{
		// A raster feature collection should have only one feature.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection->begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection->end();
		if (features_iter == features_end)
		{
			qWarning() << "Scalar field feature collection contains no features.";
			return;
		}

		// Set the scalar field feature in the layer proxy.
		const GPlatesModel::FeatureHandle::weak_ref feature_ref = (*features_iter)->reference();

		// Let the layer task params know of the new scalar field feature.
		d_layer_task_params.set_scalar_field_feature(feature_ref);

		// Let the layer proxy know of the scalar field and let it know of the new parameters.
		d_scalar_field_layer_proxy->set_current_scalar_field_feature(feature_ref, d_layer_task_params);

		// A raster feature collection should have only one feature.
		if (++features_iter != features_end)
		{
			qWarning() << "Scalar field feature collection contains more than one feature - "
					"ignoring all but the first.";
		}
	}
}


void
GPlatesAppLogic::ScalarField3DLayerTask::remove_input_file_connection(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == SCALAR_FIELD_FEATURE_CHANNEL_NAME)
	{
		// A scalar field feature collection should have only one feature.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection->begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection->end();
		if (features_iter == features_end)
		{
			qWarning() << "Scalar field feature collection contains no features.";
			return;
		}

		// Let the layer task params know of that there's now no scalar field feature.
		d_layer_task_params.set_scalar_field_feature(boost::none);

		// Set the scalar field feature to none in the layer proxy and let it know of the new parameters.
		d_scalar_field_layer_proxy->set_current_scalar_field_feature(boost::none, d_layer_task_params);

		// A scalar field feature collection should have only one feature.
		if (++features_iter != features_end)
		{
			qWarning() << "Scalar field feature collection contains more than one feature - "
					"ignoring all but the first.";
		}
	}
}


void
GPlatesAppLogic::ScalarField3DLayerTask::modified_input_file(
		const QString &input_channel_name,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (input_channel_name == SCALAR_FIELD_FEATURE_CHANNEL_NAME)
	{
		// The feature collection has been modified which means it may have a new feature such as when
		// a file is reloaded (same feature collection but all features are removed and reloaded).
		// So we have to assume the existing scalar field feature is no longer valid so we need to set
		// the scalar field feature again.
		//
		// This is pretty much the same as 'add_input_file_connection()'.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection->begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection->end();
		if (features_iter == features_end)
		{
			// A scalar field feature collection should have one feature.
			qWarning() << "Scalar field raster feature collection contains no features.";
			return;
		}

		// Set the scalar field feature in the layer proxy.
		const GPlatesModel::FeatureHandle::weak_ref feature_ref = (*features_iter)->reference();

		// Let the layer task params know of the new scalar field feature.
		d_layer_task_params.set_scalar_field_feature(feature_ref);

		// Let the layer proxy know of the scalar field and let it know of the new parameters.
		d_scalar_field_layer_proxy->set_current_scalar_field_feature(feature_ref, d_layer_task_params);

		// A scalar field feature collection should have only one feature.
		if (++features_iter != features_end)
		{
			qWarning() << "Scalar field raster feature collection contains more than one feature - "
					"ignoring all but the first.";
		}
	}
}


void
GPlatesAppLogic::ScalarField3DLayerTask::add_input_layer_proxy_connection(
		const QString &input_channel_name,
		const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == SURFACE_GEOMETRIES_CHANNEL_NAME)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological boundary resolver,
		// - topological network resolver.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_scalar_field_layer_proxy->add_reconstructed_geometries_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (topological_boundary_resolver_layer_proxy)
		{
			d_scalar_field_layer_proxy->add_topological_boundary_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_boundary_resolver_layer_proxy.get()));
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (topological_network_resolver_layer_proxy)
		{
			d_scalar_field_layer_proxy->add_topological_network_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_network_resolver_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::ScalarField3DLayerTask::remove_input_layer_proxy_connection(
		const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy)
{
	if (input_channel_name == SURFACE_GEOMETRIES_CHANNEL_NAME)
	{
		// The input layer proxy is one of the following layer proxy types:
		// - reconstruct,
		// - topological boundary resolver,
		// - topological network resolver.

		boost::optional<ReconstructLayerProxy *> reconstruct_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<ReconstructLayerProxy>(layer_proxy);
		if (reconstruct_layer_proxy)
		{
			d_scalar_field_layer_proxy->remove_reconstructed_geometries_layer_proxy(
					GPlatesUtils::get_non_null_pointer(reconstruct_layer_proxy.get()));
		}

		boost::optional<TopologyGeometryResolverLayerProxy *> topological_boundary_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyGeometryResolverLayerProxy>(layer_proxy);
		if (topological_boundary_resolver_layer_proxy)
		{
			d_scalar_field_layer_proxy->remove_topological_boundary_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_boundary_resolver_layer_proxy.get()));
		}

		boost::optional<TopologyNetworkResolverLayerProxy *> topological_network_resolver_layer_proxy =
				LayerProxyUtils::get_layer_proxy_derived_type<TopologyNetworkResolverLayerProxy>(layer_proxy);
		if (topological_network_resolver_layer_proxy)
		{
			d_scalar_field_layer_proxy->remove_topological_network_resolver_layer_proxy(
					GPlatesUtils::get_non_null_pointer(topological_network_resolver_layer_proxy.get()));
		}
	}
}


void
GPlatesAppLogic::ScalarField3DLayerTask::update(
		const Reconstruction::non_null_ptr_type &reconstruction)
{
	d_scalar_field_layer_proxy->set_current_reconstruction_time(reconstruction->get_reconstruction_time());
}


GPlatesAppLogic::LayerProxy::non_null_ptr_type
GPlatesAppLogic::ScalarField3DLayerTask::get_layer_proxy()
{
	return d_scalar_field_layer_proxy;
}


const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &
GPlatesAppLogic::ScalarField3DLayerTask::Params::get_scalar_field_feature() const
{
	return d_scalar_field_feature;
}


GPlatesAppLogic::ScalarField3DLayerTask::Params::Params()
{
}


void
GPlatesAppLogic::ScalarField3DLayerTask::Params::set_scalar_field_feature(
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> scalar_field_feature)
{
	d_scalar_field_feature = scalar_field_feature;

	updated_scalar_field_feature();
}


void
GPlatesAppLogic::ScalarField3DLayerTask::Params::updated_scalar_field_feature()
{
	// Clear everything in case error (and return early).
	d_minimum_depth_layer_radius = boost::none;
	d_maximum_depth_layer_radius = boost::none;
	d_scalar_min = boost::none;
	d_scalar_max = boost::none;
	d_scalar_mean = boost::none;
	d_scalar_standard_deviation = boost::none;

	// If there is no scalar field feature then clear everything.
	if (!d_scalar_field_feature)
	{
		return;
	}

	GPlatesAppLogic::ExtractScalarField3DFeatureProperties visitor;
	visitor.visit_feature(d_scalar_field_feature.get());

	if (!visitor.get_scalar_field_filename())
	{
		return;
	}

	const QString scalar_field_file_name =
			GPlatesUtils::make_qstring(*visitor.get_scalar_field_filename());

	if (!QFileInfo(scalar_field_file_name).exists())
	{
		return;
	}

	// Catch exceptions due to incorrect version or bad formatting.
	try
	{
		// Scalar field reader to access parameters in scalar field file.
		const GPlatesFileIO::ScalarField3DFileFormat::Reader scalar_field_reader(scalar_field_file_name);

		// Read the scalar field depth range.
		d_minimum_depth_layer_radius = scalar_field_reader.get_minimum_depth_layer_radius();
		d_maximum_depth_layer_radius = scalar_field_reader.get_maximum_depth_layer_radius();

		// Read the scalar field statistics.
		d_scalar_min = scalar_field_reader.get_scalar_min();
		d_scalar_max = scalar_field_reader.get_scalar_max();
		d_scalar_mean = scalar_field_reader.get_scalar_mean();
		d_scalar_standard_deviation = scalar_field_reader.get_scalar_standard_deviation();
	}
	catch (GPlatesFileIO::ScalarField3DFileFormat::UnsupportedVersion &exc)
	{
		qWarning() << exc;
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &exc)
	{
		qWarning() << exc;
	}
}
