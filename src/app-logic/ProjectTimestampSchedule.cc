/*
 * Copyright (C) 2026 CaliTarheel
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#include "ProjectTimestampSchedule.h"

#include <cmath>
#include <stdexcept>
#include <QFile>

#include "ProjectDocumentRegistry.h"
#include "ProjectMetadata.h"


GPlatesAppLogic::ProjectTimestampSchedule::ProjectTimestampSchedule(
		ProjectDocumentRegistry &document_registry,
		QObject *parent) :
	QObject(parent),
	d_document_registry(&document_registry),
	d_source(NO_PROJECT_TIMESTAMPS)
{
	QObject::connect(
			d_document_registry,
			SIGNAL(primary_document_changed(const QString &)),
			this,
			SLOT(reload_from_primary_document()));
	QObject::connect(
			d_document_registry,
			SIGNAL(primary_document_saved(const QString &)),
			this,
			SLOT(reload_from_primary_document()));
	reload_from_primary_document();
}


const std::vector<double> &
GPlatesAppLogic::ProjectTimestampSchedule::timestamps_older_to_younger() const
{
	return d_timestamps_older_to_younger;
}


boost::optional<double>
GPlatesAppLogic::ProjectTimestampSchedule::next_older_timestamp(
		double current_time) const
{
	boost::optional<double> adjacent;
	for (std::vector<double>::const_iterator timestamp = d_timestamps_older_to_younger.begin();
			timestamp != d_timestamps_older_to_younger.end(); ++timestamp)
	{
		if (*timestamp > current_time + 1e-9 && (!adjacent || *timestamp < adjacent.get()))
		{
			adjacent = *timestamp;
		}
	}
	return adjacent;
}


boost::optional<double>
GPlatesAppLogic::ProjectTimestampSchedule::next_younger_timestamp(
		double current_time) const
{
	boost::optional<double> adjacent;
	for (std::vector<double>::const_iterator timestamp = d_timestamps_older_to_younger.begin();
			timestamp != d_timestamps_older_to_younger.end(); ++timestamp)
	{
		if (*timestamp < current_time - 1e-9 && (!adjacent || *timestamp > adjacent.get()))
		{
			adjacent = *timestamp;
		}
	}
	return adjacent;
}


boost::optional<double>
GPlatesAppLogic::ProjectTimestampSchedule::default_older_bound(
		double current_time) const
{
	return next_older_timestamp(current_time);
}


GPlatesAppLogic::ProjectTimestampSchedule::Source
GPlatesAppLogic::ProjectTimestampSchedule::source() const
{
	return d_source;
}


const QString &
GPlatesAppLogic::ProjectTimestampSchedule::diagnostic() const
{
	return d_diagnostic;
}


void
GPlatesAppLogic::ProjectTimestampSchedule::reload_from_primary_document()
{
	const QString primary_document_path = d_document_registry->primary_document_path();
	if (primary_document_path.isEmpty())
	{
		set_state(std::vector<double>(), NO_PROJECT_TIMESTAMPS, QString());
		return;
	}

	QFile primary_document(primary_document_path);
	if (!primary_document.open(QIODevice::ReadOnly))
	{
		set_state(
				std::vector<double>(),
				INVALID_PROJECT_METADATA,
				tr("The Primary Project Document could not be read: %1.")
						.arg(primary_document.errorString()));
		return;
	}

	const ProjectMetadata metadata =
			ProjectMetadataParser::parse(QString::fromUtf8(primary_document.readAll()));
	if (!metadata.has_front_matter ||
			(metadata.required_timestamps_are_valid && !metadata.required_timestamps_ma))
	{
		set_state(std::vector<double>(), NO_PROJECT_TIMESTAMPS, QString());
		return;
	}
	if (!metadata.required_timestamps_are_valid || !metadata.required_timestamps_ma)
	{
		set_state(
				std::vector<double>(),
				INVALID_PROJECT_METADATA,
				metadata.required_timestamps_diagnostic);
		return;
	}

	set_state(metadata.required_timestamps_ma.get(), PROJECT_MARKDOWN, QString());
}


void
GPlatesAppLogic::ProjectTimestampSchedule::set_state(
		const std::vector<double> &timestamps,
		Source source,
		const QString &diagnostic)
{
	if (d_timestamps_older_to_younger == timestamps &&
			d_source == source && d_diagnostic == diagnostic)
	{
		return;
	}

	d_timestamps_older_to_younger = timestamps;
	d_source = source;
	d_diagnostic = diagnostic;
	Q_EMIT schedule_changed();
}


std::vector<double>
GPlatesAppLogic::ProjectTimestampSchedule::build(
		double youngest_time,
		double oldest_time,
		double step,
		std::size_t maximum_timestamps)
{
	if (!std::isfinite(youngest_time) || !std::isfinite(oldest_time) ||
			!std::isfinite(step))
	{
		throw std::invalid_argument("Timestamp bounds and step must be finite.");
	}
	if (youngest_time < 0.0 || oldest_time < youngest_time || step <= 0.0)
	{
		throw std::invalid_argument(
				"Timestamp schedule requires 0 <= youngest <= oldest and step > 0.");
	}
	if (maximum_timestamps == 0)
	{
		throw std::invalid_argument("Timestamp schedule capacity must be positive.");
	}

	const double span = oldest_time - youngest_time;
	const double regular_intervals_value = std::floor(span / step + 1e-12);
	if (regular_intervals_value + 1.0 > static_cast<double>(maximum_timestamps))
	{
		throw std::length_error("Timestamp schedule exceeds its safety limit.");
	}
	const std::size_t regular_intervals =
			static_cast<std::size_t>(regular_intervals_value);
	const bool needs_oldest_endpoint =
			std::fabs(youngest_time + regular_intervals * step - oldest_time) > 1e-9;
	const std::size_t timestamp_count = regular_intervals + 1 +
			(needs_oldest_endpoint ? 1 : 0);
	if (timestamp_count > maximum_timestamps)
	{
		throw std::length_error("Timestamp schedule exceeds its safety limit.");
	}

	std::vector<double> timestamps;
	timestamps.reserve(timestamp_count);
	for (std::size_t index = 0; index <= regular_intervals; ++index)
	{
		timestamps.push_back(youngest_time + index * step);
	}
	if (needs_oldest_endpoint)
	{
		timestamps.push_back(oldest_time);
	}
	else
	{
		// Eliminate accumulated floating-point drift at the inclusive endpoint.
		timestamps.back() = oldest_time;
	}
	return timestamps;
}
