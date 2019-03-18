#ifndef VISUCONTROLLER_H
#define VISUCONTROLLER_H

#include "svgscene/saxhandler.h"

#include <QGraphicsItem>

namespace shv { namespace chainpack { class RpcValue; }}

class VisuController : public QObject, public QGraphicsRectItem
{
	Q_OBJECT

	using Super = QGraphicsRectItem;
public:
	static const QString ATTR_CHILD_ID;
	static const QString ATTR_SHV_PATH;
	static const QString ATTR_SHV_TYPE;
public:
	VisuController(QGraphicsItem *parent = nullptr);

	virtual void onShvDeviceValueChanged(const std::string &shvPath, const shv::chainpack::RpcValue &val) = 0;
	virtual void updateValue();
	virtual void reload();

	const std::string& shvPath();
	virtual void init();
protected:
	template<typename T>
	T findChild(const QString &attr_name = QString(), const QString &attr_value = QString()) const
	{
		return findChild<T>(this, attr_name, attr_value);
	}
	template<typename T>
	static T findChild(const QGraphicsItem *parent_it, const QString &attr_name, const QString &attr_value)
	{
		for(QGraphicsItem *it : parent_it->childItems()) {
			if(T tit = dynamic_cast<T>(it)) {
				if(attr_name.isEmpty()) {
					return tit;
				}
				else {
					svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(tit->data(svgscene::XmlAttributesKey));
					if(attrs.value(attr_name) == attr_value)
						return tit;
				}
			}
			T tit = findChild<T>(it, attr_name, attr_value);
			if(tit)
				return tit;
		}
		return nullptr;
	}
protected:
	std::string m_shvPath;
};

class StatusVisuController : public VisuController
{
	Q_OBJECT
	using Super = VisuController;
public:
	StatusVisuController(QGraphicsItem *parent = nullptr);

	void init() override;
protected:
	unsigned m_bitMask = 0;
	QColor m_colorOn;
	QColor m_colorOff;
};

class StatusBitVisuController : public StatusVisuController
{
	Q_OBJECT
	using Super = StatusVisuController;
public:
	StatusBitVisuController(QGraphicsItem *parent = nullptr);

	void onShvDeviceValueChanged(const std::string &shvPath, const shv::chainpack::RpcValue &val) override;
};

class SwitchVisuController : public StatusVisuController
{
	Q_OBJECT
	using Super = StatusVisuController;
public:
	SwitchVisuController(QGraphicsItem *parent = nullptr);

	void onShvDeviceValueChanged(const std::string &shvPath, const shv::chainpack::RpcValue &val) override;

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
protected:
	void init() override;
private:
	QTransform m_originalTrasformation;
};

class MultimeterVisuController : public VisuController
{
	Q_OBJECT
	using Super = VisuController;
public:
	MultimeterVisuController(QGraphicsItem *parent = nullptr);

	void onShvDeviceValueChanged(const std::string &shvPath, const shv::chainpack::RpcValue &val) override;
protected:
	void init() override;
protected:
	bool m_lazyInitDone = false;
	QString m_suffix;
};

#endif // VISUCONTROLLER_H
