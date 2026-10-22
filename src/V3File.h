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

#अगर_अघोषित VERILATOR_V3FILE_H_
#घोषणा VERILATOR_V3FILE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3Stats.h"

#समावेश <array>
#समावेश <fstream>
#समावेश <list>
#समावेश <memory>
#समावेश <set>
#समावेश <stack>
#समावेश <vector>

class Ast_गाँठ;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
खुला:
    अटल std::ifstream* new_ifstream(स्थिर string& filename) {
        addSrcDepend(filename);
        वापस new_ifstream_nodepend(filename);
    }
    अटल std::ifstream* new_ifstream_nodepend(स्थिर string& filename) VL_MT_SAFE {
        वापस new std::ifstream{filename.c_str()};
    }
    अटल std::ofstream* new_ofstream(स्थिर string& filename, bool append = false) {
        addTgtDepend(filename);
        वापस new_ofstream_nodepend(filename, append);
    }
    अटल std::ofstream* new_ofstream_nodepend(स्थिर string& filename, bool append = false) {
        createMakeDirFor(filename);
        अगर (append) वापस new std::ofstream{filename.c_str(), std::ios::app};
        वापस new std::ofstream{filename.c_str()};
    }
    अटल खाता* new_fopen_w(स्थिर string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        वापस ख_खोलो(filename.c_str(), "w");
    }

    // Dependencies
    अटल व्योम addSrcDepend(स्थिर string& filename) VL_MT_SAFE;
    अटल व्योम addTgtDepend(स्थिर string& filename) VL_MT_SAFE;
    अटल व्योम writeDepend(स्थिर string& filename);
    अटल std::vector<string> getAllDeps();
    अटल व्योम writeTimes(स्थिर string& filename, स्थिर string& cmdlineIn);
    अटल bool checkTimes(स्थिर string& filename, स्थिर string& cmdlineIn);

    // Directory utilities
    अटल व्योम createMakeDirFor(स्थिर string& filename);
    अटल व्योम createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
खुला:
    // TYPES
    using StrList = std::list<std::string>;

निजी:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

