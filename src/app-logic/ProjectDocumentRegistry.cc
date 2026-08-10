/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#include <algorithm>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include "ProjectDocumentRegistry.h"


GPlatesAppLogic::ProjectDocumentRegistry::Document::Document(
		const QString &file_path_,
		const QString &display_name_) :
	file_path(file_path_),
	display_name(display_name_),
	text_is_loaded(false),
	is_dirty(false),
	is_externally_modified(false)
{  }


GPlatesAppLogic::ProjectDocumentRegistry::ProjectDocumentRegistry(
		QObject *parent) :
	QObject(parent)
{  }


int
GPlatesAppLogic::ProjectDocumentRegistry::document_count() const
{
	return static_cast<int>(d_documents.size());
}


const GPlatesAppLogic::ProjectDocumentRegistry::Document &
GPlatesAppLogic::ProjectDocumentRegistry::document_at(
		int index) const
{
	return d_documents.at(index);
}


QString
GPlatesAppLogic::ProjectDocumentRegistry::normalise_path(
		const QString &file_path)
{
	return QDir::cleanPath(QFileInfo(file_path).absoluteFilePath());
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::is_valid_index(
		int index) const
{
	return index >= 0 && index < document_count();
}


int
GPlatesAppLogic::ProjectDocumentRegistry::add_document(
		const QString &file_path,
		const QString &display_name)
{
	const QString normalised_file_path = normalise_path(file_path);
	for (int index = 0; index < document_count(); ++index)
	{
		if (normalise_path(d_documents[index].file_path) == normalised_file_path)
		{
			return index;
		}
	}

	const QString effective_display_name = display_name.isEmpty()
			? QFileInfo(normalised_file_path).fileName()
			: display_name;
	d_documents.push_back(Document(normalised_file_path, effective_display_name));
	const int new_index = document_count() - 1;

	Q_EMIT documents_changed();
	if (!d_primary_document_index)
	{
		set_primary_document(new_index);
	}
	return new_index;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::remove_document(
		int index)
{
	if (!is_valid_index(index))
	{
		return false;
	}

	const bool previous_dirty_state = has_dirty_documents();
	const bool removed_primary = d_primary_document_index && d_primary_document_index.get() == index;
	d_documents.erase(d_documents.begin() + index);

	if (removed_primary)
	{
		d_primary_document_index = boost::none;
	}
	else if (d_primary_document_index && d_primary_document_index.get() > index)
	{
		d_primary_document_index = d_primary_document_index.get() - 1;
	}

	Q_EMIT documents_changed();
	emit_dirty_state_if_changed(previous_dirty_state);
	if (removed_primary)
	{
		Q_EMIT primary_document_changed(QString());
	}
	return true;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::move_document(
		int from_index,
		int to_index)
{
	if (!is_valid_index(from_index) || !is_valid_index(to_index) || from_index == to_index)
	{
		return false;
	}

	const Document document = d_documents[from_index];
	d_documents.erase(d_documents.begin() + from_index);
	d_documents.insert(d_documents.begin() + to_index, document);

	if (d_primary_document_index)
	{
		const int primary = d_primary_document_index.get();
		if (primary == from_index)
		{
			d_primary_document_index = to_index;
		}
		else if (from_index < primary && to_index >= primary)
		{
			d_primary_document_index = primary - 1;
		}
		else if (from_index > primary && to_index <= primary)
		{
			d_primary_document_index = primary + 1;
		}
	}

	Q_EMIT documents_changed();
	return true;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::set_primary_document(
		boost::optional<int> index)
{
	if (index && !is_valid_index(index.get()))
	{
		return false;
	}
	if (index == d_primary_document_index)
	{
		return true;
	}

	d_primary_document_index = index;
	Q_EMIT documents_changed();
	Q_EMIT primary_document_changed(primary_document_path());
	return true;
}


boost::optional<int>
GPlatesAppLogic::ProjectDocumentRegistry::primary_document_index() const
{
	return d_primary_document_index;
}


QString
GPlatesAppLogic::ProjectDocumentRegistry::primary_document_path() const
{
	return d_primary_document_index
			? d_documents[d_primary_document_index.get()].file_path
			: QString();
}


void
GPlatesAppLogic::ProjectDocumentRegistry::clear()
{
	const bool previous_dirty_state = has_dirty_documents();
	const bool had_primary_document = static_cast<bool>(d_primary_document_index);
	d_documents.clear();
	d_primary_document_index = boost::none;
	Q_EMIT documents_changed();
	emit_dirty_state_if_changed(previous_dirty_state);
	if (had_primary_document)
	{
		Q_EMIT primary_document_changed(QString());
	}
}


void
GPlatesAppLogic::ProjectDocumentRegistry::restore_documents(
		const QStringList &file_paths_,
		const QStringList &display_names_,
		boost::optional<int> primary_index)
{
	const bool previous_dirty_state = has_dirty_documents();
	d_documents.clear();

	for (int index = 0; index < file_paths_.size(); ++index)
	{
		const QString path = normalise_path(file_paths_[index]);
		const QString display_name = index < display_names_.size() && !display_names_[index].isEmpty()
				? display_names_[index]
				: QFileInfo(path).fileName();
		d_documents.push_back(Document(path, display_name));
	}

	d_primary_document_index = primary_index && is_valid_index(primary_index.get())
			? primary_index
			: boost::none;

	Q_EMIT documents_changed();
	emit_dirty_state_if_changed(previous_dirty_state);
	Q_EMIT primary_document_changed(primary_document_path());
}


QStringList
GPlatesAppLogic::ProjectDocumentRegistry::file_paths() const
{
	QStringList paths;
	for (int index = 0; index < document_count(); ++index)
	{
		paths.append(d_documents[index].file_path);
	}
	return paths;
}


QStringList
GPlatesAppLogic::ProjectDocumentRegistry::display_names() const
{
	QStringList names;
	for (int index = 0; index < document_count(); ++index)
	{
		names.append(d_documents[index].display_name);
	}
	return names;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::load_document(
		int index,
		QString *error_message)
{
	if (!is_valid_index(index))
	{
		return false;
	}
	if (d_documents[index].text_is_loaded)
	{
		return true;
	}
	return reload_document(index, error_message);
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::reload_document(
		int index,
		QString *error_message)
{
	if (!is_valid_index(index))
	{
		return false;
	}

	QFile file(d_documents[index].file_path);
	if (!file.open(QIODevice::ReadOnly))
	{
		if (error_message)
		{
			*error_message = file.errorString();
		}
		return false;
	}

	const bool previous_dirty_state = has_dirty_documents();
	d_documents[index].text = QString::fromUtf8(file.readAll());
	d_documents[index].text_is_loaded = true;
	d_documents[index].is_dirty = false;
	d_documents[index].is_externally_modified = false;
	Q_EMIT document_content_changed(index);
	Q_EMIT documents_changed();
	emit_dirty_state_if_changed(previous_dirty_state);

	if (d_primary_document_index && d_primary_document_index.get() == index)
	{
		Q_EMIT primary_document_saved(d_documents[index].file_path);
	}
	return true;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::set_document_text(
		int index,
		const QString &text)
{
	if (!is_valid_index(index))
	{
		return false;
	}
	if (d_documents[index].text_is_loaded && d_documents[index].text == text)
	{
		return true;
	}

	const bool previous_dirty_state = has_dirty_documents();
	d_documents[index].text = text;
	d_documents[index].text_is_loaded = true;
	d_documents[index].is_dirty = true;
	Q_EMIT document_content_changed(index);
	emit_dirty_state_if_changed(previous_dirty_state);
	return true;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::save_document(
		int index,
		QString *error_message)
{
	if (!is_valid_index(index) || !d_documents[index].text_is_loaded)
	{
		return false;
	}

	QSaveFile file(d_documents[index].file_path);
	if (!file.open(QIODevice::WriteOnly))
	{
		if (error_message)
		{
			*error_message = file.errorString();
		}
		return false;
	}
	const QByteArray utf8_text = d_documents[index].text.toUtf8();
	if (file.write(utf8_text) != utf8_text.size() || !file.commit())
	{
		if (error_message)
		{
			*error_message = file.errorString();
		}
		return false;
	}

	const bool previous_dirty_state = has_dirty_documents();
	d_documents[index].is_dirty = false;
	d_documents[index].is_externally_modified = false;
	Q_EMIT documents_changed();
	emit_dirty_state_if_changed(previous_dirty_state);
	if (d_primary_document_index && d_primary_document_index.get() == index)
	{
		Q_EMIT primary_document_saved(d_documents[index].file_path);
	}
	return true;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::save_document_as(
		int index,
		const QString &file_path,
		QString *error_message)
{
	if (!is_valid_index(index))
	{
		return false;
	}

	const QString previous_path = d_documents[index].file_path;
	const QString previous_display_name = d_documents[index].display_name;
	d_documents[index].file_path = normalise_path(file_path);
	d_documents[index].display_name = QFileInfo(d_documents[index].file_path).fileName();
	if (!save_document(index, error_message))
	{
		d_documents[index].file_path = previous_path;
		d_documents[index].display_name = previous_display_name;
		return false;
	}

	Q_EMIT documents_changed();
	if (d_primary_document_index && d_primary_document_index.get() == index)
	{
		Q_EMIT primary_document_changed(d_documents[index].file_path);
	}
	return true;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::save_all_dirty_documents(
		QStringList *error_messages)
{
	bool all_saved = true;
	for (int index = 0; index < document_count(); ++index)
	{
		if (!d_documents[index].is_dirty)
		{
			continue;
		}

		QString error_message;
		if (!save_document(index, &error_message))
		{
			all_saved = false;
			if (error_messages)
			{
				error_messages->append(d_documents[index].display_name + ": " + error_message);
			}
		}
	}
	return all_saved;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::relocate_document(
		int index,
		const QString &file_path)
{
	if (!is_valid_index(index))
	{
		return false;
	}

	const bool previous_dirty_state = has_dirty_documents();
	d_documents[index].file_path = normalise_path(file_path);
	d_documents[index].display_name = QFileInfo(d_documents[index].file_path).fileName();
	d_documents[index].text.clear();
	d_documents[index].text_is_loaded = false;
	d_documents[index].is_dirty = false;
	d_documents[index].is_externally_modified = false;
	Q_EMIT documents_changed();
	emit_dirty_state_if_changed(previous_dirty_state);
	if (d_primary_document_index && d_primary_document_index.get() == index)
	{
		Q_EMIT primary_document_changed(d_documents[index].file_path);
	}
	return true;
}


bool
GPlatesAppLogic::ProjectDocumentRegistry::has_dirty_documents() const
{
	for (int index = 0; index < document_count(); ++index)
	{
		if (d_documents[index].is_dirty)
		{
			return true;
		}
	}
	return false;
}


QStringList
GPlatesAppLogic::ProjectDocumentRegistry::dirty_document_names() const
{
	QStringList names;
	for (int index = 0; index < document_count(); ++index)
	{
		if (d_documents[index].is_dirty)
		{
			names.append(d_documents[index].display_name);
		}
	}
	return names;
}


void
GPlatesAppLogic::ProjectDocumentRegistry::set_document_externally_modified(
		int index,
		bool externally_modified)
{
	if (!is_valid_index(index) || d_documents[index].is_externally_modified == externally_modified)
	{
		return;
	}
	d_documents[index].is_externally_modified = externally_modified;
	Q_EMIT documents_changed();
}


void
GPlatesAppLogic::ProjectDocumentRegistry::emit_dirty_state_if_changed(
		bool previous_dirty_state)
{
	const bool current_dirty_state = has_dirty_documents();
	if (previous_dirty_state != current_dirty_state)
	{
		Q_EMIT dirty_state_changed(current_dirty_state);
	}
}
