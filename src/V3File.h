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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3FILE_H_
#وصف_ڪريو VERILATOR_V3FILE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3Stats.h"

#ضمن <array>
#ضمن <fstream>
#ضمن <list>
#ضمن <memory>
#ضمن <set>
#ضمن <stack>
#ضمن <vector>

class Ast_ڳنڍ;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
عام:
    ساكن std::ifstream* new_ifstream(ثابت string& filename) {
        جوڙيو_Src_Depend(filename);
        أرجع new_ifstream_nodepend(filename);
    }
    ساكن std::ifstream* new_ifstream_nodepend(ثابت string& filename) VL_MT_محفوظ {
        أرجع new std::ifstream{filename.c_str()};
    }
    ساكن std::ofstream* new_ofstream(ثابت string& filename, bool append = false) {
        جوڙيو_Tgt_Depend(filename);
        أرجع new_ofstream_nodepend(filename, append);
    }
    ساكن std::ofstream* new_ofstream_nodepend(ثابت string& filename, bool append = false) {
        createMakeDirFor(filename);
        إذا (append) أرجع new std::ofstream{filename.c_str(), std::ios::app};
        أرجع new std::ofstream{filename.c_str()};
    }
    ساكن FILE* new_fopen_w(ثابت string& filename) {
        createMakeDirFor(filename);
        جوڙيو_Tgt_Depend(filename);
        أرجع fopen(filename.c_str(), "w");
    }

    // Dependencies
    ساكن فراغ جوڙيو_Src_Depend(ثابت string& filename) VL_MT_محفوظ;
    ساكن فراغ جوڙيو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ;
    ساكن فراغ writeDepend(ثابت string& filename);
    ساكن std::ويڪٽر<string> getAllDeps();
    ساكن فراغ writeTimes(ثابت string& filename, ثابت string& cmdline_اندريون);
    ساكن bool checkTimes(ثابت string& filename, ثابت string& cmdline_اندريون);

    // Directory utilities
    ساكن فراغ createMakeDirFor(ثابت string& filename);
    ساكن فراغ createMakeDir();
};

//============================================================================
// VInFilter: Read a input file, possibly filtering it, and caching contents

class VInFilterImp;

class VInFilter final {
عام:
    // TYPES
    using StrList = std::list<std::string>;

خاص:
    VInFilterImp* m_impp;

    // CONSTRUCTORS
    VL_UNCOPYABLE(VInFilter);

عام:
    explicit VInFilter(ثابت string& command);
    ~VInFilter();

    // METHODS
    // Read file contents and return it.  Return true on success.
    bool readWholefile(ثابت string& filename, StrList& outl);
};

//============================================================================
// V3OutFormatter: A class for automatic indentation of output code.

