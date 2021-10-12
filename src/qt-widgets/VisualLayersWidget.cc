/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <vector>
#include <algorithm>
#include <iterator>
#include <boost/foreach.hpp>
#include <QObject>
#include <QAbstractListModel>
#include <QAbstractItemDelegate>
#include <QItemDelegate>
#include <QListView>
#include <QPushButton>
#include <QString>
#include <QStringListModel>
#include <QStringList>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QMimeData>
#include <QDrag>
#include <QFrame>
#include <QDragEnterEvent>
#include <QByteArray>
#include <QDataStream>
#include <QDialog>
#include <QBasicTimer>
#include <QDebug>

#include "VisualLayersWidget.h"

#include "QtWidgetUtils.h"
#include "VisualLayerWidget.h"
#include "VisualLayersListView.h"

#include "gui/VisualLayersListModel.h"

namespace
{
	// Although text/plain would suffice, if we used that, users would be able to
	// do silly things like drag from the visual layers list into a text editor.
	const QString VISUAL_LAYERS_MIME_TYPE = "application/gplates.visuallayers.index";

	class MyModel :
			public QAbstractListModel
	{
	public:

		MyModel(
				QObject *parent_ = NULL) :
			QAbstractListModel(parent_),
			d_big_row(0)
		{
			for (int i = 0; i < 5; i++)
			{
				d_data.push_back(i);
			}
		}

		virtual
		Qt::ItemFlags
		flags(
				const QModelIndex &index_) const
		{
			Qt::ItemFlags base_flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
			if (index_.isValid())
			{
				return base_flags | Qt::ItemIsDragEnabled;
			}
			else
			{
				return base_flags;
			}
		}

		virtual
		QVariant
		data(
				const QModelIndex &index_, int role = Qt::DisplayRole) const
		{
			if (index_.isValid() && role == Qt::DisplayRole)
			{
				return QVariant(d_data[index_.row()]);
			}
			else
			{
				return QVariant();
			}
		}

		virtual
		bool
		setData(
				const QModelIndex &index_,
				const QVariant &value,
				int role = Qt::EditRole)
		{
			if (static_cast<size_t>(index_.row()) < d_data.size() && index_.column() == 0)
			{
				d_data[index_.row()] = value.toInt();
				emit dataChanged(index_, index_);
				return true;
			}
			else
			{
				return false;
			}
		}

		bool
		move_data(
				const QModelIndex &from_index,
				const QModelIndex &to_index,
				int role = Qt::EditRole)
		{
			// Sanity checks.
			if (from_index.row() < 0 || to_index.row() < 0)
			{
				return false;
			}
			unsigned int from_row = static_cast<unsigned int>(from_index.row());
			unsigned int to_row = static_cast<unsigned int>(to_index.row());
			if (from_row >= d_data.size() ||
				to_row >= d_data.size() ||
				from_index.column() > 0 ||
				to_index.column() > 0)
			{
				return false;
			}

			if (from_row == to_row)
			{
				// Nothing to do.
				return true;
			}
			else if (from_row < to_row)
			{
#if 0
				// Moving an item down the list towards the back.
				// All elements from from_row + 1 to to_row need to be moved
				// one place towards the front.
				std::vector<int> new_data;
				new_data.reserve(d_data.size());

				// Keep [0, from_row - 1] as is.
				std::copy(
						d_data.begin(),
						d_data.begin() + from_row,
						std::back_inserter(new_data));

				// Then copy [from_row + 1, to_row] to [from_row, to_row - 1].
				std::copy(
						d_data.begin() + from_row + 1,
						d_data.begin() + to_row + 1,
						std::back_inserter(new_data));

				// Put element at from_row at to_row.
				new_data.push_back(d_data[from_row]);

				// Then keep [to_row + 1, end) as is.
				std::copy(
						d_data.begin() + to_row + 1,
						d_data.end(),
						std::back_inserter(new_data));

				std::swap(d_data, new_data);
				emit dataChanged(index(from_row, 0), index(to_row, 0));
#endif
				removeRow(from_row);

				return true;
			}
			else // to_row < from_row
			{
				// Moving an item up the list towards the front.
				// All elements from to_row to from_row - 1 need to be moved
				// one place towards the back.
				std::vector<int> new_data;
				new_data.reserve(d_data.size());

				// Keep [0, to_row - 1] as is.
				std::copy(
						d_data.begin(),
						d_data.begin() + to_row,
						std::back_inserter(new_data));

				// Put element at from_row at to_row.
				new_data.push_back(d_data[from_row]);

				// Then copy [to_row, from_row - 1] to [to_row + 1, from_row].
				std::copy(
						d_data.begin() + to_row,
						d_data.begin() + from_row,
						std::back_inserter(new_data));

				// Then keep [from_row + 1, end) as is.
				std::copy(
						d_data.begin() + from_row + 1,
						d_data.end(),
						std::back_inserter(new_data));

				std::swap(d_data, new_data);
				emit dataChanged(index(to_row, 0), index(from_row, 0));
				return true;
			}
		}

		virtual
		int
		rowCount(
				const QModelIndex &parent_ = QModelIndex()) const
		{
			return d_data.size();
		}

		virtual
		bool
		insertRows(
				int row,
				int count,
				const QModelIndex &parent_ = QModelIndex())
		{
			if (static_cast<size_t>(row) > d_data.size())
			{
				return false;
			}

			beginInsertRows(parent_, row, row + count - 1);

			d_data.insert(
					d_data.begin() + row,
					count,
					0);

			endInsertRows();
			return true;
		}

		virtual
		bool
		removeRows(
				int row,
				int count,
				const QModelIndex &parent_ = QModelIndex())
		{
			if (static_cast<size_t>(row + count) > d_data.size())
			{
				return false;
			}

			beginRemoveRows(parent_, row, row + count - 1);

			d_data.erase(
					d_data.begin() + row,
					d_data.begin() + row + count);

			endRemoveRows();
			return true;
		}

		void
		wants_update()
		{
			emit layoutChanged();
		}

		virtual
		Qt::DropActions
		supportedDropActions() const
		{
			return Qt::MoveAction;
		}

		virtual
		QStringList
		mimeTypes() const
		{
			QStringList types;
			types << VISUAL_LAYERS_MIME_TYPE;
			return types;
		}

#if 0
		QMimeData *
		mimeData(
				const QModelIndexList &indices) const
		{
			QMimeData *mime_data = new QMimeData();
			QByteArray encoded_data;
			QDataStream stream(&encoded_data, QIODevice::WriteOnly);

			// Store the first valid index as a string.
			BOOST_FOREACH(QModelIndex i, indices)
			{
				if (i.isValid())
				{
					stream << QString("%1").arg(i.row());
					break;
				}
			}
			if (encoded_data.size())
			{
				mime_data->setData(VISUAL_LAYERS_MIME_TYPE, encoded_data);
			}

			return mime_data;
		}
#endif

		virtual
		bool
		dropMimeData(
				const QMimeData *mime_data,
				Qt::DropAction action,
				int row,
				int column,
				const QModelIndex &parent_)
		{
			if (action == Qt::IgnoreAction)
			{
				return true;
			}

			if (!mime_data->hasFormat(VISUAL_LAYERS_MIME_TYPE))
			{
				return false;
			}

			if (column > 0)
			{
				return false;
			}

			int to_row;
			if (row != -1)
			{
				to_row = row;
			}
			else if (parent_.isValid())
			{
				// Dropped occurred on an item.
				to_row = parent_.row();
			}
			else
			{
				// Append items if dropped elsewhere in view.
				to_row = d_data.size() - 1;
			}	

			// Read from the mime object.
			QByteArray encoded_data = mime_data->data(VISUAL_LAYERS_MIME_TYPE);
			QDataStream stream(&encoded_data, QIODevice::ReadOnly);
			QString text;
			stream >> text;

			// Attempt to parse as integer.
			bool ok;
			int from_row = text.toInt(&ok);
			if (!ok)
			{
				return false;
			}

			move_data(index(from_row, 0), index(to_row, 0));

			return true;
		}

		int
		big_row() const
		{
			return d_big_row;
		}

		void
		set_big_row(
				int row)
		{
			d_big_row = row;
		}

	private:
	
		std::vector<int> d_data;
		int d_big_row;
	};



