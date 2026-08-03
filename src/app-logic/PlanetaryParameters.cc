/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 */

#include <QFile>

#include "PlanetaryParameters.h"

#include "ProjectDocumentRegistry.h"
#include "ProjectMetadata.h"

#include "maths/MathsUtils.h"

#include "utils/Earth.h"


GPlatesAppLogic::PlanetaryParameters::PlanetaryParameters(
		ProjectDocumentRegistry &document_registry,
		QObject *parent) :
	QObject(parent),
	d_document_registry(&document_registry),
	d_effective_radius_metres(GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * 1000.0),
	d_radius_source(EARTH_DEFAULT)
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


double
GPlatesAppLogic::PlanetaryParameters::effective_radius_metres() const
{
	return d_effective_radius_metres;
}


double
GPlatesAppLogic::PlanetaryParameters::effective_radius_kilometres() const
{
	return d_effective_radius_metres / 1000.0;
}


GPlatesAppLogic::PlanetaryParameters::RadiusSource
GPlatesAppLogic::PlanetaryParameters::radius_source() const
{
	return d_radius_source;
}


const QString &
GPlatesAppLogic::PlanetaryParameters::radius_diagnostic() const
{
	return d_radius_diagnostic;
}


void
GPlatesAppLogic::PlanetaryParameters::reload_from_primary_document()
{
	const QString primary_document_path = d_document_registry->primary_document_path();
	if (primary_document_path.isEmpty())
	{
		use_earth_default(EARTH_DEFAULT);
		return;
	}

	QFile primary_document(primary_document_path);
	if (!primary_document.open(QIODevice::ReadOnly))
	{
		use_earth_default(
				INVALID_PROJECT_METADATA_USING_EARTH_DEFAULT,
				tr("The Primary Project Document could not be read: %1. GPlates is using the existing Earth-radius default.")
						.arg(primary_document.errorString()));
		return;
	}

	const ProjectMetadata metadata = ProjectMetadataParser::parse(QString::fromUtf8(primary_document.readAll()));
	if (!metadata.has_front_matter)
	{
		use_earth_default(EARTH_DEFAULT);
		return;
	}
	if (!metadata.is_valid || !metadata.planet_radius_metres)
	{
		use_earth_default(
				INVALID_PROJECT_METADATA_USING_EARTH_DEFAULT,
				metadata.diagnostic + tr(" GPlates is using the existing Earth-radius default."));
		return;
	}

	set_radius(metadata.planet_radius_metres.get(), PROJECT_MARKDOWN, QString());
}


void
GPlatesAppLogic::PlanetaryParameters::use_earth_default(
		RadiusSource source,
		const QString &diagnostic)
{
	set_radius(GPlatesUtils::Earth::EQUATORIAL_RADIUS_KMS * 1000.0, source, diagnostic);
}


void
GPlatesAppLogic::PlanetaryParameters::set_radius(
		double radius_metres,
		RadiusSource source,
		const QString &diagnostic)
{
	const bool radius_changed = !GPlatesMaths::are_almost_exactly_equal(
			d_effective_radius_metres,
			radius_metres);
	const bool metadata_state_changed = d_radius_source != source || d_radius_diagnostic != diagnostic;

	d_effective_radius_metres = radius_metres;
	d_radius_source = source;
	d_radius_diagnostic = diagnostic;

	if (radius_changed)
	{
		Q_EMIT effective_radius_changed(d_effective_radius_metres);
	}
	if (radius_changed || metadata_state_changed)
	{
		Q_EMIT metadata_changed();
	}
}
