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

class Ast_गाठ;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
सार्वजनिक:
    स्थिर std::ifstream* new_ifstream(स्थिरांक string& filename) {
        addSrcDepend(filename);
        परत new_ifstream_nodepend(filename);
    }
    स्थिर std::ifstream* new_ifstream_nodepend(स्थिरांक string& filename) VL_MT_SAFE {
        परत new std::ifstream{filename.c_str()};
    }
    स्थिर std::ofstream* new_ofstream(स्थिरांक string& filename, bool append = false) {
        addTgtDepend(filename);
        परत new_ofstream_nodepend(filename, append);
    }
    स्थिर std::ofstream* new_ofstream_nodepend(स्थिरांक string& filename, bool append = false) {
        createMakeDirFor(filename);
        जर (append) परत new std::ofstream{filename.c_str(), std::ios::app};
        परत new std::ofstream{filename.c_str()};
    }
    स्थिर खाता* new_fopen_w(स्थिरांक string& filename) {
        createMakeDirFor(filename);
        addTgtDepend(filename);
        परत ख_खोलो(filename.c_str(), "w");
    }

    // Dependencies
    स्थिर शून्य addSrcDepend(स्थिरांक string& filename) VL_MT_SAFE;
    स्थिर शून्य addTgtDepend(स्थिरांक string& filename) VL_MT_SAFE;
    स्थिर शून्य writeDepend(स्थिरांक string& filename);
    स्थिर std::vector<string> getAllDeps();
    स्थिर शून्य writeTimes(स्थिरांक string& filename, स्थिरांक string& cmdlineIn);
    स्थिर bool checkTimes(स्थिरांक string& filename, स्थिरांक string& cmdlineIn);

    // Directory utilities
    स्थिर शून्य createMakeDirFor(स्थिरांक string& filename);
    स्थिर शून्य createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
सार्वजनिक:
    // TYPES
    using StrList = std::list<std::string>;

खाजगी:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

