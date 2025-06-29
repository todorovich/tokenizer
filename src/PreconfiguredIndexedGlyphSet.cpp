#include "PreconfiguredIndexedGlyphSet.hpp"

std::unordered_map<std::string, IndexedGlyphSet> PreconfiguredIndexedGlyphSet::unicode_blocks_glyph_set =
    buildIndexedGlyphSetsFromBlocks();