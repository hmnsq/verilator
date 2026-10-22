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

#ifndef VERILATOR_V3FILE_H_
#define VERILATOR_V3FILE_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Stats.h"

#अन्तर्भाव <array>
#अन्तर्भाव <fstream>
#अन्तर्भाव <list>
#अन्तर्भाव <memory>
#अन्तर्भाव <set>
#अन्तर्भाव <stack>
#अन्तर्भाव <vector>

class Ast_ग्रन्थिः;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
सार्वजनिक:
    स्थिर std::ifstream* new_ifstream(ध्रुव string& filename) {
        addSrcDepend(filename);
        प्रत्यागम new_ifstream_nodepend(filename);
    }
    स्थिर std::ifstream* new_ifstream_nodepend(ध्रुव string& filename) VL_MT_SAFE {
        प्रत्यागम new std::ifstream{filename.c_str()};
    }
    स्थिर std::ofstream* new_ofstream(ध्रुव string& filename, bool append = false) {
        addTgtDepend(filename);
        प्रत्यागम new_ofstream_nodepend(filename, append);
    }
    स्थिर std::ofstream* new_ofstream_nodepend(ध्रुव string& filename, bool append = false) {
        createMakeDirFor(filename);
        जइ (append) प्रत्यागम new std::ofstream{filename.c_str(), std::ios::app};
        प्रत्यागम new std::ofstream{filename.c_str()};
    }
    स्थिर FILE* new_fopen_w(ध्रुव string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        प्रत्यागम fopen(filename.c_str(), "w");
    }

    // Dependencies
    स्थिर शून्य addSrcDepend(ध्रुव string& filename) VL_MT_SAFE;
    स्थिर शून्य addTgtDepend(ध्रुव string& filename) VL_MT_SAFE;
    स्थिर शून्य writeDepend(ध्रुव string& filename);
    स्थिर std::vector<string> getAllDeps();
    स्थिर शून्य writeTimes(ध्रुव string& filename, ध्रुव string& cmdlineIn);
    स्थिर bool checkTimes(ध्रुव string& filename, ध्रुव string& cmdlineIn);

    // Directory utilities
    स्थिर शून्य createMakeDirFor(ध्रुव string& filename);
    स्थिर शून्य createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
सार्वजनिक:
    // TYPES
    using StrList = std::list<std::string>;

गुप्त:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