	class MyPushButton :
			public QPushButton
	{
	public:

		MyPushButton(
				QListView *list,
				MyModel *model,
				int row,
				QWidget *parent_ = NULL) :
			QPushButton(parent_),
			d_list(list),
			d_model(model),
			d_row(row)
		{  }

		virtual
		void
		mousePressEvent(
				QMouseEvent *event_);

	private:

		QListView *d_list;
		MyModel *d_model;
		int d_row;
	};


	class MyEditWidget :
			public QFrame
	{
	public:

		MyEditWidget(
				QListView *list,
				MyModel *model,
				int row,
				QWidget *parent_ = NULL) :
			QFrame(parent_),
			d_list(list),
			d_model(model),
			d_row(row),
			d_button(new MyPushButton(list, model, row, this))
		{
			setAutoFillBackground(true);
			setFocusPolicy(Qt::StrongFocus);
			setMouseTracking(true);
			setFrameShape(QFrame::Box);

			d_button->setMaximumSize(QSize(40, 40));
			GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(d_button, this);
		}

		~MyEditWidget()
		{
			// qDebug() << "destroyed";
		}

		void
		set_data(
				int value)
		{
			d_button->setText(QString("%1").arg(value));
		}

		virtual
		void
		mouseMoveEvent(
				QMouseEvent *event_)
		{
			d_model->set_big_row(d_row);
			d_model->wants_update();
				// maybe get delegate to emit sizeHintChanged instead
		}

	private:

		QListView *d_list;
		MyModel *d_model;
		int d_row;
		MyPushButton *d_button;
	};


