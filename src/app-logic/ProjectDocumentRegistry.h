/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 */

#ifndef GPLATES_APP_LOGIC_PROJECTDOCUMENTREGISTRY_H
#define GPLATES_APP_LOGIC_PROJECTDOCUMENTREGISTRY_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QObject>
#include <QString>
#include <QStringList>


namespace GPlatesAppLogic
{
	/**
	 * Owns the project/session associations with ordinary Markdown files.
	 *
	 * Document text remains external to the project file. It is held here only
	 * while editing so unsaved state is available outside the GUI layer.
	 */
	class ProjectDocumentRegistry :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		struct Document
		{
			Document(
					const QString &file_path_,
					const QString &display_name_);

			QString file_path;
			QString display_name;
			QString text;
			bool text_is_loaded;
			bool is_dirty;
			bool is_externally_modified;
		};

		explicit
		ProjectDocumentRegistry(
				QObject *parent = NULL);

		int
		document_count() const;

		const Document &
		document_at(
				int index) const;

		int
		add_document(
				const QString &file_path,
				const QString &display_name = QString());

		bool
		remove_document(
				int index);

		bool
		move_document(
				int from_index,
				int to_index);

		bool
		set_primary_document(
				boost::optional<int> index);

		boost::optional<int>
		primary_document_index() const;

		QString
		primary_document_path() const;

		void
		clear();

		void
		restore_documents(
				const QStringList &file_paths,
				const QStringList &display_names,
				boost::optional<int> primary_index);

		QStringList
		file_paths() const;

		QStringList
		display_names() const;

		bool
		load_document(
				int index,
				QString *error_message = NULL);

		bool
		reload_document(
				int index,
				QString *error_message = NULL);

		bool
		set_document_text(
				int index,
				const QString &text);

		bool
		save_document(
				int index,
				QString *error_message = NULL);

		bool
		save_document_as(
				int index,
				const QString &file_path,
				QString *error_message = NULL);

		bool
		relocate_document(
				int index,
				const QString &file_path);

		bool
		save_all_dirty_documents(
				QStringList *error_messages = NULL);

		bool
		has_dirty_documents() const;

		QStringList
		dirty_document_names() const;

		void
		set_document_externally_modified(
				int index,
				bool externally_modified);

	Q_SIGNALS:
		void
		documents_changed();

		void
		document_content_changed(
				int index);

		void
		dirty_state_changed(
				bool has_dirty_documents);

		void
		primary_document_changed(
				const QString &file_path);

		void
		primary_document_saved(
				const QString &file_path);

	private:
		bool
		is_valid_index(
				int index) const;

		static
		QString
		normalise_path(
				const QString &file_path);

		void
		emit_dirty_state_if_changed(
				bool previous_dirty_state);

		std::vector<Document> d_documents;
		boost::optional<int> d_primary_document_index;
	};
}

#endif // GPLATES_APP_LOGIC_PROJECTDOCUMENTREGISTRY_H
