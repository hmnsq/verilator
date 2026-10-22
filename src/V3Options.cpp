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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Options.h"

#ಅನ್ತರ್ಭಾವ "V3Container.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"
#ಅನ್ತರ್ಭಾವ "V3OptionParser.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3PreShell.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

// clang-format off
#ಅನ್ತರ್ಭಾವ <sys/types.h>
#ಅನ್ತರ್ಭಾವ <sys/stat.h>
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#ಅನ್ಯಥಾ
# include <dirent.h>
#endif
#ಅನ್ತರ್ಭಾವ <fcntl.h>
#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <thread>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>

#ಅನ್ತರ್ಭಾವ "config_rev.h"

#ಯದಿ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

ವರ್ಗ V3OptionsImp final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    using DirMap = std::map<ಧ್ರುವ string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_ಸಾಲು_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, ತಾರ್ಕಿಕ>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<ಧ್ರುವ string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    ಶೂನ್ಯ addIncDirUser(ಧ್ರುವ string& incdir) {
        ಧ್ರುವ string& dir = V3Os::filenameCleanup(incdir);
        ಧ್ರುವ ತಾರ್ಕಿಕ inserted = m_incDirUsers.ಸೇರಿಸು(dir);
        ಯದಿ (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    ಶೂನ್ಯ addIncDirFallback(ಧ್ರುವ string& incdir) {
        ಧ್ರುವ string& dir = V3Os::filenameCleanup(incdir);
        ಯದಿ (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ಧ್ರುವ auto itFoundPair = m_incDirFallbackSet.ಸೇರಿಸು(dir);
            ಯದಿ (itFoundPair.second) m_incDirFallbacks.ತಳ್ಳು_back(dir);
        }
    }
    ಶೂನ್ಯ addLangExt(ಧ್ರುವ string& langext, ಧ್ರುವ V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        ಯದಿ (addext[0] == '.') addext = addext.substr(1);
        (ಶೂನ್ಯ)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    ಶೂನ್ಯ addLibExtV(ಧ್ರುವ string& libext) { m_libExtVs.ಸೇರಿಸು(libext); }
    V3OptionsImp() = ಸ್ವಭಾವ;
    ~V3OptionsImp() = ಸ್ವಭಾವ;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(ಧ್ರುವ ವರ್ಣ* textp) {
    // Return code for given string, or ERROR, which is a bad code
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        ಧ್ರುವ V3LangCode code{codei};
        ಯದಿ (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            ಪ್ರತ್ಯಾಗಮ;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ಧ್ರುವ string& ಮೌಲ್ಯ, ತಾರ್ಕಿಕ& badr)
    : m_e{VTimescale::NONE} {
    badr = ಸತ್ಯ;
    ಧ್ರುವ string spaceless = VString::removeWhitespace(ಮೌಲ್ಯ);
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = TS_100S; i < _ENUM_END; ++i) {
        ಧ್ರುವ VTimescale ts{i};
        ಯದಿ (spaceless == ts.ascii()) {
            badr = ಅಸತ್ಯ;
            m_e = ts.m_e;
            ಭಙ್ಗ;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(ಧ್ರುವ string& opts) {
    VStringList vals;
    ತಾರ್ಕಿಕ inStr = ಅಸತ್ಯ;
    string cur;
    ಸ್ಥಿರ ಧ್ರುವ string hierBlock("--hierarchical-block");
    File_ಸಾಲು cmdfl(File_ಸಾಲು::command_ಸಾಲು_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    ಪ್ರತಿ (string::const_iterator it = opts.begin(); it != opts.end();) {
        ಯದಿ (inStr) {
            ಯದಿ (*it == '\\') {
                ++it;
                ಯದಿ (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    ಭಙ್ಗ;
                }
                ಯದಿ (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    ಭಙ್ಗ;
                }
                cur.ತಳ್ಳು_back(*it);
                ++it;
            } ಅನ್ಯಥಾ ಯದಿ (*it == '"') {  // end of string
                cur.ತಳ್ಳು_back(*it);
                vals.ತಳ್ಳು_back(cur);
                cur.clear();
                ++it;
                ಯದಿ (it != opts.end()) {
                    ಯದಿ (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        ಭಙ್ಗ;
                    }
                    ++it;
                    ಯದಿ (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        ಭಙ್ಗ;
                    }
                    inStr = *it == '"';
                    cur.ತಳ್ಳು_back(*it);
                    ++it;
                }
            } ಅನ್ಯಥಾ {
                cur.ತಳ್ಳು_back(*it);
                ++it;
            }
        } ಅನ್ಯಥಾ {
            ಯದಿ (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                ಭಙ್ಗ;
            }
            ಯದಿ (*it == ',') {  // end of this parameter
                vals.ತಳ್ಳು_back(cur);
                cur.clear();
                ++it;
                ಯದಿ (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    ಭಙ್ಗ;
                }
                inStr = *it == '"';
            }
            cur.ತಳ್ಳು_back(*it);
            ++it;
        }
    }
    ಯದಿ (!cur.empty()) vals.ತಳ್ಳು_back(cur);
    ಯದಿ (vals.size() >= 2) {
        ಯದಿ (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } ಅನ್ಯಥಾ {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    ಪ್ರತಿ (size_t i = 2; i + 1 < vals.size(); i += 2) {
        ಧ್ರುವ ತಾರ್ಕಿಕ inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        ಯದಿ (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

ಶೂನ್ಯ VTimescale::parseSlashed(File_ಸಾಲು* fl, ಧ್ರುವ ವರ್ಣ* textp, VTimescale& unitr,
                              VTimescale& precr, ತಾರ್ಕಿಕ allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ಧ್ರುವ ವರ್ಣ* cp = textp;
    ಪ್ರತಿ (; std::isspace(*cp); ++cp) {}
    ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ unitp = cp;
    ಪ್ರತಿ (; *cp && *cp != '/'; ++cp) {}
    ಧ್ರುವ string unitStr(unitp, cp - unitp);
    ಪ್ರತಿ (; std::isspace(*cp); ++cp) {}
    string precStr;
    ಯದಿ (*cp == '/') {
        ++cp;
        ಪ್ರತಿ (; std::isspace(*cp); ++cp) {}
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ precp = cp;
        ಪ್ರತಿ (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    ಪ್ರತಿ (; std::isspace(*cp); ++cp) {}
    ಯದಿ (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        ಪ್ರತ್ಯಾಗಮ;
    }

    ತಾರ್ಕಿಕ unitbad = ಅಸತ್ಯ;
    ಧ್ರುವ VTimescale unit{unitStr, unitbad /*ref*/};
    ಯದಿ (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        ಪ್ರತ್ಯಾಗಮ;
    }
    unitr = unit;

    ಯದಿ (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        ತಾರ್ಕಿಕ precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        ಯದಿ (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            ಪ್ರತ್ಯಾಗಮ;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

ಶೂನ್ಯ V3Options::addIncDirUser(ಧ್ರುವ string& incdir) { m_impp->addIncDirUser(incdir); }
ಶೂನ್ಯ V3Options::addIncDirFallback(ಧ್ರುವ string& incdir) { m_impp->addIncDirFallback(incdir); }
ಶೂನ್ಯ V3Options::addLangExt(ಧ್ರುವ string& langext, ಧ್ರುವ V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
ಶೂನ್ಯ V3Options::addLibExtV(ಧ್ರುವ string& libext) { m_impp->addLibExtV(libext); }
ಶೂನ್ಯ V3Options::addDefine(ಧ್ರುವ string& defline, ತಾರ್ಕಿಕ allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ಎಡ = defline;
    ಯಾವತ್ (ಎಡ != "") {
        string def = ಎಡ;
        string::size_type pos;
        ಯದಿ (allowPlus && ((pos = ಎಡ.find('+')) != string::npos)) {
            ಎಡ = ಎಡ.substr(pos + 1);
            def.erase(pos);
        } ಅನ್ಯಥಾ {
            ಎಡ = "";
        }
        string ಮೌಲ್ಯ;
        ಯದಿ ((pos = def.find('=')) != string::npos) {
            ಮೌಲ್ಯ = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_ಸಾಲು(def, ಮೌಲ್ಯ);
    }
}
ಶೂನ್ಯ V3Options::addParameter(ಧ್ರುವ string& paramline, ತಾರ್ಕಿಕ allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ಎಡ = paramline;
    ಯಾವತ್ (ಎಡ != "") {
        string param = ಎಡ;
        string::size_type pos;
        ಯದಿ (allowPlus && ((pos = ಎಡ.find('+')) != string::npos)) {
            ಎಡ = ಎಡ.substr(pos + 1);
            param.erase(pos);
        } ಅನ್ಯಥಾ {
            ಎಡ = "";
        }
        string ಮೌಲ್ಯ;
        ಯದಿ ((pos = param.find('=')) != string::npos) {
            ಮೌಲ್ಯ = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << ಮೌಲ್ಯ);
        (ಶೂನ್ಯ)m_parameters.erase(param);
        m_parameters[param] = ಮೌಲ್ಯ;
    }
}

ತಾರ್ಕಿಕ V3Options::hasParameter(ಧ್ರುವ string& name) {
    ಪ್ರತ್ಯಾಗಮ m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(ಧ್ರುವ string& name) {
    string ಮೌಲ್ಯ = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ;
}

ಶೂನ್ಯ V3Options::checkParameters() {
    ಯದಿ (!m_parameters.empty()) {
        std::stringstream ಸಂದೇಶ;
        ಸಂದೇಶ << "Parameters from the command line were not found in the design:";
        ಪ್ರತಿ (ಧ್ರುವ auto& i : m_parameters) ಸಂದೇಶ << " " << i.first;
        v3error(ಸಂದೇಶ.str());
    }
}

ಶೂನ್ಯ V3Options::addCppFile(ಧ್ರುವ string& filename) { m_cppFiles.ಸೇರಿಸು(filename); }
ಶೂನ್ಯ V3Options::addCFlags(ಧ್ರುವ string& filename) { m_cFlags.ತಳ್ಳು_back(filename); }
ಶೂನ್ಯ V3Options::addLibMapFile(ಧ್ರುವ string& filename) { m_libmapFiles.ಸೇರಿಸು(filename); }
ಶೂನ್ಯ V3Options::addCompilerIncludes(ಧ್ರುವ string& filename) {
    m_compilerIncludes.ಸೇರಿಸು(filename);
}
ಶೂನ್ಯ V3Options::addLdLibFile(ಧ್ರುವ string& filename) {
    m_ldLibFiles.ತಳ್ಳು_back(filename);
    addLdLibs(filename);
}
ಶೂನ್ಯ V3Options::addLdLibs(ಧ್ರುವ string& filename) { m_ldLibs.ತಳ್ಳು_back(filename); }
ಶೂನ್ಯ V3Options::addMakeFlags(ಧ್ರುವ string& filename) { m_makeFlags.ತಳ್ಳು_back(filename); }
ಶೂನ್ಯ V3Options::addFuture(ಧ್ರುವ string& flag) { m_futures.ಸೇರಿಸು(flag); }
ಶೂನ್ಯ V3Options::addFuture0(ಧ್ರುವ string& flag) { m_future0s.ಸೇರಿಸು(flag); }
ಶೂನ್ಯ V3Options::addFuture1(ಧ್ರುವ string& flag) { m_future1s.ಸೇರಿಸು(flag); }
ತಾರ್ಕಿಕ V3Options::isFuture(ಧ್ರುವ string& flag) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ m_futures.find(flag) != m_futures.end();
}
ತಾರ್ಕಿಕ V3Options::isFuture0(ಧ್ರುವ string& flag) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ m_future0s.find(flag) != m_future0s.end();
}
ತಾರ್ಕಿಕ V3Options::isFuture1(ಧ್ರುವ string& flag) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ m_future1s.find(flag) != m_future1s.end();
}
ತಾರ್ಕಿಕ V3Options::isLibraryFile(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname) ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
ಶೂನ್ಯ V3Options::addLibraryFile(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname) {
    m_libraryFiles.ಸೇರಿಸು({filename, libname});
}
ಶೂನ್ಯ V3Options::addVFile(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ತಳ್ಳು_back({filename, libname});  // Not emplace
}
ಶೂನ್ಯ V3Options::addVltFile(ಧ್ರುವ string& filename, ಧ್ರುವ string& libname) {
    m_vltFiles.ಸೇರಿಸು({filename, libname});
}
ಶೂನ್ಯ V3Options::addForceInc(ಧ್ರುವ string& filename) { m_forceIncs.ತಳ್ಳು_back(filename); }

ಶೂನ್ಯ V3Options::add_ಸಾಲು_Arg(ಧ್ರುವ string& arg) { m_impp->m_ಸಾಲು_Args.ತಳ್ಳು_back(arg); }

ಶೂನ್ಯ V3Options::addArg(ವರ್ಣ** argv, size_t ಎಣಿಕೆ, ತಾರ್ಕಿಕ isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, ತಾರ್ಕಿಕ>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    ಪ್ರತಿ (size_t n = 0; n < ಎಣಿಕೆ; ++n) pair.first.emplace_back(argv[n]);
}

ಶೂನ್ಯ V3Options::addArg(ಧ್ರುವ std::string& arg, ತಾರ್ಕಿಕ isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, ತಾರ್ಕಿಕ>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

ಧ್ರುವ std::list<std::pair<std::list<std::string>, ತಾರ್ಕಿಕ>>& V3Options::allArgs() ಧ್ರುವ {
    ಪ್ರತ್ಯಾಗಮ m_impp->m_allArgs;
}

string V3Options::allArgsString() ಧ್ರುವ VL_MT_SAFE {
    string result;
    ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_impp->m_allArgs) {
        ಪ್ರತಿ (ಧ್ರುವ string& arg : pair.first) {
            ಯದಿ (!result.empty()) result += " ";
            result += arg;
        }
    }
    ಪ್ರತ್ಯಾಗಮ result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(ತಾರ್ಕಿಕ for_ತುದಿ) ಧ್ರುವ {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    ಪ್ರತಿ (ಧ್ರುವ auto& vFile : m_vFiles) vFiles.ಸೇರಿಸು(vFile.filename());
    string out;
    ತಾರ್ಕಿಕ stripArg = ಅಸತ್ಯ;
    ತಾರ್ಕಿಕ strip_Arg_If_ಅಂಕೆ = ಅಸತ್ಯ;
    ಪ್ರತಿ (ಧ್ರುವ auto& pair : m_impp->m_allArgs) {
        ಪ್ರತಿ (ಧ್ರುವ string& arg : pair.first) {
            ಯದಿ (stripArg) {
                stripArg = ಅಸತ್ಯ;
                ಅನುವರ್ತ;
            }
            ಯದಿ (strip_Arg_If_ಅಂಕೆ) {
                strip_Arg_If_ಅಂಕೆ = ಅಸತ್ಯ;
                ಯದಿ (isdigit(arg[0])) ಅನುವರ್ತ;
            }
            ಪೂರ್ಣಾಙ್ಕ skip = 0;
            ಯದಿ (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } ಅನ್ಯಥಾ ಯದಿ (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            ಯದಿ (skip > 0) {  // arg is an option
                ಧ್ರುವ string opt = arg.substr(skip);  // Remove '-' in the beginning
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_Strip = stripOptionsForChildRun(opt, for_ತುದಿ);
                ಯದಿ (ಅಂಕೆ_Strip) {
                    UASSERT(0 <= ಅಂಕೆ_Strip && ಅಂಕೆ_Strip <= 3, "should be one of 0, 1, 2, 3");
                    ಯದಿ (ಅಂಕೆ_Strip == 2) stripArg = ಸತ್ಯ;
                    ಯದಿ (ಅಂಕೆ_Strip == 3) strip_Arg_If_ಅಂಕೆ = ಸತ್ಯ;
                    ಅನುವರ್ತ;
                }
            } ಅನ್ಯಥಾ {  // Not an option
                ಯದಿ (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    ಅನುವರ್ತ;
                }
            }
            ಯದಿ (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    ಪ್ರತ್ಯಾಗಮ out;
}

ಶೂನ್ಯ V3Options::ccSet() {  // --cc
    m_outFormatOk = ಸತ್ಯ;
    m_systemC = ಅಸತ್ಯ;
}

ಶೂನ್ಯ V3Options::decorations(File_ಸಾಲು* fl, ಧ್ರುವ string& arg) {  // --decorations
    ಯದಿ (arg == "none") {
        m_decoration = ಅಸತ್ಯ;
        m_decorationNodes = ಅಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (arg == "node") {
        m_decoration = ಸತ್ಯ;
        m_decorationNodes = ಸತ್ಯ;
    } ಅನ್ಯಥಾ ಯದಿ (arg == "medium") {
        m_decoration = ಸತ್ಯ;
        m_decorationNodes = ಅಸತ್ಯ;
    } ಅನ್ಯಥಾ {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() ಧ್ರುವ VL_MT_SAFE {
    std::vector<std::string> result;
    ಯದಿ (traceEnabledFst()) result.emplace_back("VerilatedFst");
    ಯದಿ (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    ಯದಿ (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    ಪ್ರತ್ಯಾಗಮ result;
}
std::vector<std::string> V3Options::traceClassLangs() ಧ್ರುವ VL_MT_SAFE {
    std::vector<std::string> result;
    ಪ್ರತಿ (ಧ್ರುವ auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    ಪ್ರತ್ಯಾಗಮ result;
}
std::vector<std::string> V3Options::traceSourceBases() ಧ್ರುವ VL_MT_SAFE {
    std::vector<std::string> result;
    ಯದಿ (traceEnabledFst()) result.emplace_back("verilated_fst");
    ಯದಿ (traceEnabledSaif()) result.emplace_back("verilated_saif");
    ಯದಿ (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    ಪ್ರತ್ಯಾಗಮ result;
}
std::vector<std::string> V3Options::traceSourceLangs() ಧ್ರುವ VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    ಪ್ರತಿ (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    ಪ್ರತ್ಯಾಗಮ result;
}
std::string V3Options::traceClassBase() ಧ್ರುವ VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    ಪ್ರತ್ಯಾಗಮ traceClassBases().front();
}
std::string V3Options::traceClassLang() ಧ್ರುವ VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    ಪ್ರತ್ಯಾಗಮ traceClassLangs().front();
}

//######################################################################
// File searching

ತಾರ್ಕಿಕ V3Options::fileStatNormal(ಧ್ರುವ string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ಸಂರಚನಾ stat sstat;  // Stat information
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ err = stat(filename.c_str(), &sstat);
    ಯದಿ (err != 0) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಯದಿ (S_ISDIR(sstat.st_mode)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
}

string V3Options::fileExists(ಧ್ರುವ string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ಧ್ರುವ string dir = V3Os::filenameDir(filename);
    ಧ್ರುವ string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    ಯದಿ (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* ಧ್ರುವ setp = &(diriter->second);

#ifdef _MSC_VER
        ಯತ್ನ {
            ಪ್ರತಿ (ಧ್ರುವ auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->ಸೇರಿಸು(dirEntry.path().filename().string());
        } ಗೃಹಾಣ (std::filesystem::filesystem_error ಧ್ರುವ& ex) {
            (ಶೂನ್ಯ)ex;
            ಪ್ರತ್ಯಾಗಮ "";
        }
#ಅನ್ಯಥಾ
        ಯದಿ (DIR* ಧ್ರುವ dirp = opendir(dir.c_str())) {
            ಯಾವತ್ (ಸಂರಚನಾ dirent* direntp = readdir(dirp)) setp->ಸೇರಿಸು(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    ಧ್ರುವ std::set<string>* ಧ್ರುವ filesetp = &(diriter->second);
    ಧ್ರುವ auto fileiter = filesetp->find(basename);
    ಯದಿ (fileiter == filesetp->end()) ಪ್ರತ್ಯಾಗಮ "";  // Not found
    // Check if it is a directory, ignore if so
    ಧ್ರುವ string filenameOut = V3Os::filenameJoin(dir, basename);
    ಯದಿ (!fileStatNormal(filenameOut)) ಪ್ರತ್ಯಾಗಮ "";  // Directory
    ಪ್ರತ್ಯಾಗಮ filenameOut;
}

string V3Options::filePathCheckOneDir(ಧ್ರುವ string& modname, ಧ್ರುವ string& dirname) {
    ಪ್ರತಿ (ಧ್ರುವ string& i : m_impp->m_libExtVs) {
        ಧ್ರುವ string fn = V3Os::filenameJoin(dirname, modname + i);
        ಧ್ರುವ string exists = fileExists(fn);
        ಯದಿ (exists != "") ಪ್ರತ್ಯಾಗಮ exists;
    }
    ಪ್ರತ್ಯಾಗಮ "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
ಪೂರ್ಣಾಙ್ಕ V3Options::stripOptionsForChildRun(ಧ್ರುವ string& opt, ತಾರ್ಕಿಕ for_ತುದಿ) {
    // Options to strip for both the top wrapper, and hier blocks
    ಸ್ಥಿರ ಧ್ರುವ std::unordered_map<std::string, ಪೂರ್ಣಾಙ್ಕ> commonOpts{
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
    ಯದಿ (commonOpts.ಎಣಿಕೆ(opt)) ಪ್ರತ್ಯಾಗಮ commonOpts.at(opt);

    // Options to strip only for hier blocks
    ಯದಿ (!for_ತುದಿ) {
        ಸ್ಥಿರ ಧ್ರುವ std::unordered_map<std::string, ಪೂರ್ಣಾಙ್ಕ> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        ಯದಿ (subOpts.ಎಣಿಕೆ(opt)) ಪ್ರತ್ಯಾಗಮ subOpts.at(opt);
        ಯದಿ (VString::startsWith(opt, "G")) ಪ್ರತ್ಯಾಗಮ 1;
        ಯದಿ (VString::startsWith(opt, "pvalue")) ಪ್ರತ್ಯಾಗಮ 1;
    }

    // Do not strip
    ಪ್ರತ್ಯಾಗಮ 0;
}

ಶೂನ್ಯ V3Options::validateIdentifier(File_ಸಾಲು* fl, ಧ್ರುವ string& arg, ಧ್ರುವ string& opt) {
    ಯದಿ (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_ಸಾಲು* fl, ಧ್ರುವ string& modname, ಧ್ರುವ string& lastpath,
                           ಧ್ರುವ string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ಧ್ರುವ string filename = V3Os::filenameCleanup(VName::dehash(modname));
    ಯದಿ (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ಧ್ರುವ string exists = filePathCheckOneDir(filename, "");
        ಯದಿ (exists != "") ಪ್ರತ್ಯಾಗಮ exists;
    }
    ಪ್ರತಿ (ಧ್ರುವ string& dir : m_impp->m_incDirUsers) {
        ಧ್ರುವ string exists = filePathCheckOneDir(filename, dir);
        ಯದಿ (exists != "") ಪ್ರತ್ಯಾಗಮ exists;
    }
    ಪ್ರತಿ (ಧ್ರುವ string& dir : m_impp->m_incDirFallbacks) {
        ಧ್ರುವ string exists = filePathCheckOneDir(filename, dir);
        ಯದಿ (exists != "") ಪ್ರತ್ಯಾಗಮ exists;
    }

    ಯದಿ (m_relativeIncludes) {
        ಧ್ರುವ string exists = filePathCheckOneDir(filename, lastpath);
        ಯದಿ (exists != "") ಪ್ರತ್ಯಾಗಮ V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    ಯದಿ (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_ಸಂದೇಶ(fl, filename));
    }
    ಪ್ರತ್ಯಾಗಮ "";
}

string V3Options::file_Path_Looked_ಸಂದೇಶ(File_ಸಾಲು* /*fl*/, ಧ್ರುವ string& modname) {
    ಸ್ಥಿರ ತಾರ್ಕಿಕ s_shown_notfound_ಸಂದೇಶ = ಅಸತ್ಯ;
    std::ostringstream ss;
    ಯದಿ (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } ಅನ್ಯಥಾ ಯದಿ (!s_shown_notfound_ಸಂದೇಶ) {
        s_shown_notfound_ಸಂದೇಶ = ಸತ್ಯ;
        ಯದಿ (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        ಪ್ರತಿ (ಧ್ರುವ string& dir : m_impp->m_incDirUsers) {
            ಪ್ರತಿ (ಧ್ರುವ string& ext : m_impp->m_libExtVs) {
                ಧ್ರುವ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ಪ್ರತಿ (ಧ್ರುವ string& dir : m_impp->m_incDirFallbacks) {
            ಪ್ರತಿ (ಧ್ರುವ string& ext : m_impp->m_libExtVs) {
                ಧ್ರುವ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    ಪ್ರತ್ಯಾಗಮ ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(ಧ್ರುವ string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    ಯದಿ (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        ಪ್ರತ್ಯಾಗಮ V3LangCode::mostRecent();
    } ಅನ್ಯಥಾ ಯದಿ ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ಧ್ರುವ auto it = m_impp->m_langExts.find(ext);
        ಯದಿ (it != m_impp->m_langExts.end()) ಪ್ರತ್ಯಾಗಮ it->second;
    }
    ಪ್ರತ್ಯಾಗಮ m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ಧ್ರುವ string& var) {
    // If update below, also update V3Options::showVersion()
    ಯದಿ (var == "MAKE") {
        ಪ್ರತ್ಯಾಗಮ getenvMAKE();
    } ಅನ್ಯಥಾ ಯದಿ (var == "PERL") {
        ಪ್ರತ್ಯಾಗಮ getenvPERL();
    } ಅನ್ಯಥಾ ಯದಿ (var == "PYTHON3") {
        ಪ್ರತ್ಯಾಗಮ getenvPYTHON3();
    } ಅನ್ಯಥಾ ಯದಿ (var == "SYSTEMC") {
        ಪ್ರತ್ಯಾಗಮ getenvSYSTEMC();
    } ಅನ್ಯಥಾ ಯದಿ (var == "SYSTEMC_ARCH") {
        ಪ್ರತ್ಯಾಗಮ getenvSYSTEMC_ARCH();
    } ಅನ್ಯಥಾ ಯದಿ (var == "SYSTEMC_INCLUDE") {
        ಪ್ರತ್ಯಾಗಮ getenvSYSTEMC_INCLUDE();
    } ಅನ್ಯಥಾ ಯದಿ (var == "SYSTEMC_LIBDIR") {
        ಪ್ರತ್ಯಾಗಮ getenvSYSTEMC_LIBDIR();
    } ಅನ್ಯಥಾ ಯದಿ (var == "VERILATOR_ROOT") {
        ಪ್ರತ್ಯಾಗಮ getenv_VERILATOR_ಬೇರು();
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { ಪ್ರತ್ಯಾಗಮ V3Os::getenvStr("MAKE", "gmake"); }
#ಅನ್ಯಥಾ
string V3Options::getenvMAKE() { ಪ್ರತ್ಯಾಗಮ V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    ಪ್ರತ್ಯಾಗಮ V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    ಪ್ರತ್ಯಾಗಮ V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    ಪ್ರತ್ಯಾಗಮ V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಧ್ರುವ string defenv = string{DEFENV_SYSTEMC};
    ಯದಿ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    ಪ್ರತ್ಯಾಗಮ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಧ್ರುವ string defenv = string{DEFENV_SYSTEMC_ARCH};
    ಯದಿ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    ಯದಿ (var == "") {
#ಯದಿ defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ಧ್ರುವ string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#ನೋಚೇದ್ಯದಿ defined(_WIN32)
        ಧ್ರುವ string sysname = "WIN32";
        var = "win32";
#ಅನ್ಯಥಾ
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ಸಂರಚನಾ utsname uts;
        uname(&uts);
        ಧ್ರುವ string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        ಯದಿ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } ಅನ್ಯಥಾ ಯದಿ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } ಅನ್ಯಥಾ {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    ಪ್ರತ್ಯಾಗಮ var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಧ್ರುವ string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    ಯದಿ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    ಯದಿ (var == "") {
        ಧ್ರುವ string sc = getenvSYSTEMC();
        ಯದಿ (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    ಪ್ರತ್ಯಾಗಮ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಧ್ರುವ string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    ಯದಿ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    ಯದಿ (var == "") {
        ಧ್ರುವ string sc = getenvSYSTEMC();
        ಧ್ರುವ string arch = getenvSYSTEMC_ARCH();
        ಯದಿ (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    ಪ್ರತ್ಯಾಗಮ V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_ಬೇರು() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಧ್ರುವ string defenv = string{DEFENV_VERILATOR_ಬೇರು};
    ಯದಿ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    ಯದಿ (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    ಪ್ರತ್ಯಾಗಮ V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಧ್ರುವ string defenv = string{DEFENV_VERILATOR_SOLVER};
    ಯದಿ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    ಪ್ರತ್ಯಾಗಮ var;
}

string V3Options::getStdPackagePath() {
    ಪ್ರತ್ಯಾಗಮ V3Os::filenameJoin(getenv_VERILATOR_ಬೇರು(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    ಪ್ರತ್ಯಾಗಮ V3Os::filenameJoin(getenv_VERILATOR_ಬೇರು(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ಧ್ರುವ string& var) {
    // If update below, also update V3Options::showVersion()
    ಯದಿ (var == "COROUTINES" && coroutineSupport()) {
        ಪ್ರತ್ಯಾಗಮ "1";
    } ಅನ್ಯಥಾ ಯದಿ (var == "TSAN" && tsanSupport()) {
        ಪ್ರತ್ಯಾಗಮ "1";
    } ಅನ್ಯಥಾ ಯದಿ (var == "DEV_ASAN" && devAsan()) {
        ಪ್ರತ್ಯಾಗಮ "1";
    } ಅನ್ಯಥಾ ಯದಿ (var == "DEV_GCOV" && devGcov()) {
        ಪ್ರತ್ಯಾಗಮ "1";
        // cppcheck-suppress knownConditionTrueFalse
    } ಅನ್ಯಥಾ ಯದಿ (var == "SYSTEMC" && systemCFound()) {
        ಪ್ರತ್ಯಾಗಮ "1";
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ "";
    }
}

ತಾರ್ಕಿಕ V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
#endif
}

ತಾರ್ಕಿಕ V3Options::systemCFound() {
    ಪ್ರತ್ಯಾಗಮ (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

ತಾರ್ಕಿಕ V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
#endif
}

ತಾರ್ಕಿಕ V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
#endif
}

ತಾರ್ಕಿಕ V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
#endif
}

ತಾರ್ಕಿಕ V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
#ಅನ್ಯಥಾ
    ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
#endif
}

//######################################################################
// V3 Options notification methods

ಶೂನ್ಯ V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_ಸಾಲು* ಧ್ರುವ cmdfl = ನೂತನ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()};

    ಯದಿ (!outFormatOk() && v3Global.opt.main()) ccSet();  // --main implies --cc if not provided
    ಯದಿ (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    ಯದಿ (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    ಯದಿ (m_build) {
        ಯದಿ (m_ದ್ವಿಮಾನ)
            backendFlags.emplace_back("--binary");
        ಅನ್ಯಥಾ
            backendFlags.emplace_back("--build");
    }
    ಯದಿ (m_preprocOnly) backendFlags.emplace_back("-E");
    ಯದಿ (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    ಯದಿ (m_lintOnly) backendFlags.emplace_back("--lint-only");
    ಯದಿ (m_jsonOnly) backendFlags.emplace_back("--json-only");
    ಯದಿ (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        ಪ್ರತಿ (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    ಯದಿ (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    ಯದಿ (!m_gmake && !m_makeJson) m_gmake = ಸತ್ಯ;

    ಯದಿ (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    ಯದಿ (m_hierChild) {
        ಯದಿ (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = ಅಸತ್ಯ;
    }

    ಯದಿ (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = ಸತ್ಯ;  // No format, with --trace means wanted --trace-vcd
    }
    ಯದಿ (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = ಸತ್ಯ;
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    ಯದಿ (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    ಯದಿ (protectIds()) {
        ಯದಿ (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        ಯದಿ (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        ಯದಿ (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    ಯದಿ (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    ಯದಿ (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    ಯದಿ (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(ಸತ್ಯ);

    // Default split limits if not specified
    ಯದಿ (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    ಯದಿ (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    ಯದಿ (v3Global.opt.main() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    ಯದಿ (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        ಯದಿ (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    ಯದಿ (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    ಯದಿ (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    ಯದಿ (!m_dumpLevel.ಎಣಿಕೆ("tree") && m_dumpLevel.ಎಣಿಕೆ("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    ಯದಿ (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    ಯದಿ (m_buildJobs == -1) m_buildJobs = 1;
    ಯದಿ (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    ಯದಿ (timing().isSetTrue()) V3PreShell::define_Cmd_ಸಾಲು("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    ಯದಿ (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(ಅಸತ್ಯ);

    // === Leave last
    // Mark options as available
    m_available = ಸತ್ಯ;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    ಪ್ರತ್ಯಾಗಮ ver;
}

string V3Options::protect_ಕೀಲಿ_Defaulted() VL_MT_SAFE {
    ಸ್ಥಿರ V3Mutex s_mutex;
    ಧ್ರುವ V3LockGuard lock{s_mutex};
    ಯದಿ (m_protect_ಕೀಲಿ.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_ಕೀಲಿ = "VL-KEY-" + digest.digestBase64();
    }
    ಪ್ರತ್ಯಾಗಮ m_protect_ಕೀಲಿ;
}

ಶೂನ್ಯ V3Options::throwSigsegv() {  // LCOV_EXCL_START
#ಯದಿ !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<ಚಞ್ಚಲ ವರ್ಣ*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(ಧ್ರುವ VTimescale& flag) ಧ್ರುವ {
    ಯದಿ (!timeOverridePrec().isNone()) {
        ಪ್ರತ್ಯಾಗಮ timeOverridePrec();
    } ಅನ್ಯಥಾ ಯದಿ (flag.isNone()) {
        ಪ್ರತ್ಯಾಗಮ timeDefaultPrec();
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ flag;
    }
}

VTimescale V3Options::timeComputeUnit(ಧ್ರುವ VTimescale& flag) ಧ್ರುವ {
    ಯದಿ (!timeOverrideUnit().isNone()) {
        ಪ್ರತ್ಯಾಗಮ timeOverrideUnit();
    } ಅನ್ಯಥಾ ಯದಿ (flag.isNone()) {
        ಪ್ರತ್ಯಾಗಮ timeDefaultUnit();
    } ಅನ್ಯಥಾ {
        ಪ್ರತ್ಯಾಗಮ flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ** argv) {
    // Return list of arguments as simple string
    string opts;
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < argc; ++i) {
        ಯದಿ (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    ಪ್ರತ್ಯಾಗಮ opts;
}

//######################################################################
// V3 Options Parsing

ಶೂನ್ಯ V3Options::parseOpts(File_ಸಾಲು* fl, ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ** argv) VL_MT_DISABLED {
    // Save command line options
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < argc; ++i) add_ಸಾಲು_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    ಯದಿ (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    ಯದಿ (prefix() == "" && ತುದಿ_Module() != "") m_prefix = "V"s + ತುದಿ_Module();  // Already encoded
    ಯದಿ (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + Ast_ಗಂಟು::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    ಯದಿ (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

ತಾರ್ಕಿಕ V3Options::suffixed(ಧ್ರುವ string& sw, ಧ್ರುವ ವರ್ಣ* arg) {
    ಯದಿ (std::strlen(arg) > sw.length()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

ಶೂನ್ಯ V3Options::parseOptsList(File_ಸಾಲು* fl, ಧ್ರುವ string& optdir, ಪೂರ್ಣಾಙ್ಕ argc,
                              ವರ್ಣ** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ಧ್ರುವ V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    ಧ್ರುವ auto callStrSetter = [ಏತತ್](ಶೂನ್ಯ (V3Options::*cbStr)(ಧ್ರುವ string&)) {
        ಪ್ರತ್ಯಾಗಮ [ಏತತ್, cbStr](ಧ್ರುವ string& v) { (ಏತತ್->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) VL_MT_DISABLED {
        addDefine(optp, ಸತ್ಯ);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [ಏತತ್, &optdir](ಧ್ರುವ ವರ್ಣ* optp) {
        string dirs = optp;
        string::size_type pos;
        ಯಾವತ್ ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        string exts = optp;
        string::size_type pos;
        ಯಾವತ್ ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](ತಾರ್ಕಿಕ) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ಧ್ರುವ ವರ್ಣ*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_bboxUnsup = flag;
        File_ಸಾಲು::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, ಸತ್ಯ);
    });
    DECL_OPTION("-binary", CbCall, [ಏತತ್]() {
        m_ದ್ವಿಮಾನ = ಸತ್ಯ;
        m_build = ಸತ್ಯ;
        m_exe = ಸತ್ಯ;
        m_main = ಸತ್ಯ;
        ಯದಿ (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        ಪೂರ್ಣಾಙ್ಕ val = std::atoi(valp);
        ಯದಿ (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } ಅನ್ಯಥಾ ಯದಿ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [ಏತತ್]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](ಧ್ರುವ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ಧ್ರುವ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](ಪೂರ್ಣಾಙ್ಕ val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        ಯದಿ (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } ಅನ್ಯಥಾ {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_ಅಭಿವ್ಯಕ್ತಿ);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_ಅಭಿವ್ಯಕ್ತಿ_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ valp) {
        ಯದಿ (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } ಅನ್ಯಥಾ {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_ಸಾಲು);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ಧ್ರುವ auto parseBinsLimit = [fl](ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* valp, uint32_t& limitr) {
        ವರ್ಣ* endp = nullptr;
        ಧ್ರುವ unsigned long long ಮೌಲ್ಯ = std::strtoull(valp, &endp, 10);
        ಯದಿ (*endp || ಮೌಲ್ಯ < 1 || ಮೌಲ್ಯ > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            ಪ್ರತ್ಯಾಗಮ;
        }
        limitr = static_cast<uint32_t>(ಮೌಲ್ಯ);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [ಏತತ್, parseBinsLimit](ಧ್ರುವ ವರ್ಣ* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [ಏತತ್, parseBinsLimit](ಧ್ರುವ ವರ್ಣ* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) VL_MT_DISABLED {
        addDefine(valp, ಅಸತ್ಯ);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [ಏತತ್]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [ಏತತ್](ಪೂರ್ಣಾಙ್ಕ v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* valp) {
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_ರಾಶಿ_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [ಏತತ್, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [ಏತತ್, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = ಸತ್ಯ;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        ಯದಿ (flag) {
            m_stdPackage = ಅಸತ್ಯ;
            m_stdWaiver = ಅಸತ್ಯ;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<ಶೂನ್ಯ (*)(ಪೂರ್ಣಾಙ್ಕ)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [ಏತತ್, fl, &optdir](ಧ್ರುವ ವರ್ಣ* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ಸತ್ಯ);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [ಏತತ್, &optdir](ಧ್ರುವ ವರ್ಣ* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [ಏತತ್, fl, &optdir](ಧ್ರುವ ವರ್ಣ* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ಅಸತ್ಯ);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_ವೃಕ್ಷ = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_ವೃಕ್ಷ);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_ವೃಕ್ಷ);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ತಳ್ಳು_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [ಏತತ್, fl](ತಾರ್ಕಿಕ flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        ಯದಿ (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) { addParameter(optp, ಅಸತ್ಯ); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [ಏತತ್]() {
        cout << protect_ಕೀಲಿ_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ಧ್ರುವ ವರ್ಣ* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ಧ್ರುವ ವರ್ಣ* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) {
        ಧ್ರುವ V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) { m_hierParamsFile.ತಳ್ಳು_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [ಏತತ್, &optdir](ಧ್ರುವ ವರ್ಣ* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [ಏತತ್, fl](ಪೂರ್ಣಾಙ್ಕ val) {
        m_instr_ಎಣಿಕೆ_Dpi = val;
        ಯದಿ (m_instr_ಎಣಿಕೆ_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = ಸತ್ಯ;
    });
    DECL_OPTION("-json-only-output", CbVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = ಸತ್ಯ;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [ಏತತ್]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [ಏತತ್]() { m_l2Name = "v"; }).undocumented();  // Historical
    ಧ್ರುವ auto setLang = [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        ಧ್ರುವ V3LangCode optval{valp};
        ಯದಿ (optval.legal()) {
            m_defaultLanguage = optval;
        } ಅನ್ಯಥಾ {
            VSpellCheck spell;
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.ತಳ್ಳು_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_ಸಂದೇಶ(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_ತುದಿ_Name);
    DECL_OPTION("-make", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        ಯದಿ (!std::strcmp(valp, "gmake")) {
            m_gmake = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "json")) {
            m_makeJson = ಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_ಅಂಕೆ_Width);
    DECL_OPTION("-mod-prefix", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [ಏತತ್]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [ಏತತ್]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [ಏತತ್]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [ಏತತ್]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](ತಾರ್ಕಿಕ /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_outputGroups = std::atoi(valp);
        ಯದಿ (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_outputGroupsSize = std::atoll(valp);
        ಯದಿ (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        ಯದಿ (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        ಯದಿ (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_ಸಾಲು);
    DECL_OPTION("-pins64", CbCall, [ಏತತ್]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [ಏತತ್]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_pinsBv = std::atoi(valp);
        ಯದಿ (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_pinsScUint = flag;
        ಯದಿ (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        ಯದಿ (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [ಏತತ್]() { m_public = ಅಸತ್ಯ; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [ಏತತ್]() { m_profC = m_profCFuncs = ಸತ್ಯ; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [ಏತತ್]() {
        m_profC = m_profCFuncs = ಸತ್ಯ;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_ಕೀಲಿ);
    DECL_OPTION("-protect-lib", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = ಸತ್ಯ;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(ಸತ್ಯ);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_publicParams = flag;
        v3Global.dpi(ಸತ್ಯ);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [ಏತತ್](ಧ್ರುವ ವರ್ಣ* varp) { addParameter(varp, ಅಸತ್ಯ); });

    DECL_OPTION("-quiet", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_reloopLimit = std::atoi(valp);
        ಯದಿ (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_replicationLimit = std::atoi(valp);
        ಯದಿ (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [ಏತತ್, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [ಏತತ್]() {
        m_outFormatOk = ಸತ್ಯ;
        m_systemC = ಸತ್ಯ;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [ಏತತ್](ತಾರ್ಕಿಕ flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [ಏತತ್]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [ಏತತ್, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_threads = std::atoi(valp);
        ಯದಿ (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        ಯದಿ (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_hierThreads = std::atoi(valp);
        ಯದಿ (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        ಯದಿ (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = ಸತ್ಯ;
            m_threadsDpiUnpure = ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = ಅಸತ್ಯ;
            m_threadsDpiUnpure = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = ಸತ್ಯ;
            m_threadsDpiUnpure = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        ಯದಿ (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        ಯದಿ (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        ಯದಿ (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, ಸತ್ಯ);
        ಯದಿ (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        ಯದಿ (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [ಏತತ್](ಧ್ರುವ std::string& flag) { m_ತುದಿ_Module = Ast_ಗಂಟು::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [ಏತತ್](ಧ್ರುವ std::string& flag) { m_ತುದಿ_Module = Ast_ಗಂಟು::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_ತುದಿ_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [ಏತತ್]() { m_traceEnabledSaif = ಸತ್ಯ; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [ಏತತ್]() {
        m_traceEnabledFst = ಸತ್ಯ;
        addLdLibs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", Set, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", Set, &m_traceMaxWidth);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ಧ್ರುವ ವರ್ಣ*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_ತುದಿ);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [ಏತತ್]() { m_traceEnabledVcd = ಸತ್ಯ; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_ಎಣಿಕೆ).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [ಏತತ್]() {
        showVersion(ಸತ್ಯ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [ಏತತ್, &optdir](ಧ್ರುವ ವರ್ಣ* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        ಪೂರ್ಣಾಙ್ಕ val = std::atoi(valp);
        ಯದಿ (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } ಅನ್ಯಥಾ ಯದಿ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [ಏತತ್]() {
        showVersion(ಅಸತ್ಯ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_ಸಾಲು::globalWarnOff(V3ErrorCode::I_LINT, ಅಸತ್ಯ); });
    DECL_OPTION("-Werror-", CbPartialMatch, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* optp) {
        ಧ್ರುವ V3ErrorCode code{optp};
        ಯದಿ (code == V3ErrorCode::EC_ERROR) {
            ಯದಿ (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } ಅನ್ಯಥಾ {
            V3Error::pretendError(code, ಸತ್ಯ);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [ಏತತ್](ಧ್ರುವ ವರ್ಣ* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](ಧ್ರುವ ವರ್ಣ* optp) VL_MT_DISABLED {
        ಧ್ರುವ string err = File_ಸಾಲು::globalWarnOffParse(optp, ಸತ್ಯ);
        ಯದಿ (!err.empty()) {
            ಧ್ರುವ string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        ಪ್ರತಿ (ಧ್ರುವ string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [ಏತತ್]() { m_context = ಅಸತ್ಯ; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(ಅಸತ್ಯ); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_ಸಾಲು::globalWarnOff(V3ErrorCode::I_LINT, ಸತ್ಯ); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_ಸಾಲು::globalWarnOff(V3ErrorCode::I_STYLE, ಸತ್ಯ); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [ಏತತ್]() {
        m_pedantic = ಸತ್ಯ;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, ಅಸತ್ಯ);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [ಏತತ್, fl, &parser](ಧ್ರುವ ವರ್ಣ* optp) VL_MT_DISABLED {
        ಧ್ರುವ V3ErrorCode code{optp};
        ಯದಿ (code == V3ErrorCode::EC_ERROR) {
            ಯದಿ (!isFuture(optp)) {
                ಧ್ರುವ string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } ಅನ್ಯಥಾ {
            File_ಸಾಲು::globalWarnOff(code, ಅಸತ್ಯ);
            V3Error::pretendError(code, ಅಸತ್ಯ);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_ಸಾಲು::globalWarnOff(V3ErrorCode::I_LINT, ಅಸತ್ಯ); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_ಸಾಲು::globalWarnOff(V3ErrorCode::I_STYLE, ಅಸತ್ಯ); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        ಯದಿ (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } ಅನ್ಯಥಾ {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [ಏತತ್, fl](ಧ್ರುವ ವರ್ಣ* valp) {
        ಯದಿ (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } ಅನ್ಯಥಾ ಯದಿ (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } ಅನ್ಯಥಾ {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [ಏತತ್, &optdir](ಧ್ರುವ ವರ್ಣ* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        ಯದಿ (argv[i][0] == '-' || argv[i][0] == '+') {
            ಧ್ರುವ std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            ಪೂರ್ಣಾಙ್ಕ consumed = 0;
            ತಾರ್ಕಿಕ isForRerun = ಸತ್ಯ;

            // Special case for -j which has an optional argument
            ಯದಿ (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                ಪೂರ್ಣಾಙ್ಕ val = 0;
                ಯದಿ (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    ಯದಿ (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                ಯದಿ (m_buildJobs == -1) m_buildJobs = val;
                ಯದಿ (m_verilateJobs == -1) m_verilateJobs = val;
                ಯದಿ (m_outputGroups == -1) m_outputGroups = val;
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                ಯದಿ (consumed) {
                    // Already processed
                } ಅನ್ಯಥಾ ಯದಿ (isFuture0(argName)) {
                    consumed = 1;
                } ಅನ್ಯಥಾ ಯದಿ (isFuture1(argName)) {
                    consumed = 2;
                } ಅನ್ಯಥಾ {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            ಯದಿ (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            ಅನುವರ್ತ;
        }

        // Filename
        ಧ್ರುವ std::string filename = parseFileArg(optdir, argv[i]);
        ತಾರ್ಕಿಕ isForRerun = ಸತ್ಯ;
        ಯದಿ (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } ಅನ್ಯಥಾ ಯದಿ (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } ಅನ್ಯಥಾ ಯದಿ (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = ಅಸತ್ಯ;
        } ಅನ್ಯಥಾ {
            V3Options::addVFile(filename, work());
            isForRerun = ಅಸತ್ಯ;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    ಯದಿ (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

ಶೂನ್ಯ V3Options::parseOptsFile(File_ಸಾಲು* fl, ಧ್ರುವ string& filename, ತಾರ್ಕಿಕ rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ಧ್ರುವ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    ಯದಿ (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        ಪ್ರತ್ಯಾಗಮ;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    ತಾರ್ಕಿಕ has_fargs = ಅಸತ್ಯ;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ಧ್ರುವ auto parse_fargs_cmt = [&](ಧ್ರುವ ವರ್ಣ* p) -> ಶೂನ್ಯ {
        ಧ್ರುವ ತಾರ್ಕಿಕ blockCmt = p[1] == '*';
        p += 2;
        ಯಾವತ್ (std::isspace(*p)) ++p;
        ಯದಿ (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            ಯದಿ (std::isspace(*p)) {
                ಯಾವತ್ (std::isspace(*p)) ++p;
                ಯದಿ (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    ಯದಿ (std::isspace(*p)) {
                        ಯಾವತ್ (std::isspace(*p)) ++p;
                        ಯದಿ (!has_fargs) {
                            has_fargs = ಸತ್ಯ;
                            whole_file.clear();
                        }
                        whole_file += p;
                        ಯದಿ (blockCmt) {
                            ಧ್ರುವ std::string::size_type pos = whole_file.find("*/");
                            ಯದಿ (pos == std::string::npos) {
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

    ತಾರ್ಕಿಕ inCmt = ಅಸತ್ಯ;
    ಯಾವತ್ (!ifp->eof()) {
        ಧ್ರುವ std::string ಸಾಲು = V3Os::getline(*ifp);
        // Strip simple comments
        ವರ್ಣ lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        ತಾರ್ಕಿಕ space_begin = ಸತ್ಯ;  // At beginning or leading spaces only
        ಪ್ರತಿ (ಧ್ರುವ ವರ್ಣ* p = ಸಾಲು.c_str(); *p; lastch = *p++) {
            ಯದಿ (inCmt) {
                ಯದಿ (p[0] == '*' && p[1] == '/') {
                    inCmt = ಅಸತ್ಯ;
                    ++p;
                }
            } ಅನ್ಯಥಾ ಯದಿ (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                ಭಙ್ಗ;  // Ignore to EOL
            } ಅನ್ಯಥಾ ಯದಿ (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                ಭಙ್ಗ;  // Ignore to EOL
            } ಅನ್ಯಥಾ ಯದಿ (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = ಸತ್ಯ;
                space_begin = ಅಸತ್ಯ;
                ++p;
            } ಅನ್ಯಥಾ {
                ಯದಿ (!std::isspace(*p)) space_begin = ಅಸತ್ಯ;
                ಯದಿ (!has_fargs) whole_file += *p;
            }
        }
        ಯದಿ (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    ಯದಿ (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = ನೂತನ File_ಸಾಲು{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    ಗಣನಾ state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    ಪ್ರತಿ (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        ವರ್ಣ curr_char = whole_file[pos];
        ವಿಕಲ್ಪ (st) {
        ಪಕ್ಷ ST_IN_OPTION:  // Get all chars up to a white space or a "="
            ಯದಿ (std::isspace(curr_char)) {  // End of option
                ಯದಿ (!arg.empty()) {  // End of word
                    args.ತಳ್ಳು_back(arg);
                }
                arg = "";
                ಭಙ್ಗ;
            }
            ಯದಿ (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                ಭಙ್ಗ;
            }
            ಯದಿ (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                ಯದಿ (pos < whole_file.length()) curr_char = whole_file[pos];
                ಯದಿ (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } ಅನ್ಯಥಾ {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                ಭಙ್ಗ;
            }
            ಯದಿ (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                ಭಙ್ಗ;
            }
            arg += curr_char;
            ಭಙ್ಗ;
        ಪಕ್ಷ ST_IN_QUOTED_STR:  // Just store all chars inside string
            ಯದಿ (curr_char != '\'') {
                arg += curr_char;
            } ಅನ್ಯಥಾ {  // End of quoted string
                st = ST_IN_OPTION;
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            ಯದಿ (curr_char != '"') {
                ಯದಿ (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } ಅನ್ಯಥಾ {
                    arg += curr_char;
                }
            } ಅನ್ಯಥಾ {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            ಭಙ್ಗ;
        }
    }
    ಯದಿ (!arg.empty()) {  // Add last word
        args.ತಳ್ಳು_back(arg);
    }

    // Path
    ಧ್ರುವ string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<ವರ್ಣ*> argv;
    argv.reserve(args.size() + 1);
    ಪ್ರತಿ (ಧ್ರುವ string& i : args) argv.ತಳ್ಳು_back(const_cast<ವರ್ಣ*>(i.c_str()));
    argv.ತಳ್ಳು_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<ಪೂರ್ಣಾಙ್ಕ>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(ಧ್ರುವ string& optdir, ಧ್ರುವ string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    ಯದಿ (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    ಪ್ರತ್ಯಾಗಮ filename;
}

//======================================================================

ಶೂನ್ಯ V3Options::showVersion(ತಾರ್ಕಿಕ verbose) {
    cout << version();
    cout << "\n";
    ಯದಿ (!verbose) ಪ್ರತ್ಯಾಗಮ;

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
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_ಬೇರು << "\n";
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
    m_impp = ನೂತನ V3OptionsImp;

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

ಶೂನ್ಯ V3Options::setDebugMode(ಪೂರ್ಣಾಙ್ಕ level) {
    V3Error::debugDefault(level);
    ಯದಿ (!m_dumpLevel.ಎಣಿಕೆ("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = ಸತ್ಯ;
    m_debugCheck = ಸತ್ಯ;
    ಯದಿ (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(ಧ್ರುವ string& tag) ಧ್ರುವ VL_MT_SAFE {
    ಧ್ರುವ auto iter = m_debugLevel.find(tag);
    ಪ್ರತ್ಯಾಗಮ iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(ಧ್ರುವ string& srcfile_path) ಧ್ರುವ VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ಪ್ರತ್ಯಾಗಮ debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(ಧ್ರುವ string& tag) ಧ್ರುವ VL_MT_SAFE {
    ಧ್ರುವ auto iter = m_dumpLevel.find(tag);
    ಪ್ರತ್ಯಾಗಮ iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(ಧ್ರುವ string& srcfile_path) ಧ್ರುವ VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ಪ್ರತ್ಯಾಗಮ dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

ತಾರ್ಕಿಕ V3Options::dump_ವೃಕ್ಷ_Addrids() ಧ್ರುವ VL_MT_SAFE {
    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ s_level = -1;
    ಯದಿ (VL_UNLIKELY(s_level < 0)) {
        ಧ್ರುವ unsigned ಮೌಲ್ಯ = dumpLevel("tree-addrids");
        ಯದಿ (!available()) ಪ್ರತ್ಯಾಗಮ ಮೌಲ್ಯ > 0;
        s_level = static_cast<unsigned>(ಮೌಲ್ಯ);
    }
    ಪ್ರತ್ಯಾಗಮ s_level > 0;
}

ಶೂನ್ಯ V3Options::optimize(ಪೂರ್ಣಾಙ್ಕ level) {
    // Set all optimizations to on/off
    ಧ್ರುವ ತಾರ್ಕಿಕ flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_ವೃಕ್ಷ = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_ವೃಕ್ಷ = flag;
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
    ಯದಿ (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
