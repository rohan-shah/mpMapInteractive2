#include "intervalMode.h"
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QFrame>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QIntValidator>
#include "qtPlot.h"
#include <Rcpp.h>
#include "order.h"
#include <QCheckBox>
#include <QProgressBar>
#include "arsaArgs.h"
namespace mpMapInteractive
{
	void intervalMode::addSeperator(QFormLayout* formLayout)
	{
		QFrame* seperator = new QFrame;
		seperator->setFrameShape(QFrame::HLine);
		seperator->setFrameShadow(QFrame::Sunken);
		formLayout->addRow(seperator);
	}
	void intervalMode::constructFrame()
	{
		frame = new QFrame;
		QFormLayout* formLayout = new QFormLayout;
		
		undoLabel = new QLabel(QString("Undo (Ctrl + U)"));
		formLayout->addRow(undoLabel, new QLabel(""));

		{
			QFrame* seperator = new QFrame;
			seperator->setFrameShape(QFrame::HLine);
			seperator->setFrameShadow(QFrame::Sunken);
			formLayout->addRow(seperator);
		}

		orderLabel = new QLabel(QString("Order (Ctrl + O)"));
		formLayout->addRow(orderLabel);
		effortLabel = new QLabel("Effort multiplier:");
		effortEdit = new QLineEdit;
		effortEdit->setValidator(new QDoubleValidator());
		formLayout->addRow(effortLabel, effortEdit);

		maxDistEdit = new QLineEdit;
		maxDistEdit->setValidator(new QIntValidator());
		maxDistLabel = new QLabel("Max shift size:");
		formLayout->addRow(maxDistLabel, maxDistEdit);

		randomStartCheckbox = new QCheckBox("Random start");
		formLayout->addRow(randomStartCheckbox);

		addSeperator(formLayout);

		reverseLabel = new QLabel(QString("Reverse (Ctrl + R)"));
		formLayout->addRow(reverseLabel, new QLabel(""));

		addSeperator(formLayout);

		clusterOrderLabel = new QLabel(QString("Order using hclust (Ctrl + H)"));
		formLayout->addRow(clusterOrderLabel);

		clusterOrderGroupsEdit = new QLineEdit;
		clusterOrderGroupsEdit->setValidator(new QIntValidator());
		clusterOrderGroupsLabel = new QLabel("Number of groups");
		formLayout->addRow(clusterOrderGroupsLabel, clusterOrderGroupsEdit);

		clusterOrderEffortEdit = new QLineEdit;
		clusterOrderEffortEdit->setValidator(new QDoubleValidator());
		clusterOrderEffortLabel = new QLabel("Effort multiplier:");
		formLayout->addRow(clusterOrderEffortLabel, clusterOrderEffortEdit);

		addSeperator(formLayout);

		cutLabel = new QLabel(QString("Cut (Ctrl + X)"));
		formLayout->addRow(cutLabel, new QLabel(""));

		pasteLabel = new QLabel(QString("Paste (Ctrl + V)"));
		formLayout->addRow(pasteLabel, new QLabel(""));

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
		undoLabel->setEnabled(data.stackLength() != 0);
		frame->show();
		updateChoices();
	}
	void intervalMode::leaveMode()
	{
		frame->hide();
		deleteHighlighting();
	}
	void intervalMode::keyPressEvent(QKeyEvent* event)
	{
		if(event->key() == Qt::Key_U && (event->modifiers() & Qt::ControlModifier))
		{
			data.undo();
			plotObject->dataChanged();
			deleteHighlighting();
			cutStart = cutEnd = start = end = -1;
			updateChoices();
		}
		else if(event->key() == Qt::Key_O && (event->modifiers() & Qt::ControlModifier))
		{
			cutStart = cutEnd = -1;
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
				const std::vector<int>& currentPermutation = data.getCurrentPermutation();
				{
					int newStart = std::min(start, end);
					int newEnd = std::max(start, end);
					start = newStart;
					end = newEnd;
				}
				double effortMultiple = 1;
				int maxMove = 0;
				try
				{
					effortMultiple = std::stod(effortEdit->text().toStdString());
				}
				catch(...){}
				if(effortMultiple <= 0) effortMultiple = 1;
				try
				{
					maxMove = std::stod(maxDistEdit->text().toStdString());
				}
				catch(...){}
				if(maxMove <= 0) maxMove = 0;

				bool randomStart = randomStartCheckbox->isChecked();

				int nSubMarkers = end + 1 - start;
				//The ordering code crashes with only one or two markers
				if(nSubMarkers >= 3)
				{
					//Extract the subset and turn it into a dense matrix (rather than storing just the upper triangle)
					std::vector<unsigned char> copiedSubset(nSubMarkers*nSubMarkers, 0);
					for(R_xlen_t i = start; i < end+1; i++)
					{
						for(R_xlen_t j = start; j <= i; j++)
						{
							int copied1 = currentPermutation[i], copied2 = currentPermutation[j];
							if(copied1 < copied2) std::swap(copied1, copied2);
							copiedSubset[(i - start)*nSubMarkers + (j - start)] = copiedSubset[(j - start)*nSubMarkers + (i - start)] = (*imputedRawData)[copied1*(copied1 + 1)/2 + copied2];
						}
					}
					std::vector<int> resultingPermutation;
					typedef void (*arsaRawExportedType)(arsaRawArgs& args);
					arsaRawExportedType arsaRawExported = (arsaRawExportedType)R_GetCCallable("mpMap2", "arsaRaw");
					std::vector<int> totalPermutation;
					QProgressBar* progress = plotObject->addProgressBar();
					progress->setMinimum(0);
					progress->setMaximum(100);

					shouldCancel = false;
					QPushButton* cancelButton = plotObject->addCancelButton();
					QObject::connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(cancel()));

					std::function<bool(unsigned long,unsigned long)> progressFunction = [this,progress](unsigned long done, unsigned long totalSteps){
						progress->setValue(100.0 * (double)done / (double)totalSteps);
						QCoreApplication::processEvents();
						return this->shouldCancel;
					};
					arsaRawArgs arsaArgs(levels, resultingPermutation);
					arsaArgs.n = nSubMarkers;
					arsaArgs.rawDist = &copiedSubset.front();
					arsaArgs.cool = 0.5;
					arsaArgs.temperatureMin = 0.1;
					arsaArgs.nReps = 1;
					arsaArgs.progressFunction = progressFunction;
					arsaArgs.randomStart = randomStart;
					arsaArgs.maxMove = maxMove;
					arsaArgs.effortMultiplier = effortMultiple;
					arsaRawExported(arsaArgs);
					plotObject->deleteProgressBar(progress);
					plotObject->deleteCancelButton(cancelButton);

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
			updateChoices();
		}
		else if(event->key() == Qt::Key_H && (event->modifiers() & Qt::ControlModifier))
		{
			cutStart = cutEnd = -1;
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
					//Number of groups for hierarchical clustering
					int nGroups = 0;
					double effortMultiplier = 1;
					try
					{	
						nGroups = std::stoi(clusterOrderGroupsEdit->text().toStdString());
					}
					catch(...){}
					try
					{
						effortMultiplier = std::stod(clusterOrderEffortEdit->text().toStdString());
					}
					catch(...){}
					if(nGroups == 0) goto endComputation;
					
					bool nonZero = false;
					//Convert the relevant chunk of the raw symmetric matrix to a dense matrix, for use in hierarcichal clustering
					Rcpp::NumericMatrix denseMatrix(nSubMarkers, nSubMarkers);
					rawSymmetricMatrixToDense(&(denseMatrix(0, 0)), *imputedRawData, levels, nOriginalMarkers, currentPermutation, start, end+1, nonZero);
					if(!nonZero) goto endComputation;
					Rcpp::Environment fastClusterEnv("package:fastcluster");
					Rcpp::Function asDist("as.dist"), hclust = fastClusterEnv["hclust"], cutree("cutree");
					//Convert the dense matrix to a dist object
					Rcpp::RObject denseAsDist = asDist(denseMatrix);

					//Run hclust
					Rcpp::RObject clusterResult = hclust(denseAsDist, Rcpp::Named("method") = "average");
					//Get out groupings
					Rcpp::IntegerVector groupings = cutree(clusterResult, nGroups);
					//Use the groupings to convert the original raw symmetric matrix to a dissimilarity matrix
					typedef SEXP (*constructDissimilarityMatrixInternalType)(unsigned char*, std::vector<double>&, int, SEXP, int, const std::vector<int>&);
					constructDissimilarityMatrixInternalType constructDissimilarityMatrixInternal = (constructDissimilarityMatrixInternalType)R_GetCCallable("mpMap2", "constructDissimilarityMatrixInternal");
					Rcpp::NumericVector dissimilarityMatrix = constructDissimilarityMatrixInternal(*imputedRawData, levels, data.getOriginalMarkerCount(), groupings, start, currentPermutation);
					//Set the diagonal to be zero
					for(int i = 0; i < nGroups; i++) dissimilarityMatrix(i, i) = 0;
					//Get out the upper triangle of the dissimilarity matrix
					Rcpp::NumericVector dissimilarityMatrixUpper(nGroups * (nGroups + 1) / 2);
					int counter = 0;
					for(int i = 0; i < nGroups; i++)
					{
						for(int j = 0; j <= i; j++)
						{
							dissimilarityMatrixUpper(counter) = dissimilarityMatrix(j, i);
							counter++;
						}
					}
					//Use the dissimilarity matrix to run the ARSA code
					std::vector<int> orderingOfGroups;
					typedef void (*arsaType)(arsaArgs& args);
					arsaType arsa = (arsaType)R_GetCCallable("mpMap2", "arsa");
					QProgressBar* progress = plotObject->addProgressBar();
					progress->setMinimum(0);
					progress->setMaximum(100);

					shouldCancel = false;
					QPushButton* cancelButton = plotObject->addCancelButton();
					QObject::connect(cancelButton, SIGNAL(clicked(bool)), this, SLOT(cancel()));

					std::function<bool(unsigned long,unsigned long)> progressFunction = [progress,this](unsigned long done, unsigned long totalSteps){
						progress->setValue(100.0 * (double)done / (double)totalSteps);
						QCoreApplication::processEvents();
						return this->shouldCancel;
					};
					arsaArgs args;
					args.n = nGroups;
					args.dist = &(dissimilarityMatrixUpper(0));
					args.nReps = 1;
					args.cool = 0.5;
					args.temperatureMin = 0.1;
					args.effortMultiplier = effortMultiplier;
					args.randomStart = true;
					args.maxMove = 0;
					args.effortMultiplier = effortMultiplier;
					args.progressFunction = progressFunction;
					arsa(args);
					orderingOfGroups.swap(args.bestPermutationAllReps);
					plotObject->deleteProgressBar(progress);
					plotObject->deleteCancelButton(cancelButton);
	
					//Create an identity permutation
					std::vector<int> totalPermutation;
					totalPermutation.reserve(currentPermutation.size());
					for(int i = 0; i < currentPermutation.size(); i++) totalPermutation.push_back(i);
					//Alter the identity permutation to represent the reordering. 
					counter = 0;
					//Iterate over the ordere groups
					for(int i = 0; i < nGroups; i++)
					{
						for(int j = 0; j < nSubMarkers; j++)
						{
							if(groupings[j] == orderingOfGroups[i]+1)
							{
								totalPermutation[counter + start] = start + j;
								counter++;
							}
						}
					}
					data.applyPermutation(totalPermutation, data.getCurrentGroups());
					plotObject->dataChanged();
				}
endComputation:
				plotObject->endComputation();
			}
			updateChoices();
		}
		else if(event->key() == Qt::Key_R && (event->modifiers() & Qt::ControlModifier))
		{
			cutStart = cutEnd = -1;
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
			updateChoices();
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
			updateChoices();
		}
		else if(event->key() == Qt::Key_V && (event->modifiers() & Qt::ControlModifier))
		{
			if(cutEnd > -1 && cutStart > -1 && start != -1 && end == -1)
			{
				//If we're putting the cut region back in the same spot, then do nothing. 
				if(start >= cutStart && start <= cutEnd+1)
				{
					cutStart = cutEnd = -1;
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
					cutStart = cutEnd = -1;
				}
			}
			updateChoices();
		}
	}
	void intervalMode::updateChoices()
	{
		undoLabel->setEnabled(data.stackLength() > 0);
		reverseLabel->setEnabled(start != -1 && end != -1);
		cutLabel->setEnabled(start != -1 && end != -1);
		pasteLabel->setEnabled(cutStart != -1 && cutEnd != -1 && start != -1 && end == -1);

		clusterOrderLabel->setEnabled(start != -1 && end != -1);
		clusterOrderGroupsEdit->setEnabled(start != -1 && end != -1);
		clusterOrderGroupsLabel->setEnabled(start != -1 && end != -1);
		clusterOrderEffortEdit->setEnabled(start != -1 && end != -1);
		clusterOrderEffortLabel->setEnabled(start != -1 && end != -1);

		//Order related stuff
		effortLabel->setEnabled(start != -1 && end != -1);
		effortEdit->setEnabled(start != -1 && end != -1);
		orderLabel->setEnabled(start != -1 && end != -1);
		maxDistEdit->setEnabled(start != -1 && end != -1);
		randomStartCheckbox->setEnabled(start != -1 && end != -1);
		maxDistLabel->setEnabled(start != -1 && end != -1);
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
				end = std::max(0, std::min(x, nMarkers-1));
				addHighlighting();
			}
			else
			{
				deleteHighlighting();
				start = std::max(0, std::min(x, nMarkers-1));
				end = -1;
			}
		}
		updateChoices();
	}
	void intervalMode::mouseMove(int x, int y)
	{
	}
	void intervalMode::leaveFocus()
	{
	}
	void intervalMode::cancel()
	{
		shouldCancel = true;
	}
}
