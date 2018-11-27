\name{mpMapInteractive2}
\alias{mpMapInteractive2}
\title{Make interactive changes to linkage groups and marker order}
\usage{
mpMapInteractive2(mpcross, permutations = NULL, groups = NULL, auxiliaryData = NULL)
}
\arguments{
\item{mpcross}{An object of class mpcrossLG.}

\item{permutations}{Undocumented, allows you to continue using the same edit history in a new session (e.g. Undo). }

\item{groups}{Undocumented, allows you to continue using the same edit history in a new session (e.g. Undo). }

\item{auxiliaryData}{Undocumented, allows you to display extra data for the currently highlighted marker, in the status bar.}
}
\value{
A list containing the following entries:

\item{object}{The new mpcross object created during the interactive session. This means that linkage groups will be joined, and marker ordering will be changed, according to user input during the interactive session.}

\item{groups}{A history of the changes to the linkage groups during the interactive session. This can be used to continue using the same edit history, in a new session. }

\item{permutations}{A history of the changes to the marker ordering during the interactive session. This can be used to continue using the same edit history, in a new session. }
}
\description{
This function brings up an interactive window using the QT graphics system. This window displays the recombination fraction heatmap, with the markers in the input order. Based on the information in this heatmap, the user can make changes to the linkage groups, and marker ordering within those linkage groups. 

}
\details{

\subsection{The interactive display}{

	The display shows commands and inputs on the left hand side, and the recombination fraction heatmap on the right hand side. The heatmap shows low recombination fractions in red, and high recombination fractions in yellow. The split between the two sides is adjustable, in case the user wants to see more of one or the other. For example, mouse over the line between the two regions, and click and drag to the left, in order to make the display of the recombination fraction heatmap bigger. 

	The display operates in three modes, selectable using a drop-down list: "Groups", "Single marker" and "Interval". Each of these modes operates slightly differently, but there are a couple of commands that are common. Use the mousewheel to zoom in on the recombination fraction heatmap, centred on the point where the mouse is currently located. Use the arrow keys to shift the display around the heatmap. Alternatively, zoom out, and in again on the target region. 

	Pressing Ctrl + U always undoes the last change made. 

	If there are multiple linkage groups displayed in the heatmap, the difference between adjacent linkage groups is shown in alternating patterns of light and dark. 
}

\subsection{Groups}{

	The groups mode is intended to allow the user to join up linkage groups which were previously marked as distinct; based on the recombination fraction heatmap, a human can identify linkage groups that should be joined, in a way that algorithms generally can't. This is because a human can use the ordering structure of the markers, where clustering algorithms generally aren't designed to. To join up two linkage groups, highlight an off diagonal region in the heatmap. For example, to join up linkage groups 1 and 10, move the mouse over a point with a column corresponding to a marker in linkage group 1, and a row corresponding to a marker in linkage group 10. You will see blue highlighting, indicating the pair of selected linkage groups. Pressing Ctrl + J will join these two linkage groups into a single group. 

	In order to see which linkage groups should be joined, it is often useful to order all the existing linkage groups. This command run by pressing Ctrl + O. In some cases the user might want to order all linkage groups except for several very large linkage groups, which are very slow to order. To exclude some linkage groups, enter those groups in the relevant input, separated by spaces. 

	To jump to the recombination fraction heatmap for a specific linkage group, enter a single group in the input labelled "Group 1", leave the second group blank, and hit Ctrl + G. To jump to the recombination fraction heatmap BETWEEN TWO GROUPS, enter the two groups in "Group 1" and "Group 2", and hit Ctrl + G. 
}
\subsection{Interval}{

	If the input object contains only a single linkage group, the "Interval" mode is available. In this mode, select a region of markers by clicking on one point on the heatmap, and then holding shift and clicking on another point on the heatmap. The column of the first click is taken as the starting marker, and the column of the second click is taken as the ending marker. All markers between those two markers are selected, and highlighted in blue. We refer to these markers as the "selected marker interval". 

	To order the selected marker interval, hit Ctrl + O. The input "Effort multiplier" determines how much computational effort is used in the ordering step. An empty value is taken to be a value of 1, the default effort. A value of 2 indicates twice as much computational effort, etc. Input "Max shift size" indicates the maximum distance (in terms of number of markers) the algorithm will shift a marker in a single step. For example, a value of 10 indicates that when trying to improve the ordering, the algorithm will only try to shift a marker by at most ten markers to the left or right. Leaving this empty means that there is no limit. If a random start is selected, then the ordering begins at a randomly chosen marker ordering; otherwise ordering begins at the current marker ordering. Shifting markers by large distances is computationally expensive, so reducing this value to 50 - 100 is extremely useful for large linkage groups. However, if markers must be shifted by large distances, in order to reach a good ordering, then reduced values also reduce the quality of the final ordering. So for large linkage groups, the suggested approach is to put the markers into an approximately correct ordering using hclust (Ctrl + H), and then run the ordering algorithm with a limit on the maximum marker shift, and without a random start. This is relatively quick, and does not really compromise on the final marker ordering. 

	To reverse the order of the markers in a selected marker interval, hit Ctrl + R. 

	To put the linkage group into an approximately correct ordering using hierarchical clustering, hit Ctrl + H. We suggest that "Number of groups" be set to ~50, and "Effort multiplier" be set to ~10. 

	To cut the currently selected marker interval, hit Ctrl + X. No change will initially be made, and the highlighting will dissappear. Select another column by clicking once at another point on the heatmap, then hit Ctrl + V. The initially selected marker interval will be removed from its current location, and inserted at the point indicated by the selected column. 
}
\subsection{Single marker}{

	If the input object contains only a single linkage group, the "Single marker" mode is available. In this mode, select a single marker by selecting a column. The currently selected column is highligthed. Hitting Delete will delete the marker at that column. 
}

}
\examples{
library(mpMap2)
map <- qtl::sim.map(len = 100, n.mar = rep(101, 3), include.x=FALSE)
f2Pedigree <- f2Pedigree(1000)
cross <- simulateMPCross(map = map, pedigree = f2Pedigree, mapFunction = haldane, seed = 1)
rf <- estimateRF(cross)
grouped <- formGroups(rf, groups = 30)

tmp <- mpMapInteractive2(grouped)

tmp <- mpMapInteractive2(subset(grouped, groups = 1))
}

