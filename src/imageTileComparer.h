#ifndef IMAGE_TILE_COMPARER_HEADER_GUARD
#define IMAGE_TILE_COMPARER_HEADER_GUARD
namespace mpMapInteractive
{
	class imageTileWithAux;
	struct imageTileComparer
	{
		bool operator()(const imageTileWithAux& first, const imageTileWithAux& second) const;
	};
}
#endif
