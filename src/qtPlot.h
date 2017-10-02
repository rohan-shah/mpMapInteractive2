#include <QtWidgets/QMainWindow>
#include "ZoomGraphicsView.h"
#include <QLabel>
#include <QGraphicsScene>
#include <QPushButton>
#include <QGraphicsPixmapItem>
#include <QLineEdit>
#include <vector>
#include <QMutex>
#include <set>
#include "imageTileWithAux.h"
#include <functional>
#include "qtPlotData.h"
#include <QShortcut>
class QProgressBar;
namespace mpMapInteractive
{
	enum plotMode
	{
		Groups, Interval, Single
	};
	struct singleMode;
	struct intervalMode;
	struct groupsMode;
	struct plotModeObject;
	class qtPlot : public QMainWindow
	{
		Q_OBJECT
	public:
		~qtPlot();
		qtPlot(QSharedPointer<qtPlotData> inputData);
		QGraphicsView& getGraphicsView();
		void signalMouseMove();
		QGraphicsScene& getGraphicsScene();
		bool attemptBeginComputation();
		void endComputation();
		qtPlotData& getData();
		void dataChanged();
		QPushButton* addCancelButton();
		void deleteCancelButton(QPushButton*);
		QProgressBar* addProgressBar();
		void deleteProgressBar(QProgressBar*);
		std::set<imageTileWithAux, imageTileComparer> imageTiles;
	protected:
		void closeEvent(QCloseEvent* event);
		void keyPressEvent(QKeyEvent* event);
		bool eventFilter(QObject *obj, QEvent *event);
	public slots:
		void graphicsLeaveEvent(QEvent*);
		void modeChanged(const QString&);
		void changeAuxiliaryState(int);
	private:
		void setBoundingBox(int nMarkers);
		void doImputation(int group);
		void graphicsMouseMove(QPointF scenePos);
		
		plotMode currentMode;
		
		QFrame* createMode();
		QWidget* addLeftSidebar();

		void initialiseImageData(int nMarkers);
		void addStatusBar();
		void updateImageFromRaw();
		//The previously highlighted horizontal and vertical group
		QGraphicsRectItem* singleHighlight;
		//Some functions make structural changes to this. So to update the data set we just switch out this data object for a new one, which is an atomic operation. And functions that depend on the 
		//data make a copy and use that. 
		QSharedPointer<qtPlotData> data;

		//needed as the stride for the two double arrays below. 
		int nOriginalMarkers;
		std::vector<double> levels;
		bool isFullScreen;
		ZoomGraphicsView* graphicsView;
		QLabel* statusLabel;
		QGraphicsScene* graphicsScene;
		QStatusBar* statusBar;

		QLabel* positionXLabel, *positionYLabel;
		QLabel* groupXLabel, *groupYLabel;
		QLabel* markerXLabel, *markerYLabel;
		//we need a critical section around changes to the image state (basically anything that calls applyPermutation), because: The Ordering code calls into R, which will periodically
		//break out and process events, which will keep the window responsive. Which means that (for example) if you choose to order a large chunk of the image, and hit Ctrl + O again while it's doing this, it will 
		//call the ordering code again internally (from inside the first ordering computation). This is bade. 
		//We only want to be doing one bit of computation at a time. 
		QMutex computationMutex;
		
		QGraphicsRectItem* transparency;

		QSharedPointer<plotModeObject> currentModeObject;
		QSharedPointer<groupsMode> groupsModeObject;
		QSharedPointer<intervalMode> intervalModeObject;
		QSharedPointer<singleMode> singleModeObject;
		QShortcut* cancelShortcut;
		
		QVector<QRgb> colours;
		bool showAux;
	};
}