सार्वजनिक:
    explicit VInFilter(ध्रुव string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ध्रुव string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
सार्वजनिक:
    गणना AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    गणना Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

गुप्त:
    // MEMBERS
    ध्रुव Language m_lang;  // Indenting Verilog code
    पूर्णाङ्क m_blockIndent;  // Characters per block indent
    पूर्णाङ्क m_commaWidth;  // Width after which to break at ,'s
    पूर्णाङ्क m_lineno = 1;
    पूर्णाङ्क m_column = 0;
    पूर्णाङ्क m_nobreak = false;  // Basic operator or begin paren, don't break next
    पूर्णाङ्क m_sourceLastLineno = 0;
    पूर्णाङ्क m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    पूर्णाङ्क m_indentLevel = 0;  // Current {} indentation
    std::राशिः<पूर्णाङ्क> m_parenVec;  // Stack of columns where last ( was
    पूर्णाङ्क m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    पूर्णाङ्क endLevels(ध्रुव वर्ण* strg);
    शून्य putcNoTracking(वर्ण chr);

सार्वजनिक:
    explicit V3OutFormatter(Language lang);
    आभास ~V3OutFormatter() = स्वभाव;
    // ACCESSORS
    शून्य blockIndent(पूर्णाङ्क flag) { m_blockIndent = flag; }
    // METHODS
    शून्य printf(ध्रुव वर्ण* fmt...) VL_ATTR_PRINTF(2);
    शून्य puts(ध्रुव वर्ण* strg) { putns(nullptr, strg); }
    शून्य puts(ध्रुव string& strg) { putns(nullptr, strg); }
    शून्य putns(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* strg);
    शून्य putns(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& strg) { putns(nodep, strg.c_str()); }
    शून्य putsNoTracking(ध्रुव string& strg);
    शून्य putsQuoted(ध्रुव string& strg);
    शून्य putBreak();  // Print linebreak if line is too wide
    शून्य put_Break_व्यञ्जकम्();  // Print linebreak in expression if line is too wide
    शून्य putbs(ध्रुव वर्ण* strg) {
        put_Break_व्यञ्जकम्();
        putns(nullptr, strg);
    }
    शून्य putbs(ध्रुव string& strg) {
        put_Break_व्यञ्जकम्();
        putns(nullptr, strg);
    }
    शून्य putnbs(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& strg) {
        put_Break_व्यञ्जकम्();
        putns(nodep, strg);
    }
    bool exceededWidth() ध्रुव { प्रत्यागम m_column > m_commaWidth; }
    शून्य indentInc() { m_indentLevel += m_blockIndent; }
    शून्य indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    शून्य blockInc() { m_parenVec.प्रक्षिप(m_indentLevel + m_blockIndent); }
    शून्य blockDec() {
        जइ (!m_parenVec.empty()) m_parenVec.निष्कासय();
    }
    शून्य ensure_New_पङ्क्तिः() {
        जइ (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    स्थिर ध्रुव std::string& indentSpaces(पूर्णाङ्क अङ्कः);
    // Add escaped characters to strings
    स्थिर string quoteNameControls(ध्रुव string& namein, Language lang = LA_C) VL_PURE;
    स्थिर bool tokenMatch(ध्रुव वर्ण* cp, ध्रुव वर्ण* cmp);
    स्थिर bool tokenNotStart(ध्रुव वर्ण* cp);  // Import/export meaning no endfunction
    स्थिर bool tokenStart(ध्रुव वर्ण* cp);
    स्थिर bool tokenEnd(ध्रुव वर्ण* cp);

    // CALLBACKS - MUST OVERRIDE
    आभास शून्य putcOutput(वर्ण chr) = 0;
    आभास शून्य putsOutput(ध्रुव वर्ण* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : सार्वजनिक V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    स्थिर constexpr std::size_t WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    ध्रुव std::string m_filename;
    FILE* m_fp = nullptr;
    std::size_t m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::size_t m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<वर्ण, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

सार्वजनिक:
    V3OutFile(ध्रुव string& filename, V3OutFormatter::Language lang);
    V3OutFile(ध्रुव V3OutFile&) = delete;
    V3OutFile& operator=(ध्रुव V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& operator=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() ध्रुव { प्रत्यागम m_filename; }

    शून्य putsForceIncs();

    शून्य statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

गुप्त:
    शून्य writeBlock() {
        जइ (VL_LIKELY(m_usedBytes > 0)) {
            fwrite(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    शून्य putcOutput(वर्ण chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        जइ (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    शून्य putsOutput(ध्रुव वर्ण* str) override {
        std::size_t len = strlen(str);
        std::size_t availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        जाव (VL_UNLIKELY(len >= availableBytes)) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        जइ (len > 0) {
            std::memcpy(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : सार्वजनिक V3OutFile {
    पूर्णाङ्क m_guard = false;  // Created header guard
    पूर्णाङ्क m_private;  // 1 = Most recently emitted private:, 2 = public:
सार्वजनिक:
    explicit V3OutCFile(ध्रुव string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    आभास शून्य putsHeader() { puts("// Verilated -*- C++ -*-\n"); }
    आभास शून्य puts_Int_शिखरम्_Include() { putsForceIncs(); }
    आभास शून्य putsGuard();
    आभास शून्य putsEndGuard() {
        जइ (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    शून्य resetPrivate() { m_private = 0; }
    शून्य putsPrivate(bool setPrivate) {
        जइ (setPrivate && m_private != 1) {
            puts("private:\n");
            m_private = 1;
        } अण्णहा जइ (!setPrivate && m_private != 2) {
            puts("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : सार्वजनिक V3OutFile {
    // CONSTANTS
    स्थिर constexpr ध्रुव वर्ण* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
गुप्त:
    std::राशिः<वर्ण> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::राशिः<bool> m_empty;  // Current scope is empty, no comma later

सार्वजनिक:
    explicit V3OutJsonFile(ध्रुव string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        puts("\n");
    }
    आभास शून्य putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    शून्य puts(ध्रुव वर्ण* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    शून्य puts(ध्रुव string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(ध्रुव std::string& name, वर्ण type = '{') {
        comma();
        puts(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.प्रक्षिप(type == '{' ? '}' : ']');
        जइ (!m_empty.empty()) m_empty.शिखरम्() = false;
        m_empty.प्रक्षिप(true);
        प्रत्यागम *this;
    }
    V3OutJsonFile& begin(वर्ण type = '{') {
        comma();
        puts(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.प्रक्षिप(type == '{' ? '}' : ']');
        जइ (!m_empty.empty()) m_empty.शिखरम्() = false;
        m_empty.प्रक्षिप(true);
        प्रत्यागम *this;
    }

    // Put a named value
    V3OutJsonFile& put(ध्रुव std::string& name, ध्रुव वर्ण* valuep) {
        प्रत्यागम putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(ध्रुव std::string& name, ध्रुव std::string& मानम्) {
        प्रत्यागम putNamed(name, मानम्, true);
    }
    V3OutJsonFile& put(ध्रुव std::string& name, bool मानम्) {
        प्रत्यागम putNamed(name, मानम् ? "true" : "false", false);
    }
    V3OutJsonFile& put(ध्रुव std::string& name, पूर्णाङ्क मानम्) {
        प्रत्यागम putNamed(name, std::to_string(मानम्), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(ध्रुव std::string& मानम्) { प्रत्यागम putNamed("", मानम्, true); }
    V3OutJsonFile& put(bool मानम्) { प्रत्यागम putNamed("", मानम् ? "true" : "false", false); }
    V3OutJsonFile& put(पूर्णाङ्क मानम्) { प्रत्यागम putNamed("", std::to_string(मानम्), false); }

    V3OutJsonFile& putList(ध्रुव std::string& name, ध्रुव std::vector<std::string>& list) {
        जइ (list.empty()) प्रत्यागम *this;
        begin(name, '[');
        प्रति (ध्रुव std::string& str : list) put(str);
        प्रत्यागम end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= strlen(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - strlen(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        puts("\n" + m_prefix + m_scope.शिखरम्());
        m_scope.निष्कासय();
        m_empty.निष्कासय();
        प्रत्यागम *this;
    }

    V3OutJsonFile& operator+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        प्रत्यागम *this;
    }

गुप्त:
    शून्य comma() {
        जइ (m_empty.empty()) प्रत्यागम;
        जइ (!m_empty.शिखरम्()) puts(",\n");
        m_empty.शिखरम्() = true;
    }
    V3OutJsonFile& putNamed(ध्रुव std::string& name, ध्रुव std::string& मानम्, bool quoted) {
        comma();
        ध्रुव string मानम्_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(मानम्) + "\"" : मानम्;
        जइ (name.empty()) {
            puts(m_prefix + मानम्_Q);
        } अण्णहा {
            puts(m_prefix + "\"" + name + "\": " + मानम्_Q);
        }
        m_empty.शिखरम्() = false;
        प्रत्यागम *this;
    }
};

class V3OutMkFile final : सार्वजनिक V3OutFile {
सार्वजनिक:
    explicit V3OutMkFile(ध्रुव string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = स्वभाव;
    आभास शून्य putsHeader() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    शून्य puts(ध्रुव वर्ण* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    शून्य puts(ध्रुव string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    शून्य putSet(ध्रुव string& var, ध्रुव string& मानम्) {
        puts(VString::dot(var + " =", " ", मानम्) + "\n");
    }
    // Put VARIABLE ?= VALUE
    शून्य putSetQ(ध्रुव string& var, ध्रुव string& मानम्) {
        puts(VString::dot(var + " ?=", " ", मानम्) + "\n");
    }
};

class V3OutVFile final : सार्वजनिक V3OutCFile {
सार्वजनिक:
    explicit V3OutVFile(ध्रुव string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = स्वभाव;
    शून्य putsHeader() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : सार्वजनिक V3OutFile {
सार्वजनिक:
    explicit V3OutXmlFile(ध्रुव string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = स्वभाव;
    आभास शून्य putsHeader() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : सार्वजनिक V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

सार्वजनिक:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = स्वभाव;

    शून्य putcOutput(वर्ण chr) override { m_ostream << chr; };
    शून्य putsOutput(ध्रुव वर्ण* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
सार्वजनिक:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    स्थिर string ifNoProtect(ध्रुव string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    स्थिर string protect(ध्रुव string& old) VL_MT_SAFE { प्रत्यागम protectIf(old, true); }
    स्थिर string protectIf(ध्रुव string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    स्थिर string protect_शब्दाः_If(ध्रुव string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    स्थिर शून्य writeMapFile(ध्रुव string& filename);
};

#endif  // Guard
