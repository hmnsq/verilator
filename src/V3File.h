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

#اگر_تعریف_نشده VERILATOR_V3FILE_H_
#تعریف_کن VERILATOR_V3FILE_H_

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

class Ast_گره;

//============================================================================
// V3File: Create streams, recording dependency information

class V3File final {
عام:
    ساكن std::ifstream* new_ifstream(ثابت string& filename) {
        جمع_کن_Src_Depend(filename);
        أرجع new_ifstream_nodepend(filename);
    }
    ساكن std::ifstream* new_ifstream_nodepend(ثابت string& filename) VL_MT_امن {
        أرجع new std::ifstream{filename.c_str()};
    }
    ساكن std::ofstream* new_ofstream(ثابت string& filename, bool append = false) {
        جمع_کن_Tgt_Depend(filename);
        أرجع new_ofstream_nodepend(filename, append);
    }
    ساكن std::ofstream* new_ofstream_nodepend(ثابت string& filename, bool append = false) {
        createMakeDirFor(filename);
        إذا (append) أرجع new std::ofstream{filename.c_str(), std::ios::app};
        أرجع new std::ofstream{filename.c_str()};
    }
    ساكن FILE* new_fopen_w(ثابت string& filename) {
        createMakeDirFor(filename);
        جمع_کن_Tgt_Depend(filename);
        أرجع fopen(filename.c_str(), "w");
    }

    // Dependencies
    ساكن فراغ جمع_کن_Src_Depend(ثابت string& filename) VL_MT_امن;
    ساكن فراغ جمع_کن_Tgt_Depend(ثابت string& filename) VL_MT_امن;
    ساكن فراغ writeDepend(ثابت string& filename);
    ساكن std::وکتور<string> getAllDeps();
    ساكن فراغ writeTimes(ثابت string& filename, ثابت string& cmdline_درون);
    ساكن bool checkTimes(ثابت string& filename, ثابت string& cmdline_درون);

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

class V3_بیرون_Formatter VL_NOT_FINAL {
    // TYPES
عام:
    تعداد AlignClass : uint8_t { AL_AUTO = 0, AL_STATIC = 1 };
    تعداد Language : uint8_t { LA_C, LA_JSON, LA_MK, LA_VERILOG, LA_XML };

خاص:
    // MEMBERS
    ثابت Language m_lang;  // Indenting Verilog code
    صحيح m_بلوک_Indent;  // Characters per block indent
    صحيح m_comma_عرض;  // Width after which to break at ,'s
    صحيح m_lineno = 1;
    صحيح m_ستونی = 0;
    صحيح m_nobreak = false;  // Basic operator or begin paren, don't break next
    صحيح m_منبع_آخرین_Lineno = 0;
    صحيح m_منبع_آخرین_Filenameno = 0;
    bool m_prependIndent = true;
    bool m_درون_String_Literal = false;  // Inside a quote
    bool m_درون_Backslash = false;  // Inside a backslash
    صحيح m_indentLevel = 0;  // Current {} indentation
    std::پشته<صحيح> m_parenVec;  // Stack of columns where last ( was
    صحيح m_bracketLevel = 0;  // Indenting = { block, indicates number of {'s seen.

    صحيح انتها_سطوح(ثابت حرف* strg);
    فراغ putcNoTracking(حرف chr);

عام:
    explicit V3_بیرون_Formatter(Language lang);
    مجازی ~V3_بیرون_Formatter() = افتراضي;
    // ACCESSORS
    فراغ بلوک_Indent(صحيح flag) { m_بلوک_Indent = flag; }
    // METHODS
    فراغ چاپ_کن(ثابت حرف* fmt...) VL_ATTR_PRINTF(2);
    فراغ puts(ثابت حرف* strg) { putns(nullptr, strg); }
    فراغ puts(ثابت string& strg) { putns(nullptr, strg); }
    فراغ putns(ثابت Ast_گره* nodep, ثابت حرف* strg);
    فراغ putns(ثابت Ast_گره* nodep, ثابت string& strg) { putns(nodep, strg.c_str()); }
    فراغ putsNoTracking(ثابت string& strg);
    فراغ putsQuoted(ثابت string& strg);
    فراغ putBreak();  // Print linebreak if line is too wide
    فراغ put_Break_عبارت();  // Print linebreak in expression if line is too wide
    فراغ putbs(ثابت حرف* strg) {
        put_Break_عبارت();
        putns(nullptr, strg);
    }
    فراغ putbs(ثابت string& strg) {
        put_Break_عبارت();
        putns(nullptr, strg);
    }
    فراغ putnbs(ثابت Ast_گره* nodep, ثابت string& strg) {
        put_Break_عبارت();
        putns(nodep, strg);
    }
    bool exceeded_عرض() ثابت { أرجع m_ستونی > m_comma_عرض; }
    فراغ indentInc() { m_indentLevel += m_بلوک_Indent; }
    فراغ indentDec() {
        m_indentLevel -= m_بلوک_Indent;
        UASSERT(m_indentLevel >= 0, "Underflow of indentation");
    }
    فراغ بلوک_Inc() { m_parenVec.افزودن(m_indentLevel + m_بلوک_Indent); }
    فراغ بلوک_Dec() {
        إذا (!m_parenVec.خالی()) m_parenVec.برداشت();
    }
    فراغ ensure_New_سطر() {
        إذا (!m_nobreak) puts("\n");
    }
    // STATIC METHODS
    ساكن ثابت std::string& indentSpaces(صحيح رقم);
    // Add escaped characters to strings
    ساكن string quoteNameControls(ثابت string& namein, Language lang = LA_C) VL_خالص;
    ساكن bool token_تطابق(ثابت حرف* cp, ثابت حرف* cmp);
    ساكن bool token_Not_شروع(ثابت حرف* cp);  // Import/export meaning no endfunction
    ساكن bool token_شروع(ثابت حرف* cp);
    ساكن bool token_انتها(ثابت حرف* cp);

