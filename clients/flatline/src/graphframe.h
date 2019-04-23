#ifndef GRAPHFRAME_H
#define GRAPHFRAME_H

#include "datasample.h"

#include <QFrame>

#include <vector>

class QTimer;

class GraphFrame : public QFrame
{
	using Super = QFrame;
public:
	GraphFrame(QWidget *parent);

	void addSample(DataSample &&sample);

	void setYOffset(int offset) {m_yZeroOffset = offset; update();}
	int zeroOffset() const {return m_yZeroOffset;}

	unsigned xRange() const;
	void setXRange(int msec);

	void setYRange(int range) {m_yRange = range; update();}
	int yRange() const {return m_yRange;}

protected:
	void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
	void startXScan();
private:
	std::vector<DataSample> m_samples;
	//size_t m_samplesCount = 0;
	size_t m_currentSampleIndex = 0;

	uint16_t m_prevTime = 0;
	//unsigned m_timeRange = 1000;

	int m_yRange = 2048;
	int m_yZeroOffset = m_yRange;

	QTimer *m_xTimer = nullptr;
	QTimer *m_updateTimer = nullptr;
};

#endif // GRAPHFRAME_H
