// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Options parsing
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

#ضمن "V3PchAstMT.h"

#ضمن "V3Options.h"

#ضمن "V3Container.h"
#ضمن "V3Error.h"
#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3Mutex.h"
#ضمن "V3OptionParser.h"
#ضمن "V3Os.h"
#ضمن "V3PreShell.h"
#ضمن "V3String.h"

// clang-format off
#ضمن <sys/types.h>
#ضمن <sys/stat.h>
#اگر_تعریف_نشده _WIN32
# include <sys/utsname.h>
#پایان_اگر
#ضمن <algorithm>
#ضمن <cctype>
#اگر_تعریف_شده _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(نما) (((نما) & _S_IFMT) == _S_IFDIR)
#وإلا
# include <dirent.h>
#پایان_اگر
#ضمن <fcntl.h>
#ضمن <list>
#ضمن <map>
#ضمن <memory>
#ضمن <thread>
#ضمن <set>
#ضمن <string>

#ضمن "config_rev.h"

#إذا defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#پایان_اگر
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

صنف V3OptionsImp final {
عام:
    // TYPES
    استفاده DirMap = std::map<ثابت string, std::مجموعه<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_سطر_آرگومانها;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, منطقي>> m_all_آرگومانها;
    VInsertion_مجموعه<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::مجموعه<string> m_inc_Dir_Fallback_مجموعه;  // Include directories (for removing duplicates)
    std::map<ثابت string, V3_Lang_کد> m_langExts;  // Language extension map
    VInsertion_مجموعه<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    فراغ جمع_کن_Inc_Dir_User(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        ثابت منطقي inserted = m_incDirUsers.درج(dir);
        إذا (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_inc_Dir_Fallback_مجموعه.erase(dir);  // User has priority over Fallback
        }
    }
    فراغ جمع_کن_Inc_Dir_Fallback(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        إذا (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ثابت خودکار it_یافت_Pair = m_inc_Dir_Fallback_مجموعه.درج(dir);
            إذا (it_یافت_Pair.دومی) m_incDirFallbacks.افزودن_پشت(dir);
        }
    }
    فراغ جمع_کن_Lang_Ext(ثابت string& langext, ثابت V3_Lang_کد& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        إذا (addext[0] == '.') addext = addext.substr(1);
        (فراغ)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    فراغ جمع_کن_Lib_Ext_V(ثابت string& libext) { m_libExtVs.درج(libext); }
    V3OptionsImp() = افتراضي;
    ~V3OptionsImp() = افتراضي;
};

//######################################################################
// V3LangCode class functions

V3_Lang_کد::V3_Lang_کد(ثابت حرف* textp) {
    // Return code for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3_Lang_کد::L_ERROR; codei < V3_Lang_کد::ENUM_انتها; ++codei) {
        ثابت V3_Lang_کد کد{codei};
        إذا (0 == VL_STRCASECMP(textp, کد.ascii())) {
            m_e = کد;
            أرجع;
        }
    }
    m_e = V3_Lang_کد::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ثابت string& مقدار, منطقي& badr)
    : m_e{VTimescale::NONE} {
    badr = درست;
    ثابت string spaceless = VString::removeWhitespace(مقدار);
    لكل (صحيح i = TS_100S; i < ENUM_انتها; ++i) {
        ثابت VTimescale ts{i};
        إذا (spaceless == ts.ascii()) {
            badr = خطأ;
            m_e = ts.m_e;
            اكسر;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3_سلسلهمراتبی_بلوک_Option::V3_سلسلهمراتبی_بلوک_Option(ثابت string& opts) {
    VStringList vals;
    منطقي درون_Str = خطأ;
    string cur;
    ساكن ثابت string hier_بلوک("--hierarchical-block");
    File_سطر cmdfl(File_سطر::command_سطر_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    لكل (string::const_iterator it = opts.begin(); it != opts.انتها();) {
        إذا (درون_Str) {
            إذا (*it == '\\') {
                ++it;
                إذا (it == opts.انتها()) {
                    cmdfl.v3error(hier_بلوک + " must not end with \\");
                    اكسر;
                }
                إذا (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hier_بلوک + " does not allow '" + *it + "' after \\");
                    اكسر;
                }
                cur.افزودن_پشت(*it);
                ++it;
            } وإلا إذا (*it == '"') {  // end of string
                cur.افزودن_پشت(*it);
                vals.افزودن_پشت(cur);
                cur.clear();
                ++it;
                إذا (it != opts.انتها()) {
                    إذا (*it != ',') {
                        cmdfl.v3error(hier_بلوک + " expects ',', but '" + *it + "' is passed");
                        اكسر;
                    }
                    ++it;
                    إذا (it == opts.انتها()) {
                        cmdfl.v3error(hier_بلوک + " must not end with ','");
                        اكسر;
                    }
                    درون_Str = *it == '"';
                    cur.افزودن_پشت(*it);
                    ++it;
                }
            } وإلا {
                cur.افزودن_پشت(*it);
                ++it;
            }
        } وإلا {
            إذا (*it == '"') {
                cmdfl.v3error(hier_بلوک + " does not allow '\"' in the middle of literal");
                اكسر;
            }
            إذا (*it == ',') {  // end of this parameter
                vals.افزودن_پشت(cur);
                cur.clear();
                ++it;
                إذا (it == opts.انتها()) {
                    cmdfl.v3error(hier_بلوک + " must not end with ','");
                    اكسر;
                }
                درون_Str = *it == '"';
            }
            cur.افزودن_پشت(*it);
            ++it;
        }
    }
    إذا (!cur.خالی()) vals.افزودن_پشت(cur);
    إذا (vals.اندازه() >= 2) {
        إذا (vals.اندازه() % 2) {
            cmdfl.v3error(hier_بلوک + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } وإلا {
        cmdfl.v3error(hier_بلوک + " requires at least two comma-separated values");
    }
    لكل (اندازه_t i = 2; i + 1 < vals.اندازه(); i += 2) {
        ثابت منطقي inserted = m_parameters.emplace(vals[i], vals[i + 1]).دومی;
        إذا (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hier_بلوک);
        }
    }
}

//######################################################################
// V3Options class functions

فراغ VTimescale::parseSlashed(File_سطر* fl, ثابت حرف* textp, VTimescale& unitr,
                              VTimescale& precr, منطقي allow_خالی) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ثابت حرف* cp = textp;
    لكل (; std::آیا_فاصله(*cp); ++cp) {}
    ثابت حرف* ثابت unitp = cp;
    لكل (; *cp && *cp != '/'; ++cp) {}
    ثابت string unitStr(unitp, cp - unitp);
    لكل (; std::آیا_فاصله(*cp); ++cp) {}
    string precStr;
    إذا (*cp == '/') {
        ++cp;
        لكل (; std::آیا_فاصله(*cp); ++cp) {}
        ثابت حرف* ثابت precp = cp;
        لكل (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    لكل (; std::آیا_فاصله(*cp); ++cp) {}
    إذا (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        أرجع;
    }

    منطقي unitbad = خطأ;
    ثابت VTimescale unit{unitStr, unitbad /*ref*/};
    إذا (unitbad && !(unitStr.خالی() && allow_خالی)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        أرجع;
    }
    unitr = unit;

    إذا (!precStr.خالی()) {
        VTimescale prec{VTimescale::NONE};
        منطقي precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        إذا (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            أرجع;
        }
        إذا (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            أرجع;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

فراغ V3Options::جمع_کن_Inc_Dir_User(ثابت string& incdir) { m_impp->جمع_کن_Inc_Dir_User(incdir); }
فراغ V3Options::جمع_کن_Inc_Dir_Fallback(ثابت string& incdir) { m_impp->جمع_کن_Inc_Dir_Fallback(incdir); }
فراغ V3Options::جمع_کن_Lang_Ext(ثابت string& langext, ثابت V3_Lang_کد& lc) {
    m_impp->جمع_کن_Lang_Ext(langext, lc);
}
فراغ V3Options::جمع_کن_Lib_Ext_V(ثابت string& libext) { m_impp->جمع_کن_Lib_Ext_V(libext); }
فراغ V3Options::جمع_کن_Define(ثابت string& defline, منطقي allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string چپ = defline;
    طالما (چپ != "") {
        string def = چپ;
        string::اندازه_type pos;
        إذا (allowPlus && ((pos = چپ.پیدا_کن('+')) != string::npos)) {
            چپ = چپ.substr(pos + 1);
            def.erase(pos);
        } وإلا {
            چپ = "";
        }
        string مقدار;
        إذا ((pos = def.پیدا_کن('=')) != string::npos) {
            مقدار = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_سطر(def, مقدار);
    }
}
فراغ V3Options::جمع_کن_Parameter(ثابت string& paramline, منطقي allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string چپ = paramline;
    طالما (چپ != "") {
        string param = چپ;
        string::اندازه_type pos;
        إذا (allowPlus && ((pos = چپ.پیدا_کن('+')) != string::npos)) {
            چپ = چپ.substr(pos + 1);
            param.erase(pos);
        } وإلا {
            چپ = "";
        }
        string مقدار;
        إذا ((pos = param.پیدا_کن('=')) != string::npos) {
            مقدار = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << مقدار);
        (فراغ)m_parameters.erase(param);
        m_parameters[param] = مقدار;
    }
}

منطقي V3Options::hasParameter(ثابت string& name) {
    أرجع m_parameters.پیدا_کن(name) != m_parameters.انتها();
}

string V3Options::parameter(ثابت string& name) {
    string مقدار = m_parameters.پیدا_کن(name)->دومی;
    m_parameters.erase(m_parameters.پیدا_کن(name));
    أرجع مقدار;
}

فراغ V3Options::checkParameters() {
    إذا (!m_parameters.خالی()) {
        std::stringstream پیام_2;
        پیام_2 << "Parameters from the command line were not found in the design:";
        لكل (ثابت خودکار& i : m_parameters) پیام_2 << " " << i.اولی;
        v3error(پیام_2.str());
    }
}

فراغ V3Options::جمع_کن_Cpp_File(ثابت string& filename) { m_cppFiles.درج(filename); }
فراغ V3Options::جمع_کن_CFlags(ثابت string& filename) { m_cFlags.افزودن_پشت(filename); }
فراغ V3Options::جمع_کن_Lib_Map_File(ثابت string& filename) { m_libmapFiles.درج(filename); }
فراغ V3Options::جمع_کن_Compiler_Includes(ثابت string& filename) {
    m_compilerIncludes.درج(filename);
}
فراغ V3Options::جمع_کن_Ld_Lib_File(ثابت string& filename) {
    m_ldLibFiles.افزودن_پشت(filename);
    جمع_کن_Ld_Libs(filename);
}
فراغ V3Options::جمع_کن_Ld_Libs(ثابت string& filename) { m_ldLibs.افزودن_پشت(filename); }
فراغ V3Options::جمع_کن_Make_Flags(ثابت string& filename) { m_makeFlags.افزودن_پشت(filename); }
فراغ V3Options::جمع_کن_Future(ثابت string& flag) { m_futures.درج(flag); }
فراغ V3Options::جمع_کن_Future0(ثابت string& flag) { m_future0s.درج(flag); }
فراغ V3Options::جمع_کن_Future1(ثابت string& flag) { m_future1s.درج(flag); }
منطقي V3Options::isFuture(ثابت string& flag) ثابت {
    أرجع m_futures.پیدا_کن(flag) != m_futures.انتها();
}
منطقي V3Options::isFuture0(ثابت string& flag) ثابت {
    أرجع m_future0s.پیدا_کن(flag) != m_future0s.انتها();
}
منطقي V3Options::isFuture1(ثابت string& flag) ثابت {
    أرجع m_future1s.پیدا_کن(flag) != m_future1s.انتها();
}
منطقي V3Options::isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت {
    أرجع m_libraryFiles.پیدا_کن({filename, libname}) != m_libraryFiles.انتها();
}
فراغ V3Options::جمع_کن_Library_File(ثابت string& filename, ثابت string& libname) {
    m_libraryFiles.درج({filename, libname});
}
فراغ V3Options::جمع_کن_VFile(ثابت string& filename, ثابت string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.افزودن_پشت({filename, libname});  // Not emplace
}
فراغ V3Options::جمع_کن_Vlt_File(ثابت string& filename, ثابت string& libname) {
    m_vltFiles.درج({filename, libname});
}
فراغ V3Options::جمع_کن_Force_Inc(ثابت string& filename) { m_forceIncs.افزودن_پشت(filename); }

فراغ V3Options::جمع_کن_سطر_Arg(ثابت string& arg) { m_impp->m_سطر_آرگومانها.افزودن_پشت(arg); }

فراغ V3Options::جمع_کن_Arg(حرف** argv, اندازه_t شمار, منطقي isForRerun) {
    m_impp->m_all_آرگومانها.emplace_پشت();
    std::pair<std::list<std::string>, منطقي>& pair = m_impp->m_all_آرگومانها.پشت();
    pair.دومی = isForRerun;
    لكل (اندازه_t n = 0; n < شمار; ++n) pair.اولی.emplace_پشت(argv[n]);
}

فراغ V3Options::جمع_کن_Arg(ثابت std::string& arg, منطقي isForRerun) {
    m_impp->m_all_آرگومانها.emplace_پشت();
    std::pair<std::list<std::string>, منطقي>& pair = m_impp->m_all_آرگومانها.پشت();
    pair.دومی = isForRerun;
    pair.اولی.emplace_پشت(arg);
}

ثابت std::list<std::pair<std::list<std::string>, منطقي>>& V3Options::all_آرگومانها() ثابت {
    أرجع m_impp->m_all_آرگومانها;
}

string V3Options::all_آرگومانها_String() ثابت VL_MT_امن {
    string نتیجه;
    لكل (ثابت خودکار& pair : m_impp->m_all_آرگومانها) {
        لكل (ثابت string& arg : pair.اولی) {
            إذا (!نتیجه.خالی()) نتیجه += " ";
            نتیجه += arg;
        }
    }
    أرجع نتیجه;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::all_آرگومانها_String_For_Hier_بلوک(منطقي for_بالاترین) ثابت {
    // cppcheck-suppress shadowFunction
    std::مجموعه<string> vFiles;
    لكل (ثابت خودکار& vFile : m_vFiles) vFiles.درج(vFile.filename());
    string بیرون;
    منطقي stripArg = خطأ;
    منطقي strip_Arg_If_رقم = خطأ;
    لكل (ثابت خودکار& pair : m_impp->m_all_آرگومانها) {
        لكل (ثابت string& arg : pair.اولی) {
            إذا (stripArg) {
                stripArg = خطأ;
                استمر;
            }
            إذا (strip_Arg_If_رقم) {
                strip_Arg_If_رقم = خطأ;
                إذا (آیا_رقم(arg[0])) استمر;
            }
            صحيح skip = 0;
            إذا (arg.درازی() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } وإلا إذا (arg.درازی() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            إذا (skip > 0) {  // arg is an option
                ثابت string opt = arg.substr(skip);  // Remove '-' in the beginning
                ثابت صحيح رقم_Strip = strip_Options_For_فرزند_پیاپی(opt, for_بالاترین);
                إذا (رقم_Strip) {
                    UASSERT(0 <= رقم_Strip && رقم_Strip <= 3, "should be one of 0, 1, 2, 3");
                    إذا (رقم_Strip == 2) stripArg = درست;
                    إذا (رقم_Strip == 3) strip_Arg_If_رقم = درست;
                    استمر;
                }
            } وإلا {  // Not an option
                إذا (vFiles.پیدا_کن(arg) != vFiles.انتها()  // Remove HDL
                    || m_cppFiles.پیدا_کن(arg) != m_cppFiles.انتها()) {  // Remove C++
                    استمر;
                }
            }
            إذا (بیرون != "") بیرون += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            بیرون += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    أرجع بیرون;
}

فراغ V3Options::cc_مجموعه() {  // --cc
    m_بیرون_Format_Ok = درست;
    m_systemC = خطأ;
}

فراغ V3Options::decorations(File_سطر* fl, ثابت string& arg) {  // --decorations
    إذا (arg == "none") {
        m_decoration = خطأ;
        m_decorationNodes = خطأ;
    } وإلا إذا (arg == "node") {
        m_decoration = درست;
        m_decorationNodes = درست;
    } وإلا إذا (arg == "medium") {
        m_decoration = درست;
        m_decorationNodes = خطأ;
    } وإلا {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warn_بیشتر() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::وکتور<std::string> V3Options::traceClassBases() ثابت VL_MT_امن {
    std::وکتور<std::string> نتیجه;
    إذا (traceEnabledFst()) نتیجه.emplace_پشت("VerilatedFst");
    إذا (traceEnabledSaif()) نتیجه.emplace_پشت("VerilatedSaif");
    إذا (traceEnabledVcd()) نتیجه.emplace_پشت("VerilatedVcd");
    أرجع نتیجه;
}
std::وکتور<std::string> V3Options::traceClassLangs() ثابت VL_MT_امن {
    std::وکتور<std::string> نتیجه;
    لكل (ثابت خودکار& cbase : traceClassBases())
        نتیجه.emplace_پشت(cbase + (systemC() ? "Sc" : "C"));
    أرجع نتیجه;
}
std::وکتور<std::string> V3Options::trace_منبع_Bases() ثابت VL_MT_امن {
    std::وکتور<std::string> نتیجه;
    إذا (traceEnabledFst()) نتیجه.emplace_پشت("verilated_fst");
    إذا (traceEnabledSaif()) نتیجه.emplace_پشت("verilated_saif");
    إذا (traceEnabledVcd()) نتیجه.emplace_پشت("verilated_vcd");
    أرجع نتیجه;
}
std::وکتور<std::string> V3Options::trace_منبع_Langs() ثابت VL_MT_امن {
    std::وکتور<std::string> نتیجه = trace_منبع_Bases();
    لكل (std::string& str : نتیجه) str += systemC() ? "_sc"s : "_c"s;
    أرجع نتیجه;
}
std::string V3Options::trace_Class_پایه() ثابت VL_MT_امن {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().خالی(), "Call traceClassBase only when trace() enabled");
    أرجع traceClassBases().front();
}
std::string V3Options::traceClassLang() ثابت VL_MT_امن {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().خالی(), "Call traceClassLang only when trace() enabled");
    أرجع traceClassLangs().front();
}

//######################################################################
// File searching

منطقي V3Options::file_Stat_نرمال(ثابت string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    بنية stat sstat;  // Stat information
    ثابت صحيح err = stat(filename.c_str(), &sstat);
    إذا (err != 0) أرجع خطأ;
    إذا (S_ISDIR(sstat.st_نما)) أرجع خطأ;
    أرجع درست;
}

string V3Options::fileExists(ثابت string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ثابت string dir = V3Os::filenameDir(filename);
    ثابت string basename = V3Os::filenameNonDir(filename);

    خودکار diriter = m_impp->m_dirMap.پیدا_کن(dir);
    إذا (diriter == m_impp->m_dirMap.انتها()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::مجموعه<string>());
        diriter = m_impp->m_dirMap.پیدا_کن(dir);

        std::مجموعه<string>* ثابت setp = &(diriter->دومی);

#اگر_تعریف_شده _MSC_VER
        حاول {
            لكل (ثابت خودکار& dir_مدخل : std::filesystem::directory_iterator(dir.c_str()))
                setp->درج(dir_مدخل.مسیر().filename().string());
        } امسك (std::filesystem::filesystem_error ثابت& ex) {
            (فراغ)ex;
            أرجع "";
        }
#وإلا
        إذا (DIR* ثابت dirp = opendir(dir.c_str())) {
            طالما (بنية dirent* direntp = readdir(dirp)) setp->درج(direntp->d_name);
            closedir(dirp);
        }
#پایان_اگر
    }
    // Find it
    ثابت std::مجموعه<string>* ثابت filesetp = &(diriter->دومی);
    ثابت خودکار fileiter = filesetp->پیدا_کن(basename);
    إذا (fileiter == filesetp->انتها()) أرجع "";  // Not found
    // Check if it is a directory, ignore if so
    ثابت string filename_بیرون = V3Os::filenameJoin(dir, basename);
    إذا (!file_Stat_نرمال(filename_بیرون)) أرجع "";  // Directory
    أرجع filename_بیرون;
}

string V3Options::file_مسیر_Check_One_Dir(ثابت string& modname, ثابت string& dirname) {
    لكل (ثابت string& i : m_impp->m_libExtVs) {
        ثابت string fn = V3Os::filenameJoin(dirname, modname + i);
        ثابت string exists = fileExists(fn);
        إذا (exists != "") أرجع exists;
    }
    أرجع "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
صحيح V3Options::strip_Options_For_فرزند_پیاپی(ثابت string& opt, منطقي for_بالاترین) {
    // Options to strip for both the top wrapper, and hier blocks
    ساكن ثابت std::unordered_map<std::string, صحيح> commonOpts{
        //
        {"j", 3},
        //
        {"Mdir", 2},
        {"clk", 2},
        {"lib-create", 2},
        {"f", 2},
        {"F", 2},
        {"v", 2},
        {"l2-name", 2},
        {"mod-prefix", 2},
        {"prefix", 2},
        {"protect-lib", 2},
        {"protect-key", 2},
        {"threads", 2},
        {"top-module", 2},
        //
        {"build", 1},
        {"hierarchical", 1},
    };
    إذا (commonOpts.شمار(opt)) أرجع commonOpts.at(opt);

    // Options to strip only for hier blocks
    إذا (!for_بالاترین) {
        ساكن ثابت std::unordered_map<std::string, صحيح> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        إذا (subOpts.شمار(opt)) أرجع subOpts.at(opt);
        إذا (VString::startsWith(opt, "G")) أرجع 1;
        إذا (VString::startsWith(opt, "pvalue")) أرجع 1;
    }

    // Do not strip
    أرجع 0;
}

فراغ V3Options::validateIdentifier(File_سطر* fl, ثابت string& arg, ثابت string& opt) {
    إذا (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::file_مسیر(File_سطر* fl, ثابت string& modname, ثابت string& lastpath,
                           ثابت string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ثابت string filename = V3Os::filenameCleanup(VName::dehash(modname));
    إذا (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ثابت string exists = file_مسیر_Check_One_Dir(filename, "");
        إذا (exists != "") أرجع exists;
    }
    لكل (ثابت string& dir : m_impp->m_incDirUsers) {
        ثابت string exists = file_مسیر_Check_One_Dir(filename, dir);
        إذا (exists != "") أرجع exists;
    }
    لكل (ثابت string& dir : m_impp->m_incDirFallbacks) {
        ثابت string exists = file_مسیر_Check_One_Dir(filename, dir);
        إذا (exists != "") أرجع exists;
    }

    إذا (m_relativeIncludes) {
        ثابت string exists = file_مسیر_Check_One_Dir(filename, lastpath);
        إذا (exists != "") أرجع V3Os::filename_Real_مسیر(exists);
    }

    // Warn and return not found
    إذا (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_مسیر_Looked_پیام(fl, filename));
    }
    أرجع "";
}

string V3Options::file_مسیر_Looked_پیام(File_سطر* /*fl*/, ثابت string& modname) {
    ساكن منطقي s_shown_notfound_پیام = خطأ;
    std::ostringstream ss;
    إذا (modname.پیدا_کن("__Vhsh") != string::npos) {
        ss << V3Error::warn_بیشتر() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warn_بیشتر() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } وإلا إذا (!s_shown_notfound_پیام) {
        s_shown_notfound_پیام = درست;
        إذا (m_impp->m_incDirUsers.خالی()) {
            ss << V3Error::warn_بیشتر()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warn_بیشتر() << "... Looked in:\n";
        لكل (ثابت string& dir : m_impp->m_incDirUsers) {
            لكل (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_بیشتر() << "     " << fn << "\n";
            }
        }
        لكل (ثابت string& dir : m_impp->m_incDirFallbacks) {
            لكل (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_بیشتر() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warn_بیشتر() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    أرجع ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3_Lang_کد V3Options::fileLanguage(ثابت string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::اندازه_type pos;
    إذا (filename == V3Options::get_Std_Package_مسیر() || filename == V3Options::get_Std_Waiver_مسیر()) {
        أرجع V3_Lang_کد::mostRecent();
    } وإلا إذا ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ثابت خودکار it = m_impp->m_langExts.پیدا_کن(ext);
        إذا (it != m_impp->m_langExts.انتها()) أرجع it->دومی;
    }
    أرجع m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ثابت string& var) {
    // If update below, also update V3Options::showVersion()
    إذا (var == "MAKE") {
        أرجع getenvMAKE();
    } وإلا إذا (var == "PERL") {
        أرجع getenvPERL();
    } وإلا إذا (var == "PYTHON3") {
        أرجع getenvPYTHON3();
    } وإلا إذا (var == "SYSTEMC") {
        أرجع getenvSYSTEMC();
    } وإلا إذا (var == "SYSTEMC_ARCH") {
        أرجع getenvSYSTEMC_ARCH();
    } وإلا إذا (var == "SYSTEMC_INCLUDE") {
        أرجع getenvSYSTEMC_INCLUDE();
    } وإلا إذا (var == "SYSTEMC_LIBDIR") {
        أرجع getenvSYSTEMC_LIBDIR();
    } وإلا إذا (var == "VERILATOR_ROOT") {
        أرجع getenv_VERILATOR_ریشه();
    } وإلا {
        أرجع V3Os::getenvStr(var, "");
    }
}

#اگر_تعریف_شده __FreeBSD__
string V3Options::getenvMAKE() { أرجع V3Os::getenvStr("MAKE", "gmake"); }
#وإلا
string V3Options::getenvMAKE() { أرجع V3Os::getenvStr("MAKE", "make"); }
#پایان_اگر

string V3Options::getenvMAKEFLAGS() {  //
    أرجع V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    أرجع V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    أرجع V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC};
    إذا (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    أرجع V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_ARCH};
    إذا (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    إذا (var == "") {
#إذا defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ثابت string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#وإلاإذا defined(_WIN32)
        ثابت string sysname = "WIN32";
        var = "win32";
#وإلا
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        بنية utsname uts;
        uname(&uts);
        ثابت string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        إذا (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } وإلا إذا (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } وإلا {
            var = "linux";
        }
#پایان_اگر
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    أرجع var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    إذا (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    إذا (var == "") {
        ثابت string sc = getenvSYSTEMC();
        إذا (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    أرجع V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    إذا (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    إذا (var == "") {
        ثابت string sc = getenvSYSTEMC();
        ثابت string arch = getenvSYSTEMC_ARCH();
        إذا (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    أرجع V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_ریشه() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_VERILATOR_ریشه};
    إذا (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    إذا (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    أرجع V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_VERILATOR_SOLVER};
    إذا (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    أرجع var;
}

string V3Options::get_Std_Package_مسیر() {
    أرجع V3Os::filenameJoin(getenv_VERILATOR_ریشه(), "include", "verilated_std.sv");
}
string V3Options::get_Std_Waiver_مسیر() {
    أرجع V3Os::filenameJoin(getenv_VERILATOR_ریشه(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ثابت string& var) {
    // If update below, also update V3Options::showVersion()
    إذا (var == "COROUTINES" && coroutineSupport()) {
        أرجع "1";
    } وإلا إذا (var == "TSAN" && tsanSupport()) {
        أرجع "1";
    } وإلا إذا (var == "DEV_ASAN" && devAsan()) {
        أرجع "1";
    } وإلا إذا (var == "DEV_GCOV" && devGcov()) {
        أرجع "1";
        // cppcheck-suppress knownConditionTrueFalse
    } وإلا إذا (var == "SYSTEMC" && systemCFound()) {
        أرجع "1";
    } وإلا {
        أرجع "";
    }
}

منطقي V3Options::systemCSystemWide() {
#اگر_تعریف_شده HAVE_SYSTEMC
    أرجع درست;
#وإلا
    أرجع خطأ;
#پایان_اگر
}

منطقي V3Options::systemCFound() {
    أرجع (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().خالی() && !getenvSYSTEMC_LIBDIR().خالی()));
}

منطقي V3Options::coroutineSupport() {
#اگر_تعریف_شده HAVE_COROUTINES
    أرجع درست;
#وإلا
    أرجع خطأ;
#پایان_اگر
}

منطقي V3Options::devAsan() {
#اگر_تعریف_شده HAVE_DEV_ASAN
    أرجع درست;
#وإلا
    أرجع خطأ;
#پایان_اگر
}

منطقي V3Options::devGcov() {
#اگر_تعریف_شده HAVE_DEV_GCOV
    أرجع درست;
#وإلا
    أرجع خطأ;
#پایان_اگر
}

منطقي V3Options::tsanSupport() {
#اگر_تعریف_شده HAVE_TSAN
    أرجع درست;
#وإلا
    أرجع خطأ;
#پایان_اگر
}

//######################################################################
// V3 Options notification methods

فراغ V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_سطر* ثابت cmdfl = جديد File_سطر{File_سطر::command_سطر_Filename()};

    إذا (!بیرون_Format_Ok() && v3Global.opt.اصلی()) cc_مجموعه();  // --main implies --cc if not provided
    إذا (!بیرون_Format_Ok() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    إذا (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::وکتور<std::string> backendFlags;
    إذا (m_build) {
        إذا (m_دودویی)
            backendFlags.emplace_پشت("--binary");
        وإلا
            backendFlags.emplace_پشت("--build");
    }
    إذا (m_preprocOnly) backendFlags.emplace_پشت("-E");
    إذا (m_dpiHdrOnly) backendFlags.emplace_پشت("--dpi-hdr-only");
    إذا (m_lintOnly) backendFlags.emplace_پشت("--lint-only");
    إذا (m_jsonOnly) backendFlags.emplace_پشت("--json-only");
    إذا (backendFlags.اندازه() > 1) {
        std::string backendFlagsString = backendFlags.front();
        لكل (اندازه_t i = 1; i < backendFlags.اندازه(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    إذا (m_exe && !v3Global.opt.libCreate().خالی()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    إذا (!m_gmake && !m_makeJson) m_gmake = درست;

    إذا (m_سلسلهمراتبی && (m_hier_فرزند || !m_hier_بلوکها.خالی())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    إذا (m_hier_فرزند) {
        إذا (m_hier_بلوکها.خالی()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = خطأ;
    }

    إذا (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = درست;  // No format, with --trace means wanted --trace-vcd
    }
    إذا (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = درست;
    ثابت صحيح ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    إذا (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    إذا (protectIds()) {
        إذا (all_عمومی()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warn_بیشتر() + "... Suggest remove --public.");
        }
        إذا (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warn_بیشتر() + "... Suggest remove --trace.");
        }
        إذا (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warn_بیشتر() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    إذا (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.مجموعه_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    إذا (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.مجموعه_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    إذا (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.مجموعه_True_Or_False(درست);

    // Default split limits if not specified
    إذا (m_output_تقسیم_CFuncs < 0) m_output_تقسیم_CFuncs = m_output_تقسیم;
    إذا (m_output_تقسیم_CTrace < 0) m_output_تقسیم_CTrace = m_output_تقسیم;

    إذا (v3Global.opt.اصلی() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    إذا (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        إذا (سلسلهمراتبی()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    إذا (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    إذا (v3Global.opt.timing().is_مجموعه_True() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    إذا (!m_dumpLevel.شمار("tree") && m_dumpLevel.شمار("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(نخها() >= 1, "'threads()' must return a value >= 1");
    إذا (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    إذا (m_buildJobs == -1) m_buildJobs = 1;
    إذا (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    إذا (timing().is_مجموعه_True()) V3PreShell::define_Cmd_سطر("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    إذا (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.مجموعه_True_Or_False(خطأ);

    // === Leave last
    // Mark options as available
    m_موجود = درست;
}

//######################################################################
// V3 Options accessors

string V3Options::نسخه() VL_خالص {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_مقصد_Str(DTVERSION_rev);
    أرجع ver;
}

string V3Options::protect_کلید_Defaulted() VL_MT_امن {
    ساكن V3Mutex s_mutex;
    ثابت V3_قفل_Guard قفل_2{s_mutex};
    إذا (m_protect_کلید.خالی()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_کلید = "VL-KEY-" + digest.digestBase64();
    }
    أرجع m_protect_کلید;
}

فراغ V3Options::throwSigsegv() {  // LCOV_EXCL_START
#إذا !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<متطاير حرف*>(اشاره_تهی) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#پایان_اگر
}  // LCOV_EXCL_STOP

VTimescale V3Options::زمان_Compute_Prec(ثابت VTimescale& flag) ثابت {
    إذا (!زمان_Override_Prec().isNone()) {
        أرجع زمان_Override_Prec();
    } وإلا إذا (flag.isNone()) {
        أرجع زمان_Default_Prec();
    } وإلا {
        أرجع flag;
    }
}

VTimescale V3Options::زمان_Compute_Unit(ثابت VTimescale& flag) ثابت {
    إذا (!زمان_Override_Unit().isNone()) {
        أرجع زمان_Override_Unit();
    } وإلا إذا (flag.isNone()) {
        أرجع زمان_Default_Unit();
    } وإلا {
        أرجع flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(صحيح argc, حرف** argv) {
    // Return list of arguments as simple string
    string opts;
    لكل (صحيح i = 0; i < argc; ++i) {
        إذا (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    أرجع opts;
}

//######################################################################
// V3 Options Parsing

فراغ V3Options::parseOpts(File_سطر* fl, صحيح argc, حرف** argv) VL_MT_DISABLED {
    // Save command line options
    لكل (صحيح i = 0; i < argc; ++i) جمع_کن_سطر_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    إذا (vFiles().خالی()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    إذا (پیشوند() == "" && بالاترین_Module() != "") m_پیشوند = "V"s + بالاترین_Module();  // Already encoded
    إذا (پیشوند() == "" && vFiles().اندازه() >= 1)
        m_پیشوند
            = "V"s + Ast_گره::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    إذا (mod_پیشوند() == "") m_mod_پیشوند = پیشوند();

    // Find files in makedir
    جمع_کن_Inc_Dir_Fallback(makeDir());
}

//======================================================================

منطقي V3Options::suffixed(ثابت string& sw, ثابت حرف* arg) {
    إذا (std::طول_رشته(arg) > sw.درازی()) أرجع خطأ;
    أرجع (0 == std::strcmp(sw.c_str() + sw.درازی() - std::طول_رشته(arg), arg));
}

فراغ V3Options::parseOptsList(File_سطر* fl, ثابت string& optdir, صحيح argc,
                              حرف** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ثابت V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_برچسبها;

    ثابت خودکار callStrSetter = [هذا](فراغ (V3Options::*cbStr)(ثابت string&)) {
        أرجع [هذا, cbStr](ثابت string& v) { (هذا->*cbStr)(v); };
    };
    // Usage
    // DECL_OPTION("-option", action, pointer_or_lambda);
    // action: one of Set, OnOff, CbCall, CbOnOff, CbVal, CbPartialMatch, and CbPartialMatchVal
    //   Set              : Set value to a variable, pointer_or_lambda must be a pointer to the
    //                      variable.
    //                      true is set to bool-ish variable when '-opt' is passed to verilator.
    //                      val is set to int and string variable when '-opt val' is passed.
    //   OnOff            : Set value to a bool-ish variable, pointer_or_lambda must be a pointer
    //                      to bool or VOptionBool.
    //                      true is set if "-opt" is passed to verilator while false is set if
    //                      "-no-opt" is given.
    //   CbCall           : Call lambda or function that does not take argument.
    //   CbOnOff          : Call lambda or function that takes bool argument.
    //                      Supports "-opt" and "-no-opt" style options.
    //   CbVal            : Call lambda or function that takes int or const char*.
    //                      "-opt val" is passed to verilator, val is passed to the lambda.
    //                      If a function to be called is a member of V3Options that only takes
    //                      const string&, callStrSetter(&V3Options::memberFunc) can be passed
    //                      instead of lambda as a syntax sugar.
    //   CbPartialMatch   : Call lambda or function that takes remaining string.
    //                      e.g. DECL_OPTION("-opt-", CbPartialMatch, [](const char*optp) { cout <<
    //                      optp << endl; }); and "-opt-ABC" is passed, "ABC" will be emit to
    //                      stdout.
    //   CbPartialMatchVal: Call lambda or function that takes remaining string and value.
    //                      e.g. DECL_OPTION("-opt-", CbPartialMatchVal, [](const char*optp, const
    //                      char*valp) {
    //                               cout << optp << ":" << valp << endl; });
    //                      and "-opt-ABC VAL" is passed, "ABC:VAL" will be emit to stdout.
    //
    // DECL_OPTION is not C-macro to get correct line coverage even when lambda is passed.
    // (If DECL_OPTION is a macro, then lambda would be collapsed into a single line).

    // Plus options
    DECL_OPTION("+define+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) VL_MT_DISABLED {
        جمع_کن_Define(optp, درست);
    }).notForRerun();
    DECL_OPTION("+incdir+", Cb_Partial_تطابق, [هذا, &optdir](ثابت حرف* optp) {
        string dirs = optp;
        string::اندازه_type pos;
        طالما ((pos = dirs.پیدا_کن('+')) != string::npos) {
            جمع_کن_Inc_Dir_User(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        جمع_کن_Inc_Dir_User(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        string exts = optp;
        string::اندازه_type pos;
        طالما ((pos = exts.پیدا_کن('+')) != string::npos) {
            جمع_کن_Lib_Ext_V(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        جمع_کن_Lib_Ext_V(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        جمع_کن_Lang_Ext(optp, V3_Lang_کد::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](منطقي) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [هذا](منطقي flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ثابت حرف*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [هذا](منطقي flag) {
        m_bboxUnsup = flag;
        File_سطر::globalWarnOff(V3_Error_کد::E_UNSUPPORTED, درست);
    });
    DECL_OPTION("-binary", CbCall, [هذا]() {
        m_دودویی = درست;
        m_build = درست;
        m_exe = درست;
        m_main = درست;
        إذا (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", مجموعه_2, &m_build);
    DECL_OPTION("-build-dep-bin", مجموعه_2, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [هذا, fl](ثابت حرف* valp) {
        صحيح val = std::atoi(valp);
        إذا (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } وإلا إذا (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::جمع_کن_CFlags));
    DECL_OPTION("-cc", CbCall, [هذا]() { cc_مجموعه(); });
    DECL_OPTION("-clk", CbVal, [fl](ثابت std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ثابت std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", مجموعه_2, &m_comp_حد_بلوکها).undocumented();
    DECL_OPTION("-comp-limit-members", مجموعه_2,
                &m_comp_حد_Members)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", مجموعه_2, &m_comp_حد_Parens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](صحيح val) { VName::max_درازی(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "clang")) {
            m_comp_حد_بلوکها = 80;  // limit unknown
            m_comp_حد_Members = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_comp_حد_Parens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } وإلا إذا (!std::strcmp(valp, "gcc")) {
            m_comp_حد_بلوکها = 0;  // Bug free
            m_comp_حد_Members = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_comp_حد_Parens = 240;  // Unlimited, but generate same code as for clang
        } وإلا إذا (!std::strcmp(valp, "msvc")) {
            m_comp_حد_بلوکها = 80;  // 128, but allow some room
            m_comp_حد_Members = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_comp_حد_Parens = 80;  // 128, but allow some room
        } وإلا {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warn_بیشتر() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::جمع_کن_Compiler_Includes));
    DECL_OPTION("-converge-limit", مجموعه_2, &m_converge_حد);
    DECL_OPTION("-coverage", CbOnOff, [هذا](منطقي flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_عبارت);
    DECL_OPTION("-coverage-expr-max", مجموعه_2, &m_coverage_عبارت_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [هذا, fl](ثابت حرف* ثابت valp) {
        إذا (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } وإلا إذا (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } وإلا إذا (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::پر;
        } وإلا {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warn_بیشتر() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", مجموعه_2, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_سطر);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ثابت خودکار parse_Bins_حد = [fl](ثابت حرف* optp, ثابت حرف* valp, uint32_t& limitr) {
        حرف* endp = اشاره_تهی;
        ثابت بیعلامت دراز دراز مقدار = std::strtoull(valp, &endp, 10);
        إذا (*endp || مقدار < 1 || مقدار > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            أرجع;
        }
        limitr = static_cast<uint32_t>(مقدار);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [هذا, parse_Bins_حد](ثابت حرف* valp) {
        parse_Bins_حد("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [هذا, parse_Bins_حد](ثابت حرف* valp) {
        parse_Bins_حد("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", مجموعه_2, &m_coverage_Max_عرض);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverage_ادغام_Instances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coverage_در_هر_Instance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", Cb_Partial_تطابق, [هذا](ثابت حرف* valp) VL_MT_DISABLED {
        جمع_کن_Define(valp, خطأ);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [هذا]() { مجموعه_Debug_نما(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [هذا](صحيح v) { مجموعه_Debug_نما(v); }).notForRerun();
    DECL_OPTION("-debugi-", Cb_Partial_تطابق_Val, [هذا](ثابت حرف* optp, ثابت حرف* valp) {
        m_debugLevel[optp] = std::atoi(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#اگر_تعریف_شده VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", مجموعه_2, &m_debugAllocRandom).undocumented();
#پایان_اگر
    DECL_OPTION("-debug-check", OnOff, &m_debugCheck);
    DECL_OPTION("-debug-collision", OnOff, &m_debugCollision).undocumented();
    DECL_OPTION("-debug-emitv", OnOff, &m_debugEmitV).undocumented();
    DECL_OPTION("-debug-exit-elab", OnOff, &m_debugExitElab).undocumented();
    DECL_OPTION("-debug-exit-parse", OnOff, &m_debugExitParse).undocumented();
    DECL_OPTION("-debug-fatalsrc", CbCall, []() {
        v3fatalSrc("--debug-fatal-src");
    }).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-leak", OnOff, &m_debugLeak);
    DECL_OPTION("-debug-nondeterminism", OnOff, &m_debugNondeterminism).undocumented();
    DECL_OPTION("-debug-options", OnOff, &m_debugOptions).undocumented();
    DECL_OPTION("-debug-partition", OnOff, &m_debug_افراز).undocumented();
    DECL_OPTION("-debug-preproc-passthru", OnOff, &m_debugPreprocPassthru).undocumented();
    DECL_OPTION("-debug-protect", OnOff, &m_debugProtect).undocumented();
    DECL_OPTION("-debug-runtime-timeout", مجموعه_2, &m_debugRuntimeTimeout).undocumented();
    DECL_OPTION("-debug-self-test", OnOff, &m_debugSelfTest).undocumented();
    DECL_OPTION("-debug-sigsegv", CbCall, throwSigsegv).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_پشته_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debug_عرض).undocumented();
    DECL_OPTION("-decoration", CbCall, [هذا, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [هذا, fl](ثابت حرف* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [هذا, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [هذا](ثابت حرف* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = درست;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", Cb_Partial_تطابق_Val, [هذا](ثابت حرف* optp, ثابت حرف* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [هذا](منطقي flag) {
        إذا (flag) {
            m_stdPackage = خطأ;
            m_stdWaiver = خطأ;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<فراغ (*)(صحيح)>(&V3Error::error_حد));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [هذا](ثابت حرف* valp) { m_expand_حد = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [هذا, fl, &optdir](ثابت حرف* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), درست);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [هذا, &optdir](ثابت حرف* valp) { جمع_کن_Force_Inc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [هذا, fl, &optdir](ثابت حرف* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), خطأ);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [هذا](ثابت حرف* valp) { جمع_کن_Future0(valp); });
    DECL_OPTION("-future1", CbVal, [هذا](ثابت حرف* valp) { جمع_کن_Future1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_f_بیت_پیمایش_Loops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [هذا](منطقي flag) {
        m_fCaseDecoder = flag;
        m_f_Case_جدول = flag;
        m_f_Case_درخت = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_f_Case_جدول);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_درخت);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_بیت_Op_درخت);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [هذا](منطقي flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [هذا](منطقي flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_افزودن_پایین_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [هذا, fl](منطقي flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [هذا](منطقي flag) {  //
        m_f_Func_تقسیم_Cat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_f_Func_تقسیم_Cat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [هذا](منطقي flag) {  //
        m_fIcoChangeDetect.مجموعه_True_Or_False(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_f_ادغام_Cond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_f_ادغام_Cond_Motion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_f_برش);
    DECL_OPTION("-fslice-element-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_f_برش_Element_حد = std::atoi(valp);
        إذا (m_f_برش_Element_حد < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_f_تقسیم);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_f_جدول);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) { جمع_کن_Parameter(optp, خطأ); });
    DECL_OPTION("-gate-stmts", مجموعه_2, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [هذا]() {
        خروجی << protect_کلید_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ثابت حرف* valp) {
        خروجی << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ثابت حرف* valp) {
        خروجی << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_سلسلهمراتبی);
    DECL_OPTION("-hierarchical-block", CbVal, [هذا](ثابت حرف* valp) {
        ثابت V3_سلسلهمراتبی_بلوک_Option opt{valp};
        m_hier_بلوکها.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", مجموعه_2, &m_hier_فرزند);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [هذا](ثابت حرف* optp) { m_hierParamsFile.افزودن_پشت({optp, work()}); });

    DECL_OPTION("-I", Cb_Partial_تطابق, [هذا, &optdir](ثابت حرف* optp) {
        جمع_کن_Inc_Dir_User(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", مجموعه_2, &m_if_عمق);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", مجموعه_2, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", مجموعه_2, &m_inline_CFuncs_حاصلضرب);
    DECL_OPTION("-inline-mult", مجموعه_2, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [هذا, fl](صحيح val) {
        m_instr_شمار_Dpi = val;
        إذا (m_instr_شمار_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [هذا](ثابت حرف* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = درست;
    });
    DECL_OPTION("-json-only-output", CbVal, [هذا](ثابت حرف* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = درست;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::جمع_کن_Ld_Libs));
    DECL_OPTION("-l2-name", مجموعه_2, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [هذا]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [هذا]() { m_l2Name = "v"; }).undocumented();  // Historical
    ثابت خودکار مجموعه_Lang = [هذا, fl](ثابت حرف* valp) {
        ثابت V3_Lang_کد optval{valp};
        إذا (optval.legal()) {
            m_defaultLanguage = optval;
        } وإلا {
            VSpellCheck spell;
            لكل (صحيح i = V3_Lang_کد::L_ERROR + 1; i < V3_Lang_کد::ENUM_انتها; ++i) {
                spell.افزودن_Candidate(V3_Lang_کد{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.بهترین_Candidate_پیام(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, مجموعه_Lang).notForRerun();
    DECL_OPTION("-language", CbVal, مجموعه_Lang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [هذا](ثابت حرف* valp) { جمع_کن_Lib_Map_File(valp); });
    DECL_OPTION("-lib-create", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", مجموعه_2, &m_localize_Max_اندازه);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::جمع_کن_Make_Flags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [هذا](ثابت حرف* valp) {
        m_makeDir = valp;
        جمع_کن_Inc_Dir_Fallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", مجموعه_2, &m_main_بالاترین_Name);
    DECL_OPTION("-make", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "gmake")) {
            m_gmake = درست;
        } وإلا إذا (!std::strcmp(valp, "json")) {
            m_makeJson = درست;
        } وإلا {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", مجموعه_2, &m_funcRecursion);
    DECL_OPTION("-max-num-width", مجموعه_2, &m_max_رقم_عرض);
    DECL_OPTION("-mod-prefix", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_mod_پیشوند = valp;
    });

    DECL_OPTION("-O0", CbCall, [هذا]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [هذا]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [هذا]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [هذا]() { optimize(3); });

    DECL_OPTION("-o", مجموعه_2, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](منطقي /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_outputGroups = std::atoi(valp);
        إذا (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_output_Groups_اندازه = std::atoll(valp);
        إذا (m_output_Groups_اندازه < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", مجموعه_2, &m_output_تقسیم);
    DECL_OPTION("-output-split-cfuncs", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_output_تقسیم_CFuncs = std::atoi(valp);
        إذا (m_output_تقسیم_CFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_output_تقسیم_CTrace = std::atoi(valp);
        إذا (m_output_تقسیم_CTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", مجموعه_2, &m_preproc_No_سطر);
    DECL_OPTION("-pins64", CbCall, [هذا]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [هذا]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_pinsBv = std::atoi(valp);
        إذا (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [هذا](منطقي flag) {
        m_pinsScUint = flag;
        إذا (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [هذا](منطقي flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [هذا](منطقي flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", مجموعه_2, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_پیشوند = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preproc_تعریف_میکند);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_preproc_Token_حد = std::atoi(valp);
        إذا (m_preproc_Token_حد <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [هذا]() { m_عمومی = خطأ; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [هذا]() { m_profC = m_profCFuncs = درست; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [هذا]() {
        m_profC = m_profCFuncs = درست;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", مجموعه_2, &m_protect_کلید);
    DECL_OPTION("-protect-lib", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = درست;
    });
    DECL_OPTION("-public", OnOff, &m_عمومی);
    DECL_OPTION("-public-depth", مجموعه_2, &m_عمومی_عمق);
    DECL_OPTION("-public-flat-rw", CbOnOff, [هذا](منطقي flag) {
        m_عمومی_Flat_RW = flag;
        v3Global.dpi(درست);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [هذا](منطقي flag) { m_عمومی_Ignore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [هذا](منطقي flag) {
        m_عمومی_Params = flag;
        v3Global.dpi(درست);
    });
    DECL_OPTION("-pvalue+", Cb_Partial_تطابق,
                [هذا](ثابت حرف* varp) { جمع_کن_Parameter(varp, خطأ); });

    DECL_OPTION("-quiet", CbOnOff, [هذا](منطقي flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_reloop_حد = std::atoi(valp);
        إذا (m_reloop_حد < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_replication_حد = std::atoi(valp);
        إذا (m_replication_حد < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [هذا, fl]() {
        decorations(fl, "node");
        جمع_کن_CFlags("-ggdb");
        جمع_کن_Ld_Libs("-ggdb");
        جمع_کن_CFlags("-fsanitize=address,undefined");
        جمع_کن_Ld_Libs("-fsanitize=address,undefined");
        جمع_کن_CFlags("-D_GLIBCXX_DEBUG");
        جمع_کن_CFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [هذا]() {
        m_بیرون_Format_Ok = درست;
        m_systemC = درست;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [هذا](منطقي flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [هذا](منطقي flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [هذا]() { m_defaultLanguage = V3_Lang_کد::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [هذا, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_نخها = 1;
    });
    DECL_OPTION("-threads", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_نخها = std::atoi(valp);
        إذا (m_نخها < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        إذا (m_نخها == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_نخها = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_hier_نخها = std::atoi(valp);
        إذا (m_hier_نخها < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_نخها_Coarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "all")) {
            m_نخها_Dpi_خالص = درست;
            m_نخها_Dpi_Unpure = درست;
        } وإلا إذا (!std::strcmp(valp, "none")) {
            m_نخها_Dpi_خالص = خطأ;
            m_نخها_Dpi_Unpure = خطأ;
        } وإلا إذا (!std::strcmp(valp, "pure")) {
            m_نخها_Dpi_خالص = درست;
            m_نخها_Dpi_Unpure = خطأ;
        } وإلا {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warn_بیشتر() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_نخها_Max_MTasks = std::atoi(valp);
        إذا (m_نخها_Max_MTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [هذا, fl](ثابت حرف* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        إذا (!unit.isNone() && زمان_Override_Unit().isNone()) m_زمان_Default_Unit = unit;
        إذا (!prec.isNone() && زمان_Override_Prec().isNone()) m_زمان_Default_Prec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [هذا, fl](ثابت حرف* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, درست);
        إذا (!unit.isNone()) {
            m_زمان_Default_Unit = unit;
            m_زمان_Override_Unit = unit;
        }
        إذا (!prec.isNone()) {
            m_زمان_Default_Prec = prec;
            m_زمان_Override_Prec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [هذا](ثابت std::string& flag) { m_بالاترین_Module = Ast_گره::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [هذا](ثابت std::string& flag) { m_بالاترین_Module = Ast_گره::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", مجموعه_2, &m_بالاترین_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [هذا]() { m_traceEnabledSaif = درست; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", مجموعه_2, &m_trace_عمق);
    DECL_OPTION("-trace-fst", CbCall, [هذا]() {
        m_traceEnabledFst = درست;
        جمع_کن_Ld_Libs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", مجموعه_2, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", مجموعه_2, &m_trace_Max_عرض);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ثابت حرف*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", مجموعه_2, &m_no_Trace_بالاترین);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [هذا]() { m_traceEnabledVcd = درست; });

    DECL_OPTION("-U", Cb_Partial_تطابق, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", مجموعه_2, &m_constraint_Array_حد);
    DECL_OPTION("-unroll-count", مجموعه_2, &m_unroll_شمار).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", مجموعه_2, &m_unroll_حد);
    DECL_OPTION("-unroll-stmts", مجموعه_2, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", مجموعه_2, &m_استفادهنشده_Regexp);

    DECL_OPTION("-V", CbCall, [هذا]() {
        show_نسخه(درست);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [هذا, &optdir](ثابت حرف* valp) {
        V3Options::جمع_کن_Library_File(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [هذا, fl](ثابت حرف* valp) {
        صحيح val = std::atoi(valp);
        إذا (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } وإلا إذا (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [هذا]() {
        show_نسخه(خطأ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_سطر::globalWarnOff(V3_Error_کد::I_LINT, خطأ); });
    DECL_OPTION("-Werror-", Cb_Partial_تطابق, [هذا, fl](ثابت حرف* optp) {
        ثابت V3_Error_کد کد{optp};
        إذا (کد == V3_Error_کد::EC_ERROR) {
            إذا (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } وإلا {
            V3Error::pretendError(کد, درست);
        }
    });
    DECL_OPTION("-Wfuture-", Cb_Partial_تطابق, [هذا](ثابت حرف* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        جمع_کن_Future(optp);
    });
    DECL_OPTION("-Wno-", Cb_Partial_تطابق, [fl, &parser](ثابت حرف* optp) VL_MT_DISABLED {
        ثابت string err = File_سطر::globalWarnOffParse(optp, درست);
        إذا (!err.خالی()) {
            ثابت string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    لكل (صحيح i = V3_Error_کد::EC_اولی_WARN; i < V3_Error_کد::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        لكل (ثابت string پیشوند : {"-Wno-", "-Wwarn-"})
            parser.جمع_کن_Suggestion_Candidate(پیشوند + V3_Error_کد{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [هذا]() { m_context = خطأ; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(خطأ); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_سطر::globalWarnOff(V3_Error_کد::I_LINT, درست); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_سطر::globalWarnOff(V3_Error_کد::I_STYLE, درست); });
    DECL_OPTION("-work", مجموعه_2, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [هذا]() {
        m_pedantic = درست;
        V3Error::pretendError(V3_Error_کد::ASSIGNIN, خطأ);
    });
    DECL_OPTION("-Wwarn-", Cb_Partial_تطابق, [هذا, fl, &parser](ثابت حرف* optp) VL_MT_DISABLED {
        ثابت V3_Error_کد کد{optp};
        إذا (کد == V3_Error_کد::EC_ERROR) {
            إذا (!isFuture(optp)) {
                ثابت string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } وإلا {
            File_سطر::globalWarnOff(کد, خطأ);
            V3Error::pretendError(کد, خطأ);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_سطر::globalWarnOff(V3_Error_کد::I_LINT, خطأ); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_سطر::globalWarnOff(V3_Error_کد::I_STYLE, خطأ); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", مجموعه_2, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "0")) {
            m_x_تخصیص_بده = "0";
        } وإلا إذا (!std::strcmp(valp, "1")) {
            m_x_تخصیص_بده = "1";
        } وإلا إذا (!std::strcmp(valp, "fast")) {
            m_x_تخصیص_بده = "fast";
        } وإلا إذا (!std::strcmp(valp, "unique")) {
            m_x_تخصیص_بده = "unique";
        } وإلا {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warn_بیشتر() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } وإلا إذا (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } وإلا إذا (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } وإلا {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warn_بیشتر() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_x_Initial_یال);

    DECL_OPTION("-y", CbVal, [هذا, &optdir](ثابت حرف* valp) {
        جمع_کن_Inc_Dir_User(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    لكل (صحيح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        إذا (argv[i][0] == '-' || argv[i][0] == '+') {
            ثابت std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            صحيح consumed = 0;
            منطقي isForRerun = درست;

            // Special case for -j which has an optional argument
            إذا (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                صحيح val = 0;
                إذا (i + 1 < argc && std::آیا_رقم(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    إذا (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                إذا (m_buildJobs == -1) m_buildJobs = val;
                إذا (m_verilateJobs == -1) m_verilateJobs = val;
                إذا (m_outputGroups == -1) m_outputGroups = val;
            } وإلا {
                ثابت خودکار pair = parser.parse(i, argc, argv);
                consumed = pair.اولی;
                isForRerun = pair.دومی;
                إذا (consumed) {
                    // Already processed
                } وإلا إذا (isFuture0(argName)) {
                    consumed = 1;
                } وإلا إذا (isFuture1(argName)) {
                    consumed = 2;
                } وإلا {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            إذا (argName != "f" && argName != "F") جمع_کن_Arg(argv + i, consumed, isForRerun);
            i += consumed;
            استمر;
        }

        // Filename
        ثابت std::string filename = parseFileArg(optdir, argv[i]);
        منطقي isForRerun = درست;
        إذا (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::جمع_کن_Cpp_File(filename);
        } وإلا إذا (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::جمع_کن_Ld_Lib_File(filename);
        } وإلا إذا (suffixed(filename, ".vlt")) {
            V3Options::جمع_کن_Vlt_File(filename, work());
            isForRerun = خطأ;
        } وإلا {
            V3Options::جمع_کن_VFile(filename, work());
            isForRerun = خطأ;
        }
        جمع_کن_Arg(filename, isForRerun);
        ++i;
    }

    إذا (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

فراغ V3Options::parseOptsFile(File_سطر* fl, ثابت string& filename, منطقي rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    إذا (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        أرجع;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    منطقي has_fargs = خطأ;
    std::string کامل_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ثابت خودکار parse_fargs_cmt = [&](ثابت حرف* p) -> فراغ {
        ثابت منطقي بلوک_Cmt = p[1] == '*';
        p += 2;
        طالما (std::آیا_فاصله(*p)) ++p;
        إذا (VString::startsWith(p, "verilator")) {
            p += std::طول_رشته("verilator");
            إذا (std::آیا_فاصله(*p)) {
                طالما (std::آیا_فاصله(*p)) ++p;
                إذا (VString::startsWith(p, "fargs")) {
                    p += std::طول_رشته("fargs");
                    إذا (std::آیا_فاصله(*p)) {
                        طالما (std::آیا_فاصله(*p)) ++p;
                        إذا (!has_fargs) {
                            has_fargs = درست;
                            کامل_file.clear();
                        }
                        کامل_file += p;
                        إذا (بلوک_Cmt) {
                            ثابت std::string::اندازه_type pos = کامل_file.پیدا_کن("*/");
                            إذا (pos == std::string::npos) {
                                fl->v3fatal("/* verilator fargs */ must be on single line");
                            }
                            کامل_file.resize(pos);
                        }
                        کامل_file += ' ';
                    }
                }
            }
        }
    };

    منطقي درون_Cmt = خطأ;
    طالما (!ifp->eof()) {
        ثابت std::string سطر = V3Os::getline(*ifp);
        // Strip simple comments
        حرف lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        منطقي space_begin = درست;  // At beginning or leading spaces only
        لكل (ثابت حرف* p = سطر.c_str(); *p; lastch = *p++) {
            إذا (درون_Cmt) {
                إذا (p[0] == '*' && p[1] == '/') {
                    درون_Cmt = خطأ;
                    ++p;
                }
            } وإلا إذا (p[0] == '/' && p[1] == '/' && std::آیا_فاصله(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                اكسر;  // Ignore to EOL
            } وإلا إذا (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                اكسر;  // Ignore to EOL
            } وإلا إذا (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                درون_Cmt = درست;
                space_begin = خطأ;
                ++p;
            } وإلا {
                إذا (!std::آیا_فاصله(*p)) space_begin = خطأ;
                إذا (!has_fargs) کامل_file += *p;
            }
        }
        إذا (!has_fargs) کامل_file += " ";
    }
    کامل_file += "\n";  // So string match below is simplified
    إذا (درون_Cmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = جديد File_سطر{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::وکتور<string> آرگومانها;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    تعداد state : uint8_t {
        ST_درون_OPTION,
        ST_ESCAPED_CHAR,
        ST_درون_QUOTED_STR,
        ST_درون_DOUBLE_QUOTED_STR
    };

    state st = ST_درون_OPTION;
    state آخرین_st = ST_درون_OPTION;
    string arg;
    لكل (string::اندازه_type pos = 0; pos < کامل_file.درازی(); ++pos) {
        حرف curr_char = کامل_file[pos];
        بدل (st) {
        حالة ST_درون_OPTION:  // Get all chars up to a white space or a "="
            إذا (std::آیا_فاصله(curr_char)) {  // End of option
                إذا (!arg.خالی()) {  // End of word
                    آرگومانها.افزودن_پشت(arg);
                }
                arg = "";
                اكسر;
            }
            إذا (curr_char == '\\') {  // Escape char, we wait for next char
                آخرین_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                اكسر;
            }
            إذا (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                إذا (pos < کامل_file.درازی()) curr_char = کامل_file[pos];
                إذا (curr_char == '"') {  // String
                    st = ST_درون_QUOTED_STR;
                } وإلا {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                اكسر;
            }
            إذا (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_درون_DOUBLE_QUOTED_STR;
                اكسر;
            }
            arg += curr_char;
            اكسر;
        حالة ST_درون_QUOTED_STR:  // Just store all chars inside string
            إذا (curr_char != '\'') {
                arg += curr_char;
            } وإلا {  // End of quoted string
                st = ST_درون_OPTION;
            }
            اكسر;
        حالة ST_درون_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            إذا (curr_char != '"') {
                إذا (curr_char == '\\') {
                    آخرین_st = st;
                    st = ST_ESCAPED_CHAR;
                } وإلا {
                    arg += curr_char;
                }
            } وإلا {  // End of double quoted string
                st = ST_درون_OPTION;
            }
            اكسر;
        حالة ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = آخرین_st;
            اكسر;
        }
    }
    إذا (!arg.خالی()) {  // Add last word
        آرگومانها.افزودن_پشت(arg);
    }

    //مسیر
    ثابت string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::وکتور<حرف*> argv;
    argv.reserve(آرگومانها.اندازه() + 1);
    لكل (ثابت string& i : آرگومانها) argv.افزودن_پشت(const_cast<حرف*>(i.c_str()));
    argv.افزودن_پشت(اشاره_تهی);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<صحيح>(argv.اندازه() - 1), argv.داده());
}

//======================================================================

string V3Options::parseFileArg(ثابت string& optdir, ثابت string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    إذا (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    أرجع filename;
}

//======================================================================

فراغ V3Options::show_نسخه(منطقي verbose) {
    خروجی << نسخه();
    خروجی << "\n";
    إذا (!verbose) أرجع;

    خروجی << "\n";
    خروجی << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    خروجی << "redistribute it and/or modify the Verilator internals under the terms of\n";
    خروجی << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    خروجی << "License Version 2.0.\n";

    خروجی << "\n";
    خروجی << "See https://verilator.org for documentation\n";

    خروجی << "\n";
    خروجی << "Summary of configuration:\n";
    خروجی << "  Compiled in defaults if not in environment:\n";
    خروجی << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    خروجی << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    خروجی << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    خروجی << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    خروجی << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_ریشه << "\n";
    خروجی << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    خروجی << "    SystemC system-wide = " << cvt_مقصد_Str(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    خروجی << "\n";
    خروجی << "Environment:\n";
    خروجی << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    خروجی << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    خروجی << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    خروجی << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    خروجی << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    خروجی << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    خروجی << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    خروجی << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    خروجی << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    خروجی << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    خروجی << "\n";
    خروجی << "Supported features (compiled-in or forced by environment):\n";
    خروجی << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    خروجی << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    خروجی << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = جديد V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_استفادهنشده_Regexp = "*unused*";
    m_x_تخصیص_بده = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3_Lang_کد::mostRecent();

    VName::max_درازی(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    جمع_کن_Lib_Ext_V("");  // So include "filename.v" will find the same file
    جمع_کن_Lib_Ext_V(".v");
    جمع_کن_Lib_Ext_V(".sv");
    // Default -I
    جمع_کن_Inc_Dir_Fallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(حذف m_impp, m_impp = اشاره_تهی); }

فراغ V3Options::مجموعه_Debug_نما(صحيح level) {
    V3Error::debugDefault(level);
    إذا (!m_dumpLevel.شمار("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = درست;
    m_debugCheck = درست;
    إذا (level) {
        خروجی << "- Starting " << نسخه() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

بیعلامت V3Options::debugLevel(ثابت string& برچسب) ثابت VL_MT_امن {
    ثابت خودکار iter = m_debugLevel.پیدا_کن(برچسب);
    أرجع iter != m_debugLevel.انتها() ? iter->دومی : V3Error::debugDefault();
}

بیعلامت V3Options::debugSrcLevel(ثابت string& srcfile_مسیر) ثابت VL_MT_امن {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    أرجع debugLevel(V3Os::filenameNonDirExt(srcfile_مسیر));
}

بیعلامت V3Options::dumpLevel(ثابت string& برچسب) ثابت VL_MT_امن {
    ثابت خودکار iter = m_dumpLevel.پیدا_کن(برچسب);
    أرجع iter != m_dumpLevel.انتها() ? iter->دومی : 0;
}

بیعلامت V3Options::dumpSrcLevel(ثابت string& srcfile_مسیر) ثابت VL_MT_امن {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    أرجع dumpLevel(V3Os::filenameNonDirExt(srcfile_مسیر));
}

منطقي V3Options::dump_درخت_Addrids() ثابت VL_MT_امن {
    ساكن صحيح s_level = -1;
    إذا (VL_UNLIKELY(s_level < 0)) {
        ثابت بیعلامت مقدار = dumpLevel("tree-addrids");
        إذا (!موجود()) أرجع مقدار > 0;
        s_level = static_cast<بیعلامت>(مقدار);
    }
    أرجع s_level > 0;
}

فراغ V3Options::optimize(صحيح level) {
    // Set all optimizations to on/off
    ثابت منطقي flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_f_بیت_پیمایش_Loops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_f_Case_جدول = flag;
    m_f_Case_درخت = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_بیت_Op_درخت = flag;
    m_fDedupe = flag;
    m_fDfg = flag;
    m_fDeadAssigns = flag;
    m_fDeadCells = flag;
    m_fDeadMethods = flag;
    m_fDecomposePacked = flag;
    m_fDecomposeUnpacked = flag;
    m_fExpand = flag;
    m_fGate = flag;
    m_fInline = flag;
    m_fInlineCFuncs = flag;
    m_fLife = flag;
    m_fLifePost = flag;
    m_fLocalize = flag;
    m_f_ادغام_Cond = flag;
    m_fReloop = flag;
    m_fReorder = flag;
    m_f_تقسیم = flag;
    m_fSubst = flag;
    m_fSubstConst = flag;
    m_f_جدول = flag;
    // And set specific optimization levels
    إذا (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
