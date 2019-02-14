#ifndef SVGHANDLER_H
#define SVGHANDLER_H

#include <src/svgscene/saxhandler.h>

class SvgHandler : public svgscene::SaxHandler
{
	using Super = svgscene::SaxHandler;
public:
	SvgHandler(QGraphicsScene *scene);

protected:
	QGraphicsItem *createGroupItem(const SvgElement &el) override;
};

#endif // SVGHANDLER_H
