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

#include "V3PchAstMT.h"

#include "V3Options.h"

#include "V3Container.h"
#include "V3Error.h"
#include "V3File.h"
#include "V3Global.h"
#include "V3Mutex.h"
#include "V3OptionParser.h"
#include "V3Os.h"
#include "V3PreShell.h"
#include "V3String.h"

// clang-format off
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#include <algorithm>
#include <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#ᱵᱟᱝ
# include <dirent.h>
#endif
#include <fcntl.h>
#include <list>
#include <map>
#include <memory>
#include <thread>
#include <set>
#include <string>

#include "config_rev.h"

#ᱡᱩᱫᱤ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

class V3OptionsImp final {
public:
    // TYPES
    using DirMap = std::map<const string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_lineArgs;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, bool>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<const string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    void addIncDirUser(const string& incdir) {
        const string& dir = V3Os::filenameCleanup(incdir);
        const bool inserted = m_incDirUsers.insert(dir);
        ᱡᱩᱫᱤ (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    void addIncDirFallback(const string& incdir) {
        const string& dir = V3Os::filenameCleanup(incdir);
        ᱡᱩᱫᱤ (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            const auto itFoundPair = m_incDirFallbackSet.insert(dir);
            ᱡᱩᱫᱤ (itFoundPair.second) m_incDirFallbacks.push_back(dir);
        }
    }
    void addLangExt(const string& langext, const V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        ᱡᱩᱫᱤ (addext[0] == '.') addext = addext.substr(1);
        (void)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    void addLibExtV(const string& libext) { m_libExtVs.insert(libext); }
    V3OptionsImp() = default;
    ~V3OptionsImp() = default;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(const var_na* textp) {
    // Return code for given string, or ERROR, which is a bad code
    for (puur_naa_nka codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        const V3LangCode code{codei};
        ᱡᱩᱫᱤ (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            ᱨᱩᱣᱟᱹᱲ;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(const string& value, bool& badr)
    : m_e{VTimescale::NONE} {
    badr = true;
    const string spaceless = VString::removeWhitespace(value);
    for (puur_naa_nka i = TS_100S; i < _ENUM_END; ++i) {
        const VTimescale ts{i};
        ᱡᱩᱫᱤ (spaceless == ts.ascii()) {
            badr = false;
            m_e = ts.m_e;
            bha_nga;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(const string& opts) {
    VStringList vals;
    bool inStr = false;
    string cur;
    static const string hierBlock("--hierarchical-block");
    FileLine cmdfl(FileLine::commandLineFilename());
    // Split by ','. If ',' appears between "", that is not a separator.
    for (string::const_iterator it = opts.begin(); it != opts.end();) {
        ᱡᱩᱫᱤ (inStr) {
            ᱡᱩᱫᱤ (*it == '\\') {
                ++it;
                ᱡᱩᱫᱤ (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    bha_nga;
                }
                ᱡᱩᱫᱤ (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    bha_nga;
                }
                cur.push_back(*it);
                ++it;
            } ᱵᱟᱝ ᱡᱩᱫᱤ (*it == '"') {  // end of string
                cur.push_back(*it);
                vals.push_back(cur);
                cur.clear();
                ++it;
                ᱡᱩᱫᱤ (it != opts.end()) {
                    ᱡᱩᱫᱤ (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        bha_nga;
                    }
                    ++it;
                    ᱡᱩᱫᱤ (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        bha_nga;
                    }
                    inStr = *it == '"';
                    cur.push_back(*it);
                    ++it;
                }
            } ᱵᱟᱝ {
                cur.push_back(*it);
                ++it;
            }
        } ᱵᱟᱝ {
            ᱡᱩᱫᱤ (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                bha_nga;
            }
            ᱡᱩᱫᱤ (*it == ',') {  // end of this parameter
                vals.push_back(cur);
                cur.clear();
                ++it;
                ᱡᱩᱫᱤ (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    bha_nga;
                }
                inStr = *it == '"';
            }
            cur.push_back(*it);
            ++it;
        }
    }
    ᱡᱩᱫᱤ (!cur.empty()) vals.push_back(cur);
    ᱡᱩᱫᱤ (vals.size() >= 2) {
        ᱡᱩᱫᱤ (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } ᱵᱟᱝ {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    for (size_t i = 2; i + 1 < vals.size(); i += 2) {
        const bool inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        ᱡᱩᱫᱤ (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

void VTimescale::parseSlashed(FileLine* fl, const var_na* textp, VTimescale& unitr,
                              VTimescale& precr, bool allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    const var_na* cp = textp;
    for (; std::isspace(*cp); ++cp) {}
    const var_na* const unitp = cp;
    for (; *cp && *cp != '/'; ++cp) {}
    const string unitStr(unitp, cp - unitp);
    for (; std::isspace(*cp); ++cp) {}
    string precStr;
    ᱡᱩᱫᱤ (*cp == '/') {
        ++cp;
        for (; std::isspace(*cp); ++cp) {}
        const var_na* const precp = cp;
        for (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    for (; std::isspace(*cp); ++cp) {}
    ᱡᱩᱫᱤ (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        ᱨᱩᱣᱟᱹᱲ;
    }

    bool unitbad = false;
    const VTimescale unit{unitStr, unitbad /*ref*/};
    ᱡᱩᱫᱤ (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        ᱨᱩᱣᱟᱹᱲ;
    }
    unitr = unit;

    ᱡᱩᱫᱤ (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        bool precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        ᱡᱩᱫᱤ (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            ᱨᱩᱣᱟᱹᱲ;
        }
        ᱡᱩᱫᱤ (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            ᱨᱩᱣᱟᱹᱲ;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

void V3Options::addIncDirUser(const string& incdir) { m_impp->addIncDirUser(incdir); }
void V3Options::addIncDirFallback(const string& incdir) { m_impp->addIncDirFallback(incdir); }
void V3Options::addLangExt(const string& langext, const V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
void V3Options::addLibExtV(const string& libext) { m_impp->addLibExtV(libext); }
void V3Options::addDefine(const string& defline, bool allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string left = defline;
    ᱡᱚᱠᱷᱚᱱ (left != "") {
        string def = left;
        string::size_type pos;
        ᱡᱩᱫᱤ (allowPlus && ((pos = left.find('+')) != string::npos)) {
            left = left.substr(pos + 1);
            def.erase(pos);
        } ᱵᱟᱝ {
            left = "";
        }
        string value;
        ᱡᱩᱫᱤ ((pos = def.find('=')) != string::npos) {
            value = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::defineCmdLine(def, value);
    }
}
void V3Options::addParameter(const string& paramline, bool allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string left = paramline;
    ᱡᱚᱠᱷᱚᱱ (left != "") {
        string param = left;
        string::size_type pos;
        ᱡᱩᱫᱤ (allowPlus && ((pos = left.find('+')) != string::npos)) {
            left = left.substr(pos + 1);
            param.erase(pos);
        } ᱵᱟᱝ {
            left = "";
        }
        string value;
        ᱡᱩᱫᱤ ((pos = param.find('=')) != string::npos) {
            value = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << value);
        (void)m_parameters.erase(param);
        m_parameters[param] = value;
    }
}

bool V3Options::hasParameter(const string& name) {
    ᱨᱩᱣᱟᱹᱲ m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(const string& name) {
    string value = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    ᱨᱩᱣᱟᱹᱲ value;
}

void V3Options::checkParameters() {
    ᱡᱩᱫᱤ (!m_parameters.empty()) {
        std::stringstream msg;
        msg << "Parameters from the command line were not found in the design:";
        for (const auto& i : m_parameters) msg << " " << i.first;
        v3error(msg.str());
    }
}

void V3Options::addCppFile(const string& filename) { m_cppFiles.insert(filename); }
void V3Options::addCFlags(const string& filename) { m_cFlags.push_back(filename); }
void V3Options::addLibMapFile(const string& filename) { m_libmapFiles.insert(filename); }
void V3Options::addCompilerIncludes(const string& filename) {
    m_compilerIncludes.insert(filename);
}
void V3Options::addLdLibFile(const string& filename) {
    m_ldLibFiles.push_back(filename);
    addLdLibs(filename);
}
void V3Options::addLdLibs(const string& filename) { m_ldLibs.push_back(filename); }
void V3Options::addMakeFlags(const string& filename) { m_makeFlags.push_back(filename); }
void V3Options::addFuture(const string& flag) { m_futures.insert(flag); }
void V3Options::addFuture0(const string& flag) { m_future0s.insert(flag); }
void V3Options::addFuture1(const string& flag) { m_future1s.insert(flag); }
bool V3Options::isFuture(const string& flag) const {
    ᱨᱩᱣᱟᱹᱲ m_futures.find(flag) != m_futures.end();
}
bool V3Options::isFuture0(const string& flag) const {
    ᱨᱩᱣᱟᱹᱲ m_future0s.find(flag) != m_future0s.end();
}
bool V3Options::isFuture1(const string& flag) const {
    ᱨᱩᱣᱟᱹᱲ m_future1s.find(flag) != m_future1s.end();
}
bool V3Options::isLibraryFile(const string& filename, const string& libname) const {
    ᱨᱩᱣᱟᱹᱲ m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
void V3Options::addLibraryFile(const string& filename, const string& libname) {
    m_libraryFiles.insert({filename, libname});
}
void V3Options::addVFile(const string& filename, const string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.push_back({filename, libname});  // Not emplace
}
void V3Options::addVltFile(const string& filename, const string& libname) {
    m_vltFiles.insert({filename, libname});
}
void V3Options::addForceInc(const string& filename) { m_forceIncs.push_back(filename); }

void V3Options::addLineArg(const string& arg) { m_impp->m_lineArgs.push_back(arg); }

void V3Options::addArg(var_na** argv, size_t count, bool isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, bool>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    for (size_t n = 0; n < count; ++n) pair.first.emplace_back(argv[n]);
}

void V3Options::addArg(const std::string& arg, bool isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, bool>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

const std::list<std::pair<std::list<std::string>, bool>>& V3Options::allArgs() const {
    ᱨᱩᱣᱟᱹᱲ m_impp->m_allArgs;
}

string V3Options::allArgsString() const VL_MT_SAFE {
    string result;
    for (const auto& pair : m_impp->m_allArgs) {
        for (const string& arg : pair.first) {
            ᱡᱩᱫᱤ (!result.empty()) result += " ";
            result += arg;
        }
    }
    ᱨᱩᱣᱟᱹᱲ result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(bool forTop) const {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    for (const auto& vFile : m_vFiles) vFiles.insert(vFile.filename());
    string out;
    bool stripArg = false;
    bool stripArgIfNum = false;
    for (const auto& pair : m_impp->m_allArgs) {
        for (const string& arg : pair.first) {
            ᱡᱩᱫᱤ (stripArg) {
                stripArg = false;
                continue;
            }
            ᱡᱩᱫᱤ (stripArgIfNum) {
                stripArgIfNum = false;
                ᱡᱩᱫᱤ (isdigit(arg[0])) continue;
            }
            puur_naa_nka skip = 0;
            ᱡᱩᱫᱤ (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } ᱵᱟᱝ ᱡᱩᱫᱤ (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            ᱡᱩᱫᱤ (skip > 0) {  // arg is an option
                const string opt = arg.substr(skip);  // Remove '-' in the beginning
                const puur_naa_nka numStrip = stripOptionsForChildRun(opt, forTop);
                ᱡᱩᱫᱤ (numStrip) {
                    UASSERT(0 <= numStrip && numStrip <= 3, "should be one of 0, 1, 2, 3");
                    ᱡᱩᱫᱤ (numStrip == 2) stripArg = true;
                    ᱡᱩᱫᱤ (numStrip == 3) stripArgIfNum = true;
                    continue;
                }
            } ᱵᱟᱝ {  // Not an option
                ᱡᱩᱫᱤ (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    continue;
                }
            }
            ᱡᱩᱫᱤ (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    ᱨᱩᱣᱟᱹᱲ out;
}

void V3Options::ccSet() {  // --cc
    m_outFormatOk = true;
    m_systemC = false;
}

void V3Options::decorations(FileLine* fl, const string& arg) {  // --decorations
    ᱡᱩᱫᱤ (arg == "none") {
        m_decoration = false;
        m_decorationNodes = false;
    } ᱵᱟᱝ ᱡᱩᱫᱤ (arg == "node") {
        m_decoration = true;
        m_decorationNodes = true;
    } ᱵᱟᱝ ᱡᱩᱫᱤ (arg == "medium") {
        m_decoration = true;
        m_decorationNodes = false;
    } ᱵᱟᱝ {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() const VL_MT_SAFE {
    std::vector<std::string> result;
    ᱡᱩᱫᱤ (traceEnabledFst()) result.emplace_back("VerilatedFst");
    ᱡᱩᱫᱤ (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    ᱡᱩᱫᱤ (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    ᱨᱩᱣᱟᱹᱲ result;
}
std::vector<std::string> V3Options::traceClassLangs() const VL_MT_SAFE {
    std::vector<std::string> result;
    for (const auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    ᱨᱩᱣᱟᱹᱲ result;
}
std::vector<std::string> V3Options::traceSourceBases() const VL_MT_SAFE {
    std::vector<std::string> result;
    ᱡᱩᱫᱤ (traceEnabledFst()) result.emplace_back("verilated_fst");
    ᱡᱩᱫᱤ (traceEnabledSaif()) result.emplace_back("verilated_saif");
    ᱡᱩᱫᱤ (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    ᱨᱩᱣᱟᱹᱲ result;
}
std::vector<std::string> V3Options::traceSourceLangs() const VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    for (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    ᱨᱩᱣᱟᱹᱲ result;
}
std::string V3Options::traceClassBase() const VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    ᱨᱩᱣᱟᱹᱲ traceClassBases().front();
}
std::string V3Options::traceClassLang() const VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    ᱨᱩᱣᱟᱹᱲ traceClassLangs().front();
}

//######################################################################
// File searching

bool V3Options::fileStatNormal(const string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    sa_nracanaa stat sstat;  // Stat information
    const puur_naa_nka err = stat(filename.c_str(), &sstat);
    ᱡᱩᱫᱤ (err != 0) ᱨᱩᱣᱟᱹᱲ false;
    ᱡᱩᱫᱤ (S_ISDIR(sstat.st_mode)) ᱨᱩᱣᱟᱹᱲ false;
    ᱨᱩᱣᱟᱹᱲ true;
}

string V3Options::fileExists(const string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    const string dir = V3Os::filenameDir(filename);
    const string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    ᱡᱩᱫᱤ (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* const setp = &(diriter->second);

#ifdef _MSC_VER
        try {
            for (const auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->insert(dirEntry.path().filename().string());
        } gr_haa_na (std::filesystem::filesystem_error const& ex) {
            (void)ex;
            ᱨᱩᱣᱟᱹᱲ "";
        }
#ᱵᱟᱝ
        ᱡᱩᱫᱤ (DIR* const dirp = opendir(dir.c_str())) {
            ᱡᱚᱠᱷᱚᱱ (sa_nracanaa dirent* direntp = readdir(dirp)) setp->insert(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    const std::set<string>* const filesetp = &(diriter->second);
    const auto fileiter = filesetp->find(basename);
    ᱡᱩᱫᱤ (fileiter == filesetp->end()) ᱨᱩᱣᱟᱹᱲ "";  // Not found
    // Check if it is a directory, ignore if so
    const string filenameOut = V3Os::filenameJoin(dir, basename);
    ᱡᱩᱫᱤ (!fileStatNormal(filenameOut)) ᱨᱩᱣᱟᱹᱲ "";  // Directory
    ᱨᱩᱣᱟᱹᱲ filenameOut;
}

string V3Options::filePathCheckOneDir(const string& modname, const string& dirname) {
    for (const string& i : m_impp->m_libExtVs) {
        const string fn = V3Os::filenameJoin(dirname, modname + i);
        const string exists = fileExists(fn);
        ᱡᱩᱫᱤ (exists != "") ᱨᱩᱣᱟᱹᱲ exists;
    }
    ᱨᱩᱣᱟᱹᱲ "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
puur_naa_nka V3Options::stripOptionsForChildRun(const string& opt, bool forTop) {
    // Options to strip for both the top wrapper, and hier blocks
    static const std::unordered_map<std::string, puur_naa_nka> commonOpts{
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
    ᱡᱩᱫᱤ (commonOpts.count(opt)) ᱨᱩᱣᱟᱹᱲ commonOpts.at(opt);

    // Options to strip only for hier blocks
    ᱡᱩᱫᱤ (!forTop) {
        static const std::unordered_map<std::string, puur_naa_nka> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        ᱡᱩᱫᱤ (subOpts.count(opt)) ᱨᱩᱣᱟᱹᱲ subOpts.at(opt);
        ᱡᱩᱫᱤ (VString::startsWith(opt, "G")) ᱨᱩᱣᱟᱹᱲ 1;
        ᱡᱩᱫᱤ (VString::startsWith(opt, "pvalue")) ᱨᱩᱣᱟᱹᱲ 1;
    }

    // Do not strip
    ᱨᱩᱣᱟᱹᱲ 0;
}

void V3Options::validateIdentifier(FileLine* fl, const string& arg, const string& opt) {
    ᱡᱩᱫᱤ (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(FileLine* fl, const string& modname, const string& lastpath,
                           const string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    const string filename = V3Os::filenameCleanup(VName::dehash(modname));
    ᱡᱩᱫᱤ (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        const string exists = filePathCheckOneDir(filename, "");
        ᱡᱩᱫᱤ (exists != "") ᱨᱩᱣᱟᱹᱲ exists;
    }
    for (const string& dir : m_impp->m_incDirUsers) {
        const string exists = filePathCheckOneDir(filename, dir);
        ᱡᱩᱫᱤ (exists != "") ᱨᱩᱣᱟᱹᱲ exists;
    }
    for (const string& dir : m_impp->m_incDirFallbacks) {
        const string exists = filePathCheckOneDir(filename, dir);
        ᱡᱩᱫᱤ (exists != "") ᱨᱩᱣᱟᱹᱲ exists;
    }

    ᱡᱩᱫᱤ (m_relativeIncludes) {
        const string exists = filePathCheckOneDir(filename, lastpath);
        ᱡᱩᱫᱤ (exists != "") ᱨᱩᱣᱟᱹᱲ V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    ᱡᱩᱫᱤ (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << filePathLookedMsg(fl, filename));
    }
    ᱨᱩᱣᱟᱹᱲ "";
}

string V3Options::filePathLookedMsg(FileLine* /*fl*/, const string& modname) {
    static bool s_shown_notfound_msg = false;
    std::ostringstream ss;
    ᱡᱩᱫᱤ (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } ᱵᱟᱝ ᱡᱩᱫᱤ (!s_shown_notfound_msg) {
        s_shown_notfound_msg = true;
        ᱡᱩᱫᱤ (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        for (const string& dir : m_impp->m_incDirUsers) {
            for (const string& ext : m_impp->m_libExtVs) {
                const string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        for (const string& dir : m_impp->m_incDirFallbacks) {
            for (const string& ext : m_impp->m_libExtVs) {
                const string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    ᱨᱩᱣᱟᱹᱲ ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(const string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    ᱡᱩᱫᱤ (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        ᱨᱩᱣᱟᱹᱲ V3LangCode::mostRecent();
    } ᱵᱟᱝ ᱡᱩᱫᱤ ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        const auto it = m_impp->m_langExts.find(ext);
        ᱡᱩᱫᱤ (it != m_impp->m_langExts.end()) ᱨᱩᱣᱟᱹᱲ it->second;
    }
    ᱨᱩᱣᱟᱹᱲ m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(const string& var) {
    // If update below, also update V3Options::showVersion()
    ᱡᱩᱫᱤ (var == "MAKE") {
        ᱨᱩᱣᱟᱹᱲ getenvMAKE();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "PERL") {
        ᱨᱩᱣᱟᱹᱲ getenvPERL();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "PYTHON3") {
        ᱨᱩᱣᱟᱹᱲ getenvPYTHON3();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "SYSTEMC") {
        ᱨᱩᱣᱟᱹᱲ getenvSYSTEMC();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "SYSTEMC_ARCH") {
        ᱨᱩᱣᱟᱹᱲ getenvSYSTEMC_ARCH();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "SYSTEMC_INCLUDE") {
        ᱨᱩᱣᱟᱹᱲ getenvSYSTEMC_INCLUDE();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "SYSTEMC_LIBDIR") {
        ᱨᱩᱣᱟᱹᱲ getenvSYSTEMC_LIBDIR();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "VERILATOR_ROOT") {
        ᱨᱩᱣᱟᱹᱲ getenvVERILATOR_ROOT();
    } ᱵᱟᱝ {
        ᱨᱩᱣᱟᱹᱲ V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { ᱨᱩᱣᱟᱹᱲ V3Os::getenvStr("MAKE", "gmake"); }
#ᱵᱟᱝ
string V3Options::getenvMAKE() { ᱨᱩᱣᱟᱹᱲ V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    ᱨᱩᱣᱟᱹᱲ V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    ᱨᱩᱣᱟᱹᱲ V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    ᱨᱩᱣᱟᱹᱲ V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_SYSTEMC};
    ᱡᱩᱫᱤ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    ᱨᱩᱣᱟᱹᱲ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_SYSTEMC_ARCH};
    ᱡᱩᱫᱤ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    ᱡᱩᱫᱤ (var == "") {
#ᱡᱩᱫᱤ defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        const string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#elif defined(_WIN32)
        const string sysname = "WIN32";
        var = "win32";
#ᱵᱟᱝ
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        sa_nracanaa utsname uts;
        uname(&uts);
        const string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        ᱡᱩᱫᱤ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } ᱵᱟᱝ ᱡᱩᱫᱤ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } ᱵᱟᱝ {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    ᱨᱩᱣᱟᱹᱲ var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    ᱡᱩᱫᱤ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    ᱡᱩᱫᱤ (var == "") {
        const string sc = getenvSYSTEMC();
        ᱡᱩᱫᱤ (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    ᱨᱩᱣᱟᱹᱲ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    ᱡᱩᱫᱤ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    ᱡᱩᱫᱤ (var == "") {
        const string sc = getenvSYSTEMC();
        const string arch = getenvSYSTEMC_ARCH();
        ᱡᱩᱫᱤ (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    ᱨᱩᱣᱟᱹᱲ V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_ROOT() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_VERILATOR_ROOT};
    ᱡᱩᱫᱤ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    ᱡᱩᱫᱤ (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    ᱨᱩᱣᱟᱹᱲ V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_VERILATOR_SOLVER};
    ᱡᱩᱫᱤ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    ᱨᱩᱣᱟᱹᱲ var;
}

string V3Options::getStdPackagePath() {
    ᱨᱩᱣᱟᱹᱲ V3Os::filenameJoin(getenvVERILATOR_ROOT(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    ᱨᱩᱣᱟᱹᱲ V3Os::filenameJoin(getenvVERILATOR_ROOT(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(const string& var) {
    // If update below, also update V3Options::showVersion()
    ᱡᱩᱫᱤ (var == "COROUTINES" && coroutineSupport()) {
        ᱨᱩᱣᱟᱹᱲ "1";
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "TSAN" && tsanSupport()) {
        ᱨᱩᱣᱟᱹᱲ "1";
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "DEV_ASAN" && devAsan()) {
        ᱨᱩᱣᱟᱹᱲ "1";
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "DEV_GCOV" && devGcov()) {
        ᱨᱩᱣᱟᱹᱲ "1";
        // cppcheck-suppress knownConditionTrueFalse
    } ᱵᱟᱝ ᱡᱩᱫᱤ (var == "SYSTEMC" && systemCFound()) {
        ᱨᱩᱣᱟᱹᱲ "1";
    } ᱵᱟᱝ {
        ᱨᱩᱣᱟᱹᱲ "";
    }
}

bool V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    ᱨᱩᱣᱟᱹᱲ true;
#ᱵᱟᱝ
    ᱨᱩᱣᱟᱹᱲ false;
#endif
}

bool V3Options::systemCFound() {
    ᱨᱩᱣᱟᱹᱲ (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

bool V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    ᱨᱩᱣᱟᱹᱲ true;
#ᱵᱟᱝ
    ᱨᱩᱣᱟᱹᱲ false;
#endif
}

bool V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    ᱨᱩᱣᱟᱹᱲ true;
#ᱵᱟᱝ
    ᱨᱩᱣᱟᱹᱲ false;
#endif
}

bool V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    ᱨᱩᱣᱟᱹᱲ true;
#ᱵᱟᱝ
    ᱨᱩᱣᱟᱹᱲ false;
#endif
}

bool V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    ᱨᱩᱣᱟᱹᱲ true;
#ᱵᱟᱝ
    ᱨᱩᱣᱟᱹᱲ false;
#endif
}

//######################################################################
// V3 Options notification methods

void V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    FileLine* const cmdfl = new FileLine{FileLine::commandLineFilename()};

    ᱡᱩᱫᱤ (!outFormatOk() && v3Global.opt.main()) ccSet();  // --main implies --cc if not provided
    ᱡᱩᱫᱤ (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    ᱡᱩᱫᱤ (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    ᱡᱩᱫᱤ (m_build) {
        ᱡᱩᱫᱤ (m_binary)
            backendFlags.emplace_back("--binary");
        ᱵᱟᱝ
            backendFlags.emplace_back("--build");
    }
    ᱡᱩᱫᱤ (m_preprocOnly) backendFlags.emplace_back("-E");
    ᱡᱩᱫᱤ (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    ᱡᱩᱫᱤ (m_lintOnly) backendFlags.emplace_back("--lint-only");
    ᱡᱩᱫᱤ (m_jsonOnly) backendFlags.emplace_back("--json-only");
    ᱡᱩᱫᱤ (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        for (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    ᱡᱩᱫᱤ (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    ᱡᱩᱫᱤ (!m_gmake && !m_makeJson) m_gmake = true;

    ᱡᱩᱫᱤ (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    ᱡᱩᱫᱤ (m_hierChild) {
        ᱡᱩᱫᱤ (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = false;
    }

    ᱡᱩᱫᱤ (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = true;  // No format, with --trace means wanted --trace-vcd
    }
    ᱡᱩᱫᱤ (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = true;
    const puur_naa_nka ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    ᱡᱩᱫᱤ (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    ᱡᱩᱫᱤ (protectIds()) {
        ᱡᱩᱫᱤ (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        ᱡᱩᱫᱤ (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        ᱡᱩᱫᱤ (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    ᱡᱩᱫᱤ (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    ᱡᱩᱫᱤ (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    ᱡᱩᱫᱤ (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(true);

    // Default split limits if not specified
    ᱡᱩᱫᱤ (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    ᱡᱩᱫᱤ (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    ᱡᱩᱫᱤ (v3Global.opt.main() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    ᱡᱩᱫᱤ (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        ᱡᱩᱫᱤ (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    ᱡᱩᱫᱤ (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    ᱡᱩᱫᱤ (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    ᱡᱩᱫᱤ (!m_dumpLevel.count("tree") && m_dumpLevel.count("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    ᱡᱩᱫᱤ (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    ᱡᱩᱫᱤ (m_buildJobs == -1) m_buildJobs = 1;
    ᱡᱩᱫᱤ (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    ᱡᱩᱫᱤ (timing().isSetTrue()) V3PreShell::defineCmdLine("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    ᱡᱩᱫᱤ (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(false);

    // === Leave last
    // Mark options as available
    m_available = true;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    ᱨᱩᱣᱟᱹᱲ ver;
}

string V3Options::protectKeyDefaulted() VL_MT_SAFE {
    static V3Mutex s_mutex;
    const V3LockGuard lock{s_mutex};
    ᱡᱩᱫᱤ (m_protectKey.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protectKey = "VL-KEY-" + digest.digestBase64();
    }
    ᱨᱩᱣᱟᱹᱲ m_protectKey;
}

void V3Options::throwSigsegv() {  // LCOV_EXCL_START
#ᱡᱩᱫᱤ !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<ca_ncala var_na*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(const VTimescale& flag) const {
    ᱡᱩᱫᱤ (!timeOverridePrec().isNone()) {
        ᱨᱩᱣᱟᱹᱲ timeOverridePrec();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (flag.isNone()) {
        ᱨᱩᱣᱟᱹᱲ timeDefaultPrec();
    } ᱵᱟᱝ {
        ᱨᱩᱣᱟᱹᱲ flag;
    }
}

VTimescale V3Options::timeComputeUnit(const VTimescale& flag) const {
    ᱡᱩᱫᱤ (!timeOverrideUnit().isNone()) {
        ᱨᱩᱣᱟᱹᱲ timeOverrideUnit();
    } ᱵᱟᱝ ᱡᱩᱫᱤ (flag.isNone()) {
        ᱨᱩᱣᱟᱹᱲ timeDefaultUnit();
    } ᱵᱟᱝ {
        ᱨᱩᱣᱟᱹᱲ flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(puur_naa_nka argc, var_na** argv) {
    // Return list of arguments as simple string
    string opts;
    for (puur_naa_nka i = 0; i < argc; ++i) {
        ᱡᱩᱫᱤ (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    ᱨᱩᱣᱟᱹᱲ opts;
}

//######################################################################
// V3 Options Parsing

void V3Options::parseOpts(FileLine* fl, puur_naa_nka argc, var_na** argv) VL_MT_DISABLED {
    // Save command line options
    for (puur_naa_nka i = 0; i < argc; ++i) addLineArg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    ᱡᱩᱫᱤ (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    ᱡᱩᱫᱤ (prefix() == "" && topModule() != "") m_prefix = "V"s + topModule();  // Already encoded
    ᱡᱩᱫᱤ (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + AstNode::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    ᱡᱩᱫᱤ (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

bool V3Options::suffixed(const string& sw, const var_na* arg) {
    ᱡᱩᱫᱤ (std::strlen(arg) > sw.length()) ᱨᱩᱣᱟᱹᱲ false;
    ᱨᱩᱣᱟᱹᱲ (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

void V3Options::parseOptsList(FileLine* fl, const string& optdir, puur_naa_nka argc,
                              var_na** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    const V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    const auto callStrSetter = [this](void (V3Options::*cbStr)(const string&)) {
        ᱨᱩᱣᱟᱹᱲ [this, cbStr](const string& v) { (this->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [this](const var_na* optp) VL_MT_DISABLED {
        addDefine(optp, true);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [this, &optdir](const var_na* optp) {
        string dirs = optp;
        string::size_type pos;
        ᱡᱚᱠᱷᱚᱱ ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [this](const var_na* optp) {
        string exts = optp;
        string::size_type pos;
        ᱡᱚᱠᱷᱚᱱ ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [this](const var_na* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](bool) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [this](bool flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](const var_na*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [this](bool flag) {
        m_bboxUnsup = flag;
        FileLine::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, true);
    });
    DECL_OPTION("-binary", CbCall, [this]() {
        m_binary = true;
        m_build = true;
        m_exe = true;
        m_main = true;
        ᱡᱩᱫᱤ (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [this, fl](const var_na* valp) {
        puur_naa_nka val = std::atoi(valp);
        ᱡᱩᱫᱤ (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } ᱵᱟᱝ ᱡᱩᱫᱤ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [this]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](const std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](const std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](puur_naa_nka val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [this, fl](const var_na* valp) {
        ᱡᱩᱫᱤ (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } ᱵᱟᱝ {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [this](bool flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverageExpr);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverageExprMax);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [this, fl](const var_na* const valp) {
        ᱡᱩᱫᱤ (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } ᱵᱟᱝ {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverageLine);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    const auto parseBinsLimit = [fl](const var_na* optp, const var_na* valp, uint32_t& limitr) {
        var_na* endp = nullptr;
        const unsigned long long value = std::strtoull(valp, &endp, 10);
        ᱡᱩᱫᱤ (*endp || value < 1 || value > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            ᱨᱩᱣᱟᱹᱲ;
        }
        limitr = static_cast<uint32_t>(value);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [this, parseBinsLimit](const var_na* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [this, parseBinsLimit](const var_na* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [this](const var_na* valp) VL_MT_DISABLED {
        addDefine(valp, false);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [this]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [this](puur_naa_nka v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [this](const var_na* optp, const var_na* valp) {
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debugStackCheck).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [this, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [this, fl](const var_na* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [this, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [this](const var_na* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = true;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [this](const var_na* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [this](const var_na* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [this](const var_na* optp, const var_na* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [this](bool flag) {
        ᱡᱩᱫᱤ (flag) {
            m_stdPackage = false;
            m_stdWaiver = false;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<void (*)(puur_naa_nka)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [this](const var_na* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [this, fl, &optdir](const var_na* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), true);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [this, &optdir](const var_na* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [this, fl, &optdir](const var_na* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), false);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [this](const var_na* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [this](const var_na* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [this](bool flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_fCaseTree = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_fCaseTree);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_fConstBitOpTree);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [this](bool flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [this](bool flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [this](const var_na* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [this](const var_na* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_fDfgPushDownSels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [this, fl](bool flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [this](bool flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [this](bool flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [this, fl](const var_na* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        ᱡᱩᱫᱤ (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [this](const var_na* optp) { addParameter(optp, false); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [this]() {
        cout << protectKeyDefaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](const var_na* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](const var_na* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [this](const var_na* valp) {
        const V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [this](const var_na* optp) { m_hierParamsFile.push_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [this, &optdir](const var_na* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [this, fl](puur_naa_nka val) {
        m_instrCountDpi = val;
        ᱡᱩᱫᱤ (m_instrCountDpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [this](const var_na* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = true;
    });
    DECL_OPTION("-json-only-output", CbVal, [this](const var_na* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = true;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [this]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [this]() { m_l2Name = "v"; }).undocumented();  // Historical
    const auto setLang = [this, fl](const var_na* valp) {
        const V3LangCode optval{valp};
        ᱡᱩᱫᱤ (optval.legal()) {
            m_defaultLanguage = optval;
        } ᱵᱟᱝ {
            VSpellCheck spell;
            for (puur_naa_nka i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.pushCandidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.bestCandidateMsg(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [this](const var_na* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [this, fl](const var_na* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [this](const var_na* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_mainTopName);
    DECL_OPTION("-make", CbVal, [this, fl](const var_na* valp) {
        ᱡᱩᱫᱤ (!std::strcmp(valp, "gmake")) {
            m_gmake = true;
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "json")) {
            m_makeJson = true;
        } ᱵᱟᱝ {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_maxNumWidth);
    DECL_OPTION("-mod-prefix", CbVal, [this, fl](const var_na* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [this]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [this]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [this]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [this]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](bool /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [this, fl](const var_na* valp) {
        m_outputGroups = std::atoi(valp);
        ᱡᱩᱫᱤ (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [this, fl](const var_na* valp) {
        m_outputGroupsSize = std::atoll(valp);
        ᱡᱩᱫᱤ (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [this, fl](const var_na* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        ᱡᱩᱫᱤ (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [this, fl](const var_na* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        ᱡᱩᱫᱤ (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preprocNoLine);
    DECL_OPTION("-pins64", CbCall, [this]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [this]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [this, fl](const var_na* valp) {
        m_pinsBv = std::atoi(valp);
        ᱡᱩᱫᱤ (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [this](bool flag) {
        m_pinsScUint = flag;
        ᱡᱩᱫᱤ (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [this](bool flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [this](bool flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [this, fl](const var_na* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [this, fl](const var_na* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        ᱡᱩᱫᱤ (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [this]() { m_public = false; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [this]() { m_profC = m_profCFuncs = true; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [this]() {
        m_profC = m_profCFuncs = true;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protectKey);
    DECL_OPTION("-protect-lib", CbVal, [this, fl](const var_na* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = true;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [this](bool flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(true);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [this](bool flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [this](bool flag) {
        m_publicParams = flag;
        v3Global.dpi(true);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [this](const var_na* varp) { addParameter(varp, false); });

    DECL_OPTION("-quiet", CbOnOff, [this](bool flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [this, fl](const var_na* valp) {
        m_reloopLimit = std::atoi(valp);
        ᱡᱩᱫᱤ (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [this, fl](const var_na* valp) {
        m_replicationLimit = std::atoi(valp);
        ᱡᱩᱫᱤ (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [this, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [this]() {
        m_outFormatOk = true;
        m_systemC = true;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [this](bool flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [this](bool flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [this]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [this, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [this, fl](const var_na* valp) {
        m_threads = std::atoi(valp);
        ᱡᱩᱫᱤ (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        ᱡᱩᱫᱤ (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [this, fl](const var_na* valp) {
        m_hierThreads = std::atoi(valp);
        ᱡᱩᱫᱤ (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [this, fl](const var_na* valp) {
        ᱡᱩᱫᱤ (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = true;
            m_threadsDpiUnpure = true;
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = false;
            m_threadsDpiUnpure = false;
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = true;
            m_threadsDpiUnpure = false;
        } ᱵᱟᱝ {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [this, fl](const var_na* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        ᱡᱩᱫᱤ (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [this, fl](const var_na* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        ᱡᱩᱫᱤ (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        ᱡᱩᱫᱤ (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [this, fl](const var_na* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, true);
        ᱡᱩᱫᱤ (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        ᱡᱩᱫᱤ (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [this](const std::string& flag) { m_topModule = AstNode::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [this](const std::string& flag) { m_topModule = AstNode::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_topModule).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [this]() { m_traceEnabledSaif = true; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [this]() {
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
    DECL_OPTION("-trace-threads", CbVal, [fl](const var_na*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_noTraceTop);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [this]() { m_traceEnabledVcd = true; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unrollCount).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [this]() {
        showVersion(true);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [this, &optdir](const var_na* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [this, fl](const var_na* valp) {
        puur_naa_nka val = std::atoi(valp);
        ᱡᱩᱫᱤ (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } ᱵᱟᱝ ᱡᱩᱫᱤ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [this]() {
        showVersion(false);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { FileLine::globalWarnOff(V3ErrorCode::I_LINT, false); });
    DECL_OPTION("-Werror-", CbPartialMatch, [this, fl](const var_na* optp) {
        const V3ErrorCode code{optp};
        ᱡᱩᱫᱤ (code == V3ErrorCode::EC_ERROR) {
            ᱡᱩᱫᱤ (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } ᱵᱟᱝ {
            V3Error::pretendError(code, true);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [this](const var_na* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](const var_na* optp) VL_MT_DISABLED {
        const string err = FileLine::globalWarnOffParse(optp, true);
        ᱡᱩᱫᱤ (!err.empty()) {
            const string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    for (puur_naa_nka i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        for (const string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [this]() { m_context = false; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(false); });
    DECL_OPTION("-Wno-lint", CbCall, []() { FileLine::globalWarnOff(V3ErrorCode::I_LINT, true); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { FileLine::globalWarnOff(V3ErrorCode::I_STYLE, true); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [this]() {
        m_pedantic = true;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, false);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [this, fl, &parser](const var_na* optp) VL_MT_DISABLED {
        const V3ErrorCode code{optp};
        ᱡᱩᱫᱤ (code == V3ErrorCode::EC_ERROR) {
            ᱡᱩᱫᱤ (!isFuture(optp)) {
                const string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } ᱵᱟᱝ {
            FileLine::globalWarnOff(code, false);
            V3Error::pretendError(code, false);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { FileLine::globalWarnOff(V3ErrorCode::I_LINT, false); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { FileLine::globalWarnOff(V3ErrorCode::I_STYLE, false); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [this, fl](const var_na* valp) {
        ᱡᱩᱫᱤ (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } ᱵᱟᱝ {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [this, fl](const var_na* valp) {
        ᱡᱩᱫᱤ (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } ᱵᱟᱝ ᱡᱩᱫᱤ (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } ᱵᱟᱝ {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [this, &optdir](const var_na* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    for (puur_naa_nka i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        ᱡᱩᱫᱤ (argv[i][0] == '-' || argv[i][0] == '+') {
            const std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            puur_naa_nka consumed = 0;
            bool isForRerun = true;

            // Special case for -j which has an optional argument
            ᱡᱩᱫᱤ (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                puur_naa_nka val = 0;
                ᱡᱩᱫᱤ (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    ᱡᱩᱫᱤ (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                ᱡᱩᱫᱤ (m_buildJobs == -1) m_buildJobs = val;
                ᱡᱩᱫᱤ (m_verilateJobs == -1) m_verilateJobs = val;
                ᱡᱩᱫᱤ (m_outputGroups == -1) m_outputGroups = val;
            } ᱵᱟᱝ {
                const auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                ᱡᱩᱫᱤ (consumed) {
                    // Already processed
                } ᱵᱟᱝ ᱡᱩᱫᱤ (isFuture0(argName)) {
                    consumed = 1;
                } ᱵᱟᱝ ᱡᱩᱫᱤ (isFuture1(argName)) {
                    consumed = 2;
                } ᱵᱟᱝ {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            ᱡᱩᱫᱤ (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            continue;
        }

        // Filename
        const std::string filename = parseFileArg(optdir, argv[i]);
        bool isForRerun = true;
        ᱡᱩᱫᱤ (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } ᱵᱟᱝ ᱡᱩᱫᱤ (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } ᱵᱟᱝ ᱡᱩᱫᱤ (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = false;
        } ᱵᱟᱝ {
            V3Options::addVFile(filename, work());
            isForRerun = false;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    ᱡᱩᱫᱤ (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

void V3Options::parseOptsFile(FileLine* fl, const string& filename, bool rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    const std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    ᱡᱩᱫᱤ (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        ᱨᱩᱣᱟᱹᱲ;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    bool has_fargs = false;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    const auto parse_fargs_cmt = [&](const var_na* p) -> void {
        const bool blockCmt = p[1] == '*';
        p += 2;
        ᱡᱚᱠᱷᱚᱱ (std::isspace(*p)) ++p;
        ᱡᱩᱫᱤ (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            ᱡᱩᱫᱤ (std::isspace(*p)) {
                ᱡᱚᱠᱷᱚᱱ (std::isspace(*p)) ++p;
                ᱡᱩᱫᱤ (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    ᱡᱩᱫᱤ (std::isspace(*p)) {
                        ᱡᱚᱠᱷᱚᱱ (std::isspace(*p)) ++p;
                        ᱡᱩᱫᱤ (!has_fargs) {
                            has_fargs = true;
                            whole_file.clear();
                        }
                        whole_file += p;
                        ᱡᱩᱫᱤ (blockCmt) {
                            const std::string::size_type pos = whole_file.find("*/");
                            ᱡᱩᱫᱤ (pos == std::string::npos) {
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

    bool inCmt = false;
    ᱡᱚᱠᱷᱚᱱ (!ifp->eof()) {
        const std::string line = V3Os::getline(*ifp);
        // Strip simple comments
        var_na lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        bool space_begin = true;  // At beginning or leading spaces only
        for (const var_na* p = line.c_str(); *p; lastch = *p++) {
            ᱡᱩᱫᱤ (inCmt) {
                ᱡᱩᱫᱤ (p[0] == '*' && p[1] == '/') {
                    inCmt = false;
                    ++p;
                }
            } ᱵᱟᱝ ᱡᱩᱫᱤ (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                bha_nga;  // Ignore to EOL
            } ᱵᱟᱝ ᱡᱩᱫᱤ (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                bha_nga;  // Ignore to EOL
            } ᱵᱟᱝ ᱡᱩᱫᱤ (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = true;
                space_begin = false;
                ++p;
            } ᱵᱟᱝ {
                ᱡᱩᱫᱤ (!std::isspace(*p)) space_begin = false;
                ᱡᱩᱫᱤ (!has_fargs) whole_file += *p;
            }
        }
        ᱡᱩᱫᱤ (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    ᱡᱩᱫᱤ (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = new FileLine{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    ga_nanaa state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    for (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        var_na curr_char = whole_file[pos];
        switch (st) {
        pak_sa ST_IN_OPTION:  // Get all chars up to a white space or a "="
            ᱡᱩᱫᱤ (std::isspace(curr_char)) {  // End of option
                ᱡᱩᱫᱤ (!arg.empty()) {  // End of word
                    args.push_back(arg);
                }
                arg = "";
                bha_nga;
            }
            ᱡᱩᱫᱤ (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                bha_nga;
            }
            ᱡᱩᱫᱤ (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                ᱡᱩᱫᱤ (pos < whole_file.length()) curr_char = whole_file[pos];
                ᱡᱩᱫᱤ (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } ᱵᱟᱝ {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                bha_nga;
            }
            ᱡᱩᱫᱤ (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                bha_nga;
            }
            arg += curr_char;
            bha_nga;
        pak_sa ST_IN_QUOTED_STR:  // Just store all chars inside string
            ᱡᱩᱫᱤ (curr_char != '\'') {
                arg += curr_char;
            } ᱵᱟᱝ {  // End of quoted string
                st = ST_IN_OPTION;
            }
            bha_nga;
        pak_sa ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            ᱡᱩᱫᱤ (curr_char != '"') {
                ᱡᱩᱫᱤ (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } ᱵᱟᱝ {
                    arg += curr_char;
                }
            } ᱵᱟᱝ {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            bha_nga;
        pak_sa ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            bha_nga;
        }
    }
    ᱡᱩᱫᱤ (!arg.empty()) {  // Add last word
        args.push_back(arg);
    }

    // Path
    const string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<var_na*> argv;
    argv.reserve(args.size() + 1);
    for (const string& i : args) argv.push_back(const_cast<var_na*>(i.c_str()));
    argv.push_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<puur_naa_nka>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(const string& optdir, const string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    ᱡᱩᱫᱤ (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    ᱨᱩᱣᱟᱹᱲ filename;
}

//======================================================================

void V3Options::showVersion(bool verbose) {
    cout << version();
    cout << "\n";
    ᱡᱩᱫᱤ (!verbose) ᱨᱩᱣᱟᱹᱲ;

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
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_ROOT << "\n";
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
    m_impp = new V3OptionsImp;

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

void V3Options::setDebugMode(puur_naa_nka level) {
    V3Error::debugDefault(level);
    ᱡᱩᱫᱤ (!m_dumpLevel.count("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = true;
    m_debugCheck = true;
    ᱡᱩᱫᱤ (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(const string& tag) const VL_MT_SAFE {
    const auto iter = m_debugLevel.find(tag);
    ᱨᱩᱣᱟᱹᱲ iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(const string& srcfile_path) const VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ᱨᱩᱣᱟᱹᱲ debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(const string& tag) const VL_MT_SAFE {
    const auto iter = m_dumpLevel.find(tag);
    ᱨᱩᱣᱟᱹᱲ iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(const string& srcfile_path) const VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ᱨᱩᱣᱟᱹᱲ dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

bool V3Options::dumpTreeAddrids() const VL_MT_SAFE {
    static puur_naa_nka s_level = -1;
    ᱡᱩᱫᱤ (VL_UNLIKELY(s_level < 0)) {
        const unsigned value = dumpLevel("tree-addrids");
        ᱡᱩᱫᱤ (!available()) ᱨᱩᱣᱟᱹᱲ value > 0;
        s_level = static_cast<unsigned>(value);
    }
    ᱨᱩᱣᱟᱹᱲ s_level > 0;
}

void V3Options::optimize(puur_naa_nka level) {
    // Set all optimizations to on/off
    const bool flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_fCaseTree = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_fConstBitOpTree = flag;
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
    ᱡᱩᱫᱤ (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
