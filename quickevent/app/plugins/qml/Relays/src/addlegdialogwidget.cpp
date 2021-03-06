#include "addlegdialogwidget.h"
#include "ui_addlegdialogwidget.h"

#include <Competitors/competitordocument.h>
#include <Competitors/competitorsplugin.h>

#include <qf/qmlwidgets/framework/mainwindow.h>
#include <qf/qmlwidgets/dialogs/messagebox.h>

#include <qf/core/log.h>
#include <qf/core/exception.h>
#include <qf/core/assert.h>
#include <qf/core/model/sqltablemodel.h>

static Competitors::CompetitorsPlugin* competitorsPlugin()
{
	qf::qmlwidgets::framework::MainWindow *fwk = qf::qmlwidgets::framework::MainWindow::frameWork();
	auto *plugin = qobject_cast<Competitors::CompetitorsPlugin*>(fwk->plugin("Competitors"));
	QF_ASSERT_EX(plugin != nullptr, "Bad Competitors plugin!");
	return plugin;
}

AddLegDialogWidget::AddLegDialogWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::AddLegDialogWidget)
{
	qfLogFuncFrame() << objectName();
	ui->setupUi(this);
	setPersistentSettingsId(objectName());
	ui->tblCompetitors->setPersistentSettingsId(ui->tblCompetitors->objectName());
	ui->tblRegistrations->setPersistentSettingsId(ui->tblRegistrations->objectName());

	qf::core::model::SqlTableModel *competitors_model = new qf::core::model::SqlTableModel(this);
	//competitors_model->addColumn("relays.club", tr("Club"));
	competitors_model->addColumn("relayName", tr("Name"));
	competitors_model->addColumn("runs.leg", tr("Leg"));
	competitors_model->addColumn("competitorName", tr("Name"));
	competitors_model->addColumn("registration", tr("Reg"));
	competitors_model->addColumn("licence", tr("Lic"));
	competitors_model->addColumn("competitors.siId", tr("SI"));
	qf::core::sql::QueryBuilder qb;
	qb.select2("runs", "id, relayId, leg")
			.select2("competitors", "id, registration, licence, siId")
			.select2("classes", "name")
			.select2("relays", "classId")
			.select("COALESCE(relays.club, '') || ' ' || COALESCE(relays.name, '') AS relayName")
			.select("COALESCE(lastName, '') || ' ' || COALESCE(firstName, '') AS competitorName")
			.from("runs")
			.join("runs.competitorId", "competitors.id")
			.join("runs.relayId", "relays.id")
			.join("relays.classId", "classes.id")
			.orderBy("competitorName");//.limit(10);
	competitors_model->setQueryBuilder(qb);
	ui->tblCompetitors->setTableModel(competitors_model);
	ui->tblCompetitors->setReadOnly(true);
	competitors_model->reload();

	qf::core::model::SqlTableModel *reg_model = competitorsPlugin()->registrationsModel();
	ui->tblRegistrations->setTableModel(reg_model);
	ui->tblRegistrations->setReadOnly(true);
	connect(reg_model, &qf::core::model::SqlTableModel::reloaded, [this]() {
		ui->tblRegistrations->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	});

	connect(ui->edFilter, &QLineEdit::textChanged, this, &AddLegDialogWidget::onFilterTextChanged);
	connect(ui->tblCompetitors, &qf::qmlwidgets::TableView::doubleClicked, this, &AddLegDialogWidget::onCompetitorSelected);
	connect(ui->tblRegistrations, &qf::qmlwidgets::TableView::doubleClicked, this, &AddLegDialogWidget::onRegistrationSelected);
}

AddLegDialogWidget::~AddLegDialogWidget()
{
	delete ui;
}

void AddLegDialogWidget::onFilterTextChanged()
{
	QString txt = ui->edFilter->text().trimmed();
	if(txt.length() < 3)
		return;
	ui->tblCompetitors->filterByString(txt);
	ui->tblRegistrations->filterByString(txt);
	ui->edFilter->setFocus();
}

void AddLegDialogWidget::onCompetitorSelected()
{
	qf::core::utils::TableRow row = ui->tblCompetitors->selectedRow();
	//int curr_leg = row.value("runs.leg").toInt();
	int competitor_id = row.value("competitors.id").toInt();
	int siid = row.value("competitors.siid").toInt();
	int curr_run_id = row.value("runs.id").toInt();
	int curr_relay_id = row.value("relayId").toInt();
	if(curr_relay_id > 0 && curr_relay_id != relayId()) {
		if(false == qf::qmlwidgets::dialogs::MessageBox::askYesNo(this, tr("Competitor has different relay assigned already. Move it to current one?")))
			return;
		if(row.value("relays.classId").toInt() != classId()) {
			qf::core::sql::Query q;
			q.exec("UPDATE competitors SET "
				   "classId=" + QString::number(classId())
				   + " WHERE id=" + QString::number(competitor_id), qf::core::Exception::Throw);
		}
	}
	int free_leg = findFreeLeg();
	qf::core::sql::Query q;
	if(curr_run_id == 0 || curr_relay_id == relayId()) {
		q.exec("INSERT INTO runs (competitorId, relayId, leg, siid) VALUES ("
			   + QString::number(competitor_id) + ", "
			   + QString::number(relayId()) + ", "
			   + QString::number(free_leg) + ", "
			   + QString::number(siid) + " "
			   + ") ", qf::core::Exception::Throw);
	}
	else {
		q.exec("UPDATE runs SET "
			   "relayId=" + QString::number(relayId())
			   + ", leg=" + QString::number(free_leg)
			   + ", isRunning=(1=1)" // TRUE is not accepted by SQLite
			   + " WHERE id=" + QString::number(curr_run_id), qf::core::Exception::Throw);
	}
	emit legAdded();
}

void AddLegDialogWidget::onRegistrationSelected()
{
	qf::core::utils::TableRow row = ui->tblRegistrations->selectedRow();
	Competitors::CompetitorDocument doc;
	doc.loadForInsert();
	doc.setValue("firstName", row.value("firstName"));
	doc.setValue("lastName", row.value("lastName"));
	doc.setValue("registration", row.value("registration"));
	doc.setValue("licence", row.value("licence"));
	doc.setValue("siid", row.value("siid"));
	doc.setValue("classId", classId());
	doc.save();
	int run_id = doc.lastInsertedRunsIds().value(0);
	QF_ASSERT(run_id > 0, "Bad insert", return);
	int free_leg = findFreeLeg();
	qf::core::sql::Query q;
	q.exec("UPDATE runs SET relayId=" + QString::number(relayId()) + ", leg=" + QString::number(free_leg)
		   + " WHERE id=" + QString::number(run_id), qf::core::Exception::Throw);
	emit legAdded();
}

int AddLegDialogWidget::findFreeLeg()
{
	qf::core::sql::Query q;
	q.exec("SELECT leg FROM runs WHERE leg IS NOT NULL AND relayId=" + QString::number(relayId()) + " ORDER BY leg", qf::core::Exception::Throw);
	int free_leg = 1;
	while(q.next()) {
		int leg = q.value(0).toInt();
		if(leg != free_leg) {
			break;
		}
		free_leg++;
	}
	return free_leg;
}
