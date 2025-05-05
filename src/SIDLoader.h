#pragma once

#include "Common.h"
#include "SIDFileFormat.h"

#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// Forward declarations
class CPU6510;

/**
 * @class SIDLoader
 * @brief Handles loading and processing SID music files for the C64
 *
 * This class can load SID, PRG, and BIN files containing C64 music data,
 * and manages loading them into a CPU6510 emulator for playback and analysis.
 */
class SIDLoader {
public:
    /**
     * @brief Construct a new SIDLoader
     */
    SIDLoader();

    /**
     * @brief Set the CPU instance to use for loading music data
     * @param cpu Pointer to a CPU6510 instance
     */
    void setCPU(CPU6510* cpu);

    /**
     * @brief Override the init address in the SID header
     * @param address New init address
     */
    void setInitAddress(u16 address);

    /**
     * @brief Override the play address in the SID header
     * @param address New play address
     */
    void setPlayAddress(u16 address);

    /**
     * @brief Override the load address in the SID header
     * @param address New load address
     */
    void setLoadAddress(u16 address);

    /**
     * @brief Set the title in the SID header
     * @param title New title string
     */
    void setTitle(const std::string& title) {
        strncpy(header_.name, title.c_str(), sizeof(header_.name) - 1);
        header_.name[sizeof(header_.name) - 1] = '\0';
    }

    /**
     * @brief Set the author in the SID header
     * @param author New author string
     */
    void setAuthor(const std::string& author) {
        strncpy(header_.author, author.c_str(), sizeof(header_.author) - 1);
        header_.author[sizeof(header_.author) - 1] = '\0';
    }

    /**
     * @brief Set the copyright in the SID header
     * @param copyright New copyright string
     */
    void setCopyright(const std::string& copyright) {
        strncpy(header_.copyright, copyright.c_str(), sizeof(header_.copyright) - 1);
        header_.copyright[sizeof(header_.copyright) - 1] = '\0';
    }

    /**
     * @brief Load a SID file
     * @param filename Path to the SID file
     * @return true if loading succeeded, false otherwise
     */
    bool loadSID(const std::string& filename);

    /**
     * @brief Load a raw binary file
     * @param filename Path to the BIN file
     * @param loadAddr Memory address where the data should be loaded
     * @param initAddr Address of initialization routine
     * @param playAddr Address of play routine
     * @return true if loading succeeded, false otherwise
     */
    bool loadBIN(const std::string& filename, u16 loadAddr, u16 initAddr, u16 playAddr);

    /**
     * @brief Load a PRG file (C64 program file with load address header)
     * @param filename Path to the PRG file
     * @param initAddr Address of initialization routine
     * @param playAddr Address of play routine
     * @return true if loading succeeded, false otherwise
     */
    bool loadPRG(const std::string& filename, u16 initAddr, u16 playAddr);

    // Accessor methods
    u16 getInitAddress() const { return header_.initAddress; }
    u16 getPlayAddress() const { return header_.playAddress; }
    u16 getLoadAddress() const { return header_.loadAddress; }
    u16 getDataSize() const { return dataSize_; }
    const SIDHeader& getHeader() const { return header_; }

    const std::vector<u8>& getOriginalMemory() const { return originalMemory_; }
    u16 getOriginalMemoryBase() const { return originalMemoryBase_; }

    /**
     * @brief Check if the SID file is for PAL or NTSC
     * @return true for PAL, false for NTSC
     */
    bool isPAL() const;

    /**
     * @brief Get the number of times the play routine should be called per frame
     * @return Number of calls per frame
     */
    int getNumPlayCallsPerFrame() const { return numPlayCallsPerFrame_; }

    /**
     * @brief Set the number of times the play routine should be called per frame
     * @param num Number of calls per frame
     */
    void setNumPlayCallsPerFrame(int num) { numPlayCallsPerFrame_ = num; }

    /**
     * @brief Backup the current memory to allow restoration later
     * @return True if backup succeeded
     */
    bool backupMemory();

    /**
     * @brief Restore memory from backup
     * @return True if restoration succeeded
     */
    bool restoreMemory();

private:
    /**
     * @brief Create a SID header for non-SID format files
     * @param loadAddr Memory load address
     * @param initAddr Initialization routine address
     * @param playAddr Play routine address
     * @return true if header creation succeeded
     */
    bool createSIDHeader(u16 loadAddr, u16 initAddr, u16 playAddr);

    /**
     * @brief Copy music data to CPU memory
     * @param data Pointer to the music data
     * @param size Size of the data in bytes
     * @param loadAddr Memory address to load the data
     * @return true if copying succeeded
     */
    bool copyMusicToMemory(const u8* data, u16 size, u16 loadAddr);

    /**
     * @brief Fix SID header endianness (SID files use big-endian)
     * @param header Reference to the header to fix
     */
    void fixHeaderEndianness(SIDHeader& header);

    // Add to SIDLoader.h:

    // Member variables
    SIDHeader header_;
    u16 dataSize_ = 0;
    CPU6510* cpu_ = nullptr;

    // Keep a copy of the original loaded data
    std::vector<u8> originalMemory_;
    u16 originalMemoryBase_ = 0;

    // Playback configuration
    u8 numPlayCallsPerFrame_ = 1;

    // Backup of memory for analysis pass
    std::vector<u8> memoryBackup_;
};