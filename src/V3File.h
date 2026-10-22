// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3FILE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3FILE_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <fstream>
#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <stack>
#ಅನ್ತರ್ಭಾವ <vector>

class Ast_ಗಂಟು;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ std::ifstream* new_ifstream(ಸ್ಥಿರಾಂಕ string& filename) {
        ಕೂಡಿಸು_Src_Depend(filename);
        ಹಿಂತಿರುಗಿ new_ifstream_nodepend(filename);
    }
    ಸ್ಥಿರ std::ifstream* new_ifstream_nodepend(ಸ್ಥಿರಾಂಕ string& filename) VL_MT_ಸುರಕ್ಷಿತ {
        ಹಿಂತಿರುಗಿ new std::ifstream{filename.c_str()};
    }
    ಸ್ಥಿರ std::ofstream* new_ofstream(ಸ್ಥಿರಾಂಕ string& filename, bool append = false) {
        ಕೂಡಿಸು_Tgt_Depend(filename);
        ಹಿಂತಿರುಗಿ new_ofstream_nodepend(filename, append);
    }
    ಸ್ಥಿರ std::ofstream* new_ofstream_nodepend(ಸ್ಥಿರಾಂಕ string& filename, bool append = false) {
        createMakeDirFor(filename);
        ಆದರೆ (append) ಹಿಂತಿರುಗಿ new std::ofstream{filename.c_str(), std::ios::app};
        ಹಿಂತಿರುಗಿ new std::ofstream{filename.c_str()};
    }
    ಸ್ಥಿರ FILE* new_fopen_w(ಸ್ಥಿರಾಂಕ string& filename) {
        createMakeDirFor(filename);
        ಕೂಡಿಸು_Tgt_Depend(filename);
        ಹಿಂತಿರುಗಿ fopen(filename.c_str(), "w");
    }

    // Dependencies
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Src_Depend(ಸ್ಥಿರಾಂಕ string& filename) VL_MT_ಸುರಕ್ಷಿತ;
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Tgt_Depend(ಸ್ಥಿರಾಂಕ string& filename) VL_MT_ಸುರಕ್ಷಿತ;
    ಸ್ಥಿರ ಶೂನ್ಯ writeDepend(ಸ್ಥಿರಾಂಕ string& filename);
    ಸ್ಥಿರ std::ಸದಿಶ<string> getAllDeps();
    ಸ್ಥಿರ ಶೂನ್ಯ writeTimes(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& cmdline_ಒಳ);
    ಸ್ಥಿರ bool checkTimes(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& cmdline_ಒಳ);

    // Directory utilities
    ಸ್ಥಿರ ಶೂನ್ಯ createMakeDirFor(ಸ್ಥಿರಾಂಕ string& filename);
    ಸ್ಥಿರ ಶೂನ್ಯ createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using StrList = std::list<std::string>;

ಖಾಸಗಿ:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

