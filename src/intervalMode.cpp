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
	void intervalMode::keyPressEvent(QKeyEvent* event)
	{
		if(event->key() == Qt::Key_O && (event->modifiers() & Qt::ControlModifier))
		{
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
