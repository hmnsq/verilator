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
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#ضمن <algorithm>
#ضمن <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#وإلا
# include <dirent.h>
#endif
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
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

صنف V3OptionsImp final {
عام:
    // TYPES
    using DirMap = std::map<ثابت string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_قۇر_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, منطقي>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<ثابت string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    فراغ addIncDirUser(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        ثابت منطقي inserted = m_incDirUsers.قىستۇرۇش(dir);
        إذا (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    فراغ addIncDirFallback(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        إذا (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ثابت auto itFoundPair = m_incDirFallbackSet.قىستۇرۇش(dir);
            إذا (itFoundPair.second) m_incDirFallbacks.ئىتتىرىش_back(dir);
        }
    }
    فراغ addLangExt(ثابت string& langext, ثابت V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        إذا (addext[0] == '.') addext = addext.substr(1);
        (فراغ)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    فراغ addLibExtV(ثابت string& libext) { m_libExtVs.قىستۇرۇش(libext); }
    V3OptionsImp() = افتراضي;
    ~V3OptionsImp() = افتراضي;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(ثابت حرف* textp) {
    // Return code for given string, or ERROR, which is a bad code
    لكل (صحيح codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        ثابت V3LangCode code{codei};
        إذا (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            أرجع;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ثابت string& قىممەت, منطقي& badr)
    : m_e{VTimescale::NONE} {
    badr = true;
    ثابت string spaceless = VString::removeWhitespace(قىممەت);
    لكل (صحيح i = TS_100S; i < _ENUM_END; ++i) {
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
V3HierarchicalBlockOption::V3HierarchicalBlockOption(ثابت string& opts) {
    VStringList vals;
    منطقي inStr = خطأ;
    string cur;
    ساكن ثابت string hierBlock("--hierarchical-block");
    File_قۇر cmdfl(File_قۇر::command_قۇر_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    لكل (string::const_iterator it = opts.begin(); it != opts.end();) {
        إذا (inStr) {
            إذا (*it == '\\') {
                ++it;
                إذا (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    اكسر;
                }
                إذا (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    اكسر;
                }
                cur.ئىتتىرىش_back(*it);
                ++it;
            } وإلا إذا (*it == '"') {  // end of string
                cur.ئىتتىرىش_back(*it);
                vals.ئىتتىرىش_back(cur);
                cur.clear();
                ++it;
                إذا (it != opts.end()) {
                    إذا (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        اكسر;
                    }
                    ++it;
                    إذا (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        اكسر;
                    }
                    inStr = *it == '"';
                    cur.ئىتتىرىش_back(*it);
                    ++it;
                }
            } وإلا {
                cur.ئىتتىرىش_back(*it);
                ++it;
            }
        } وإلا {
            إذا (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                اكسر;
            }
            إذا (*it == ',') {  // end of this parameter
                vals.ئىتتىرىش_back(cur);
                cur.clear();
                ++it;
                إذا (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    اكسر;
                }
                inStr = *it == '"';
            }
            cur.ئىتتىرىش_back(*it);
            ++it;
        }
    }
    إذا (!cur.empty()) vals.ئىتتىرىش_back(cur);
    إذا (vals.size() >= 2) {
        إذا (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } وإلا {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    لكل (size_t i = 2; i + 1 < vals.size(); i += 2) {
        ثابت منطقي inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        إذا (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

فراغ VTimescale::parseSlashed(File_قۇر* fl, ثابت حرف* textp, VTimescale& unitr,
                              VTimescale& precr, منطقي allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ثابت حرف* cp = textp;
    لكل (; std::isspace(*cp); ++cp) {}
    ثابت حرف* ثابت unitp = cp;
    لكل (; *cp && *cp != '/'; ++cp) {}
    ثابت string unitStr(unitp, cp - unitp);
    لكل (; std::isspace(*cp); ++cp) {}
    string precStr;
    إذا (*cp == '/') {
        ++cp;
        لكل (; std::isspace(*cp); ++cp) {}
        ثابت حرف* ثابت precp = cp;
        لكل (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    لكل (; std::isspace(*cp); ++cp) {}
    إذا (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        أرجع;
    }

    منطقي unitbad = خطأ;
    ثابت VTimescale unit{unitStr, unitbad /*ref*/};
    إذا (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        أرجع;
    }
    unitr = unit;

    إذا (!precStr.empty()) {
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

فراغ V3Options::addIncDirUser(ثابت string& incdir) { m_impp->addIncDirUser(incdir); }
فراغ V3Options::addIncDirFallback(ثابت string& incdir) { m_impp->addIncDirFallback(incdir); }
فراغ V3Options::addLangExt(ثابت string& langext, ثابت V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
فراغ V3Options::addLibExtV(ثابت string& libext) { m_impp->addLibExtV(libext); }
فراغ V3Options::addDefine(ثابت string& defline, منطقي allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string سول = defline;
    طالما (سول != "") {
        string def = سول;
        string::size_type pos;
        إذا (allowPlus && ((pos = سول.find('+')) != string::npos)) {
            سول = سول.substr(pos + 1);
            def.erase(pos);
        } وإلا {
            سول = "";
        }
        string قىممەت;
        إذا ((pos = def.find('=')) != string::npos) {
            قىممەت = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_قۇر(def, قىممەت);
    }
}
فراغ V3Options::addParameter(ثابت string& paramline, منطقي allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string سول = paramline;
    طالما (سول != "") {
        string param = سول;
        string::size_type pos;
        إذا (allowPlus && ((pos = سول.find('+')) != string::npos)) {
            سول = سول.substr(pos + 1);
            param.erase(pos);
        } وإلا {
            سول = "";
        }
        string قىممەت;
        إذا ((pos = param.find('=')) != string::npos) {
            قىممەت = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << قىممەت);
        (فراغ)m_parameters.erase(param);
        m_parameters[param] = قىممەت;
    }
}

منطقي V3Options::hasParameter(ثابت string& name) {
    أرجع m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(ثابت string& name) {
    string قىممەت = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    أرجع قىممەت;
}

فراغ V3Options::checkParameters() {
    إذا (!m_parameters.empty()) {
        std::stringstream ئۇچۇر;
        ئۇچۇر << "Parameters from the command line were not found in the design:";
        لكل (ثابت auto& i : m_parameters) ئۇچۇر << " " << i.first;
        v3error(ئۇچۇر.str());
    }
}

فراغ V3Options::addCppFile(ثابت string& filename) { m_cppFiles.قىستۇرۇش(filename); }
فراغ V3Options::addCFlags(ثابت string& filename) { m_cFlags.ئىتتىرىش_back(filename); }
فراغ V3Options::addLibMapFile(ثابت string& filename) { m_libmapFiles.قىستۇرۇش(filename); }
فراغ V3Options::addCompilerIncludes(ثابت string& filename) {
    m_compilerIncludes.قىستۇرۇش(filename);
}
فراغ V3Options::addLdLibFile(ثابت string& filename) {
    m_ldLibFiles.ئىتتىرىش_back(filename);
    addLdLibs(filename);
}
فراغ V3Options::addLdLibs(ثابت string& filename) { m_ldLibs.ئىتتىرىش_back(filename); }
فراغ V3Options::addMakeFlags(ثابت string& filename) { m_makeFlags.ئىتتىرىش_back(filename); }
فراغ V3Options::addFuture(ثابت string& flag) { m_futures.قىستۇرۇش(flag); }
فراغ V3Options::addFuture0(ثابت string& flag) { m_future0s.قىستۇرۇش(flag); }
فراغ V3Options::addFuture1(ثابت string& flag) { m_future1s.قىستۇرۇش(flag); }
منطقي V3Options::isFuture(ثابت string& flag) ثابت {
    أرجع m_futures.find(flag) != m_futures.end();
}
منطقي V3Options::isFuture0(ثابت string& flag) ثابت {
    أرجع m_future0s.find(flag) != m_future0s.end();
}
منطقي V3Options::isFuture1(ثابت string& flag) ثابت {
    أرجع m_future1s.find(flag) != m_future1s.end();
}
منطقي V3Options::isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت {
    أرجع m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
فراغ V3Options::addLibraryFile(ثابت string& filename, ثابت string& libname) {
    m_libraryFiles.قىستۇرۇش({filename, libname});
}
فراغ V3Options::addVFile(ثابت string& filename, ثابت string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ئىتتىرىش_back({filename, libname});  // Not emplace
}
فراغ V3Options::addVltFile(ثابت string& filename, ثابت string& libname) {
    m_vltFiles.قىستۇرۇش({filename, libname});
}
فراغ V3Options::addForceInc(ثابت string& filename) { m_forceIncs.ئىتتىرىش_back(filename); }

فراغ V3Options::add_قۇر_Arg(ثابت string& arg) { m_impp->m_قۇر_Args.ئىتتىرىش_back(arg); }

فراغ V3Options::addArg(حرف** argv, size_t سانى, منطقي isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, منطقي>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    لكل (size_t n = 0; n < سانى; ++n) pair.first.emplace_back(argv[n]);
}

فراغ V3Options::addArg(ثابت std::string& arg, منطقي isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, منطقي>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

ثابت std::list<std::pair<std::list<std::string>, منطقي>>& V3Options::allArgs() ثابت {
    أرجع m_impp->m_allArgs;
}

string V3Options::allArgsString() ثابت VL_MT_SAFE {
    string result;
    لكل (ثابت auto& pair : m_impp->m_allArgs) {
        لكل (ثابت string& arg : pair.first) {
            إذا (!result.empty()) result += " ";
            result += arg;
        }
    }
    أرجع result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(منطقي for_چوققا) ثابت {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    لكل (ثابت auto& vFile : m_vFiles) vFiles.قىستۇرۇش(vFile.filename());
    string out;
    منطقي stripArg = خطأ;
    منطقي strip_Arg_If_رەقەم = خطأ;
    لكل (ثابت auto& pair : m_impp->m_allArgs) {
        لكل (ثابت string& arg : pair.first) {
            إذا (stripArg) {
                stripArg = خطأ;
                استمر;
            }
            إذا (strip_Arg_If_رەقەم) {
                strip_Arg_If_رەقەم = خطأ;
                إذا (isdigit(arg[0])) استمر;
            }
            صحيح skip = 0;
            إذا (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } وإلا إذا (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            إذا (skip > 0) {  // arg is an option
                ثابت string opt = arg.substr(skip);  // Remove '-' in the beginning
                ثابت صحيح رەقەم_Strip = stripOptionsForChildRun(opt, for_چوققا);
                إذا (رەقەم_Strip) {
                    UASSERT(0 <= رەقەم_Strip && رەقەم_Strip <= 3, "should be one of 0, 1, 2, 3");
                    إذا (رەقەم_Strip == 2) stripArg = true;
                    إذا (رەقەم_Strip == 3) strip_Arg_If_رەقەم = true;
                    استمر;
                }
            } وإلا {  // Not an option
                إذا (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    استمر;
                }
            }
            إذا (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    أرجع out;
}

فراغ V3Options::ccSet() {  // --cc
    m_outFormatOk = true;
    m_systemC = خطأ;
}

فراغ V3Options::decorations(File_قۇر* fl, ثابت string& arg) {  // --decorations
    إذا (arg == "none") {
        m_decoration = خطأ;
        m_decorationNodes = خطأ;
    } وإلا إذا (arg == "node") {
        m_decoration = true;
        m_decorationNodes = true;
    } وإلا إذا (arg == "medium") {
        m_decoration = true;
        m_decorationNodes = خطأ;
    } وإلا {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() ثابت VL_MT_SAFE {
    std::vector<std::string> result;
    إذا (traceEnabledFst()) result.emplace_back("VerilatedFst");
    إذا (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    إذا (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    أرجع result;
}
std::vector<std::string> V3Options::traceClassLangs() ثابت VL_MT_SAFE {
    std::vector<std::string> result;
    لكل (ثابت auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    أرجع result;
}
std::vector<std::string> V3Options::traceSourceBases() ثابت VL_MT_SAFE {
    std::vector<std::string> result;
    إذا (traceEnabledFst()) result.emplace_back("verilated_fst");
    إذا (traceEnabledSaif()) result.emplace_back("verilated_saif");
    إذا (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    أرجع result;
}
std::vector<std::string> V3Options::traceSourceLangs() ثابت VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    لكل (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    أرجع result;
}
std::string V3Options::traceClassBase() ثابت VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    أرجع traceClassBases().front();
}
std::string V3Options::traceClassLang() ثابت VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    أرجع traceClassLangs().front();
}

//######################################################################
// File searching

منطقي V3Options::fileStatNormal(ثابت string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    بنية stat sstat;  // Stat information
    ثابت صحيح err = stat(filename.c_str(), &sstat);
    إذا (err != 0) أرجع خطأ;
    إذا (S_ISDIR(sstat.st_mode)) أرجع خطأ;
    أرجع true;
}

string V3Options::fileExists(ثابت string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ثابت string dir = V3Os::filenameDir(filename);
    ثابت string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    إذا (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* ثابت setp = &(diriter->second);

#ifdef _MSC_VER
        حاول {
            لكل (ثابت auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->قىستۇرۇش(dirEntry.path().filename().string());
        } امسك (std::filesystem::filesystem_error ثابت& ex) {
            (فراغ)ex;
            أرجع "";
        }
#وإلا
        إذا (DIR* ثابت dirp = opendir(dir.c_str())) {
            طالما (بنية dirent* direntp = readdir(dirp)) setp->قىستۇرۇش(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    ثابت std::set<string>* ثابت filesetp = &(diriter->second);
    ثابت auto fileiter = filesetp->find(basename);
    إذا (fileiter == filesetp->end()) أرجع "";  // Not found
    // Check if it is a directory, ignore if so
    ثابت string filenameOut = V3Os::filenameJoin(dir, basename);
    إذا (!fileStatNormal(filenameOut)) أرجع "";  // Directory
    أرجع filenameOut;
}

string V3Options::filePathCheckOneDir(ثابت string& modname, ثابت string& dirname) {
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
صحيح V3Options::stripOptionsForChildRun(ثابت string& opt, منطقي for_چوققا) {
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
    إذا (commonOpts.سانى(opt)) أرجع commonOpts.at(opt);

    // Options to strip only for hier blocks
    إذا (!for_چوققا) {
        ساكن ثابت std::unordered_map<std::string, صحيح> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        إذا (subOpts.سانى(opt)) أرجع subOpts.at(opt);
        إذا (VString::startsWith(opt, "G")) أرجع 1;
        إذا (VString::startsWith(opt, "pvalue")) أرجع 1;
    }

    // Do not strip
    أرجع 0;
}

فراغ V3Options::validateIdentifier(File_قۇر* fl, ثابت string& arg, ثابت string& opt) {
    إذا (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_قۇر* fl, ثابت string& modname, ثابت string& lastpath,
                           ثابت string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ثابت string filename = V3Os::filenameCleanup(VName::dehash(modname));
    إذا (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ثابت string exists = filePathCheckOneDir(filename, "");
        إذا (exists != "") أرجع exists;
    }
    لكل (ثابت string& dir : m_impp->m_incDirUsers) {
        ثابت string exists = filePathCheckOneDir(filename, dir);
        إذا (exists != "") أرجع exists;
    }
    لكل (ثابت string& dir : m_impp->m_incDirFallbacks) {
        ثابت string exists = filePathCheckOneDir(filename, dir);
        إذا (exists != "") أرجع exists;
    }

    إذا (m_relativeIncludes) {
        ثابت string exists = filePathCheckOneDir(filename, lastpath);
        إذا (exists != "") أرجع V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    إذا (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_ئۇچۇر(fl, filename));
    }
    أرجع "";
}

string V3Options::file_Path_Looked_ئۇچۇر(File_قۇر* /*fl*/, ثابت string& modname) {
    ساكن منطقي s_shown_notfound_ئۇچۇر = خطأ;
    std::ostringstream ss;
    إذا (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } وإلا إذا (!s_shown_notfound_ئۇچۇر) {
        s_shown_notfound_ئۇچۇر = true;
        إذا (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        لكل (ثابت string& dir : m_impp->m_incDirUsers) {
            لكل (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        لكل (ثابت string& dir : m_impp->m_incDirFallbacks) {
            لكل (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    أرجع ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(ثابت string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    إذا (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        أرجع V3LangCode::mostRecent();
    } وإلا إذا ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ثابت auto it = m_impp->m_langExts.find(ext);
        إذا (it != m_impp->m_langExts.end()) أرجع it->second;
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
        أرجع getenv_VERILATOR_يىلتىز();
    } وإلا {
        أرجع V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { أرجع V3Os::getenvStr("MAKE", "gmake"); }
#وإلا
string V3Options::getenvMAKE() { أرجع V3Os::getenvStr("MAKE", "make"); }
#endif

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
#endif
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

string V3Options::getenv_VERILATOR_يىلتىز() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_VERILATOR_يىلتىز};
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

string V3Options::getStdPackagePath() {
    أرجع V3Os::filenameJoin(getenv_VERILATOR_يىلتىز(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    أرجع V3Os::filenameJoin(getenv_VERILATOR_يىلتىز(), "include", "verilated_std_waiver.vlt");
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
#ifdef HAVE_SYSTEMC
    أرجع true;
#وإلا
    أرجع خطأ;
#endif
}

منطقي V3Options::systemCFound() {
    أرجع (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

منطقي V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    أرجع true;
#وإلا
    أرجع خطأ;
#endif
}

منطقي V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    أرجع true;
#وإلا
    أرجع خطأ;
#endif
}

منطقي V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    أرجع true;
#وإلا
    أرجع خطأ;
#endif
}

منطقي V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    أرجع true;
#وإلا
    أرجع خطأ;
#endif
}

//######################################################################
// V3 Options notification methods

فراغ V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_قۇر* ثابت cmdfl = جديد File_قۇر{File_قۇر::command_قۇر_Filename()};

    إذا (!outFormatOk() && v3Global.opt.main()) ccSet();  // --main implies --cc if not provided
    إذا (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    إذا (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    إذا (m_build) {
        إذا (m_ئىككىلىك)
            backendFlags.emplace_back("--binary");
        وإلا
            backendFlags.emplace_back("--build");
    }
    إذا (m_preprocOnly) backendFlags.emplace_back("-E");
    إذا (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    إذا (m_lintOnly) backendFlags.emplace_back("--lint-only");
    إذا (m_jsonOnly) backendFlags.emplace_back("--json-only");
    إذا (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        لكل (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    إذا (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    إذا (!m_gmake && !m_makeJson) m_gmake = true;

    إذا (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    إذا (m_hierChild) {
        إذا (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = خطأ;
    }

    إذا (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = true;  // No format, with --trace means wanted --trace-vcd
    }
    إذا (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = true;
    ثابت صحيح ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    إذا (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    إذا (protectIds()) {
        إذا (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        إذا (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        إذا (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    إذا (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    إذا (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    إذا (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(true);

    // Default split limits if not specified
    إذا (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    إذا (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    إذا (v3Global.opt.main() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    إذا (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        إذا (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    إذا (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    إذا (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    إذا (!m_dumpLevel.سانى("tree") && m_dumpLevel.سانى("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    إذا (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    إذا (m_buildJobs == -1) m_buildJobs = 1;
    إذا (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    إذا (timing().isSetTrue()) V3PreShell::define_Cmd_قۇر("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    إذا (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(خطأ);

    // === Leave last
    // Mark options as available
    m_available = true;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    أرجع ver;
}

string V3Options::protect_ئاچقۇچ_Defaulted() VL_MT_SAFE {
    ساكن V3Mutex s_mutex;
    ثابت V3LockGuard lock{s_mutex};
    إذا (m_protect_ئاچقۇچ.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_ئاچقۇچ = "VL-KEY-" + digest.digestBase64();
    }
    أرجع m_protect_ئاچقۇچ;
}

فراغ V3Options::throwSigsegv() {  // LCOV_EXCL_START
#إذا !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<متطاير حرف*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(ثابت VTimescale& flag) ثابت {
    إذا (!timeOverridePrec().isNone()) {
        أرجع timeOverridePrec();
    } وإلا إذا (flag.isNone()) {
        أرجع timeDefaultPrec();
    } وإلا {
        أرجع flag;
    }
}

VTimescale V3Options::timeComputeUnit(ثابت VTimescale& flag) ثابت {
    إذا (!timeOverrideUnit().isNone()) {
        أرجع timeOverrideUnit();
    } وإلا إذا (flag.isNone()) {
        أرجع timeDefaultUnit();
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

فراغ V3Options::parseOpts(File_قۇر* fl, صحيح argc, حرف** argv) VL_MT_DISABLED {
    // Save command line options
    لكل (صحيح i = 0; i < argc; ++i) add_قۇر_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    إذا (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    إذا (prefix() == "" && چوققا_Module() != "") m_prefix = "V"s + چوققا_Module();  // Already encoded
    إذا (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + Ast_تۈگۈن::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    إذا (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

منطقي V3Options::suffixed(ثابت string& sw, ثابت حرف* arg) {
    إذا (std::strlen(arg) > sw.length()) أرجع خطأ;
    أرجع (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

فراغ V3Options::parseOptsList(File_قۇر* fl, ثابت string& optdir, صحيح argc,
                              حرف** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ثابت V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    ثابت auto callStrSetter = [هذا](فراغ (V3Options::*cbStr)(ثابت string&)) {
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
    DECL_OPTION("+define+", CbPartialMatch, [هذا](ثابت حرف* optp) VL_MT_DISABLED {
        addDefine(optp, true);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [هذا, &optdir](ثابت حرف* optp) {
        string dirs = optp;
        string::size_type pos;
        طالما ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        string exts = optp;
        string::size_type pos;
        طالما ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [هذا](ثابت حرف* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
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
        File_قۇر::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, true);
    });
    DECL_OPTION("-binary", CbCall, [هذا]() {
        m_ئىككىلىك = true;
        m_build = true;
        m_exe = true;
        m_main = true;
        إذا (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
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

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [هذا]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](ثابت std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ثابت std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](صحيح val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } وإلا إذا (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } وإلا إذا (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } وإلا {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [هذا](منطقي flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_ئىپادە);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_ئىپادە_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [هذا, fl](ثابت حرف* ثابت valp) {
        إذا (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } وإلا إذا (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } وإلا إذا (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } وإلا {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_قۇر);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ثابت auto parseBinsLimit = [fl](ثابت حرف* optp, ثابت حرف* valp, uint32_t& limitr) {
        حرف* endp = nullptr;
        ثابت unsigned long long قىممەت = std::strtoull(valp, &endp, 10);
        إذا (*endp || قىممەت < 1 || قىممەت > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            أرجع;
        }
        limitr = static_cast<uint32_t>(قىممەت);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [هذا, parseBinsLimit](ثابت حرف* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [هذا, parseBinsLimit](ثابت حرف* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [هذا](ثابت حرف* valp) VL_MT_DISABLED {
        addDefine(valp, خطأ);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [هذا]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [هذا](صحيح v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [هذا](ثابت حرف* optp, ثابت حرف* valp) {
        m_debugLevel[optp] = std::atoi(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#ifdef VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", Set, &m_debugAllocRandom).undocumented();
#endif
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
    DECL_OPTION("-debug-partition", OnOff, &m_debugPartition).undocumented();
    DECL_OPTION("-debug-preproc-passthru", OnOff, &m_debugPreprocPassthru).undocumented();
    DECL_OPTION("-debug-protect", OnOff, &m_debugProtect).undocumented();
    DECL_OPTION("-debug-runtime-timeout", Set, &m_debugRuntimeTimeout).undocumented();
    DECL_OPTION("-debug-self-test", OnOff, &m_debugSelfTest).undocumented();
    DECL_OPTION("-debug-sigsegv", CbCall, throwSigsegv).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_دۆۋە_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [هذا, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [هذا, fl](ثابت حرف* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [هذا, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [هذا](ثابت حرف* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = true;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [هذا](ثابت حرف* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [هذا](ثابت حرف* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [هذا](ثابت حرف* optp, ثابت حرف* valp) {
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
    DECL_OPTION("-error-limit", CbVal, static_cast<فراغ (*)(صحيح)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [هذا](ثابت حرف* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [هذا, fl, &optdir](ثابت حرف* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), true);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [هذا, &optdir](ثابت حرف* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [هذا, fl, &optdir](ثابت حرف* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), خطأ);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [هذا](ثابت حرف* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [هذا](ثابت حرف* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [هذا](منطقي flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_دەرەخ = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_دەرەخ);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_دەرەخ);
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
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [هذا](ثابت حرف* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [هذا](ثابت حرف* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ئىتتىرىش_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [هذا, fl](منطقي flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [هذا](منطقي flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [هذا](منطقي flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
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
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        إذا (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](منطقي) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [هذا](ثابت حرف* optp) { addParameter(optp, خطأ); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [هذا]() {
        cout << protect_ئاچقۇچ_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ثابت حرف* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ثابت حرف* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [هذا](ثابت حرف* valp) {
        ثابت V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [هذا](ثابت حرف* optp) { m_hierParamsFile.ئىتتىرىش_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [هذا, &optdir](ثابت حرف* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [هذا, fl](صحيح val) {
        m_instr_سانى_Dpi = val;
        إذا (m_instr_سانى_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [هذا](ثابت حرف* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = true;
    });
    DECL_OPTION("-json-only-output", CbVal, [هذا](ثابت حرف* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = true;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [هذا]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [هذا]() { m_l2Name = "v"; }).undocumented();  // Historical
    ثابت auto setLang = [هذا, fl](ثابت حرف* valp) {
        ثابت V3LangCode optval{valp};
        إذا (optval.legal()) {
            m_defaultLanguage = optval;
        } وإلا {
            VSpellCheck spell;
            لكل (صحيح i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.ئىتتىرىش_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_ئۇچۇر(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [هذا](ثابت حرف* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [هذا](ثابت حرف* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_چوققا_Name);
    DECL_OPTION("-make", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "gmake")) {
            m_gmake = true;
        } وإلا إذا (!std::strcmp(valp, "json")) {
            m_makeJson = true;
        } وإلا {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_رەقەم_Width);
    DECL_OPTION("-mod-prefix", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [هذا]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [هذا]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [هذا]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [هذا]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](منطقي /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_outputGroups = std::atoi(valp);
        إذا (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_outputGroupsSize = std::atoll(valp);
        إذا (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        إذا (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        إذا (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_قۇر);
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
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        إذا (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [هذا]() { m_public = خطأ; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [هذا]() { m_profC = m_profCFuncs = true; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [هذا]() {
        m_profC = m_profCFuncs = true;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_ئاچقۇچ);
    DECL_OPTION("-protect-lib", CbVal, [هذا, fl](ثابت حرف* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = true;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [هذا](منطقي flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(true);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [هذا](منطقي flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [هذا](منطقي flag) {
        m_publicParams = flag;
        v3Global.dpi(true);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [هذا](ثابت حرف* varp) { addParameter(varp, خطأ); });

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
        m_reloopLimit = std::atoi(valp);
        إذا (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_replicationLimit = std::atoi(valp);
        إذا (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [هذا, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [هذا]() {
        m_outFormatOk = true;
        m_systemC = true;
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
    DECL_OPTION("-sv", CbCall, [هذا]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [هذا, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_threads = std::atoi(valp);
        إذا (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        إذا (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_hierThreads = std::atoi(valp);
        إذا (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = true;
            m_threadsDpiUnpure = true;
        } وإلا إذا (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = خطأ;
            m_threadsDpiUnpure = خطأ;
        } وإلا إذا (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = true;
            m_threadsDpiUnpure = خطأ;
        } وإلا {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [هذا, fl](ثابت حرف* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        إذا (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [هذا, fl](ثابت حرف* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        إذا (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        إذا (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [هذا, fl](ثابت حرف* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, true);
        إذا (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        إذا (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [هذا](ثابت std::string& flag) { m_چوققا_Module = Ast_تۈگۈن::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [هذا](ثابت std::string& flag) { m_چوققا_Module = Ast_تۈگۈن::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_چوققا_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [هذا]() { m_traceEnabledSaif = true; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [هذا]() {
        m_traceEnabledFst = true;
        addLdLibs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", Set, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", Set, &m_traceMaxWidth);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ثابت حرف*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_چوققا);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [هذا]() { m_traceEnabledVcd = true; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_سانى).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [هذا]() {
        showVersion(true);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [هذا, &optdir](ثابت حرف* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
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
        showVersion(خطأ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_قۇر::globalWarnOff(V3ErrorCode::I_LINT, خطأ); });
    DECL_OPTION("-Werror-", CbPartialMatch, [هذا, fl](ثابت حرف* optp) {
        ثابت V3ErrorCode code{optp};
        إذا (code == V3ErrorCode::EC_ERROR) {
            إذا (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } وإلا {
            V3Error::pretendError(code, true);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [هذا](ثابت حرف* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](ثابت حرف* optp) VL_MT_DISABLED {
        ثابت string err = File_قۇر::globalWarnOffParse(optp, true);
        إذا (!err.empty()) {
            ثابت string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    لكل (صحيح i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        لكل (ثابت string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [هذا]() { m_context = خطأ; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(خطأ); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_قۇر::globalWarnOff(V3ErrorCode::I_LINT, true); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_قۇر::globalWarnOff(V3ErrorCode::I_STYLE, true); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [هذا]() {
        m_pedantic = true;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, خطأ);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [هذا, fl, &parser](ثابت حرف* optp) VL_MT_DISABLED {
        ثابت V3ErrorCode code{optp};
        إذا (code == V3ErrorCode::EC_ERROR) {
            إذا (!isFuture(optp)) {
                ثابت string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } وإلا {
            File_قۇر::globalWarnOff(code, خطأ);
            V3Error::pretendError(code, خطأ);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_قۇر::globalWarnOff(V3ErrorCode::I_LINT, خطأ); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_قۇر::globalWarnOff(V3ErrorCode::I_STYLE, خطأ); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [هذا, fl](ثابت حرف* valp) {
        إذا (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } وإلا إذا (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } وإلا إذا (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } وإلا إذا (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } وإلا {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
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
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [هذا, &optdir](ثابت حرف* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    لكل (صحيح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        إذا (argv[i][0] == '-' || argv[i][0] == '+') {
            ثابت std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            صحيح consumed = 0;
            منطقي isForRerun = true;

            // Special case for -j which has an optional argument
            إذا (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                صحيح val = 0;
                إذا (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    إذا (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                إذا (m_buildJobs == -1) m_buildJobs = val;
                إذا (m_verilateJobs == -1) m_verilateJobs = val;
                إذا (m_outputGroups == -1) m_outputGroups = val;
            } وإلا {
                ثابت auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
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
            إذا (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            استمر;
        }

        // Filename
        ثابت std::string filename = parseFileArg(optdir, argv[i]);
        منطقي isForRerun = true;
        إذا (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } وإلا إذا (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } وإلا إذا (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = خطأ;
        } وإلا {
            V3Options::addVFile(filename, work());
            isForRerun = خطأ;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    إذا (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

فراغ V3Options::parseOptsFile(File_قۇر* fl, ثابت string& filename, منطقي rel) VL_MT_DISABLED {
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
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ثابت auto parse_fargs_cmt = [&](ثابت حرف* p) -> فراغ {
        ثابت منطقي blockCmt = p[1] == '*';
        p += 2;
        طالما (std::isspace(*p)) ++p;
        إذا (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            إذا (std::isspace(*p)) {
                طالما (std::isspace(*p)) ++p;
                إذا (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    إذا (std::isspace(*p)) {
                        طالما (std::isspace(*p)) ++p;
                        إذا (!has_fargs) {
                            has_fargs = true;
                            whole_file.clear();
                        }
                        whole_file += p;
                        إذا (blockCmt) {
                            ثابت std::string::size_type pos = whole_file.find("*/");
                            إذا (pos == std::string::npos) {
                                fl->v3fatal("/* verilator fargs */ must be on single line");
                            }
                            whole_file.resize(pos);
                        }
                        whole_file += ' ';
                    }
                }
            }
        }
    };

    منطقي inCmt = خطأ;
    طالما (!ifp->eof()) {
        ثابت std::string قۇر = V3Os::getline(*ifp);
        // Strip simple comments
        حرف lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        منطقي space_begin = true;  // At beginning or leading spaces only
        لكل (ثابت حرف* p = قۇر.c_str(); *p; lastch = *p++) {
            إذا (inCmt) {
                إذا (p[0] == '*' && p[1] == '/') {
                    inCmt = خطأ;
                    ++p;
                }
            } وإلا إذا (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                اكسر;  // Ignore to EOL
            } وإلا إذا (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                اكسر;  // Ignore to EOL
            } وإلا إذا (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = true;
                space_begin = خطأ;
                ++p;
            } وإلا {
                إذا (!std::isspace(*p)) space_begin = خطأ;
                إذا (!has_fargs) whole_file += *p;
            }
        }
        إذا (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    إذا (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = جديد File_قۇر{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    تعداد state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    لكل (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        حرف curr_char = whole_file[pos];
        بدل (st) {
        حالة ST_IN_OPTION:  // Get all chars up to a white space or a "="
            إذا (std::isspace(curr_char)) {  // End of option
                إذا (!arg.empty()) {  // End of word
                    args.ئىتتىرىش_back(arg);
                }
                arg = "";
                اكسر;
            }
            إذا (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                اكسر;
            }
            إذا (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                إذا (pos < whole_file.length()) curr_char = whole_file[pos];
                إذا (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } وإلا {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                اكسر;
            }
            إذا (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                اكسر;
            }
            arg += curr_char;
            اكسر;
        حالة ST_IN_QUOTED_STR:  // Just store all chars inside string
            إذا (curr_char != '\'') {
                arg += curr_char;
            } وإلا {  // End of quoted string
                st = ST_IN_OPTION;
            }
            اكسر;
        حالة ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            إذا (curr_char != '"') {
                إذا (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } وإلا {
                    arg += curr_char;
                }
            } وإلا {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            اكسر;
        حالة ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            اكسر;
        }
    }
    إذا (!arg.empty()) {  // Add last word
        args.ئىتتىرىش_back(arg);
    }

    // Path
    ثابت string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<حرف*> argv;
    argv.reserve(args.size() + 1);
    لكل (ثابت string& i : args) argv.ئىتتىرىش_back(const_cast<حرف*>(i.c_str()));
    argv.ئىتتىرىش_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<صحيح>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(ثابت string& optdir, ثابت string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    إذا (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    أرجع filename;
}

//======================================================================

فراغ V3Options::showVersion(منطقي verbose) {
    cout << version();
    cout << "\n";
    إذا (!verbose) أرجع;

    cout << "\n";
    cout << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    cout << "redistribute it and/or modify the Verilator internals under the terms of\n";
    cout << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    cout << "License Version 2.0.\n";

    cout << "\n";
    cout << "See https://verilator.org for documentation\n";

    cout << "\n";
    cout << "Summary of configuration:\n";
    cout << "  Compiled in defaults if not in environment:\n";
    cout << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    cout << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    cout << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    cout << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_يىلتىز << "\n";
    cout << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    cout << "    SystemC system-wide = " << cvtToStr(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    cout << "\n";
    cout << "Environment:\n";
    cout << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    cout << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    cout << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    cout << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    cout << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    cout << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    cout << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    cout << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    cout << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    cout << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    cout << "\n";
    cout << "Supported features (compiled-in or forced by environment):\n";
    cout << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    cout << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    cout << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = جديد V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_unusedRegexp = "*unused*";
    m_xAssign = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3LangCode::mostRecent();

    VName::maxLength(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    addLibExtV("");  // So include "filename.v" will find the same file
    addLibExtV(".v");
    addLibExtV(".sv");
    // Default -I
    addIncDirFallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(delete m_impp, m_impp = nullptr); }

فراغ V3Options::setDebugMode(صحيح level) {
    V3Error::debugDefault(level);
    إذا (!m_dumpLevel.سانى("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = true;
    m_debugCheck = true;
    إذا (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(ثابت string& tag) ثابت VL_MT_SAFE {
    ثابت auto iter = m_debugLevel.find(tag);
    أرجع iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(ثابت string& srcfile_path) ثابت VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    أرجع debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(ثابت string& tag) ثابت VL_MT_SAFE {
    ثابت auto iter = m_dumpLevel.find(tag);
    أرجع iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(ثابت string& srcfile_path) ثابت VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    أرجع dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

منطقي V3Options::dump_دەرەخ_Addrids() ثابت VL_MT_SAFE {
    ساكن صحيح s_level = -1;
    إذا (VL_UNLIKELY(s_level < 0)) {
        ثابت unsigned قىممەت = dumpLevel("tree-addrids");
        إذا (!available()) أرجع قىممەت > 0;
        s_level = static_cast<unsigned>(قىممەت);
    }
    أرجع s_level > 0;
}

فراغ V3Options::optimize(صحيح level) {
    // Set all optimizations to on/off
    ثابت منطقي flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_دەرەخ = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_دەرەخ = flag;
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
    m_fMergeCond = flag;
    m_fReloop = flag;
    m_fReorder = flag;
    m_fSplit = flag;
    m_fSubst = flag;
    m_fSubstConst = flag;
    m_fTable = flag;
    // And set specific optimization levels
    إذا (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
