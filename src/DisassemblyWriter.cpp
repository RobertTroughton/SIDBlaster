// ==================================
//             SIDBlaster
//
//  Raistlin / Genesis Project (G*P)
// ==================================
#include "DisassemblyWriter.h"
#include "SIDLoader.h"
#include "cpu6510.h"

#include <algorithm>
#include <iostream>
#include <set>

namespace sidblaster {

    /**
     * @brief Constructor for DisassemblyWriter
     *
     * Initializes the disassembly writer with references to the CPU, SID loader,
     * memory analyzer, label generator, and code formatter.
     *
     * @param cpu Reference to the CPU
     * @param sid Reference to the SID loader
     * @param analyzer Reference to the memory analyzer
     * @param labelGenerator Reference to the label generator
     * @param formatter Reference to the code formatter
     */
    DisassemblyWriter::DisassemblyWriter(
        const CPU6510& cpu,
        const SIDLoader& sid,
        const MemoryAnalyzer& analyzer,
        const LabelGenerator& labelGenerator,
        const CodeFormatter& formatter)
        : cpu_(cpu),
        sid_(sid),
        analyzer_(analyzer),
        labelGenerator_(labelGenerator),
        formatter_(formatter) {
        // No longer need to check for includePlayer_ configuration
    }

    /**
     * @brief Generate an assembly file
     *
     * Creates a complete assembly language file for the disassembled SID,
     * including header comments, constants, and code.
     *
     * @param filename Output filename
     * @param sidLoad New SID load address
     * @param sidInit New SID init address
     * @param sidPlay New SID play address
     * @return Number of unused bytes removed
     */
    int DisassemblyWriter::generateAsmFile(
        const std::string& filename,
        u16 sidLoad,
        u16 sidInit,
        u16 sidPlay) {

        util::Logger::info("Generating assembly file: " + filename);

        // Propagate relocation sources
        propagateRelocationSources();

        // Open the output file
        std::ofstream file(filename);
        if (!file) {
            util::Logger::error("Failed to open output file: " + filename);
            return 0;
        }

        // Write file header
        file << "//; ------------------------------------------\n";
        file << "//; Generated by " << SIDBLASTER_VERSION << "\n";
        file << "//; \n";
        file << "//; Name: " << sid_.getHeader().name << "\n";
        file << "//; Author: " << sid_.getHeader().author << "\n";
        file << "//; Copyright: " << sid_.getHeader().copyright << "\n";
        file << "//; ------------------------------------------\n\n";

        // Output addresses as constants
        file << ".const SIDLoad = $" << util::wordToHex(sidLoad) << "\n";

        // Output hardware registers as constants
        outputHardwareConstants(file);

        // Output zero page defines
        emitZPDefines(file);

        // Disassemble to file
        int unusedByteCount = disassembleToFile(file);

        // Output unused byte count
        file << "//; " << unusedByteCount << " unused bytes zeroed out\n\n";

        file.close();

        return unusedByteCount;
    }

    /**
     * @brief Add a relocation byte
     *
     * Registers a byte as a relocation point (address reference).
     *
     * @param address Address of the byte
     * @param info Relocation information
     */
    void DisassemblyWriter::addRelocationByte(
        u16 address,
        const RelocationInfo& info) {

        relocationBytes_[address] = info;
    }

