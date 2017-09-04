#include "colour.h"
namespace mpMapInteractive
{
	void constructColourTable(int n, QVector<QRgb>& vector)
	{
		vector.clear();
		QColor colour;
		//First n colours for actual values
		for(int counter = 0; counter < n; counter++)
		{
			colour.setHsvF(counter * (1.0f/6.0f)/(float) (n-1), 1.0, 1.0);
			vector.push_back(colour.rgb());
		}
		//and an additional for NA values
		const QColor black("black");
		vector.push_back(black.rgb());
	}
	void constructColourTableTheta(QVector<QRgb>& vector, const std::vector<double>& levels)
	{
		vector.clear();
		//NA value
		const QColor black("black");
		vector.resize(256);
		std::fill(vector.begin(), vector.end(), black.rgb());

		QColor colour;
		for(int counter = 0; counter < (int)levels.size(); counter++)
		{
			colour.setHsvF(levels[counter] * (1.0f/6.0f)/0.5, 1.0, 1.0);
			vector[counter] = colour.rgb();
		}
	}
}
