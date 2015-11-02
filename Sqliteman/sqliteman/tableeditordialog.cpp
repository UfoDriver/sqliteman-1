/*
For general Sqliteman copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Sqliteman
for which a new license (GPL+exception) is in place.
*/

#include <QTableWidget>
#include <QCheckBox>
#include <QtDebug>
#include <QSettings>
#include <QLineEdit>

#include "tableeditordialog.h"
#include "utils.h"

TableEditorDialog::TableEditorDialog(QWidget * parent)//, Mode mode, const QString & tableName): QDialog(parent)
{
	ui.setupUi(this);
	
	QSettings settings("yarpen.cz", "sqliteman");
	int hh = settings.value("tableeditor/height", QVariant(600)).toInt();
	int ww = settings.value("tableeditor/width", QVariant(800)).toInt();
	resize(ww, hh);
	m_prefs = Preferences::instance();

	ui.databaseCombo->addItems(Database::getDatabases().keys());

	ui.columnTable->setColumnWidth(0, 150);
	ui.columnTable->setColumnWidth(1, 200);
	connect(ui.columnTable, SIGNAL(itemSelectionChanged()),
			this, SLOT(fieldSelected()));
	connect(ui.addButton, SIGNAL(clicked()), this, SLOT(addField()));
	connect(ui.removeButton, SIGNAL(clicked()), this, SLOT(removeField()));
}

TableEditorDialog::~TableEditorDialog()
{
	QSettings settings("yarpen.cz", "sqliteman");
    settings.setValue("tableeditor/height", QVariant(height()));
    settings.setValue("tableeditor/width", QVariant(width()));
}

void TableEditorDialog::addField(QString oldName, QString oldType,
								 int x, QString oldDefault)
{
	bool useNull = m_prefs->nullHighlight();
	QString nullText = m_prefs->nullHighlightText();

	int rc = ui.columnTable->rowCount();
	ui.columnTable->setRowCount(rc + 1);

	QLineEdit * name = new QLineEdit(this);
	name->setText(oldName);
	name->setFrame(false);
	connect(name, SIGNAL(textEdited(const QString &)),
			this, SLOT(checkChanges()));
	ui.columnTable->setCellWidget(rc, 0, name);

	QComboBox * typeBox = new QComboBox(this);
	QStringList types;
	if (useNull && !nullText.isEmpty()) { types << nullText; }
	else { types << "NULL"; }
	types << "TEXT" << "INTEGER" << "REAL" << "BLOB";
	typeBox->addItems(types);
	typeBox->setEditable(true);
	int n;
	if (oldType.isEmpty())
	{
		n = 0;
	}
	else
	{
		n = typeBox->findText(oldType, Qt::MatchFixedString
									   | Qt::MatchCaseSensitive);
		if (n <= 0)
		{
			n = typeBox->count();
			typeBox->addItem(oldType);
		}
	}
	typeBox->setCurrentIndex(n);
	connect(typeBox, SIGNAL(currentIndexChanged(int)),
			this, SLOT(checkChanges()));
	connect(typeBox, SIGNAL(editTextChanged(const QString &)),
			this, SLOT(checkChanges()));
	ui.columnTable->setCellWidget(rc, 1, typeBox);

	QComboBox * extraBox = new QComboBox(this);
	QStringList extras;
	extras << "" << "NOT NULL" << "PRIMARY KEY" << "AUTOINCREMENT";
	extraBox->addItems(extras);
	extraBox->setCurrentIndex(x);
	extraBox->setEditable(false);
	ui.columnTable->setCellWidget(rc, 2, extraBox);
	connect(extraBox, SIGNAL(currentIndexChanged(const QString &)),
			this, SLOT(checkChanges()));

	QLineEdit * defval = new QLineEdit(this);
	defval->setText(oldDefault);
	if (useNull && oldDefault.isNull())
	{
		defval->setPlaceholderText(nullText);
	}
	defval->setFrame(false);
	connect(defval, SIGNAL(textEdited(const QString &)),
			this, SLOT(checkChanges()));
	ui.columnTable->setCellWidget(rc, 3, defval);
	ui.columnTable->resizeColumnToContents(1);
	ui.columnTable->resizeColumnToContents(2);
}

QString TableEditorDialog::getFullName(QString tableOrView)
{
	return QString("CREATE %1 ").arg(tableOrView)
		   + Utils::quote(ui.databaseCombo->currentText())
		   + "."
		   + Utils::quote(ui.nameEdit->text());
}

