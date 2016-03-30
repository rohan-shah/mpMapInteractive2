#include "groupsMode.h"
#include <QFormLayout>
#include <QIntValidator>
#include <QKeyEvent>
#include "order.h"
#include "qtPlot.h"
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
namespace mpMapInteractive
{
	void groupsMode::constructFrame()
	{
		frame = new QFrame;
		//set up layout on left hand side for labels / inputs
		QFormLayout* formLayout = new QFormLayout;

		QLabel* undoLabel = new QLabel(QString("Undo (Ctrl + U)"));
		//set up pallete to highlight enabled labels / shortcuts
		QPalette p = undoLabel->palette();
		p.setColor(QPalette::Active, QPalette::WindowText, QColor("blue"));
		undoLabel->setPalette(p);
		formLayout->addRow(undoLabel, new QLabel(""));

		joinGroupsLabel = new QLabel("Join groups (Ctrl + j)");
		joinGroupsLabel->setEnabled(false);
		joinGroupsLabel->setPalette(p);
		formLayout->addRow(joinGroupsLabel, new QLabel(""));

		QHBoxLayout* gotoLayout = new QHBoxLayout;
		group1Edit = new QLineEdit;
		group1Edit->setValidator(new QIntValidator());
		group2Edit = new QLineEdit;
		group2Edit->setValidator(new QIntValidator());
		
		QObject::connect(group1Edit, SIGNAL(returnPressed()), this, SLOT(group1ReturnPressed()));
		QObject::connect(group2Edit, SIGNAL(returnPressed()), this, SLOT(group2ReturnPressed()));
		gotoLayout->addWidget(group1Edit);
		gotoLayout->addWidget(group2Edit);

		QLabel* gotoLabel = new QLabel("Goto groups (Ctrl + G)");
		gotoLabel->setPalette(p);
		formLayout->addRow(gotoLabel, gotoLayout);
		frame->setLayout(formLayout);

		frame->setTabOrder(group1Edit, group2Edit);
		frame->setTabOrder(group2Edit, group1Edit);

		QLabel* orderLabel = new QLabel("Order all groups except (Ctrl + O)");
		orderLabel->setEnabled(true);
		orderLabel->setPalette(p);

		orderAllExcept = new QLineEdit;
		QRegExp intList(QString("(\\d+\\s*)*"));
		orderAllExcept->setValidator(new QRegExpValidator(intList));
		formLayout->addRow(orderLabel, orderAllExcept);
	}
	groupsMode::~groupsMode()
	{
		delete joinGroupsLabel;
		delete group1Edit;
		delete group2Edit;
		delete orderAllExcept;
		delete frame;
	}
	void groupsMode::group2ReturnPressed()
	{
		int group1, group2;
		group1 = std::atoi(group1Edit->text().toStdString().c_str());
		const std::vector<int>& currentGroups = data.getCurrentGroups();
		if(std::find(currentGroups.begin(), currentGroups.end(), group1) == currentGroups.end())
		{
			return;
		}
		group2 = std::atoi(group2Edit->text().toStdString().c_str());
		if(std::find(currentGroups.begin(), currentGroups.end(), group2) == currentGroups.end())
		{
			return;
		}
		int start1 = data.startOfGroup(group1);
		int end1 = data.endOfGroup(group1);
		int start2 = data.startOfGroup(group2);
		int end2 = data.endOfGroup(group2);
		plotObject->getGraphicsView().fitInView(start2,start1,end2-start2,end1-start1,Qt::KeepAspectRatio);
		plotObject->signalMouseMove();
	}
	void groupsMode::leaveMode()
	{
		deleteHighlighting();
	}
	void groupsMode::deleteHighlighting()
	{
		QGraphicsScene& graphicsScene = plotObject->getGraphicsScene();
		if(horizontalGroup != -1)
		{
			graphicsScene.removeItem(static_cast<QGraphicsItem*>(horizontalHighlight));
			delete horizontalHighlight;
			horizontalHighlight = NULL;
		}
		if(verticalGroup != -1)
		{
			graphicsScene.removeItem(static_cast<QGraphicsItem*>(verticalHighlight));
			delete verticalHighlight;
			verticalHighlight = NULL;
		}
		horizontalGroup = -1;
		verticalGroup = -1;
	}
	void groupsMode::mouseMove(int x, int y)
	{
		int nMarkers = data.getMarkerCount();
		if(0 <= x && x < nMarkers && 0 <= y && y < nMarkers)
		{
			renewHighlighting(x, y);
			const std::vector<int> currentGroups = data.getCurrentGroups();
			joinGroupsLabel->setEnabled(currentGroups[y] != currentGroups[x]);
		}
		else
		{
			deleteHighlighting();
			joinGroupsLabel->setEnabled(false);
		}
	}
	void groupsMode::renewHighlighting(int x, int y)
	{
		//are we not highlighting anything?
		if(x == -1 || y == -1)
		{
			//is this different from previous state?
			if(horizontalGroup == -1 || verticalGroup == -1)
			{
				return;
			}
			deleteHighlighting();
			return;
		}
		QGraphicsScene& graphicsScene = plotObject->getGraphicsScene();
		const std::vector<int>& currentGroups = data.getCurrentGroups();
		int newHorizontalGroup = currentGroups[y];
		int newVerticalGroup = currentGroups[x];
		//If we're highlighting the same stuff as previously, do nothing
		if(newHorizontalGroup == horizontalGroup && newVerticalGroup == verticalGroup)
		{
			return;
		}
		if(horizontalGroup != -1)
		{
			graphicsScene.removeItem(static_cast<QGraphicsItem*>(horizontalHighlight));
			delete horizontalHighlight;
			horizontalHighlight = NULL;
		}
		if(verticalGroup != -1)
		{
			graphicsScene.removeItem(static_cast<QGraphicsItem*>(verticalHighlight));
			delete verticalHighlight;
			verticalHighlight = NULL;
		}

		int firstHorizontalIndex = data.startOfGroup(newHorizontalGroup);
		int lastHorizontalIndex = data.endOfGroup(newHorizontalGroup);

		int firstVerticalIndex = data.startOfGroup(newVerticalGroup);
		int lastVerticalIndex = data.endOfGroup(newVerticalGroup);

		int nMarkers = data.getMarkerCount();
		horizontalHighlight = graphicsScene.addRect(0, firstHorizontalIndex, nMarkers, lastHorizontalIndex - firstHorizontalIndex, QPen(Qt::NoPen), highlightColour);
		verticalHighlight = graphicsScene.addRect(firstVerticalIndex, 0, lastVerticalIndex - firstVerticalIndex, nMarkers, QPen(Qt::NoPen), highlightColour);
		verticalHighlight->setZValue(2);
		horizontalHighlight->setZValue(2);
		
		horizontalGroup = newHorizontalGroup;
		verticalGroup = newVerticalGroup;
		
		graphicsScene.update();
	}
	void groupsMode::enterMode()
	{
		frame->show();
		plotObject->signalMouseMove();
	}
	void groupsMode::keyPressEvent(QKeyEvent* event)
	{
		if(event->key() == Qt::Key_U && (event->modifiers() & Qt::ControlModifier))
		{
			data.undo();
			plotObject->dataChanged();
			//if there's been a structural change, we HAVE to redo the highlighting, irrespective of whether the highlighted group number is the same
			deleteHighlighting();
			plotObject->signalMouseMove();
		}
		if(event->key() == Qt::Key_O && (event->modifiers() & Qt::ControlModifier))
		{
			if(plotObject->attemptBeginComputation())
			{
				QMessageBox confirm;
				confirm.setText("This could take a while. Continue?");
				confirm.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
				confirm.setDefaultButton(QMessageBox::No);
				int ret = confirm.exec();
				if(ret == QMessageBox::Yes)
				{
					std::vector<int> uniqueGroups = data.getCurrentGroups();
					std::sort(uniqueGroups.begin(), uniqueGroups.end());
					uniqueGroups.erase(std::unique(uniqueGroups.begin(), uniqueGroups.end()), uniqueGroups.end());
					size_t nGroups = uniqueGroups.size();
					
					QRegExp intListRegex(QString("(\\d+)"));
					QString exceptionsText = orderAllExcept->text();
					int pos = 0;
					std::vector<int> exceptionsList;
					while((pos = intListRegex.indexIn(exceptionsText, pos))!= -1)
					{
						bool ok;
						exceptionsList.push_back(intListRegex.cap(1).toInt(&ok));
						pos += intListRegex.matchedLength();
					}

					//do the imputation again - If groups have been joined then we could end up with NAs in the recombination fraction matrix. Remember that the imputation only removes NAs between markers IN THE SAME GROUP, using the group structure as currently set. We need to assign a group to EVERY marker that was ORIGINALLY here. So everything that has been deleted, and therefore doesn't have a group, goes in (max(group) + 1). 
					int nOriginalMarkers = data.getOriginalMarkerCount();
					if(*imputedRawData == NULL) *imputedRawData = new unsigned char[(nOriginalMarkers * (nOriginalMarkers + 1))/2];
					memcpy(*imputedRawData, rawImageData, sizeof(unsigned char)*(nOriginalMarkers * (nOriginalMarkers + 1))/2);
					for(std::vector<int>::iterator group = uniqueGroups.begin(); group != uniqueGroups.end(); group++)
					{
						if(std::find(exceptionsList.begin(), exceptionsList.end(), *group) == exceptionsList.end()) doImputation(*group);
					}

					QProgressBar* progress = plotObject->addProgressBar();
					progress->setMinimum(0);
					progress->setMaximum((int)(nGroups - exceptionsList.size()));
					
					int nMarkers = data.getOriginalMarkerCount();
					std::vector<int> totalResultingPermutation;
					totalResultingPermutation.resize(nMarkers);
					for(int groupCounter = 0; groupCounter < nGroups; groupCounter++)
					{
						if(std::find(exceptionsList.begin(), exceptionsList.end(), uniqueGroups[groupCounter]) != exceptionsList.end()) continue;
						const std::vector<int>& currentPermutation = data.getCurrentPermutation();
						int startOfGroup = data.startOfGroup(uniqueGroups[groupCounter]);
						int endOfGroup = data.endOfGroup(uniqueGroups[groupCounter]);
						int nSubMarkers = endOfGroup - startOfGroup;
						//The ordering code crashes with only one or two markers. So avoid that case. 
						if(nSubMarkers >= 3)
						{
							//get out the permutation that is going to be applied just to this bit
							std::vector<int> resultingPermutation;
							order(*imputedRawData, levels, nOriginalMarkers, currentPermutation, startOfGroup, endOfGroup, resultingPermutation);
						
							//extend this to a permutation that will be applied to the whole matrix
							//mostly it's the identity, because we're only ordering a small part of the matrix in this step
							for(int i = 0; i < nMarkers; i++)
							{
								totalResultingPermutation[i] = i;
							}
							//...but part is not the identity
							for(int i = 0; i < nSubMarkers; i++)
							{
								totalResultingPermutation[i + startOfGroup] = startOfGroup + resultingPermutation[i];
							}
							data.applyPermutation(totalResultingPermutation, data.getCurrentGroups());
						}
						progress->setValue(groupCounter+1);
					}
					plotObject->dataChanged();
					plotObject->deleteProgressBar(progress);
				}
				plotObject->endComputation();
			}
		}
		if(event->key() == Qt::Key_J && (event->modifiers() & Qt::ControlModifier))
		{
			int nMarkers = data.getOriginalMarkerCount();
			QGraphicsView& graphicsView = plotObject->getGraphicsView();
			QPointF cursorPos = graphicsView.mapToScene(graphicsView.mapFromGlobal(QCursor::pos()));
			int x = cursorPos.x(), y = cursorPos.y();
			if(0 <= x && x < nMarkers && 0 <= y && y < nMarkers && plotObject->attemptBeginComputation())
			{
				joinGroups(x, y);
				renewHighlighting(x, y);
				plotObject->endComputation();
			}
		}
		if(event->key() == Qt::Key_G && (event->modifiers() & Qt::ControlModifier))
		{
			group1Edit->setFocus();
		}
	}
	void groupsMode::leaveFocus()
	{
		horizontalGroup = -1;
		verticalGroup = -1;
		deleteHighlighting();
	}
	void groupsMode::group1ReturnPressed()
	{
		int group = std::atoi(group1Edit->text().toStdString().c_str());
		const std::vector<int>& currentGroups = data.getCurrentGroups();
		if(std::find(currentGroups.begin(), currentGroups.end(), group) != currentGroups.end())
		{
			int start = data.startOfGroup(group);
			int end = data.endOfGroup(group);
			QGraphicsView& graphicsView = plotObject->getGraphicsView();
			graphicsView.fitInView(start,start,end-start,end-start,Qt::KeepAspectRatioByExpanding);
			plotObject->signalMouseMove();
		}
	}
	void groupsMode::mousePressed(int x, int y, Qt::MouseButtons pressed)
	{
	}
	void groupsMode::joinGroups(int x, int y)
	{
		int nMarkers = data.getMarkerCount();
		if(!(0 <= x && x < nMarkers && 0 <= y && y < nMarkers))
		{
			throw std::runtime_error("Internal error");
		}
		const std::vector<int>& currentGroups = data.getCurrentGroups();
		int group1 = currentGroups[x];
		int group2 = currentGroups[y];
		
		//can't join a group to itself
		if(group1 == group2) return;

		int newGroup = std::min(group1, group2);

		int startGroup1 = data.startOfGroup(group1);
		int startGroup2 = data.startOfGroup(group2);
		int endGroup1 = data.endOfGroup(group1), endGroup2 = data.endOfGroup(group2);

		std::vector<int> permutation;
		permutation.resize(nMarkers);
		std::vector<int> newGroups;
		newGroups.resize(nMarkers);

		int counter = 0;
		while(counter < std::min(startGroup1, startGroup2))
		{
			permutation[counter] = counter;
			newGroups[counter] = currentGroups[counter];
			counter++;
		}
		//put in groups 1 and 2
		for(int i = startGroup1; i < endGroup1; i++)
		{
			permutation[counter] = i;
			newGroups[counter] = newGroup;
			counter++;
		}
		for(int i = startGroup2; i < endGroup2; i++)
		{
			permutation[counter] = i;
			newGroups[counter] = newGroup;
			counter++;
		}
		//now put in everything else
		for(int i = std::min(startGroup1, startGroup2); i < nMarkers; i++)
		{
			if(currentGroups[i] != group1 && currentGroups[i] != group2)
			{
				permutation[counter] = i;
				newGroups[counter] = currentGroups[i];
				counter++;
			}
		}
		data.applyPermutation(permutation, newGroups);
		plotObject->dataChanged();
	}

}
