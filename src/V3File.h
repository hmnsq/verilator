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

#यदि_अपरिभाषितम् VERILATOR_V3FILE_H_
#परिभाषय VERILATOR_V3FILE_H_

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
        सङ्कलय_Src_Depend(filename);
        प्रत्यागम new_ifstream_nodepend(filename);
    }
    स्थिर std::ifstream* new_ifstream_nodepend(ध्रुव string& filename) VL_MT_SAFE {
        प्रत्यागम new std::ifstream{filename.c_str()};
    }
    स्थिर std::ofstream* new_ofstream(ध्रुव string& filename, bool append = false) {
        सङ्कलय_Tgt_Depend(filename);
        प्रत्यागम new_ofstream_nodepend(filename, append);
    }
    स्थिर std::ofstream* new_ofstream_nodepend(ध्रुव string& filename, bool append = false) {
        createMakeDirFor(filename);
        यदि (append) प्रत्यागम new std::ofstream{filename.c_str(), std::ios::app};
        प्रत्यागम new std::ofstream{filename.c_str()};
    }
    स्थिर FILE* new_fopen_w(ध्रुव string& filename) {
        createMakeDirFor(filename);
        सङ्कलय_Tgt_Depend(filename);
        प्रत्यागम fopen(filename.c_str(), "w");
    }

    // Dependencies
    स्थिर शून्य सङ्कलय_Src_Depend(ध्रुव string& filename) VL_MT_SAFE;
    स्थिर शून्य सङ्कलय_Tgt_Depend(ध्रुव string& filename) VL_MT_SAFE;
    स्थिर शून्य writeDepend(ध्रुव string& filename);
    स्थिर std::सदिशम्<string> getAllDeps();
    स्थिर शून्य writeTimes(ध्रुव string& filename, ध्रुव string& cmdline_अन्तर्);
    स्थिर bool checkTimes(ध्रुव string& filename, ध्रुव string& cmdline_अन्तर्);

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

