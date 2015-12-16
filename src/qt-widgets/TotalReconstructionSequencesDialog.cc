/* $Id$ */

/**
 * \file 
 * Contains the definitions of member functions of class TotalReconstructionSequencesDialog.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 * Copyright (C) 2011 Geological Survey of Norway
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

#include <vector>
#include <map>
#include <QFileDialog>
#include <QHeaderView>
#include <QTableWidget>
#include <QMap>
#include <QMessageBox>
#include <QMetaType>
#include <QDebug>

#include "CreateTotalReconstructionSequenceDialog.h"
#include "EditTotalReconstructionSequenceDialog.h"
#include "TotalReconstructionSequencesDialog.h"
#include "MetadataDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructUtils.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"

#include "global/LogException.h"

#include "maths/MathsUtils.h"

#include "model/ModelUtils.h"

#include "presentation/ViewState.h"

#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlMetadata.h"

#include "utils/ReferenceCount.h"
#include "utils/UnicodeStringUtils.h"


Q_DECLARE_METATYPE( GPlatesModel::FeatureCollectionHandle * )
Q_DECLARE_METATYPE( GPlatesPropertyValues::GpmlTimeSample * )

namespace
{
    class ColumnNames
    {
    public:
            /**
            * 
            */
        enum ColumnName
        {
            COLSPAN = 0,  // The first column, when used for column-spanning text.
            ICON = 0, TIME, LATITUDE, LONGITUDE, ANGLE, COMMENT, 
            NUMCOLS,  // This should always be last.
            INVALID
        };

        ColumnNames()
        {
            d_id_vec.assign(NUMCOLS, "");
            d_name_vec.assign(NUMCOLS, "");
            add("File / Seq", "File / Seq", ICON);
            add("Time", "Time", TIME);
            add("Lat", "Latitude", LATITUDE);
            add("Lon", "Lontitude", LONGITUDE);
            add("Angle", "Angle", ANGLE);
            add("Comment", "Comment", COMMENT);
        }


        ColumnName
        get_index(
                const QString& id)
        {
            ColumnNameMap::iterator it = d_id_index_map.find(id);
            if( it != d_id_index_map.end())
            {
                return it->second;
            }
            else
            {
                qWarning() << "Invalid name: " << id;
                return INVALID;
            }
        }

        QString
        get_id(
                ColumnName idx)
        {
            if( 0 <= idx && static_cast<size_t>(idx) < d_id_vec.size())
                return d_id_vec[idx];
            else
                return "";
        }

        std::vector<QString>
        get_ids()
        {
            return d_id_vec;
        }

        QString
        get_name(
                ColumnName idx)
        {
            QString ret;
            try
            {
                ret = d_name_vec.at(idx);
            }
            catch(std::out_of_range&)
            {
                qWarning() << "The index is out of range.";
                ret = "";
            }
            return ret;
        }

    protected:
        void
        add(
                const QString& id,
                const QString& name,
                ColumnName index)
        {
            d_id_vec[index] = id;
            d_name_vec[index] = name;
            d_id_index_map[id]  = index;
        }

        typedef std::map<QString, ColumnName> ColumnNameMap;
        ColumnNameMap d_id_index_map;
        std::vector<QString> d_id_vec, d_name_vec;
    };

    ColumnNames column_names;
}

namespace UserItemTypes
{
    /**
     * A type to describe what sort of data the QTreeWidgetItem represents - file, sequence, or pole.                                                                    
     */
    enum UserItemType
    {
        FILE_ITEM_TYPE = 1000, /* 1000 is the minimum value for custom types. */
        SEQUENCE_ITEM_TYPE,
        POLE_ITEM_TYPE
    };
}


namespace
{
    class AllowAnyPlateIdFilteringPredicate:
            public GPlatesQtWidgets::PlateIdFilteringPredicate
    {
    public:
        AllowAnyPlateIdFilteringPredicate()
        {  }

        virtual
        bool
        allow_plate_id(
                GPlatesModel::integer_plate_id_type plate_id) const
        {
            return true;
        }
    };

    class AllowSinglePlateIdFilteringPredicate:
            public GPlatesQtWidgets::PlateIdFilteringPredicate
    {
    public:
        explicit
        AllowSinglePlateIdFilteringPredicate(
                GPlatesModel::integer_plate_id_type plate_id_to_allow):
            d_plate_id_to_allow(plate_id_to_allow)
        {  }

        virtual
        bool
        allow_plate_id(
                GPlatesModel::integer_plate_id_type plate_id) const
        {
            return (plate_id == d_plate_id_to_allow);
        }

    private:
        GPlatesModel::integer_plate_id_type d_plate_id_to_allow;
    };

	inline
	const GPlatesModel::PropertyName totalReconstructionPole_prop_name()
	{
		static const GPlatesModel::PropertyName prop_name =
			GPlatesModel::PropertyName::create_gpml("totalReconstructionPole");
		return prop_name;
	}
}


namespace GPlatesQtWidgets
{
    /**
     * This class contains a search index for the Total Reconstruction Sequences contained in
     * the TotalReconstructionSequenceDialog.
     *
     * This search index enables searching by plate ID and text-in-comment.
     *
     * Note that the elements in this class contain pointers to QTreeWidgetItem.  Since these
     * QTreeWidgetItem instances are managed by the QTreeWidget, we need to be wary of dangling
     * pointers:  Whenever the contents of the dialog are updated, the QTreeWidget will be
     * cleared, so all these QTreeWidgetItem instances will be deleted.  Hence, we need to
     * ensure that whenever the QTreeWidget is cleared, or QTreeWidgetItem instances are
     * created or deleted for any other reason, the elements of this class are updated
     * accordingly.
     *
     * The separation between this class and the TotalReconstructionSequencesDialog is
     * obviously suggestive of the Model/View pattern proposed by Qt:
     *  - http://doc.qt.nokia.com/4.0/model-view.html
     *  - http://doc.qt.nokia.com/4.0/model-view-programming.html
     *
     * For this reason, I investigated the possibility of incorporating the Model/View pattern
     * into these classes; namely, changing the TotalReconstructionSequencesDialog to contain a
     * QTreeView instead of a QTreeWidget to display the tree to the user, and making this
     * class derive from QAbstractItemModel like the "Simple Tree Model" example:
     *  - http://doc.trolltech.com/4.3/qtreeview.html
     *  - http://doc.trolltech.com/4.3/qtreewidget.html
     *  - http://doc.qt.nokia.com/4.0/qabstractitemmodel.html
     *  - http://doc.qt.nokia.com/4.0/itemviews-simpletreemodel.html
     *
     * I spent some time studying the Model/View pattern in general, and the Simple Tree Model
     * example in particular.  In the end, I concluded that it was not worth the effort:
     *  -# It would be about twice as much coding to implement conforming Model/View classes.
     *  -# I can't see any real need for the Model/View separation at this time -- we don't
     *     need to have multiple Views onto the same Model.
     *  -# Simple functions such as setting the cell background would be much more effort:
     *    - http://doc.trolltech.com/4.3/qtreewidgetitem.html#setBackground
     *    - http://doc.trolltech.com/4.3/qtreeview.html#drawRow
     */
    class TotalReconstructionSequencesSearchIndex
    {
    public:
        struct TotalReconstructionPole:
                public GPlatesUtils::ReferenceCount<TotalReconstructionPole>
        {
            TotalReconstructionPole(
                    const QString &comment,
                    QTreeWidgetItem *item):
                d_comment(comment),
                d_item(item)
            {  }

            void
            hide()
            {
                d_item->setHidden(true);
            }

            void
            show()
            {
                d_item->setHidden(false);
            }

            /**
             * The descriptive pole comment.
             */
            QString d_comment;
            QTreeWidgetItem *d_item;
        };