ಸಾರ್ವಜನಿಕ:
    explicit VInFilter(ಸ್ಥಿರಾಂಕ string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ಸ್ಥಿರಾಂಕ string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3_ಹೊರಹೋಗು_Formatter VL_NOT_FINAL {
    // TYPES
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    ಎಣಿಕೆ Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

ಖಾಸಗಿ:
    // MEMBERS
    ಸ್ಥಿರಾಂಕ Language m_lang;  // Indenting Verilog code
    ಪೂರ್ಣಾಂಕ m_ಖಂಡ_Indent;  // Characters per block indent
    ಪೂರ್ಣಾಂಕ m_comma_ಅಗಲ;  // Width after which to break at ,'s
    ಪೂರ್ಣಾಂಕ m_lineno = 1;
    ಪೂರ್ಣಾಂಕ m_ಸ್ತಂಭ = 0;
    ಪೂರ್ಣಾಂಕ m_nobreak = false;  // Basic operator or begin paren, don't break next
    ಪೂರ್ಣಾಂಕ m_ಮೂಲ_ಕೊನೆಯ_Lineno = 0;
    ಪೂರ್ಣಾಂಕ m_ಮೂಲ_ಕೊನೆಯ_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_ಒಳ_String_Literal = false;  // Inside a quote
    bool m_ಒಳ_Backslash = false;  // Inside a backslash
    ಪೂರ್ಣಾಂಕ m_indentLevel = 0;  // Current {} indentation
    std::ರಾಶಿ<ಪೂರ್ಣಾಂಕ> m_parenVec;  // Stack of columns where last ( was
    ಪೂರ್ಣಾಂಕ m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    ಪೂರ್ಣಾಂಕ ಕೊನೆ_ಮಟ್ಟಗಳು(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg);
    ಶೂನ್ಯ putcNoTracking(ಅಕ್ಷರ chr);

ಸಾರ್ವಜನಿಕ:
    explicit V3_ಹೊರಹೋಗು_Formatter(Language lang);
    ವಾಸ್ತವಿಕ ~V3_ಹೊರಹೋಗು_Formatter() = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    ಶೂನ್ಯ ಖಂಡ_Indent(ಪೂರ್ಣಾಂಕ flag) { m_ಖಂಡ_Indent = flag; }
    // METHODS
    ಶೂನ್ಯ ಬರೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* fmt...) VL_ATTR_PRINTF(2);
    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg) { putns(nullptr, strg); }
    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ string& strg) { putns(nullptr, strg); }
    ಶೂನ್ಯ putns(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg);
    ಶೂನ್ಯ putns(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& strg) { putns(nodep, strg.c_str()); }
    ಶೂನ್ಯ putsNoTracking(ಸ್ಥಿರಾಂಕ string& strg);
    ಶೂನ್ಯ putsQuoted(ಸ್ಥಿರಾಂಕ string& strg);
    ಶೂನ್ಯ putBreak();  // Print linebreak if line is too wide
    ಶೂನ್ಯ put_Break_ಅಭಿವ್ಯಕ್ತಿ();  // Print linebreak in expression if line is too wide
    ಶೂನ್ಯ putbs(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg) {
        put_Break_ಅಭಿವ್ಯಕ್ತಿ();
        putns(nullptr, strg);
    }
    ಶೂನ್ಯ putbs(ಸ್ಥಿರಾಂಕ string& strg) {
        put_Break_ಅಭಿವ್ಯಕ್ತಿ();
        putns(nullptr, strg);
    }
    ಶೂನ್ಯ putnbs(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& strg) {
        put_Break_ಅಭಿವ್ಯಕ್ತಿ();
        putns(nodep, strg);
    }
    bool exceeded_ಅಗಲ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಸ್ತಂಭ > m_comma_ಅಗಲ; }
    ಶೂನ್ಯ indentInc() { m_indentLevel += m_ಖಂಡ_Indent; }
    ಶೂನ್ಯ indentDec() {
        m_indentLevel -= m_ಖಂಡ_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    ಶೂನ್ಯ ಖಂಡ_Inc() { m_parenVec.ತಳ್ಳು(m_indentLevel + m_ಖಂಡ_Indent); }
    ಶೂನ್ಯ ಖಂಡ_Dec() {
        ಆದರೆ (!m_parenVec.ಖಾಲಿಗಳು()) m_parenVec.ತೆಗೆ_2();
    }
    ಶೂನ್ಯ ensure_New_ಸಾಲು() {
        ಆದರೆ (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ std::string& indentSpaces(ಪೂರ್ಣಾಂಕ ಅಂಕೆ);
    // Add escaped characters to strings
    ಸ್ಥಿರ string quoteNameControls(ಸ್ಥಿರಾಂಕ string& namein, Language lang = LA_C) VL_ಶುದ್ಧ;
    ಸ್ಥಿರ bool token_ಹೊಂದಿಕೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cmp);
    ಸ್ಥಿರ bool token_Not_ಆರಂಭ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp);  // Import/export meaning no endfunction
    ಸ್ಥಿರ bool token_ಆರಂಭ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp);
    ಸ್ಥಿರ bool token_ಕೊನೆ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp);

    // CALLBACKS - MUST OVERRIDE
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ putcOutput(ಅಕ್ಷರ chr) = 0;
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ putsOutput(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_ಹೊರಹೋಗು_File VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ಹೊರಹೋಗು_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ಸ್ಥಿರ constexpr std::ಗಾತ್ರ_t WRITE_ಬಫರ್_ಗಾತ್ರ_ಬೈಟುಗಳು = 128 * 1024;

    // MEMBERS
    ಸ್ಥಿರಾಂಕ std::string m_filename;
    FILE* m_fp = nullptr;
    std::ಗಾತ್ರ_t m_ಬಳಸಿದ_ಬೈಟುಗಳು = 0;  // Number of bytes stored in m_bufferp
    std::ಗಾತ್ರ_t m_written_ಬೈಟುಗಳು = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<ಅಕ್ಷರ, WRITE_ಬಫರ್_ಗಾತ್ರ_ಬೈಟುಗಳು>> m_bufferp;  // Write buffer

ಸಾರ್ವಜನಿಕ:
    V3_ಹೊರಹೋಗು_File(ಸ್ಥಿರಾಂಕ string& filename, V3_ಹೊರಹೋಗು_Formatter::Language lang);
    V3_ಹೊರಹೋಗು_File(ಸ್ಥಿರಾಂಕ V3_ಹೊರಹೋಗು_File&) = delete;
    V3_ಹೊರಹೋಗು_File& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ V3_ಹೊರಹೋಗು_File&) = delete;
    V3_ಹೊರಹೋಗು_File(V3_ಹೊರಹೋಗು_File&&) = delete;
    V3_ಹೊರಹೋಗು_File& ಕ್ರಿಯಾಕಾರಕ=(V3_ಹೊರಹೋಗು_File&&) = delete;
    ~V3_ಹೊರಹೋಗು_File() override;

    std::string filename() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_filename; }

    ಶೂನ್ಯ putsForceIncs();

    ಶೂನ್ಯ stat_ದಾಖಲೆ_Written() {
        write_ಖಂಡ();
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ(V3Stats::STAT_CPP_CHARS, m_written_ಬೈಟುಗಳು);
    }