    /**
     * @brief Add an indirect memory access
     *
     * Records information about an indirect memory access for later analysis.
     * This helps identify address references and pointer tables.
     *
     * @param pc Program counter
     * @param zpAddr Zero page address
     * @param effectiveAddr Effective address
     */
    void DisassemblyWriter::addIndirectAccess(u16 pc, u8 zpAddr, u16 effectiveAddr) {
        // Get the sources of the ZP variables
        const auto& lowSource = cpu_.getWriteSourceInfo(zpAddr);
        const auto& highSource = cpu_.getWriteSourceInfo(zpAddr + 1);

        // Only process if both low and high bytes were loaded from memory
        if (lowSource.type == RegisterSourceInfo::SourceType::Memory &&
            highSource.type == RegisterSourceInfo::SourceType::Memory) {

            IndirectAccessInfo info;
            info.instructionAddress = pc;
            info.zpAddr = zpAddr;
            info.zpPairAddr = zpAddr + 1;
            info.lastWriteLow = cpu_.getLastWriteTo(zpAddr);
            info.lastWriteHigh = cpu_.getLastWriteTo(zpAddr + 1);
            info.sourceLowAddress = lowSource.address;
            info.sourceHighAddress = highSource.address;
            info.effectiveAddress = effectiveAddr;

            indirectAccesses_.push_back(info);

            util::Logger::debug("Recorded indirect access at $" +
                util::wordToHex(pc) +
                " through ZP $" + util::byteToHex(zpAddr) +
                "/$" + util::byteToHex(zpAddr + 1) +
                " pointing to $" + util::wordToHex(effectiveAddr));
        }
    }

    /**
     * @brief Output hardware constants to the assembly file
     *
     * Identifies hardware components (like SID chips) that are accessed
     * in the code and generates appropriate constant definitions.
     *
     * @param file Output stream
     */
    void DisassemblyWriter::outputHardwareConstants(std::ofstream& file) {
        // Find all accessed hardware components

        // SID detection
        std::set<u16> sidBases;
        for (u16 addr = 0xD400; addr <= 0xD7FF; addr++) {
            if (analyzer_.getMemoryType(addr) & (MemoryType::Accessed)) {
                u16 base = addr & 0xFFE0; // Align to 32 bytes for SID
                sidBases.insert(base);
            }
        }

        // Ensure at least one SID is always present
        if (sidBases.empty()) {
            sidBases.insert(0xD400);
        }

        // Register SID bases
        int sidIndex = 0;
        for (u16 base : sidBases) {
            const std::string name = "SID" + std::to_string(sidIndex);

            // Register with label generator
            const_cast<LabelGenerator&>(labelGenerator_).addHardwareBase(
                HardwareType::SID, base, sidIndex, name);

            // Output to assembly file
            file << ".const " << name << " = $" << util::wordToHex(base) << "\n";

            sidIndex++;
        }

        // Future hardware component detection can be added here:
        // VIC-II detection (0xD000-0xD3FF)
        // CIA detection (0xDC00-0xDCFF for CIA1, 0xDD00-0xDDFF for CIA2)
        // etc.

        file << "\n";
    }

    /**
     * @brief Output zero page definitions to the assembly file
     *
     * Identifies zero page variables used by the code and generates
     * appropriate constant definitions.
     *
     * @param file Output stream
     */
    void DisassemblyWriter::emitZPDefines(std::ofstream& file) {
        // Collect all used zero page addresses
        std::set<u8> usedZP;
        for (u16 addr = 0x0000; addr <= 0x00FF; ++addr) {
            if (analyzer_.getMemoryType(addr) & MemoryType::Accessed) {
                usedZP.insert(static_cast<u8>(addr));
            }
        }

        if (usedZP.empty()) {
            return;
        }

        // Convert to vector for sorting
        std::vector<u8> zpList(usedZP.begin(), usedZP.end());
        std::sort(zpList.begin(), zpList.end());

        // Calculate ZP base
        u8 zpBase = 0xFF - static_cast<u8>(zpList.size()) + 1;

        // Output ZP defines
        file << ".const ZP_BASE = $" << util::byteToHex(zpBase) << "\n";
        for (size_t i = 0; i < zpList.size(); ++i) {
            std::string varName = "ZP_" + std::to_string(i);
            file << ".const " << varName << " = ZP_BASE + " << i << " // $" << util::byteToHex(zpList[i]) << "\n";

            // Add to label generator
            const_cast<LabelGenerator&>(labelGenerator_).addZeroPageVar(zpList[i], varName);
        }

        file << "\n";
    }