        struct TotalReconstructionSequence:
                public GPlatesUtils::ReferenceCount<TotalReconstructionSequence>
        {
            typedef std::vector<boost::intrusive_ptr<TotalReconstructionPole> >
                    pole_sequence_type;

            TotalReconstructionSequence(
                    GPlatesModel::integer_plate_id_type moving_plate_id,
                    GPlatesModel::integer_plate_id_type fixed_plate_id,
                    QTreeWidgetItem *item):
                d_moving_plate_id(moving_plate_id),
                d_fixed_plate_id(fixed_plate_id),
                d_item(item)
            {  }

            TotalReconstructionPole *
            append_new_pole(
                    const QString &comment,
                    QTreeWidgetItem *item);

            void
            apply_filter_recursively(
                    const boost::shared_ptr<PlateIdFilteringPredicate> &predicate);

            void
            show_all_recursively();

            GPlatesModel::integer_plate_id_type d_moving_plate_id;
            GPlatesModel::integer_plate_id_type d_fixed_plate_id;
            QTreeWidgetItem *d_item;
            pole_sequence_type d_poles;
        };

        struct File
        {
        public:
            typedef std::vector<boost::intrusive_ptr<TotalReconstructionSequence> >
                    sequence_sequence_type;

            File(
                    const QString &filename,
                    QTreeWidgetItem *item):
                d_filename(filename),
                d_item(item)
            {  }

            TotalReconstructionSequence *
            append_new_sequence(
                    GPlatesModel::integer_plate_id_type moving_plate_id,
                    GPlatesModel::integer_plate_id_type fixed_plate_id,
                    QTreeWidgetItem *item);

            void
            apply_filter_recursively(
                    const boost::shared_ptr<PlateIdFilteringPredicate> &predicate);

            void
            show_all_recursively();

        private:
            QString d_filename;
            QTreeWidgetItem *d_item;
            sequence_sequence_type d_sequences;
        };

        typedef std::vector<boost::shared_ptr<File> > file_sequence_type;

        File *
        append_new_file(
                const QString &filename,
                QTreeWidgetItem *item);

        void
        apply_filter(
                const boost::shared_ptr<PlateIdFilteringPredicate> &predicate);

        void
        reset_filter();

        void
        clear()
        {
            d_files.clear();
        }

    protected:

        void
        apply_filter_recursively(
                const boost::shared_ptr<PlateIdFilteringPredicate> &predicate);

        void
        show_all_recursively();

    private:
        /**
         * The predicate used to filter by plate ID.
         */
        boost::shared_ptr<PlateIdFilteringPredicate> d_filtering_predicate_ptr;
        file_sequence_type d_files;
    };

}


GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::TotalReconstructionPole *
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::TotalReconstructionSequence::append_new_pole(
        const QString &comment,
        QTreeWidgetItem *item)
{
    boost::intrusive_ptr<TotalReconstructionPole> pole(new TotalReconstructionPole(comment, item));
    d_poles.push_back(pole);
    return d_poles.back().get();
}