ಖಾಸಗಿ:
    ಶೂನ್ಯ write_ಖಂಡ() {
        ಆದರೆ (VL_LIKELY(m_ಬಳಸಿದ_ಬೈಟುಗಳು > 0)) {
            fwrite(m_bufferp->ದತ್ತಾಂಶ(), m_ಬಳಸಿದ_ಬೈಟುಗಳು, 1, m_fp);
            m_written_ಬೈಟುಗಳು += m_ಬಳಸಿದ_ಬೈಟುಗಳು;
            m_ಬಳಸಿದ_ಬೈಟುಗಳು = 0;
        }
    }
    // CALLBACKS
    ಶೂನ್ಯ putcOutput(ಅಕ್ಷರ chr) override {
        m_bufferp->at(m_ಬಳಸಿದ_ಬೈಟುಗಳು++) = chr;
        ಆದರೆ (VL_UNLIKELY(m_ಬಳಸಿದ_ಬೈಟುಗಳು >= WRITE_ಬಫರ್_ಗಾತ್ರ_ಬೈಟುಗಳು)) write_ಖಂಡ();
    }
    ಶೂನ್ಯ putsOutput(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* str) override {
        std::ಗಾತ್ರ_t len = ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(str);
        std::ಗಾತ್ರ_t ಲಭ್ಯ_ಬೈಟುಗಳು = WRITE_ಬಫರ್_ಗಾತ್ರ_ಬೈಟುಗಳು - m_ಬಳಸಿದ_ಬೈಟುಗಳು;
        ಆಗ (VL_UNLIKELY(len >= ಲಭ್ಯ_ಬೈಟುಗಳು)) {
            std::ಸ್ಮರಣೆ_ನಕಲು(m_bufferp->ದತ್ತಾಂಶ() + m_ಬಳಸಿದ_ಬೈಟುಗಳು, str, ಲಭ್ಯ_ಬೈಟುಗಳು);
            m_ಬಳಸಿದ_ಬೈಟುಗಳು = WRITE_ಬಫರ್_ಗಾತ್ರ_ಬೈಟುಗಳು;
            write_ಖಂಡ();
            str += ಲಭ್ಯ_ಬೈಟುಗಳು;
            len -= ಲಭ್ಯ_ಬೈಟುಗಳು;
            ಲಭ್ಯ_ಬೈಟುಗಳು = WRITE_ಬಫರ್_ಗಾತ್ರ_ಬೈಟುಗಳು;
        }
        ಆದರೆ (len > 0) {
            std::ಸ್ಮರಣೆ_ನಕಲು(m_bufferp->ದತ್ತಾಂಶ() + m_ಬಳಸಿದ_ಬೈಟುಗಳು, str, len);
            m_ಬಳಸಿದ_ಬೈಟುಗಳು += len;
        }
    }
};

