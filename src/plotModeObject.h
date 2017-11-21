#ifndef QTPLOT_PLOT_MODE_OBJECT_HEADER_GUARD
#define QTPLOT_PLOT_MODE_OBJECT_HEADER_GUARD
#include <QMouseEvent>
#include <QColor>
#include "qtPlotData.h"
#include <functional>
class QKeyEvent;
namespace mpMapInteractive
{
	class qtPlot;
	struct plotModeObject
	{
	public:
		typedef bool (*imputeFunctionType)(const unsigned char* originalTheta, unsigned char* imputedTheta, std::vector<double>& thetaLevels, double* lod, double* lkhd, std::vector<int>& markers, std::function<void(unsigned long, unsigned long)> statusFunction, bool allErrors, std::vector<std::pair<int, int> >& reportedError);
		plotModeObject(qtPlot* plotObject, qtPlotData& data, unsigned char** imputedRawData, unsigned char* rawImageData, imputeFunctionType imputeFunction, std::vector<double>& levels)
			: data(data), plotObject(plotObject), highlightColour("blue"), imputedRawData(imputedRawData), rawImageData(rawImageData), imputeFunction(imputeFunction), levels(levels)
		{
			highlightColour.setAlphaF(0.3);
		}
		virtual void mouseMove(int x, int y) = 0;
		virtual void leaveMode() = 0;
		virtual void leaveFocus() = 0;
		virtual void enterMode() = 0;
		virtual void keyPressEvent(QKeyEvent* event) = 0;
		virtual void mousePressed(int x, int y, Qt::MouseButtons pressed) = 0;
		void doImputation(int group);
	protected:
		qtPlotData& data;
		qtPlot* plotObject;
		QColor highlightColour;
		unsigned char** imputedRawData;
		unsigned char* rawImageData;
		imputeFunctionType imputeFunction;
		std::vector<double>& levels;
	};
}
#endif
