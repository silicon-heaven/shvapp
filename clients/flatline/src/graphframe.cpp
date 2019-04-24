#include "graphframe.h"

#include <shv/chainpack/rpcvalue.h>

#include <QPainter>
#include <QTimer>
#include <QtGlobal>

GraphFrame::GraphFrame(QWidget *parent)
	: Super(parent)
{
	m_updateTimer = new QTimer(this);
	connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&GraphFrame::update));
	m_updateTimer->start(50);
	
	m_xTimer = new QTimer(this);
	connect(m_xTimer, &QTimer::timeout, this, &GraphFrame::startXScan);
	m_xTimer->start(50);
}

void GraphFrame::addSample(DataSample &&sample)
{
	uint16_t tm = static_cast<uint16_t>(sample.time);
	if(m_currentSampleIndex == 0) {
		sample.time = 0;
	}
	else {
		uint16_t tdiff = tm - m_prevTime;
		const DataSample &last = m_samples.at(m_currentSampleIndex - 1);
		sample.time = last.time + tdiff;
	}
	m_prevTime = tm;
	if(m_currentSampleIndex < m_samples.size())
		m_samples[m_currentSampleIndex] = std::move(sample);
	else
		m_samples.push_back(std::move(sample));
	m_currentSampleIndex++;
}

unsigned GraphFrame::xRange() const
{
	return static_cast<unsigned>(m_xTimer->interval());
}

void GraphFrame::setXRange(int msec)
{
	if(msec < 10)
		msec = 10;
	m_xTimer->setInterval(msec);
}

void GraphFrame::startXScan()
{
	m_currentSampleIndex = 0;
}

void GraphFrame::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	QPainter painter(this);
	//painter.setRenderHint(QPainter::Antialiasing);

	QSize canvas_size = geometry().size();

	auto to_y = [canvas_size, this](int value) -> double
	{
		double y = (value - m_yZeroOffset);
		y = y * (canvas_size.height() / 2) / m_yRange;
		y = canvas_size.height() / 2 - y;
		return y;
	};

	painter.save();
	painter.fillRect(QRect(QPoint(), canvas_size), Qt::black);

	{
		double y = to_y(0);
		QPainterPath path;
		path.moveTo(0, y);
		path.lineTo(canvas_size.width(), y);
		painter.setPen(Qt::darkGreen);
		painter.drawPath(path);
	}

	//int sample_cnt = m_samples.size();
	if(m_yRange > 0) {
		for (unsigned i = 0; i < DataSample::CHANNEL_CNT; ++i) {
			{
				painter.setPen(DataSample::channelColor(i));
				unsigned ix;
				unsigned last_sample_time = 0;
				{
					QPainterPath path;
					for (ix = 0; ix < m_currentSampleIndex; ++ix) {
						const DataSample &sample = m_samples.at(ix);
						last_sample_time = sample.time;
						double x = sample.time;
						x = x * canvas_size.width() / xRange();
						double y = to_y(sample.values[i]);
						if(ix == 0)
							path.moveTo(x, y);
						else
							path.lineTo(x, y);
					}
					painter.drawPath(path);
				}
				/*
				for (; ix < m_samples.size(); ++ix) {
					const Sample &sample = m_samples.at(ix);
					if(sample.time < last_sample_time)
						break;
				}
				{
					QPainterPath path;
					unsigned ix0 = ix;
					for (; ix < m_samples.size(); ++ix) {
						const Sample &sample = m_samples.at(ix);
						if(sample.time > xRange())
							break;
						double x = sample.time;
						x = x * canvas_size.width() / xRange();
						double y = to_y(sample.values[i]);
						if(ix == ix0)
							path.moveTo(x, y);
						else
							path.lineTo(x, y);
					}
					painter.drawPath(path);
				}
				*/
			}
		}
	}
	painter.restore();
}