खुला:
    explicit VInFilter(स्थिर string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(स्थिर string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
खुला:
    क्रमागत AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    क्रमागत Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

निजी:
    // MEMBERS
    स्थिर Language m_lang;  // Indenting Verilog code
    पूर्णांक m_blockIndent;  // Characters per block indent
    पूर्णांक m_commaWidth;  // Width after which to break at ,'s
    पूर्णांक m_lineno = 1;
    पूर्णांक m_column = 0;
    पूर्णांक m_nobreak = false;  // Basic operator or begin paren, don't break next
    पूर्णांक m_sourceLastLineno = 0;
    पूर्णांक m_sourceLastFilenameno = 0;
    bool m_prependIndent = true;
    bool m_inStringLiteral = false;  // Inside a quote
    bool m_inBackslash = false;  // Inside a backslash
    पूर्णांक m_indentLevel = 0;  // Current {} indentation
    std::ढेर<पूर्णांक> m_parenVec;  // Stack of columns where last ( was
    पूर्णांक m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    पूर्णांक endLevels(स्थिर अक्षर* strg);
    व्योम putcNoTracking(अक्षर chr);

खुला:
    explicit V3OutFormatter(Language lang);
    भव ~V3OutFormatter() = शेष;
    // ACCESSORS
    व्योम blockIndent(पूर्णांक flag) { m_blockIndent = flag; }
    // METHODS
    व्योम म_लिखो(स्थिर अक्षर* fmt...) VL_ATTR_PRINTF(2);
    व्योम माला_दो(स्थिर अक्षर* strg) { putns(nullptr, strg); }
    व्योम माला_दो(स्थिर string& strg) { putns(nullptr, strg); }
    व्योम putns(स्थिर Ast_गाँठ* nodep, स्थिर अक्षर* strg);
    व्योम putns(स्थिर Ast_गाँठ* nodep, स्थिर string& strg) { putns(nodep, strg.c_str()); }
    व्योम putsNoTracking(स्थिर string& strg);
    व्योम putsQuoted(स्थिर string& strg);
    व्योम putBreak();  // Print linebreak if line is too wide
    व्योम put_Break_व्यंजक();  // Print linebreak in expression if line is too wide
    व्योम putbs(स्थिर अक्षर* strg) {
        put_Break_व्यंजक();
        putns(nullptr, strg);
    }
    व्योम putbs(स्थिर string& strg) {
        put_Break_व्यंजक();
        putns(nullptr, strg);
    }
    व्योम putnbs(स्थिर Ast_गाँठ* nodep, स्थिर string& strg) {
        put_Break_व्यंजक();
        putns(nodep, strg);
    }
    bool exceededWidth() स्थिर { वापस m_column > m_commaWidth; }
    व्योम indentInc() { m_indentLevel += m_blockIndent; }
    व्योम indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    व्योम blockInc() { m_parenVec.धकेलो(m_indentLevel + m_blockIndent); }
    व्योम blockDec() {
        अगर (!m_parenVec.empty()) m_parenVec.निकालो();
    }
    व्योम ensure_New_पंक्ति() {
        अगर (!m_nobreak) माला_दो("\n");
    }
    // STATIC METHODS
    अटल स्थिर std::string& indentSpaces(पूर्णांक अंक);
    // Add escaped characters to strings
    अटल string quoteNameControls(स्थिर string& namein, Language lang = LA_C) VL_PURE;
    अटल bool tokenMatch(स्थिर अक्षर* cp, स्थिर अक्षर* cmp);
    अटल bool tokenNotStart(स्थिर अक्षर* cp);  // Import/export meaning no endfunction
    अटल bool tokenStart(स्थिर अक्षर* cp);
    अटल bool tokenEnd(स्थिर अक्षर* cp);

    // CALLBACKS - MUST OVERRIDE
    भव व्योम putcOutput(अक्षर chr) = 0;
    भव व्योम putsOutput(स्थिर अक्षर* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3OutFile VL_NOT_FINAL : खुला V3OutFormatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    अटल constexpr std::माप_प्रकार WRITE_बफर_आकार_BYTES = 128 * 1024;

    // MEMBERS
    स्थिर std::string m_filename;
    खाता* m_fp = nullptr;
    std::माप_प्रकार m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::माप_प्रकार m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<अक्षर, WRITE_बफर_आकार_BYTES>> m_bufferp;  // Write buffer

खुला:
    V3OutFile(स्थिर string& filename, V3OutFormatter::Language lang);
    V3OutFile(स्थिर V3OutFile&) = delete;
    V3OutFile& चालक=(स्थिर V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& चालक=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() स्थिर { वापस m_filename; }

    व्योम putsForceIncs();

    व्योम statRecordWritten() {
        writeBlock();
        V3Stats::add_Stat_योग(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

निजी:
    व्योम writeBlock() {
        अगर (VL_LIKELY(m_usedBytes > 0)) {
            ख_डालो(m_bufferp->तथ्य(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    व्योम putcOutput(अक्षर chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        अगर (VL_UNLIKELY(m_usedBytes >= WRITE_बफर_आकार_BYTES)) writeBlock();
    }
    व्योम putsOutput(स्थिर अक्षर* str) override {
        std::माप_प्रकार len = म_माप(str);
        std::माप_प्रकार availableBytes = WRITE_बफर_आकार_BYTES - m_usedBytes;
        जबतक (VL_UNLIKELY(len >= availableBytes)) {
            std::स_नकल(m_bufferp->तथ्य() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_बफर_आकार_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_बफर_आकार_BYTES;
        }
        अगर (len > 0) {
            std::स_नकल(m_bufferp->तथ्य() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : खुला V3OutFile {
    पूर्णांक m_guard = false;  // Created header guard
    पूर्णांक m_private;  // 1 = Most recently emitted private:, 2 = public:
खुला:
    explicit V3OutCFile(स्थिर string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    भव व्योम putsHeader() { माला_दो("// Verilated -*- C++ -*-\n"); }
    भव व्योम puts_Int_शीर्ष_Include() { putsForceIncs(); }
    भव व्योम putsGuard();
    भव व्योम putsEndGuard() {
        अगर (m_guard) माला_दो("\n#endif  // guard\n");
    }
    // Print out public/privates
    व्योम resetPrivate() { m_private = 0; }
    व्योम putsPrivate(bool setPrivate) {
        अगर (setPrivate && m_private != 1) {
            माला_दो("private:\n");
            m_private = 1;
        } अन्यथा अगर (!setPrivate && m_private != 2) {
            माला_दो("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : खुला V3OutFile {
    // CONSTANTS
    अटल constexpr स्थिर अक्षर* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
निजी:
    std::ढेर<अक्षर> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::ढेर<bool> m_empty;  // Current scope is empty, no comma later

खुला:
    explicit V3OutJsonFile(स्थिर string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        माला_दो("\n");
    }
    भव व्योम putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    व्योम माला_दो(स्थिर अक्षर* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    व्योम माला_दो(स्थिर string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(स्थिर std::string& name, अक्षर type = '{') {
        comma();
        माला_दो(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.धकेलो(type == '{' ? '}' : ']');
        अगर (!m_empty.empty()) m_empty.शीर्ष() = false;
        m_empty.धकेलो(true);
        वापस *this;
    }
    V3OutJsonFile& begin(अक्षर type = '{') {
        comma();
        माला_दो(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.धकेलो(type == '{' ? '}' : ']');
        अगर (!m_empty.empty()) m_empty.शीर्ष() = false;
        m_empty.धकेलो(true);
        वापस *this;
    }

    // Put a named value
    V3OutJsonFile& put(स्थिर std::string& name, स्थिर अक्षर* valuep) {
        वापस putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(स्थिर std::string& name, स्थिर std::string& मूल्य) {
        वापस putNamed(name, मूल्य, true);
    }
    V3OutJsonFile& put(स्थिर std::string& name, bool मूल्य) {
        वापस putNamed(name, मूल्य ? "true" : "false", false);
    }
    V3OutJsonFile& put(स्थिर std::string& name, पूर्णांक मूल्य) {
        वापस putNamed(name, std::to_string(मूल्य), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(स्थिर std::string& मूल्य) { वापस putNamed("", मूल्य, true); }
    V3OutJsonFile& put(bool मूल्य) { वापस putNamed("", मूल्य ? "true" : "false", false); }
    V3OutJsonFile& put(पूर्णांक मूल्य) { वापस putNamed("", std::to_string(मूल्य), false); }

    V3OutJsonFile& putList(स्थिर std::string& name, स्थिर std::vector<std::string>& list) {
        अगर (list.empty()) वापस *this;
        begin(name, '[');
        क्रम (स्थिर std::string& str : list) put(str);
        वापस end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.लंबाई() >= म_माप(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - म_माप(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        माला_दो("\n" + m_prefix + m_scope.शीर्ष());
        m_scope.निकालो();
        m_empty.निकालो();
        वापस *this;
    }

    V3OutJsonFile& चालक+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        वापस *this;
    }

निजी:
    व्योम comma() {
        अगर (m_empty.empty()) वापस;
        अगर (!m_empty.शीर्ष()) माला_दो(",\n");
        m_empty.शीर्ष() = true;
    }
    V3OutJsonFile& putNamed(स्थिर std::string& name, स्थिर std::string& मूल्य, bool quoted) {
        comma();
        स्थिर string मूल्य_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(मूल्य) + "\"" : मूल्य;
        अगर (name.empty()) {
            माला_दो(m_prefix + मूल्य_Q);
        } अन्यथा {
            माला_दो(m_prefix + "\"" + name + "\": " + मूल्य_Q);
        }
        m_empty.शीर्ष() = false;
        वापस *this;
    }
};

class V3OutMkFile final : खुला V3OutFile {
खुला:
    explicit V3OutMkFile(स्थिर string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = शेष;
    भव व्योम putsHeader() { माला_दो("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    व्योम माला_दो(स्थिर अक्षर* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    व्योम माला_दो(स्थिर string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    व्योम putSet(स्थिर string& var, स्थिर string& मूल्य) {
        माला_दो(VString::dot(var + " =", " ", मूल्य) + "\n");
    }
    // Put VARIABLE ?= VALUE
    व्योम putSetQ(स्थिर string& var, स्थिर string& मूल्य) {
        माला_दो(VString::dot(var + " ?=", " ", मूल्य) + "\n");
    }
};

class V3OutVFile final : खुला V3OutCFile {
खुला:
    explicit V3OutVFile(स्थिर string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = शेष;
    व्योम putsHeader() override { माला_दो("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : खुला V3OutFile {
खुला:
    explicit V3OutXmlFile(स्थिर string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = शेष;
    भव व्योम putsHeader() { माला_दो("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3OutStream VL_NOT_FINAL : खुला V3OutFormatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3OutStream);
    VL_UNMOVABLE(V3OutStream);

खुला:
    V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang);
    ~V3OutStream() override = शेष;

    व्योम putcOutput(अक्षर chr) override { m_ostream << chr; };
    व्योम putsOutput(स्थिर अक्षर* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
खुला:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    अटल string ifNoProtect(स्थिर string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    अटल string protect(स्थिर string& old) VL_MT_SAFE { वापस protectIf(old, true); }
    अटल string protectIf(स्थिर string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    अटल string protect_शब्दगण_If(स्थिर string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    अटल व्योम writeMapFile(स्थिर string& filename);
};

#पूर्ण_अगर  // Guard
