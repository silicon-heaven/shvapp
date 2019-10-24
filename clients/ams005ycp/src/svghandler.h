#ifndef SVGHANDLER_H
#define SVGHANDLER_H

#include <shv/visu/svgscene/saxhandler.h>

class SvgHandler : public shv::visu::svgscene::SaxHandler
{
	using Super = shv::visu::svgscene::SaxHandler;
public:
	SvgHandler(QGraphicsScene *scene);

protected:
	QGraphicsItem *createGroupItem(const SvgElement &el) override;
	void setXmlAttributes(QGraphicsItem *git, const SvgElement &el) override;
};

#endif // SVGHANDLER_H