    /**
     * @brief Disassemble to the output file
     *
     * Performs the actual disassembly writing to the file, handling code,
     * data, and labels appropriately.
     *
     * @param file Output stream
     * @return Number of unused bytes removed
     */
    int DisassemblyWriter::disassembleToFile(std::ofstream& file) {
        u16 pc = sid_.getLoadAddress();
        file << "\n* = SIDLoad\n\n";

        const u16 sidEnd = sid_.getLoadAddress() + sid_.getDataSize();
        int unusedByteCount = 0;

        while (pc < sidEnd) {
            // Check if we need to output a label
            const std::string label = labelGenerator_.getLabel(pc);
            if (!label.empty() && (analyzer_.getMemoryType(pc) & MemoryType::Code)) {
                file << label << ":\n";
            }

            // Check if this is code or data
            if (analyzer_.getMemoryType(pc) & MemoryType::Code) {
                const u16 startPc = pc;
                const std::string line = formatter_.formatInstruction(pc);

                file << util::padToColumn(line, 96);
                file << " //; $" << util::wordToHex(startPc) << " - "
                    << util::wordToHex(pc - 1) << "\n";
            }
            else if (analyzer_.getMemoryType(pc) & MemoryType::Data) {
                // Convert RelocationInfo to map of RelocEntry format expected by formatter
                std::map<u16, struct RelocEntry> formatterRelocBytes;
                for (const auto& [addr, info] : relocationBytes_) {
                    RelocEntry entry;
                    entry.effectiveAddr = info.effectiveAddr;
                    entry.type = (info.type == RelocationInfo::Type::Low) ?
                        RelocEntry::Type::Low : RelocEntry::Type::High;
                    formatterRelocBytes[addr] = entry;
                }

                // Format data bytes
                unusedByteCount += formatter_.formatDataBytes(
                    file,
                    pc,
                    sid_.getOriginalMemory(),
                    sid_.getOriginalMemoryBase(),
                    sidEnd,
                    formatterRelocBytes,
                    analyzer_.getMemoryTypes());
            }
            else {
                // Unknown memory type, just increment PC
                ++pc;
            }
        }

        return unusedByteCount;
    }

    /**
     * @brief Propagate relocation sources
     *
     * Analyzes and propagates relocation information across the disassembly
     * to ensure consistent address references. This helps identify pointer
     * tables and other address references in the code.
     */
    void DisassemblyWriter::propagateRelocationSources() {
        util::Logger::debug("Propagating relocation sources...");

        bool changed = true;
        int pass = 0;
        const int maxPasses = 10;

        while (changed && pass++ < maxPasses) {
            changed = false;

            // Work on copy to avoid modifying during iteration
            auto currentEntries = relocationBytes_;

            for (const auto& [addr, entry] : currentEntries) {
                const auto& source = cpu_.getWriteSourceInfo(addr);
                if (source.type != RegisterSourceInfo::SourceType::Memory) {
                    continue;
                }

                // Recalculate effective address using original memory
                const auto& original = sid_.getOriginalMemory();
                const u16 base = sid_.getOriginalMemoryBase();

                if (entry.type == RelocationInfo::Type::Low) {
                    const u16 loAddr = source.address;
                    u16 hiAddr = 0;
                    u8 lo = 0, hi = 0;

                    // Attempt to find matching high byte for the original pair
                    // Try several common patterns: +1, +2, +3 (for pointer tables)
                    for (int offset = 1; offset <= 8; ++offset) {
                        auto hiIt = relocationBytes_.find(addr + offset);
                        if (hiIt != relocationBytes_.end() &&
                            hiIt->second.type == RelocationInfo::Type::High) {
                            hiAddr = cpu_.getWriteSourceInfo(addr + offset).address;

                            if (hiAddr >= base && hiAddr < base + original.size() &&
                                loAddr >= base && loAddr < base + original.size()) {

                                lo = original[loAddr - base];
                                hi = original[hiAddr - base];
                                const u16 newEffective = lo | (hi << 8);

                                if (relocationBytes_.count(loAddr) == 0) {
                                    relocationBytes_[loAddr] = {
                                        newEffective,
                                        RelocationInfo::Type::Low
                                    };
                                    changed = true;
                                    util::Logger::debug("Propagated relocation: $" +
                                        util::wordToHex(loAddr) +
                                        " (lo) for address $" +
                                        util::wordToHex(newEffective));

                                    // Mark for subdivision
                                    const_cast<LabelGenerator&>(labelGenerator_).addPendingSubdivisionAddress(loAddr);
                                }

                                if (relocationBytes_.count(hiAddr) == 0) {
                                    relocationBytes_[hiAddr] = {
                                        newEffective,
                                        RelocationInfo::Type::High
                                    };
                                    changed = true;
                                    util::Logger::debug("Propagated relocation: $" +
                                        util::wordToHex(hiAddr) +
                                        " (hi) for address $" +
                                        util::wordToHex(newEffective));

                                    // Mark for subdivision
                                    const_cast<LabelGenerator&>(labelGenerator_).addPendingSubdivisionAddress(hiAddr);
                                }

                                break;  // Found a match, no need to check other offsets
                            }
                        }
                    }
                }
            }
        }

        util::Logger::debug("Propagation complete, found " +
            std::to_string(relocationBytes_.size()) +
            " relocation bytes");
    }