    // CALLBACKS - MUST OVERRIDE
    مجازی فراغ putcOutput(حرف chr) = 0;
    مجازی فراغ putsOutput(ثابت حرف* str) = 0;
};

//============================================================================
// V3OutFile: A class for printing to a file, with automatic indentation of C++ code.

class V3_بیرون_File VL_NOT_FINAL : عام V3_بیرون_Formatter {
    // Size of m_bufferp.
    // 128kB has been experimentally determined to be in the zone of buffer sizes that work best.
    // It is also considered to be the smallest I/O buffer size in GNU coreutils (io_blksize) that
    // allows to best minimize syscall overhead.
    // The hard boundaries are CPU L2/L3 cache size on the top and filesystem block size
    // on the bottom.
    ساكن constexpr std::اندازه_t WRITE_میانگیر_اندازه_بایتها = 128 * 1024;

    // MEMBERS
    ثابت std::string m_filename;
    FILE* m_fp = nullptr;
    std::اندازه_t m_استفاده_شده_بایتها = 0;  // Number of bytes stored in m_bufferp
    std::اندازه_t m_written_بایتها = 0;  // Number of bytes written to output
    std::unique_ptr<std::array<حرف, WRITE_میانگیر_اندازه_بایتها>> m_bufferp;  // Write buffer

عام:
    V3_بیرون_File(ثابت string& filename, V3_بیرون_Formatter::Language lang);
    V3_بیرون_File(ثابت V3_بیرون_File&) = delete;
    V3_بیرون_File& عملگر=(ثابت V3_بیرون_File&) = delete;
    V3_بیرون_File(V3_بیرون_File&&) = delete;
    V3_بیرون_File& عملگر=(V3_بیرون_File&&) = delete;
    ~V3_بیرون_File() override;

    std::string filename() ثابت { أرجع m_filename; }

    فراغ putsForceIncs();

