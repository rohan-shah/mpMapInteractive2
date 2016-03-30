#include "plotModeObject.h"
class QFrame;
class QGraphicsRectItem;
namespace mpMapInteractive
{
	struct singleMode : public plotModeObject
	{
	public:
		singleMode(qtPlot* plotObject, qtPlotData& data, unsigned char** imputedRawData, unsigned char* rawImageData, plotModeObject::imputeFunctionType imputeFunction, std::vector<double>& levels)
			: plotModeObject(plotObject, data, imputedRawData, rawImageData, imputeFunction, levels), position(-1), highlight(NULL)
		{}
		virtual void mouseMove(int x, int y);
		virtual void leaveFocus();
		virtual void leaveMode();
		virtual void enterMode();
		virtual void mousePressed(int x, int y, Qt::MouseButtons pressed);
		virtual void keyPressEvent(QKeyEvent* event);
		QFrame* frame;
	private:
		void constructFrame();
		void deleteHighlight();
		void addHighlight();
		int position;
		QGraphicsRectItem* highlight;
	};
}
