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
#không_thì
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

#nếu defined(_WIN32) || defined(__MINGW32__)
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
        nếu (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    void addIncDirFallback(const string& incdir) {
        const string& dir = V3Os::filenameCleanup(incdir);
        nếu (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            const auto itFoundPair = m_incDirFallbackSet.insert(dir);
            nếu (itFoundPair.second) m_incDirFallbacks.push_back(dir);
        }
    }
    void addLangExt(const string& langext, const V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        nếu (addext[0] == '.') addext = addext.substr(1);
        (void)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    void addLibExtV(const string& libext) { m_libExtVs.insert(libext); }
    V3OptionsImp() = default;
    ~V3OptionsImp() = default;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(const char* textp) {
    // Return code for given string, or ERROR, which is a bad code
    cho (số_nguyên codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        const V3LangCode code{codei};
        nếu (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            trả_về;
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
    cho (số_nguyên i = TS_100S; i < _ENUM_END; ++i) {
        const VTimescale ts{i};
        nếu (spaceless == ts.ascii()) {
            badr = false;
            m_e = ts.m_e;
            break;
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
    cho (string::const_iterator it = opts.begin(); it != opts.end();) {
        nếu (inStr) {
            nếu (*it == '\\') {
                ++it;
                nếu (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    break;
                }
                nếu (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    break;
                }
                cur.push_back(*it);
                ++it;
            } không_thì nếu (*it == '"') {  // end of string
                cur.push_back(*it);
                vals.push_back(cur);
                cur.clear();
                ++it;
                nếu (it != opts.end()) {
                    nếu (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        break;
                    }
                    ++it;
                    nếu (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        break;
                    }
                    inStr = *it == '"';
                    cur.push_back(*it);
                    ++it;
                }
            } không_thì {
                cur.push_back(*it);
                ++it;
            }
        } không_thì {
            nếu (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                break;
            }
            nếu (*it == ',') {  // end of this parameter
                vals.push_back(cur);
                cur.clear();
                ++it;
                nếu (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    break;
                }
                inStr = *it == '"';
            }
            cur.push_back(*it);
            ++it;
        }
    }
    nếu (!cur.empty()) vals.push_back(cur);
    nếu (vals.size() >= 2) {
        nếu (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } không_thì {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    cho (size_t i = 2; i + 1 < vals.size(); i += 2) {
        const bool inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        nếu (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

void VTimescale::parseSlashed(FileLine* fl, const char* textp, VTimescale& unitr,
                              VTimescale& precr, bool allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    const char* cp = textp;
    cho (; std::isspace(*cp); ++cp) {}
    const char* const unitp = cp;
    cho (; *cp && *cp != '/'; ++cp) {}
    const string unitStr(unitp, cp - unitp);
    cho (; std::isspace(*cp); ++cp) {}
    string precStr;
    nếu (*cp == '/') {
        ++cp;
        cho (; std::isspace(*cp); ++cp) {}
        const char* const precp = cp;
        cho (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    cho (; std::isspace(*cp); ++cp) {}
    nếu (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        trả_về;
    }

    bool unitbad = false;
    const VTimescale unit{unitStr, unitbad /*ref*/};
    nếu (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        trả_về;
    }
    unitr = unit;

    nếu (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        bool precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        nếu (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            trả_về;
        }
        nếu (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            trả_về;
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
    trong_khi (left != "") {
        string def = left;
        string::size_type pos;
        nếu (allowPlus && ((pos = left.find('+')) != string::npos)) {
            left = left.substr(pos + 1);
            def.erase(pos);
        } không_thì {
            left = "";
        }
        string value;
        nếu ((pos = def.find('=')) != string::npos) {
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
    trong_khi (left != "") {
        string param = left;
        string::size_type pos;
        nếu (allowPlus && ((pos = left.find('+')) != string::npos)) {
            left = left.substr(pos + 1);
            param.erase(pos);
        } không_thì {
            left = "";
        }
        string value;
        nếu ((pos = param.find('=')) != string::npos) {
            value = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << value);
        (void)m_parameters.erase(param);
        m_parameters[param] = value;
    }
}

bool V3Options::hasParameter(const string& name) {
    trả_về m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(const string& name) {
    string value = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    trả_về value;
}

void V3Options::checkParameters() {
    nếu (!m_parameters.empty()) {
        std::stringstream msg;
        msg << "Parameters from the command line were not found in the design:";
        cho (const auto& i : m_parameters) msg << " " << i.first;
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
    trả_về m_futures.find(flag) != m_futures.end();
}
bool V3Options::isFuture0(const string& flag) const {
    trả_về m_future0s.find(flag) != m_future0s.end();
}
bool V3Options::isFuture1(const string& flag) const {
    trả_về m_future1s.find(flag) != m_future1s.end();
}
bool V3Options::isLibraryFile(const string& filename, const string& libname) const {
    trả_về m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
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

void V3Options::addArg(char** argv, size_t count, bool isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, bool>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    cho (size_t n = 0; n < count; ++n) pair.first.emplace_back(argv[n]);
}

void V3Options::addArg(const std::string& arg, bool isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, bool>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

const std::list<std::pair<std::list<std::string>, bool>>& V3Options::allArgs() const {
    trả_về m_impp->m_allArgs;
}

string V3Options::allArgsString() const VL_MT_SAFE {
    string result;
    cho (const auto& pair : m_impp->m_allArgs) {
        cho (const string& arg : pair.first) {
            nếu (!result.empty()) result += " ";
            result += arg;
        }
    }
    trả_về result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(bool forTop) const {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    cho (const auto& vFile : m_vFiles) vFiles.insert(vFile.filename());
    string out;
    bool stripArg = false;
    bool stripArgIfNum = false;
    cho (const auto& pair : m_impp->m_allArgs) {
        cho (const string& arg : pair.first) {
            nếu (stripArg) {
                stripArg = false;
                continue;
            }
            nếu (stripArgIfNum) {
                stripArgIfNum = false;
                nếu (isdigit(arg[0])) continue;
            }
            số_nguyên skip = 0;
            nếu (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } không_thì nếu (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            nếu (skip > 0) {  // arg is an option
                const string opt = arg.substr(skip);  // Remove '-' in the beginning
                const số_nguyên numStrip = stripOptionsForChildRun(opt, forTop);
                nếu (numStrip) {
                    UASSERT(0 <= numStrip && numStrip <= 3, "should be one of 0, 1, 2, 3");
                    nếu (numStrip == 2) stripArg = true;
                    nếu (numStrip == 3) stripArgIfNum = true;
                    continue;
                }
            } không_thì {  // Not an option
                nếu (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    continue;
                }
            }
            nếu (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    trả_về out;
}

void V3Options::ccSet() {  // --cc
    m_outFormatOk = true;
    m_systemC = false;
}

void V3Options::decorations(FileLine* fl, const string& arg) {  // --decorations
    nếu (arg == "none") {
        m_decoration = false;
        m_decorationNodes = false;
    } không_thì nếu (arg == "node") {
        m_decoration = true;
        m_decorationNodes = true;
    } không_thì nếu (arg == "medium") {
        m_decoration = true;
        m_decorationNodes = false;
    } không_thì {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() const VL_MT_SAFE {
    std::vector<std::string> result;
    nếu (traceEnabledFst()) result.emplace_back("VerilatedFst");
    nếu (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    nếu (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    trả_về result;
}
std::vector<std::string> V3Options::traceClassLangs() const VL_MT_SAFE {
    std::vector<std::string> result;
    cho (const auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    trả_về result;
}
std::vector<std::string> V3Options::traceSourceBases() const VL_MT_SAFE {
    std::vector<std::string> result;
    nếu (traceEnabledFst()) result.emplace_back("verilated_fst");
    nếu (traceEnabledSaif()) result.emplace_back("verilated_saif");
    nếu (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    trả_về result;
}
std::vector<std::string> V3Options::traceSourceLangs() const VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    cho (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    trả_về result;
}
std::string V3Options::traceClassBase() const VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    trả_về traceClassBases().front();
}
std::string V3Options::traceClassLang() const VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    trả_về traceClassLangs().front();
}

//######################################################################
// File searching

bool V3Options::fileStatNormal(const string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    struct stat sstat;  // Stat information
    const số_nguyên err = stat(filename.c_str(), &sstat);
    nếu (err != 0) trả_về false;
    nếu (S_ISDIR(sstat.st_mode)) trả_về false;
    trả_về true;
}

string V3Options::fileExists(const string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    const string dir = V3Os::filenameDir(filename);
    const string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    nếu (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* const setp = &(diriter->second);

#ifdef _MSC_VER
        try {
            cho (const auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->insert(dirEntry.path().filename().string());
        } catch (std::filesystem::filesystem_error const& ex) {
            (void)ex;
            trả_về "";
        }
#không_thì
        nếu (DIR* const dirp = opendir(dir.c_str())) {
            trong_khi (struct dirent* direntp = readdir(dirp)) setp->insert(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    const std::set<string>* const filesetp = &(diriter->second);
    const auto fileiter = filesetp->find(basename);
    nếu (fileiter == filesetp->end()) trả_về "";  // Not found
    // Check if it is a directory, ignore if so
    const string filenameOut = V3Os::filenameJoin(dir, basename);
    nếu (!fileStatNormal(filenameOut)) trả_về "";  // Directory
    trả_về filenameOut;
}

string V3Options::filePathCheckOneDir(const string& modname, const string& dirname) {
    cho (const string& i : m_impp->m_libExtVs) {
        const string fn = V3Os::filenameJoin(dirname, modname + i);
        const string exists = fileExists(fn);
        nếu (exists != "") trả_về exists;
    }
    trả_về "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
số_nguyên V3Options::stripOptionsForChildRun(const string& opt, bool forTop) {
    // Options to strip for both the top wrapper, and hier blocks
    static const std::unordered_map<std::string, số_nguyên> commonOpts{
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
    nếu (commonOpts.count(opt)) trả_về commonOpts.at(opt);

    // Options to strip only for hier blocks
    nếu (!forTop) {
        static const std::unordered_map<std::string, số_nguyên> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        nếu (subOpts.count(opt)) trả_về subOpts.at(opt);
        nếu (VString::startsWith(opt, "G")) trả_về 1;
        nếu (VString::startsWith(opt, "pvalue")) trả_về 1;
    }

    // Do not strip
    trả_về 0;
}

void V3Options::validateIdentifier(FileLine* fl, const string& arg, const string& opt) {
    nếu (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(FileLine* fl, const string& modname, const string& lastpath,
                           const string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    const string filename = V3Os::filenameCleanup(VName::dehash(modname));
    nếu (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        const string exists = filePathCheckOneDir(filename, "");
        nếu (exists != "") trả_về exists;
    }
    cho (const string& dir : m_impp->m_incDirUsers) {
        const string exists = filePathCheckOneDir(filename, dir);
        nếu (exists != "") trả_về exists;
    }
    cho (const string& dir : m_impp->m_incDirFallbacks) {
        const string exists = filePathCheckOneDir(filename, dir);
        nếu (exists != "") trả_về exists;
    }

    nếu (m_relativeIncludes) {
        const string exists = filePathCheckOneDir(filename, lastpath);
        nếu (exists != "") trả_về V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    nếu (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << filePathLookedMsg(fl, filename));
    }
    trả_về "";
}

string V3Options::filePathLookedMsg(FileLine* /*fl*/, const string& modname) {
    static bool s_shown_notfound_msg = false;
    std::ostringstream ss;
    nếu (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } không_thì nếu (!s_shown_notfound_msg) {
        s_shown_notfound_msg = true;
        nếu (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        cho (const string& dir : m_impp->m_incDirUsers) {
            cho (const string& ext : m_impp->m_libExtVs) {
                const string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        cho (const string& dir : m_impp->m_incDirFallbacks) {
            cho (const string& ext : m_impp->m_libExtVs) {
                const string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    trả_về ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(const string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    nếu (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        trả_về V3LangCode::mostRecent();
    } không_thì nếu ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        const auto it = m_impp->m_langExts.find(ext);
        nếu (it != m_impp->m_langExts.end()) trả_về it->second;
    }
    trả_về m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(const string& var) {
    // If update below, also update V3Options::showVersion()
    nếu (var == "MAKE") {
        trả_về getenvMAKE();
    } không_thì nếu (var == "PERL") {
        trả_về getenvPERL();
    } không_thì nếu (var == "PYTHON3") {
        trả_về getenvPYTHON3();
    } không_thì nếu (var == "SYSTEMC") {
        trả_về getenvSYSTEMC();
    } không_thì nếu (var == "SYSTEMC_ARCH") {
        trả_về getenvSYSTEMC_ARCH();
    } không_thì nếu (var == "SYSTEMC_INCLUDE") {
        trả_về getenvSYSTEMC_INCLUDE();
    } không_thì nếu (var == "SYSTEMC_LIBDIR") {
        trả_về getenvSYSTEMC_LIBDIR();
    } không_thì nếu (var == "VERILATOR_ROOT") {
        trả_về getenvVERILATOR_ROOT();
    } không_thì {
        trả_về V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { trả_về V3Os::getenvStr("MAKE", "gmake"); }
#không_thì
string V3Options::getenvMAKE() { trả_về V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    trả_về V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    trả_về V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    trả_về V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_SYSTEMC};
    nếu (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    trả_về V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_SYSTEMC_ARCH};
    nếu (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    nếu (var == "") {
#nếu defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        const string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#elif defined(_WIN32)
        const string sysname = "WIN32";
        var = "win32";
#không_thì
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        struct utsname uts;
        uname(&uts);
        const string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        nếu (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } không_thì nếu (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } không_thì {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    trả_về var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    nếu (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    nếu (var == "") {
        const string sc = getenvSYSTEMC();
        nếu (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    trả_về V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    nếu (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    nếu (var == "") {
        const string sc = getenvSYSTEMC();
        const string arch = getenvSYSTEMC_ARCH();
        nếu (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    trả_về V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_ROOT() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_VERILATOR_ROOT};
    nếu (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    nếu (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    trả_về V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    const string defenv = string{DEFENV_VERILATOR_SOLVER};
    nếu (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    trả_về var;
}

string V3Options::getStdPackagePath() {
    trả_về V3Os::filenameJoin(getenvVERILATOR_ROOT(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    trả_về V3Os::filenameJoin(getenvVERILATOR_ROOT(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(const string& var) {
    // If update below, also update V3Options::showVersion()
    nếu (var == "COROUTINES" && coroutineSupport()) {
        trả_về "1";
    } không_thì nếu (var == "TSAN" && tsanSupport()) {
        trả_về "1";
    } không_thì nếu (var == "DEV_ASAN" && devAsan()) {
        trả_về "1";
    } không_thì nếu (var == "DEV_GCOV" && devGcov()) {
        trả_về "1";
        // cppcheck-suppress knownConditionTrueFalse
    } không_thì nếu (var == "SYSTEMC" && systemCFound()) {
        trả_về "1";
    } không_thì {
        trả_về "";
    }
}

bool V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    trả_về true;
#không_thì
    trả_về false;
#endif
}

bool V3Options::systemCFound() {
    trả_về (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

bool V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    trả_về true;
#không_thì
    trả_về false;
#endif
}

bool V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    trả_về true;
#không_thì
    trả_về false;
#endif
}

bool V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    trả_về true;
#không_thì
    trả_về false;
#endif
}

bool V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    trả_về true;
#không_thì
    trả_về false;
#endif
}

//######################################################################
// V3 Options notification methods

void V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    FileLine* const cmdfl = new FileLine{FileLine::commandLineFilename()};

    nếu (!outFormatOk() && v3Global.opt.main()) ccSet();  // --main implies --cc if not provided
    nếu (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    nếu (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    nếu (m_build) {
        nếu (m_binary)
            backendFlags.emplace_back("--binary");
        không_thì
            backendFlags.emplace_back("--build");
    }
    nếu (m_preprocOnly) backendFlags.emplace_back("-E");
    nếu (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    nếu (m_lintOnly) backendFlags.emplace_back("--lint-only");
    nếu (m_jsonOnly) backendFlags.emplace_back("--json-only");
    nếu (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        cho (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    nếu (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    nếu (!m_gmake && !m_makeJson) m_gmake = true;

    nếu (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    nếu (m_hierChild) {
        nếu (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = false;
    }

    nếu (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = true;  // No format, with --trace means wanted --trace-vcd
    }
    nếu (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = true;
    const số_nguyên ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    nếu (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    nếu (protectIds()) {
        nếu (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        nếu (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        nếu (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    nếu (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    nếu (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    nếu (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(true);

    // Default split limits if not specified
    nếu (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    nếu (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    nếu (v3Global.opt.main() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    nếu (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        nếu (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    nếu (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    nếu (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    nếu (!m_dumpLevel.count("tree") && m_dumpLevel.count("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    nếu (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    nếu (m_buildJobs == -1) m_buildJobs = 1;
    nếu (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    nếu (timing().isSetTrue()) V3PreShell::defineCmdLine("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    nếu (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(false);

    // === Leave last
    // Mark options as available
    m_available = true;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    trả_về ver;
}

string V3Options::protectKeyDefaulted() VL_MT_SAFE {
    static V3Mutex s_mutex;
    const V3LockGuard lock{s_mutex};
    nếu (m_protectKey.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protectKey = "VL-KEY-" + digest.digestBase64();
    }
    trả_về m_protectKey;
}

void V3Options::throwSigsegv() {  // LCOV_EXCL_START
#nếu !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<volatile char*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(const VTimescale& flag) const {
    nếu (!timeOverridePrec().isNone()) {
        trả_về timeOverridePrec();
    } không_thì nếu (flag.isNone()) {
        trả_về timeDefaultPrec();
    } không_thì {
        trả_về flag;
    }
}

VTimescale V3Options::timeComputeUnit(const VTimescale& flag) const {
    nếu (!timeOverrideUnit().isNone()) {
        trả_về timeOverrideUnit();
    } không_thì nếu (flag.isNone()) {
        trả_về timeDefaultUnit();
    } không_thì {
        trả_về flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(số_nguyên argc, char** argv) {
    // Return list of arguments as simple string
    string opts;
    cho (số_nguyên i = 0; i < argc; ++i) {
        nếu (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    trả_về opts;
}

//######################################################################
// V3 Options Parsing

void V3Options::parseOpts(FileLine* fl, số_nguyên argc, char** argv) VL_MT_DISABLED {
    // Save command line options
    cho (số_nguyên i = 0; i < argc; ++i) addLineArg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    nếu (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    nếu (prefix() == "" && topModule() != "") m_prefix = "V"s + topModule();  // Already encoded
    nếu (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + AstNode::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    nếu (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

bool V3Options::suffixed(const string& sw, const char* arg) {
    nếu (std::strlen(arg) > sw.length()) trả_về false;
    trả_về (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

void V3Options::parseOptsList(FileLine* fl, const string& optdir, số_nguyên argc,
                              char** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    const V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    const auto callStrSetter = [this](void (V3Options::*cbStr)(const string&)) {
        trả_về [this, cbStr](const string& v) { (this->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [this](const char* optp) VL_MT_DISABLED {
        addDefine(optp, true);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [this, &optdir](const char* optp) {
        string dirs = optp;
        string::size_type pos;
        trong_khi ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [this](const char* optp) {
        string exts = optp;
        string::size_type pos;
        trong_khi ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [this](const char* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](bool) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [this](bool flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](const char*) {
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
        nếu (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [this, fl](const char* valp) {
        số_nguyên val = std::atoi(valp);
        nếu (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } không_thì nếu (val == 0) {
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
    DECL_OPTION("-comp-limit-syms", CbVal, [](số_nguyên val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [this, fl](const char* valp) {
        nếu (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } không_thì nếu (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } không_thì nếu (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } không_thì {
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
    DECL_OPTION("-coverage-fsm-expand", CbVal, [this, fl](const char* const valp) {
        nếu (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } không_thì nếu (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } không_thì nếu (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } không_thì {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverageLine);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    const auto parseBinsLimit = [fl](const char* optp, const char* valp, uint32_t& limitr) {
        char* endp = nullptr;
        const unsigned long long value = std::strtoull(valp, &endp, 10);
        nếu (*endp || value < 1 || value > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            trả_về;
        }
        limitr = static_cast<uint32_t>(value);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [this, parseBinsLimit](const char* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [this, parseBinsLimit](const char* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [this](const char* valp) VL_MT_DISABLED {
        addDefine(valp, false);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [this]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [this](số_nguyên v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [this](const char* optp, const char* valp) {
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
    DECL_OPTION("-decorations", CbVal, [this, fl](const char* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [this, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [this](const char* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = true;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [this](const char* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [this](const char* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [this](const char* optp, const char* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [this](bool flag) {
        nếu (flag) {
            m_stdPackage = false;
            m_stdWaiver = false;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<void (*)(số_nguyên)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [this](const char* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [this, fl, &optdir](const char* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), true);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [this, &optdir](const char* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [this, fl, &optdir](const char* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), false);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [this](const char* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [this](const char* valp) { addFuture1(valp); });

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
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [this](const char* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [this](const char* optp) {  //
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
    DECL_OPTION("-fslice-element-limit", CbVal, [this, fl](const char* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        nếu (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [this](const char* optp) { addParameter(optp, false); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [this]() {
        cout << protectKeyDefaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](const char* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](const char* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [this](const char* valp) {
        const V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [this](const char* optp) { m_hierParamsFile.push_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [this, &optdir](const char* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [this, fl](số_nguyên val) {
        m_instrCountDpi = val;
        nếu (m_instrCountDpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [this](const char* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = true;
    });
    DECL_OPTION("-json-only-output", CbVal, [this](const char* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = true;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [this]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [this]() { m_l2Name = "v"; }).undocumented();  // Historical
    const auto setLang = [this, fl](const char* valp) {
        const V3LangCode optval{valp};
        nếu (optval.legal()) {
            m_defaultLanguage = optval;
        } không_thì {
            VSpellCheck spell;
            cho (số_nguyên i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.pushCandidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.bestCandidateMsg(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [this](const char* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [this, fl](const char* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [this](const char* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_mainTopName);
    DECL_OPTION("-make", CbVal, [this, fl](const char* valp) {
        nếu (!std::strcmp(valp, "gmake")) {
            m_gmake = true;
        } không_thì nếu (!std::strcmp(valp, "json")) {
            m_makeJson = true;
        } không_thì {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_maxNumWidth);
    DECL_OPTION("-mod-prefix", CbVal, [this, fl](const char* valp) {
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
    DECL_OPTION("-output-groups", CbVal, [this, fl](const char* valp) {
        m_outputGroups = std::atoi(valp);
        nếu (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [this, fl](const char* valp) {
        m_outputGroupsSize = std::atoll(valp);
        nếu (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [this, fl](const char* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        nếu (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [this, fl](const char* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        nếu (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preprocNoLine);
    DECL_OPTION("-pins64", CbCall, [this]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [this]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [this, fl](const char* valp) {
        m_pinsBv = std::atoi(valp);
        nếu (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [this](bool flag) {
        m_pinsScUint = flag;
        nếu (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [this](bool flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [this](bool flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [this, fl](const char* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [this, fl](const char* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        nếu (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
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
    DECL_OPTION("-protect-lib", CbVal, [this, fl](const char* valp) {
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
                [this](const char* varp) { addParameter(varp, false); });

    DECL_OPTION("-quiet", CbOnOff, [this](bool flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [this, fl](const char* valp) {
        m_reloopLimit = std::atoi(valp);
        nếu (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [this, fl](const char* valp) {
        m_replicationLimit = std::atoi(valp);
        nếu (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
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
    DECL_OPTION("-threads", CbVal, [this, fl](const char* valp) {
        m_threads = std::atoi(valp);
        nếu (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        nếu (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [this, fl](const char* valp) {
        m_hierThreads = std::atoi(valp);
        nếu (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [this, fl](const char* valp) {
        nếu (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = true;
            m_threadsDpiUnpure = true;
        } không_thì nếu (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = false;
            m_threadsDpiUnpure = false;
        } không_thì nếu (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = true;
            m_threadsDpiUnpure = false;
        } không_thì {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [this, fl](const char* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        nếu (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [this, fl](const char* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        nếu (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        nếu (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [this, fl](const char* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, true);
        nếu (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        nếu (!prec.isNone()) {
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
    DECL_OPTION("-trace-threads", CbVal, [fl](const char*) {
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
    DECL_OPTION("-v", CbVal, [this, &optdir](const char* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [this, fl](const char* valp) {
        số_nguyên val = std::atoi(valp);
        nếu (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } không_thì nếu (val == 0) {
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
    DECL_OPTION("-Werror-", CbPartialMatch, [this, fl](const char* optp) {
        const V3ErrorCode code{optp};
        nếu (code == V3ErrorCode::EC_ERROR) {
            nếu (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } không_thì {
            V3Error::pretendError(code, true);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [this](const char* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](const char* optp) VL_MT_DISABLED {
        const string err = FileLine::globalWarnOffParse(optp, true);
        nếu (!err.empty()) {
            const string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    cho (số_nguyên i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        cho (const string prefix : {"-Wno-", "-Wwarn-"})
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
    DECL_OPTION("-Wwarn-", CbPartialMatch, [this, fl, &parser](const char* optp) VL_MT_DISABLED {
        const V3ErrorCode code{optp};
        nếu (code == V3ErrorCode::EC_ERROR) {
            nếu (!isFuture(optp)) {
                const string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } không_thì {
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

    DECL_OPTION("-x-assign", CbVal, [this, fl](const char* valp) {
        nếu (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } không_thì nếu (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } không_thì nếu (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } không_thì nếu (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } không_thì {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [this, fl](const char* valp) {
        nếu (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } không_thì nếu (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } không_thì nếu (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } không_thì {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [this, &optdir](const char* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    cho (số_nguyên i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        nếu (argv[i][0] == '-' || argv[i][0] == '+') {
            const std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            số_nguyên consumed = 0;
            bool isForRerun = true;

            // Special case for -j which has an optional argument
            nếu (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                số_nguyên val = 0;
                nếu (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    nếu (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                nếu (m_buildJobs == -1) m_buildJobs = val;
                nếu (m_verilateJobs == -1) m_verilateJobs = val;
                nếu (m_outputGroups == -1) m_outputGroups = val;
            } không_thì {
                const auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                nếu (consumed) {
                    // Already processed
                } không_thì nếu (isFuture0(argName)) {
                    consumed = 1;
                } không_thì nếu (isFuture1(argName)) {
                    consumed = 2;
                } không_thì {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            nếu (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            continue;
        }

        // Filename
        const std::string filename = parseFileArg(optdir, argv[i]);
        bool isForRerun = true;
        nếu (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } không_thì nếu (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } không_thì nếu (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = false;
        } không_thì {
            V3Options::addVFile(filename, work());
            isForRerun = false;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    nếu (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

void V3Options::parseOptsFile(FileLine* fl, const string& filename, bool rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    const std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    nếu (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        trả_về;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    bool has_fargs = false;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    const auto parse_fargs_cmt = [&](const char* p) -> void {
        const bool blockCmt = p[1] == '*';
        p += 2;
        trong_khi (std::isspace(*p)) ++p;
        nếu (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            nếu (std::isspace(*p)) {
                trong_khi (std::isspace(*p)) ++p;
                nếu (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    nếu (std::isspace(*p)) {
                        trong_khi (std::isspace(*p)) ++p;
                        nếu (!has_fargs) {
                            has_fargs = true;
                            whole_file.clear();
                        }
                        whole_file += p;
                        nếu (blockCmt) {
                            const std::string::size_type pos = whole_file.find("*/");
                            nếu (pos == std::string::npos) {
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
    trong_khi (!ifp->eof()) {
        const std::string line = V3Os::getline(*ifp);
        // Strip simple comments
        char lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        bool space_begin = true;  // At beginning or leading spaces only
        cho (const char* p = line.c_str(); *p; lastch = *p++) {
            nếu (inCmt) {
                nếu (p[0] == '*' && p[1] == '/') {
                    inCmt = false;
                    ++p;
                }
            } không_thì nếu (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                break;  // Ignore to EOL
            } không_thì nếu (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                break;  // Ignore to EOL
            } không_thì nếu (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = true;
                space_begin = false;
                ++p;
            } không_thì {
                nếu (!std::isspace(*p)) space_begin = false;
                nếu (!has_fargs) whole_file += *p;
            }
        }
        nếu (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    nếu (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = new FileLine{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    enum state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    cho (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        char curr_char = whole_file[pos];
        switch (st) {
        case ST_IN_OPTION:  // Get all chars up to a white space or a "="
            nếu (std::isspace(curr_char)) {  // End of option
                nếu (!arg.empty()) {  // End of word
                    args.push_back(arg);
                }
                arg = "";
                break;
            }
            nếu (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                break;
            }
            nếu (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                nếu (pos < whole_file.length()) curr_char = whole_file[pos];
                nếu (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } không_thì {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                break;
            }
            nếu (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                break;
            }
            arg += curr_char;
            break;
        case ST_IN_QUOTED_STR:  // Just store all chars inside string
            nếu (curr_char != '\'') {
                arg += curr_char;
            } không_thì {  // End of quoted string
                st = ST_IN_OPTION;
            }
            break;
        case ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            nếu (curr_char != '"') {
                nếu (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } không_thì {
                    arg += curr_char;
                }
            } không_thì {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            break;
        case ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            break;
        }
    }
    nếu (!arg.empty()) {  // Add last word
        args.push_back(arg);
    }

    // Path
    const string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    cho (const string& i : args) argv.push_back(const_cast<char*>(i.c_str()));
    argv.push_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<số_nguyên>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(const string& optdir, const string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    nếu (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    trả_về filename;
}

//======================================================================

void V3Options::showVersion(bool verbose) {
    cout << version();
    cout << "\n";
    nếu (!verbose) trả_về;

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

void V3Options::setDebugMode(số_nguyên level) {
    V3Error::debugDefault(level);
    nếu (!m_dumpLevel.count("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = true;
    m_debugCheck = true;
    nếu (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(const string& tag) const VL_MT_SAFE {
    const auto iter = m_debugLevel.find(tag);
    trả_về iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(const string& srcfile_path) const VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    trả_về debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(const string& tag) const VL_MT_SAFE {
    const auto iter = m_dumpLevel.find(tag);
    trả_về iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(const string& srcfile_path) const VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    trả_về dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

bool V3Options::dumpTreeAddrids() const VL_MT_SAFE {
    static số_nguyên s_level = -1;
    nếu (VL_UNLIKELY(s_level < 0)) {
        const unsigned value = dumpLevel("tree-addrids");
        nếu (!available()) trả_về value > 0;
        s_level = static_cast<unsigned>(value);
    }
    trả_về s_level > 0;
}

void V3Options::optimize(số_nguyên level) {
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
    nếu (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
