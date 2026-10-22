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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3Options.h"

#અન્તર્ભાવ "V3Container.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Mutex.h"
#અન્તર્ભાવ "V3OptionParser.h"
#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3PreShell.h"
#અન્તર્ભાવ "V3String.h"

// clang-format off
#અન્તર્ભાવ <sys/types.h>
#અન્તર્ભાવ <sys/stat.h>
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#અન્તર્ભાવ <algorithm>
#અન્તર્ભાવ <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#નહીંતર
# include <dirent.h>
#endif
#અન્તર્ભાવ <fcntl.h>
#અન્તર્ભાવ <list>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <thread>
#અન્તર્ભાવ <set>
#અન્તર્ભાવ <string>

#અન્તર્ભાવ "config_rev.h"

#જો defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

વર્ગ V3OptionsImp final {
જાહેર:
    // TYPES
    using DirMap = std::map<અચલ string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_લીટી_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, તાર્કિક>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<અચલ string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    શૂન્ય addIncDirUser(અચલ string& incdir) {
        અચલ string& dir = V3Os::filenameCleanup(incdir);
        અચલ તાર્કિક inserted = m_incDirUsers.ઉમેરો(dir);
        જો (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    શૂન્ય addIncDirFallback(અચલ string& incdir) {
        અચલ string& dir = V3Os::filenameCleanup(incdir);
        જો (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            અચલ auto itFoundPair = m_incDirFallbackSet.ઉમેરો(dir);
            જો (itFoundPair.second) m_incDirFallbacks.ધકેલો_back(dir);
        }
    }
    શૂન્ય addLangExt(અચલ string& langext, અચલ V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        જો (addext[0] == '.') addext = addext.substr(1);
        (શૂન્ય)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    શૂન્ય addLibExtV(અચલ string& libext) { m_libExtVs.ઉમેરો(libext); }
    V3OptionsImp() = મૂળભૂત;
    ~V3OptionsImp() = મૂળભૂત;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(અચલ અક્ષર* textp) {
    // Return code for given string, or ERROR, which is a bad code
    માટે (પૂર્ણાંક codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        અચલ V3LangCode code{codei};
        જો (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            પાછા;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(અચલ string& કિંમત, તાર્કિક& badr)
    : m_e{VTimescale::NONE} {
    badr = સાચું;
    અચલ string spaceless = VString::removeWhitespace(કિંમત);
    માટે (પૂર્ણાંક i = TS_100S; i < _ENUM_END; ++i) {
        અચલ VTimescale ts{i};
        જો (spaceless == ts.ascii()) {
            badr = ખોટું;
            m_e = ts.m_e;
            તોડો;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(અચલ string& opts) {
    VStringList vals;
    તાર્કિક inStr = ખોટું;
    string cur;
    સ્થિર અચલ string hierBlock("--hierarchical-block");
    File_લીટી cmdfl(File_લીટી::command_લીટી_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    માટે (string::const_iterator it = opts.begin(); it != opts.end();) {
        જો (inStr) {
            જો (*it == '\\') {
                ++it;
                જો (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    તોડો;
                }
                જો (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    તોડો;
                }
                cur.ધકેલો_back(*it);
                ++it;
            } નહીંતર જો (*it == '"') {  // end of string
                cur.ધકેલો_back(*it);
                vals.ધકેલો_back(cur);
                cur.clear();
                ++it;
                જો (it != opts.end()) {
                    જો (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        તોડો;
                    }
                    ++it;
                    જો (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        તોડો;
                    }
                    inStr = *it == '"';
                    cur.ધકેલો_back(*it);
                    ++it;
                }
            } નહીંતર {
                cur.ધકેલો_back(*it);
                ++it;
            }
        } નહીંતર {
            જો (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                તોડો;
            }
            જો (*it == ',') {  // end of this parameter
                vals.ધકેલો_back(cur);
                cur.clear();
                ++it;
                જો (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    તોડો;
                }
                inStr = *it == '"';
            }
            cur.ધકેલો_back(*it);
            ++it;
        }
    }
    જો (!cur.empty()) vals.ધકેલો_back(cur);
    જો (vals.size() >= 2) {
        જો (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } નહીંતર {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    માટે (size_t i = 2; i + 1 < vals.size(); i += 2) {
        અચલ તાર્કિક inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        જો (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

શૂન્ય VTimescale::parseSlashed(File_લીટી* fl, અચલ અક્ષર* textp, VTimescale& unitr,
                              VTimescale& precr, તાર્કિક allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    અચલ અક્ષર* cp = textp;
    માટે (; std::isspace(*cp); ++cp) {}
    અચલ અક્ષર* અચલ unitp = cp;
    માટે (; *cp && *cp != '/'; ++cp) {}
    અચલ string unitStr(unitp, cp - unitp);
    માટે (; std::isspace(*cp); ++cp) {}
    string precStr;
    જો (*cp == '/') {
        ++cp;
        માટે (; std::isspace(*cp); ++cp) {}
        અચલ અક્ષર* અચલ precp = cp;
        માટે (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    માટે (; std::isspace(*cp); ++cp) {}
    જો (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        પાછા;
    }

    તાર્કિક unitbad = ખોટું;
    અચલ VTimescale unit{unitStr, unitbad /*ref*/};
    જો (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        પાછા;
    }
    unitr = unit;

    જો (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        તાર્કિક precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        જો (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            પાછા;
        }
        જો (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            પાછા;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

શૂન્ય V3Options::addIncDirUser(અચલ string& incdir) { m_impp->addIncDirUser(incdir); }
શૂન્ય V3Options::addIncDirFallback(અચલ string& incdir) { m_impp->addIncDirFallback(incdir); }
શૂન્ય V3Options::addLangExt(અચલ string& langext, અચલ V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
શૂન્ય V3Options::addLibExtV(અચલ string& libext) { m_impp->addLibExtV(libext); }
શૂન્ય V3Options::addDefine(અચલ string& defline, તાર્કિક allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ડાબું = defline;
    જ્યારે (ડાબું != "") {
        string def = ડાબું;
        string::size_type pos;
        જો (allowPlus && ((pos = ડાબું.find('+')) != string::npos)) {
            ડાબું = ડાબું.substr(pos + 1);
            def.erase(pos);
        } નહીંતર {
            ડાબું = "";
        }
        string કિંમત;
        જો ((pos = def.find('=')) != string::npos) {
            કિંમત = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_લીટી(def, કિંમત);
    }
}
શૂન્ય V3Options::addParameter(અચલ string& paramline, તાર્કિક allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ડાબું = paramline;
    જ્યારે (ડાબું != "") {
        string param = ડાબું;
        string::size_type pos;
        જો (allowPlus && ((pos = ડાબું.find('+')) != string::npos)) {
            ડાબું = ડાબું.substr(pos + 1);
            param.erase(pos);
        } નહીંતર {
            ડાબું = "";
        }
        string કિંમત;
        જો ((pos = param.find('=')) != string::npos) {
            કિંમત = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << કિંમત);
        (શૂન્ય)m_parameters.erase(param);
        m_parameters[param] = કિંમત;
    }
}

તાર્કિક V3Options::hasParameter(અચલ string& name) {
    પાછા m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(અચલ string& name) {
    string કિંમત = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    પાછા કિંમત;
}

શૂન્ય V3Options::checkParameters() {
    જો (!m_parameters.empty()) {
        std::stringstream સંદેશ;
        સંદેશ << "Parameters from the command line were not found in the design:";
        માટે (અચલ auto& i : m_parameters) સંદેશ << " " << i.first;
        v3error(સંદેશ.str());
    }
}

શૂન્ય V3Options::addCppFile(અચલ string& filename) { m_cppFiles.ઉમેરો(filename); }
શૂન્ય V3Options::addCFlags(અચલ string& filename) { m_cFlags.ધકેલો_back(filename); }
શૂન્ય V3Options::addLibMapFile(અચલ string& filename) { m_libmapFiles.ઉમેરો(filename); }
શૂન્ય V3Options::addCompilerIncludes(અચલ string& filename) {
    m_compilerIncludes.ઉમેરો(filename);
}
શૂન્ય V3Options::addLdLibFile(અચલ string& filename) {
    m_ldLibFiles.ધકેલો_back(filename);
    addLdLibs(filename);
}
શૂન્ય V3Options::addLdLibs(અચલ string& filename) { m_ldLibs.ધકેલો_back(filename); }
શૂન્ય V3Options::addMakeFlags(અચલ string& filename) { m_makeFlags.ધકેલો_back(filename); }
શૂન્ય V3Options::addFuture(અચલ string& flag) { m_futures.ઉમેરો(flag); }
શૂન્ય V3Options::addFuture0(અચલ string& flag) { m_future0s.ઉમેરો(flag); }
શૂન્ય V3Options::addFuture1(અચલ string& flag) { m_future1s.ઉમેરો(flag); }
તાર્કિક V3Options::isFuture(અચલ string& flag) અચલ {
    પાછા m_futures.find(flag) != m_futures.end();
}
તાર્કિક V3Options::isFuture0(અચલ string& flag) અચલ {
    પાછા m_future0s.find(flag) != m_future0s.end();
}
તાર્કિક V3Options::isFuture1(અચલ string& flag) અચલ {
    પાછા m_future1s.find(flag) != m_future1s.end();
}
તાર્કિક V3Options::isLibraryFile(અચલ string& filename, અચલ string& libname) અચલ {
    પાછા m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
શૂન્ય V3Options::addLibraryFile(અચલ string& filename, અચલ string& libname) {
    m_libraryFiles.ઉમેરો({filename, libname});
}
શૂન્ય V3Options::addVFile(અચલ string& filename, અચલ string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ધકેલો_back({filename, libname});  // Not emplace
}
શૂન્ય V3Options::addVltFile(અચલ string& filename, અચલ string& libname) {
    m_vltFiles.ઉમેરો({filename, libname});
}
શૂન્ય V3Options::addForceInc(અચલ string& filename) { m_forceIncs.ધકેલો_back(filename); }

શૂન્ય V3Options::add_લીટી_Arg(અચલ string& arg) { m_impp->m_લીટી_Args.ધકેલો_back(arg); }

શૂન્ય V3Options::addArg(અક્ષર** argv, size_t ગણતરી, તાર્કિક isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, તાર્કિક>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    માટે (size_t n = 0; n < ગણતરી; ++n) pair.first.emplace_back(argv[n]);
}

શૂન્ય V3Options::addArg(અચલ std::string& arg, તાર્કિક isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, તાર્કિક>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

અચલ std::list<std::pair<std::list<std::string>, તાર્કિક>>& V3Options::allArgs() અચલ {
    પાછા m_impp->m_allArgs;
}

string V3Options::allArgsString() અચલ VL_MT_SAFE {
    string result;
    માટે (અચલ auto& pair : m_impp->m_allArgs) {
        માટે (અચલ string& arg : pair.first) {
            જો (!result.empty()) result += " ";
            result += arg;
        }
    }
    પાછા result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(તાર્કિક for_ટોચ) અચલ {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    માટે (અચલ auto& vFile : m_vFiles) vFiles.ઉમેરો(vFile.filename());
    string out;
    તાર્કિક stripArg = ખોટું;
    તાર્કિક strip_Arg_If_અંક = ખોટું;
    માટે (અચલ auto& pair : m_impp->m_allArgs) {
        માટે (અચલ string& arg : pair.first) {
            જો (stripArg) {
                stripArg = ખોટું;
                ચાલુ;
            }
            જો (strip_Arg_If_અંક) {
                strip_Arg_If_અંક = ખોટું;
                જો (isdigit(arg[0])) ચાલુ;
            }
            પૂર્ણાંક skip = 0;
            જો (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } નહીંતર જો (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            જો (skip > 0) {  // arg is an option
                અચલ string opt = arg.substr(skip);  // Remove '-' in the beginning
                અચલ પૂર્ણાંક અંક_Strip = stripOptionsForChildRun(opt, for_ટોચ);
                જો (અંક_Strip) {
                    UASSERT(0 <= અંક_Strip && અંક_Strip <= 3, "should be one of 0, 1, 2, 3");
                    જો (અંક_Strip == 2) stripArg = સાચું;
                    જો (અંક_Strip == 3) strip_Arg_If_અંક = સાચું;
                    ચાલુ;
                }
            } નહીંતર {  // Not an option
                જો (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    ચાલુ;
                }
            }
            જો (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    પાછા out;
}

શૂન્ય V3Options::ccSet() {  // --cc
    m_outFormatOk = સાચું;
    m_systemC = ખોટું;
}

શૂન્ય V3Options::decorations(File_લીટી* fl, અચલ string& arg) {  // --decorations
    જો (arg == "none") {
        m_decoration = ખોટું;
        m_decorationNodes = ખોટું;
    } નહીંતર જો (arg == "node") {
        m_decoration = સાચું;
        m_decorationNodes = સાચું;
    } નહીંતર જો (arg == "medium") {
        m_decoration = સાચું;
        m_decorationNodes = ખોટું;
    } નહીંતર {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() અચલ VL_MT_SAFE {
    std::vector<std::string> result;
    જો (traceEnabledFst()) result.emplace_back("VerilatedFst");
    જો (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    જો (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    પાછા result;
}
std::vector<std::string> V3Options::traceClassLangs() અચલ VL_MT_SAFE {
    std::vector<std::string> result;
    માટે (અચલ auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    પાછા result;
}
std::vector<std::string> V3Options::traceSourceBases() અચલ VL_MT_SAFE {
    std::vector<std::string> result;
    જો (traceEnabledFst()) result.emplace_back("verilated_fst");
    જો (traceEnabledSaif()) result.emplace_back("verilated_saif");
    જો (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    પાછા result;
}
std::vector<std::string> V3Options::traceSourceLangs() અચલ VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    માટે (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    પાછા result;
}
std::string V3Options::traceClassBase() અચલ VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    પાછા traceClassBases().front();
}
std::string V3Options::traceClassLang() અચલ VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    પાછા traceClassLangs().front();
}

//######################################################################
// File searching

તાર્કિક V3Options::fileStatNormal(અચલ string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    સંરચના stat sstat;  // Stat information
    અચલ પૂર્ણાંક err = stat(filename.c_str(), &sstat);
    જો (err != 0) પાછા ખોટું;
    જો (S_ISDIR(sstat.st_mode)) પાછા ખોટું;
    પાછા સાચું;
}

string V3Options::fileExists(અચલ string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    અચલ string dir = V3Os::filenameDir(filename);
    અચલ string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    જો (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* અચલ setp = &(diriter->second);

#ifdef _MSC_VER
        પ્રયાસ {
            માટે (અચલ auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->ઉમેરો(dirEntry.path().filename().string());
        } પકડો (std::filesystem::filesystem_error અચલ& ex) {
            (શૂન્ય)ex;
            પાછા "";
        }
#નહીંતર
        જો (DIR* અચલ dirp = opendir(dir.c_str())) {
            જ્યારે (સંરચના dirent* direntp = readdir(dirp)) setp->ઉમેરો(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    અચલ std::set<string>* અચલ filesetp = &(diriter->second);
    અચલ auto fileiter = filesetp->find(basename);
    જો (fileiter == filesetp->end()) પાછા "";  // Not found
    // Check if it is a directory, ignore if so
    અચલ string filenameOut = V3Os::filenameJoin(dir, basename);
    જો (!fileStatNormal(filenameOut)) પાછા "";  // Directory
    પાછા filenameOut;
}

string V3Options::filePathCheckOneDir(અચલ string& modname, અચલ string& dirname) {
    માટે (અચલ string& i : m_impp->m_libExtVs) {
        અચલ string fn = V3Os::filenameJoin(dirname, modname + i);
        અચલ string exists = fileExists(fn);
        જો (exists != "") પાછા exists;
    }
    પાછા "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
પૂર્ણાંક V3Options::stripOptionsForChildRun(અચલ string& opt, તાર્કિક for_ટોચ) {
    // Options to strip for both the top wrapper, and hier blocks
    સ્થિર અચલ std::unordered_map<std::string, પૂર્ણાંક> commonOpts{
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
    જો (commonOpts.ગણતરી(opt)) પાછા commonOpts.at(opt);

    // Options to strip only for hier blocks
    જો (!for_ટોચ) {
        સ્થિર અચલ std::unordered_map<std::string, પૂર્ણાંક> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        જો (subOpts.ગણતરી(opt)) પાછા subOpts.at(opt);
        જો (VString::startsWith(opt, "G")) પાછા 1;
        જો (VString::startsWith(opt, "pvalue")) પાછા 1;
    }

    // Do not strip
    પાછા 0;
}

શૂન્ય V3Options::validateIdentifier(File_લીટી* fl, અચલ string& arg, અચલ string& opt) {
    જો (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_લીટી* fl, અચલ string& modname, અચલ string& lastpath,
                           અચલ string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    અચલ string filename = V3Os::filenameCleanup(VName::dehash(modname));
    જો (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        અચલ string exists = filePathCheckOneDir(filename, "");
        જો (exists != "") પાછા exists;
    }
    માટે (અચલ string& dir : m_impp->m_incDirUsers) {
        અચલ string exists = filePathCheckOneDir(filename, dir);
        જો (exists != "") પાછા exists;
    }
    માટે (અચલ string& dir : m_impp->m_incDirFallbacks) {
        અચલ string exists = filePathCheckOneDir(filename, dir);
        જો (exists != "") પાછા exists;
    }

    જો (m_relativeIncludes) {
        અચલ string exists = filePathCheckOneDir(filename, lastpath);
        જો (exists != "") પાછા V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    જો (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_સંદેશ(fl, filename));
    }
    પાછા "";
}

string V3Options::file_Path_Looked_સંદેશ(File_લીટી* /*fl*/, અચલ string& modname) {
    સ્થિર તાર્કિક s_shown_notfound_સંદેશ = ખોટું;
    std::ostringstream ss;
    જો (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } નહીંતર જો (!s_shown_notfound_સંદેશ) {
        s_shown_notfound_સંદેશ = સાચું;
        જો (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        માટે (અચલ string& dir : m_impp->m_incDirUsers) {
            માટે (અચલ string& ext : m_impp->m_libExtVs) {
                અચલ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        માટે (અચલ string& dir : m_impp->m_incDirFallbacks) {
            માટે (અચલ string& ext : m_impp->m_libExtVs) {
                અચલ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    પાછા ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(અચલ string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    જો (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        પાછા V3LangCode::mostRecent();
    } નહીંતર જો ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        અચલ auto it = m_impp->m_langExts.find(ext);
        જો (it != m_impp->m_langExts.end()) પાછા it->second;
    }
    પાછા m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(અચલ string& var) {
    // If update below, also update V3Options::showVersion()
    જો (var == "MAKE") {
        પાછા getenvMAKE();
    } નહીંતર જો (var == "PERL") {
        પાછા getenvPERL();
    } નહીંતર જો (var == "PYTHON3") {
        પાછા getenvPYTHON3();
    } નહીંતર જો (var == "SYSTEMC") {
        પાછા getenvSYSTEMC();
    } નહીંતર જો (var == "SYSTEMC_ARCH") {
        પાછા getenvSYSTEMC_ARCH();
    } નહીંતર જો (var == "SYSTEMC_INCLUDE") {
        પાછા getenvSYSTEMC_INCLUDE();
    } નહીંતર જો (var == "SYSTEMC_LIBDIR") {
        પાછા getenvSYSTEMC_LIBDIR();
    } નહીંતર જો (var == "VERILATOR_ROOT") {
        પાછા getenv_VERILATOR_મૂળ();
    } નહીંતર {
        પાછા V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { પાછા V3Os::getenvStr("MAKE", "gmake"); }
#નહીંતર
string V3Options::getenvMAKE() { પાછા V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    પાછા V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    પાછા V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    પાછા V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    અચલ string defenv = string{DEFENV_SYSTEMC};
    જો (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    પાછા V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    અચલ string defenv = string{DEFENV_SYSTEMC_ARCH};
    જો (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    જો (var == "") {
#જો defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        અચલ string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#અથવાજો defined(_WIN32)
        અચલ string sysname = "WIN32";
        var = "win32";
#નહીંતર
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        સંરચના utsname uts;
        uname(&uts);
        અચલ string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        જો (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } નહીંતર જો (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } નહીંતર {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    પાછા var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    અચલ string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    જો (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    જો (var == "") {
        અચલ string sc = getenvSYSTEMC();
        જો (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    પાછા V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    અચલ string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    જો (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    જો (var == "") {
        અચલ string sc = getenvSYSTEMC();
        અચલ string arch = getenvSYSTEMC_ARCH();
        જો (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    પાછા V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_મૂળ() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    અચલ string defenv = string{DEFENV_VERILATOR_મૂળ};
    જો (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    જો (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    પાછા V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    અચલ string defenv = string{DEFENV_VERILATOR_SOLVER};
    જો (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    પાછા var;
}

string V3Options::getStdPackagePath() {
    પાછા V3Os::filenameJoin(getenv_VERILATOR_મૂળ(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    પાછા V3Os::filenameJoin(getenv_VERILATOR_મૂળ(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(અચલ string& var) {
    // If update below, also update V3Options::showVersion()
    જો (var == "COROUTINES" && coroutineSupport()) {
        પાછા "1";
    } નહીંતર જો (var == "TSAN" && tsanSupport()) {
        પાછા "1";
    } નહીંતર જો (var == "DEV_ASAN" && devAsan()) {
        પાછા "1";
    } નહીંતર જો (var == "DEV_GCOV" && devGcov()) {
        પાછા "1";
        // cppcheck-suppress knownConditionTrueFalse
    } નહીંતર જો (var == "SYSTEMC" && systemCFound()) {
        પાછા "1";
    } નહીંતર {
        પાછા "";
    }
}

તાર્કિક V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    પાછા સાચું;
#નહીંતર
    પાછા ખોટું;
#endif
}

તાર્કિક V3Options::systemCFound() {
    પાછા (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

તાર્કિક V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    પાછા સાચું;
#નહીંતર
    પાછા ખોટું;
#endif
}

તાર્કિક V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    પાછા સાચું;
#નહીંતર
    પાછા ખોટું;
#endif
}

તાર્કિક V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    પાછા સાચું;
#નહીંતર
    પાછા ખોટું;
#endif
}

તાર્કિક V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    પાછા સાચું;
#નહીંતર
    પાછા ખોટું;
#endif
}

//######################################################################
// V3 Options notification methods

શૂન્ય V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_લીટી* અચલ cmdfl = નવું File_લીટી{File_લીટી::command_લીટી_Filename()};

    જો (!outFormatOk() && v3Global.opt.મુખ્ય()) ccSet();  // --main implies --cc if not provided
    જો (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    જો (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    જો (m_build) {
        જો (m_દ્વિઅંકી)
            backendFlags.emplace_back("--binary");
        નહીંતર
            backendFlags.emplace_back("--build");
    }
    જો (m_preprocOnly) backendFlags.emplace_back("-E");
    જો (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    જો (m_lintOnly) backendFlags.emplace_back("--lint-only");
    જો (m_jsonOnly) backendFlags.emplace_back("--json-only");
    જો (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        માટે (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    જો (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    જો (!m_gmake && !m_makeJson) m_gmake = સાચું;

    જો (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    જો (m_hierChild) {
        જો (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = ખોટું;
    }

    જો (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = સાચું;  // No format, with --trace means wanted --trace-vcd
    }
    જો (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = સાચું;
    અચલ પૂર્ણાંક ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    જો (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    જો (protectIds()) {
        જો (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        જો (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        જો (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    જો (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    જો (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    જો (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(સાચું);

    // Default split limits if not specified
    જો (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    જો (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    જો (v3Global.opt.મુખ્ય() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    જો (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        જો (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    જો (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    જો (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    જો (!m_dumpLevel.ગણતરી("tree") && m_dumpLevel.ગણતરી("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    જો (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    જો (m_buildJobs == -1) m_buildJobs = 1;
    જો (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    જો (timing().isSetTrue()) V3PreShell::define_Cmd_લીટી("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    જો (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(ખોટું);

    // === Leave last
    // Mark options as available
    m_available = સાચું;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    પાછા ver;
}

string V3Options::protect_ચાવી_Defaulted() VL_MT_SAFE {
    સ્થિર V3Mutex s_mutex;
    અચલ V3LockGuard lock{s_mutex};
    જો (m_protect_ચાવી.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_ચાવી = "VL-KEY-" + digest.digestBase64();
    }
    પાછા m_protect_ચાવી;
}

શૂન્ય V3Options::throwSigsegv() {  // LCOV_EXCL_START
#જો !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<અસ્થિર અક્ષર*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(અચલ VTimescale& flag) અચલ {
    જો (!timeOverridePrec().isNone()) {
        પાછા timeOverridePrec();
    } નહીંતર જો (flag.isNone()) {
        પાછા timeDefaultPrec();
    } નહીંતર {
        પાછા flag;
    }
}

VTimescale V3Options::timeComputeUnit(અચલ VTimescale& flag) અચલ {
    જો (!timeOverrideUnit().isNone()) {
        પાછા timeOverrideUnit();
    } નહીંતર જો (flag.isNone()) {
        પાછા timeDefaultUnit();
    } નહીંતર {
        પાછા flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(પૂર્ણાંક argc, અક્ષર** argv) {
    // Return list of arguments as simple string
    string opts;
    માટે (પૂર્ણાંક i = 0; i < argc; ++i) {
        જો (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    પાછા opts;
}

//######################################################################
// V3 Options Parsing

શૂન્ય V3Options::parseOpts(File_લીટી* fl, પૂર્ણાંક argc, અક્ષર** argv) VL_MT_DISABLED {
    // Save command line options
    માટે (પૂર્ણાંક i = 0; i < argc; ++i) add_લીટી_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    જો (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    જો (prefix() == "" && ટોચ_Module() != "") m_prefix = "V"s + ટોચ_Module();  // Already encoded
    જો (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + Ast_ગાંઠ::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    જો (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

તાર્કિક V3Options::suffixed(અચલ string& sw, અચલ અક્ષર* arg) {
    જો (std::strlen(arg) > sw.length()) પાછા ખોટું;
    પાછા (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

શૂન્ય V3Options::parseOptsList(File_લીટી* fl, અચલ string& optdir, પૂર્ણાંક argc,
                              અક્ષર** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    અચલ V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    અચલ auto callStrSetter = [આ](શૂન્ય (V3Options::*cbStr)(અચલ string&)) {
        પાછા [આ, cbStr](અચલ string& v) { (આ->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [આ](અચલ અક્ષર* optp) VL_MT_DISABLED {
        addDefine(optp, સાચું);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [આ, &optdir](અચલ અક્ષર* optp) {
        string dirs = optp;
        string::size_type pos;
        જ્યારે ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        string exts = optp;
        string::size_type pos;
        જ્યારે ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](તાર્કિક) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [આ](તાર્કિક flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](અચલ અક્ષર*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [આ](તાર્કિક flag) {
        m_bboxUnsup = flag;
        File_લીટી::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, સાચું);
    });
    DECL_OPTION("-binary", CbCall, [આ]() {
        m_દ્વિઅંકી = સાચું;
        m_build = સાચું;
        m_exe = સાચું;
        m_main = સાચું;
        જો (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        પૂર્ણાંક val = std::atoi(valp);
        જો (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } નહીંતર જો (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [આ]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](અચલ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](અચલ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](પૂર્ણાંક val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        જો (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } નહીંતર જો (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } નહીંતર જો (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } નહીંતર {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [આ](તાર્કિક flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_પદાવલિ);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_પદાવલિ_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [આ, fl](અચલ અક્ષર* અચલ valp) {
        જો (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } નહીંતર જો (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } નહીંતર જો (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } નહીંતર {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_લીટી);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    અચલ auto parseBinsLimit = [fl](અચલ અક્ષર* optp, અચલ અક્ષર* valp, uint32_t& limitr) {
        અક્ષર* endp = nullptr;
        અચલ unsigned long long કિંમત = std::strtoull(valp, &endp, 10);
        જો (*endp || કિંમત < 1 || કિંમત > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            પાછા;
        }
        limitr = static_cast<uint32_t>(કિંમત);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [આ, parseBinsLimit](અચલ અક્ષર* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [આ, parseBinsLimit](અચલ અક્ષર* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [આ](અચલ અક્ષર* valp) VL_MT_DISABLED {
        addDefine(valp, ખોટું);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [આ]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [આ](પૂર્ણાંક v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [આ](અચલ અક્ષર* optp, અચલ અક્ષર* valp) {
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_ઢગલો_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [આ, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [આ, fl](અચલ અક્ષર* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [આ, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [આ](અચલ અક્ષર* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = સાચું;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [આ](અચલ અક્ષર* optp, અચલ અક્ષર* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [આ](તાર્કિક flag) {
        જો (flag) {
            m_stdPackage = ખોટું;
            m_stdWaiver = ખોટું;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<શૂન્ય (*)(પૂર્ણાંક)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [આ](અચલ અક્ષર* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [આ, fl, &optdir](અચલ અક્ષર* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), સાચું);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [આ, &optdir](અચલ અક્ષર* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [આ, fl, &optdir](અચલ અક્ષર* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ખોટું);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [આ](અચલ અક્ષર* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [આ](અચલ અક્ષર* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [આ](તાર્કિક flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_વૃક્ષ = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_વૃક્ષ);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_વૃક્ષ);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [આ](તાર્કિક flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [આ](તાર્કિક flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](તાર્કિક) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [આ](અચલ અક્ષર* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [આ](અચલ અક્ષર* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](તાર્કિક) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](તાર્કિક) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ધકેલો_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [આ, fl](તાર્કિક flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [આ](તાર્કિક flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [આ](તાર્કિક flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](તાર્કિક) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](તાર્કિક) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        જો (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](તાર્કિક) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [આ](અચલ અક્ષર* optp) { addParameter(optp, ખોટું); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [આ]() {
        cout << protect_ચાવી_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](અચલ અક્ષર* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](અચલ અક્ષર* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [આ](અચલ અક્ષર* valp) {
        અચલ V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [આ](અચલ અક્ષર* optp) { m_hierParamsFile.ધકેલો_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [આ, &optdir](અચલ અક્ષર* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [આ, fl](પૂર્ણાંક val) {
        m_instr_ગણતરી_Dpi = val;
        જો (m_instr_ગણતરી_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [આ](અચલ અક્ષર* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = સાચું;
    });
    DECL_OPTION("-json-only-output", CbVal, [આ](અચલ અક્ષર* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = સાચું;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [આ]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [આ]() { m_l2Name = "v"; }).undocumented();  // Historical
    અચલ auto setLang = [આ, fl](અચલ અક્ષર* valp) {
        અચલ V3LangCode optval{valp};
        જો (optval.legal()) {
            m_defaultLanguage = optval;
        } નહીંતર {
            VSpellCheck spell;
            માટે (પૂર્ણાંક i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.ધકેલો_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_સંદેશ(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [આ](અચલ અક્ષર* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [આ](અચલ અક્ષર* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_ટોચ_Name);
    DECL_OPTION("-make", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        જો (!std::strcmp(valp, "gmake")) {
            m_gmake = સાચું;
        } નહીંતર જો (!std::strcmp(valp, "json")) {
            m_makeJson = સાચું;
        } નહીંતર {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_અંક_Width);
    DECL_OPTION("-mod-prefix", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [આ]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [આ]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [આ]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [આ]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](તાર્કિક /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_outputGroups = std::atoi(valp);
        જો (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_outputGroupsSize = std::atoll(valp);
        જો (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        જો (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        જો (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_લીટી);
    DECL_OPTION("-pins64", CbCall, [આ]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [આ]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_pinsBv = std::atoi(valp);
        જો (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [આ](તાર્કિક flag) {
        m_pinsScUint = flag;
        જો (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [આ](તાર્કિક flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [આ](તાર્કિક flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        જો (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [આ]() { m_public = ખોટું; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [આ]() { m_profC = m_profCFuncs = સાચું; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [આ]() {
        m_profC = m_profCFuncs = સાચું;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_ચાવી);
    DECL_OPTION("-protect-lib", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = સાચું;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [આ](તાર્કિક flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(સાચું);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [આ](તાર્કિક flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [આ](તાર્કિક flag) {
        m_publicParams = flag;
        v3Global.dpi(સાચું);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [આ](અચલ અક્ષર* varp) { addParameter(varp, ખોટું); });

    DECL_OPTION("-quiet", CbOnOff, [આ](તાર્કિક flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_reloopLimit = std::atoi(valp);
        જો (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_replicationLimit = std::atoi(valp);
        જો (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [આ, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [આ]() {
        m_outFormatOk = સાચું;
        m_systemC = સાચું;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [આ](તાર્કિક flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [આ](તાર્કિક flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [આ]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [આ, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_threads = std::atoi(valp);
        જો (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        જો (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_hierThreads = std::atoi(valp);
        જો (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        જો (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = સાચું;
            m_threadsDpiUnpure = સાચું;
        } નહીંતર જો (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = ખોટું;
            m_threadsDpiUnpure = ખોટું;
        } નહીંતર જો (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = સાચું;
            m_threadsDpiUnpure = ખોટું;
        } નહીંતર {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        જો (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        જો (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        જો (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, સાચું);
        જો (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        જો (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [આ](અચલ std::string& flag) { m_ટોચ_Module = Ast_ગાંઠ::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [આ](અચલ std::string& flag) { m_ટોચ_Module = Ast_ગાંઠ::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_ટોચ_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [આ]() { m_traceEnabledSaif = સાચું; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [આ]() {
        m_traceEnabledFst = સાચું;
        addLdLibs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", Set, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", Set, &m_traceMaxWidth);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](અચલ અક્ષર*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_ટોચ);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [આ]() { m_traceEnabledVcd = સાચું; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_ગણતરી).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [આ]() {
        showVersion(સાચું);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [આ, &optdir](અચલ અક્ષર* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        પૂર્ણાંક val = std::atoi(valp);
        જો (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } નહીંતર જો (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [આ]() {
        showVersion(ખોટું);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_લીટી::globalWarnOff(V3ErrorCode::I_LINT, ખોટું); });
    DECL_OPTION("-Werror-", CbPartialMatch, [આ, fl](અચલ અક્ષર* optp) {
        અચલ V3ErrorCode code{optp};
        જો (code == V3ErrorCode::EC_ERROR) {
            જો (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } નહીંતર {
            V3Error::pretendError(code, સાચું);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [આ](અચલ અક્ષર* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](અચલ અક્ષર* optp) VL_MT_DISABLED {
        અચલ string err = File_લીટી::globalWarnOffParse(optp, સાચું);
        જો (!err.empty()) {
            અચલ string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    માટે (પૂર્ણાંક i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        માટે (અચલ string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [આ]() { m_context = ખોટું; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(ખોટું); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_લીટી::globalWarnOff(V3ErrorCode::I_LINT, સાચું); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_લીટી::globalWarnOff(V3ErrorCode::I_STYLE, સાચું); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [આ]() {
        m_pedantic = સાચું;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, ખોટું);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [આ, fl, &parser](અચલ અક્ષર* optp) VL_MT_DISABLED {
        અચલ V3ErrorCode code{optp};
        જો (code == V3ErrorCode::EC_ERROR) {
            જો (!isFuture(optp)) {
                અચલ string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } નહીંતર {
            File_લીટી::globalWarnOff(code, ખોટું);
            V3Error::pretendError(code, ખોટું);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_લીટી::globalWarnOff(V3ErrorCode::I_LINT, ખોટું); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_લીટી::globalWarnOff(V3ErrorCode::I_STYLE, ખોટું); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        જો (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } નહીંતર જો (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } નહીંતર જો (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } નહીંતર જો (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } નહીંતર {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [આ, fl](અચલ અક્ષર* valp) {
        જો (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } નહીંતર જો (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } નહીંતર જો (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } નહીંતર {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [આ, &optdir](અચલ અક્ષર* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    માટે (પૂર્ણાંક i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        જો (argv[i][0] == '-' || argv[i][0] == '+') {
            અચલ std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            પૂર્ણાંક consumed = 0;
            તાર્કિક isForRerun = સાચું;

            // Special case for -j which has an optional argument
            જો (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                પૂર્ણાંક val = 0;
                જો (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    જો (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                જો (m_buildJobs == -1) m_buildJobs = val;
                જો (m_verilateJobs == -1) m_verilateJobs = val;
                જો (m_outputGroups == -1) m_outputGroups = val;
            } નહીંતર {
                અચલ auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                જો (consumed) {
                    // Already processed
                } નહીંતર જો (isFuture0(argName)) {
                    consumed = 1;
                } નહીંતર જો (isFuture1(argName)) {
                    consumed = 2;
                } નહીંતર {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            જો (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            ચાલુ;
        }

        // Filename
        અચલ std::string filename = parseFileArg(optdir, argv[i]);
        તાર્કિક isForRerun = સાચું;
        જો (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } નહીંતર જો (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } નહીંતર જો (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = ખોટું;
        } નહીંતર {
            V3Options::addVFile(filename, work());
            isForRerun = ખોટું;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    જો (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

શૂન્ય V3Options::parseOptsFile(File_લીટી* fl, અચલ string& filename, તાર્કિક rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    અચલ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    જો (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        પાછા;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    તાર્કિક has_fargs = ખોટું;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    અચલ auto parse_fargs_cmt = [&](અચલ અક્ષર* p) -> શૂન્ય {
        અચલ તાર્કિક blockCmt = p[1] == '*';
        p += 2;
        જ્યારે (std::isspace(*p)) ++p;
        જો (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            જો (std::isspace(*p)) {
                જ્યારે (std::isspace(*p)) ++p;
                જો (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    જો (std::isspace(*p)) {
                        જ્યારે (std::isspace(*p)) ++p;
                        જો (!has_fargs) {
                            has_fargs = સાચું;
                            whole_file.clear();
                        }
                        whole_file += p;
                        જો (blockCmt) {
                            અચલ std::string::size_type pos = whole_file.find("*/");
                            જો (pos == std::string::npos) {
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

    તાર્કિક inCmt = ખોટું;
    જ્યારે (!ifp->eof()) {
        અચલ std::string લીટી = V3Os::getline(*ifp);
        // Strip simple comments
        અક્ષર lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        તાર્કિક space_begin = સાચું;  // At beginning or leading spaces only
        માટે (અચલ અક્ષર* p = લીટી.c_str(); *p; lastch = *p++) {
            જો (inCmt) {
                જો (p[0] == '*' && p[1] == '/') {
                    inCmt = ખોટું;
                    ++p;
                }
            } નહીંતર જો (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                તોડો;  // Ignore to EOL
            } નહીંતર જો (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                તોડો;  // Ignore to EOL
            } નહીંતર જો (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = સાચું;
                space_begin = ખોટું;
                ++p;
            } નહીંતર {
                જો (!std::isspace(*p)) space_begin = ખોટું;
                જો (!has_fargs) whole_file += *p;
            }
        }
        જો (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    જો (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = નવું File_લીટી{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    ગણના state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    માટે (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        અક્ષર curr_char = whole_file[pos];
        પસંદગી (st) {
        સ્થિતિ ST_IN_OPTION:  // Get all chars up to a white space or a "="
            જો (std::isspace(curr_char)) {  // End of option
                જો (!arg.empty()) {  // End of word
                    args.ધકેલો_back(arg);
                }
                arg = "";
                તોડો;
            }
            જો (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                તોડો;
            }
            જો (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                જો (pos < whole_file.length()) curr_char = whole_file[pos];
                જો (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } નહીંતર {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                તોડો;
            }
            જો (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                તોડો;
            }
            arg += curr_char;
            તોડો;
        સ્થિતિ ST_IN_QUOTED_STR:  // Just store all chars inside string
            જો (curr_char != '\'') {
                arg += curr_char;
            } નહીંતર {  // End of quoted string
                st = ST_IN_OPTION;
            }
            તોડો;
        સ્થિતિ ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            જો (curr_char != '"') {
                જો (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } નહીંતર {
                    arg += curr_char;
                }
            } નહીંતર {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            તોડો;
        સ્થિતિ ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            તોડો;
        }
    }
    જો (!arg.empty()) {  // Add last word
        args.ધકેલો_back(arg);
    }

    // Path
    અચલ string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<અક્ષર*> argv;
    argv.reserve(args.size() + 1);
    માટે (અચલ string& i : args) argv.ધકેલો_back(const_cast<અક્ષર*>(i.c_str()));
    argv.ધકેલો_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<પૂર્ણાંક>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(અચલ string& optdir, અચલ string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    જો (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    પાછા filename;
}

//======================================================================

શૂન્ય V3Options::showVersion(તાર્કિક verbose) {
    cout << version();
    cout << "\n";
    જો (!verbose) પાછા;

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
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_મૂળ << "\n";
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
    m_impp = નવું V3OptionsImp;

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

શૂન્ય V3Options::setDebugMode(પૂર્ણાંક level) {
    V3Error::debugDefault(level);
    જો (!m_dumpLevel.ગણતરી("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = સાચું;
    m_debugCheck = સાચું;
    જો (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(અચલ string& tag) અચલ VL_MT_SAFE {
    અચલ auto iter = m_debugLevel.find(tag);
    પાછા iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(અચલ string& srcfile_path) અચલ VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    પાછા debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(અચલ string& tag) અચલ VL_MT_SAFE {
    અચલ auto iter = m_dumpLevel.find(tag);
    પાછા iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(અચલ string& srcfile_path) અચલ VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    પાછા dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

તાર્કિક V3Options::dump_વૃક્ષ_Addrids() અચલ VL_MT_SAFE {
    સ્થિર પૂર્ણાંક s_level = -1;
    જો (VL_UNLIKELY(s_level < 0)) {
        અચલ unsigned કિંમત = dumpLevel("tree-addrids");
        જો (!available()) પાછા કિંમત > 0;
        s_level = static_cast<unsigned>(કિંમત);
    }
    પાછા s_level > 0;
}

શૂન્ય V3Options::optimize(પૂર્ણાંક level) {
    // Set all optimizations to on/off
    અચલ તાર્કિક flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_વૃક્ષ = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_વૃક્ષ = flag;
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
    જો (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
