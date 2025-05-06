// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#pragma once

#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

/**
 * @file Common.h
 * @brief Common type definitions and version information for SIDBlaster
 *
 * Provides consistent type aliases and version information used throughout
 * the SIDBlaster codebase.
 */

 // Tool name and version
#define SIDBLASTER_VERSION "SIDBlaster 0.7.0"

// Type aliases for consistent usage across the project
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;