    فراغ stat_رکورد_Written() {
        write_بلوک();
        V3Stats::جمع_کن_Stat_جمع(V3Stats::STAT_CPP_CHARS, m_written_بایتها);
    }

خاص:
    فراغ write_بلوک() {
        إذا (VL_LIKELY(m_استفاده_شده_بایتها > 0)) {
            fwrite(m_bufferp->داده(), m_استفاده_شده_بایتها, 1, m_fp);
            m_written_بایتها += m_استفاده_شده_بایتها;
            m_استفاده_شده_بایتها = 0;
        }
    }
    // CALLBACKS
    فراغ putcOutput(حرف chr) override {
        m_bufferp->at(m_استفاده_شده_بایتها++) = chr;
        إذا (VL_UNLIKELY(m_استفاده_شده_بایتها >= WRITE_میانگیر_اندازه_بایتها)) write_بلوک();
    }
    فراغ putsOutput(ثابت حرف* str) override {
        std::اندازه_t len = طول_رشته(str);
        std::اندازه_t موجود_بایتها = WRITE_میانگیر_اندازه_بایتها - m_استفاده_شده_بایتها;
        طالما (VL_UNLIKELY(len >= موجود_بایتها)) {
            std::کپی_حافظه(m_bufferp->داده() + m_استفاده_شده_بایتها, str, موجود_بایتها);
            m_استفاده_شده_بایتها = WRITE_میانگیر_اندازه_بایتها;
            write_بلوک();
            str += موجود_بایتها;
            len -= موجود_بایتها;
            موجود_بایتها = WRITE_میانگیر_اندازه_بایتها;
        }
        إذا (len > 0) {
            std::کپی_حافظه(m_bufferp->داده() + m_استفاده_شده_بایتها, str, len);
            m_استفاده_شده_بایتها += len;
        }
    }
};

class V3_بیرون_CFile VL_NOT_FINAL : عام V3_بیرون_File {
    صحيح m_guard = false;  // Created header guard
    صحيح m_خصوصی;  // 1 = Most recently emitted private:, 2 = public:
عام:
    explicit V3_بیرون_CFile(ثابت string& filename,
                        V3_بیرون_Formatter::Language lang = V3_بیرون_Formatter::LA_C)
        : V3_بیرون_File{filename, lang} {
        reset_خصوصی();
    }
    ~V3_بیرون_CFile() override { stat_رکورد_Written(); }
    مجازی فراغ puts_سرآیند() { puts("// Verilated -*- C++ -*-\n"); }
    مجازی فراغ puts_Int_بالاترین_Include() { putsForceIncs(); }
    مجازی فراغ putsGuard();
    مجازی فراغ puts_انتها_Guard() {
        إذا (m_guard) puts("\n#endif  // guard\n");
    }
    // Print out public/privates
    فراغ reset_خصوصی() { m_خصوصی = 0; }
    فراغ puts_خصوصی(bool مجموعه_خصوصی) {
        إذا (مجموعه_خصوصی && m_خصوصی != 1) {
            puts("private:\n");
            m_خصوصی = 1;
        } وإلا إذا (!مجموعه_خصوصی && m_خصوصی != 2) {
            puts("public:\n");
            m_خصوصی = 2;
        }
    }
};

class V3_بیرون_Json_File final : عام V3_بیرون_File {
    // CONSTANTS
    ساكن constexpr ثابت حرف* INDENT = "    ";  // Single indent (4, per JSON std)

    // MEMBERS
خاص:
    std::پشته<حرف> m_scope;  // Stack of ']' and '}'  to close currently open scopes
    std::string m_پیشوند;  // Prefix emitted before each line in current scope
    std::پشته<bool> m_خالی;  // Current scope is empty, no comma later

عام:
    explicit V3_بیرون_Json_File(ثابت string& filename)
        : V3_بیرون_File{filename, V3_بیرون_Formatter::LA_JSON} {
        begin();
    }
    ~V3_بیرون_Json_File() override {
        انتها();
        puts("\n");
    }
    مجازی فراغ puts_سرآیند() {}
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }

    // METHODS
    V3_بیرون_Json_File& begin(ثابت std::string& name, حرف type = '{') {
        comma();
        puts(m_پیشوند + "\"" + name + "\": " + type + "\n");
        m_پیشوند += INDENT;
        m_scope.افزودن(type == '{' ? '}' : ']');
        إذا (!m_خالی.خالی()) m_خالی.بالاترین() = false;
        m_خالی.افزودن(true);
        أرجع *this;
    }
    V3_بیرون_Json_File& begin(حرف type = '{') {
        comma();
        puts(m_پیشوند + type + "\n");
        m_پیشوند += INDENT;
        m_scope.افزودن(type == '{' ? '}' : ']');
        إذا (!m_خالی.خالی()) m_خالی.بالاترین() = false;
        m_خالی.افزودن(true);
        أرجع *this;
    }

    // Put a named value
    V3_بیرون_Json_File& put(ثابت std::string& name, ثابت حرف* valuep) {
        أرجع putNamed(name, std::string{valuep}, true);
    }
    V3_بیرون_Json_File& put(ثابت std::string& name, ثابت std::string& مقدار) {
        أرجع putNamed(name, مقدار, true);
    }
    V3_بیرون_Json_File& put(ثابت std::string& name, bool مقدار) {
        أرجع putNamed(name, مقدار ? "true" : "false", false);
    }
    V3_بیرون_Json_File& put(ثابت std::string& name, صحيح مقدار) {
        أرجع putNamed(name, std::مقصد_string(مقدار), false);
    }

    // Put unnamed value
    V3_بیرون_Json_File& put(ثابت std::string& مقدار) { أرجع putNamed("", مقدار, true); }
    V3_بیرون_Json_File& put(bool مقدار) { أرجع putNamed("", مقدار ? "true" : "false", false); }
    V3_بیرون_Json_File& put(صحيح مقدار) { أرجع putNamed("", std::مقصد_string(مقدار), false); }

    V3_بیرون_Json_File& putList(ثابت std::string& name, ثابت std::وکتور<std::string>& list) {
        إذا (list.خالی()) أرجع *this;
        begin(name, '[');
        لكل (ثابت std::string& str : list) put(str);
        أرجع انتها();
    }

