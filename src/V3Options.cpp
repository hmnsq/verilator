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

#شامل "V3PchAstMT.h"

#شامل "V3Options.h"

#شامل "V3Container.h"
#شامل "V3Error.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3Mutex.h"
#شامل "V3OptionParser.h"
#شامل "V3Os.h"
#شامل "V3PreShell.h"
#شامل "V3String.h"

// clang-format off
#شامل <sys/types.h>
#شامل <sys/stat.h>
#اگر_غیر_تعریف_شدہ _WIN32
# include <sys/utsname.h>
#اگر_ختم
#شامل <algorithm>
#شامل <cctype>
#اگر_تعریف_شدہ _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(منوال) (((منوال) & _S_IFMT) == _S_IFDIR)
#ورنہ
# include <dirent.h>
#اگر_ختم
#شامل <fcntl.h>
#شامل <list>
#شامل <map>
#شامل <memory>
#شامل <thread>
#شامل <set>
#شامل <string>

#شامل "config_rev.h"

#اگر defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#اگر_ختم
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

جماعت V3OptionsImp final {
عوامی:
    // TYPES
    استعمال DirMap = std::map<ثابت string, std::سیٹ<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_سطر_دلائل;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, منطقی>> m_all_دلائل;
    VInsertion_سیٹ<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::سیٹ<string> m_inc_Dir_Fallback_سیٹ;  // Include directories (for removing duplicates)
    std::map<ثابت string, V3_Lang_رمز> m_langExts;  // Language extension map
    VInsertion_سیٹ<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    خالی جوڑو_Inc_Dir_User(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        ثابت منطقی inserted = m_incDirUsers.داخل_کرو(dir);
        اگر (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_inc_Dir_Fallback_سیٹ.erase(dir);  // User has priority over Fallback
        }
    }
    خالی جوڑو_Inc_Dir_Fallback(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        اگر (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ثابت خودکار it_ملا_Pair = m_inc_Dir_Fallback_سیٹ.داخل_کرو(dir);
            اگر (it_ملا_Pair.دوسرا) m_incDirFallbacks.ڈالو_عقب(dir);
        }
    }
    خالی جوڑو_Lang_Ext(ثابت string& langext, ثابت V3_Lang_رمز& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        اگر (addext[0] == '.') addext = addext.substr(1);
        (خالی)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    خالی جوڑو_Lib_Ext_V(ثابت string& libext) { m_libExtVs.داخل_کرو(libext); }
    V3OptionsImp() = طےشدہ;
    ~V3OptionsImp() = طےشدہ;
};

//######################################################################
// V3LangCode class functions

V3_Lang_رمز::V3_Lang_رمز(ثابت حرف* textp) {
    // Return code for given string, or ERROR, which is a bad code
    برائے (صحیح codei = V3_Lang_رمز::L_ERROR; codei < V3_Lang_رمز::ENUM_انتہا; ++codei) {
        ثابت V3_Lang_رمز رمز{codei};
        اگر (0 == VL_STRCASECMP(textp, رمز.ascii())) {
            m_e = رمز;
            واپس;
        }
    }
    m_e = V3_Lang_رمز::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ثابت string& قدر, منطقی& badr)
    : m_e{VTimescale::NONE} {
    badr = سچ;
    ثابت string spaceless = VString::removeWhitespace(قدر);
    برائے (صحیح i = TS_100S; i < ENUM_انتہا; ++i) {
        ثابت VTimescale ts{i};
        اگر (spaceless == ts.ascii()) {
            badr = جھوٹ;
            m_e = ts.m_e;
            توڑو;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3_درجہ_وار_بلاک_Option::V3_درجہ_وار_بلاک_Option(ثابت string& opts) {
    VStringList vals;
    منطقی اندر_Str = جھوٹ;
    string cur;
    ساکن ثابت string hier_بلاک("--hierarchical-block");
    File_سطر cmdfl(File_سطر::command_سطر_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    برائے (string::const_iterator it = opts.begin(); it != opts.انتہا();) {
        اگر (اندر_Str) {
            اگر (*it == '\\') {
                ++it;
                اگر (it == opts.انتہا()) {
                    cmdfl.v3error(hier_بلاک + " must not end with \\");
                    توڑو;
                }
                اگر (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hier_بلاک + " does not allow '" + *it + "' after \\");
                    توڑو;
                }
                cur.ڈالو_عقب(*it);
                ++it;
            } ورنہ اگر (*it == '"') {  // end of string
                cur.ڈالو_عقب(*it);
                vals.ڈالو_عقب(cur);
                cur.clear();
                ++it;
                اگر (it != opts.انتہا()) {
                    اگر (*it != ',') {
                        cmdfl.v3error(hier_بلاک + " expects ',', but '" + *it + "' is passed");
                        توڑو;
                    }
                    ++it;
                    اگر (it == opts.انتہا()) {
                        cmdfl.v3error(hier_بلاک + " must not end with ','");
                        توڑو;
                    }
                    اندر_Str = *it == '"';
                    cur.ڈالو_عقب(*it);
                    ++it;
                }
            } ورنہ {
                cur.ڈالو_عقب(*it);
                ++it;
            }
        } ورنہ {
            اگر (*it == '"') {
                cmdfl.v3error(hier_بلاک + " does not allow '\"' in the middle of literal");
                توڑو;
            }
            اگر (*it == ',') {  // end of this parameter
                vals.ڈالو_عقب(cur);
                cur.clear();
                ++it;
                اگر (it == opts.انتہا()) {
                    cmdfl.v3error(hier_بلاک + " must not end with ','");
                    توڑو;
                }
                اندر_Str = *it == '"';
            }
            cur.ڈالو_عقب(*it);
            ++it;
        }
    }
    اگر (!cur.خالی_خانے()) vals.ڈالو_عقب(cur);
    اگر (vals.سائز() >= 2) {
        اگر (vals.سائز() % 2) {
            cmdfl.v3error(hier_بلاک + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } ورنہ {
        cmdfl.v3error(hier_بلاک + " requires at least two comma-separated values");
    }
    برائے (سائز_t i = 2; i + 1 < vals.سائز(); i += 2) {
        ثابت منطقی inserted = m_parameters.emplace(vals[i], vals[i + 1]).دوسرا;
        اگر (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hier_بلاک);
        }
    }
}

//######################################################################
// V3Options class functions

خالی VTimescale::parseSlashed(File_سطر* fl, ثابت حرف* textp, VTimescale& unitr,
                              VTimescale& precr, منطقی allow_خالی_خانے) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ثابت حرف* cp = textp;
    برائے (; std::خلا_ہے(*cp); ++cp) {}
    ثابت حرف* ثابت unitp = cp;
    برائے (; *cp && *cp != '/'; ++cp) {}
    ثابت string unitStr(unitp, cp - unitp);
    برائے (; std::خلا_ہے(*cp); ++cp) {}
    string precStr;
    اگر (*cp == '/') {
        ++cp;
        برائے (; std::خلا_ہے(*cp); ++cp) {}
        ثابت حرف* ثابت precp = cp;
        برائے (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    برائے (; std::خلا_ہے(*cp); ++cp) {}
    اگر (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        واپس;
    }

    منطقی unitbad = جھوٹ;
    ثابت VTimescale unit{unitStr, unitbad /*ref*/};
    اگر (unitbad && !(unitStr.خالی_خانے() && allow_خالی_خانے)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        واپس;
    }
    unitr = unit;

    اگر (!precStr.خالی_خانے()) {
        VTimescale prec{VTimescale::NONE};
        منطقی precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        اگر (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            واپس;
        }
        اگر (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            واپس;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

خالی V3Options::جوڑو_Inc_Dir_User(ثابت string& incdir) { m_impp->جوڑو_Inc_Dir_User(incdir); }
خالی V3Options::جوڑو_Inc_Dir_Fallback(ثابت string& incdir) { m_impp->جوڑو_Inc_Dir_Fallback(incdir); }
خالی V3Options::جوڑو_Lang_Ext(ثابت string& langext, ثابت V3_Lang_رمز& lc) {
    m_impp->جوڑو_Lang_Ext(langext, lc);
}
خالی V3Options::جوڑو_Lib_Ext_V(ثابت string& libext) { m_impp->جوڑو_Lib_Ext_V(libext); }
خالی V3Options::جوڑو_Define(ثابت string& defline, منطقی allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string بائیں = defline;
    جب (بائیں != "") {
        string def = بائیں;
        string::سائز_type pos;
        اگر (allowPlus && ((pos = بائیں.ڈھونڈو('+')) != string::npos)) {
            بائیں = بائیں.substr(pos + 1);
            def.erase(pos);
        } ورنہ {
            بائیں = "";
        }
        string قدر;
        اگر ((pos = def.ڈھونڈو('=')) != string::npos) {
            قدر = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_سطر(def, قدر);
    }
}
خالی V3Options::جوڑو_Parameter(ثابت string& paramline, منطقی allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string بائیں = paramline;
    جب (بائیں != "") {
        string param = بائیں;
        string::سائز_type pos;
        اگر (allowPlus && ((pos = بائیں.ڈھونڈو('+')) != string::npos)) {
            بائیں = بائیں.substr(pos + 1);
            param.erase(pos);
        } ورنہ {
            بائیں = "";
        }
        string قدر;
        اگر ((pos = param.ڈھونڈو('=')) != string::npos) {
            قدر = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << قدر);
        (خالی)m_parameters.erase(param);
        m_parameters[param] = قدر;
    }
}

منطقی V3Options::hasParameter(ثابت string& name) {
    واپس m_parameters.ڈھونڈو(name) != m_parameters.انتہا();
}

string V3Options::parameter(ثابت string& name) {
    string قدر = m_parameters.ڈھونڈو(name)->دوسرا;
    m_parameters.erase(m_parameters.ڈھونڈو(name));
    واپس قدر;
}

خالی V3Options::checkParameters() {
    اگر (!m_parameters.خالی_خانے()) {
        std::stringstream پیغام_2;
        پیغام_2 << "Parameters from the command line were not found in the design:";
        برائے (ثابت خودکار& i : m_parameters) پیغام_2 << " " << i.پہلا;
        v3error(پیغام_2.str());
    }
}

خالی V3Options::جوڑو_Cpp_File(ثابت string& filename) { m_cppFiles.داخل_کرو(filename); }
خالی V3Options::جوڑو_CFlags(ثابت string& filename) { m_cFlags.ڈالو_عقب(filename); }
خالی V3Options::جوڑو_Lib_Map_File(ثابت string& filename) { m_libmapFiles.داخل_کرو(filename); }
خالی V3Options::جوڑو_Compiler_Includes(ثابت string& filename) {
    m_compilerIncludes.داخل_کرو(filename);
}
خالی V3Options::جوڑو_Ld_Lib_File(ثابت string& filename) {
    m_ldLibFiles.ڈالو_عقب(filename);
    جوڑو_Ld_Libs(filename);
}
خالی V3Options::جوڑو_Ld_Libs(ثابت string& filename) { m_ldLibs.ڈالو_عقب(filename); }
خالی V3Options::جوڑو_Make_Flags(ثابت string& filename) { m_makeFlags.ڈالو_عقب(filename); }
خالی V3Options::جوڑو_Future(ثابت string& flag) { m_futures.داخل_کرو(flag); }
خالی V3Options::جوڑو_Future0(ثابت string& flag) { m_future0s.داخل_کرو(flag); }
خالی V3Options::جوڑو_Future1(ثابت string& flag) { m_future1s.داخل_کرو(flag); }
منطقی V3Options::isFuture(ثابت string& flag) ثابت {
    واپس m_futures.ڈھونڈو(flag) != m_futures.انتہا();
}
منطقی V3Options::isFuture0(ثابت string& flag) ثابت {
    واپس m_future0s.ڈھونڈو(flag) != m_future0s.انتہا();
}
منطقی V3Options::isFuture1(ثابت string& flag) ثابت {
    واپس m_future1s.ڈھونڈو(flag) != m_future1s.انتہا();
}
منطقی V3Options::isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت {
    واپس m_libraryFiles.ڈھونڈو({filename, libname}) != m_libraryFiles.انتہا();
}
خالی V3Options::جوڑو_Library_File(ثابت string& filename, ثابت string& libname) {
    m_libraryFiles.داخل_کرو({filename, libname});
}
خالی V3Options::جوڑو_VFile(ثابت string& filename, ثابت string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ڈالو_عقب({filename, libname});  // Not emplace
}
خالی V3Options::جوڑو_Vlt_File(ثابت string& filename, ثابت string& libname) {
    m_vltFiles.داخل_کرو({filename, libname});
}
خالی V3Options::جوڑو_Force_Inc(ثابت string& filename) { m_forceIncs.ڈالو_عقب(filename); }

خالی V3Options::جوڑو_سطر_Arg(ثابت string& arg) { m_impp->m_سطر_دلائل.ڈالو_عقب(arg); }

خالی V3Options::جوڑو_Arg(حرف** argv, سائز_t گنتی, منطقی isForRerun) {
    m_impp->m_all_دلائل.emplace_عقب();
    std::pair<std::list<std::string>, منطقی>& pair = m_impp->m_all_دلائل.عقب();
    pair.دوسرا = isForRerun;
    برائے (سائز_t n = 0; n < گنتی; ++n) pair.پہلا.emplace_عقب(argv[n]);
}

خالی V3Options::جوڑو_Arg(ثابت std::string& arg, منطقی isForRerun) {
    m_impp->m_all_دلائل.emplace_عقب();
    std::pair<std::list<std::string>, منطقی>& pair = m_impp->m_all_دلائل.عقب();
    pair.دوسرا = isForRerun;
    pair.پہلا.emplace_عقب(arg);
}

ثابت std::list<std::pair<std::list<std::string>, منطقی>>& V3Options::all_دلائل() ثابت {
    واپس m_impp->m_all_دلائل;
}

string V3Options::all_دلائل_String() ثابت VL_MT_محفوظ {
    string نتیجہ;
    برائے (ثابت خودکار& pair : m_impp->m_all_دلائل) {
        برائے (ثابت string& arg : pair.پہلا) {
            اگر (!نتیجہ.خالی_خانے()) نتیجہ += " ";
            نتیجہ += arg;
        }
    }
    واپس نتیجہ;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::all_دلائل_String_For_Hier_بلاک(منطقی for_چوٹی) ثابت {
    // cppcheck-suppress shadowFunction
    std::سیٹ<string> vFiles;
    برائے (ثابت خودکار& vFile : m_vFiles) vFiles.داخل_کرو(vFile.filename());
    string برآمد;
    منطقی stripArg = جھوٹ;
    منطقی strip_Arg_If_عدد = جھوٹ;
    برائے (ثابت خودکار& pair : m_impp->m_all_دلائل) {
        برائے (ثابت string& arg : pair.پہلا) {
            اگر (stripArg) {
                stripArg = جھوٹ;
                جاری;
            }
            اگر (strip_Arg_If_عدد) {
                strip_Arg_If_عدد = جھوٹ;
                اگر (ہندسہ_ہے(arg[0])) جاری;
            }
            صحیح skip = 0;
            اگر (arg.لمبائی_2() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } ورنہ اگر (arg.لمبائی_2() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            اگر (skip > 0) {  // arg is an option
                ثابت string opt = arg.substr(skip);  // Remove '-' in the beginning
                ثابت صحیح عدد_Strip = strip_Options_For_بچہ_تسلسل(opt, for_چوٹی);
                اگر (عدد_Strip) {
                    UASSERT(0 <= عدد_Strip && عدد_Strip <= 3, "should be one of 0, 1, 2, 3");
                    اگر (عدد_Strip == 2) stripArg = سچ;
                    اگر (عدد_Strip == 3) strip_Arg_If_عدد = سچ;
                    جاری;
                }
            } ورنہ {  // Not an option
                اگر (vFiles.ڈھونڈو(arg) != vFiles.انتہا()  // Remove HDL
                    || m_cppFiles.ڈھونڈو(arg) != m_cppFiles.انتہا()) {  // Remove C++
                    جاری;
                }
            }
            اگر (برآمد != "") برآمد += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            برآمد += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    واپس برآمد;
}

خالی V3Options::cc_سیٹ() {  // --cc
    m_برآمد_Format_Ok = سچ;
    m_systemC = جھوٹ;
}

خالی V3Options::decorations(File_سطر* fl, ثابت string& arg) {  // --decorations
    اگر (arg == "none") {
        m_decoration = جھوٹ;
        m_decorationNodes = جھوٹ;
    } ورنہ اگر (arg == "node") {
        m_decoration = سچ;
        m_decorationNodes = سچ;
    } ورنہ اگر (arg == "medium") {
        m_decoration = سچ;
        m_decorationNodes = جھوٹ;
    } ورنہ {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warn_مزید() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::سمتیہ<std::string> V3Options::traceClassBases() ثابت VL_MT_محفوظ {
    std::سمتیہ<std::string> نتیجہ;
    اگر (traceEnabledFst()) نتیجہ.emplace_عقب("VerilatedFst");
    اگر (traceEnabledSaif()) نتیجہ.emplace_عقب("VerilatedSaif");
    اگر (traceEnabledVcd()) نتیجہ.emplace_عقب("VerilatedVcd");
    واپس نتیجہ;
}
std::سمتیہ<std::string> V3Options::traceClassLangs() ثابت VL_MT_محفوظ {
    std::سمتیہ<std::string> نتیجہ;
    برائے (ثابت خودکار& cbase : traceClassBases())
        نتیجہ.emplace_عقب(cbase + (systemC() ? "Sc" : "C"));
    واپس نتیجہ;
}
std::سمتیہ<std::string> V3Options::trace_ماخذ_Bases() ثابت VL_MT_محفوظ {
    std::سمتیہ<std::string> نتیجہ;
    اگر (traceEnabledFst()) نتیجہ.emplace_عقب("verilated_fst");
    اگر (traceEnabledSaif()) نتیجہ.emplace_عقب("verilated_saif");
    اگر (traceEnabledVcd()) نتیجہ.emplace_عقب("verilated_vcd");
    واپس نتیجہ;
}
std::سمتیہ<std::string> V3Options::trace_ماخذ_Langs() ثابت VL_MT_محفوظ {
    std::سمتیہ<std::string> نتیجہ = trace_ماخذ_Bases();
    برائے (std::string& str : نتیجہ) str += systemC() ? "_sc"s : "_c"s;
    واپس نتیجہ;
}
std::string V3Options::trace_Class_بنیاد() ثابت VL_MT_محفوظ {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().خالی_خانے(), "Call traceClassBase only when trace() enabled");
    واپس traceClassBases().front();
}
std::string V3Options::traceClassLang() ثابت VL_MT_محفوظ {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().خالی_خانے(), "Call traceClassLang only when trace() enabled");
    واپس traceClassLangs().front();
}

//######################################################################
// File searching

منطقی V3Options::file_Stat_معمول(ثابت string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ساخت stat sstat;  // Stat information
    ثابت صحیح err = stat(filename.c_str(), &sstat);
    اگر (err != 0) واپس جھوٹ;
    اگر (S_ISDIR(sstat.st_منوال)) واپس جھوٹ;
    واپس سچ;
}

string V3Options::fileExists(ثابت string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ثابت string dir = V3Os::filenameDir(filename);
    ثابت string basename = V3Os::filenameNonDir(filename);

    خودکار diriter = m_impp->m_dirMap.ڈھونڈو(dir);
    اگر (diriter == m_impp->m_dirMap.انتہا()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::سیٹ<string>());
        diriter = m_impp->m_dirMap.ڈھونڈو(dir);

        std::سیٹ<string>* ثابت setp = &(diriter->دوسرا);

#اگر_تعریف_شدہ _MSC_VER
        کوشش {
            برائے (ثابت خودکار& dir_اندراج : std::filesystem::directory_iterator(dir.c_str()))
                setp->داخل_کرو(dir_اندراج.راستہ().filename().string());
        } پکڑو (std::filesystem::filesystem_error ثابت& ex) {
            (خالی)ex;
            واپس "";
        }
#ورنہ
        اگر (DIR* ثابت dirp = opendir(dir.c_str())) {
            جب (ساخت dirent* direntp = readdir(dirp)) setp->داخل_کرو(direntp->d_name);
            closedir(dirp);
        }
#اگر_ختم
    }
    // Find it
    ثابت std::سیٹ<string>* ثابت filesetp = &(diriter->دوسرا);
    ثابت خودکار fileiter = filesetp->ڈھونڈو(basename);
    اگر (fileiter == filesetp->انتہا()) واپس "";  // Not found
    // Check if it is a directory, ignore if so
    ثابت string filename_برآمد = V3Os::filenameJoin(dir, basename);
    اگر (!file_Stat_معمول(filename_برآمد)) واپس "";  // Directory
    واپس filename_برآمد;
}

string V3Options::file_راستہ_Check_One_Dir(ثابت string& modname, ثابت string& dirname) {
    برائے (ثابت string& i : m_impp->m_libExtVs) {
        ثابت string fn = V3Os::filenameJoin(dirname, modname + i);
        ثابت string exists = fileExists(fn);
        اگر (exists != "") واپس exists;
    }
    واپس "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
صحیح V3Options::strip_Options_For_بچہ_تسلسل(ثابت string& opt, منطقی for_چوٹی) {
    // Options to strip for both the top wrapper, and hier blocks
    ساکن ثابت std::unordered_map<std::string, صحیح> commonOpts{
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
    اگر (commonOpts.گنتی(opt)) واپس commonOpts.at(opt);

    // Options to strip only for hier blocks
    اگر (!for_چوٹی) {
        ساکن ثابت std::unordered_map<std::string, صحیح> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        اگر (subOpts.گنتی(opt)) واپس subOpts.at(opt);
        اگر (VString::startsWith(opt, "G")) واپس 1;
        اگر (VString::startsWith(opt, "pvalue")) واپس 1;
    }

    // Do not strip
    واپس 0;
}

خالی V3Options::validateIdentifier(File_سطر* fl, ثابت string& arg, ثابت string& opt) {
    اگر (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::file_راستہ(File_سطر* fl, ثابت string& modname, ثابت string& lastpath,
                           ثابت string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ثابت string filename = V3Os::filenameCleanup(VName::dehash(modname));
    اگر (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ثابت string exists = file_راستہ_Check_One_Dir(filename, "");
        اگر (exists != "") واپس exists;
    }
    برائے (ثابت string& dir : m_impp->m_incDirUsers) {
        ثابت string exists = file_راستہ_Check_One_Dir(filename, dir);
        اگر (exists != "") واپس exists;
    }
    برائے (ثابت string& dir : m_impp->m_incDirFallbacks) {
        ثابت string exists = file_راستہ_Check_One_Dir(filename, dir);
        اگر (exists != "") واپس exists;
    }

    اگر (m_relativeIncludes) {
        ثابت string exists = file_راستہ_Check_One_Dir(filename, lastpath);
        اگر (exists != "") واپس V3Os::filename_Real_راستہ(exists);
    }

    // Warn and return not found
    اگر (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_راستہ_Looked_پیغام(fl, filename));
    }
    واپس "";
}

string V3Options::file_راستہ_Looked_پیغام(File_سطر* /*fl*/, ثابت string& modname) {
    ساکن منطقی s_shown_notfound_پیغام = جھوٹ;
    std::ostringstream ss;
    اگر (modname.ڈھونڈو("__Vhsh") != string::npos) {
        ss << V3Error::warn_مزید() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warn_مزید() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } ورنہ اگر (!s_shown_notfound_پیغام) {
        s_shown_notfound_پیغام = سچ;
        اگر (m_impp->m_incDirUsers.خالی_خانے()) {
            ss << V3Error::warn_مزید()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warn_مزید() << "... Looked in:\n";
        برائے (ثابت string& dir : m_impp->m_incDirUsers) {
            برائے (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_مزید() << "     " << fn << "\n";
            }
        }
        برائے (ثابت string& dir : m_impp->m_incDirFallbacks) {
            برائے (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_مزید() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warn_مزید() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    واپس ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3_Lang_رمز V3Options::fileLanguage(ثابت string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::سائز_type pos;
    اگر (filename == V3Options::get_Std_Package_راستہ() || filename == V3Options::get_Std_Waiver_راستہ()) {
        واپس V3_Lang_رمز::mostRecent();
    } ورنہ اگر ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ثابت خودکار it = m_impp->m_langExts.ڈھونڈو(ext);
        اگر (it != m_impp->m_langExts.انتہا()) واپس it->دوسرا;
    }
    واپس m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ثابت string& var) {
    // If update below, also update V3Options::showVersion()
    اگر (var == "MAKE") {
        واپس getenvMAKE();
    } ورنہ اگر (var == "PERL") {
        واپس getenvPERL();
    } ورنہ اگر (var == "PYTHON3") {
        واپس getenvPYTHON3();
    } ورنہ اگر (var == "SYSTEMC") {
        واپس getenvSYSTEMC();
    } ورنہ اگر (var == "SYSTEMC_ARCH") {
        واپس getenvSYSTEMC_ARCH();
    } ورنہ اگر (var == "SYSTEMC_INCLUDE") {
        واپس getenvSYSTEMC_INCLUDE();
    } ورنہ اگر (var == "SYSTEMC_LIBDIR") {
        واپس getenvSYSTEMC_LIBDIR();
    } ورنہ اگر (var == "VERILATOR_ROOT") {
        واپس getenv_VERILATOR_جڑ();
    } ورنہ {
        واپس V3Os::getenvStr(var, "");
    }
}

#اگر_تعریف_شدہ __FreeBSD__
string V3Options::getenvMAKE() { واپس V3Os::getenvStr("MAKE", "gmake"); }
#ورنہ
string V3Options::getenvMAKE() { واپس V3Os::getenvStr("MAKE", "make"); }
#اگر_ختم

string V3Options::getenvMAKEFLAGS() {  //
    واپس V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    واپس V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    واپس V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    واپس V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_ARCH};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    اگر (var == "") {
#اگر defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ثابت string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#وگرنہ_اگر defined(_WIN32)
        ثابت string sysname = "WIN32";
        var = "win32";
#ورنہ
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ساخت utsname uts;
        uname(&uts);
        ثابت string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        اگر (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } ورنہ اگر (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } ورنہ {
            var = "linux";
        }
#اگر_ختم
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    واپس var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    اگر (var == "") {
        ثابت string sc = getenvSYSTEMC();
        اگر (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    واپس V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    اگر (var == "") {
        ثابت string sc = getenvSYSTEMC();
        ثابت string arch = getenvSYSTEMC_ARCH();
        اگر (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    واپس V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_جڑ() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_VERILATOR_جڑ};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    اگر (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    واپس V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_VERILATOR_SOLVER};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    واپس var;
}

string V3Options::get_Std_Package_راستہ() {
    واپس V3Os::filenameJoin(getenv_VERILATOR_جڑ(), "include", "verilated_std.sv");
}
string V3Options::get_Std_Waiver_راستہ() {
    واپس V3Os::filenameJoin(getenv_VERILATOR_جڑ(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ثابت string& var) {
    // If update below, also update V3Options::showVersion()
    اگر (var == "COROUTINES" && coroutineSupport()) {
        واپس "1";
    } ورنہ اگر (var == "TSAN" && tsanSupport()) {
        واپس "1";
    } ورنہ اگر (var == "DEV_ASAN" && devAsan()) {
        واپس "1";
    } ورنہ اگر (var == "DEV_GCOV" && devGcov()) {
        واپس "1";
        // cppcheck-suppress knownConditionTrueFalse
    } ورنہ اگر (var == "SYSTEMC" && systemCFound()) {
        واپس "1";
    } ورنہ {
        واپس "";
    }
}

منطقی V3Options::systemCSystemWide() {
#اگر_تعریف_شدہ HAVE_SYSTEMC
    واپس سچ;
#ورنہ
    واپس جھوٹ;
#اگر_ختم
}

منطقی V3Options::systemCFound() {
    واپس (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().خالی_خانے() && !getenvSYSTEMC_LIBDIR().خالی_خانے()));
}

منطقی V3Options::coroutineSupport() {
#اگر_تعریف_شدہ HAVE_COROUTINES
    واپس سچ;
#ورنہ
    واپس جھوٹ;
#اگر_ختم
}

منطقی V3Options::devAsan() {
#اگر_تعریف_شدہ HAVE_DEV_ASAN
    واپس سچ;
#ورنہ
    واپس جھوٹ;
#اگر_ختم
}

منطقی V3Options::devGcov() {
#اگر_تعریف_شدہ HAVE_DEV_GCOV
    واپس سچ;
#ورنہ
    واپس جھوٹ;
#اگر_ختم
}

منطقی V3Options::tsanSupport() {
#اگر_تعریف_شدہ HAVE_TSAN
    واپس سچ;
#ورنہ
    واپس جھوٹ;
#اگر_ختم
}

//######################################################################
// V3 Options notification methods

خالی V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_سطر* ثابت cmdfl = نیا File_سطر{File_سطر::command_سطر_Filename()};

    اگر (!برآمد_Format_Ok() && v3Global.opt.مین()) cc_سیٹ();  // --main implies --cc if not provided
    اگر (!برآمد_Format_Ok() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    اگر (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::سمتیہ<std::string> backendFlags;
    اگر (m_build) {
        اگر (m_ثنائی)
            backendFlags.emplace_عقب("--binary");
        ورنہ
            backendFlags.emplace_عقب("--build");
    }
    اگر (m_preprocOnly) backendFlags.emplace_عقب("-E");
    اگر (m_dpiHdrOnly) backendFlags.emplace_عقب("--dpi-hdr-only");
    اگر (m_lintOnly) backendFlags.emplace_عقب("--lint-only");
    اگر (m_jsonOnly) backendFlags.emplace_عقب("--json-only");
    اگر (backendFlags.سائز() > 1) {
        std::string backendFlagsString = backendFlags.front();
        برائے (سائز_t i = 1; i < backendFlags.سائز(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    اگر (m_exe && !v3Global.opt.libCreate().خالی_خانے()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    اگر (!m_gmake && !m_makeJson) m_gmake = سچ;

    اگر (m_درجہ_وار && (m_hier_بچہ || !m_hier_بلاکیں.خالی_خانے())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    اگر (m_hier_بچہ) {
        اگر (m_hier_بلاکیں.خالی_خانے()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = جھوٹ;
    }

    اگر (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = سچ;  // No format, with --trace means wanted --trace-vcd
    }
    اگر (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = سچ;
    ثابت صحیح ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    اگر (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    اگر (protectIds()) {
        اگر (all_عوامی()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warn_مزید() + "... Suggest remove --public.");
        }
        اگر (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warn_مزید() + "... Suggest remove --trace.");
        }
        اگر (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warn_مزید() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    اگر (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.سیٹ_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    اگر (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.سیٹ_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    اگر (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.سیٹ_True_Or_False(سچ);

    // Default split limits if not specified
    اگر (m_output_بانٹو_CFuncs < 0) m_output_بانٹو_CFuncs = m_output_بانٹو;
    اگر (m_output_بانٹو_CTrace < 0) m_output_بانٹو_CTrace = m_output_بانٹو;

    اگر (v3Global.opt.مین() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    اگر (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        اگر (درجہ_وار()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    اگر (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    اگر (v3Global.opt.timing().is_سیٹ_True() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    اگر (!m_dumpLevel.گنتی("tree") && m_dumpLevel.گنتی("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(دھاگے() >= 1, "'threads()' must return a value >= 1");
    اگر (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    اگر (m_buildJobs == -1) m_buildJobs = 1;
    اگر (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    اگر (timing().is_سیٹ_True()) V3PreShell::define_Cmd_سطر("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    اگر (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.سیٹ_True_Or_False(جھوٹ);

    // === Leave last
    // Mark options as available
    m_دستیاب = سچ;
}

//######################################################################
// V3 Options accessors

string V3Options::نسخہ() VL_خالص {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_مقصد_Str(DTVERSION_rev);
    واپس ver;
}

string V3Options::protect_کلید_Defaulted() VL_MT_محفوظ {
    ساکن V3Mutex s_mutex;
    ثابت V3_تالا_Guard تالا_2{s_mutex};
    اگر (m_protect_کلید.خالی_خانے()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_کلید = "VL-KEY-" + digest.digestBase64();
    }
    واپس m_protect_کلید;
}

خالی V3Options::throwSigsegv() {  // LCOV_EXCL_START
#اگر !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<متلون حرف*>(خالی_اشاریہ) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#اگر_ختم
}  // LCOV_EXCL_STOP

VTimescale V3Options::وقت_Compute_Prec(ثابت VTimescale& flag) ثابت {
    اگر (!وقت_Override_Prec().isNone()) {
        واپس وقت_Override_Prec();
    } ورنہ اگر (flag.isNone()) {
        واپس وقت_Default_Prec();
    } ورنہ {
        واپس flag;
    }
}

VTimescale V3Options::وقت_Compute_Unit(ثابت VTimescale& flag) ثابت {
    اگر (!وقت_Override_Unit().isNone()) {
        واپس وقت_Override_Unit();
    } ورنہ اگر (flag.isNone()) {
        واپس وقت_Default_Unit();
    } ورنہ {
        واپس flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(صحیح argc, حرف** argv) {
    // Return list of arguments as simple string
    string opts;
    برائے (صحیح i = 0; i < argc; ++i) {
        اگر (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    واپس opts;
}

//######################################################################
// V3 Options Parsing

خالی V3Options::parseOpts(File_سطر* fl, صحیح argc, حرف** argv) VL_MT_DISABLED {
    // Save command line options
    برائے (صحیح i = 0; i < argc; ++i) جوڑو_سطر_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    اگر (vFiles().خالی_خانے()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    اگر (سابقہ() == "" && چوٹی_Module() != "") m_سابقہ = "V"s + چوٹی_Module();  // Already encoded
    اگر (سابقہ() == "" && vFiles().سائز() >= 1)
        m_سابقہ
            = "V"s + Ast_گرہ::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    اگر (mod_سابقہ() == "") m_mod_سابقہ = سابقہ();

    // Find files in makedir
    جوڑو_Inc_Dir_Fallback(makeDir());
}

//======================================================================

منطقی V3Options::suffixed(ثابت string& sw, ثابت حرف* arg) {
    اگر (std::لڑی_لمبائی(arg) > sw.لمبائی_2()) واپس جھوٹ;
    واپس (0 == std::strcmp(sw.c_str() + sw.لمبائی_2() - std::لڑی_لمبائی(arg), arg));
}

خالی V3Options::parseOptsList(File_سطر* fl, ثابت string& optdir, صحیح argc,
                              حرف** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ثابت V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_لیبل_فہرست;

    ثابت خودکار callStrSetter = [یہ](خالی (V3Options::*cbStr)(ثابت string&)) {
        واپس [یہ, cbStr](ثابت string& v) { (یہ->*cbStr)(v); };
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
    DECL_OPTION("+define+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) VL_MT_DISABLED {
        جوڑو_Define(optp, سچ);
    }).notForRerun();
    DECL_OPTION("+incdir+", Cb_Partial_مطابقت, [یہ, &optdir](ثابت حرف* optp) {
        string dirs = optp;
        string::سائز_type pos;
        جب ((pos = dirs.ڈھونڈو('+')) != string::npos) {
            جوڑو_Inc_Dir_User(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        جوڑو_Inc_Dir_User(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        string exts = optp;
        string::سائز_type pos;
        جب ((pos = exts.ڈھونڈو('+')) != string::npos) {
            جوڑو_Lib_Ext_V(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        جوڑو_Lib_Ext_V(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        جوڑو_Lang_Ext(optp, V3_Lang_رمز::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](منطقی) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [یہ](منطقی flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ثابت حرف*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [یہ](منطقی flag) {
        m_bboxUnsup = flag;
        File_سطر::globalWarnOff(V3_Error_رمز::E_UNSUPPORTED, سچ);
    });
    DECL_OPTION("-binary", CbCall, [یہ]() {
        m_ثنائی = سچ;
        m_build = سچ;
        m_exe = سچ;
        m_main = سچ;
        اگر (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", سیٹ_2, &m_build);
    DECL_OPTION("-build-dep-bin", سیٹ_2, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [یہ, fl](ثابت حرف* valp) {
        صحیح val = std::atoi(valp);
        اگر (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } ورنہ اگر (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::جوڑو_CFlags));
    DECL_OPTION("-cc", CbCall, [یہ]() { cc_سیٹ(); });
    DECL_OPTION("-clk", CbVal, [fl](ثابت std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ثابت std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", سیٹ_2, &m_comp_حد_بندی_بلاکیں).undocumented();
    DECL_OPTION("-comp-limit-members", سیٹ_2,
                &m_comp_حد_بندی_Members)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", سیٹ_2, &m_comp_حد_بندی_Parens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](صحیح val) { VName::max_لمبائی(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [یہ, fl](ثابت حرف* valp) {
        اگر (!std::strcmp(valp, "clang")) {
            m_comp_حد_بندی_بلاکیں = 80;  // limit unknown
            m_comp_حد_بندی_Members = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_comp_حد_بندی_Parens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } ورنہ اگر (!std::strcmp(valp, "gcc")) {
            m_comp_حد_بندی_بلاکیں = 0;  // Bug free
            m_comp_حد_بندی_Members = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_comp_حد_بندی_Parens = 240;  // Unlimited, but generate same code as for clang
        } ورنہ اگر (!std::strcmp(valp, "msvc")) {
            m_comp_حد_بندی_بلاکیں = 80;  // 128, but allow some room
            m_comp_حد_بندی_Members = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_comp_حد_بندی_Parens = 80;  // 128, but allow some room
        } ورنہ {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warn_مزید() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::جوڑو_Compiler_Includes));
    DECL_OPTION("-converge-limit", سیٹ_2, &m_converge_حد_بندی);
    DECL_OPTION("-coverage", CbOnOff, [یہ](منطقی flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_اظہاریہ);
    DECL_OPTION("-coverage-expr-max", سیٹ_2, &m_coverage_اظہاریہ_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [یہ, fl](ثابت حرف* ثابت valp) {
        اگر (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } ورنہ اگر (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } ورنہ اگر (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::بھرا;
        } ورنہ {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warn_مزید() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", سیٹ_2, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_سطر);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ثابت خودکار parse_Bins_حد_بندی = [fl](ثابت حرف* optp, ثابت حرف* valp, uint32_t& limitr) {
        حرف* endp = خالی_اشاریہ;
        ثابت غیرعلامتی طویل طویل قدر = std::strtoull(valp, &endp, 10);
        اگر (*endp || قدر < 1 || قدر > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            واپس;
        }
        limitr = static_cast<uint32_t>(قدر);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [یہ, parse_Bins_حد_بندی](ثابت حرف* valp) {
        parse_Bins_حد_بندی("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [یہ, parse_Bins_حد_بندی](ثابت حرف* valp) {
        parse_Bins_حد_بندی("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", سیٹ_2, &m_coverage_Max_چوڑائی);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverage_ملاؤ_Instances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coverage_فی_Instance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", Cb_Partial_مطابقت, [یہ](ثابت حرف* valp) VL_MT_DISABLED {
        جوڑو_Define(valp, جھوٹ);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [یہ]() { سیٹ_Debug_منوال(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [یہ](صحیح v) { سیٹ_Debug_منوال(v); }).notForRerun();
    DECL_OPTION("-debugi-", Cb_Partial_مطابقت_Val, [یہ](ثابت حرف* optp, ثابت حرف* valp) {
        m_debugLevel[optp] = std::atoi(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#اگر_تعریف_شدہ VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", سیٹ_2, &m_debugAllocRandom).undocumented();
#اگر_ختم
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
    DECL_OPTION("-debug-partition", OnOff, &m_debug_تقسیم).undocumented();
    DECL_OPTION("-debug-preproc-passthru", OnOff, &m_debugPreprocPassthru).undocumented();
    DECL_OPTION("-debug-protect", OnOff, &m_debugProtect).undocumented();
    DECL_OPTION("-debug-runtime-timeout", سیٹ_2, &m_debugRuntimeTimeout).undocumented();
    DECL_OPTION("-debug-self-test", OnOff, &m_debugSelfTest).undocumented();
    DECL_OPTION("-debug-sigsegv", CbCall, throwSigsegv).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_ڈھیر_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debug_چوڑائی).undocumented();
    DECL_OPTION("-decoration", CbCall, [یہ, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [یہ, fl](ثابت حرف* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [یہ, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [یہ](ثابت حرف* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = سچ;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", Cb_Partial_مطابقت_Val, [یہ](ثابت حرف* optp, ثابت حرف* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [یہ](منطقی flag) {
        اگر (flag) {
            m_stdPackage = جھوٹ;
            m_stdWaiver = جھوٹ;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<خالی (*)(صحیح)>(&V3Error::error_حد_بندی));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [یہ](ثابت حرف* valp) { m_expand_حد_بندی = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [یہ, fl, &optdir](ثابت حرف* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), سچ);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [یہ, &optdir](ثابت حرف* valp) { جوڑو_Force_Inc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [یہ, fl, &optdir](ثابت حرف* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), جھوٹ);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [یہ](ثابت حرف* valp) { جوڑو_Future0(valp); });
    DECL_OPTION("-future1", CbVal, [یہ](ثابت حرف* valp) { جوڑو_Future1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_f_ثنائی_ہندسہ_جائزہ_Loops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [یہ](منطقی flag) {
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
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_ثنائی_ہندسہ_Op_درخت);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [یہ](منطقی flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [یہ](منطقی flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ڈالو_نیچے_کو_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [یہ, fl](منطقی flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [یہ](منطقی flag) {  //
        m_f_Func_بانٹو_Cat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_f_Func_بانٹو_Cat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [یہ](منطقی flag) {  //
        m_fIcoChangeDetect.سیٹ_True_Or_False(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_f_ملاؤ_Cond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_f_ملاؤ_Cond_Motion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_f_قاش);
    DECL_OPTION("-fslice-element-limit", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_f_قاش_Element_حد_بندی = std::atoi(valp);
        اگر (m_f_قاش_Element_حد_بندی < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_f_بانٹو);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_f_جدول);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) { جوڑو_Parameter(optp, جھوٹ); });
    DECL_OPTION("-gate-stmts", سیٹ_2, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [یہ]() {
        خارج << protect_کلید_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ثابت حرف* valp) {
        خارج << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ثابت حرف* valp) {
        خارج << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_درجہ_وار);
    DECL_OPTION("-hierarchical-block", CbVal, [یہ](ثابت حرف* valp) {
        ثابت V3_درجہ_وار_بلاک_Option opt{valp};
        m_hier_بلاکیں.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", سیٹ_2, &m_hier_بچہ);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [یہ](ثابت حرف* optp) { m_hierParamsFile.ڈالو_عقب({optp, work()}); });

    DECL_OPTION("-I", Cb_Partial_مطابقت, [یہ, &optdir](ثابت حرف* optp) {
        جوڑو_Inc_Dir_User(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", سیٹ_2, &m_if_گہرائی);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", سیٹ_2, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", سیٹ_2, &m_inline_CFuncs_حاصل_ضرب);
    DECL_OPTION("-inline-mult", سیٹ_2, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [یہ, fl](صحیح val) {
        m_instr_گنتی_Dpi = val;
        اگر (m_instr_گنتی_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [یہ](ثابت حرف* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = سچ;
    });
    DECL_OPTION("-json-only-output", CbVal, [یہ](ثابت حرف* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = سچ;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::جوڑو_Ld_Libs));
    DECL_OPTION("-l2-name", سیٹ_2, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [یہ]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [یہ]() { m_l2Name = "v"; }).undocumented();  // Historical
    ثابت خودکار سیٹ_Lang = [یہ, fl](ثابت حرف* valp) {
        ثابت V3_Lang_رمز optval{valp};
        اگر (optval.legal()) {
            m_defaultLanguage = optval;
        } ورنہ {
            VSpellCheck spell;
            برائے (صحیح i = V3_Lang_رمز::L_ERROR + 1; i < V3_Lang_رمز::ENUM_انتہا; ++i) {
                spell.ڈالو_Candidate(V3_Lang_رمز{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.بہترین_Candidate_پیغام(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, سیٹ_Lang).notForRerun();
    DECL_OPTION("-language", CbVal, سیٹ_Lang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [یہ](ثابت حرف* valp) { جوڑو_Lib_Map_File(valp); });
    DECL_OPTION("-lib-create", CbVal, [یہ, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", سیٹ_2, &m_localize_Max_سائز);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::جوڑو_Make_Flags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [یہ](ثابت حرف* valp) {
        m_makeDir = valp;
        جوڑو_Inc_Dir_Fallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", سیٹ_2, &m_main_چوٹی_Name);
    DECL_OPTION("-make", CbVal, [یہ, fl](ثابت حرف* valp) {
        اگر (!std::strcmp(valp, "gmake")) {
            m_gmake = سچ;
        } ورنہ اگر (!std::strcmp(valp, "json")) {
            m_makeJson = سچ;
        } ورنہ {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", سیٹ_2, &m_funcRecursion);
    DECL_OPTION("-max-num-width", سیٹ_2, &m_max_عدد_چوڑائی);
    DECL_OPTION("-mod-prefix", CbVal, [یہ, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_mod_سابقہ = valp;
    });

    DECL_OPTION("-O0", CbCall, [یہ]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [یہ]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [یہ]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [یہ]() { optimize(3); });

    DECL_OPTION("-o", سیٹ_2, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](منطقی /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_outputGroups = std::atoi(valp);
        اگر (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_output_Groups_سائز = std::atoll(valp);
        اگر (m_output_Groups_سائز < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", سیٹ_2, &m_output_بانٹو);
    DECL_OPTION("-output-split-cfuncs", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_output_بانٹو_CFuncs = std::atoi(valp);
        اگر (m_output_بانٹو_CFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_output_بانٹو_CTrace = std::atoi(valp);
        اگر (m_output_بانٹو_CTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", سیٹ_2, &m_preproc_No_سطر);
    DECL_OPTION("-pins64", CbCall, [یہ]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [یہ]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_pinsBv = std::atoi(valp);
        اگر (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [یہ](منطقی flag) {
        m_pinsScUint = flag;
        اگر (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [یہ](منطقی flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [یہ](منطقی flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", سیٹ_2, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [یہ, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_سابقہ = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preproc_متعین_کرتا);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_preproc_Token_حد_بندی = std::atoi(valp);
        اگر (m_preproc_Token_حد_بندی <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [یہ]() { m_عوامی = جھوٹ; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [یہ]() { m_profC = m_profCFuncs = سچ; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [یہ]() {
        m_profC = m_profCFuncs = سچ;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", سیٹ_2, &m_protect_کلید);
    DECL_OPTION("-protect-lib", CbVal, [یہ, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = سچ;
    });
    DECL_OPTION("-public", OnOff, &m_عوامی);
    DECL_OPTION("-public-depth", سیٹ_2, &m_عوامی_گہرائی);
    DECL_OPTION("-public-flat-rw", CbOnOff, [یہ](منطقی flag) {
        m_عوامی_Flat_RW = flag;
        v3Global.dpi(سچ);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [یہ](منطقی flag) { m_عوامی_Ignore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [یہ](منطقی flag) {
        m_عوامی_Params = flag;
        v3Global.dpi(سچ);
    });
    DECL_OPTION("-pvalue+", Cb_Partial_مطابقت,
                [یہ](ثابت حرف* varp) { جوڑو_Parameter(varp, جھوٹ); });

    DECL_OPTION("-quiet", CbOnOff, [یہ](منطقی flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_reloop_حد_بندی = std::atoi(valp);
        اگر (m_reloop_حد_بندی < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_replication_حد_بندی = std::atoi(valp);
        اگر (m_replication_حد_بندی < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [یہ, fl]() {
        decorations(fl, "node");
        جوڑو_CFlags("-ggdb");
        جوڑو_Ld_Libs("-ggdb");
        جوڑو_CFlags("-fsanitize=address,undefined");
        جوڑو_Ld_Libs("-fsanitize=address,undefined");
        جوڑو_CFlags("-D_GLIBCXX_DEBUG");
        جوڑو_CFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [یہ]() {
        m_برآمد_Format_Ok = سچ;
        m_systemC = سچ;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [یہ](منطقی flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [یہ](منطقی flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [یہ]() { m_defaultLanguage = V3_Lang_رمز::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [یہ, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_دھاگے = 1;
    });
    DECL_OPTION("-threads", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_دھاگے = std::atoi(valp);
        اگر (m_دھاگے < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        اگر (m_دھاگے == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_دھاگے = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_hier_دھاگے = std::atoi(valp);
        اگر (m_hier_دھاگے < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_دھاگے_Coarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [یہ, fl](ثابت حرف* valp) {
        اگر (!std::strcmp(valp, "all")) {
            m_دھاگے_Dpi_خالص = سچ;
            m_دھاگے_Dpi_Unpure = سچ;
        } ورنہ اگر (!std::strcmp(valp, "none")) {
            m_دھاگے_Dpi_خالص = جھوٹ;
            m_دھاگے_Dpi_Unpure = جھوٹ;
        } ورنہ اگر (!std::strcmp(valp, "pure")) {
            m_دھاگے_Dpi_خالص = سچ;
            m_دھاگے_Dpi_Unpure = جھوٹ;
        } ورنہ {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warn_مزید() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [یہ, fl](ثابت حرف* valp) {
        m_دھاگے_Max_MTasks = std::atoi(valp);
        اگر (m_دھاگے_Max_MTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [یہ, fl](ثابت حرف* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        اگر (!unit.isNone() && وقت_Override_Unit().isNone()) m_وقت_Default_Unit = unit;
        اگر (!prec.isNone() && وقت_Override_Prec().isNone()) m_وقت_Default_Prec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [یہ, fl](ثابت حرف* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, سچ);
        اگر (!unit.isNone()) {
            m_وقت_Default_Unit = unit;
            m_وقت_Override_Unit = unit;
        }
        اگر (!prec.isNone()) {
            m_وقت_Default_Prec = prec;
            m_وقت_Override_Prec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [یہ](ثابت std::string& flag) { m_چوٹی_Module = Ast_گرہ::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [یہ](ثابت std::string& flag) { m_چوٹی_Module = Ast_گرہ::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", سیٹ_2, &m_چوٹی_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [یہ]() { m_traceEnabledSaif = سچ; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", سیٹ_2, &m_trace_گہرائی);
    DECL_OPTION("-trace-fst", CbCall, [یہ]() {
        m_traceEnabledFst = سچ;
        جوڑو_Ld_Libs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", سیٹ_2, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", سیٹ_2, &m_trace_Max_چوڑائی);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ثابت حرف*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", سیٹ_2, &m_no_Trace_چوٹی);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [یہ]() { m_traceEnabledVcd = سچ; });

    DECL_OPTION("-U", Cb_Partial_مطابقت, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", سیٹ_2, &m_constraint_Array_حد_بندی);
    DECL_OPTION("-unroll-count", سیٹ_2, &m_unroll_گنتی).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", سیٹ_2, &m_unroll_حد_بندی);
    DECL_OPTION("-unroll-stmts", سیٹ_2, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", سیٹ_2, &m_غیر_مستعمل_Regexp);

    DECL_OPTION("-V", CbCall, [یہ]() {
        show_نسخہ(سچ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [یہ, &optdir](ثابت حرف* valp) {
        V3Options::جوڑو_Library_File(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [یہ, fl](ثابت حرف* valp) {
        صحیح val = std::atoi(valp);
        اگر (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } ورنہ اگر (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [یہ]() {
        show_نسخہ(جھوٹ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_سطر::globalWarnOff(V3_Error_رمز::I_LINT, جھوٹ); });
    DECL_OPTION("-Werror-", Cb_Partial_مطابقت, [یہ, fl](ثابت حرف* optp) {
        ثابت V3_Error_رمز رمز{optp};
        اگر (رمز == V3_Error_رمز::EC_ERROR) {
            اگر (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } ورنہ {
            V3Error::pretendError(رمز, سچ);
        }
    });
    DECL_OPTION("-Wfuture-", Cb_Partial_مطابقت, [یہ](ثابت حرف* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        جوڑو_Future(optp);
    });
    DECL_OPTION("-Wno-", Cb_Partial_مطابقت, [fl, &parser](ثابت حرف* optp) VL_MT_DISABLED {
        ثابت string err = File_سطر::globalWarnOffParse(optp, سچ);
        اگر (!err.خالی_خانے()) {
            ثابت string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    برائے (صحیح i = V3_Error_رمز::EC_پہلا_WARN; i < V3_Error_رمز::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        برائے (ثابت string سابقہ : {"-Wno-", "-Wwarn-"})
            parser.جوڑو_Suggestion_Candidate(سابقہ + V3_Error_رمز{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [یہ]() { m_context = جھوٹ; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(جھوٹ); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_سطر::globalWarnOff(V3_Error_رمز::I_LINT, سچ); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_سطر::globalWarnOff(V3_Error_رمز::I_STYLE, سچ); });
    DECL_OPTION("-work", سیٹ_2, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [یہ]() {
        m_pedantic = سچ;
        V3Error::pretendError(V3_Error_رمز::ASSIGNIN, جھوٹ);
    });
    DECL_OPTION("-Wwarn-", Cb_Partial_مطابقت, [یہ, fl, &parser](ثابت حرف* optp) VL_MT_DISABLED {
        ثابت V3_Error_رمز رمز{optp};
        اگر (رمز == V3_Error_رمز::EC_ERROR) {
            اگر (!isFuture(optp)) {
                ثابت string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } ورنہ {
            File_سطر::globalWarnOff(رمز, جھوٹ);
            V3Error::pretendError(رمز, جھوٹ);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_سطر::globalWarnOff(V3_Error_رمز::I_LINT, جھوٹ); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_سطر::globalWarnOff(V3_Error_رمز::I_STYLE, جھوٹ); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", سیٹ_2, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [یہ, fl](ثابت حرف* valp) {
        اگر (!std::strcmp(valp, "0")) {
            m_x_تفویض = "0";
        } ورنہ اگر (!std::strcmp(valp, "1")) {
            m_x_تفویض = "1";
        } ورنہ اگر (!std::strcmp(valp, "fast")) {
            m_x_تفویض = "fast";
        } ورنہ اگر (!std::strcmp(valp, "unique")) {
            m_x_تفویض = "unique";
        } ورنہ {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warn_مزید() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [یہ, fl](ثابت حرف* valp) {
        اگر (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } ورنہ اگر (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } ورنہ اگر (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } ورنہ {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warn_مزید() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_x_Initial_کنارہ);

    DECL_OPTION("-y", CbVal, [یہ, &optdir](ثابت حرف* valp) {
        جوڑو_Inc_Dir_User(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    برائے (صحیح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        اگر (argv[i][0] == '-' || argv[i][0] == '+') {
            ثابت std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            صحیح consumed = 0;
            منطقی isForRerun = سچ;

            // Special case for -j which has an optional argument
            اگر (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                صحیح val = 0;
                اگر (i + 1 < argc && std::ہندسہ_ہے(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    اگر (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                اگر (m_buildJobs == -1) m_buildJobs = val;
                اگر (m_verilateJobs == -1) m_verilateJobs = val;
                اگر (m_outputGroups == -1) m_outputGroups = val;
            } ورنہ {
                ثابت خودکار pair = parser.parse(i, argc, argv);
                consumed = pair.پہلا;
                isForRerun = pair.دوسرا;
                اگر (consumed) {
                    // Already processed
                } ورنہ اگر (isFuture0(argName)) {
                    consumed = 1;
                } ورنہ اگر (isFuture1(argName)) {
                    consumed = 2;
                } ورنہ {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            اگر (argName != "f" && argName != "F") جوڑو_Arg(argv + i, consumed, isForRerun);
            i += consumed;
            جاری;
        }

        // Filename
        ثابت std::string filename = parseFileArg(optdir, argv[i]);
        منطقی isForRerun = سچ;
        اگر (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::جوڑو_Cpp_File(filename);
        } ورنہ اگر (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::جوڑو_Ld_Lib_File(filename);
        } ورنہ اگر (suffixed(filename, ".vlt")) {
            V3Options::جوڑو_Vlt_File(filename, work());
            isForRerun = جھوٹ;
        } ورنہ {
            V3Options::جوڑو_VFile(filename, work());
            isForRerun = جھوٹ;
        }
        جوڑو_Arg(filename, isForRerun);
        ++i;
    }

    اگر (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

خالی V3Options::parseOptsFile(File_سطر* fl, ثابت string& filename, منطقی rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    اگر (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        واپس;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    منطقی has_fargs = جھوٹ;
    std::string پورا_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ثابت خودکار parse_fargs_cmt = [&](ثابت حرف* p) -> خالی {
        ثابت منطقی بلاک_Cmt = p[1] == '*';
        p += 2;
        جب (std::خلا_ہے(*p)) ++p;
        اگر (VString::startsWith(p, "verilator")) {
            p += std::لڑی_لمبائی("verilator");
            اگر (std::خلا_ہے(*p)) {
                جب (std::خلا_ہے(*p)) ++p;
                اگر (VString::startsWith(p, "fargs")) {
                    p += std::لڑی_لمبائی("fargs");
                    اگر (std::خلا_ہے(*p)) {
                        جب (std::خلا_ہے(*p)) ++p;
                        اگر (!has_fargs) {
                            has_fargs = سچ;
                            پورا_file.clear();
                        }
                        پورا_file += p;
                        اگر (بلاک_Cmt) {
                            ثابت std::string::سائز_type pos = پورا_file.ڈھونڈو("*/");
                            اگر (pos == std::string::npos) {
                                fl->v3fatal("/* verilator fargs */ must be on single line");
                            }
                            پورا_file.resize(pos);
                        }
                        پورا_file += ' ';
                    }
                }
            }
        }
    };

    منطقی اندر_Cmt = جھوٹ;
    جب (!ifp->eof()) {
        ثابت std::string سطر = V3Os::getline(*ifp);
        // Strip simple comments
        حرف lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        منطقی space_begin = سچ;  // At beginning or leading spaces only
        برائے (ثابت حرف* p = سطر.c_str(); *p; lastch = *p++) {
            اگر (اندر_Cmt) {
                اگر (p[0] == '*' && p[1] == '/') {
                    اندر_Cmt = جھوٹ;
                    ++p;
                }
            } ورنہ اگر (p[0] == '/' && p[1] == '/' && std::خلا_ہے(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                توڑو;  // Ignore to EOL
            } ورنہ اگر (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                توڑو;  // Ignore to EOL
            } ورنہ اگر (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                اندر_Cmt = سچ;
                space_begin = جھوٹ;
                ++p;
            } ورنہ {
                اگر (!std::خلا_ہے(*p)) space_begin = جھوٹ;
                اگر (!has_fargs) پورا_file += *p;
            }
        }
        اگر (!has_fargs) پورا_file += " ";
    }
    پورا_file += "\n";  // So string match below is simplified
    اگر (اندر_Cmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = نیا File_سطر{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::سمتیہ<string> دلائل;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    شمار state : uint8_t {
        ST_اندر_OPTION,
        ST_ESCAPED_CHAR,
        ST_اندر_QUOTED_STR,
        ST_اندر_DOUBLE_QUOTED_STR
    };

    state st = ST_اندر_OPTION;
    state آخری_st = ST_اندر_OPTION;
    string arg;
    برائے (string::سائز_type pos = 0; pos < پورا_file.لمبائی_2(); ++pos) {
        حرف curr_char = پورا_file[pos];
        انتخاب (st) {
        حالت ST_اندر_OPTION:  // Get all chars up to a white space or a "="
            اگر (std::خلا_ہے(curr_char)) {  // End of option
                اگر (!arg.خالی_خانے()) {  // End of word
                    دلائل.ڈالو_عقب(arg);
                }
                arg = "";
                توڑو;
            }
            اگر (curr_char == '\\') {  // Escape char, we wait for next char
                آخری_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                توڑو;
            }
            اگر (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                اگر (pos < پورا_file.لمبائی_2()) curr_char = پورا_file[pos];
                اگر (curr_char == '"') {  // String
                    st = ST_اندر_QUOTED_STR;
                } ورنہ {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                توڑو;
            }
            اگر (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_اندر_DOUBLE_QUOTED_STR;
                توڑو;
            }
            arg += curr_char;
            توڑو;
        حالت ST_اندر_QUOTED_STR:  // Just store all chars inside string
            اگر (curr_char != '\'') {
                arg += curr_char;
            } ورنہ {  // End of quoted string
                st = ST_اندر_OPTION;
            }
            توڑو;
        حالت ST_اندر_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            اگر (curr_char != '"') {
                اگر (curr_char == '\\') {
                    آخری_st = st;
                    st = ST_ESCAPED_CHAR;
                } ورنہ {
                    arg += curr_char;
                }
            } ورنہ {  // End of double quoted string
                st = ST_اندر_OPTION;
            }
            توڑو;
        حالت ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = آخری_st;
            توڑو;
        }
    }
    اگر (!arg.خالی_خانے()) {  // Add last word
        دلائل.ڈالو_عقب(arg);
    }

    //راستہ
    ثابت string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::سمتیہ<حرف*> argv;
    argv.reserve(دلائل.سائز() + 1);
    برائے (ثابت string& i : دلائل) argv.ڈالو_عقب(const_cast<حرف*>(i.c_str()));
    argv.ڈالو_عقب(خالی_اشاریہ);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<صحیح>(argv.سائز() - 1), argv.ڈیٹا());
}

//======================================================================

string V3Options::parseFileArg(ثابت string& optdir, ثابت string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    اگر (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    واپس filename;
}

//======================================================================

خالی V3Options::show_نسخہ(منطقی verbose) {
    خارج << نسخہ();
    خارج << "\n";
    اگر (!verbose) واپس;

    خارج << "\n";
    خارج << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    خارج << "redistribute it and/or modify the Verilator internals under the terms of\n";
    خارج << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    خارج << "License Version 2.0.\n";

    خارج << "\n";
    خارج << "See https://verilator.org for documentation\n";

    خارج << "\n";
    خارج << "Summary of configuration:\n";
    خارج << "  Compiled in defaults if not in environment:\n";
    خارج << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    خارج << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    خارج << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    خارج << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    خارج << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_جڑ << "\n";
    خارج << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    خارج << "    SystemC system-wide = " << cvt_مقصد_Str(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    خارج << "\n";
    خارج << "Environment:\n";
    خارج << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    خارج << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    خارج << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    خارج << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    خارج << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    خارج << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    خارج << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    خارج << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    خارج << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    خارج << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    خارج << "\n";
    خارج << "Supported features (compiled-in or forced by environment):\n";
    خارج << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    خارج << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    خارج << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = نیا V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_غیر_مستعمل_Regexp = "*unused*";
    m_x_تفویض = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3_Lang_رمز::mostRecent();

    VName::max_لمبائی(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    جوڑو_Lib_Ext_V("");  // So include "filename.v" will find the same file
    جوڑو_Lib_Ext_V(".v");
    جوڑو_Lib_Ext_V(".sv");
    // Default -I
    جوڑو_Inc_Dir_Fallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(حذف m_impp, m_impp = خالی_اشاریہ); }

خالی V3Options::سیٹ_Debug_منوال(صحیح level) {
    V3Error::debugDefault(level);
    اگر (!m_dumpLevel.گنتی("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = سچ;
    m_debugCheck = سچ;
    اگر (level) {
        خارج << "- Starting " << نسخہ() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

غیرعلامتی V3Options::debugLevel(ثابت string& لیبل) ثابت VL_MT_محفوظ {
    ثابت خودکار iter = m_debugLevel.ڈھونڈو(لیبل);
    واپس iter != m_debugLevel.انتہا() ? iter->دوسرا : V3Error::debugDefault();
}

غیرعلامتی V3Options::debugSrcLevel(ثابت string& srcfile_راستہ) ثابت VL_MT_محفوظ {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    واپس debugLevel(V3Os::filenameNonDirExt(srcfile_راستہ));
}

غیرعلامتی V3Options::dumpLevel(ثابت string& لیبل) ثابت VL_MT_محفوظ {
    ثابت خودکار iter = m_dumpLevel.ڈھونڈو(لیبل);
    واپس iter != m_dumpLevel.انتہا() ? iter->دوسرا : 0;
}

غیرعلامتی V3Options::dumpSrcLevel(ثابت string& srcfile_راستہ) ثابت VL_MT_محفوظ {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    واپس dumpLevel(V3Os::filenameNonDirExt(srcfile_راستہ));
}

منطقی V3Options::dump_درخت_Addrids() ثابت VL_MT_محفوظ {
    ساکن صحیح s_level = -1;
    اگر (VL_UNLIKELY(s_level < 0)) {
        ثابت غیرعلامتی قدر = dumpLevel("tree-addrids");
        اگر (!دستیاب()) واپس قدر > 0;
        s_level = static_cast<غیرعلامتی>(قدر);
    }
    واپس s_level > 0;
}

خالی V3Options::optimize(صحیح level) {
    // Set all optimizations to on/off
    ثابت منطقی flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_f_ثنائی_ہندسہ_جائزہ_Loops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_f_Case_جدول = flag;
    m_f_Case_درخت = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_ثنائی_ہندسہ_Op_درخت = flag;
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
    m_f_ملاؤ_Cond = flag;
    m_fReloop = flag;
    m_fReorder = flag;
    m_f_بانٹو = flag;
    m_fSubst = flag;
    m_fSubstConst = flag;
    m_f_جدول = flag;
    // And set specific optimization levels
    اگر (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
