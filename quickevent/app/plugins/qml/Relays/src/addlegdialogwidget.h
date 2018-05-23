#ifndef ADDLEGDIALOGWIDGET_H
#define ADDLEGDIALOGWIDGET_H

#include <qf/qmlwidgets/framework/dialogwidget.h>
#include <qf/core/utils.h>

namespace Ui {
class AddLegDialogWidget;
}

class AddLegDialogWidget : public qf::qmlwidgets::framework::DialogWidget
{
	Q_OBJECT

	using Super = qf::qmlwidgets::framework::DialogWidget;

	QF_FIELD_IMPL2(int, r, R, elayId, 0)
	QF_FIELD_IMPL2(int, c, C, lassId, 0)

public:
	explicit AddLegDialogWidget(QWidget *parent = 0);
	~AddLegDialogWidget();

	Q_SIGNAL void legAdded();
private:
	void onFilterTextChanged();
	void onRegistrationSelected();
private:
	Ui::AddLegDialogWidget *ui;
};

#endif // ADDLEGDIALOGWIDGET_H