    /**
     * @brief Process all recorded indirect accesses
     *
     * Analyzes indirect memory access patterns to identify address references
     * and pointer tables. This enhances the quality of the disassembly by
     * properly labeling and formatting these references.
     */
    void DisassemblyWriter::processIndirectAccesses() {
        if (indirectAccesses_.empty()) {
            util::Logger::debug("No indirect accesses to process");
            return;
        }

        util::Logger::debug("Processing " + std::to_string(indirectAccesses_.size()) + " indirect accesses");

        for (const auto& access : indirectAccesses_) {
            // Get the original byte values from the SID to determine the effective address
            const auto& original = sid_.getOriginalMemory();
            const u16 base = sid_.getOriginalMemoryBase();

            // Skip if source addresses are outside the original memory
            if (access.sourceLowAddress < base ||
                access.sourceLowAddress >= base + original.size() ||
                access.sourceHighAddress < base ||
                access.sourceHighAddress >= base + original.size()) {
                continue;
            }

            // Get the low and high bytes from the original memory
            const u8 lo = original[access.sourceLowAddress - base];
            const u8 hi = original[access.sourceHighAddress - base];

            // Reconstruct the effective address from the original bytes
            const u16 originalEffectiveAddr = lo | (hi << 8);

            // Add relocation entries for the low and high bytes
            relocationBytes_[access.sourceLowAddress] = {
                originalEffectiveAddr,
                RelocationInfo::Type::Low
            };

            relocationBytes_[access.sourceHighAddress] = {
                originalEffectiveAddr,
                RelocationInfo::Type::High
            };

            util::Logger::debug("Added relocation: $" +
                util::wordToHex(access.sourceLowAddress) +
                " (lo) and $" + util::wordToHex(access.sourceHighAddress) +
                " (hi) for address $" + util::wordToHex(originalEffectiveAddr));

            // Propagate relocations by analyzing the memory that contains these pointers
            const auto sidStart = sid_.getLoadAddress();
            if (access.sourceLowAddress >= sidStart &&
                access.sourceHighAddress >= sidStart) {

                // Mark these bytes for data block subdivision
                const_cast<LabelGenerator&>(labelGenerator_).addPendingSubdivisionAddress(access.sourceLowAddress);
                const_cast<LabelGenerator&>(labelGenerator_).addPendingSubdivisionAddress(access.sourceHighAddress);
            }
        }

        // After processing all indirect accesses, propagate relocation info
        propagateRelocationSources();
    }

} // namespace sidblaster