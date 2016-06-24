#include "plotModeObject.h"
class QFrame;
class QGraphicsRectItem;
class QLineEdit;
class QLabel;
class QCheckBox;
class QFormLayout;
namespace mpMapInteractive
{
	struct intervalMode : public plotModeObject
	{
	public:
		intervalMode(qtPlot* plotObject, qtPlotData& data, unsigned char** imputedRawData, unsigned char* rawImageData, plotModeObject::imputeFunctionType imputeFunction, std::vector<double>& levels)
			: plotModeObject(plotObject, data, imputedRawData, rawImageData, imputeFunction, levels), highlight(NULL), start(-1), end(-1), cutStart(-1), cutEnd(-1)
		{
			constructFrame();
		}
		virtual void mouseMove(int x, int y);
		virtual void leaveMode();
		virtual void enterMode();
		virtual void keyPressEvent(QKeyEvent* event);
		virtual void mousePressed(int x, int y, Qt::MouseButtons pressed);
		virtual void leaveFocus();
		QFrame* frame;
	private:
		static void addSeperator(QFormLayout* formLayout);
		void updateChoices();
		void constructFrame();
		void deleteHighlighting();
		void addHighlighting();
		QGraphicsRectItem* highlight;
		int start, end;
		int cutStart, cutEnd;
		QLineEdit* clusterOrderGroupsEdit, *effortEdit, *maxDistEdit, *clusterOrderEffortEdit;
		QLabel* undoLabel, *orderLabel, *reverseLabel, *clusterOrderLabel, *cutLabel, *pasteLabel, *effortLabel, *clusterOrderGroupsLabel, *maxDistLabel, *clusterOrderEffortLabel;
		QCheckBox *randomStartCheckbox;
	};
}
