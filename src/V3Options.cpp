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
#جيڪڏهن_وصف_نه_ٿيل _WIN32
# include <sys/utsname.h>
#جيڪڏهن_پورو
#ضمن <algorithm>
#ضمن <cctype>
#جيڪڏهن_وصف_ٿيل _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(گهڻائي) (((گهڻائي) & _S_IFMT) == _S_IFDIR)
#وإلا
# include <dirent.h>
#جيڪڏهن_پورو
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
#جيڪڏهن_پورو
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

صنف V3OptionsImp final {
عام:
    // TYPES
    استعمال DirMap = std::map<ثابت string, std::سيٽ<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_سٽ_دليلون;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, منطقي>> m_all_دليلون;
    VInsertion_سيٽ<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::سيٽ<string> m_inc_Dir_Fallback_سيٽ;  // Include directories (for removing duplicates)
    std::map<ثابت string, V3_Lang_ڪوڊ> m_langExts;  // Language extension map
    VInsertion_سيٽ<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    فراغ جوڙيو_Inc_Dir_User(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        ثابت منطقي inserted = m_incDirUsers.وجهو(dir);
        إذا (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_inc_Dir_Fallback_سيٽ.erase(dir);  // User has priority over Fallback
        }
    }
    فراغ جوڙيو_Inc_Dir_Fallback(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        إذا (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ثابت خودڪار it_مليو_Pair = m_inc_Dir_Fallback_سيٽ.وجهو(dir);
            إذا (it_مليو_Pair.ٻيو) m_incDirFallbacks.ڌڪو_پٺ(dir);
        }
    }
    فراغ جوڙيو_Lang_Ext(ثابت string& langext, ثابت V3_Lang_ڪوڊ& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        إذا (addext[0] == '.') addext = addext.substr(1);
        (فراغ)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    فراغ جوڙيو_Lib_Ext_V(ثابت string& libext) { m_libExtVs.وجهو(libext); }
    V3OptionsImp() = افتراضي;
    ~V3OptionsImp() = افتراضي;
};

//######################################################################
// V3LangCode class functions

V3_Lang_ڪوڊ::V3_Lang_ڪوڊ(ثابت حرف* textp) {
    // Return code for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3_Lang_ڪوڊ::L_ERROR; codei < V3_Lang_ڪوڊ::ENUM_پڇاڙي; ++codei) {
        ثابت V3_Lang_ڪوڊ ڪوڊ{codei};
        إذا (0 == VL_STRCASECMP(textp, ڪوڊ.ascii())) {
            m_e = ڪوڊ;
            أرجع;
        }
    }
    m_e = V3_Lang_ڪوڊ::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ثابت string& قدر, منطقي& badr)
    : m_e{VTimescale::NONE} {
    badr = سچ;
    ثابت string spaceless = VString::removeWhitespace(قدر);
    لكل (صحيح i = TS_100S; i < ENUM_پڇاڙي; ++i) {
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
V3_درجيوار_بلاڪ_Option::V3_درجيوار_بلاڪ_Option(ثابت string& opts) {
    VStringList vals;
    منطقي اندريون_Str = خطأ;
    string cur;
    ساكن ثابت string hier_بلاڪ("--hierarchical-block");
    File_سٽ cmdfl(File_سٽ::command_سٽ_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    لكل (string::const_iterator it = opts.begin(); it != opts.پڇاڙي();) {
        إذا (اندريون_Str) {
            إذا (*it == '\\') {
                ++it;
                إذا (it == opts.پڇاڙي()) {
                    cmdfl.v3error(hier_بلاڪ + " must not end with \\");
                    اكسر;
                }
                إذا (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hier_بلاڪ + " does not allow '" + *it + "' after \\");
                    اكسر;
                }
                cur.ڌڪو_پٺ(*it);
                ++it;
            } وإلا إذا (*it == '"') {  // end of string
                cur.ڌڪو_پٺ(*it);
                vals.ڌڪو_پٺ(cur);
                cur.clear();
                ++it;
                إذا (it != opts.پڇاڙي()) {
                    إذا (*it != ',') {
                        cmdfl.v3error(hier_بلاڪ + " expects ',', but '" + *it + "' is passed");
                        اكسر;
                    }
                    ++it;
                    إذا (it == opts.پڇاڙي()) {
                        cmdfl.v3error(hier_بلاڪ + " must not end with ','");
                        اكسر;
                    }
                    اندريون_Str = *it == '"';
                    cur.ڌڪو_پٺ(*it);
                    ++it;
                }
            } وإلا {
                cur.ڌڪو_پٺ(*it);
                ++it;
            }
        } وإلا {
            إذا (*it == '"') {
                cmdfl.v3error(hier_بلاڪ + " does not allow '\"' in the middle of literal");
                اكسر;
            }
            إذا (*it == ',') {  // end of this parameter
                vals.ڌڪو_پٺ(cur);
                cur.clear();
                ++it;
                إذا (it == opts.پڇاڙي()) {
                    cmdfl.v3error(hier_بلاڪ + " must not end with ','");
                    اكسر;
                }
                اندريون_Str = *it == '"';
            }
            cur.ڌڪو_پٺ(*it);
            ++it;
        }
    }
    إذا (!cur.خالي_جايون()) vals.ڌڪو_پٺ(cur);
    إذا (vals.سائيز() >= 2) {
        إذا (vals.سائيز() % 2) {
            cmdfl.v3error(hier_بلاڪ + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } وإلا {
        cmdfl.v3error(hier_بلاڪ + " requires at least two comma-separated values");
    }
    لكل (سائيز_t i = 2; i + 1 < vals.سائيز(); i += 2) {
        ثابت منطقي inserted = m_parameters.emplace(vals[i], vals[i + 1]).ٻيو;
        إذا (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hier_بلاڪ);
        }
    }
}

//######################################################################
// V3Options class functions

فراغ VTimescale::parseSlashed(File_سٽ* fl, ثابت حرف* textp, VTimescale& unitr,
                              VTimescale& precr, منطقي allow_خالي_جايون) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ثابت حرف* cp = textp;
    لكل (; std::خال_آهي(*cp); ++cp) {}
    ثابت حرف* ثابت unitp = cp;
    لكل (; *cp && *cp != '/'; ++cp) {}
    ثابت string unitStr(unitp, cp - unitp);
    لكل (; std::خال_آهي(*cp); ++cp) {}
    string precStr;
    إذا (*cp == '/') {
        ++cp;
        لكل (; std::خال_آهي(*cp); ++cp) {}
        ثابت حرف* ثابت precp = cp;
        لكل (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    لكل (; std::خال_آهي(*cp); ++cp) {}
    إذا (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        أرجع;
    }

    منطقي unitbad = خطأ;
    ثابت VTimescale unit{unitStr, unitbad /*ref*/};
    إذا (unitbad && !(unitStr.خالي_جايون() && allow_خالي_جايون)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        أرجع;
    }
    unitr = unit;

    إذا (!precStr.خالي_جايون()) {
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

فراغ V3Options::جوڙيو_Inc_Dir_User(ثابت string& incdir) { m_impp->جوڙيو_Inc_Dir_User(incdir); }
فراغ V3Options::جوڙيو_Inc_Dir_Fallback(ثابت string& incdir) { m_impp->جوڙيو_Inc_Dir_Fallback(incdir); }
فراغ V3Options::جوڙيو_Lang_Ext(ثابت string& langext, ثابت V3_Lang_ڪوڊ& lc) {
    m_impp->جوڙيو_Lang_Ext(langext, lc);
}
فراغ V3Options::جوڙيو_Lib_Ext_V(ثابت string& libext) { m_impp->جوڙيو_Lib_Ext_V(libext); }
فراغ V3Options::جوڙيو_Define(ثابت string& defline, منطقي allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string کاٻي = defline;
    طالما (کاٻي != "") {
        string def = کاٻي;
        string::سائيز_type pos;
        إذا (allowPlus && ((pos = کاٻي.ڳوليو('+')) != string::npos)) {
            کاٻي = کاٻي.substr(pos + 1);
            def.erase(pos);
        } وإلا {
            کاٻي = "";
        }
        string قدر;
        إذا ((pos = def.ڳوليو('=')) != string::npos) {
            قدر = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_سٽ(def, قدر);
    }
}
فراغ V3Options::جوڙيو_Parameter(ثابت string& paramline, منطقي allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string کاٻي = paramline;
    طالما (کاٻي != "") {
        string param = کاٻي;
        string::سائيز_type pos;
        إذا (allowPlus && ((pos = کاٻي.ڳوليو('+')) != string::npos)) {
            کاٻي = کاٻي.substr(pos + 1);
            param.erase(pos);
        } وإلا {
            کاٻي = "";
        }
        string قدر;
        إذا ((pos = param.ڳوليو('=')) != string::npos) {
            قدر = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << قدر);
        (فراغ)m_parameters.erase(param);
        m_parameters[param] = قدر;
    }
}

منطقي V3Options::hasParameter(ثابت string& name) {
    أرجع m_parameters.ڳوليو(name) != m_parameters.پڇاڙي();
}

string V3Options::parameter(ثابت string& name) {
    string قدر = m_parameters.ڳوليو(name)->ٻيو;
    m_parameters.erase(m_parameters.ڳوليو(name));
    أرجع قدر;
}

فراغ V3Options::checkParameters() {
    إذا (!m_parameters.خالي_جايون()) {
        std::stringstream نياپو;
        نياپو << "Parameters from the command line were not found in the design:";
        لكل (ثابت خودڪار& i : m_parameters) نياپو << " " << i.پهريون;
        v3error(نياپو.str());
    }
}

فراغ V3Options::جوڙيو_Cpp_File(ثابت string& filename) { m_cppFiles.وجهو(filename); }
فراغ V3Options::جوڙيو_CFlags(ثابت string& filename) { m_cFlags.ڌڪو_پٺ(filename); }
فراغ V3Options::جوڙيو_Lib_Map_File(ثابت string& filename) { m_libmapFiles.وجهو(filename); }
فراغ V3Options::جوڙيو_Compiler_Includes(ثابت string& filename) {
    m_compilerIncludes.وجهو(filename);
}
فراغ V3Options::جوڙيو_Ld_Lib_File(ثابت string& filename) {
    m_ldLibFiles.ڌڪو_پٺ(filename);
    جوڙيو_Ld_Libs(filename);
}
فراغ V3Options::جوڙيو_Ld_Libs(ثابت string& filename) { m_ldLibs.ڌڪو_پٺ(filename); }
فراغ V3Options::جوڙيو_Make_Flags(ثابت string& filename) { m_makeFlags.ڌڪو_پٺ(filename); }
فراغ V3Options::جوڙيو_Future(ثابت string& flag) { m_futures.وجهو(flag); }
فراغ V3Options::جوڙيو_Future0(ثابت string& flag) { m_future0s.وجهو(flag); }
فراغ V3Options::جوڙيو_Future1(ثابت string& flag) { m_future1s.وجهو(flag); }
منطقي V3Options::isFuture(ثابت string& flag) ثابت {
    أرجع m_futures.ڳوليو(flag) != m_futures.پڇاڙي();
}
منطقي V3Options::isFuture0(ثابت string& flag) ثابت {
    أرجع m_future0s.ڳوليو(flag) != m_future0s.پڇاڙي();
}
منطقي V3Options::isFuture1(ثابت string& flag) ثابت {
    أرجع m_future1s.ڳوليو(flag) != m_future1s.پڇاڙي();
}
منطقي V3Options::isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت {
    أرجع m_libraryFiles.ڳوليو({filename, libname}) != m_libraryFiles.پڇاڙي();
}
فراغ V3Options::جوڙيو_Library_File(ثابت string& filename, ثابت string& libname) {
    m_libraryFiles.وجهو({filename, libname});
}
فراغ V3Options::جوڙيو_VFile(ثابت string& filename, ثابت string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ڌڪو_پٺ({filename, libname});  // Not emplace
}
فراغ V3Options::جوڙيو_Vlt_File(ثابت string& filename, ثابت string& libname) {
    m_vltFiles.وجهو({filename, libname});
}
فراغ V3Options::جوڙيو_Force_Inc(ثابت string& filename) { m_forceIncs.ڌڪو_پٺ(filename); }

فراغ V3Options::جوڙيو_سٽ_Arg(ثابت string& arg) { m_impp->m_سٽ_دليلون.ڌڪو_پٺ(arg); }

فراغ V3Options::جوڙيو_Arg(حرف** argv, سائيز_t ڳڻپ, منطقي isForRerun) {
    m_impp->m_all_دليلون.emplace_پٺ();
    std::pair<std::list<std::string>, منطقي>& pair = m_impp->m_all_دليلون.پٺ();
    pair.ٻيو = isForRerun;
    لكل (سائيز_t n = 0; n < ڳڻپ; ++n) pair.پهريون.emplace_پٺ(argv[n]);
}

فراغ V3Options::جوڙيو_Arg(ثابت std::string& arg, منطقي isForRerun) {
    m_impp->m_all_دليلون.emplace_پٺ();
    std::pair<std::list<std::string>, منطقي>& pair = m_impp->m_all_دليلون.پٺ();
    pair.ٻيو = isForRerun;
    pair.پهريون.emplace_پٺ(arg);
}

ثابت std::list<std::pair<std::list<std::string>, منطقي>>& V3Options::all_دليلون() ثابت {
    أرجع m_impp->m_all_دليلون;
}

string V3Options::all_دليلون_String() ثابت VL_MT_محفوظ {
    string نتيجو;
    لكل (ثابت خودڪار& pair : m_impp->m_all_دليلون) {
        لكل (ثابت string& arg : pair.پهريون) {
            إذا (!نتيجو.خالي_جايون()) نتيجو += " ";
            نتيجو += arg;
        }
    }
    أرجع نتيجو;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::all_دليلون_String_For_Hier_بلاڪ(منطقي for_چوٽي) ثابت {
    // cppcheck-suppress shadowFunction
    std::سيٽ<string> vFiles;
    لكل (ثابت خودڪار& vFile : m_vFiles) vFiles.وجهو(vFile.filename());
    string ٻاهريون;
    منطقي stripArg = خطأ;
    منطقي strip_Arg_If_انگ = خطأ;
    لكل (ثابت خودڪار& pair : m_impp->m_all_دليلون) {
        لكل (ثابت string& arg : pair.پهريون) {
            إذا (stripArg) {
                stripArg = خطأ;
                استمر;
            }
            إذا (strip_Arg_If_انگ) {
                strip_Arg_If_انگ = خطأ;
                إذا (انگ_آهي(arg[0])) استمر;
            }
            صحيح skip = 0;
            إذا (arg.ڊيگهائي() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } وإلا إذا (arg.ڊيگهائي() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            إذا (skip > 0) {  // arg is an option
                ثابت string opt = arg.substr(skip);  // Remove '-' in the beginning
                ثابت صحيح انگ_Strip = strip_Options_For_ٻار_لاڳيتو(opt, for_چوٽي);
                إذا (انگ_Strip) {
                    UASSERT(0 <= انگ_Strip && انگ_Strip <= 3, "should be one of 0, 1, 2, 3");
                    إذا (انگ_Strip == 2) stripArg = سچ;
                    إذا (انگ_Strip == 3) strip_Arg_If_انگ = سچ;
                    استمر;
                }
            } وإلا {  // Not an option
                إذا (vFiles.ڳوليو(arg) != vFiles.پڇاڙي()  // Remove HDL
                    || m_cppFiles.ڳوليو(arg) != m_cppFiles.پڇاڙي()) {  // Remove C++
                    استمر;
                }
            }
            إذا (ٻاهريون != "") ٻاهريون += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            ٻاهريون += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    أرجع ٻاهريون;
}

فراغ V3Options::cc_سيٽ() {  // --cc
    m_ٻاهريون_Format_Ok = سچ;
    m_systemC = خطأ;
}

فراغ V3Options::decorations(File_سٽ* fl, ثابت string& arg) {  // --decorations
    إذا (arg == "none") {
        m_decoration = خطأ;
        m_decorationNodes = خطأ;
    } وإلا إذا (arg == "node") {
        m_decoration = سچ;
        m_decorationNodes = سچ;
    } وإلا إذا (arg == "medium") {
        m_decoration = سچ;
        m_decorationNodes = خطأ;
    } وإلا {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warn_وڌيڪ() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::ويڪٽر<std::string> V3Options::traceClassBases() ثابت VL_MT_محفوظ {
    std::ويڪٽر<std::string> نتيجو;
    إذا (traceEnabledFst()) نتيجو.emplace_پٺ("VerilatedFst");
    إذا (traceEnabledSaif()) نتيجو.emplace_پٺ("VerilatedSaif");
    إذا (traceEnabledVcd()) نتيجو.emplace_پٺ("VerilatedVcd");
    أرجع نتيجو;
}
std::ويڪٽر<std::string> V3Options::traceClassLangs() ثابت VL_MT_محفوظ {
    std::ويڪٽر<std::string> نتيجو;
    لكل (ثابت خودڪار& cbase : traceClassBases())
        نتيجو.emplace_پٺ(cbase + (systemC() ? "Sc" : "C"));
    أرجع نتيجو;
}
std::ويڪٽر<std::string> V3Options::trace_ذريعو_Bases() ثابت VL_MT_محفوظ {
    std::ويڪٽر<std::string> نتيجو;
    إذا (traceEnabledFst()) نتيجو.emplace_پٺ("verilated_fst");
    إذا (traceEnabledSaif()) نتيجو.emplace_پٺ("verilated_saif");
    إذا (traceEnabledVcd()) نتيجو.emplace_پٺ("verilated_vcd");
    أرجع نتيجو;
}
std::ويڪٽر<std::string> V3Options::trace_ذريعو_Langs() ثابت VL_MT_محفوظ {
    std::ويڪٽر<std::string> نتيجو = trace_ذريعو_Bases();
    لكل (std::string& str : نتيجو) str += systemC() ? "_sc"s : "_c"s;
    أرجع نتيجو;
}
std::string V3Options::trace_Class_بنياد() ثابت VL_MT_محفوظ {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().خالي_جايون(), "Call traceClassBase only when trace() enabled");
    أرجع traceClassBases().front();
}
std::string V3Options::traceClassLang() ثابت VL_MT_محفوظ {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().خالي_جايون(), "Call traceClassLang only when trace() enabled");
    أرجع traceClassLangs().front();
}

//######################################################################
// File searching

منطقي V3Options::file_Stat_عام(ثابت string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    بنية stat sstat;  // Stat information
    ثابت صحيح err = stat(filename.c_str(), &sstat);
    إذا (err != 0) أرجع خطأ;
    إذا (S_ISDIR(sstat.st_گهڻائي)) أرجع خطأ;
    أرجع سچ;
}

string V3Options::fileExists(ثابت string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ثابت string dir = V3Os::filenameDir(filename);
    ثابت string basename = V3Os::filenameNonDir(filename);

    خودڪار diriter = m_impp->m_dirMap.ڳوليو(dir);
    إذا (diriter == m_impp->m_dirMap.پڇاڙي()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::سيٽ<string>());
        diriter = m_impp->m_dirMap.ڳوليو(dir);

        std::سيٽ<string>* ثابت setp = &(diriter->ٻيو);

#جيڪڏهن_وصف_ٿيل _MSC_VER
        حاول {
            لكل (ثابت خودڪار& dir_داخلا : std::filesystem::directory_iterator(dir.c_str()))
                setp->وجهو(dir_داخلا.رستو().filename().string());
        } امسك (std::filesystem::filesystem_error ثابت& ex) {
            (فراغ)ex;
            أرجع "";
        }
#وإلا
        إذا (DIR* ثابت dirp = opendir(dir.c_str())) {
            طالما (بنية dirent* direntp = readdir(dirp)) setp->وجهو(direntp->d_name);
            closedir(dirp);
        }
#جيڪڏهن_پورو
    }
    // Find it
    ثابت std::سيٽ<string>* ثابت filesetp = &(diriter->ٻيو);
    ثابت خودڪار fileiter = filesetp->ڳوليو(basename);
    إذا (fileiter == filesetp->پڇاڙي()) أرجع "";  // Not found
    // Check if it is a directory, ignore if so
    ثابت string filename_ٻاهريون = V3Os::filenameJoin(dir, basename);
    إذا (!file_Stat_عام(filename_ٻاهريون)) أرجع "";  // Directory
    أرجع filename_ٻاهريون;
}

string V3Options::file_رستو_Check_One_Dir(ثابت string& modname, ثابت string& dirname) {
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
صحيح V3Options::strip_Options_For_ٻار_لاڳيتو(ثابت string& opt, منطقي for_چوٽي) {
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
    إذا (commonOpts.ڳڻپ(opt)) أرجع commonOpts.at(opt);

    // Options to strip only for hier blocks
    إذا (!for_چوٽي) {
        ساكن ثابت std::unordered_map<std::string, صحيح> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        إذا (subOpts.ڳڻپ(opt)) أرجع subOpts.at(opt);
        إذا (VString::startsWith(opt, "G")) أرجع 1;
        إذا (VString::startsWith(opt, "pvalue")) أرجع 1;
    }

    // Do not strip
    أرجع 0;
}

فراغ V3Options::validateIdentifier(File_سٽ* fl, ثابت string& arg, ثابت string& opt) {
    إذا (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::file_رستو(File_سٽ* fl, ثابت string& modname, ثابت string& lastpath,
                           ثابت string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ثابت string filename = V3Os::filenameCleanup(VName::dehash(modname));
    إذا (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ثابت string exists = file_رستو_Check_One_Dir(filename, "");
        إذا (exists != "") أرجع exists;
    }
    لكل (ثابت string& dir : m_impp->m_incDirUsers) {
        ثابت string exists = file_رستو_Check_One_Dir(filename, dir);
        إذا (exists != "") أرجع exists;
    }
    لكل (ثابت string& dir : m_impp->m_incDirFallbacks) {
        ثابت string exists = file_رستو_Check_One_Dir(filename, dir);
        إذا (exists != "") أرجع exists;
    }

    إذا (m_relativeIncludes) {
        ثابت string exists = file_رستو_Check_One_Dir(filename, lastpath);
        إذا (exists != "") أرجع V3Os::filename_Real_رستو(exists);
    }

    // Warn and return not found
    إذا (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_رستو_Looked_نياپو(fl, filename));
    }
    أرجع "";
}

string V3Options::file_رستو_Looked_نياپو(File_سٽ* /*fl*/, ثابت string& modname) {
    ساكن منطقي s_shown_notfound_نياپو = خطأ;
    std::ostringstream ss;
    إذا (modname.ڳوليو("__Vhsh") != string::npos) {
        ss << V3Error::warn_وڌيڪ() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warn_وڌيڪ() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } وإلا إذا (!s_shown_notfound_نياپو) {
        s_shown_notfound_نياپو = سچ;
        إذا (m_impp->m_incDirUsers.خالي_جايون()) {
            ss << V3Error::warn_وڌيڪ()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warn_وڌيڪ() << "... Looked in:\n";
        لكل (ثابت string& dir : m_impp->m_incDirUsers) {
            لكل (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_وڌيڪ() << "     " << fn << "\n";
            }
        }
        لكل (ثابت string& dir : m_impp->m_incDirFallbacks) {
            لكل (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_وڌيڪ() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warn_وڌيڪ() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    أرجع ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3_Lang_ڪوڊ V3Options::fileLanguage(ثابت string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::سائيز_type pos;
    إذا (filename == V3Options::get_Std_Package_رستو() || filename == V3Options::get_Std_Waiver_رستو()) {
        أرجع V3_Lang_ڪوڊ::mostRecent();
    } وإلا إذا ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ثابت خودڪار it = m_impp->m_langExts.ڳوليو(ext);
        إذا (it != m_impp->m_langExts.پڇاڙي()) أرجع it->ٻيو;
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
        أرجع getenv_VERILATOR_پاڙ();
    } وإلا {
        أرجع V3Os::getenvStr(var, "");
    }
}

#جيڪڏهن_وصف_ٿيل __FreeBSD__
string V3Options::getenvMAKE() { أرجع V3Os::getenvStr("MAKE", "gmake"); }
#وإلا
string V3Options::getenvMAKE() { أرجع V3Os::getenvStr("MAKE", "make"); }
#جيڪڏهن_پورو

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
#جيڪڏهن_پورو
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

string V3Options::getenv_VERILATOR_پاڙ() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_VERILATOR_پاڙ};
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

string V3Options::get_Std_Package_رستو() {
    أرجع V3Os::filenameJoin(getenv_VERILATOR_پاڙ(), "include", "verilated_std.sv");
}
string V3Options::get_Std_Waiver_رستو() {
    أرجع V3Os::filenameJoin(getenv_VERILATOR_پاڙ(), "include", "verilated_std_waiver.vlt");
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
#جيڪڏهن_وصف_ٿيل HAVE_SYSTEMC
    أرجع سچ;
#وإلا
    أرجع خطأ;
#جيڪڏهن_پورو
}

منطقي V3Options::systemCFound() {
    أرجع (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().خالي_جايون() && !getenvSYSTEMC_LIBDIR().خالي_جايون()));
}

منطقي V3Options::coroutineSupport() {
#جيڪڏهن_وصف_ٿيل HAVE_COROUTINES
    أرجع سچ;
#وإلا
    أرجع خطأ;
#جيڪڏهن_پورو
}

منطقي V3Options::devAsan() {
#جيڪڏهن_وصف_ٿيل HAVE_DEV_ASAN
    أرجع سچ;
#وإلا
    أرجع خطأ;
#جيڪڏهن_پورو
}

منطقي V3Options::devGcov() {
#جيڪڏهن_وصف_ٿيل HAVE_DEV_GCOV
    أرجع سچ;
#وإلا
    أرجع خطأ;
#جيڪڏهن_پورو
}

منطقي V3Options::tsanSupport() {
#جيڪڏهن_وصف_ٿيل HAVE_TSAN
    أرجع سچ;
#وإلا
    أرجع خطأ;
#جيڪڏهن_پورو
}

//######################################################################
// V3 Options notification methods

فراغ V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_سٽ* ثابت cmdfl = جديد File_سٽ{File_سٽ::command_سٽ_Filename()};

    إذا (!ٻاهريون_Format_Ok() && v3Global.opt.مکيه()) cc_سيٽ();  // --main implies --cc if not provided
    إذا (!ٻاهريون_Format_Ok() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    إذا (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::ويڪٽر<std::string> backendFlags;
    إذا (m_build) {
        إذا (m_ٻٽو)
            backendFlags.emplace_پٺ("--binary");
        وإلا
            backendFlags.emplace_پٺ("--build");
    }
    إذا (m_preprocOnly) backendFlags.emplace_پٺ("-E");
    إذا (m_dpiHdrOnly) backendFlags.emplace_پٺ("--dpi-hdr-only");
    إذا (m_lintOnly) backendFlags.emplace_پٺ("--lint-only");
    إذا (m_jsonOnly) backendFlags.emplace_پٺ("--json-only");
    إذا (backendFlags.سائيز() > 1) {
        std::string backendFlagsString = backendFlags.front();
        لكل (سائيز_t i = 1; i < backendFlags.سائيز(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    إذا (m_exe && !v3Global.opt.libCreate().خالي_جايون()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    إذا (!m_gmake && !m_makeJson) m_gmake = سچ;

    إذا (m_درجيوار && (m_hier_ٻار || !m_hier_بلاڪون.خالي_جايون())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    إذا (m_hier_ٻار) {
        إذا (m_hier_بلاڪون.خالي_جايون()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = خطأ;
    }

    إذا (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = سچ;  // No format, with --trace means wanted --trace-vcd
    }
    إذا (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = سچ;
    ثابت صحيح ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    إذا (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    إذا (protectIds()) {
        إذا (all_عوامي()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warn_وڌيڪ() + "... Suggest remove --public.");
        }
        إذا (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warn_وڌيڪ() + "... Suggest remove --trace.");
        }
        إذا (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warn_وڌيڪ() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    إذا (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.سيٽ_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    إذا (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.سيٽ_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    إذا (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.سيٽ_True_Or_False(سچ);

    // Default split limits if not specified
    إذا (m_output_ورهايو_CFuncs < 0) m_output_ورهايو_CFuncs = m_output_ورهايو;
    إذا (m_output_ورهايو_CTrace < 0) m_output_ورهايو_CTrace = m_output_ورهايو;

    إذا (v3Global.opt.مکيه() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    إذا (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        إذا (درجيوار()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    إذا (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    إذا (v3Global.opt.timing().is_سيٽ_True() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    إذا (!m_dumpLevel.ڳڻپ("tree") && m_dumpLevel.ڳڻپ("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(ڌاڳا() >= 1, "'threads()' must return a value >= 1");
    إذا (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    إذا (m_buildJobs == -1) m_buildJobs = 1;
    إذا (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    إذا (timing().is_سيٽ_True()) V3PreShell::define_Cmd_سٽ("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    إذا (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.سيٽ_True_Or_False(خطأ);

    // === Leave last
    // Mark options as available
    m_موجود = سچ;
}

//######################################################################
// V3 Options accessors

string V3Options::نسخو() VL_خالص {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_منزل_Str(DTVERSION_rev);
    أرجع ver;
}

string V3Options::protect_ڪنجي_Defaulted() VL_MT_محفوظ {
    ساكن V3Mutex s_mutex;
    ثابت V3_تالو_Guard تالو_2{s_mutex};
    إذا (m_protect_ڪنجي.خالي_جايون()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_ڪنجي = "VL-KEY-" + digest.digestBase64();
    }
    أرجع m_protect_ڪنجي;
}

فراغ V3Options::throwSigsegv() {  // LCOV_EXCL_START
#إذا !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<متطاير حرف*>(خالي_اشارو) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#جيڪڏهن_پورو
}  // LCOV_EXCL_STOP

VTimescale V3Options::وقت_Compute_Prec(ثابت VTimescale& flag) ثابت {
    إذا (!وقت_Override_Prec().isNone()) {
        أرجع وقت_Override_Prec();
    } وإلا إذا (flag.isNone()) {
        أرجع وقت_Default_Prec();
    } وإلا {
        أرجع flag;
    }
}

VTimescale V3Options::وقت_Compute_Unit(ثابت VTimescale& flag) ثابت {
    إذا (!وقت_Override_Unit().isNone()) {
        أرجع وقت_Override_Unit();
    } وإلا إذا (flag.isNone()) {
        أرجع وقت_Default_Unit();
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

فراغ V3Options::parseOpts(File_سٽ* fl, صحيح argc, حرف** argv) VL_MT_DISABLED {
    // Save command line options
    لكل (صحيح i = 0; i < argc; ++i) جوڙيو_سٽ_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    إذا (vFiles().خالي_جايون()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    إذا (اڳياڙي() == "" && چوٽي_Module() != "") m_اڳياڙي = "V"s + چوٽي_Module();  // Already encoded
    إذا (اڳياڙي() == "" && vFiles().سائيز() >= 1)
        m_اڳياڙي
            = "V"s + Ast_ڳنڍ::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    إذا (mod_اڳياڙي() == "") m_mod_اڳياڙي = اڳياڙي();

    // Find files in makedir
    جوڙيو_Inc_Dir_Fallback(makeDir());
}

//======================================================================

منطقي V3Options::suffixed(ثابت string& sw, ثابت حرف* arg) {
    إذا (std::لڙي_ڊيگهه(arg) > sw.ڊيگهائي()) أرجع خطأ;
    أرجع (0 == std::strcmp(sw.c_str() + sw.ڊيگهائي() - std::لڙي_ڊيگهه(arg), arg));
}

فراغ V3Options::parseOptsList(File_سٽ* fl, ثابت string& optdir, صحيح argc,
                              حرف** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ثابت V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_ٽيگ_فهرست;

    ثابت خودڪار callStrSetter = [هذا](فراغ (V3Options::*cbStr)(ثابت string&)) {
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
    DECL_OPTION("+define+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) VL_MT_DISABLED {
        جوڙيو_Define(optp, سچ);
    }).notForRerun();
    DECL_OPTION("+incdir+", Cb_Partial_ميلاپ, [هذا, &optdir](ثابت حرف* optp) {
        string dirs = optp;
        string::سائيز_type pos;
        طالما ((pos = dirs.ڳوليو('+')) != string::npos) {
            جوڙيو_Inc_Dir_User(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        جوڙيو_Inc_Dir_User(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        string exts = optp;
        string::سائيز_type pos;
        طالما ((pos = exts.ڳوليو('+')) != string::npos) {
            جوڙيو_Lib_Ext_V(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        جوڙيو_Lib_Ext_V(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        جوڙيو_Lang_Ext(optp, V3_Lang_ڪوڊ::L1800_2023);
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
        File_سٽ::globalWarnOff(V3_Error_ڪوڊ::E_UNSUPPORTED, سچ);
    });
    DECL_OPTION("-binary", CbCall, [هذا]() {
        m_ٻٽو = سچ;
        m_build = سچ;
        m_exe = سچ;
        m_main = سچ;
        إذا (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", سيٽ_2, &m_build);
    DECL_OPTION("-build-dep-bin", سيٽ_2, &m_buildDepBin);
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

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::جوڙيو_CFlags));
    DECL_OPTION("-cc", CbCall, [هذا]() { cc_سيٽ(); });
    DECL_OPTION("-clk", CbVal, [fl](ثابت std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ثابت std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", سيٽ_2, &m_comp_حد_بندي_بلاڪون).undocumented();
    DECL_OPTION("-comp-limit-members", سيٽ_2,
                &m_comp_حد_بندي_Members)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", سيٽ_2, &m_comp_حد_بندي_Parens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](صحيح val) { VName::max_ڊيگهائي(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "clang")) {
            m_comp_حد_بندي_بلاڪون = 80;  // limit unknown
            m_comp_حد_بندي_Members = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_comp_حد_بندي_Parens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } وإلا إذا (!std::strcmp(valp, "gcc")) {
            m_comp_حد_بندي_بلاڪون = 0;  // Bug free
            m_comp_حد_بندي_Members = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_comp_حد_بندي_Parens = 240;  // Unlimited, but generate same code as for clang
        } وإلا إذا (!std::strcmp(valp, "msvc")) {
            m_comp_حد_بندي_بلاڪون = 80;  // 128, but allow some room
            m_comp_حد_بندي_Members = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_comp_حد_بندي_Parens = 80;  // 128, but allow some room
        } وإلا {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warn_وڌيڪ() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::جوڙيو_Compiler_Includes));
    DECL_OPTION("-converge-limit", سيٽ_2, &m_converge_حد_بندي);
    DECL_OPTION("-coverage", CbOnOff, [هذا](منطقي flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_اظهار);
    DECL_OPTION("-coverage-expr-max", سيٽ_2, &m_coverage_اظهار_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [هذا, fl](ثابت حرف* ثابت valp) {
        إذا (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } وإلا إذا (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } وإلا إذا (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::ڀريل;
        } وإلا {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warn_وڌيڪ() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", سيٽ_2, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_سٽ);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ثابت خودڪار parse_Bins_حد_بندي = [fl](ثابت حرف* optp, ثابت حرف* valp, uint32_t& limitr) {
        حرف* endp = خالي_اشارو;
        ثابت بنانشان ڊگهو ڊگهو قدر = std::strtoull(valp, &endp, 10);
        إذا (*endp || قدر < 1 || قدر > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            أرجع;
        }
        limitr = static_cast<uint32_t>(قدر);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [هذا, parse_Bins_حد_بندي](ثابت حرف* valp) {
        parse_Bins_حد_بندي("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [هذا, parse_Bins_حد_بندي](ثابت حرف* valp) {
        parse_Bins_حد_بندي("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", سيٽ_2, &m_coverage_Max_ويڪر);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverage_ملايو_Instances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coverage_في_Instance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", Cb_Partial_ميلاپ, [هذا](ثابت حرف* valp) VL_MT_DISABLED {
        جوڙيو_Define(valp, خطأ);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [هذا]() { سيٽ_Debug_گهڻائي(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [هذا](صحيح v) { سيٽ_Debug_گهڻائي(v); }).notForRerun();
    DECL_OPTION("-debugi-", Cb_Partial_ميلاپ_Val, [هذا](ثابت حرف* optp, ثابت حرف* valp) {
        m_debugLevel[optp] = std::atoi(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#جيڪڏهن_وصف_ٿيل VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", سيٽ_2, &m_debugAllocRandom).undocumented();
#جيڪڏهن_پورو
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
    DECL_OPTION("-debug-partition", OnOff, &m_debug_حصا_ڪريو).undocumented();
    DECL_OPTION("-debug-preproc-passthru", OnOff, &m_debugPreprocPassthru).undocumented();
    DECL_OPTION("-debug-protect", OnOff, &m_debugProtect).undocumented();
    DECL_OPTION("-debug-runtime-timeout", سيٽ_2, &m_debugRuntimeTimeout).undocumented();
    DECL_OPTION("-debug-self-test", OnOff, &m_debugSelfTest).undocumented();
    DECL_OPTION("-debug-sigsegv", CbCall, throwSigsegv).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_ڍير_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debug_ويڪر).undocumented();
    DECL_OPTION("-decoration", CbCall, [هذا, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [هذا, fl](ثابت حرف* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [هذا, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [هذا](ثابت حرف* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = سچ;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", Cb_Partial_ميلاپ_Val, [هذا](ثابت حرف* optp, ثابت حرف* valp) {
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
    DECL_OPTION("-error-limit", CbVal, static_cast<فراغ (*)(صحيح)>(&V3Error::error_حد_بندي));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [هذا](ثابت حرف* valp) { m_expand_حد_بندي = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [هذا, fl, &optdir](ثابت حرف* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), سچ);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [هذا, &optdir](ثابت حرف* valp) { جوڙيو_Force_Inc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [هذا, fl, &optdir](ثابت حرف* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), خطأ);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [هذا](ثابت حرف* valp) { جوڙيو_Future0(valp); });
    DECL_OPTION("-future1", CbVal, [هذا](ثابت حرف* valp) { جوڙيو_Future1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_f_ٻٽو_انگ_جاچ_Loops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [هذا](منطقي flag) {
        m_fCaseDecoder = flag;
        m_f_Case_جدول = flag;
        m_f_Case_وڻ = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_f_Case_جدول);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_وڻ);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_ٻٽو_انگ_Op_وڻ);
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
    DECL_OPTION("-fdfg-peephole-", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ڌڪو_هيٺ_ڏانهن_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [هذا, fl](منطقي flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [هذا](منطقي flag) {  //
        m_f_Func_ورهايو_Cat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_f_Func_ورهايو_Cat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [هذا](منطقي flag) {  //
        m_fIcoChangeDetect.سيٽ_True_Or_False(flag);
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
    DECL_OPTION("-fmerge-cond", FOnOff, &m_f_ملايو_Cond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_f_ملايو_Cond_Motion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_f_ٽڪرو);
    DECL_OPTION("-fslice-element-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_f_ٽڪرو_Element_حد_بندي = std::atoi(valp);
        إذا (m_f_ٽڪرو_Element_حد_بندي < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_f_ورهايو);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_f_جدول);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) { جوڙيو_Parameter(optp, خطأ); });
    DECL_OPTION("-gate-stmts", سيٽ_2, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [هذا]() {
        ٻاهر << protect_ڪنجي_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ثابت حرف* valp) {
        ٻاهر << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ثابت حرف* valp) {
        ٻاهر << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_درجيوار);
    DECL_OPTION("-hierarchical-block", CbVal, [هذا](ثابت حرف* valp) {
        ثابت V3_درجيوار_بلاڪ_Option opt{valp};
        m_hier_بلاڪون.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", سيٽ_2, &m_hier_ٻار);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [هذا](ثابت حرف* optp) { m_hierParamsFile.ڌڪو_پٺ({optp, work()}); });

    DECL_OPTION("-I", Cb_Partial_ميلاپ, [هذا, &optdir](ثابت حرف* optp) {
        جوڙيو_Inc_Dir_User(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", سيٽ_2, &m_if_اونهائي);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", سيٽ_2, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", سيٽ_2, &m_inline_CFuncs_حاصل_ضرب);
    DECL_OPTION("-inline-mult", سيٽ_2, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [هذا, fl](صحيح val) {
        m_instr_ڳڻپ_Dpi = val;
        إذا (m_instr_ڳڻپ_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [هذا](ثابت حرف* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = سچ;
    });
    DECL_OPTION("-json-only-output", CbVal, [هذا](ثابت حرف* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = سچ;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::جوڙيو_Ld_Libs));
    DECL_OPTION("-l2-name", سيٽ_2, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [هذا]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [هذا]() { m_l2Name = "v"; }).undocumented();  // Historical
    ثابت خودڪار سيٽ_Lang = [هذا, fl](ثابت حرف* valp) {
        ثابت V3_Lang_ڪوڊ optval{valp};
        إذا (optval.legal()) {
            m_defaultLanguage = optval;
        } وإلا {
            VSpellCheck spell;
            لكل (صحيح i = V3_Lang_ڪوڊ::L_ERROR + 1; i < V3_Lang_ڪوڊ::ENUM_پڇاڙي; ++i) {
                spell.ڌڪو_Candidate(V3_Lang_ڪوڊ{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.بهترين_Candidate_نياپو(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, سيٽ_Lang).notForRerun();
    DECL_OPTION("-language", CbVal, سيٽ_Lang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [هذا](ثابت حرف* valp) { جوڙيو_Lib_Map_File(valp); });
    DECL_OPTION("-lib-create", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", سيٽ_2, &m_localize_Max_سائيز);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::جوڙيو_Make_Flags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [هذا](ثابت حرف* valp) {
        m_makeDir = valp;
        جوڙيو_Inc_Dir_Fallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", سيٽ_2, &m_main_چوٽي_Name);
    DECL_OPTION("-make", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "gmake")) {
            m_gmake = سچ;
        } وإلا إذا (!std::strcmp(valp, "json")) {
            m_makeJson = سچ;
        } وإلا {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", سيٽ_2, &m_funcRecursion);
    DECL_OPTION("-max-num-width", سيٽ_2, &m_max_انگ_ويڪر);
    DECL_OPTION("-mod-prefix", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_mod_اڳياڙي = valp;
    });

    DECL_OPTION("-O0", CbCall, [هذا]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [هذا]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [هذا]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [هذا]() { optimize(3); });

    DECL_OPTION("-o", سيٽ_2, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](منطقي /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_outputGroups = std::atoi(valp);
        إذا (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_output_Groups_سائيز = std::atoll(valp);
        إذا (m_output_Groups_سائيز < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", سيٽ_2, &m_output_ورهايو);
    DECL_OPTION("-output-split-cfuncs", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_output_ورهايو_CFuncs = std::atoi(valp);
        إذا (m_output_ورهايو_CFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_output_ورهايو_CTrace = std::atoi(valp);
        إذا (m_output_ورهايو_CTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", سيٽ_2, &m_preproc_No_سٽ);
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
    DECL_OPTION("-pipe-filter", سيٽ_2, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_اڳياڙي = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preproc_مقرر_ڪري);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_preproc_Token_حد_بندي = std::atoi(valp);
        إذا (m_preproc_Token_حد_بندي <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [هذا]() { m_عوامي = خطأ; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [هذا]() { m_profC = m_profCFuncs = سچ; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [هذا]() {
        m_profC = m_profCFuncs = سچ;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", سيٽ_2, &m_protect_ڪنجي);
    DECL_OPTION("-protect-lib", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = سچ;
    });
    DECL_OPTION("-public", OnOff, &m_عوامي);
    DECL_OPTION("-public-depth", سيٽ_2, &m_عوامي_اونهائي);
    DECL_OPTION("-public-flat-rw", CbOnOff, [هذا](منطقي flag) {
        m_عوامي_Flat_RW = flag;
        v3Global.dpi(سچ);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [هذا](منطقي flag) { m_عوامي_Ignore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [هذا](منطقي flag) {
        m_عوامي_Params = flag;
        v3Global.dpi(سچ);
    });
    DECL_OPTION("-pvalue+", Cb_Partial_ميلاپ,
                [هذا](ثابت حرف* varp) { جوڙيو_Parameter(varp, خطأ); });

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
        m_reloop_حد_بندي = std::atoi(valp);
        إذا (m_reloop_حد_بندي < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_replication_حد_بندي = std::atoi(valp);
        إذا (m_replication_حد_بندي < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [هذا, fl]() {
        decorations(fl, "node");
        جوڙيو_CFlags("-ggdb");
        جوڙيو_Ld_Libs("-ggdb");
        جوڙيو_CFlags("-fsanitize=address,undefined");
        جوڙيو_Ld_Libs("-fsanitize=address,undefined");
        جوڙيو_CFlags("-D_GLIBCXX_DEBUG");
        جوڙيو_CFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [هذا]() {
        m_ٻاهريون_Format_Ok = سچ;
        m_systemC = سچ;
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
    DECL_OPTION("-sv", CbCall, [هذا]() { m_defaultLanguage = V3_Lang_ڪوڊ::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [هذا, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_ڌاڳا = 1;
    });
    DECL_OPTION("-threads", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_ڌاڳا = std::atoi(valp);
        إذا (m_ڌاڳا < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        إذا (m_ڌاڳا == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_ڌاڳا = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_hier_ڌاڳا = std::atoi(valp);
        إذا (m_hier_ڌاڳا < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_ڌاڳا_Coarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "all")) {
            m_ڌاڳا_Dpi_خالص = سچ;
            m_ڌاڳا_Dpi_Unpure = سچ;
        } وإلا إذا (!std::strcmp(valp, "none")) {
            m_ڌاڳا_Dpi_خالص = خطأ;
            m_ڌاڳا_Dpi_Unpure = خطأ;
        } وإلا إذا (!std::strcmp(valp, "pure")) {
            m_ڌاڳا_Dpi_خالص = سچ;
            m_ڌاڳا_Dpi_Unpure = خطأ;
        } وإلا {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warn_وڌيڪ() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_ڌاڳا_Max_MTasks = std::atoi(valp);
        إذا (m_ڌاڳا_Max_MTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [هذا, fl](ثابت حرف* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        إذا (!unit.isNone() && وقت_Override_Unit().isNone()) m_وقت_Default_Unit = unit;
        إذا (!prec.isNone() && وقت_Override_Prec().isNone()) m_وقت_Default_Prec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [هذا, fl](ثابت حرف* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, سچ);
        إذا (!unit.isNone()) {
            m_وقت_Default_Unit = unit;
            m_وقت_Override_Unit = unit;
        }
        إذا (!prec.isNone()) {
            m_وقت_Default_Prec = prec;
            m_وقت_Override_Prec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [هذا](ثابت std::string& flag) { m_چوٽي_Module = Ast_ڳنڍ::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [هذا](ثابت std::string& flag) { m_چوٽي_Module = Ast_ڳنڍ::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", سيٽ_2, &m_چوٽي_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [هذا]() { m_traceEnabledSaif = سچ; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", سيٽ_2, &m_trace_اونهائي);
    DECL_OPTION("-trace-fst", CbCall, [هذا]() {
        m_traceEnabledFst = سچ;
        جوڙيو_Ld_Libs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", سيٽ_2, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", سيٽ_2, &m_trace_Max_ويڪر);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ثابت حرف*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", سيٽ_2, &m_no_Trace_چوٽي);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [هذا]() { m_traceEnabledVcd = سچ; });

    DECL_OPTION("-U", Cb_Partial_ميلاپ, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", سيٽ_2, &m_constraint_Array_حد_بندي);
    DECL_OPTION("-unroll-count", سيٽ_2, &m_unroll_ڳڻپ).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", سيٽ_2, &m_unroll_حد_بندي);
    DECL_OPTION("-unroll-stmts", سيٽ_2, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", سيٽ_2, &m_اڻ_استعمال_ٿيل_Regexp);

    DECL_OPTION("-V", CbCall, [هذا]() {
        show_نسخو(سچ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [هذا, &optdir](ثابت حرف* valp) {
        V3Options::جوڙيو_Library_File(parseFileArg(optdir, valp), work());
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
        show_نسخو(خطأ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_سٽ::globalWarnOff(V3_Error_ڪوڊ::I_LINT, خطأ); });
    DECL_OPTION("-Werror-", Cb_Partial_ميلاپ, [هذا, fl](ثابت حرف* optp) {
        ثابت V3_Error_ڪوڊ ڪوڊ{optp};
        إذا (ڪوڊ == V3_Error_ڪوڊ::EC_ERROR) {
            إذا (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } وإلا {
            V3Error::pretendError(ڪوڊ, سچ);
        }
    });
    DECL_OPTION("-Wfuture-", Cb_Partial_ميلاپ, [هذا](ثابت حرف* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        جوڙيو_Future(optp);
    });
    DECL_OPTION("-Wno-", Cb_Partial_ميلاپ, [fl, &parser](ثابت حرف* optp) VL_MT_DISABLED {
        ثابت string err = File_سٽ::globalWarnOffParse(optp, سچ);
        إذا (!err.خالي_جايون()) {
            ثابت string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    لكل (صحيح i = V3_Error_ڪوڊ::EC_پهريون_WARN; i < V3_Error_ڪوڊ::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        لكل (ثابت string اڳياڙي : {"-Wno-", "-Wwarn-"})
            parser.جوڙيو_Suggestion_Candidate(اڳياڙي + V3_Error_ڪوڊ{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [هذا]() { m_context = خطأ; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(خطأ); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_سٽ::globalWarnOff(V3_Error_ڪوڊ::I_LINT, سچ); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_سٽ::globalWarnOff(V3_Error_ڪوڊ::I_STYLE, سچ); });
    DECL_OPTION("-work", سيٽ_2, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [هذا]() {
        m_pedantic = سچ;
        V3Error::pretendError(V3_Error_ڪوڊ::ASSIGNIN, خطأ);
    });
    DECL_OPTION("-Wwarn-", Cb_Partial_ميلاپ, [هذا, fl, &parser](ثابت حرف* optp) VL_MT_DISABLED {
        ثابت V3_Error_ڪوڊ ڪوڊ{optp};
        إذا (ڪوڊ == V3_Error_ڪوڊ::EC_ERROR) {
            إذا (!isFuture(optp)) {
                ثابت string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } وإلا {
            File_سٽ::globalWarnOff(ڪوڊ, خطأ);
            V3Error::pretendError(ڪوڊ, خطأ);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_سٽ::globalWarnOff(V3_Error_ڪوڊ::I_LINT, خطأ); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_سٽ::globalWarnOff(V3_Error_ڪوڊ::I_STYLE, خطأ); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", سيٽ_2, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "0")) {
            m_x_سونپيو = "0";
        } وإلا إذا (!std::strcmp(valp, "1")) {
            m_x_سونپيو = "1";
        } وإلا إذا (!std::strcmp(valp, "fast")) {
            m_x_سونپيو = "fast";
        } وإلا إذا (!std::strcmp(valp, "unique")) {
            m_x_سونپيو = "unique";
        } وإلا {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warn_وڌيڪ() << "... Suggest '0', '1', 'fast', or 'unique'");
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
                        << fl->warn_وڌيڪ() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_x_Initial_ڪنارو);

    DECL_OPTION("-y", CbVal, [هذا, &optdir](ثابت حرف* valp) {
        جوڙيو_Inc_Dir_User(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    لكل (صحيح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        إذا (argv[i][0] == '-' || argv[i][0] == '+') {
            ثابت std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            صحيح consumed = 0;
            منطقي isForRerun = سچ;

            // Special case for -j which has an optional argument
            إذا (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                صحيح val = 0;
                إذا (i + 1 < argc && std::انگ_آهي(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    إذا (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                إذا (m_buildJobs == -1) m_buildJobs = val;
                إذا (m_verilateJobs == -1) m_verilateJobs = val;
                إذا (m_outputGroups == -1) m_outputGroups = val;
            } وإلا {
                ثابت خودڪار pair = parser.parse(i, argc, argv);
                consumed = pair.پهريون;
                isForRerun = pair.ٻيو;
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
            إذا (argName != "f" && argName != "F") جوڙيو_Arg(argv + i, consumed, isForRerun);
            i += consumed;
            استمر;
        }

        // Filename
        ثابت std::string filename = parseFileArg(optdir, argv[i]);
        منطقي isForRerun = سچ;
        إذا (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::جوڙيو_Cpp_File(filename);
        } وإلا إذا (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::جوڙيو_Ld_Lib_File(filename);
        } وإلا إذا (suffixed(filename, ".vlt")) {
            V3Options::جوڙيو_Vlt_File(filename, work());
            isForRerun = خطأ;
        } وإلا {
            V3Options::جوڙيو_VFile(filename, work());
            isForRerun = خطأ;
        }
        جوڙيو_Arg(filename, isForRerun);
        ++i;
    }

    إذا (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

فراغ V3Options::parseOptsFile(File_سٽ* fl, ثابت string& filename, منطقي rel) VL_MT_DISABLED {
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
    std::string سڄو_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ثابت خودڪار parse_fargs_cmt = [&](ثابت حرف* p) -> فراغ {
        ثابت منطقي بلاڪ_Cmt = p[1] == '*';
        p += 2;
        طالما (std::خال_آهي(*p)) ++p;
        إذا (VString::startsWith(p, "verilator")) {
            p += std::لڙي_ڊيگهه("verilator");
            إذا (std::خال_آهي(*p)) {
                طالما (std::خال_آهي(*p)) ++p;
                إذا (VString::startsWith(p, "fargs")) {
                    p += std::لڙي_ڊيگهه("fargs");
                    إذا (std::خال_آهي(*p)) {
                        طالما (std::خال_آهي(*p)) ++p;
                        إذا (!has_fargs) {
                            has_fargs = سچ;
                            سڄو_file.clear();
                        }
                        سڄو_file += p;
                        إذا (بلاڪ_Cmt) {
                            ثابت std::string::سائيز_type pos = سڄو_file.ڳوليو("*/");
                            إذا (pos == std::string::npos) {
                                fl->v3fatal("/* verilator fargs */ must be on single line");
                            }
                            سڄو_file.resize(pos);
                        }
                        سڄو_file += ' ';
                    }
                }
            }
        }
    };

    منطقي اندريون_Cmt = خطأ;
    طالما (!ifp->eof()) {
        ثابت std::string سٽ = V3Os::getline(*ifp);
        // Strip simple comments
        حرف lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        منطقي space_begin = سچ;  // At beginning or leading spaces only
        لكل (ثابت حرف* p = سٽ.c_str(); *p; lastch = *p++) {
            إذا (اندريون_Cmt) {
                إذا (p[0] == '*' && p[1] == '/') {
                    اندريون_Cmt = خطأ;
                    ++p;
                }
            } وإلا إذا (p[0] == '/' && p[1] == '/' && std::خال_آهي(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                اكسر;  // Ignore to EOL
            } وإلا إذا (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                اكسر;  // Ignore to EOL
            } وإلا إذا (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                اندريون_Cmt = سچ;
                space_begin = خطأ;
                ++p;
            } وإلا {
                إذا (!std::خال_آهي(*p)) space_begin = خطأ;
                إذا (!has_fargs) سڄو_file += *p;
            }
        }
        إذا (!has_fargs) سڄو_file += " ";
    }
    سڄو_file += "\n";  // So string match below is simplified
    إذا (اندريون_Cmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = جديد File_سٽ{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::ويڪٽر<string> دليلون;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    تعداد state : uint8_t {
        ST_اندريون_OPTION,
        ST_ESCAPED_CHAR,
        ST_اندريون_QUOTED_STR,
        ST_اندريون_DOUBLE_QUOTED_STR
    };

    state st = ST_اندريون_OPTION;
    state آخري_st = ST_اندريون_OPTION;
    string arg;
    لكل (string::سائيز_type pos = 0; pos < سڄو_file.ڊيگهائي(); ++pos) {
        حرف curr_char = سڄو_file[pos];
        بدل (st) {
        حالة ST_اندريون_OPTION:  // Get all chars up to a white space or a "="
            إذا (std::خال_آهي(curr_char)) {  // End of option
                إذا (!arg.خالي_جايون()) {  // End of word
                    دليلون.ڌڪو_پٺ(arg);
                }
                arg = "";
                اكسر;
            }
            إذا (curr_char == '\\') {  // Escape char, we wait for next char
                آخري_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                اكسر;
            }
            إذا (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                إذا (pos < سڄو_file.ڊيگهائي()) curr_char = سڄو_file[pos];
                إذا (curr_char == '"') {  // String
                    st = ST_اندريون_QUOTED_STR;
                } وإلا {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                اكسر;
            }
            إذا (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_اندريون_DOUBLE_QUOTED_STR;
                اكسر;
            }
            arg += curr_char;
            اكسر;
        حالة ST_اندريون_QUOTED_STR:  // Just store all chars inside string
            إذا (curr_char != '\'') {
                arg += curr_char;
            } وإلا {  // End of quoted string
                st = ST_اندريون_OPTION;
            }
            اكسر;
        حالة ST_اندريون_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            إذا (curr_char != '"') {
                إذا (curr_char == '\\') {
                    آخري_st = st;
                    st = ST_ESCAPED_CHAR;
                } وإلا {
                    arg += curr_char;
                }
            } وإلا {  // End of double quoted string
                st = ST_اندريون_OPTION;
            }
            اكسر;
        حالة ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = آخري_st;
            اكسر;
        }
    }
    إذا (!arg.خالي_جايون()) {  // Add last word
        دليلون.ڌڪو_پٺ(arg);
    }

    //رستو
    ثابت string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::ويڪٽر<حرف*> argv;
    argv.reserve(دليلون.سائيز() + 1);
    لكل (ثابت string& i : دليلون) argv.ڌڪو_پٺ(const_cast<حرف*>(i.c_str()));
    argv.ڌڪو_پٺ(خالي_اشارو);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<صحيح>(argv.سائيز() - 1), argv.معلومات());
}

//======================================================================

string V3Options::parseFileArg(ثابت string& optdir, ثابت string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    إذا (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    أرجع filename;
}

//======================================================================

فراغ V3Options::show_نسخو(منطقي verbose) {
    ٻاهر << نسخو();
    ٻاهر << "\n";
    إذا (!verbose) أرجع;

    ٻاهر << "\n";
    ٻاهر << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    ٻاهر << "redistribute it and/or modify the Verilator internals under the terms of\n";
    ٻاهر << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    ٻاهر << "License Version 2.0.\n";

    ٻاهر << "\n";
    ٻاهر << "See https://verilator.org for documentation\n";

    ٻاهر << "\n";
    ٻاهر << "Summary of configuration:\n";
    ٻاهر << "  Compiled in defaults if not in environment:\n";
    ٻاهر << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    ٻاهر << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    ٻاهر << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    ٻاهر << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    ٻاهر << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_پاڙ << "\n";
    ٻاهر << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    ٻاهر << "    SystemC system-wide = " << cvt_منزل_Str(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    ٻاهر << "\n";
    ٻاهر << "Environment:\n";
    ٻاهر << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    ٻاهر << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    ٻاهر << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    ٻاهر << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    ٻاهر << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    ٻاهر << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    ٻاهر << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    ٻاهر << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    ٻاهر << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    ٻاهر << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    ٻاهر << "\n";
    ٻاهر << "Supported features (compiled-in or forced by environment):\n";
    ٻاهر << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    ٻاهر << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    ٻاهر << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = جديد V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_اڻ_استعمال_ٿيل_Regexp = "*unused*";
    m_x_سونپيو = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3_Lang_ڪوڊ::mostRecent();

    VName::max_ڊيگهائي(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    جوڙيو_Lib_Ext_V("");  // So include "filename.v" will find the same file
    جوڙيو_Lib_Ext_V(".v");
    جوڙيو_Lib_Ext_V(".sv");
    // Default -I
    جوڙيو_Inc_Dir_Fallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(ميٽيو m_impp, m_impp = خالي_اشارو); }

فراغ V3Options::سيٽ_Debug_گهڻائي(صحيح level) {
    V3Error::debugDefault(level);
    إذا (!m_dumpLevel.ڳڻپ("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = سچ;
    m_debugCheck = سچ;
    إذا (level) {
        ٻاهر << "- Starting " << نسخو() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

بنانشان V3Options::debugLevel(ثابت string& ٽيگ) ثابت VL_MT_محفوظ {
    ثابت خودڪار iter = m_debugLevel.ڳوليو(ٽيگ);
    أرجع iter != m_debugLevel.پڇاڙي() ? iter->ٻيو : V3Error::debugDefault();
}

بنانشان V3Options::debugSrcLevel(ثابت string& srcfile_رستو) ثابت VL_MT_محفوظ {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    أرجع debugLevel(V3Os::filenameNonDirExt(srcfile_رستو));
}

بنانشان V3Options::dumpLevel(ثابت string& ٽيگ) ثابت VL_MT_محفوظ {
    ثابت خودڪار iter = m_dumpLevel.ڳوليو(ٽيگ);
    أرجع iter != m_dumpLevel.پڇاڙي() ? iter->ٻيو : 0;
}

بنانشان V3Options::dumpSrcLevel(ثابت string& srcfile_رستو) ثابت VL_MT_محفوظ {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    أرجع dumpLevel(V3Os::filenameNonDirExt(srcfile_رستو));
}

منطقي V3Options::dump_وڻ_Addrids() ثابت VL_MT_محفوظ {
    ساكن صحيح s_level = -1;
    إذا (VL_UNLIKELY(s_level < 0)) {
        ثابت بنانشان قدر = dumpLevel("tree-addrids");
        إذا (!موجود()) أرجع قدر > 0;
        s_level = static_cast<بنانشان>(قدر);
    }
    أرجع s_level > 0;
}

فراغ V3Options::optimize(صحيح level) {
    // Set all optimizations to on/off
    ثابت منطقي flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_f_ٻٽو_انگ_جاچ_Loops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_f_Case_جدول = flag;
    m_f_Case_وڻ = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_ٻٽو_انگ_Op_وڻ = flag;
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
    m_f_ملايو_Cond = flag;
    m_fReloop = flag;
    m_fReorder = flag;
    m_f_ورهايو = flag;
    m_fSubst = flag;
    m_fSubstConst = flag;
    m_f_جدول = flag;
    // And set specific optimization levels
    إذا (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