class V3_बहिर्गमनम्_Formatter VL_NOT_FINAL {
    // TYPES
सार्वजनिक:
    गणना AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    गणना Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

गुप्त:
    // MEMBERS
    ध्रुव Language m_lang;  // Indenting Verilog code
    पूर्णाङ्क m_खण्डम्_Indent;  // Characters per block indent
    पूर्णाङ्क m_comma_विस्तारः;  // Width after which to break at ,'s
    पूर्णाङ्क m_lineno = 1;
    पूर्णाङ्क m_column = 0;
    पूर्णाङ्क m_nobreak = false;  // Basic operator or begin paren, don't break next
    पूर्णाङ्क m_source_चरमः_Lineno = 0;
    पूर्णाङ्क m_source_चरमः_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_अन्तर्_String_Literal = false;  // Inside a quote
    bool m_अन्तर्_Backslash = false;  // Inside a backslash
    पूर्णाङ्क m_indentLevel = 0;  // Current {} indentation
    std::राशिः<पूर्णाङ्क> m_parenVec;  // Stack of columns where last ( was
    पूर्णाङ्क m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    पूर्णाङ्क अन्तः_स्तराः(ध्रुव वर्ण* strg);
    शून्य putcNoTracking(वर्ण chr);

सार्वजनिक:
    explicit V3_बहिर्गमनम्_Formatter(Language lang);
    आभास ~V3_बहिर्गमनम्_Formatter() = स्वभाव;
    // ACCESSORS
    शून्य खण्डम्_Indent(पूर्णाङ्क flag) { m_खण्डम्_Indent = flag; }
    // METHODS
    शून्य प्रारूप_लिख(ध्रुव वर्ण* fmt...) VL_ATTR_PRINTF(2);
    शून्य पङ्क्तिं_लिख(ध्रुव वर्ण* strg) { putns(nullptr, strg); }
    शून्य पङ्क्तिं_लिख(ध्रुव string& strg) { putns(nullptr, strg); }
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
    bool exceeded_विस्तारः() ध्रुव { प्रत्यागम m_column > m_comma_विस्तारः; }
    शून्य indentInc() { m_indentLevel += m_खण्डम्_Indent; }
    शून्य indentDec() {
        m_indentLevel -= m_खण्डम्_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    शून्य खण्डम्_Inc() { m_parenVec.प्रक्षिप(m_indentLevel + m_खण्डम्_Indent); }
    शून्य खण्डम्_Dec() {
        यदि (!m_parenVec.रिक्तानि()) m_parenVec.निष्कासय();
    }
    शून्य ensure_New_पङ्क्तिः() {
        यदि (!m_nobreak) पङ्क्तिं_लिख("\n");
    }
    // STATIC METHODS
    स्थिर ध्रुव std::string& indentSpaces(पूर्णाङ्क अङ्कः);
    // Add escaped characters to strings
    स्थिर string quoteNameControls(ध्रुव string& namein, Language lang = LA_C) VL_विशुद्धम्;
    स्थिर bool token_मेलनम्(ध्रुव वर्ण* cp, ध्रुव वर्ण* cmp);
    स्थिर bool tokenNotStart(ध्रुव वर्ण* cp);  // Import/export meaning no endfunction
    स्थिर bool tokenStart(ध्रुव वर्ण* cp);
    स्थिर bool token_अन्तः(ध्रुव वर्ण* cp);

    // CALLBACKS - MUST OVERRIDE
    आभास शून्य putcOutput(वर्ण chr) = 0;
    आभास शून्य putsOutput(ध्रुव वर्ण* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_बहिर्गमनम्_File VL_NOT_FINAL : सार्वजनिक V3_बहिर्गमनम्_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    स्थिर constexpr std::आकार_t WRITE_बफर_आकार_अष्टकानि = 128 * 1024;

    // MEMBERS
    ध्रुव std::string m_filename;
    FILE* m_fp = nullptr;
    std::आकार_t m_प्रयुक्तम्_अष्टकानि = 0;  // Number of bytes stored in m_bufferp
    std::आकार_t m_written_अष्टकानि = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<वर्ण, WRITE_बफर_आकार_अष्टकानि>> m_bufferp;  // Write buffer

सार्वजनिक:
    V3_बहिर्गमनम्_File(ध्रुव string& filename, V3_बहिर्गमनम्_Formatter::Language lang);
    V3_बहिर्गमनम्_File(ध्रुव V3_बहिर्गमनम्_File&) = delete;
    V3_बहिर्गमनम्_File& संक्रियकः=(ध्रुव V3_बहिर्गमनम्_File&) = delete;
    V3_बहिर्गमनम्_File(V3_बहिर्गमनम्_File&&) = delete;
    V3_बहिर्गमनम्_File& संक्रियकः=(V3_बहिर्गमनम्_File&&) = delete;
    ~V3_बहिर्गमनम्_File() override;

    std::string filename() ध्रुव { प्रत्यागम m_filename; }

    शून्य putsForceIncs();

    शून्य stat_लेखः_Written() {
        write_खण्डम्();
        V3Stats::सङ्कलय_Stat_योग(V3Stats::STAT_CPP_CHARS, m_written_अष्टकानि);
    }

गुप्त:
    शून्य write_खण्डम्() {
        यदि (VL_LIKELY(m_प्रयुक्तम्_अष्टकानि > 0)) {
            fwrite(m_bufferp->दत्त(), m_प्रयुक्तम्_अष्टकानि, 1, m_fp);
            m_written_अष्टकानि += m_प्रयुक्तम्_अष्टकानि;
            m_प्रयुक्तम्_अष्टकानि = 0;
        }
    }
    // CALLBACKS
    शून्य putcOutput(वर्ण chr) override {
        m_bufferp->at(m_प्रयुक्तम्_अष्टकानि++) = chr;
        यदि (VL_UNLIKELY(m_प्रयुक्तम्_अष्टकानि >= WRITE_बफर_आकार_अष्टकानि)) write_खण्डम्();
    }
    शून्य putsOutput(ध्रुव वर्ण* str) override {
        std::आकार_t len = वाक्यदैर्घ्यम्(str);
        std::आकार_t उपलब्धम्_अष्टकानि = WRITE_बफर_आकार_अष्टकानि - m_प्रयुक्तम्_अष्टकानि;
        यावत् (VL_UNLIKELY(len >= उपलब्धम्_अष्टकानि)) {
            std::स्मृतिं_प्रतिलिख(m_bufferp->दत्त() + m_प्रयुक्तम्_अष्टकानि, str, उपलब्धम्_अष्टकानि);
            m_प्रयुक्तम्_अष्टकानि = WRITE_बफर_आकार_अष्टकानि;
            write_खण्डम्();
            str += उपलब्धम्_अष्टकानि;
            len -= उपलब्धम्_अष्टकानि;
            उपलब्धम्_अष्टकानि = WRITE_बफर_आकार_अष्टकानि;
        }
        यदि (len > 0) {
            std::स्मृतिं_प्रतिलिख(m_bufferp->दत्त() + m_प्रयुक्तम्_अष्टकानि, str, len);
            m_प्रयुक्तम्_अष्टकानि += len;
        }
    }
};

class V3_बहिर्गमनम्_CFile VL_NOT_FINAL : सार्वजनिक V3_बहिर्गमनम्_File {
    पूर्णाङ्क m_guard = false;  // Created header guard
    पूर्णाङ्क m_वैयक्तिकम्;  // 1 = Most recently emitted private:, 2 = public:
सार्वजनिक:
    explicit V3_बहिर्गमनम्_CFile(ध्रुव string& filename,
                        V3_बहिर्गमनम्_Formatter::Language lang = V3_बहिर्गमनम्_Formatter::LA_C)
        : V3_बहिर्गमनम्_File{filename, lang} {
        reset_वैयक्तिकम्();
    }
    ~V3_बहिर्गमनम्_CFile() override { stat_लेखः_Written(); }
    आभास शून्य puts_शीर्षकम्() { पङ्क्तिं_लिख("// Verilated -*- C++ -*-\n"); }
    आभास शून्य puts_Int_शिखरम्_Include() { putsForceIncs(); }
    आभास शून्य putsGuard();
    आभास शून्य puts_अन्तः_Guard() {
        यदि (m_guard) पङ्क्तिं_लिख("\n#endif  // guard\n");
    }
    // Print out public/privates
    शून्य reset_वैयक्तिकम्() { m_वैयक्तिकम् = 0; }
    शून्य puts_वैयक्तिकम्(bool समुच्चयः_वैयक्तिकम्) {
        यदि (समुच्चयः_वैयक्तिकम् && m_वैयक्तिकम् != 1) {
            पङ्क्तिं_लिख("private:\n");
            m_वैयक्तिकम् = 1;
        } अन्यथा यदि (!समुच्चयः_वैयक्तिकम् && m_वैयक्तिकम् != 2) {
            पङ्क्तिं_लिख("public:\n");
            m_वैयक्तिकम् = 2;
        }
    }
};

class V3_बहिर्गमनम्_Json_File final : सार्वजनिक V3_बहिर्गमनम्_File {
    // CONSTANTS
    स्थिर constexpr ध्रुव वर्ण* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
गुप्त:
    std::राशिः<वर्ण> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_उपसर्गः;  // Prefix emitted before each line in current scope
    std::राशिः<bool> m_रिक्तानि;  // Current scope is empty, no comma later

सार्वजनिक:
    explicit V3_बहिर्गमनम्_Json_File(ध्रुव string& filename)
        : V3_बहिर्गमनम्_File{filename, V3_बहिर्गमनम्_Formatter::LA_JSON} {
        begin();
    }
    ~V3_बहिर्गमनम्_Json_File() override {
        अन्तः_2();
        पङ्क्तिं_लिख("\n");
    }
    आभास शून्य puts_शीर्षकम्() {}
    // cppcheck-suppress duplInheritedMember
    शून्य पङ्क्तिं_लिख(ध्रुव वर्ण* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    शून्य पङ्क्तिं_लिख(ध्रुव string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_बहिर्गमनम्_Json_File& begin(ध्रुव std::string& name, वर्ण type = '{') {
        comma();
        पङ्क्तिं_लिख(m_उपसर्गः + "\"" + name + "\": " + type + "\n");
        m_उपसर्गः += INDENT;
        m_scope.प्रक्षिप(type == '{' ? '}' : ']');
        यदि (!m_रिक्तानि.रिक्तानि()) m_रिक्तानि.शिखरम्() = false;
        m_रिक्तानि.प्रक्षिप(true);
        प्रत्यागम *this;
    }
    V3_बहिर्गमनम्_Json_File& begin(वर्ण type = '{') {
        comma();
        पङ्क्तिं_लिख(m_उपसर्गः + type + "\n");
        m_उपसर्गः += INDENT;
        m_scope.प्रक्षिप(type == '{' ? '}' : ']');
        यदि (!m_रिक्तानि.रिक्तानि()) m_रिक्तानि.शिखरम्() = false;
        m_रिक्तानि.प्रक्षिप(true);
        प्रत्यागम *this;
    }

    // Put a named value
    V3_बहिर्गमनम्_Json_File& put(ध्रुव std::string& name, ध्रुव वर्ण* valuep) {
        प्रत्यागम putNamed(name, std::string{valuep}, true);
    }
    V3_बहिर्गमनम्_Json_File& put(ध्रुव std::string& name, ध्रुव std::string& मानम्) {
        प्रत्यागम putNamed(name, मानम्, true);
    }
    V3_बहिर्गमनम्_Json_File& put(ध्रुव std::string& name, bool मानम्) {
        प्रत्यागम putNamed(name, मानम् ? "true" : "false", false);
    }
    V3_बहिर्गमनम्_Json_File& put(ध्रुव std::string& name, पूर्णाङ्क मानम्) {
        प्रत्यागम putNamed(name, std::गन्तव्यम्_string(मानम्), false);
    }

    // Put unnamed value
    V3_बहिर्गमनम्_Json_File& put(ध्रुव std::string& मानम्) { प्रत्यागम putNamed("", मानम्, true); }
    V3_बहिर्गमनम्_Json_File& put(bool मानम्) { प्रत्यागम putNamed("", मानम् ? "true" : "false", false); }
    V3_बहिर्गमनम्_Json_File& put(पूर्णाङ्क मानम्) { प्रत्यागम putNamed("", std::गन्तव्यम्_string(मानम्), false); }

    V3_बहिर्गमनम्_Json_File& putList(ध्रुव std::string& name, ध्रुव std::सदिशम्<std::string>& list) {
        यदि (list.रिक्तानि()) प्रत्यागम *this;
        begin(name, '[');
        प्रति (ध्रुव std::string& str : list) put(str);
        प्रत्यागम अन्तः_2();
    }

    V3_बहिर्गमनम्_Json_File& अन्तः_2() {
        UASSERT(m_उपसर्गः.दीर्घता_2() >= वाक्यदैर्घ्यम्(INDENT), "prefix underflow");
        m_उपसर्गः.erase(m_उपसर्गः.अन्तः_2() - वाक्यदैर्घ्यम्(INDENT), m_उपसर्गः.अन्तः_2());
        UASSERT(!m_scope.रिक्तानि(), "end() without begin()");
        UASSERT(!m_रिक्तानि.रिक्तानि(), "end() without begin()");
        पङ्क्तिं_लिख("\n" + m_उपसर्गः + m_scope.शिखरम्());
        m_scope.निष्कासय();
        m_रिक्तानि.निष्कासय();
        प्रत्यागम *this;
    }

    V3_बहिर्गमनम्_Json_File& संक्रियकः+=(V3_बहिर्गमनम्_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        प्रत्यागम *this;
    }

गुप्त:
    शून्य comma() {
        यदि (m_रिक्तानि.रिक्तानि()) प्रत्यागम;
        यदि (!m_रिक्तानि.शिखरम्()) पङ्क्तिं_लिख(",\n");
        m_रिक्तानि.शिखरम्() = true;
    }
    V3_बहिर्गमनम्_Json_File& putNamed(ध्रुव std::string& name, ध्रुव std::string& मानम्, bool quoted) {
        comma();
        ध्रुव string मानम्_Q
            = quoted ? "\""s + V3_बहिर्गमनम्_Formatter::quoteNameControls(मानम्) + "\"" : मानम्;
        यदि (name.रिक्तानि()) {
            पङ्क्तिं_लिख(m_उपसर्गः + मानम्_Q);
        } अन्यथा {
            पङ्क्तिं_लिख(m_उपसर्गः + "\"" + name + "\": " + मानम्_Q);
        }
        m_रिक्तानि.शिखरम्() = false;
        प्रत्यागम *this;
    }
};

class V3_बहिर्गमनम्_Mk_File final : सार्वजनिक V3_बहिर्गमनम्_File {
सार्वजनिक:
    explicit V3_बहिर्गमनम्_Mk_File(ध्रुव string& filename)
        : V3_बहिर्गमनम्_File{filename, V3_बहिर्गमनम्_Formatter::LA_MK} {}
    ~V3_बहिर्गमनम्_Mk_File() override = स्वभाव;
    आभास शून्य puts_शीर्षकम्() { पङ्क्तिं_लिख("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    शून्य पङ्क्तिं_लिख(ध्रुव वर्ण* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    शून्य पङ्क्तिं_लिख(ध्रुव string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    शून्य put_समुच्चयः(ध्रुव string& var, ध्रुव string& मानम्) {
        पङ्क्तिं_लिख(VString::dot(var + " =", " ", मानम्) + "\n");
    }
    // Put VARIABLE ?= VALUE
    शून्य put_समुच्चयः_Q(ध्रुव string& var, ध्रुव string& मानम्) {
        पङ्क्तिं_लिख(VString::dot(var + " ?=", " ", मानम्) + "\n");
    }
};

class V3_बहिर्गमनम्_VFile final : सार्वजनिक V3_बहिर्गमनम्_CFile {
सार्वजनिक:
    explicit V3_बहिर्गमनम्_VFile(ध्रुव string& filename)
        : V3_बहिर्गमनम्_CFile{filename, V3_बहिर्गमनम्_Formatter::LA_VERILOG} {}
    ~V3_बहिर्गमनम्_VFile() override = स्वभाव;
    शून्य puts_शीर्षकम्() override { पङ्क्तिं_लिख("// Verilated -*- Verilog -*-\n"); }
};

class V3_बहिर्गमनम्_Xml_File final : सार्वजनिक V3_बहिर्गमनम्_File {
सार्वजनिक:
    explicit V3_बहिर्गमनम्_Xml_File(ध्रुव string& filename)
        : V3_बहिर्गमनम्_File{filename, V3_बहिर्गमनम्_Formatter::LA_XML} {
        खण्डम्_Indent(2);
    }
    ~V3_बहिर्गमनम्_Xml_File() override = स्वभाव;
    आभास शून्य puts_शीर्षकम्() { पङ्क्तिं_लिख("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_बहिर्गमनम्_प्रवाहः VL_NOT_FINAL : सार्वजनिक V3_बहिर्गमनम्_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_बहिर्गमनम्_प्रवाहः);
    VL_UNMOVABLE(V3_बहिर्गमनम्_प्रवाहः);

सार्वजनिक:
    V3_बहिर्गमनम्_प्रवाहः(std::ostream& ostream, V3_बहिर्गमनम्_Formatter::Language lang);
    ~V3_बहिर्गमनम्_प्रवाहः() override = स्वभाव;

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
    स्थिर string ifNoProtect(ध्रुव string& अन्तर्) VL_MT_SAFE;
    // Rename to a new encoded string (unless earlier passthru'ed)
    स्थिर string protect(ध्रुव string& old) VL_MT_SAFE { प्रत्यागम protectIf(old, true); }
    स्थिर string protectIf(ध्रुव string& old, bool doIt = true) VL_MT_SAFE;
    // Rename words to a new encoded string
    स्थिर string protect_शब्दाः_If(ध्रुव string& old, bool doIt = true) VL_MT_SAFE;
    // Write map of renames to output file
    स्थिर शून्य writeMapFile(ध्रुव string& filename);
};

#यदि_अन्तः  // Guard
