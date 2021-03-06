/*
For general Sqliteman copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Sqliteman
for which a new license (GPL+exception) is in place.
*/

#ifndef QUERYSTRINGMODEL_H
#define QUERYSTRINGMODEL_H

#include <QStringListModel>

/*! \brief Improved QStringListModel for items handling in paralell
layouts. See queryeditordilog.ui in Qt4 designer.
QueryStringModels are used in "swap" items views.
\author Petr Vanek <petr@scribus.info>
*/
class QueryStringModel : public QStringListModel
{
	Q_OBJECT

	public:
		QueryStringModel(QObject * parent = 0);
		//! Make it non-editable
		Qt::ItemFlags flags (const QModelIndex & index) const;
		//! remove all items from model
		void clear();
		//! append new string at the end of the model
		void append(const QString & value);
};

#endif //QUERYSTRINGMODEL_H
