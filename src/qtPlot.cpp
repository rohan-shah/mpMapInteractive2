#include <QProgressBar>
#include "qtPlot.h"
#include <R_ext/Rdynload.h>
#include <sstream>
#include <QtGui>
#include "colour.h"
#include <QVector>
#include <QGraphicsView>
#include "ZoomGraphicsView.h"
#include <QStatusBar>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItem>
#include <exception>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QStandardItemModel>
#include <QApplication>
#include <stdexcept>
#include <cmath>
#include "singleMode.h"
#include "intervalMode.h"
#include "groupsMode.h"
#include <QGraphicsItemGroup>
namespace mpMapInteractive
{
	qtPlot::~qtPlot()
	{
		groupsModeObject.reset();
		singleModeObject.reset();
		intervalModeObject.clear();
		currentModeObject.clear();

		imageTiles.clear();
		delete[] imputedRawImageData;
		delete transparency;

		delete graphicsView;
		delete graphicsScene;
		delete auxillaryLabel;
		delete statusLabel;
		delete statusBar;
	}
	void qtPlot::addStatusBar()
	{
		statusBar = new QStatusBar();
		statusLabel = new QLabel();
		statusLabel->setText("");
		statusBar->addPermanentWidget(statusLabel);

		auxillaryLabel = new QLabel();
		auxillaryLabel->setText("");
		statusBar->addWidget(auxillaryLabel);
		auxillaryLabel->setTextFormat(Qt::RichText);

		setStatusBar(statusBar);
	}
	void qtPlot::initialiseImageData(int nMarkers)
	{
		originalDataToChar.resize(((unsigned long long)nMarkers * ((unsigned long long)nMarkers+1ULL))/2ULL);
		//Conversion vector from levels to colours. Filled with the NA colour. 
		std::vector<uchar> levelToChar(0xff+1, nColours);
		for(int i = 0; i < levels.size(); i++)
		{
			levelToChar[i] = (uchar)std::floor(0.5f + (nColours - 1)* levels[i] / 0.5);
		}
		//scale data from float to integer
		for(unsigned long long i = 0; i < ((unsigned long long)nMarkers*((unsigned long long)nMarkers+1ULL))/2ULL; i++)
		{
			originalDataToChar[i] = levelToChar[rawImageData[i]];
		}
	}
	QFrame* qtPlot::createMode()
	{
		QFrame* comboContainer = new QFrame;
		QComboBox* comboMode = new QComboBox;
		comboMode->addItem("Groups");
		if(data->singleGroup())
		{
			comboMode->addItem("Single marker");
			comboMode->addItem("Interval");
		}
		QObject::connect(comboMode, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(modeChanged(const QString&)));

		QFormLayout* modeLayout = new QFormLayout;
		modeLayout->addRow("Mode", comboMode);
		comboContainer->setLayout(modeLayout);
		return comboContainer;
	}
	void qtPlot::modeChanged(const QString& mode)
	{
		currentModeObject->leaveMode();
		if(mode == "Groups")
		{
			currentMode = Groups;
			currentModeObject = groupsModeObject;
		}
		else if(mode == "Interval")
		{
			currentMode = Interval;
			currentModeObject = intervalModeObject;
		}
		else
		{
			currentMode = Single;
			currentModeObject = singleModeObject;
		}
		currentModeObject->enterMode();
	}
	QWidget* qtPlot::addLeftSidebar()
	{
		QWidget* leftSidebar = new QWidget;
		QVBoxLayout* sidebarLayout = new QVBoxLayout;
		QFrame* modeWidget = createMode();

		sidebarLayout->setAlignment(Qt::AlignTop);
		
		sidebarLayout->addWidget(modeWidget, 0, Qt::AlignTop);
		sidebarLayout->addSpacing(1);
		sidebarLayout->addWidget(groupsModeObject->frame, 1, Qt::AlignTop);
		sidebarLayout->addWidget(intervalModeObject->frame, 1, Qt::AlignTop);
		sidebarLayout->addWidget(singleModeObject->frame, 1, Qt::AlignTop);
		
		leftSidebar->setLayout(sidebarLayout);
		leftSidebar->setMinimumWidth(400);
		return leftSidebar;
	}
	void qtPlot::setBoundingBox(int nMarkers)
	{
		//leave some space around the outside, when zooming. 
		QRectF bounding;
		bounding.setX(0 - nMarkers/20.0);
		bounding.setY(0 - nMarkers/20.0);
		bounding.setWidth(nMarkers + nMarkers/10.0);
		bounding.setHeight(nMarkers + nMarkers/10.0);
		graphicsView->setSceneRect(bounding);
	}
	qtPlot::qtPlot(unsigned char* rawImageData, std::vector<double>& levels, const std::vector<int>& originalGroups, const std::vector<std::string>& originalMarkerNames, double* auxData, int auxRows, unsigned char* imputedRawImageData, imputeFunctionType imputeFunction)
		:currentMode(Groups), data(new qtPlotData(originalGroups, originalMarkerNames)), nOriginalMarkers((int)originalGroups.size()), rawImageData(rawImageData), imputedRawImageData(imputedRawImageData), levels(levels), isFullScreen(false), auxData(auxData), auxRows(auxRows), computationMutex(QMutex::NonRecursive), transparency(NULL), imputeFunction(imputeFunction)
	{
		int nMarkers = (int)originalGroups.size();
		initialiseImageData(nMarkers);
		QHBoxLayout* topLayout = new QHBoxLayout();
		graphicsScene = new QGraphicsScene();	
		graphicsScene->setItemIndexMethod(QGraphicsScene::NoIndex);
		
		//Add transparency quad
		QColor whiteColour("white");
		whiteColour.setAlphaF(0.4);
		QBrush whiteBrush(whiteColour);
		transparency = graphicsScene->addRect(0, 0, nMarkers, nMarkers, QPen(Qt::NoPen), whiteBrush);
		transparency->setZValue(0);
		
		graphicsView = new ZoomGraphicsView(graphicsScene);
		updateImageFromRaw();
		graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		//mouse move events should be handled at a higher level
		graphicsScene->installEventFilter(this);
		graphicsView->viewport()->installEventFilter(this);
		
		setBoundingBox(nMarkers);

		addStatusBar();

		groupsModeObject.reset(new mpMapInteractive::groupsMode(this, *data, &this->imputedRawImageData, rawImageData, imputeFunction, levels));
		intervalModeObject.reset(new mpMapInteractive::intervalMode(this, *data, &this->imputedRawImageData, rawImageData, imputeFunction, levels));
		singleModeObject.reset(new mpMapInteractive::singleMode(this, *data, &this->imputedRawImageData, rawImageData, imputeFunction, levels));
		currentModeObject = groupsModeObject;
		QWidget* sidebarWidget = addLeftSidebar();
		
		groupsModeObject->frame->hide();
		intervalModeObject->frame->hide();
		singleModeObject->frame->hide();

		//add form layout to top level layout (same level as the graphics view)
		topLayout->addWidget(sidebarWidget, 0);
		topLayout->addWidget(graphicsView);
		topLayout->setStretchFactor(graphicsView, 1);

		//no margins needed
		QWidget* topLayoutWidget = new QWidget;
		topLayout->setContentsMargins(0,0,0,0);
		topLayoutWidget->setContentsMargins(0,0,0,0);
		topLayoutWidget->setLayout(topLayout);
		setCentralWidget(topLayoutWidget);

		currentModeObject->enterMode();

		graphicsView->setFocus();
	}
	void qtPlot::graphicsLeaveEvent(QEvent*)
	{
		statusLabel->setText(QString(""));
		auxillaryLabel->setText(QString(""));
	}
	void qtPlot::signalMouseMove()
	{
		QPointF cursorPos = graphicsView->mapToScene(graphicsView->mapFromGlobal(QCursor::pos()));
		if(graphicsView->underMouse()) graphicsMouseMove(cursorPos);
	}
	qtPlotData& qtPlot::getData()
	{
		return *data;
	}
	bool qtPlot::eventFilter(QObject* object, QEvent *event)
	{
		if(object == graphicsScene && event->type() == QEvent::GraphicsSceneMouseMove)
		{
			QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
			QPointF scenePos = mouseEvent->scenePos();
			graphicsMouseMove(scenePos);
			return true;
		}
		if(object == graphicsScene && event->type() == QEvent::GraphicsSceneMousePress)
		{
			int nMarkers = data->getMarkerCount();
			QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
			QPointF scenePos = mouseEvent->scenePos();
			qreal x_ = scenePos.x(), y_ = scenePos.y();
			int x = (int)(x_  + 0), y = (int)(y_ + 0);
			if(x < 0) x = 0;
			if(x >= nMarkers) x = nMarkers - 1;
			if(y < 0) y = 0;
			if(y >= nMarkers) y = nMarkers - 1;

			Qt::MouseButtons pressed = mouseEvent->buttons();
			currentModeObject->mousePressed(x, y, pressed);
			return true;
		}
		if(event->type() == QEvent::Leave)
		{
			statusLabel->setText(QString(""));
			currentModeObject->leaveFocus();
			return true;
		}
		return false;
	}
	void qtPlot::graphicsMouseMove(QPointF scenePos)
	{
		const double threshold = 1e-5;
		qreal x_ = scenePos.x(), y_ = scenePos.y();
		int x = (int)(x_  + 0), y = (int)(y_ + 0);
		std::stringstream ss;
		int nMarkers = data->getMarkerCount();
		if(0 <= x && x < nMarkers && 0 <= y && y < nMarkers)
		{
			const std::vector<int>& currentPermutation = data->getCurrentPermutation();
			int xMarker = currentPermutation[x];
			int yMarker = currentPermutation[y];
			if(auxData != NULL)
			{
				std::stringstream aux_ss;
				aux_ss << "Column = (";
				for(int i = 0; i < auxRows-1; i++)
				{
					double value = (auxData + xMarker * auxRows)[i];
					if(value < 1e-5)
					{
						aux_ss << "<b>" << value << "</b>, ";
					}
					else aux_ss << value << ", ";
				}
				double value = (auxData + xMarker * auxRows)[auxRows-1];
				if(value < 1e-5)
				{
					aux_ss << "<b>" << value << "</b>";
				}
				else aux_ss << value;
				aux_ss << "), Row = (";
				for(int i = 0; i < auxRows-1; i++)
				{
					double value = (auxData + yMarker * auxRows)[i];
					if(value < threshold)
					{
						aux_ss << "<b>" << value << "</b>, ";
					}
					else aux_ss << value << ", ";
				}
				value = (auxData + yMarker * auxRows)[auxRows-1];
				if(value < 1e-5)
				{
					aux_ss << "<b>" << value << "</b>";
				}
				else aux_ss << value;
				aux_ss << ")";
				auxillaryLabel->setText(QString(aux_ss.str().c_str()));
			}
			const std::vector<std::string> currentMarkerNames = data->getCurrentMarkerNames();
			const std::vector<int> currentGroups = data->getCurrentGroups();
			ss << "Markers (column = " << currentMarkerNames[x] << ", row = " << currentMarkerNames[y] << ")\t\t\t";
			ss << "Groups (column = " << currentGroups[x] << ", row = " << currentGroups[y] << ")\t\t\t";
			
		}
		ss << "position (" << x << ", " << y << ")";
		statusLabel->setText(QString(ss.str().c_str()));
		currentModeObject->mouseMove(x, y);
	}
	void qtPlot::dataChanged()
	{
		//swap out image for new one
		updateImageFromRaw();
		setBoundingBox(data->getMarkerCount());
	}
	void qtPlot::updateImageFromRaw()
	{
		const std::vector<int>& permutation = data->getCurrentPermutation();
		const std::vector<int>& groups = data->getCurrentGroups();
		int nMarkers = data->getMarkerCount();
		
		std::vector<int> uniqueGroups = groups;
		//sort
		std::sort(uniqueGroups.begin(), uniqueGroups.end());
		//discard duplicates
		uniqueGroups.erase(std::unique(uniqueGroups.begin(), uniqueGroups.end()), uniqueGroups.end());
		
		//pre-cache some data, so it doesn't need to be recomputed in a deeply nested loop
		size_t nGroups = uniqueGroups.size();
		std::vector<int> startGroups(nGroups);
		std::vector<int> endGroups(nGroups);
		std::vector<std::vector<int> > expectedIndices;
		expectedIndices.resize(nGroups);
		for(size_t i = 0; i < nGroups; i++)
		{
			int currentGroup = uniqueGroups[i];
			startGroups[i] = data->startOfGroup(currentGroup);
			endGroups[i] = data->endOfGroup(currentGroup);

			std::vector<int>& currentGroupIndices = expectedIndices[i];
			currentGroupIndices.reserve(permutation.size());
			for(size_t j = 0; j != permutation.size(); j++)
			{
				if(groups[j] == currentGroup) currentGroupIndices.push_back(permutation[j]);
			}
		}
		
		//Go through the image data and delete any groups that have changed in their row / column member indices
		for(size_t rowGroupCounter = 0; rowGroupCounter < nGroups; rowGroupCounter++)
		{
			int rowGroup = uniqueGroups[rowGroupCounter];
			for(int columnGroupCounter = 0; columnGroupCounter < nGroups; columnGroupCounter++)
			{
				int columnGroup = uniqueGroups[columnGroupCounter];
				std::set<imageTile, imageTileComparer>::const_iterator located = imageTile::find(imageTiles, rowGroup, columnGroup);
				int startOfRowGroup = startGroups[rowGroupCounter], startOfColumnGroup = startGroups[columnGroupCounter];

				std::vector<int>& expectedRowIndices = expectedIndices[rowGroupCounter];
				std::vector<int>& expectedColumnIndices = expectedIndices[columnGroupCounter];

				if(located != imageTiles.end())
				{
					if(!located->checkIndices(expectedRowIndices, expectedColumnIndices))
					{
						imageTiles.erase(located);
					}
				}
				//Add in any rows / column that are now missing
				located = imageTile::find(imageTiles, rowGroup, columnGroup);
				if(located == imageTiles.end())
				{
					imageTile newTile(&originalDataToChar, nOriginalMarkers, rowGroup, columnGroup, expectedRowIndices, expectedColumnIndices, graphicsScene);
					imageTiles.insert(std::move(newTile));
				}
				//set position
				located = imageTile::find(imageTiles, rowGroup, columnGroup);
				if(located == imageTiles.end())
				{
					throw std::runtime_error("Internal error");
				}
				QGraphicsItemGroup* currentItem = located->getItem();
				currentItem->setPos(startOfRowGroup, startOfColumnGroup);
			}
		}
		//Go through and remove unnecessary groups. Anything that doesn't match here just gets wiped
		std::set<imageTile>::iterator currentTile = imageTiles.begin();
		while(currentTile != imageTiles.end())
		{
			{
				int rowGroup = currentTile->getRowGroup(), columnGroup = currentTile->getColumnGroup();
				std::vector<int>::iterator findRowGroup = std::find(uniqueGroups.begin(), uniqueGroups.end(), rowGroup);
				std::vector<int>::iterator findColumnGroup = std::find(uniqueGroups.begin(), uniqueGroups.end(), columnGroup);
				//does the group still exist?
				if(findRowGroup == uniqueGroups.end() || findColumnGroup == uniqueGroups.end())
				{
					goto delete_tile;
				}
				int rowGroupIndexInAll = std::distance(uniqueGroups.begin(), findRowGroup);
				int columnGroupIndexInAll = std::distance(uniqueGroups.begin(), findColumnGroup);
				std::vector<int>& expectedRowIndices = expectedIndices[rowGroupIndexInAll];
				std::vector<int>& expectedColumnIndices = expectedIndices[columnGroupIndexInAll];
				//Are the row and column indices correct?
				/*{
					for(int i = 0; i != permutation.size(); i++)
					{
						if(groups[i] == rowGroup) newRowIndices.push_back(permutation[i]);
						if(groups[i] == columnGroup) newColumnIndices.push_back(permutation[i]);
					}*/
					if(!currentTile->checkIndices(expectedRowIndices, expectedColumnIndices)) goto delete_tile;
				//}
				QGraphicsItemGroup* pixMapItem = currentTile->getItem();
				if(rowGroupIndexInAll %2 == columnGroupIndexInAll %2)
				{
					pixMapItem->setZValue(1);
				}
				else
				{
					pixMapItem->setZValue(-1);
				}
				currentTile++;
				continue;
			}
delete_tile:
			currentTile = imageTiles.erase(currentTile);
			continue;

		}
		setBoundingBox(nMarkers);
		//signal redraw
		graphicsScene->update();
	}
	void qtPlot::keyPressEvent(QKeyEvent* event)
	{
		if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
		{
			Qt::KeyboardModifiers mod = event->modifiers();
			if(mod & Qt::ControlModifier)
			{
				if(isFullScreen)
				{
					isFullScreen = false;
					showNormal();
				}
				else
				{
					showFullScreen();
					isFullScreen = true;
				}
				QPointF cursorPos = graphicsView->mapToScene(graphicsView->mapFromGlobal(QCursor::pos()));
				if(graphicsView->underMouse()) graphicsMouseMove(cursorPos);
				return;
			}
		}

		currentModeObject->keyPressEvent(event);
		QMainWindow::keyPressEvent(event);
	}
	bool qtPlot::attemptBeginComputation()
	{
		if(computationMutex.tryLock())
		{
			return true;
		}
		return false;
	}
	void qtPlot::endComputation()
	{
		computationMutex.unlock();
	}
	void qtPlot::closeEvent(QCloseEvent* event)
	{
		event->accept();
	}
	QGraphicsScene& qtPlot::getGraphicsScene()
	{
		return *graphicsScene;
	}
	QGraphicsView& qtPlot::getGraphicsView()
	{
		return *graphicsView;
	}
	QProgressBar* qtPlot::addProgressBar()
	{
		QProgressBar* progress = new QProgressBar;
		statusBar->addWidget(progress);
		return progress;
	}
	void qtPlot::deleteProgressBar(QProgressBar* progress)
	{
		statusBar->removeWidget(progress);
		delete progress;
	}
}
