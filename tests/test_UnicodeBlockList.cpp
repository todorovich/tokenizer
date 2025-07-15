#include <catch2/catch_test_macros.hpp>

#include "../src/UnicodeBlockList.hpp"

#include <unordered_map>

inline std::string hex_cp(uint32_t cp) {
    std::ostringstream oss;
    oss << "0x" << std::hex << cp;
    return oss.str();
}

TEST_CASE("Unicode block ranges are disjoint (no overlaps, block names shown)") {
    std::unordered_map<uint32_t, const char*> cp_to_block;
    constexpr size_t count = std::size(unicode_blocks);
    for (size_t i = 0; i < count; ++i)
    {

        const auto& [name, start, end] = unicode_blocks[i];

        for (uint32_t cp = start; cp <= end; ++cp)
        {
            auto [iterator, success] = cp_to_block.emplace(cp, name);
            INFO("Unicode block overlap: code point "
                 << hex_cp(cp)
                 << " is in both '" << iterator->second
                 << "' and '" << name << "'");
            REQUIRE(success);
        }
    }
}
