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

	virtual void onShvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val) = 0;

	const std::string& shvPath();
protected:
	template<typename T>
	T findChild(const QString &attr_name, const QString &attr_value) const
	{
		return findChild<T>(this, attr_name, attr_value);
	}
	template<typename T>
	static T findChild(const QGraphicsItem *parent_it, const QString &attr_name, const QString &attr_value)
	{
		for(QGraphicsItem *it : parent_it->childItems()) {
			if(T tit = dynamic_cast<T>(it)) {
				svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(tit->data(svgscene::XmlAttributesKey));
				if(attrs.value(attr_name) == attr_value)
					return tit;
			}
			T tit = findChild<T>(it, attr_name, attr_value);
			if(tit)
				return tit;
		}
		return nullptr;
	}
protected:
	std::string m_shvPath;
	bool m_shvPathLoaded = false;
};

class SwitchVisuController : public VisuController
{
	Q_OBJECT
	using Super = VisuController;
public:
	SwitchVisuController(QGraphicsItem *parent = nullptr);

	void onShvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val) override;

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
	//QString statusToColor(Jn50ViewApp::SwitchStatus status);
private:
	QTransform m_originalTrasformation;
	qreal m_originalRotation;
	bool m_originalTrasformationLoaded = false;
};


class MultimeterVisuController : public VisuController
{
	Q_OBJECT
	using Super = VisuController;
public:
	MultimeterVisuController(QGraphicsItem *parent = nullptr);

	void onShvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val) override;
};

#endif // VISUCONTROLLER_H