QString TableEditorDialog::getSQLfromGUI()
{
	QString sql = getFullName("TABLE");
	bool first = true;
	QStringList primaryKeys;
	for (int i = 0; i < ui.columnTable->rowCount(); i++)
	{
		if (checkRetained(i))
		{
			if (first)
			{
				sql += " (\n";
				first = false;
			}
			else
			{
				sql += ",\n";
			}
			QLineEdit * ed =
				qobject_cast<QLineEdit *>(ui.columnTable->cellWidget(i, 0));
			QString name(ed->text());
			sql += Utils::quote(name);
			QComboBox * box =
				qobject_cast<QComboBox *>(ui.columnTable->cellWidget(i, 1));
			QString type(box->currentText());
			if (box->currentIndex())
			{
				sql += " " + Utils::quote(type);
			}
			box = qobject_cast<QComboBox *>(ui.columnTable->cellWidget(i, 2));
			QString extra(box->currentText());
			if (extra.contains("AUTOINCREMENT"))
			{
				sql += " PRIMARY KEY AUTOINCREMENT";
			}
			else if (extra.contains("PRIMARY KEY"))
			{
				primaryKeys.append(name);
			}
			if (extra.contains("NOT NULL"))
			{
				sql += " NOT NULL";
			}
			ed = qobject_cast<QLineEdit *>(ui.columnTable->cellWidget(i, 3));
			QString defval(ed->text());
			if (!defval.isEmpty())
			{
				sql += " DEFAULT " + defval;
			}
		}
	}
	if (primaryKeys.count() > 0)
	{
		sql += ",\nPRIMARY KEY ( ";
		sql += Utils::quote(primaryKeys);
		sql += " )";
	}
	sql += "\n)";
	if (ui.withoutRowid->isChecked())
	{
		sql += " WITHOUT ROWID ";
	}
	sql += ";";
	return sql;
}

void TableEditorDialog::addField()
{
	addField(QString(), QString(), 0, QString());
	checkChanges();
}

void TableEditorDialog::removeField()
{
	ui.columnTable->removeRow(ui.columnTable->currentRow());
	if (ui.columnTable->rowCount() == 0)
	{
		addField();
	}
	else
	{
		checkChanges();
	}
}

void TableEditorDialog::fieldSelected()
{
	ui.removeButton->setEnabled(ui.columnTable->selectedRanges().count() != 0);
}

bool TableEditorDialog::checkOk(QString newName)
{
	int cols = ui.columnTable->rowCount();
	int colsLeft = cols;
	bool ok = !newName.isEmpty();
	int pkCount = 0;
	bool autoSeen = false;
	for (int i = 0; i < cols; i++)
	{
		QLineEdit * edit =
			qobject_cast<QLineEdit*>(ui.columnTable->cellWidget(i, 0));
		QString cname(edit->text());
		QComboBox * types =
			qobject_cast<QComboBox*>(ui.columnTable->cellWidget(i, 1));
		QString ctype(types->currentText());
		QComboBox * extras =
			qobject_cast<QComboBox*>(ui.columnTable->cellWidget(i, 2));
		QString cextra(extras->currentText());
		if (checkColumn(i, cname, ctype, cextra))
		{
			--colsLeft;
			continue;
		}
		if (cname.isEmpty())
		{
			ok = false;
			break;
		}
		if (cextra.contains("AUTOINCREMENT"))
		{
			if (ctype.compare("INTEGER", Qt::CaseInsensitive))
			{
				ok = false;
				break;
			}
			autoSeen = true;
		}
		if (cextra.contains("PRIMARY KEY"))
		{
			++pkCount;
		}
	}
	if (   (   (ui.withoutRowid->isChecked())
			&& ((pkCount == 0) || autoSeen))
		|| (autoSeen && (pkCount > 0))
		|| (colsLeft == 0))
	{
		ok = false;
	}
	return ok;
}

void TableEditorDialog::resultAppend(QString text)
{
	ui.resultEdit->append(text);
	int lh = QFontMetrics(ui.resultEdit->currentFont()).lineSpacing();
	QTextDocument * doc = ui.resultEdit->document();
	if (doc)
	{
		int h = (int)(doc->size().height());
		if (h < lh * 2) { h = lh * 2 + lh / 2; }
		ui.resultEdit->setFixedHeight(h + lh / 2);
	}
	else
	{
		int lines = text.split("<br/>").count() + 1;
		ui.resultEdit->setFixedHeight(lh * lines);
	}
}
