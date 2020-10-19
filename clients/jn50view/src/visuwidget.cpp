#include "visuwidget.h"
#include "jn50viewapp.h"
#include "settingsdialog.h"
#include "svghandler.h"
#include "visucontroller.h"
#include "svgscene/saxhandler.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/clientconnection.h>

#include <QDomDocument>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QGraphicsScene>
#include <QXmlStreamReader>

VisuWidget::VisuWidget(QWidget *parent)
	: Super(parent)
{
	m_scaleToFitTimer = new QTimer(this);
	m_scaleToFitTimer->setSingleShot(true);
	m_scaleToFitTimer->setInterval(200);
	connect(m_scaleToFitTimer, &QTimer::timeout, this, &VisuWidget::zoomToFit);

	Jn50ViewApp *app = Jn50ViewApp::instance();
	connect(app, &Jn50ViewApp::shvDeviceConnectedChanged, [this](bool ) {
		for(VisuController *vc : findVisuControllers()) {
			vc->reload();
		}
	});

	m_scene = new QGraphicsScene(this);
	setScene(m_scene);

	switch(QLocale::system().language()) {
		case QLocale::Language::English: load(":/images/visu_en.svg"); break;
		case QLocale::Language::Czech: load(":/images/visu_cs.svg"); break;
		default: load(":/images/visu_en.svg");break;
	}

	for(VisuController *vc : findVisuControllers()) {
		vc->updateValue();
	}
}

VisuWidget::~VisuWidget()
{
}

bool VisuWidget::load(const QString &file_name)
{
	m_scene->clear();
	QFile file(file_name);
	if (!file.open(QIODevice::ReadOnly))
		SHV_EXCEPTION("Cannot open file " + file.fileName().toStdString() + " for reading!");
	//shvInfo() << m_xDoc.toString();
	QXmlStreamReader rd(&file);
	SvgHandler h(m_scene);
	h.load(&rd);
	for(VisuController *vc : findVisuControllers()) {
		vc->init();
	}
	zoomToFitDeferred();
	return true;
}

void VisuWidget::zoomToFitDeferred()
{
	m_scaleToFitTimer->start();
}

void VisuWidget::paintEvent(QPaintEvent *event)
{
	//QPainter p(this->viewport());
	//p.fillRect(QRect(QPoint(), geometry().size()), QColor("#2b4174"));
	Super::paintEvent(event);
}

static QList<VisuController *> findVisuControllers_helper(QGraphicsItem *parent_it)
{
	QList<VisuController *> ret;
	if(VisuController *vc = dynamic_cast<VisuController*>(parent_it))
		ret << vc;
	for(QGraphicsItem *it : parent_it->childItems()) {
		ret << findVisuControllers_helper(it);
	}
	return ret;
}

QList<VisuController *> VisuWidget::findVisuControllers()
{
	QList<VisuController *> ret;
	for(QGraphicsItem *it : m_scene->items()) {
		ret << findVisuControllers_helper(it);
	}
	return ret;
}


