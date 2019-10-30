#ifndef VISUCONTROLLER_H
#define VISUCONTROLLER_H

#include <shv/visu/svgscene/types.h>
#include <shv/visu/svgscene/saxhandler.h>
#include <shv/visu/svgscene/visucontroller.h>

#include <QGraphicsItem>

namespace shv { namespace chainpack { class RpcValue; }}
namespace svgscene = shv::visu::svgscene;

class VisuController : public svgscene::VisuController
{
	Q_OBJECT

	using Super = svgscene::VisuController;
public:
	static const QString ATTR_CHILD_ID;
	static const QString ATTR_SHV_PATH;
	static const QString ATTR_SHV_TYPE;
public:
	VisuController(QGraphicsItem *parent = nullptr);

	virtual void onOpcValueChanged(const std::string &opcPath, const QVariant &val) = 0;
	virtual void updateValue();
	virtual void reload();

	const std::string& opcPath();
	virtual void init();
protected:
	template<typename T>
	T findChildGraphicsItem(const QString &attr_name = QString(), const QString &attr_value = QString()) const
	{
		return findChildGraphicsItem<T>(m_graphicsItem, attr_name, attr_value);
	}
	template<typename T>
	static T findChildGraphicsItem(const QGraphicsItem *parent_it, const QString &attr_name, const QString &attr_value)
	{
		for(QGraphicsItem *it : parent_it->childItems()) {
			if(T tit = dynamic_cast<T>(it)) {
				if(attr_name.isEmpty()) {
					return tit;
				}
				else {
					svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(tit->data(svgscene::Types::DataKey::XmlAttributes));
					if(attrs.value(attr_name) == attr_value)
						return tit;
				}
			}
			T tit = findChildGraphicsItem<T>(it, attr_name, attr_value);
			if(tit)
				return tit;
		}
		return nullptr;
	}
protected:
	std::string m_opcPath;
};

class RouteVisuController : public VisuController
{
	Q_OBJECT
	using Super = VisuController;
public:
	RouteVisuController(QGraphicsItem *parent = nullptr);

	void init() override;
	void onOpcValueChanged(const std::string &opcPath, const QVariant &val) override;
protected:
	QColor m_colorOn;
	QColor m_colorOff;
};


class PushButtonVisuController : public VisuController
{
	Q_OBJECT
	using Super = VisuController;
public:
	PushButtonVisuController(QGraphicsItem *parent = nullptr);

	void onOpcValueChanged(const std::string &opcPath, const QVariant &val) override;
protected:
	void init() override;
private:
};

#endif // VISUCONTROLLER_H
