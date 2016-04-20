#include "intervalMode.h"
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QFrame>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include "qtPlot.h"
#include "order.h"
namespace mpMapInteractive
{
	void intervalMode::constructFrame()
	{
		frame = new QFrame;
		QFormLayout* formLayout = new QFormLayout;
		
		QLabel* undoLabel = new QLabel(QString("Undo (Ctrl + U)"));
		//set up pallete to highlight enabled labels / shortcuts
		QPalette p = undoLabel->palette();
		p.setColor(QPalette::Active, QPalette::WindowText, QColor("blue"));
		undoLabel->setPalette(p);
		formLayout->addRow(undoLabel, new QLabel(""));

		QLabel* orderLabel = new QLabel(QString("Order (Ctrl + O)"));
		orderLabel->setPalette(p);
		formLayout->addRow(orderLabel, new QLabel(""));

		QLabel* reverseLabel = new QLabel(QString("Reverse (Ctrl + R)"));
		orderLabel->setPalette(p);
		formLayout->addRow(reverseLabel, new QLabel(""));

		frame->setLayout(formLayout);
	}
	void intervalMode::deleteHighlighting()
	{
		if(highlight != NULL)
		{
			QGraphicsScene& graphicsScene = plotObject->getGraphicsScene();
			graphicsScene.removeItem(static_cast<QGraphicsItem*>(highlight));
			delete highlight;
			highlight = NULL;
			graphicsScene.update();
		}
	}
	void intervalMode::addHighlighting()
	{
		deleteHighlighting();
		QGraphicsScene& graphicsScene = plotObject->getGraphicsScene();
		int nMarkers = data.getMarkerCount();
		if(start < end)
		{
			highlight = graphicsScene.addRect(start, 0, end - start + 1, nMarkers, QPen(Qt::NoPen), highlightColour);
		}
		else
		{
			highlight = graphicsScene.addRect(end, 0, start - end + 1, nMarkers, QPen(Qt::NoPen), highlightColour);
		}
		highlight->setZValue(2);
		graphicsScene.update();
	}
	void intervalMode::enterMode()
	{
		start = end = -1;
		deleteHighlighting();
		frame->show();
	}
	void intervalMode::leaveMode()
	{
		frame->hide();
		deleteHighlighting();
	}
	void intervalMode::clearCut()
	{
		cutStart = cutEnd = -1;
	}
	void intervalMode::keyPressEvent(QKeyEvent* event)
	{
		if(event->key() == Qt::Key_U && (event->modifiers() & Qt::ControlModifier))
		{
			data.undo();
			plotObject->dataChanged();
			deleteHighlighting();
			start = -1;
			end = -1;
			clearCut();
		}
		else if(event->key() == Qt::Key_O && (event->modifiers() & Qt::ControlModifier))
		{
			clearCut();
			//See documentation for attemptBeginComputation
			if(start > -1 && end > -1 && plotObject->attemptBeginComputation())
			{
				//we only need to do imputation here if it hasn't been done previously.
				//Reason: This mode is only available if there's only one group. In which case no group joining is possible
				int nOriginalMarkers = data.getOriginalMarkerCount();
				if(*imputedRawData == NULL) 
				{
					*imputedRawData = new unsigned char[(nOriginalMarkers * (nOriginalMarkers + 1))/2];
					memcpy(*imputedRawData, rawImageData, sizeof(unsigned char)*(nOriginalMarkers * (nOriginalMarkers + 1))/2);
					doImputation(data.getCurrentGroups()[0]);
				}
				//permutation from just the submatrix
				std::vector<int> resultingPermutation;
				const std::vector<int>& currentPermutation = data.getCurrentPermutation();
				{
					int newStart = std::min(start, end);
					int newEnd = std::max(start, end);
					start = newStart;
					end = newEnd;
				}
				int nSubMarkers = end + 1 - start;
				//The ordering code crashes with only one or two markers
				if(nSubMarkers >= 3)
				{
					order(*imputedRawData, levels, nOriginalMarkers, currentPermutation, start, end+1, resultingPermutation);
					//and the conversion of the submatrix permutation to the bigger matrix
					std::vector<int> totalPermutation;
					int nMarkers = data.getMarkerCount();
					totalPermutation.reserve(nMarkers);
					for(int i = 0; i < nMarkers; i++) totalPermutation.push_back(i);
					for(int i = 0; i < nSubMarkers; i++)
					{
						totalPermutation[i + start] = start + resultingPermutation[i];
					}
					data.applyPermutation(totalPermutation, data.getCurrentGroups());
					plotObject->dataChanged();
				}
				plotObject->endComputation();
			}
		}
		else if(event->key() == Qt::Key_R && (event->modifiers() & Qt::ControlModifier))
		{
			clearCut();
			if(start > -1 && end > -1 && plotObject->attemptBeginComputation()) 
			{
				{
					int newStart = std::min(start, end);
					int newEnd = std::max(start, end);
					start = newStart;
					end = newEnd;
				}
				int nMarkers = data.getMarkerCount();
				std::vector<int> resultingPermutation;
				for(int i = 0; i < nMarkers; i++) resultingPermutation.push_back(i);
				for(int i = start; i <= end; i++) resultingPermutation[i] = end - (i-start);
				data.applyPermutation(resultingPermutation, data.getCurrentGroups());
				plotObject->dataChanged();
				plotObject->endComputation();
			}
		}
		else if(event->key() == Qt::Key_X && (event->modifiers() & Qt::ControlModifier))
		{
			if(start > -1 && end > -1)
			{
				cutStart = std::min(start, end);
				cutEnd = std::max(start, end);
				start = end = -1;
				deleteHighlighting();
			}
		}
		else if(event->key() == Qt::Key_V && (event->modifiers() & Qt::ControlModifier))
		{
			if(cutEnd > -1 && cutStart > -1)
			{
				//If we're putting the cut region back in the same spot, then do nothing. 
				if(start >= cutStart && start <= cutEnd+1)
				{
					clearCut();
				}
				else
				{
					if(plotObject->imageTiles.size() > 1)
					{
						throw std::runtime_error("Cannot perform a cut and paste if there is more than one group shown");
					}
					plotObject->imageTiles.begin()->shiftMarkers(cutStart, cutEnd, start);
					//Create permutation
					int nMarkers = data.getMarkerCount();
					std::vector<int> permutation(nMarkers);
					int cutSize = cutEnd - cutStart + 1;
					if(start < cutStart)
					{
						//Start until the destination
						for(int i = 0; i < start; i++) permutation[i] = i;
						//Put in the cut markers
						for(int i = start; i < start + cutSize; i++) permutation[i] = cutStart + (i - start);
						//Markers between the destination and source
						for(int i = start + cutSize; i < cutEnd+1; i++) permutation[i] = i - cutSize;
						//Markers after the source are unchanged
						for(int i = cutEnd + 1; i < nMarkers; i++) permutation[i] = i;
					}
					else if(start > cutEnd+1)
					{
						//Start until the source
						for(int i = 0; i < cutStart; i++) permutation[i] = i;
						//Take out the cut markers
						for(int i = cutStart; i < start - cutSize; i++) permutation[i] = i + cutSize;
						//Put in the cut markers
						for(int i = start - cutSize;  i < start; i++) permutation[i] = i - (start - cutSize) + cutStart;
						//Markers after the target are unchanged
						for(int i = start; i < nMarkers; i++) permutation[i] = i;
					}
					else
					{
						throw std::runtime_error("Internal error");
					}
					data.applyPermutation(permutation, data.getCurrentGroups());

					start = -1;
					end = -1;
					deleteHighlighting();
					plotObject->dataChanged();
					clearCut();
				}
			}
		}
	}
	void intervalMode::mousePressed(int x, int y, Qt::MouseButtons pressed)
	{
		int nMarkers = data.getMarkerCount();
		if(pressed & Qt::RightButton)
		{
			deleteHighlighting();
		}
		else if(pressed & Qt::LeftButton)
		{
			if(QApplication::keyboardModifiers() & Qt::ShiftModifier && start > -1)
			{
				clearCut();
				end = x;
				addHighlighting();
			}
			else
			{
				deleteHighlighting();
				start = x;
				end = -1;
			}
		}
	}
	void intervalMode::mouseMove(int x, int y)
	{
	}
	void intervalMode::leaveFocus()
	{
	}
}
