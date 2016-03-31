#ifndef GROUPS_MODE_HEADER_GUARD
#define GROUPS_MODE_HEADER_GUARD
#include "plotModeObject.h"
class QLabel;
class QLineEdit;
class QGraphicsRectItem;
class QFrame;
namespace mpMapInteractive
{
	struct groupsMode : public QObject, public plotModeObject
	{
		Q_OBJECT
	public slots:
		void group1ReturnPressed();
		void group2ReturnPressed();
	public:
		groupsMode(qtPlot* plotObject, qtPlotData& data, unsigned char** imputedRawData, unsigned char* rawImageData, plotModeObject::imputeFunctionType imputeFunction, std::vector<double>& levels)
			: plotModeObject(plotObject, data, imputedRawData, rawImageData, imputeFunction, levels), horizontalGroup(-1), verticalGroup(-1), horizontalHighlight(NULL), verticalHighlight(NULL)
		{
			constructFrame();
		}
		~groupsMode();
		virtual void mouseMove(int x, int y);
		virtual void leaveMode();
		virtual void enterMode();
		virtual void keyPressEvent(QKeyEvent* event);
		virtual void mousePressed(int x, int y, Qt::MouseButtons pressed);
		virtual void leaveFocus();
		QFrame* frame;
	private:
		void joinGroups(int x, int y);
		void renewHighlighting(int x, int y);
		void deleteHighlighting();
		void constructFrame();
		QLabel* joinGroupsLabel;
		QLineEdit* group1Edit, *group2Edit, *orderAllExcept;
		int horizontalGroup;
		int verticalGroup;
		QGraphicsRectItem* horizontalHighlight, *verticalHighlight;
	};
}
#endif