class V3_ٻاهريون_Formatter VL_NOT_FINAL {
    // TYPES
عام:
    تعداد AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    تعداد Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

خاص:
    // MEMBERS
    ثابت Language m_lang;  // Indenting Verilog code
    صحيح m_بلاڪ_Indent;  // Characters per block indent
    صحيح m_comma_ويڪر;  // Width after which to break at ,'s
    صحيح m_lineno = 1;
    صحيح m_ڪالم = 0;
    صحيح m_nobreak = false;  // Basic operator or begin paren, don't break next
    صحيح m_ذريعو_آخري_Lineno = 0;
    صحيح m_ذريعو_آخري_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_اندريون_String_Literal = false;  // Inside a quote
    bool m_اندريون_Backslash = false;  // Inside a backslash
    صحيح m_indentLevel = 0;  // Current {} indentation
    std::ڍير<صحيح> m_parenVec;  // Stack of columns where last ( was
    صحيح m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    صحيح پڇاڙي_سطحون(ثابت حرف* strg);
    فراغ putcNoTracking(حرف chr);

عام:
    explicit V3_ٻاهريون_Formatter(Language lang);
    مجازي ~V3_ٻاهريون_Formatter() = افتراضي;
    // ACCESSORS
    فراغ بلاڪ_Indent(صحيح flag) { m_بلاڪ_Indent = flag; }
    // METHODS
    فراغ لکو(ثابت حرف* fmt...) VL_ATTR_PRINTF(2);
    فراغ puts(ثابت حرف* strg) { putns(nullptr, strg); }
    فراغ puts(ثابت string& strg) { putns(nullptr, strg); }
    فراغ putns(ثابت Ast_ڳنڍ* nodep, ثابت حرف* strg);
    فراغ putns(ثابت Ast_ڳنڍ* nodep, ثابت string& strg) { putns(nodep, strg.c_str()); }
    فراغ putsNoTracking(ثابت string& strg);
    فراغ putsQuoted(ثابت string& strg);
    فراغ putBreak();  // Print linebreak if line is too wide
    فراغ put_Break_اظهار();  // Print linebreak in expression if line is too wide
    فراغ putbs(ثابت حرف* strg) {
        put_Break_اظهار();
        putns(nullptr, strg);
    }
    فراغ putbs(ثابت string& strg) {
        put_Break_اظهار();
        putns(nullptr, strg);
    }
    فراغ putnbs(ثابت Ast_ڳنڍ* nodep, ثابت string& strg) {
        put_Break_اظهار();
        putns(nodep, strg);
    }
    bool exceeded_ويڪر() ثابت { أرجع m_ڪالم > m_comma_ويڪر; }
    فراغ indentInc() { m_indentLevel += m_بلاڪ_Indent; }
    فراغ indentDec() {
        m_indentLevel -= m_بلاڪ_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    فراغ بلاڪ_Inc() { m_parenVec.ڌڪو(m_indentLevel + m_بلاڪ_Indent); }
    فراغ بلاڪ_Dec() {
        إذا (!m_parenVec.خالي_جايون()) m_parenVec.ڪڍو();
    }
    فراغ ensure_New_سٽ() {
        إذا (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ساكن ثابت std::string& indentSpaces(صحيح انگ);
    // Add escaped characters to strings
    ساكن string quoteNameControls(ثابت string& namein, Language lang = LA_C) VL_خالص;
    ساكن bool token_ميلاپ(ثابت حرف* cp, ثابت حرف* cmp);
    ساكن bool token_Not_شروعات(ثابت حرف* cp);  // Import/export meaning no endfunction
    ساكن bool token_شروعات(ثابت حرف* cp);
    ساكن bool token_پڇاڙي(ثابت حرف* cp);

    // CALLBACKS - MUST OVERRIDE
    مجازي فراغ putcOutput(حرف chr) = 0;
    مجازي فراغ putsOutput(ثابت حرف* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_ٻاهريون_File VL_NOT_FINAL : عام V3_ٻاهريون_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ساكن constexpr std::سائيز_t WRITE_بفر_سائيز_بائيٽون = 128 * 1024;

    // MEMBERS
    ثابت std::string m_filename;
    FILE* m_fp = nullptr;
    std::سائيز_t m_استعمال_ٿيل_بائيٽون = 0;  // Number of bytes stored in m_bufferp
    std::سائيز_t m_written_بائيٽون = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<حرف, WRITE_بفر_سائيز_بائيٽون>> m_bufferp;  // Write buffer

عام:
    V3_ٻاهريون_File(ثابت string& filename, V3_ٻاهريون_Formatter::Language lang);
    V3_ٻاهريون_File(ثابت V3_ٻاهريون_File&) = delete;
    V3_ٻاهريون_File& عامل=(ثابت V3_ٻاهريون_File&) = delete;
    V3_ٻاهريون_File(V3_ٻاهريون_File&&) = delete;
    V3_ٻاهريون_File& عامل=(V3_ٻاهريون_File&&) = delete;
    ~V3_ٻاهريون_File() override;

    std::string filename() ثابت { أرجع m_filename; }

    فراغ putsForceIncs();

    فراغ stat_رڪارڊ_Written() {
        write_بلاڪ();
        V3Stats::جوڙيو_Stat_جوڙ(V3Stats::STAT_CPP_CHARS, m_written_بائيٽون);
    }

خاص:
    فراغ write_بلاڪ() {
        إذا (VL_LIKELY(m_استعمال_ٿيل_بائيٽون > 0)) {
            fwrite(m_bufferp->معلومات(), m_استعمال_ٿيل_بائيٽون, 1, m_fp);
            m_written_بائيٽون += m_استعمال_ٿيل_بائيٽون;
            m_استعمال_ٿيل_بائيٽون = 0;
        }
    }
    // CALLBACKS
    فراغ putcOutput(حرف chr) override {
        m_bufferp->at(m_استعمال_ٿيل_بائيٽون++) = chr;
        إذا (VL_UNLIKELY(m_استعمال_ٿيل_بائيٽون >= WRITE_بفر_سائيز_بائيٽون)) write_بلاڪ();
    }
    فراغ putsOutput(ثابت حرف* str) override {
        std::سائيز_t len = لڙي_ڊيگهه(str);
        std::سائيز_t موجود_بائيٽون = WRITE_بفر_سائيز_بائيٽون - m_استعمال_ٿيل_بائيٽون;
        طالما (VL_UNLIKELY(len >= موجود_بائيٽون)) {
            std::يادگيري_نقل(m_bufferp->معلومات() + m_استعمال_ٿيل_بائيٽون, str, موجود_بائيٽون);
            m_استعمال_ٿيل_بائيٽون = WRITE_بفر_سائيز_بائيٽون;
            write_بلاڪ();
            str += موجود_بائيٽون;
            len -= موجود_بائيٽون;
            موجود_بائيٽون = WRITE_بفر_سائيز_بائيٽون;
        }
        إذا (len > 0) {
            std::يادگيري_نقل(m_bufferp->معلومات() + m_استعمال_ٿيل_بائيٽون, str, len);
            m_استعمال_ٿيل_بائيٽون += len;
        }
    }
};

class V3_ٻاهريون_CFile VL_NOT_FINAL : عام V3_ٻاهريون_File {
    صحيح m_guard = false;  // Created header guard
    صحيح m_ذاتي;  // 1 = Most recently emitted private:, 2 = public:
عام:
    explicit V3_ٻاهريون_CFile(ثابت string& filename,
                        V3_ٻاهريون_Formatter::Language lang = V3_ٻاهريون_Formatter::LA_C)
        : V3_ٻاهريون_File{filename, lang} {
        reset_ذاتي();
    }
    ~V3_ٻاهريون_CFile() override { stat_رڪارڊ_Written(); }
    مجازي فراغ puts_سرو() { puts("// Verilated -*- C++ -*-\n"); }
    مجازي فراغ puts_Int_چوٽي_Include() { putsForceIncs(); }
    مجازي فراغ putsGuard();
    مجازي فراغ puts_پڇاڙي_Guard() {
        إذا (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    فراغ reset_ذاتي() { m_ذاتي = 0; }
    فراغ puts_ذاتي(bool سيٽ_ذاتي) {
        إذا (سيٽ_ذاتي && m_ذاتي != 1) {
            puts("private:\n");
            m_ذاتي = 1;
        } وإلا إذا (!سيٽ_ذاتي && m_ذاتي != 2) {
            puts("public:\n");
            m_ذاتي = 2;
        }
    }
};

class V3_ٻاهريون_Json_File final : عام V3_ٻاهريون_File {
    // CONSTANTS
    ساكن constexpr ثابت حرف* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
خاص:
    std::ڍير<حرف> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_اڳياڙي;  // Prefix emitted before each line in current scope
    std::ڍير<bool> m_خالي_جايون;  // Current scope is empty, no comma later

عام:
    explicit V3_ٻاهريون_Json_File(ثابت string& filename)
        : V3_ٻاهريون_File{filename, V3_ٻاهريون_Formatter::LA_JSON} {
        begin();
    }
    ~V3_ٻاهريون_Json_File() override {
        پڇاڙي();
        puts("\n");
    }
    مجازي فراغ puts_سرو() {}
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_ٻاهريون_Json_File& begin(ثابت std::string& name, حرف type = '{') {
        comma();
        puts(m_اڳياڙي + "\"" + name + "\": " + type + "\n");
        m_اڳياڙي += INDENT;
        m_scope.ڌڪو(type == '{' ? '}' : ']');
        إذا (!m_خالي_جايون.خالي_جايون()) m_خالي_جايون.چوٽي() = false;
        m_خالي_جايون.ڌڪو(true);
        أرجع *this;
    }
    V3_ٻاهريون_Json_File& begin(حرف type = '{') {
        comma();
        puts(m_اڳياڙي + type + "\n");
        m_اڳياڙي += INDENT;
        m_scope.ڌڪو(type == '{' ? '}' : ']');
        إذا (!m_خالي_جايون.خالي_جايون()) m_خالي_جايون.چوٽي() = false;
        m_خالي_جايون.ڌڪو(true);
        أرجع *this;
    }

    // Put a named value
    V3_ٻاهريون_Json_File& put(ثابت std::string& name, ثابت حرف* valuep) {
        أرجع putNamed(name, std::string{valuep}, true);
    }
    V3_ٻاهريون_Json_File& put(ثابت std::string& name, ثابت std::string& قدر) {
        أرجع putNamed(name, قدر, true);
    }
    V3_ٻاهريون_Json_File& put(ثابت std::string& name, bool قدر) {
        أرجع putNamed(name, قدر ? "true" : "false", false);
    }
    V3_ٻاهريون_Json_File& put(ثابت std::string& name, صحيح قدر) {
        أرجع putNamed(name, std::منزل_string(قدر), false);
    }

    // Put unnamed value
    V3_ٻاهريون_Json_File& put(ثابت std::string& قدر) { أرجع putNamed("", قدر, true); }
    V3_ٻاهريون_Json_File& put(bool قدر) { أرجع putNamed("", قدر ? "true" : "false", false); }
    V3_ٻاهريون_Json_File& put(صحيح قدر) { أرجع putNamed("", std::منزل_string(قدر), false); }

    V3_ٻاهريون_Json_File& putList(ثابت std::string& name, ثابت std::ويڪٽر<std::string>& list) {
        إذا (list.خالي_جايون()) أرجع *this;
        begin(name, '[');
        لكل (ثابت std::string& str : list) put(str);
        أرجع پڇاڙي();
    }

    V3_ٻاهريون_Json_File& پڇاڙي() {
        UASSERT(m_اڳياڙي.ڊيگهائي() >= لڙي_ڊيگهه(INDENT), "prefix underflow");
        m_اڳياڙي.erase(m_اڳياڙي.پڇاڙي() - لڙي_ڊيگهه(INDENT), m_اڳياڙي.پڇاڙي());
        UASSERT(!m_scope.خالي_جايون(), "end() without begin()");
        UASSERT(!m_خالي_جايون.خالي_جايون(), "end() without begin()");
        puts("\n" + m_اڳياڙي + m_scope.چوٽي());
        m_scope.ڪڍو();
        m_خالي_جايون.ڪڍو();
        أرجع *this;
    }

    V3_ٻاهريون_Json_File& عامل+=(V3_ٻاهريون_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        أرجع *this;
    }

خاص:
    فراغ comma() {
        إذا (m_خالي_جايون.خالي_جايون()) أرجع;
        إذا (!m_خالي_جايون.چوٽي()) puts(",\n");
        m_خالي_جايون.چوٽي() = true;
    }
    V3_ٻاهريون_Json_File& putNamed(ثابت std::string& name, ثابت std::string& قدر, bool quoted) {
        comma();
        ثابت string قدر_Q
            = quoted ? "\""s + V3_ٻاهريون_Formatter::quoteNameControls(قدر) + "\"" : قدر;
        إذا (name.خالي_جايون()) {
            puts(m_اڳياڙي + قدر_Q);
        } وإلا {
            puts(m_اڳياڙي + "\"" + name + "\": " + قدر_Q);
        }
        m_خالي_جايون.چوٽي() = false;
        أرجع *this;
    }
};

class V3_ٻاهريون_Mk_File final : عام V3_ٻاهريون_File {
عام:
    explicit V3_ٻاهريون_Mk_File(ثابت string& filename)
        : V3_ٻاهريون_File{filename, V3_ٻاهريون_Formatter::LA_MK} {}
    ~V3_ٻاهريون_Mk_File() override = افتراضي;
    مجازي فراغ puts_سرو() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    فراغ put_سيٽ(ثابت string& var, ثابت string& قدر) {
        puts(VString::dot(var + " =", " ", قدر) + "\n");
    }
    // Put VARIABLE ?= VALUE
    فراغ put_سيٽ_Q(ثابت string& var, ثابت string& قدر) {
        puts(VString::dot(var + " ?=", " ", قدر) + "\n");
    }
};

class V3_ٻاهريون_VFile final : عام V3_ٻاهريون_CFile {
عام:
    explicit V3_ٻاهريون_VFile(ثابت string& filename)
        : V3_ٻاهريون_CFile{filename, V3_ٻاهريون_Formatter::LA_VERILOG} {}
    ~V3_ٻاهريون_VFile() override = افتراضي;
    فراغ puts_سرو() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3_ٻاهريون_Xml_File final : عام V3_ٻاهريون_File {
عام:
    explicit V3_ٻاهريون_Xml_File(ثابت string& filename)
        : V3_ٻاهريون_File{filename, V3_ٻاهريون_Formatter::LA_XML} {
        بلاڪ_Indent(2);
    }
    ~V3_ٻاهريون_Xml_File() override = افتراضي;
    مجازي فراغ puts_سرو() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_ٻاهريون_وهڪرو VL_NOT_FINAL : عام V3_ٻاهريون_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_ٻاهريون_وهڪرو);
    VL_UNMOVABLE(V3_ٻاهريون_وهڪرو);

عام:
    V3_ٻاهريون_وهڪرو(std::ostream& ostream, V3_ٻاهريون_Formatter::Language lang);
    ~V3_ٻاهريون_وهڪرو() override = افتراضي;

    فراغ putcOutput(حرف chr) override { m_ostream << chr; };
    فراغ putsOutput(ثابت حرف* str) override { m_ostream << str; };
};

//============================================================================
// VIdProtect: Hash identifier names in output files to protect them

class VIdProtectImp;

class VIdProtect final {
عام:
    // METHODS
    // Return 'in' only if not protecting (e.g. for emitting a comment)
    ساكن string ifNoProtect(ثابت string& اندريون) VL_MT_محفوظ;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ساكن string protect(ثابت string& old) VL_MT_محفوظ { أرجع protectIf(old, true); }
    ساكن string protectIf(ثابت string& old, bool doIt = true) VL_MT_محفوظ;
    // Rename words to a new encoded string
    ساكن string protect_لفظ_If(ثابت string& old, bool doIt = true) VL_MT_محفوظ;
    // Write map of renames to output file
    ساكن فراغ writeMapFile(ثابت string& filename);
};

#جيڪڏهن_پورو  // Guard
