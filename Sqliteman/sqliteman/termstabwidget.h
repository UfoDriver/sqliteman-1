/*
For general Sqliteman copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Sqliteman
for which a new license (GPL+exception) is in place.
*/

#ifndef TERMSTABWIDGET_H
#define TERMSTABWIDGET_H

#include "ui_termstabwidget.h"

/*!
 * @brief A widget for manging a list of test terms
 * \author Igor Khanin
 * \author Petr Vanek <petr@scribus.info>
 * \author Richard Parkins extracted from QueryEditorDialog
 *         also used in FindDialog
 */
class TermsTabWidget : public QWidget, public Ui::TermsTabWidget
{
	Q_OBJECT

	public:
		/*!
		 * @brief Creates the terms tab.
		 * @param parent The parent widget.
		 */
		TermsTabWidget(QWidget * parent = 0);

		QStringList m_columnList;

	private:
		void paintEvent(QPaintEvent * event);

	private slots:
		void moreTerms();
		void lessTerms();
		void relationsIndexChanged(const QString &);
};

#endif //TERMSTABWIDGET_H