class V3_ಹೊರಹೋಗು_CFile VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ಹೊರಹೋಗು_File {
    ಪೂರ್ಣಾಂಕ m_guard = false;  // Created header guard
    ಪೂರ್ಣಾಂಕ m_ಖಾಸಗಿ;  // 1 = Most recently emitted private:, 2 = public:
ಸಾರ್ವಜನಿಕ:
    explicit V3_ಹೊರಹೋಗು_CFile(ಸ್ಥಿರಾಂಕ string& filename,
                        V3_ಹೊರಹೋಗು_Formatter::Language lang = V3_ಹೊರಹೋಗು_Formatter::LA_C)
        : V3_ಹೊರಹೋಗು_File{filename, lang} {
        reset_ಖಾಸಗಿ();
    }
    ~V3_ಹೊರಹೋಗು_CFile() override { stat_ದಾಖಲೆ_Written(); }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ puts_ಶೀರ್ಷಿಕೆ() { puts("// Verilated -*- C++ -*-\n"); }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ puts_Int_ತುದಿ_Include() { putsForceIncs(); }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ putsGuard();
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ puts_ಕೊನೆ_Guard() {
        ಆದರೆ (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    ಶೂನ್ಯ reset_ಖಾಸಗಿ() { m_ಖಾಸಗಿ = 0; }
    ಶೂನ್ಯ puts_ಖಾಸಗಿ(bool ಗಣ_ಖಾಸಗಿ) {
        ಆದರೆ (ಗಣ_ಖಾಸಗಿ && m_ಖಾಸಗಿ != 1) {
            puts("private:\n");
            m_ಖಾಸಗಿ = 1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!ಗಣ_ಖಾಸಗಿ && m_ಖಾಸಗಿ != 2) {
            puts("public:\n");
            m_ಖಾಸಗಿ = 2;
        }
    }
};

class V3_ಹೊರಹೋಗು_Json_File final : ಸಾರ್ವಜನಿಕ V3_ಹೊರಹೋಗು_File {
    // CONSTANTS
    ಸ್ಥಿರ constexpr ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
ಖಾಸಗಿ:
    std::ರಾಶಿ<ಅಕ್ಷರ> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Prefix emitted before each line in current scope
    std::ರಾಶಿ<bool> m_ಖಾಲಿಗಳು;  // Current scope is empty, no comma later

ಸಾರ್ವಜನಿಕ:
    explicit V3_ಹೊರಹೋಗು_Json_File(ಸ್ಥಿರಾಂಕ string& filename)
        : V3_ಹೊರಹೋಗು_File{filename, V3_ಹೊರಹೋಗು_Formatter::LA_JSON} {
        begin();
    }
    ~V3_ಹೊರಹೋಗು_Json_File() override {
        ಕೊನೆ();
        puts("\n");
    }
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ puts_ಶೀರ್ಷಿಕೆ() {}
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_ಹೊರಹೋಗು_Json_File& begin(ಸ್ಥಿರಾಂಕ std::string& name, ಅಕ್ಷರ type = '{') {
        comma();
        puts(m_ಪೂರ್ವಪ್ರತ್ಯಯ + "\"" + name + "\": " + type + "\n");
        m_ಪೂರ್ವಪ್ರತ್ಯಯ += INDENT;
        m_scope.ತಳ್ಳು(type == '{' ? '}' : ']');
        ಆದರೆ (!m_ಖಾಲಿಗಳು.ಖಾಲಿಗಳು()) m_ಖಾಲಿಗಳು.ತುದಿ() = false;
        m_ಖಾಲಿಗಳು.ತಳ್ಳು(true);
        ಹಿಂತಿರುಗಿ *this;
    }
    V3_ಹೊರಹೋಗು_Json_File& begin(ಅಕ್ಷರ type = '{') {
        comma();
        puts(m_ಪೂರ್ವಪ್ರತ್ಯಯ + type + "\n");
        m_ಪೂರ್ವಪ್ರತ್ಯಯ += INDENT;
        m_scope.ತಳ್ಳು(type == '{' ? '}' : ']');
        ಆದರೆ (!m_ಖಾಲಿಗಳು.ಖಾಲಿಗಳು()) m_ಖಾಲಿಗಳು.ತುದಿ() = false;
        m_ಖಾಲಿಗಳು.ತಳ್ಳು(true);
        ಹಿಂತಿರುಗಿ *this;
    }

    // Put a named value
    V3_ಹೊರಹೋಗು_Json_File& put(ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valuep) {
        ಹಿಂತಿರುಗಿ putNamed(name, std::string{valuep}, true);
    }
    V3_ಹೊರಹೋಗು_Json_File& put(ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ std::string& ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ putNamed(name, ಮೌಲ್ಯ, true);
    }
    V3_ಹೊರಹೋಗು_Json_File& put(ಸ್ಥಿರಾಂಕ std::string& name, bool ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ putNamed(name, ಮೌಲ್ಯ ? "true" : "false", false);
    }
    V3_ಹೊರಹೋಗು_Json_File& put(ಸ್ಥಿರಾಂಕ std::string& name, ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) {
        ಹಿಂತಿರುಗಿ putNamed(name, std::ಗಮ್ಯ_string(ಮೌಲ್ಯ), false);
    }

    // Put unnamed value
    V3_ಹೊರಹೋಗು_Json_File& put(ಸ್ಥಿರಾಂಕ std::string& ಮೌಲ್ಯ) { ಹಿಂತಿರುಗಿ putNamed("", ಮೌಲ್ಯ, true); }
    V3_ಹೊರಹೋಗು_Json_File& put(bool ಮೌಲ್ಯ) { ಹಿಂತಿರುಗಿ putNamed("", ಮೌಲ್ಯ ? "true" : "false", false); }
    V3_ಹೊರಹೋಗು_Json_File& put(ಪೂರ್ಣಾಂಕ ಮೌಲ್ಯ) { ಹಿಂತಿರುಗಿ putNamed("", std::ಗಮ್ಯ_string(ಮೌಲ್ಯ), false); }

    V3_ಹೊರಹೋಗು_Json_File& putList(ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<std::string>& list) {
        ಆದರೆ (list.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ *this;
        begin(name, '[');
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ std::string& str : list) put(str);
        ಹಿಂತಿರುಗಿ ಕೊನೆ();
    }

    V3_ಹೊರಹೋಗು_Json_File& ಕೊನೆ() {
        UASSERT(m_ಪೂರ್ವಪ್ರತ್ಯಯ.ಉದ್ದಳತೆ() >= ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(INDENT), "prefix underflow");
        m_ಪೂರ್ವಪ್ರತ್ಯಯ.erase(m_ಪೂರ್ವಪ್ರತ್ಯಯ.ಕೊನೆ() - ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(INDENT), m_ಪೂರ್ವಪ್ರತ್ಯಯ.ಕೊನೆ());
        UASSERT(!m_scope.ಖಾಲಿಗಳು(), "end() without begin()");
        UASSERT(!m_ಖಾಲಿಗಳು.ಖಾಲಿಗಳು(), "end() without begin()");
        puts("\n" + m_ಪೂರ್ವಪ್ರತ್ಯಯ + m_scope.ತುದಿ());
        m_scope.ತೆಗೆ_2();
        m_ಖಾಲಿಗಳು.ತೆಗೆ_2();
        ಹಿಂತಿರುಗಿ *this;
    }

    V3_ಹೊರಹೋಗು_Json_File& ಕ್ರಿಯಾಕಾರಕ+=(V3_ಹೊರಹೋಗು_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        ಹಿಂತಿರುಗಿ *this;
    }

ಖಾಸಗಿ:
    ಶೂನ್ಯ comma() {
        ಆದರೆ (m_ಖಾಲಿಗಳು.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!m_ಖಾಲಿಗಳು.ತುದಿ()) puts(",\n");
        m_ಖಾಲಿಗಳು.ತುದಿ() = true;
    }
    V3_ಹೊರಹೋಗು_Json_File& putNamed(ಸ್ಥಿರಾಂಕ std::string& name, ಸ್ಥಿರಾಂಕ std::string& ಮೌಲ್ಯ, bool quoted) {
        comma();
        ಸ್ಥಿರಾಂಕ string ಮೌಲ್ಯ_Q
            = quoted ? "\""s + V3_ಹೊರಹೋಗು_Formatter::quoteNameControls(ಮೌಲ್ಯ) + "\"" : ಮೌಲ್ಯ;
        ಆದರೆ (name.ಖಾಲಿಗಳು()) {
            puts(m_ಪೂರ್ವಪ್ರತ್ಯಯ + ಮೌಲ್ಯ_Q);
        } ಇಲ್ಲದಿದ್ದರೆ {
            puts(m_ಪೂರ್ವಪ್ರತ್ಯಯ + "\"" + name + "\": " + ಮೌಲ್ಯ_Q);
        }
        m_ಖಾಲಿಗಳು.ತುದಿ() = false;
        ಹಿಂತಿರುಗಿ *this;
    }
};

class V3_ಹೊರಹೋಗು_Mk_File final : ಸಾರ್ವಜನಿಕ V3_ಹೊರಹೋಗು_File {
ಸಾರ್ವಜನಿಕ:
    explicit V3_ಹೊರಹೋಗು_Mk_File(ಸ್ಥಿರಾಂಕ string& filename)
        : V3_ಹೊರಹೋಗು_File{filename, V3_ಹೊರಹೋಗು_Formatter::LA_MK} {}
    ~V3_ಹೊರಹೋಗು_Mk_File() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ puts_ಶೀರ್ಷಿಕೆ() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    ಶೂನ್ಯ put_ಗಣ(ಸ್ಥಿರಾಂಕ string& var, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) {
        puts(VString::dot(var + " =", " ", ಮೌಲ್ಯ) + "\n");
    }
    // Put VARIABLE ?= VALUE
    ಶೂನ್ಯ put_ಗಣ_Q(ಸ್ಥಿರಾಂಕ string& var, ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ) {
        puts(VString::dot(var + " ?=", " ", ಮೌಲ್ಯ) + "\n");
    }
};

class V3_ಹೊರಹೋಗು_VFile final : ಸಾರ್ವಜನಿಕ V3_ಹೊರಹೋಗು_CFile {
ಸಾರ್ವಜನಿಕ:
    explicit V3_ಹೊರಹೋಗು_VFile(ಸ್ಥಿರಾಂಕ string& filename)
        : V3_ಹೊರಹೋಗು_CFile{filename, V3_ಹೊರಹೋಗು_Formatter::LA_VERILOG} {}
    ~V3_ಹೊರಹೋಗು_VFile() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ puts_ಶೀರ್ಷಿಕೆ() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3_ಹೊರಹೋಗು_Xml_File final : ಸಾರ್ವಜನಿಕ V3_ಹೊರಹೋಗು_File {
ಸಾರ್ವಜನಿಕ:
    explicit V3_ಹೊರಹೋಗು_Xml_File(ಸ್ಥಿರಾಂಕ string& filename)
        : V3_ಹೊರಹೋಗು_File{filename, V3_ಹೊರಹೋಗು_Formatter::LA_XML} {
        ಖಂಡ_Indent(2);
    }
    ~V3_ಹೊರಹೋಗು_Xml_File() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ puts_ಶೀರ್ಷಿಕೆ() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_ಹೊರಹೋಗು_ಹರಿವು VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ಹೊರಹೋಗು_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_ಹೊರಹೋಗು_ಹರಿವು);
    VL_UNMOVABLE(V3_ಹೊರಹೋಗು_ಹರಿವು);

ಸಾರ್ವಜನಿಕ:
    V3_ಹೊರಹೋಗು_ಹರಿವು(std::ostream& ostream, V3_ಹೊರಹೋಗು_Formatter::Language lang);
    ~V3_ಹೊರಹೋಗು_ಹರಿವು() override = ಪೂರ್ವನಿಯೋಜಿತ;

    ಶೂನ್ಯ putcOutput(ಅಕ್ಷರ chr) override { m_ostream << chr; };
    ಶೂನ್ಯ putsOutput(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
ಸಾರ್ವಜನಿಕ:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    ಸ್ಥಿರ string ifNoProtect(ಸ್ಥಿರಾಂಕ string& ಒಳ) VL_MT_ಸುರಕ್ಷಿತ;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ಸ್ಥಿರ string protect(ಸ್ಥಿರಾಂಕ string& old) VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ protectIf(old, true); }
    ಸ್ಥಿರ string protectIf(ಸ್ಥಿರಾಂಕ string& old, bool doIt = true) VL_MT_ಸುರಕ್ಷಿತ;
    // Rename words to a new encoded string
    ಸ್ಥಿರ string protect_ಪದಗಳು_If(ಸ್ಥಿರಾಂಕ string& old, bool doIt = true) VL_MT_ಸುರಕ್ಷಿತ;
    // Write map of renames to output file
    ಸ್ಥಿರ ಶೂನ್ಯ writeMapFile(ಸ್ಥಿರಾಂಕ string& filename);
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