    V3_بیرون_Json_File& انتها() {
        UASSERT(m_پیشوند.درازی() >= طول_رشته(INDENT), "prefix underflow");
        m_پیشوند.erase(m_پیشوند.انتها() - طول_رشته(INDENT), m_پیشوند.انتها());
        UASSERT(!m_scope.خالی(), "end() without begin()");
        UASSERT(!m_خالی.خالی(), "end() without begin()");
        puts("\n" + m_پیشوند + m_scope.بالاترین());
        m_scope.برداشت();
        m_خالی.برداشت();
        أرجع *this;
    }

    V3_بیرون_Json_File& عملگر+=(V3_بیرون_Json_File& /*cursor*/) {
        // Meaningless syntax sugar, at least for now
        أرجع *this;
    }

خاص:
    فراغ comma() {
        إذا (m_خالی.خالی()) أرجع;
        إذا (!m_خالی.بالاترین()) puts(",\n");
        m_خالی.بالاترین() = true;
    }
    V3_بیرون_Json_File& putNamed(ثابت std::string& name, ثابت std::string& مقدار, bool quoted) {
        comma();
        ثابت string مقدار_Q
            = quoted ? "\""s + V3_بیرون_Formatter::quoteNameControls(مقدار) + "\"" : مقدار;
        إذا (name.خالی()) {
            puts(m_پیشوند + مقدار_Q);
        } وإلا {
            puts(m_پیشوند + "\"" + name + "\": " + مقدار_Q);
        }
        m_خالی.بالاترین() = false;
        أرجع *this;
    }
};

class V3_بیرون_Mk_File final : عام V3_بیرون_File {
عام:
    explicit V3_بیرون_Mk_File(ثابت string& filename)
        : V3_بیرون_File{filename, V3_بیرون_Formatter::LA_MK} {}
    ~V3_بیرون_Mk_File() override = افتراضي;
    مجازی فراغ puts_سرآیند() { puts("# Verilated -*- Makefile -*-\n"); }
    // No automatic indentation yet.
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت حرف* strg) { putsNoTracking(strg); }
    // cppcheck-suppress duplInheritedMember
    فراغ puts(ثابت string& strg) { putsNoTracking(strg); }
    // Put VARIABLE = VALUE
    فراغ put_مجموعه(ثابت string& var, ثابت string& مقدار) {
        puts(VString::dot(var + " =", " ", مقدار) + "\n");
    }
    // Put VARIABLE ?= VALUE
    فراغ put_مجموعه_Q(ثابت string& var, ثابت string& مقدار) {
        puts(VString::dot(var + " ?=", " ", مقدار) + "\n");
    }
};

class V3_بیرون_VFile final : عام V3_بیرون_CFile {
عام:
    explicit V3_بیرون_VFile(ثابت string& filename)
        : V3_بیرون_CFile{filename, V3_بیرون_Formatter::LA_VERILOG} {}
    ~V3_بیرون_VFile() override = افتراضي;
    فراغ puts_سرآیند() override { puts("// Verilated -*- Verilog -*-\n"); }
};

class V3_بیرون_Xml_File final : عام V3_بیرون_File {
عام:
    explicit V3_بیرون_Xml_File(ثابت string& filename)
        : V3_بیرون_File{filename, V3_بیرون_Formatter::LA_XML} {
        بلوک_Indent(2);
    }
    ~V3_بیرون_Xml_File() override = افتراضي;
    مجازی فراغ puts_سرآیند() { puts("<?xml version=\"1.0\" ?>\n"); }
};

//============================================================================
// V3OutStream: A class for printing formatted code to any std::ostream

class V3_بیرون_جریان VL_NOT_FINAL : عام V3_بیرون_Formatter {
    // MEMBERS
    std::ostream& m_ostream;

    VL_UNCOPYABLE(V3_بیرون_جریان);
    VL_UNMOVABLE(V3_بیرون_جریان);

عام:
    V3_بیرون_جریان(std::ostream& ostream, V3_بیرون_Formatter::Language lang);
    ~V3_بیرون_جریان() override = افتراضي;

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
    ساكن string ifNoProtect(ثابت string& درون) VL_MT_امن;
    // Rename to a new encoded string (unless earlier passthru'ed)
    ساكن string protect(ثابت string& old) VL_MT_امن { أرجع protectIf(old, true); }
    ساكن string protectIf(ثابت string& old, bool doIt = true) VL_MT_امن;
    // Rename words to a new encoded string
    ساكن string protect_واژگان_If(ثابت string& old, bool doIt = true) VL_MT_امن;
    // Write map of renames to output file
    ساكن فراغ writeMapFile(ثابت string& filename);
};

#پایان_اگر  // Guard
