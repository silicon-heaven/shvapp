#include "visuwidget.h"
#include "jn50viewapp.h"
#include "settingsdialog.h"
#include "svghandler.h"
#include "svgscene/saxhandler.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>

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

	m_scene = new QGraphicsScene(this);
	setScene(m_scene);
	load(":/images/visu.svg");
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