	class MyDelegate :
			public QItemDelegate
	{
	public:

		MyDelegate(
				QListView *list,
				MyModel *model,
				QObject *parent_ = NULL) :
			QItemDelegate(parent_),
			d_list(list),
			d_model(model),
			d_current_index(0)
		{  }

		virtual
		~MyDelegate()
		{  }

		virtual
		void
		paint(
				QPainter *painter,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const
		{
			QItemDelegate::paint(painter, option, index);

			if (option.state & QStyle::State_Selected)
			{
				if (index.row() != d_current_index)
				{
					d_current_index = index.row();
					d_model->wants_update();
				}
			}

#if 0
			painter->save();

			painter->setPen(QPen(Qt::NoPen));
			painter->setBrush(QBrush(Qt::blue));
			painter->drawRect(option.rect);

			painter->setPen(QPen(Qt::white));
			QVariant value = index.data(Qt::DisplayRole);
			if (value.isValid())
			{
				painter->drawText(option.rect, Qt::AlignLeft, value.toString());
			}

			painter->restore();
#endif
		}

		virtual
		QSize
		sizeHint(
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const
		{
			QSize base = QItemDelegate::sizeHint(option, index);
#if 0
			// if (d_current_index == index.row())
			if (index.row() == d_model->big_row())
			{
				base.setHeight(100);
			}
			else
			{
				base.setHeight(50);
			}
#else
			base.setHeight(100);
#endif
			return base;
		}

		virtual
		QWidget *
		createEditor(
				QWidget *parent_,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const
		{
			// return new GPlatesQtWidgets::VisualLayerWidget(parent_);
			return new MyEditWidget(d_list, d_model, index.row(), parent_);
		}

		virtual
		void
		setEditorData(
				QWidget *editor,
				const QModelIndex &index) const
		{
			MyEditWidget *my_editor = dynamic_cast<MyEditWidget *>(editor);
			if (my_editor)
			{
				my_editor->set_data(
						index.model()->data(index, Qt::DisplayRole).toInt());
			}
		}

		virtual
		void
		setModelData(
				QWidget *editor,
				QAbstractItemModel *model,
				const QModelIndex &index) const
		{
		}

	private:

		QListView *d_list;
		MyModel *d_model;
		mutable int d_current_index;
	};


	class MyListView :
			public QListView
	{
	public:

		class TimerHandler :
				public QObject
		{
		public:

			TimerHandler(
					MyListView *list,
					QBasicTimer &timer) :
				d_list(list),
				d_timer(timer),
				d_mime_data(NULL)
			{
			}

			void
			timerEvent(
					QTimerEvent *event_)
			{
				d_timer.stop();

				QDrag *drag = new QDrag(d_list);
				drag->setMimeData(d_mime_data);
				drag->exec();
			}

			void
			set_mime_data(
					QMimeData *mime_data)
			{
				d_mime_data = mime_data;
			}

		private:

			MyListView *d_list;
			QBasicTimer &d_timer;
			QMimeData *d_mime_data;
		};

		MyListView(
				QWidget *parent_ = NULL) :
			QListView(parent_),
			d_timer_handler(this, d_timer)
		{
		}

		virtual
		void
		dragEnterEvent(
				QDragEnterEvent *event_)
		{
			QListView::dragEnterEvent(event_);

			// Check that we're being dropped on from a target in the same application.
			// source() returns NULL if it's being dragged from another application,
			// including the case where that other application is another instance of GPlates.
			if (!event_->source())
			{
				event_->ignore();
			}

			// qDebug() << this->indexAt(event_->pos()).row();
		}

		virtual
		void
		dropEvent(
				QDropEvent *event_)
		{
			QListView::dropEvent(event_);

#if 0
			MyModel *m = dynamic_cast<MyModel *>(model());
			d_timer_handler.set_model(m);
			d_timer.start(0, &d_timer_handler);
#endif
		}

		void
		deferred_drag(
				QMimeData *mime_data)
		{
			d_timer_handler.set_mime_data(mime_data);
			d_timer.start(0, &d_timer_handler);
		}

	protected slots:

		virtual
		void
		rowsInserted(
				const QModelIndex &parent_,
				int start,
				int end)
		{
			for (int i = start; i != end + 1; ++i)
			{
				openPersistentEditor(model()->index(i, 0));
			}
		}

	private:

		QBasicTimer d_timer;
		TimerHandler d_timer_handler;
	};

	void
	MyPushButton::mousePressEvent(
			QMouseEvent *event_)
	{
		// QDrag *drag = new QDrag(this);

		// Store the row index as a string in the mime data.
		QMimeData *mime_data = new QMimeData();
		QByteArray encoded_data;
		QDataStream stream(&encoded_data, QIODevice::WriteOnly);
		stream << QString("%1").arg(d_row);
		mime_data->setData(VISUAL_LAYERS_MIME_TYPE, encoded_data);

		// drag->setMimeData(mime_data);
		// drag->exec();
		MyListView *l = dynamic_cast<MyListView *>(d_list);
		l->deferred_drag(mime_data);
#if 0
		QDialog *dialog = new QDialog(this);
		GPlatesQtWidgets::VisualLayerWidget *blah = new GPlatesQtWidgets::VisualLayerWidget(dialog);
		GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(blah, dialog);
		dialog->show();
#endif
	}
}


GPlatesQtWidgets::VisualLayersWidget::VisualLayersWidget(
		GPlatesPresentation::VisualLayers &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QString &open_file_path,
		ReadErrorAccumulationDialog *read_errors_dialog,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers)
{
	setupUi(this);

#if 0
	QListView *list = new MyListView(this);
	MyModel *model = new MyModel(list);

	list->setModel(model);
	// list->setEditTriggers(QAbstractItemView::AllEditTriggers);
	list->setDragEnabled(true);
	list->setAcceptDrops(true);
	list->setDropIndicatorShown(false);
	// list->setDragDropMode(QAbstractItemView::InternalMove);
	list->setDragDropMode(QAbstractItemView::DragDrop);
	list->setItemDelegate(new MyDelegate(list, model, this));
	// list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	// list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	list->setSelectionMode(QAbstractItemView::NoSelection);

	// list->openPersistentEditor(model->index(0, 0));
	for (int i = 0; i < 5; ++i)
	{
		list->openPersistentEditor(model->index(i, 0));
	}
#else
	QListView *list = new VisualLayersListView(
			d_visual_layers,
			application_state,
			view_state,
			open_file_path,
			read_errors_dialog,
			this);
	add_new_layer_frame->hide(); // not implemented yet.
#endif

	QtWidgetUtils::add_widget_to_placeholder(list, layers_list_placeholder_widget);
	list->setFocus();

	// Hide things for now...
	control_widget->hide();
}