void
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::TotalReconstructionSequence::apply_filter_recursively(
        const boost::shared_ptr<PlateIdFilteringPredicate> &predicate)
{
    // Check whether these plate IDs are allowed by the current plate ID filtering predicate.
    if (predicate->allow_plate_id(d_fixed_plate_id) ||
            predicate->allow_plate_id(d_moving_plate_id)) {
        d_item->setHidden(false);
#if 0
        pole_sequence_type::iterator iter = d_poles.begin();
        pole_sequence_type::iterator end = d_poles.end();
        for ( ; iter != end; ++iter) {
            (*iter)->show();
        }
#endif
    } else {
        d_item->setHidden(true);
        pole_sequence_type::iterator iter = d_poles.begin();
        pole_sequence_type::iterator end = d_poles.end();
        for ( ; iter != end; ++iter) {
            (*iter)->hide();
        }
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::TotalReconstructionSequence::show_all_recursively()
{
    d_item->setHidden(false);
    pole_sequence_type::iterator iter = d_poles.begin();
    pole_sequence_type::iterator end = d_poles.end();
    for ( ; iter != end; ++iter) {
        (*iter)->show();
    }
}


GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::TotalReconstructionSequence *
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::File::append_new_sequence(
        GPlatesModel::integer_plate_id_type moving_plate_id,
        GPlatesModel::integer_plate_id_type fixed_plate_id,
        QTreeWidgetItem *item)
{
    boost::intrusive_ptr<TotalReconstructionSequence> sequence(
            new TotalReconstructionSequence(moving_plate_id, fixed_plate_id, item));
    d_sequences.push_back(sequence);
    return d_sequences.back().get();
}


void
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::File::apply_filter_recursively(
        const boost::shared_ptr<PlateIdFilteringPredicate> &predicate)
{
    sequence_sequence_type::iterator iter = d_sequences.begin();
    sequence_sequence_type::iterator end = d_sequences.end();
    for ( ; iter != end; ++iter) {
        (*iter)->apply_filter_recursively(predicate);
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::File::show_all_recursively()
{
    sequence_sequence_type::iterator iter = d_sequences.begin();
    sequence_sequence_type::iterator end = d_sequences.end();
    for ( ; iter != end; ++iter) {
        (*iter)->show_all_recursively();
    }
}


GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::File *
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::append_new_file(
        const QString &filename,
        QTreeWidgetItem *item)
{
    boost::shared_ptr<File> file(new File(filename, item));
    d_files.push_back(file);
    return d_files.back().get();
}


void
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::apply_filter(
        const boost::shared_ptr<PlateIdFilteringPredicate> &predicate)
{
    d_filtering_predicate_ptr = predicate;
    apply_filter_recursively(predicate);
}


void
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::reset_filter()
{
    boost::shared_ptr<PlateIdFilteringPredicate> predicate(
            new AllowAnyPlateIdFilteringPredicate());
    d_filtering_predicate_ptr = predicate;

    show_all_recursively();
}


void
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::apply_filter_recursively(
        const boost::shared_ptr<PlateIdFilteringPredicate> &predicate)
{
    file_sequence_type::iterator iter = d_files.begin();
    file_sequence_type::iterator end = d_files.end();
    for ( ; iter != end; ++iter) {
        (*iter)->apply_filter_recursively(predicate);
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::show_all_recursively()
{
    file_sequence_type::iterator iter = d_files.begin();
    file_sequence_type::iterator end = d_files.end();
    for ( ; iter != end; ++iter) {
        (*iter)->show_all_recursively();
    }
}


GPlatesQtWidgets::TotalReconstructionSequencesDialog::TotalReconstructionSequencesDialog(
        GPlatesAppLogic::FeatureCollectionFileState &file_state,
        GPlatesPresentation::ViewState &view_state,
        QWidget *parent_):
    GPlatesDialog(parent_, Qt::Window),
    d_file_state_ptr(&file_state),
    d_search_index_ptr(new TotalReconstructionSequencesSearchIndex()),
    d_current_item(0),
    d_current_trs_was_expanded(false),
        d_app_state(view_state.get_application_state()),
        d_create_trs_dialog_ptr(NULL),
        d_edit_trs_dialog_ptr(NULL),
        d_metadata_dlg(NULL)
{
    setupUi(this);

    QTreeWidgetItem *header_item = treewidget_seqs->headerItem();
    BOOST_FOREACH(const QString& id, column_names.get_ids())
    {
        header_item->setText(
            column_names.get_index(id), 
            QApplication::translate("TotalReconstructionSequencesDialog", id.toUtf8(), 0, QApplication::UnicodeUTF8));
        treewidget_seqs->header()->setResizeMode(column_names.get_index(id),QHeaderView::ResizeToContents);
    }

    // Resize the width of the first column slightly, to include space for indentation.
    treewidget_seqs->header()->resizeSection(ColumnNames::ICON, 82);
    // Resize the width of the longitude column slightly, since longitude values might be up to
    // 5 digits and a minus sign.
    treewidget_seqs->header()->resizeSection(ColumnNames::LONGITUDE, 70);
    
    show_metadata_button->setDisabled(true);
	disable_seq_button->setVisible(false);
	enable_seq_button->setVisible(false);
    
    make_signal_slot_connections();
}

GPlatesQtWidgets::TotalReconstructionSequencesDialog::~TotalReconstructionSequencesDialog()
{
}

namespace
{
    inline
    void
    set_cell_background_to_show_error(
            QTreeWidgetItem *item,
            int which_column)
    {
        QColor red(255, 0, 0);
        item->setBackground(which_column, QBrush(red));
    }


    inline
    void
    set_colspan_background_to_show_disabled_seq(
            QTreeWidgetItem *item)
    {
        QColor light_grey(0xd0, 0xd0, 0xd0);
        item->setBackground(ColumnNames::COLSPAN, QBrush(light_grey));
    }


    inline
    void
    set_row_background_to_show_disabled_pole(
            QTreeWidgetItem *item)
    {
        QColor light_grey(0xd0, 0xd0, 0xd0);
        for (unsigned i = ColumnNames::TIME; i < ColumnNames::NUMCOLS; i++) {
            item->setBackground(i, QBrush(light_grey));
        }
    }


    void
    fill_tree_widget_pole_time_instant(
            QTreeWidgetItem *item,
            const GPlatesPropertyValues::GeoTimeInstant &gti,
            const QLocale &locale_)
    {
        // Check that the geo-time instant of the TimeSample is valid
        // (ie, in neither the distant past nor the distant future).

        if (gti.is_real()) {
            item->setText(ColumnNames::TIME, locale_.toString(gti.value()));
        } else {
            // This is a string to display if the geo-time instant is in either
            // the distant past or distant future (which it should not be).
            // Assume that this string won't change after the first time this function
            // is called, so we can keep the QString in a static local var.
            static QString invalid_time =
                    GPlatesQtWidgets::TotalReconstructionSequencesDialog::tr(
                            "invalid time");
            item->setText(ColumnNames::TIME, invalid_time);
            set_cell_background_to_show_error(item, ColumnNames::TIME);
        }
    }


    void
    fill_tree_widget_pole_finite_rotation(
            QTreeWidgetItem *item,
            const GPlatesPropertyValues::GpmlFiniteRotation &finite_rotation,
            const QLocale &locale_)
    {
        const GPlatesMaths::FiniteRotation &fr = finite_rotation.get_finite_rotation();
        const GPlatesMaths::UnitQuaternion3D &uq = fr.unit_quat();
        if (GPlatesMaths::represents_identity_rotation(uq)) {
            // It's an identity rotation (ie, a rotation of angle == 0.0), so there's
            // no determinate axis of rotation.
            static const double zero_angle = 0.0;

            // Assume that this string won't change after the first time this function
            // is called, so we can keep the QString in a static local var.
            static QString indeterm_tr_str =
                    GPlatesQtWidgets::TotalReconstructionSequencesDialog::tr(
                            "indet");

            item->setText(ColumnNames::LATITUDE, indeterm_tr_str);
            item->setText(ColumnNames::LONGITUDE, indeterm_tr_str);
            item->setText(ColumnNames::ANGLE, locale_.toString(zero_angle));
        } else {
            // There is a well-defined axis of rotation and a non-zero angle.
            using namespace GPlatesMaths;

            UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());
            PointOnSphere euler_pole(params.axis);
            LatLonPoint llp = make_lat_lon_point(euler_pole);
            double angle = convert_rad_to_deg(params.angle).dval();

            item->setText(ColumnNames::LATITUDE, locale_.toString(llp.latitude()));
            item->setText(ColumnNames::LONGITUDE, locale_.toString(llp.longitude()));
            item->setText(ColumnNames::ANGLE, locale_.toString(angle));
        }
    }


    void
    fill_tree_widget_pole_sample_value(
            QTreeWidgetItem *item,
            const GPlatesModel::PropertyValue::non_null_ptr_to_const_type &time_sample_value,
            const QLocale &locale_)
    {
        using namespace GPlatesPropertyValues;
        
        
        const GpmlFiniteRotation *finite_rotation =
            dynamic_cast<const GpmlFiniteRotation *>(time_sample_value.get());
        if (finite_rotation) {
            // OK, so we definitely have a FiniteRotation.  Now we have to determine
            // whether it's an identity rotation or a rotation with a well-defined axis.
            fill_tree_widget_pole_finite_rotation(item, *finite_rotation, locale_);
        }
        else{
            // The value of the TimeSample was NOT a FiniteRotation as it should
            // have been.  Hence, we can only display an error message in place of
            // the rotation.
            // Assume that this string won't change after the first time this function
            // is called, so we can keep the QString in a static local var.
            static QString not_found =
                    GPlatesQtWidgets::TotalReconstructionSequencesDialog::tr(
                            "x");
            item->setText(ColumnNames::LATITUDE, not_found);
            set_cell_background_to_show_error(item, ColumnNames::LATITUDE);
            item->setText(ColumnNames::LONGITUDE, not_found);
            set_cell_background_to_show_error(item, ColumnNames::LONGITUDE);
            item->setText(ColumnNames::ANGLE, not_found);
            set_cell_background_to_show_error(item, ColumnNames::ANGLE);
        }
    }


    void
    fill_tree_widget_items_for_poles(
            QTreeWidgetItem *parent_item_for_sequence,
            const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
            GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::TotalReconstructionSequence *
                    sequence)
    {
        // Keep track of whether we find one or more non-disabled poles.
        // (If we don't find any non-disabled poles, we'll colour the parent
        // tree-widget-item grey too, just like all its poles.)
        bool found_non_disabled_pole = false;

        // Obtain the IrregularSampling that contains the TimeSamples.
		boost::optional<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_to_const_type> irreg_sampling =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlIrregularSampling>(
						feature_ref, 
						totalReconstructionPole_prop_name());
        if (!irreg_sampling)
		{
            // For some reason, we can't find an IrregularSampling.
            // This is particularly strange, because we should already have invoked
            // TotalReconstructionSequenceTimePeriodFinder, which should have obtained
            // the begin and end times from an IrregularSampling.
            // FIXME: What can we do?  Should we complain?
            return;
        }

        // We use this to express floating-point values (the TimeSample time positions)
        // in the correct format for this locale.
        QLocale locale_;

        using namespace GPlatesPropertyValues;
		// FIXME: This const cast bypasses the model revisioning system.
		GPlatesModel::RevisionedVector<GpmlTimeSample>::const_iterator iter =
                irreg_sampling.get()->time_samples().begin();
		GPlatesModel::RevisionedVector<GpmlTimeSample>::const_iterator end =
                irreg_sampling.get()->time_samples().end();
        for ( ; iter != end; ++iter) {
            // First, append a new tree-widget-item for this TimeSample.
            QTreeWidgetItem *item_for_pole = new QTreeWidgetItem(
                    parent_item_for_sequence, 
                    UserItemTypes::POLE_ITEM_TYPE);
            QVariant qv;
			// FIXME: This const cast bypasses the model revisioning system.
            qv.setValue(const_cast<GpmlTimeSample *>(iter->get().get()));
            item_for_pole->setData(0,Qt::UserRole,qv);

#if 0
            // Display an icon if the pole is disabled.
            static const QIcon icon_pole_disabled(":/gnome_dialog_error_16.png");
            if (iter->get()->is_disabled()) {
                item_for_pole->setIcon(ColumnNames::ICON, icon_pole_disabled);
            }
#endif
            // Colour the background if the pole is disabled.
            if (iter->get()->is_disabled() || irreg_sampling.get()->is_disabled()) {
                set_row_background_to_show_disabled_pole(item_for_pole);
            } else {
                // OK, we've found at least one non-disabled pole.
                found_non_disabled_pole = true;
            }

            // Now display the geo-time instant of the TimeSample.
            fill_tree_widget_pole_time_instant(item_for_pole,
                    iter->get()->valid_time()->get_time_position(),
                    locale_);
            
            // Display the pole's FiniteRotation (the expected value of the TimeSample).
            fill_tree_widget_pole_sample_value(item_for_pole,
                    iter->get()->value(),
                    locale_);

            // Display the pole comment (the TimeSample description), if present.
            if (iter->get()->description()) {
                QString comment = GPlatesUtils::make_qstring_from_icu_string(
                        iter->get()->description().get()->get_value().get());
                item_for_pole->setText(ColumnNames::COMMENT, comment);
                sequence->append_new_pole(comment, item_for_pole);
            } else {
                item_for_pole->setText(ColumnNames::COMMENT, QString());
                sequence->append_new_pole(QString(), item_for_pole);
            }
        }

        if ( ! found_non_disabled_pole || irreg_sampling.get()->is_disabled()) {
            set_colspan_background_to_show_disabled_seq(parent_item_for_sequence);
        }
    }


    void
    fill_tree_widget_items_for_features(
            QTreeWidgetItem *parent_item_for_filename,
            const GPlatesModel::FeatureCollectionHandle::weak_ref &fc,
            GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::File *file,
            GPlatesQtWidgets::tree_item_to_feature_map_type &tree_item_to_feature_map)
    {
        using namespace GPlatesFeatureVisitors;

        TotalReconstructionSequencePlateIdFinder plate_id_finder;
        TotalReconstructionSequenceTimePeriodFinder time_period_finder(false);

        GPlatesModel::FeatureCollectionHandle::iterator iter = fc->begin();
        GPlatesModel::FeatureCollectionHandle::iterator end = fc->end();
        for ( ; iter != end; ++iter) {
            using namespace GPlatesModel;

            // First, extract the plate ID and timePeriod values from the TRS.

            plate_id_finder.reset();
            plate_id_finder.visit_feature(iter);
            if (( ! plate_id_finder.fixed_ref_frame_plate_id()) ||
                    ( ! plate_id_finder.moving_ref_frame_plate_id())) {
                // We did not find either or both of the fixed plate ID or moving
                // plate ID.  Hence, we'll assume that this is not a reconstruction
                // feature.
                continue;
            }
            integer_plate_id_type fixed_plate_id = *plate_id_finder.fixed_ref_frame_plate_id();
            integer_plate_id_type moving_plate_id = *plate_id_finder.moving_ref_frame_plate_id();

            time_period_finder.reset();
            time_period_finder.visit_feature(iter);
            if (( ! time_period_finder.begin_time()) ||
                    ( ! time_period_finder.end_time())) {
                // We did not find the begin time and end time.  Hence, we'll
                // assume that this is not a valid reconstruction feature, since it
                // does not contain a valid IrregularSampling (since we couldn't
                // find at least one TimeSample).
                continue;
            }
            GPlatesPropertyValues::GeoTimeInstant begin_time = *time_period_finder.begin_time();
            GPlatesPropertyValues::GeoTimeInstant end_time = *time_period_finder.end_time();

            QLocale locale_;

            // This is a string to display if the begin-time or end-time is in either
            // the distant past or distant future (which it should not be).
            // Assume that this string won't change after the first time this function
            // is called, so we can keep the QString in a static local var.
            static QString invalid_time =
                    GPlatesQtWidgets::TotalReconstructionSequencesDialog::tr("invalid time");
            QString begin_time_as_str = invalid_time;
            if (begin_time.is_real()) {
                begin_time_as_str = locale_.toString(begin_time.value());
            }
            QString end_time_as_str = invalid_time;
            if (end_time.is_real()) {
                end_time_as_str = locale_.toString(end_time.value());
            }
            
            QString feature_descr =
                    GPlatesQtWidgets::TotalReconstructionSequencesDialog::tr(
                        "%1 rel %2\t[%3 : %4]")
                    .arg(moving_plate_id, 3, 10, QLatin1Char('0'))
                    .arg(fixed_plate_id, 3, 10, QLatin1Char('0'))
                    .arg(end_time_as_str)
                    .arg(begin_time_as_str);

            QTreeWidgetItem *item = new QTreeWidgetItem(parent_item_for_filename, UserItemTypes::SEQUENCE_ITEM_TYPE);
            item->setFirstColumnSpanned(true);
            item->setText(ColumnNames::COLSPAN, feature_descr);

            GPlatesQtWidgets::TotalReconstructionSequencesSearchIndex::TotalReconstructionSequence *seq =
                    file->append_new_sequence(
                            moving_plate_id, fixed_plate_id, item);

            // Store in the map.
            tree_item_to_feature_map.insert(std::make_pair(item,(*iter)->reference()));

            // Now print the poles in this sequence.
            fill_tree_widget_items_for_poles(item, (*iter)->reference(), seq);
        }
    }

    /**
     * A reverse look up in the tree_item_to_feature_map.
     *
     * Returns the iterator for the map element which has value @a feature_weak_ref.
     *
     * This won't be very efficient, but we don't need to use this very often - each
     * time we've finished editing a TRS in the tree. 
     */
    GPlatesQtWidgets::tree_item_to_feature_map_type::const_iterator
    reverse_lookup(
        const GPlatesQtWidgets::tree_item_to_feature_map_type &tree_item_to_feature_map,
        const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref)
    {
        GPlatesQtWidgets::tree_item_to_feature_map_type::const_iterator it =
            tree_item_to_feature_map.begin();
        for (; it != tree_item_to_feature_map.end() ; ++it)
        {
            if (it->second == feature_weak_ref)
            {
                return it;
            }
        }
        return tree_item_to_feature_map.end();
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::update()
{
    using namespace GPlatesAppLogic;

    d_tree_item_to_feature_map.clear();

    d_search_index_ptr->clear();
    treewidget_seqs->clear();

    std::vector<FeatureCollectionFileState::file_reference> loaded_files =
            d_file_state_ptr->get_loaded_files();

    std::vector<FeatureCollectionFileState::file_reference>::iterator iter = loaded_files.begin();
    std::vector<FeatureCollectionFileState::file_reference>::iterator end = loaded_files.end();
    for ( ; iter != end; ++iter) {
        GPlatesModel::FeatureCollectionHandle::weak_ref fc = iter->get_file().get_feature_collection();
        if (ReconstructUtils::has_reconstruction_features(fc)) {
            // This feature collection contains reconstruction features.
            // Add a top-level QTreeWidgetItem for the filename.
            
            QTreeWidgetItem *item = new QTreeWidgetItem(treewidget_seqs, UserItemTypes::FILE_ITEM_TYPE);
			QVariant qv;
			qv.setValue(fc.handle_ptr());
			item->setData(0,Qt::UserRole,qv);
            item->setFirstColumnSpanned(true);
            QString filename = iter->get_file().get_file_info().get_display_name(false);
            item->setText(ColumnNames::COLSPAN, filename);

            TotalReconstructionSequencesSearchIndex::File *file =
                    d_search_index_ptr->append_new_file(filename, item);
            fill_tree_widget_items_for_features(item, fc, file,d_tree_item_to_feature_map);
        }
    }

    // Sort the tree by moving plate id anytime we update. This means the tree elements may have a different
    // order from that in the corresponding rotation file. 
    treewidget_seqs->sortItems(ColumnNames::COLSPAN,Qt::AscendingOrder);
    button_Delete_Sequence->setDisabled(true);
    button_Edit_Sequence->setDisabled(true);
}


const boost::shared_ptr<GPlatesQtWidgets::PlateIdFilteringPredicate>
GPlatesQtWidgets::TotalReconstructionSequencesDialog::parse_plate_id_filtering_text() const
{
    QString text = lineedit_Filter_by_Plate_ID->text();
    if (text.isEmpty()) {
        // Since the user hasn't entered any text, we won't block any plate IDs.
        boost::shared_ptr<PlateIdFilteringPredicate> pred(
                new AllowAnyPlateIdFilteringPredicate());
        return pred;
    }
    // At this time, we don't accept sequences or ranges or anything fancy like that: just a
    // single plate ID.  Also, at this time, the QLineEdit input mask doesn't allow any text
    // other than numeric digits, so we should be able to assume that the following conversion
    // worked OK.
    bool ok;
    GPlatesModel::integer_plate_id_type plate_id = text.toULong(&ok);
    boost::shared_ptr<PlateIdFilteringPredicate> pred(
            new AllowSinglePlateIdFilteringPredicate(plate_id));
    return pred;
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::apply_filter()
{
    d_search_index_ptr->apply_filter(parse_plate_id_filtering_text());

    if (treewidget_seqs->currentItem()) {
        treewidget_seqs->scrollToItem(treewidget_seqs->currentItem(),
                QAbstractItemView::PositionAtCenter);
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::reset_filter()
{
    d_search_index_ptr->reset_filter();

    if (treewidget_seqs->currentItem()) {
        treewidget_seqs->scrollToItem(treewidget_seqs->currentItem(),
                QAbstractItemView::PositionAtCenter);
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::handle_current_item_changed(
        QTreeWidgetItem *current,
        QTreeWidgetItem *previous)
{
	disable_seq_button->setVisible(false);
	enable_seq_button->setVisible(false);
    if (current == 0)
    {
        return;
    }
    button_Edit_Sequence->setEnabled(false);
    button_Delete_Sequence->setEnabled(false);
	try
	{
		if(has_metadata(get_current_file_ref().get_feature_collection()))
		{
			show_metadata_button->setEnabled(true);
		}
		else
		{
			show_metadata_button->setEnabled(false);
		}
    
		if(d_metadata_dlg && d_metadata_dlg->isVisible())
		{
			if(show_metadata_button->isEnabled())
			{
				show_metadata();
			}
			else
			{
				d_metadata_dlg->clear_data();
				d_metadata_dlg->setVisible(false);
				QMessageBox::warning(this,
						QObject::tr("Not Support Metadata"),
						QObject::tr((QString("The feature collection does not support metadata.") + 
								QString("The metadata dialog is closed. Click OK to continue.")).toUtf8().data()),
						QMessageBox::Ok);
			}
		}
	}catch(GPlatesGlobal::LogException& e)
	{
		std::ostringstream ostr;
		e.write(ostr);
		qWarning() << ostr.str().c_str();
	}

	QTreeWidgetItem *current_item = current;
    if(current_item->type() == UserItemTypes::FILE_ITEM_TYPE)
    {
        return;
    }
	
    if (current_item->type() == UserItemTypes::POLE_ITEM_TYPE)
    {
        current_item = current_item->parent();
    }

    tree_item_to_feature_map_type::iterator iter = d_tree_item_to_feature_map.find(current_item);

    if (iter == d_tree_item_to_feature_map.end())
    {
        return;
    }	
    
    GPlatesModel::FeatureHandle::weak_ref feature_ref = iter->second;
    if (!feature_ref.is_valid())
    {
        button_Edit_Sequence->setEnabled(false);
        button_Delete_Sequence->setEnabled(false);
        return;
    }

    bool disabled_sequence = GPlatesAppLogic::TRSUtils::one_of_trs_plate_ids_is_999(feature_ref);

    button_Edit_Sequence->setEnabled(
            ((current->type() == UserItemTypes::SEQUENCE_ITEM_TYPE)||
            (current->type() == UserItemTypes::POLE_ITEM_TYPE))  && 
            !disabled_sequence);

    button_Delete_Sequence->setEnabled
        (current->type() == UserItemTypes::SEQUENCE_ITEM_TYPE);

	if (current_item->type() == UserItemTypes::SEQUENCE_ITEM_TYPE)
	{
		if(is_seq_disabled(feature_ref))
		{
			enable_seq_button->setVisible(true);
		}
		else
		{
			disable_seq_button->setVisible(true);
		}
	}
	
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::make_signal_slot_connections()
{
    // Buttons.
    connect(
            button_Apply_Filter,
            SIGNAL(clicked()),
            this,
            SLOT(apply_filter()));
    connect(
            button_Reset_Filter,
            SIGNAL(clicked()),
            this,
            SLOT(reset_filter()));
    connect(
            button_Edit_Sequence,
            SIGNAL(clicked()),
            this,
            SLOT(edit_sequence()));
    connect(
            button_New_Sequence,
            SIGNAL(clicked()),
            this,
            SLOT(create_new_sequence()));
    connect(
            button_Delete_Sequence,
            SIGNAL(clicked()),
            this,
            SLOT(delete_sequence()));
    
    connect(
            show_metadata_button,
            SIGNAL(clicked()),
            this,
            SLOT(show_metadata()));
    // Pressing Enter in a line-edit widget.
    connect(
            lineedit_Filter_by_Plate_ID,
            SIGNAL(returnPressed()),
            this,
            SLOT(apply_filter()));

    // Events from the tree-widget.
    connect(
            treewidget_seqs,
            SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
            this,
            SLOT(handle_current_item_changed(QTreeWidgetItem *, QTreeWidgetItem *)));

    // Listen for feature collection changes so that we can update the tree.
    connect(
            &(d_app_state.get_feature_collection_file_state()),
            SIGNAL(file_state_changed(GPlatesAppLogic::FeatureCollectionFileState &)),
            this,
            SLOT(handle_feature_collection_file_state_changed()));

	connect(
            &(d_app_state.get_feature_collection_file_state()),
            SIGNAL(file_reloaded(GPlatesAppLogic::FeatureCollectionFileState &)),
            this,
            SLOT(handle_file_reloaded()));

    connect(
            disable_seq_button,
            SIGNAL(clicked()),
            this,
            SLOT(disable_sequence()));

	connect(
            enable_seq_button,
            SIGNAL(clicked()),
            this,
            SLOT(enable_sequence()));
}


GPlatesFileIO::File::Reference &
GPlatesQtWidgets::TotalReconstructionSequencesDialog::get_current_file_ref()
{
    QTreeWidgetItem *current_item = treewidget_seqs->currentItem();
    
    if(!current_item)
    {
        throw GPlatesGlobal::LogException(
            GPLATES_EXCEPTION_SOURCE,
            "Invalid tree item found!");
    }
    int user_item_type = current_item->type();
    QTreeWidgetItem *file_item = NULL;
    switch(user_item_type)
    {
    case UserItemTypes::SEQUENCE_ITEM_TYPE:
        file_item = current_item->parent();
        break;
    case UserItemTypes::POLE_ITEM_TYPE:
        file_item = current_item->parent()->parent();
        break;
    case UserItemTypes::FILE_ITEM_TYPE:
        file_item = current_item;
        break;
    default:
        throw GPlatesGlobal::LogException(
            GPLATES_EXCEPTION_SOURCE,
            "Unexpected tree item found!");
    }

	GPlatesModel::FeatureCollectionHandle* fc = 
		file_item->data(0,Qt::UserRole).value<GPlatesModel::FeatureCollectionHandle*>();
	bool valid_flag;
	GPlatesFileIO::File::Reference *file_ref;
	boost::tie(valid_flag, file_ref) = get_file_ref(fc);
    if(valid_flag)
    {
        return *file_ref;
    }
    throw GPlatesGlobal::LogException(
        GPLATES_EXCEPTION_SOURCE,
        "Cannot get current file reference.");
}


boost::tuple<
		bool,
		GPlatesFileIO::File::Reference* >
GPlatesQtWidgets::TotalReconstructionSequencesDialog::get_file_ref(
		GPlatesModel::FeatureCollectionHandle *fc) const
{
	std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
		d_file_state_ptr->get_loaded_files();
	std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>::iterator 
		iter = loaded_files.begin(),
		end = loaded_files.end();
	for ( ; iter != end; ++iter) 
	{
		if(fc == iter->get_file().get_feature_collection().handle_ptr())
		{
			return boost::make_tuple(true, &iter->get_file());
		}
	}
	GPlatesFileIO::File::Reference *tmp=NULL;
	return  boost::make_tuple(false,tmp);;
}


GPlatesFileIO::PlatesRotationFileProxy*
GPlatesQtWidgets::TotalReconstructionSequencesDialog::get_rotation_file_proxy(
        GPlatesFileIO::File::Reference &file_ref)
{
    try
    {
        using namespace GPlatesFileIO;
        const boost::optional<FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> &cfg = 
            file_ref.get_file_configuration();

        if(cfg)
        {
            const FeatureCollectionFileFormat::RotationFileConfiguration *rot_file_cfg_const = 
                dynamic_cast<const FeatureCollectionFileFormat::RotationFileConfiguration*>((*cfg).get());
            FeatureCollectionFileFormat::RotationFileConfiguration* rot_file_cfg = 
                const_cast<FeatureCollectionFileFormat::RotationFileConfiguration*>(rot_file_cfg_const);
            return &rot_file_cfg->get_rotation_file_proxy();
        }
    }
    catch(GPlatesGlobal::LogException &e)
    {
        std::ostringstream ostr;
        e.write(ostr);
        qDebug() << ostr.str().c_str();
    }
    return NULL;
}


GPlatesFileIO::PlatesRotationFileProxy*
GPlatesQtWidgets::TotalReconstructionSequencesDialog::get_current_rotation_file_proxy()
{
    try
    {
		GPlatesFileIO::File::Reference &file_ref = get_current_file_ref();
		QString fn = file_ref.get_file_info().get_display_name(false);
        if(fn.endsWith(".grot"))
        {
			return get_rotation_file_proxy(file_ref);
		}
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE, 
				"The current rotaion file is not in a .grot file.");
    }
    catch(GPlatesGlobal::LogException &e)
    {
        std::ostringstream ostr;
        e.write(ostr);
        qDebug() << ostr.str().c_str();
        return NULL;
    }
}


bool
GPlatesQtWidgets::TotalReconstructionSequencesDialog::has_metadata(
		GPlatesModel::FeatureCollectionHandle::weak_ref fc)
{
	using namespace GPlatesModel;
	if(!fc)
	{
		return false;
	}
	
	std::vector<FeatureHandle::iterator> prop_vec;
	BOOST_FOREACH(FeatureHandle::non_null_ptr_type feature, *fc)
	{
		prop_vec = ModelUtils::get_top_level_properties(
				PropertyName::create_gpml(QString("metadata")),
				WeakReference<FeatureHandle>(*feature));
		if(prop_vec.size()>0)
		{
			return true;
		}
	}
	return false;
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::disable_sequence()
{
	set_seq_disabled(get_current_feature(), true);
	update();
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::enable_sequence()
{
	set_seq_disabled(get_current_feature(), false);
	update();
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::disable_enable_pole()
{
    QTreeWidgetItem *current_item = treewidget_seqs->currentItem();
    if( current_item )
    {
        if( current_item->type() == UserItemTypes::POLE_ITEM_TYPE)
        {
            GPlatesPropertyValues::GpmlTimeSample* sample = 
                current_item->data(0,Qt::UserRole).value<GPlatesPropertyValues::GpmlTimeSample*>();
            if(sample->is_disabled())
            {
                sample->set_disabled(false);
            }
            else
            {
                sample->set_disabled(true);
                set_row_background_to_show_disabled_pole(current_item);
            }
        }
    }
}

void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::show_metadata()
{
	using namespace GPlatesModel;
    if(!d_metadata_dlg)
    {
        d_metadata_dlg = new MetadataDialog(this);
        d_metadata_dlg->set_grot_proxy(get_current_rotation_file_proxy());
    }

    std::vector<boost::shared_ptr<const Metadata> > meta_data;
    QTreeWidgetItem *current_item = treewidget_seqs->currentItem();

    if(!current_item)
    {
        return;
    }
    QTreeWidgetItem *parent_item = current_item->parent();

    int user_item_type = current_item->type();
    tree_item_to_feature_map_type::iterator iter;
    tree_item_to_feature_collection_map_type::iterator iter_fc;
    
	try{
		switch(user_item_type)
		{
		case UserItemTypes::SEQUENCE_ITEM_TYPE:
			iter = d_tree_item_to_feature_map.find(current_item);
			if(iter !=  d_tree_item_to_feature_map.end())
			{
				FeatureHandle::weak_ref feature_ref = iter->second;
				if (feature_ref.is_valid())
				{
					try{
						static const  PropertyName mprs_attrs = 
							PropertyName::create_gpml(QString("mprsAttributes"));
                    
						FeatureHandle::iterator it = feature_ref->begin();
						for(;it != feature_ref->end(); it++)
						{
							if((*it)->get_property_name() == mprs_attrs)
							{
								d_metadata_dlg->set_data(it, current_item);
							}
						}
					}catch(GPlatesGlobal::LogException& e)
					{
						std::ostringstream ostr;
						e.write(ostr);
						qDebug() << ostr.str().c_str();
					}
				}
			}
			break;
		case UserItemTypes::POLE_ITEM_TYPE:
			iter = d_tree_item_to_feature_map.find(parent_item);
			if(iter !=  d_tree_item_to_feature_map.end())
			{
				FeatureHandle::weak_ref feature_ref = iter->second;
				if(feature_ref.is_valid())
				{
					d_metadata_dlg->set_data(feature_ref,current_item);
				}
			}
			d_metadata_dlg->set_data(get_current_fc_metadata());
			break;
		case UserItemTypes::FILE_ITEM_TYPE:
			d_metadata_dlg->set_data(get_current_metadata_property());
			break;
		default:
			qWarning() << "Unrecognized tree item in total reconstruction sequences dialog.";
			break;
		}
	}catch(GPlatesGlobal::LogException &ex)
	{
		std::ostringstream ostr;
		ex.write(ostr);
		qDebug() << ostr.str().c_str();
	}
    d_metadata_dlg->show();
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::edit_sequence()
{
    QTreeWidgetItem *current_item = treewidget_seqs->currentItem();
    if(!current_item)
    {
        button_Edit_Sequence->setDisabled(true);
        return;
    }

    int user_item_type = current_item->type();

    // The current item should be of type SEQUENCE_ITEM_TYPE or POLE_ITEM_TYPE
    if ((user_item_type != UserItemTypes::SEQUENCE_ITEM_TYPE) &&
        (user_item_type != UserItemTypes::POLE_ITEM_TYPE))
    {
        return;
    }

    
    if (user_item_type == UserItemTypes::POLE_ITEM_TYPE)
    {
        current_item = current_item->parent();
    }

    tree_item_to_feature_map_type::iterator iter = d_tree_item_to_feature_map.find(current_item);

    if (iter == d_tree_item_to_feature_map.end())
    {
        return;
    }

    // Save the current item
    d_current_item = current_item;

    GPlatesModel::FeatureHandle::weak_ref feature_ref = iter->second;

    if (!feature_ref.is_valid())
    {
        return;
    }

    d_edit_trs_dialog_ptr.reset(
             new EditTotalReconstructionSequenceDialog(feature_ref,*this,this));

    d_current_trs_was_expanded = current_item->isExpanded();

    // The edit TRS dialog is modal.
    d_edit_trs_dialog_ptr->exec();

}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::update_edited_feature()
{
    tree_item_to_feature_map_type::const_iterator it = 
        d_tree_item_to_feature_map.find(d_current_item);

    if (it == d_tree_item_to_feature_map.end())
    {
        return;
    }

    GPlatesModel::FeatureHandle::weak_ref trs_feature = it->second;

    update();

    it = reverse_lookup(d_tree_item_to_feature_map,trs_feature);

    if (it == d_tree_item_to_feature_map.end())
    {
        return;
    }

    treewidget_seqs->setCurrentItem(it->first);

    if (d_current_trs_was_expanded)
    {
        treewidget_seqs->expandItem(it->first);
    }

    // Store the current item so that subsequent updates will work. 
    d_current_item = it->first;


    // The plate ids might have changed; sort the tree.
    treewidget_seqs->sortItems(ColumnNames::COLSPAN,Qt::AscendingOrder);
    treewidget_seqs->scrollToItem(d_current_item);
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::create_new_sequence()
{
    d_create_trs_dialog_ptr.reset(
            new CreateTotalReconstructionSequenceDialog(*this,d_app_state,this));
    d_create_trs_dialog_ptr->init();
    if (d_create_trs_dialog_ptr->exec())
    {
        update();
        // The plate ids might have changed; sort the tree.
        // FIXME: we should do this separately per collection.
        treewidget_seqs->sortItems(ColumnNames::COLSPAN,Qt::AscendingOrder);

        boost::optional<GPlatesModel::FeatureHandle::weak_ref> new_feature = d_create_trs_dialog_ptr->created_feature();
        if (new_feature)
        {
            tree_item_to_feature_map_type::const_iterator it = 
                reverse_lookup(d_tree_item_to_feature_map,*new_feature);
            if (it != d_tree_item_to_feature_map.end())
            {
                treewidget_seqs->scrollToItem(it->first);
                treewidget_seqs->expandItem(it->first);
            }
        }
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::delete_sequence()
{
    GPlatesModel::FeatureHandle::weak_ref feature_ref = get_current_feature();

    d_current_item = NULL;

    if (feature_ref)
    {
        QString summary_string = GPlatesAppLogic::TRSUtils::build_trs_summary_string_from_trs_feature(feature_ref);

        QString message = tr("Are you sure you want to delete the total reconstruction sequence\n(") + summary_string + tr(")?");
        if (QMessageBox::question(
            this,
            tr("Delete Total Reconstruction Sequence"),
            message,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) == QMessageBox::Yes)
        {
            remove_feature_from_proxy(feature_ref);
            feature_ref->remove_from_parent();
            d_app_state.reconstruct();
            update();
        }
    }
    else
    {
        button_Delete_Sequence->setDisabled(true);
    }
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesQtWidgets::TotalReconstructionSequencesDialog::get_current_feature()
{
    QTreeWidgetItem *current_item = treewidget_seqs->currentItem();
    if(!current_item)
    {
        qWarning() << "Invalid current item.";
        return GPlatesModel::FeatureHandle::weak_ref();
    }

    int user_item_type = current_item->type();

    // The current item should be of type SEQUENCE_ITEM_TYPE or POLE_ITEM_TYPE
    if ((user_item_type != UserItemTypes::SEQUENCE_ITEM_TYPE) &&
        (user_item_type != UserItemTypes::POLE_ITEM_TYPE))
    {
        return GPlatesModel::FeatureHandle::weak_ref();
    }


    if (user_item_type == UserItemTypes::POLE_ITEM_TYPE)
    {
        current_item = current_item->parent();
    }

    tree_item_to_feature_map_type::iterator iter = d_tree_item_to_feature_map.find(current_item);

    if (iter == d_tree_item_to_feature_map.end())
    {
        return GPlatesModel::FeatureHandle::weak_ref();
    }

    return iter->second;
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::remove_feature_from_proxy(
        GPlatesModel::FeatureHandle::weak_ref feature_ref,
        bool keep_mprs_header)
{
    using namespace GPlatesFileIO;
    using namespace GPlatesFileIO;

    PlatesRotationFileProxy *proxy = get_current_rotation_file_proxy();
    if(proxy)
    {
        BOOST_FOREACH(const RotationPoleData &d, get_pole_data_from_feature(feature_ref))
        {
            proxy->delete_pole(d);
        }
        if(!keep_mprs_header)
        {
            proxy->remove_dangling_mprs_header();
        }
    }
    else
    {
        qDebug() << "Unable to get the grot rotation file proxy.";
    }		
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::handle_feature_collection_file_state_changed()
{
    // FIXME: store the state of expanded files/sequences etc so we can restore them after the update.
    update();
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::update_current_sequence(
        GPlatesModel::TopLevelProperty::non_null_ptr_type moving_plate_id,
        GPlatesModel::TopLevelProperty::non_null_ptr_type fix_plate_id,
        GPlatesModel::TopLevelProperty::non_null_ptr_type trs)
{
    using namespace GPlatesFileIO;
    using namespace GPlatesModel;
    using namespace GPlatesPropertyValues;
    FeatureHandle::weak_ref feature_ref = get_current_feature();
    if(!feature_ref)
    {
        qWarning() << "Invalid feature weak reference found in update_current_sequence()";
        return;
    }
    
    std::vector<RotationPoleData> old_data = get_pole_data_from_feature(feature_ref);
    //Step 1: update the feature in model
    GPlatesAppLogic::TRSUtils::TRSFinder trs_finder;
    trs_finder.visit_feature(feature_ref);
    if (trs_finder.can_process_trs())
    {
        **trs_finder.irregular_sampling_property_iterator() = trs;	
        **trs_finder.moving_ref_frame_property_iterator() = moving_plate_id;
        **trs_finder.fixed_ref_frame_property_iterator() = fix_plate_id;
    }

	//Step 2: update the pole data in PlatesRotationFileProxy
    std::vector<RotationPoleData> new_data = get_pole_data_from_feature(feature_ref);
    PlatesRotationFileProxy *proxy = get_current_rotation_file_proxy();
    if(proxy)
    {
        std::vector<RotationPoleData>::iterator
            iter_new = new_data.begin(), iter_new_end = new_data.end(),
            iter_old = old_data.begin(), iter_old_end = old_data.end();
        for(;(iter_new != iter_new_end) || (iter_old != iter_old_end); )
        {
            if(iter_new == iter_new_end)
            {
                proxy->delete_pole(*iter_old);//if old data have more lines, remove all the rest.
                iter_old++;
                continue;
            }
            if(iter_old == iter_old_end)
            {
                proxy->insert_pole(*iter_new);//if new data have more lines, insert all the rest.
                iter_new++;
                continue;
            }
            if(std::fabs(iter_new->time - iter_old->time) < std::numeric_limits<double>::epsilon())
            {
                if(!((*iter_old) == (*iter_new)))
                {
                    proxy->update_pole(*iter_old, *iter_new);
                }
                iter_new++; iter_old++;
            }
            else if(iter_new->time > iter_old->time)
            {
                proxy->delete_pole(*iter_old);
                iter_old++;
            }
            else
            {
                proxy->insert_pole(*iter_new);
                iter_new++;
            }
        }
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::insert_feature_to_proxy(
        GPlatesModel::FeatureHandle::weak_ref feature_ref,
        GPlatesFileIO::PlatesRotationFileProxy &proxy)
{
    if(feature_ref)
    {
        BOOST_FOREACH(const GPlatesFileIO::RotationPoleData &d, get_pole_data_from_feature(feature_ref))
        {
            proxy.insert_pole(d);
        }
    }
    else
    {
        qWarning() << "Invalid input feature weak reference.";
    }
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::insert_feature_to_proxy(
        GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
    using namespace GPlatesFileIO;
    PlatesRotationFileProxy *proxy = get_current_rotation_file_proxy();
    if(proxy)
    {
        insert_feature_to_proxy(feature_ref, *proxy);
    }
    else
    {
        qDebug() << "Unable to get the grot rotation file proxy.";
    }
}


std::vector<GPlatesFileIO::RotationPoleData>
GPlatesQtWidgets::TotalReconstructionSequencesDialog::get_pole_data_from_feature(
        GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
    using namespace GPlatesModel;
    using namespace GPlatesFeatureVisitors;
    using namespace GPlatesPropertyValues;
    using namespace GPlatesFileIO;

    std::vector<RotationPoleData> ret;
    TotalReconstructionSequencePlateIdFinder id_finder;
    id_finder.visit_feature(feature_ref);
    integer_plate_id_type 
        moving_plate_id = id_finder.moving_ref_frame_plate_id() ?  
			*id_finder.moving_ref_frame_plate_id() : 0,
        fixed_plate_id = id_finder.fixed_ref_frame_plate_id() ? 
			*id_finder.fixed_ref_frame_plate_id() : 0;

	boost::optional<GpmlIrregularSampling::non_null_ptr_to_const_type> irreg_sampling =
			get_property_value<GpmlIrregularSampling>(
					feature_ref, 
					totalReconstructionPole_prop_name());
    if (irreg_sampling)
    {
		RevisionedVector<GpmlTimeSample>::const_iterator 
            iter =	irreg_sampling.get()->time_samples().begin(),
            end =	irreg_sampling.get()->time_samples().end();
        for ( ; iter != end; ++iter) 
        {
            const GpmlFiniteRotation *time_sample_value =
                dynamic_cast<const GpmlFiniteRotation*>(iter->get()->value().get());
            if(time_sample_value)
            {
                RotationPoleData pole(
                        time_sample_value->get_finite_rotation(),
                        moving_plate_id,
                        fixed_plate_id,
                        iter->get()->valid_time()->get_time_position().value(),
						iter->get()->is_disabled());
                ret.push_back(pole);
            }
        }
    }
    return ret;
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::insert_feature_to_proxy(
        GPlatesModel::FeatureHandle::weak_ref feature_ref,
        GPlatesFileIO::File::Reference& file_ref)
{
    GPlatesFileIO::PlatesRotationFileProxy *proxy = get_rotation_file_proxy(file_ref);
    if(proxy)
    {
        insert_feature_to_proxy(feature_ref, *proxy);
    }
    else
    {
        qDebug() << "Unable to get PlatesRotationFileProxy from file reference.";
    }
}


GPlatesModel::FeatureCollectionMetadata
GPlatesQtWidgets::TotalReconstructionSequencesDialog::get_current_fc_metadata()
{
    using namespace GPlatesModel;
    FeatureCollectionMetadata ret;
    GPlatesModel::FeatureHandle::iterator iter = get_current_metadata_property();
    boost::optional<PropertyValue::non_null_ptr_type> value = 
        ModelUtils::get_property_value(**iter);
    if(value)
    {
        const GPlatesPropertyValues::GpmlMetadata* gpml_metadata =
            dynamic_cast<const GPlatesPropertyValues::GpmlMetadata*>((*value).get());
        if(gpml_metadata)
        {
            ret = gpml_metadata->get_data();
        }
    }
    return ret;
}


GPlatesModel::FeatureHandle::iterator
GPlatesQtWidgets::TotalReconstructionSequencesDialog::get_current_metadata_property()
{
    using namespace GPlatesModel;
    GPlatesModel::FeatureHandle::iterator ret;
	GPlatesFileIO::File::Reference &file_ref = get_current_file_ref();
    FeatureCollectionHandle::weak_ref fc = file_ref.get_feature_collection();
    if(fc.is_valid())
    {
        std::vector<FeatureHandle::iterator> prop_vec;
        BOOST_FOREACH(FeatureHandle::non_null_ptr_type feature, *fc)
        {
            prop_vec = ModelUtils::get_top_level_properties(
                    PropertyName::create_gpml(QString("metadata")),
                    WeakReference<FeatureHandle>(*feature));
            if(prop_vec.size()>0)
            {
                break; // We found it. break out.
            }
        }
        if(prop_vec.size()<1)
        {
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,
					("Cannot find metadata for the feature collection."));
        }
        else
        {
            ret = prop_vec[0];
            if(prop_vec.size()>1)
            {
                qWarning() << "More than one metadata found for the feature collection, only use the first one.";
            }
        }
    }
    return ret;
}


bool
GPlatesQtWidgets::TotalReconstructionSequencesDialog::is_seq_disabled(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	bool ret = false;
	if(feature_ref.is_valid())
	{
		boost::optional<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_to_const_type> irreg_sampling_const =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlIrregularSampling>(
						feature_ref,
						totalReconstructionPole_prop_name());
		if (irreg_sampling_const)
		{
			return irreg_sampling_const.get()->is_disabled();
		}
	}
	return ret;
}


void
GPlatesQtWidgets::TotalReconstructionSequencesDialog::set_seq_disabled(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		bool flag)
{
	using namespace GPlatesPropertyValues;
	using namespace GPlatesFeatureVisitors;
	using namespace GPlatesModel;
	if(!feature_ref.is_valid())
	{
		return;
	}
	//step 1: get GpmlIrregularSampling and set "disabled" in model.
	GPlatesAppLogic::TRSUtils::TRSFinder trs_finder;
	trs_finder.visit_feature(feature_ref);
	if (trs_finder.can_process_trs())
	{
		TopLevelProperty::non_null_ptr_type
			trs = (**trs_finder.irregular_sampling_property_iterator())->clone();
		boost::optional<PropertyValue::non_null_ptr_type> trs_value = 
			ModelUtils::get_property_value(*trs);
		if(trs_value)
		{
			PropertyValue *trs_value_ptr = trs_value->get();
			GpmlIrregularSampling *irreg_sampling = dynamic_cast<GpmlIrregularSampling *>(trs_value_ptr);
			if(irreg_sampling)
			{
				irreg_sampling->set_disabled(flag);
			}
		}
		(**trs_finder.irregular_sampling_property_iterator()) = trs;
	}
	
	//step 2: update the pole metadata in PlatesRotationFileProxy so that
	//the change can be saved to file.
	GPlatesFileIO::PlatesRotationFileProxy *proxy = get_current_rotation_file_proxy();
	if(proxy)
	{
		boost::optional<GpmlIrregularSampling::non_null_ptr_to_const_type> irreg_sampling_const =
				get_property_value<GpmlIrregularSampling>(
						feature_ref, 
						totalReconstructionPole_prop_name());
		if (!irreg_sampling_const)
		{
			qWarning() << "Failed to get GpmlIrregularSampling value. This is an impossible situation.";
			return;
		}
		TotalReconstructionSequencePlateIdFinder plate_id_finder;
		plate_id_finder.reset();
		plate_id_finder.visit_feature(feature_ref);
		if (( ! plate_id_finder.fixed_ref_frame_plate_id()) ||
			( ! plate_id_finder.moving_ref_frame_plate_id())) 
		{
				return;
		}
		int fixed_plate_id = static_cast<int>(*plate_id_finder.fixed_ref_frame_plate_id());
		int moving_plate_id =  static_cast<int>(*plate_id_finder.moving_ref_frame_plate_id());

		const RevisionedVector<GpmlTimeSample> &samples =
				irreg_sampling_const.get()->time_samples();
		BOOST_FOREACH(GpmlTimeSample::non_null_ptr_to_const_type sample, samples)
		{
			const GpmlFiniteRotation *trs_pole = 
				dynamic_cast<const GpmlFiniteRotation *>(sample->value().get());
			if(trs_pole)
			{
				double time = sample->valid_time()->get_time_position().value();
				proxy->update_pole_metadata(
						trs_pole->get_metadata(), 
						GPlatesFileIO::RotationPoleData(
								trs_pole->get_finite_rotation(),
								moving_plate_id, 
								fixed_plate_id,
								time));
			}
		}
	}
}





