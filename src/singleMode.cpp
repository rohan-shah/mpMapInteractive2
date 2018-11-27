#include "singleMode.h"
#include "qtPlot.h"
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QGraphicsRectItem>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
namespace mpMapInteractive
{
	void singleMode::enterMode()
	{
		deleteHighlight();
		position = -1;
		frame->show();
	}
	void singleMode::keyPressEvent(QKeyEvent* event)
	{
		if(event->key() == Qt::Key_U && (event->modifiers() & Qt::ControlModifier))
		{
			data.undo();
			plotObject->dataChanged();
			deleteHighlight();
			position = -1;
		}
		else if(event->key() == Qt::Key_Delete)
		{
			if(position > -1 && plotObject->attemptBeginComputation())
			{
				int nMarkers = data.getMarkerCount();

				//Construct permutation by which we are altering the data
				std::vector<int> permutation;
				for(int i = 0; i < nMarkers; i++)
				{
					if(i != position) permutation.push_back(i);
				}
				//Get the current permutation, so we can get out the index of the marker shown at position
				const std::vector<int>& currentPermutation = data.getCurrentPermutation();
				int markerAtPosition = currentPermutation[position];
				//remove one element from groups vector
				const std::vector<int>& previousGroups = data.getCurrentGroups();
				std::vector<int> newGroups(nMarkers-1);
				std::copy(previousGroups.begin(), previousGroups.begin() + position, newGroups.begin());
				std::copy(previousGroups.begin() + position +1, previousGroups.end(), newGroups.begin() + position);
				//Apply the permutation
				data.applyPermutation(permutation, newGroups);
				//Get out the image tiles
				//Go through them and tell each one to remove the marker. We do this because we can do this more efficiently than just letting dataChanged() recreate everything from scratch. The changes we make to the image tiles here mean that dataChanged() will decide that nothing actually needs to be done. Although it will still delete imageTiles which become empty. 
				for(std::set<imageTileWithAux, imageTileComparer>::iterator i = plotObject->imageTiles.begin(); i != plotObject->imageTiles.end(); i++)
				{
					i->deleteMarker(markerAtPosition);
				}

				//Signal to update the image onscreen. This re-creates from scratch the bits of the image that no longer match the underlying data
				plotObject->dataChanged();
				//update the highlighting
				nMarkers = data.getMarkerCount();
				if(position >= nMarkers) position -= 1;
				deleteHighlight();
				addHighlight();
				//Now that we've updated the highlighting, tell the graphicsScene to update again
				QGraphicsScene& graphicsScene = plotObject->getGraphicsScene();
				graphicsScene.update();
				//End the computation
				plotObject->endComputation();
			}
		}
	}
	void singleMode::leaveMode()
	{
		frame->hide();
		position = -1;
		deleteHighlight();
	}
	void singleMode::deleteHighlight()
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
	void singleMode::mouseMove(int x, int y)
	{
	}
	void singleMode::mousePressed(int x, int y, Qt::MouseButtons pressed)
	{
		int nMarkers = data.getMarkerCount();

		if(pressed & Qt::RightButton)
		{
			position = -1;
			deleteHighlight();
		}
		else if(x >= 0 && x < nMarkers && y >= 0 && y < nMarkers && pressed & Qt::LeftButton)
		{
			deleteHighlight();
			position = x;
			addHighlight();
		}
	}
	void singleMode::addHighlight()
	{
		if(position > -1)
		{
			int nMarkers = data.getMarkerCount();
			QGraphicsScene& graphicsScene = plotObject->getGraphicsScene();
			highlight = graphicsScene.addRect(position, 0, 1, nMarkers, QPen(Qt::NoPen), highlightColour);
			highlight->setZValue(2);
			graphicsScene.update();
		}
	}
	void singleMode::leaveFocus()
	{
	}
	void singleMode::constructFrame()
	{
		frame = new QFrame;
		QFormLayout* formLayout = new QFormLayout;
		
		QLabel* undoLabel = new QLabel(QString("Undo (Ctrl + U)"));
		undoLabel->setToolTip(QString("Undo last change"));

		//set up pallete to highlight enabled labels / shortcuts
		QPalette p = undoLabel->palette();
		p.setColor(QPalette::Active, QPalette::WindowText, QColor("blue"));
		undoLabel->setPalette(p);
		formLayout->addRow(undoLabel, new QLabel(""));

		deleteLabel = new QLabel(QString("Delete (Del)"));
		deleteLabel->setPalette(p);
		deleteLabel->setToolTip(QString("Delete the currently selected marker"));

		formLayout->addRow(deleteLabel, new QLabel(""));

		frame->setLayout(formLayout);
	}
}
