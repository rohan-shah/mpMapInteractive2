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
#include "order.h"
#include <Rcpp.h>
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

		QLabel* clusterOrderLabel = new QLabel(QString("Groups for hclust ordering"));
		clusterOrderLabel->setPalette(p);
		orderingEdit = new QLineEdit;
		orderingEdit->setValidator(new QIntValidator());
		formLayout->addRow(clusterOrderLabel, orderingEdit);

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
		else if(event->key() == Qt::Key_L && (event->modifiers() & Qt::ControlModifier))
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
					int nGroups = std::atoi(orderingEdit->text().toStdString().c_str());
					if(nGroups == 0) goto endComputation;
					
					bool nonZero = false;
					//Convert the relevant chunk of the raw symmetric matrix to a dense matrix, for use in hierarcichal clustering
					Rcpp::NumericMatrix denseMatrix(nSubMarkers, nSubMarkers);
					rawSymmetricMatrixToDense(&(denseMatrix(0, 0)), *imputedRawData, levels, nOriginalMarkers, currentPermutation, start, end+1, nonZero);
					if(!nonZero) goto endComputation;
					Rcpp::Environment fastClusterEnv("package:fastcluster");
					Rcpp::Function asDist("as.dist"), hclust = fastClusterEnv["hclust"], cutree("cutree"), dotCall(".Call");
					//Convert the dense matrix to a dist object
					Rcpp::RObject denseAsDist = asDist(denseMatrix);

					//Run hclust
					Rcpp::RObject clusterResult = hclust(denseAsDist, Rcpp::Named("method") = "average");
					//Get out groupings
					Rcpp::IntegerVector groupings = cutree(clusterResult, nGroups);
					//Use the groupings to convert the original raw symmetric matrix to a dissimilarity matrix
					SEXP (*constructDissimilarityMatrixInternal)(unsigned char*, std::vector<double>&, int, SEXP, int, const std::vector<int>&) = (SEXP (*)(unsigned char*, std::vector<double>&, int, SEXP, int, const std::vector<int>&))R_GetCCallable("mpMap2", "constructDissimilarityMatrixInternal");
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
					Rcpp::IntegerVector orderingOfGroups = dotCall("arsa", nGroups, dissimilarityMatrixUpper, 0.5, 0.1, 1, Rcpp::Named("PACKAGE") = "mpMap2");
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