सार्वजनिक:
    explicit VInFilter(स्थिरांक string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(स्थिरांक string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3OutFormatter VL_NOT_FINAL {
    // TYPES
सार्वजनिक:
    गणना AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    गणना Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

खाजगी:
    // MEMBERS
    स्थिरांक Language m_lang;  // Indenting Verilog code
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
    std::चळत<पूर्णांक> m_parenVec;  // Stack of columns where last ( was
    पूर्णांक m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    पूर्णांक endLevels(स्थिरांक अक्षर* strg);
    शून्य putcNoTracking(अक्षर chr);

सार्वजनिक:
    explicit V3OutFormatter(Language lang);
    आभासी ~V3OutFormatter() = पूर्वनिर्धारित;
    // ACCESSORS
    शून्य blockIndent(पूर्णांक flag) { m_blockIndent = flag; }
    // METHODS
    शून्य म_लिखो(स्थिरांक अक्षर* fmt...) VL_ATTR_PRINTF(2);
    शून्य माला_दो(स्थिरांक अक्षर* strg) { putns(nullptr, strg); }
    शून्य माला_दो(स्थिरांक string& strg) { putns(nullptr, strg); }
    शून्य putns(स्थिरांक Ast_गाठ* nodep, स्थिरांक अक्षर* strg);
    शून्य putns(स्थिरांक Ast_गाठ* nodep, स्थिरांक string& strg) { putns(nodep, strg.c_str()); }
    शून्य putsNoTracking(स्थिरांक string& strg);
    शून्य putsQuoted(स्थिरांक string& strg);
    शून्य putBreak();  // Print linebreak if line is too wide
    शून्य put_Break_अभिव्यक्ती();  // Print linebreak in expression if line is too wide
    शून्य putbs(स्थिरांक अक्षर* strg) {
        put_Break_अभिव्यक्ती();
        putns(nullptr, strg);
    }
    शून्य putbs(स्थिरांक string& strg) {
        put_Break_अभिव्यक्ती();
        putns(nullptr, strg);
    }
    शून्य putnbs(स्थिरांक Ast_गाठ* nodep, स्थिरांक string& strg) {
        put_Break_अभिव्यक्ती();
        putns(nodep, strg);
    }
    bool exceededWidth() स्थिरांक { परत m_column > m_commaWidth; }
    शून्य indentInc() { m_indentLevel += m_blockIndent; }
    शून्य indentDec() {
        m_indentLevel -= m_blockIndent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    शून्य blockInc() { m_parenVec.ढकला(m_indentLevel + m_blockIndent); }
    शून्य blockDec() {
        जर (!m_parenVec.empty()) m_parenVec.काढा();
    }
    शून्य ensure_New_ओळ() {
        जर (!m_nobreak) माला_दो("\n");
    }
    // STATIC METHODS
    स्थिर स्थिरांक std::string& indentSpaces(पूर्णांक अंक);
    // Add escaped characters to strings
    स्थिर string quoteNameControls(स्थिरांक string& namein, Language lang = LA_C) VL_PURE;
    स्थिर bool tokenMatch(स्थिरांक अक्षर* cp, स्थिरांक अक्षर* cmp);
    स्थिर bool tokenNotStart(स्थिरांक अक्षर* cp);  // Import/export meaning no endfunction
    स्थिर bool tokenStart(स्थिरांक अक्षर* cp);
    स्थिर bool tokenEnd(स्थिरांक अक्षर* cp);

    // CALLBACKS - MUST OVERRIDE
    आभासी शून्य putcOutput(अक्षर chr) = 0;
    आभासी शून्य putsOutput(स्थिरांक अक्षर* str) = 0;
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
    स्थिर constexpr std::माप_प्रकार WRITE_BUFFER_SIZE_BYTES = 128 * 1024;

    // MEMBERS
    स्थिरांक std::string m_filename;
    खाता* m_fp = nullptr;
    std::माप_प्रकार m_usedBytes = 0;  // Number of bytes stored in m_bufferp
    std::माप_प्रकार m_writtenBytes = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<अक्षर, WRITE_BUFFER_SIZE_BYTES>> m_bufferp;  // Write buffer

सार्वजनिक:
    V3OutFile(स्थिरांक string& filename, V3OutFormatter::Language lang);
    V3OutFile(स्थिरांक V3OutFile&) = delete;
    V3OutFile& चालक=(स्थिरांक V3OutFile&) = delete;
    V3OutFile(V3OutFile&&) = delete;
    V3OutFile& चालक=(V3OutFile&&) = delete;
    ~V3OutFile() override;

    std::string filename() स्थिरांक { परत m_filename; }

    शून्य putsForceIncs();

    शून्य statRecordWritten() {
        writeBlock();
        V3Stats::addStatSum(V3Stats::STAT_CPP_CHARS, m_writtenBytes);
    }

खाजगी:
    शून्य writeBlock() {
        जर (VL_LIKELY(m_usedBytes > 0)) {
            ख_डालो(m_bufferp->data(), m_usedBytes, 1, m_fp);
            m_writtenBytes += m_usedBytes;
            m_usedBytes = 0;
        }
    }
    // CALLBACKS
    शून्य putcOutput(अक्षर chr) override {
        m_bufferp->at(m_usedBytes++) = chr;
        जर (VL_UNLIKELY(m_usedBytes >= WRITE_BUFFER_SIZE_BYTES)) writeBlock();
    }
    शून्य putsOutput(स्थिरांक अक्षर* str) override {
        std::माप_प्रकार len = म_माप(str);
        std::माप_प्रकार availableBytes = WRITE_BUFFER_SIZE_BYTES - m_usedBytes;
        पर्यंत (VL_UNLIKELY(len >= availableBytes)) {
            std::स_नकल(m_bufferp->data() + m_usedBytes, str, availableBytes);
            m_usedBytes = WRITE_BUFFER_SIZE_BYTES;
            writeBlock();
            str += availableBytes;
            len -= availableBytes;
            availableBytes = WRITE_BUFFER_SIZE_BYTES;
        }
        जर (len > 0) {
            std::स_नकल(m_bufferp->data() + m_usedBytes, str, len);
            m_usedBytes += len;
        }
    }
};

class V3OutCFile VL_NOT_FINAL : सार्वजनिक V3OutFile {
    पूर्णांक m_guard = false;  // Created header guard
    पूर्णांक m_private;  // 1 = Most recently emitted private:, 2 = public:
सार्वजनिक:
    explicit V3OutCFile(स्थिरांक string& filename,
                        V3OutFormatter::Language lang = V3OutFormatter::LA_C)
        : V3OutFile{filename, lang} {
        resetPrivate();
    }
    ~V3OutCFile() override { statRecordWritten(); }
    आभासी शून्य putsHeader() { माला_दो("// Verilated -*- C++ -*-\n"); }
    आभासी शून्य puts_Int_शीर्ष_Include() { putsForceIncs(); }
    आभासी शून्य putsGuard();
    आभासी शून्य putsEndGuard() {
        जर (m_guard) माला_दो("\n#endif  // guard\n");
    }
    // Print out public/privates
    शून्य resetPrivate() { m_private = 0; }
    शून्य putsPrivate(bool setPrivate) {
        जर (setPrivate && m_private != 1) {
            माला_दो("private:\n");
            m_private = 1;
        } अन्यथा जर (!setPrivate && m_private != 2) {
            माला_दो("public:\n");
            m_private = 2;
        }
    }
};

class V3OutJsonFile final : सार्वजनिक V3OutFile {
    // CONSTANTS
    स्थिर constexpr स्थिरांक अक्षर* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
खाजगी:
    std::चळत<अक्षर> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_prefix;  // Prefix emitted before each line in current scope
    std::चळत<bool> m_empty;  // Current scope is empty, no comma later

सार्वजनिक:
    explicit V3OutJsonFile(स्थिरांक string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_JSON} {
        begin();
    }
    ~V3OutJsonFile() override {
        end();
        माला_दो("\n");
    }
    आभासी शून्य putsHeader() {}
    // cppcheck-suppress duplInheritedMember
    शून्य माला_दो(स्थिरांक अक्षर* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    शून्य माला_दो(स्थिरांक string& strg) { putsNoTracking(strg); }

    // METHODS
    V3OutJsonFile& begin(स्थिरांक std::string& name, अक्षर type = '{') {
        comma();
        माला_दो(m_prefix + "\"" + name + "\": " + type + "\n");
        m_prefix += INDENT;
        m_scope.ढकला(type == '{' ? '}' : ']');
        जर (!m_empty.empty()) m_empty.शीर्ष() = false;
        m_empty.ढकला(true);
        परत *this;
    }
    V3OutJsonFile& begin(अक्षर type = '{') {
        comma();
        माला_दो(m_prefix + type + "\n");
        m_prefix += INDENT;
        m_scope.ढकला(type == '{' ? '}' : ']');
        जर (!m_empty.empty()) m_empty.शीर्ष() = false;
        m_empty.ढकला(true);
        परत *this;
    }

    // Put a named value
    V3OutJsonFile& put(स्थिरांक std::string& name, स्थिरांक अक्षर* valuep) {
        परत putNamed(name, std::string{valuep}, true);
    }
    V3OutJsonFile& put(स्थिरांक std::string& name, स्थिरांक std::string& मूल्य) {
        परत putNamed(name, मूल्य, true);
    }
    V3OutJsonFile& put(स्थिरांक std::string& name, bool मूल्य) {
        परत putNamed(name, मूल्य ? "true" : "false", false);
    }
    V3OutJsonFile& put(स्थिरांक std::string& name, पूर्णांक मूल्य) {
        परत putNamed(name, std::to_string(मूल्य), false);
    }

    // Put unnamed value
    V3OutJsonFile& put(स्थिरांक std::string& मूल्य) { परत putNamed("", मूल्य, true); }
    V3OutJsonFile& put(bool मूल्य) { परत putNamed("", मूल्य ? "true" : "false", false); }
    V3OutJsonFile& put(पूर्णांक मूल्य) { परत putNamed("", std::to_string(मूल्य), false); }

    V3OutJsonFile& putList(स्थिरांक std::string& name, स्थिरांक std::vector<std::string>& list) {
        जर (list.empty()) परत *this;
        begin(name, '[');
        साठी (स्थिरांक std::string& str : list) put(str);
        परत end();
    }

    V3OutJsonFile& end() {
        UASSERT(m_prefix.length() >= म_माप(INDENT), "prefix underflow");
        m_prefix.erase(m_prefix.end() - म_माप(INDENT), m_prefix.end());
        UASSERT(!m_scope.empty(), "end() without begin()");
        UASSERT(!m_empty.empty(), "end() without begin()");
        माला_दो("\n" + m_prefix + m_scope.शीर्ष());
        m_scope.काढा();
        m_empty.काढा();
        परत *this;
    }

    V3OutJsonFile& चालक+=(V3OutJsonFile& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        परत *this;
    }

खाजगी:
    शून्य comma() {
        जर (m_empty.empty()) परत;
        जर (!m_empty.शीर्ष()) माला_दो(",\n");
        m_empty.शीर्ष() = true;
    }
    V3OutJsonFile& putNamed(स्थिरांक std::string& name, स्थिरांक std::string& मूल्य, bool quoted) {
        comma();
        स्थिरांक string मूल्य_Q
            = quoted ? "\""s + V3OutFormatter::quoteNameControls(मूल्य) + "\"" : मूल्य;
        जर (name.empty()) {
            माला_दो(m_prefix + मूल्य_Q);
        } अन्यथा {
            माला_दो(m_prefix + "\"" + name + "\": " + मूल्य_Q);
        }
        m_empty.शीर्ष() = false;
        परत *this;
    }
};

class V3OutMkFile final : सार्वजनिक V3OutFile {
सार्वजनिक:
    explicit V3OutMkFile(स्थिरांक string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_MK} {}
    ~V3OutMkFile() override = पूर्वनिर्धारित;
    आभासी शून्य putsHeader() { माला_दो("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    शून्य माला_दो(स्थिरांक अक्षर* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    शून्य माला_दो(स्थिरांक string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    शून्य putSet(स्थिरांक string& var, स्थिरांक string& मूल्य) {
        माला_दो(VString::dot(var + " =", " ", मूल्य) + "\n");
    }
    // Put VARIABLE ?= VALUE
    शून्य putSetQ(स्थिरांक string& var, स्थिरांक string& मूल्य) {
        माला_दो(VString::dot(var + " ?=", " ", मूल्य) + "\n");
    }
};

class V3OutVFile final : सार्वजनिक V3OutCFile {
सार्वजनिक:
    explicit V3OutVFile(स्थिरांक string& filename)
        : V3OutCFile{filename, V3OutFormatter::LA_VERILOG} {}
    ~V3OutVFile() override = पूर्वनिर्धारित;
    शून्य putsHeader() override { माला_दो("// Verilated -*- Verilog -*-\n"); }
};

class V3OutXmlFile final : सार्वजनिक V3OutFile {
सार्वजनिक:
    explicit V3OutXmlFile(स्थिरांक string& filename)
        : V3OutFile{filename, V3OutFormatter::LA_XML} {
        blockIndent(2);
    }
    ~V3OutXmlFile() override = पूर्वनिर्धारित;
    आभासी शून्य putsHeader() { माला_दो("<?xml version=\"1.0\" ?>\n"); }
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
    ~V3OutStream() override = पूर्वनिर्धारित;

    शून्य putcOutput(अक्षर chr) override { m_ostream << chr; };
    शून्य putsOutput(स्थिरांक अक्षर* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
सार्वजनिक:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    स्थिर string ifNoProtect(स्थिरांक string& in) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    स्थिर string protect(स्थिरांक string& old) VL_MT_SAFE { परत protectIf(old, true); }
    स्थिर string protectIf(स्थिरांक string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    स्थिर string protect_शब्द_If(स्थिरांक string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    स्थिर शून्य writeMapFile(स्थिरांक string& filename);
};

#endif  // Guard
