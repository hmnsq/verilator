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

#समावेश "V3PchAstMT.h"

#समावेश "V3Options.h"

#समावेश "V3Container.h"
#समावेश "V3Error.h"
#समावेश "V3File.h"
#समावेश "V3Global.h"
#समावेश "V3Mutex.h"
#समावेश "V3OptionParser.h"
#समावेश "V3Os.h"
#समावेश "V3PreShell.h"
#समावेश "V3String.h"

// clang-format off
#समावेश <sys/types.h>
#समावेश <sys/stat.h>
#अगर_अघोषित _WIN32
# include <sys/utsname.h>
#पूर्ण_अगर
#समावेश <algorithm>
#समावेश <cctype>
#अगर_घोषित _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#अन्यथा
# include <dirent.h>
#पूर्ण_अगर
#समावेश <fcntl.h>
#समावेश <list>
#समावेश <map>
#समावेश <memory>
#समावेश <thread>
#समावेश <set>
#समावेश <string>

#समावेश "config_rev.h"

#अगर defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#पूर्ण_अगर
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

श्रेणी V3OptionsImp final {
खुला:
    // TYPES
    using DirMap = std::map<स्थिर string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_पंक्ति_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, bool>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<स्थिर string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    व्योम addIncDirUser(स्थिर string& incdir) {
        स्थिर string& dir = V3Os::filenameCleanup(incdir);
        स्थिर bool inserted = m_incDirUsers.जोड़ो(dir);
        अगर (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.हटाओ(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    व्योम addIncDirFallback(स्थिर string& incdir) {
        स्थिर string& dir = V3Os::filenameCleanup(incdir);
        अगर (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            स्थिर स्वतः itFoundPair = m_incDirFallbackSet.जोड़ो(dir);
            अगर (itFoundPair.second) m_incDirFallbacks.धकेलो_back(dir);
        }
    }
    व्योम addLangExt(स्थिर string& langext, स्थिर V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        अगर (addext[0] == '.') addext = addext.substr(1);
        (व्योम)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    व्योम addLibExtV(स्थिर string& libext) { m_libExtVs.जोड़ो(libext); }
    V3OptionsImp() = शेष;
    ~V3OptionsImp() = शेष;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(स्थिर अक्षर* textp) {
    // Return code for given string, or ERROR, which is a bad code
    क्रम (पूर्णांक codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        स्थिर V3LangCode code{codei};
        अगर (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            वापस;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(स्थिर string& मूल्य, bool& badr)
    : m_e{VTimescale::NONE} {
    badr = true;
    स्थिर string spaceless = VString::removeWhitespace(मूल्य);
    क्रम (पूर्णांक i = TS_100S; i < _ENUM_END; ++i) {
        स्थिर VTimescale ts{i};
        अगर (spaceless == ts.ascii()) {
            badr = false;
            m_e = ts.m_e;
            अवरोध;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(स्थिर string& opts) {
    VStringList vals;
    bool inStr = false;
    string cur;
    अटल स्थिर string hierBlock("--hierarchical-block");
    File_पंक्ति cmdfl(File_पंक्ति::command_पंक्ति_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    क्रम (string::const_iterator it = opts.begin(); it != opts.end();) {
        अगर (inStr) {
            अगर (*it == '\\') {
                ++it;
                अगर (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    अवरोध;
                }
                अगर (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    अवरोध;
                }
                cur.धकेलो_back(*it);
                ++it;
            } अन्यथा अगर (*it == '"') {  // end of string
                cur.धकेलो_back(*it);
                vals.धकेलो_back(cur);
                cur.सफा();
                ++it;
                अगर (it != opts.end()) {
                    अगर (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        अवरोध;
                    }
                    ++it;
                    अगर (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        अवरोध;
                    }
                    inStr = *it == '"';
                    cur.धकेलो_back(*it);
                    ++it;
                }
            } अन्यथा {
                cur.धकेलो_back(*it);
                ++it;
            }
        } अन्यथा {
            अगर (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                अवरोध;
            }
            अगर (*it == ',') {  // end of this parameter
                vals.धकेलो_back(cur);
                cur.सफा();
                ++it;
                अगर (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    अवरोध;
                }
                inStr = *it == '"';
            }
            cur.धकेलो_back(*it);
            ++it;
        }
    }
    अगर (!cur.empty()) vals.धकेलो_back(cur);
    अगर (vals.आकार() >= 2) {
        अगर (vals.आकार() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } अन्यथा {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    क्रम (माप_प्रकार i = 2; i + 1 < vals.आकार(); i += 2) {
        स्थिर bool inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        अगर (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

व्योम VTimescale::parseSlashed(File_पंक्ति* fl, स्थिर अक्षर* textp, VTimescale& unitr,
                              VTimescale& precr, bool allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    स्थिर अक्षर* cp = textp;
    क्रम (; std::है_खाली(*cp); ++cp) {}
    स्थिर अक्षर* स्थिर unitp = cp;
    क्रम (; *cp && *cp != '/'; ++cp) {}
    स्थिर string unitStr(unitp, cp - unitp);
    क्रम (; std::है_खाली(*cp); ++cp) {}
    string precStr;
    अगर (*cp == '/') {
        ++cp;
        क्रम (; std::है_खाली(*cp); ++cp) {}
        स्थिर अक्षर* स्थिर precp = cp;
        क्रम (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    क्रम (; std::है_खाली(*cp); ++cp) {}
    अगर (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        वापस;
    }

    bool unitbad = false;
    स्थिर VTimescale unit{unitStr, unitbad /*ref*/};
    अगर (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        वापस;
    }
    unitr = unit;

    अगर (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        bool precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        अगर (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            वापस;
        }
        अगर (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            वापस;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

व्योम V3Options::addIncDirUser(स्थिर string& incdir) { m_impp->addIncDirUser(incdir); }
व्योम V3Options::addIncDirFallback(स्थिर string& incdir) { m_impp->addIncDirFallback(incdir); }
व्योम V3Options::addLangExt(स्थिर string& langext, स्थिर V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
व्योम V3Options::addLibExtV(स्थिर string& libext) { m_impp->addLibExtV(libext); }
व्योम V3Options::addDefine(स्थिर string& defline, bool allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string बाँए = defline;
    जबतक (बाँए != "") {
        string def = बाँए;
        string::आकार_type pos;
        अगर (allowPlus && ((pos = बाँए.find('+')) != string::npos)) {
            बाँए = बाँए.substr(pos + 1);
            def.erase(pos);
        } अन्यथा {
            बाँए = "";
        }
        string मूल्य;
        अगर ((pos = def.find('=')) != string::npos) {
            मूल्य = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_पंक्ति(def, मूल्य);
    }
}
व्योम V3Options::addParameter(स्थिर string& paramline, bool allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string बाँए = paramline;
    जबतक (बाँए != "") {
        string param = बाँए;
        string::आकार_type pos;
        अगर (allowPlus && ((pos = बाँए.find('+')) != string::npos)) {
            बाँए = बाँए.substr(pos + 1);
            param.erase(pos);
        } अन्यथा {
            बाँए = "";
        }
        string मूल्य;
        अगर ((pos = param.find('=')) != string::npos) {
            मूल्य = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << मूल्य);
        (व्योम)m_parameters.erase(param);
        m_parameters[param] = मूल्य;
    }
}

bool V3Options::hasParameter(स्थिर string& name) {
    वापस m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(स्थिर string& name) {
    string मूल्य = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    वापस मूल्य;
}

व्योम V3Options::checkParameters() {
    अगर (!m_parameters.empty()) {
        std::stringstream संदेश;
        संदेश << "Parameters from the command line were not found in the design:";
        क्रम (स्थिर स्वतः& i : m_parameters) संदेश << " " << i.first;
        v3error(संदेश.str());
    }
}

व्योम V3Options::addCppFile(स्थिर string& filename) { m_cppFiles.जोड़ो(filename); }
व्योम V3Options::addCFlags(स्थिर string& filename) { m_cFlags.धकेलो_back(filename); }
व्योम V3Options::addLibMapFile(स्थिर string& filename) { m_libmapFiles.जोड़ो(filename); }
व्योम V3Options::addCompilerIncludes(स्थिर string& filename) {
    m_compilerIncludes.जोड़ो(filename);
}
व्योम V3Options::addLdLibFile(स्थिर string& filename) {
    m_ldLibFiles.धकेलो_back(filename);
    addLdLibs(filename);
}
व्योम V3Options::addLdLibs(स्थिर string& filename) { m_ldLibs.धकेलो_back(filename); }
व्योम V3Options::addMakeFlags(स्थिर string& filename) { m_makeFlags.धकेलो_back(filename); }
व्योम V3Options::addFuture(स्थिर string& flag) { m_futures.जोड़ो(flag); }
व्योम V3Options::addFuture0(स्थिर string& flag) { m_future0s.जोड़ो(flag); }
व्योम V3Options::addFuture1(स्थिर string& flag) { m_future1s.जोड़ो(flag); }
bool V3Options::isFuture(स्थिर string& flag) स्थिर {
    वापस m_futures.find(flag) != m_futures.end();
}
bool V3Options::isFuture0(स्थिर string& flag) स्थिर {
    वापस m_future0s.find(flag) != m_future0s.end();
}
bool V3Options::isFuture1(स्थिर string& flag) स्थिर {
    वापस m_future1s.find(flag) != m_future1s.end();
}
bool V3Options::isLibraryFile(स्थिर string& filename, स्थिर string& libname) स्थिर {
    वापस m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
व्योम V3Options::addLibraryFile(स्थिर string& filename, स्थिर string& libname) {
    m_libraryFiles.जोड़ो({filename, libname});
}
व्योम V3Options::addVFile(स्थिर string& filename, स्थिर string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.धकेलो_back({filename, libname});  // Not emplace
}
व्योम V3Options::addVltFile(स्थिर string& filename, स्थिर string& libname) {
    m_vltFiles.जोड़ो({filename, libname});
}
व्योम V3Options::addForceInc(स्थिर string& filename) { m_forceIncs.धकेलो_back(filename); }

व्योम V3Options::add_पंक्ति_Arg(स्थिर string& arg) { m_impp->m_पंक्ति_Args.धकेलो_back(arg); }

व्योम V3Options::addArg(अक्षर** argv, माप_प्रकार गिनती, bool isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, bool>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    क्रम (माप_प्रकार n = 0; n < गिनती; ++n) pair.first.emplace_back(argv[n]);
}

व्योम V3Options::addArg(स्थिर std::string& arg, bool isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, bool>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

स्थिर std::list<std::pair<std::list<std::string>, bool>>& V3Options::allArgs() स्थिर {
    वापस m_impp->m_allArgs;
}

string V3Options::allArgsString() स्थिर VL_MT_SAFE {
    string परिणाम;
    क्रम (स्थिर स्वतः& pair : m_impp->m_allArgs) {
        क्रम (स्थिर string& arg : pair.first) {
            अगर (!परिणाम.empty()) परिणाम += " ";
            परिणाम += arg;
        }
    }
    वापस परिणाम;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(bool for_शीर्ष) स्थिर {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    क्रम (स्थिर स्वतः& vFile : m_vFiles) vFiles.जोड़ो(vFile.filename());
    string out;
    bool stripArg = false;
    bool strip_Arg_If_अंक = false;
    क्रम (स्थिर स्वतः& pair : m_impp->m_allArgs) {
        क्रम (स्थिर string& arg : pair.first) {
            अगर (stripArg) {
                stripArg = false;
                जारी;
            }
            अगर (strip_Arg_If_अंक) {
                strip_Arg_If_अंक = false;
                अगर (है_अंक(arg[0])) जारी;
            }
            पूर्णांक skip = 0;
            अगर (arg.लंबाई() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } अन्यथा अगर (arg.लंबाई() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            अगर (skip > 0) {  // arg is an option
                स्थिर string opt = arg.substr(skip);  // Remove '-' in the beginning
                स्थिर पूर्णांक अंक_Strip = stripOptionsForChildRun(opt, for_शीर्ष);
                अगर (अंक_Strip) {
                    UASSERT(0 <= अंक_Strip && अंक_Strip <= 3, "should be one of 0, 1, 2, 3");
                    अगर (अंक_Strip == 2) stripArg = true;
                    अगर (अंक_Strip == 3) strip_Arg_If_अंक = true;
                    जारी;
                }
            } अन्यथा {  // Not an option
                अगर (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    जारी;
                }
            }
            अगर (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    वापस out;
}

व्योम V3Options::ccSet() {  // --cc
    m_outFormatOk = true;
    m_systemC = false;
}

व्योम V3Options::decorations(File_पंक्ति* fl, स्थिर string& arg) {  // --decorations
    अगर (arg == "none") {
        m_decoration = false;
        m_decorationNodes = false;
    } अन्यथा अगर (arg == "node") {
        m_decoration = true;
        m_decorationNodes = true;
    } अन्यथा अगर (arg == "medium") {
        m_decoration = true;
        m_decorationNodes = false;
    } अन्यथा {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() स्थिर VL_MT_SAFE {
    std::vector<std::string> परिणाम;
    अगर (traceEnabledFst()) परिणाम.emplace_back("VerilatedFst");
    अगर (traceEnabledSaif()) परिणाम.emplace_back("VerilatedSaif");
    अगर (traceEnabledVcd()) परिणाम.emplace_back("VerilatedVcd");
    वापस परिणाम;
}
std::vector<std::string> V3Options::traceClassLangs() स्थिर VL_MT_SAFE {
    std::vector<std::string> परिणाम;
    क्रम (स्थिर स्वतः& cbase : traceClassBases())
        परिणाम.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    वापस परिणाम;
}
std::vector<std::string> V3Options::traceSourceBases() स्थिर VL_MT_SAFE {
    std::vector<std::string> परिणाम;
    अगर (traceEnabledFst()) परिणाम.emplace_back("verilated_fst");
    अगर (traceEnabledSaif()) परिणाम.emplace_back("verilated_saif");
    अगर (traceEnabledVcd()) परिणाम.emplace_back("verilated_vcd");
    वापस परिणाम;
}
std::vector<std::string> V3Options::traceSourceLangs() स्थिर VL_MT_SAFE {
    std::vector<std::string> परिणाम = traceSourceBases();
    क्रम (std::string& str : परिणाम) str += systemC() ? "_sc"s : "_c"s;
    वापस परिणाम;
}
std::string V3Options::traceClassBase() स्थिर VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    वापस traceClassBases().front();
}
std::string V3Options::traceClassLang() स्थिर VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    वापस traceClassLangs().front();
}

//######################################################################
// File searching

bool V3Options::fileStatNormal(स्थिर string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    काष्ठा stat sstat;  // Stat information
    स्थिर पूर्णांक err = stat(filename.c_str(), &sstat);
    अगर (err != 0) वापस false;
    अगर (S_ISDIR(sstat.st_mode)) वापस false;
    वापस true;
}

string V3Options::fileExists(स्थिर string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    स्थिर string dir = V3Os::filenameDir(filename);
    स्थिर string basename = V3Os::filenameNonDir(filename);

    स्वतः diriter = m_impp->m_dirMap.find(dir);
    अगर (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* स्थिर setp = &(diriter->second);

#अगर_घोषित _MSC_VER
        प्रयास {
            क्रम (स्थिर स्वतः& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->जोड़ो(dirEntry.path().filename().string());
        } पकड़ो (std::filesystem::filesystem_error स्थिर& ex) {
            (व्योम)ex;
            वापस "";
        }
#अन्यथा
        अगर (सूची* स्थिर dirp = सूची_खोलो(dir.c_str())) {
            जबतक (काष्ठा dirent* direntp = सूची_पढ़ो(dirp)) setp->जोड़ो(direntp->d_name);
            बंद_सूची(dirp);
        }
#पूर्ण_अगर
    }
    // Find it
    स्थिर std::set<string>* स्थिर filesetp = &(diriter->second);
    स्थिर स्वतः fileiter = filesetp->find(basename);
    अगर (fileiter == filesetp->end()) वापस "";  // Not found
    // Check if it is a directory, ignore if so
    स्थिर string filenameOut = V3Os::filenameJoin(dir, basename);
    अगर (!fileStatNormal(filenameOut)) वापस "";  // Directory
    वापस filenameOut;
}

string V3Options::filePathCheckOneDir(स्थिर string& modname, स्थिर string& स_नाम) {
    क्रम (स्थिर string& i : m_impp->m_libExtVs) {
        स्थिर string fn = V3Os::filenameJoin(स_नाम, modname + i);
        स्थिर string exists = fileExists(fn);
        अगर (exists != "") वापस exists;
    }
    वापस "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
पूर्णांक V3Options::stripOptionsForChildRun(स्थिर string& opt, bool for_शीर्ष) {
    // Options to strip for both the top wrapper, and hier blocks
    अटल स्थिर std::unordered_map<std::string, पूर्णांक> commonOpts{
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
    अगर (commonOpts.गिनती(opt)) वापस commonOpts.at(opt);

    // Options to strip only for hier blocks
    अगर (!for_शीर्ष) {
        अटल स्थिर std::unordered_map<std::string, पूर्णांक> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        अगर (subOpts.गिनती(opt)) वापस subOpts.at(opt);
        अगर (VString::startsWith(opt, "G")) वापस 1;
        अगर (VString::startsWith(opt, "pvalue")) वापस 1;
    }

    // Do not strip
    वापस 0;
}

व्योम V3Options::validateIdentifier(File_पंक्ति* fl, स्थिर string& arg, स्थिर string& opt) {
    अगर (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_पंक्ति* fl, स्थिर string& modname, स्थिर string& lastpath,
                           स्थिर string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    स्थिर string filename = V3Os::filenameCleanup(VName::dehash(modname));
    अगर (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        स्थिर string exists = filePathCheckOneDir(filename, "");
        अगर (exists != "") वापस exists;
    }
    क्रम (स्थिर string& dir : m_impp->m_incDirUsers) {
        स्थिर string exists = filePathCheckOneDir(filename, dir);
        अगर (exists != "") वापस exists;
    }
    क्रम (स्थिर string& dir : m_impp->m_incDirFallbacks) {
        स्थिर string exists = filePathCheckOneDir(filename, dir);
        अगर (exists != "") वापस exists;
    }

    अगर (m_relativeIncludes) {
        स्थिर string exists = filePathCheckOneDir(filename, lastpath);
        अगर (exists != "") वापस V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    अगर (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_संदेश(fl, filename));
    }
    वापस "";
}

string V3Options::file_Path_Looked_संदेश(File_पंक्ति* /*fl*/, स्थिर string& modname) {
    अटल bool s_shown_notfound_संदेश = false;
    std::ostringstream ss;
    अगर (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } अन्यथा अगर (!s_shown_notfound_संदेश) {
        s_shown_notfound_संदेश = true;
        अगर (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        क्रम (स्थिर string& dir : m_impp->m_incDirUsers) {
            क्रम (स्थिर string& ext : m_impp->m_libExtVs) {
                स्थिर string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        क्रम (स्थिर string& dir : m_impp->m_incDirFallbacks) {
            क्रम (स्थिर string& ext : m_impp->m_libExtVs) {
                स्थिर string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    वापस ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(स्थिर string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::आकार_type pos;
    अगर (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        वापस V3LangCode::mostRecent();
    } अन्यथा अगर ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        स्थिर स्वतः it = m_impp->m_langExts.find(ext);
        अगर (it != m_impp->m_langExts.end()) वापस it->second;
    }
    वापस m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(स्थिर string& var) {
    // If update below, also update V3Options::showVersion()
    अगर (var == "MAKE") {
        वापस getenvMAKE();
    } अन्यथा अगर (var == "PERL") {
        वापस getenvPERL();
    } अन्यथा अगर (var == "PYTHON3") {
        वापस getenvPYTHON3();
    } अन्यथा अगर (var == "SYSTEMC") {
        वापस getenvSYSTEMC();
    } अन्यथा अगर (var == "SYSTEMC_ARCH") {
        वापस getenvSYSTEMC_ARCH();
    } अन्यथा अगर (var == "SYSTEMC_INCLUDE") {
        वापस getenvSYSTEMC_INCLUDE();
    } अन्यथा अगर (var == "SYSTEMC_LIBDIR") {
        वापस getenvSYSTEMC_LIBDIR();
    } अन्यथा अगर (var == "VERILATOR_ROOT") {
        वापस getenv_VERILATOR_मूल();
    } अन्यथा {
        वापस V3Os::getenvStr(var, "");
    }
}

#अगर_घोषित __FreeBSD__
string V3Options::getenvMAKE() { वापस V3Os::getenvStr("MAKE", "gmake"); }
#अन्यथा
string V3Options::getenvMAKE() { वापस V3Os::getenvStr("MAKE", "make"); }
#पूर्ण_अगर

string V3Options::getenvMAKEFLAGS() {  //
    वापस V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    वापस V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    वापस V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    स्थिर string defenv = string{DEFENV_SYSTEMC};
    अगर (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    वापस V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    स्थिर string defenv = string{DEFENV_SYSTEMC_ARCH};
    अगर (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    अगर (var == "") {
#अगर defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        स्थिर string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#या_अगर defined(_WIN32)
        स्थिर string sysname = "WIN32";
        var = "win32";
#अन्यथा
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        काष्ठा utsname uts;
        uname(&uts);
        स्थिर string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        अगर (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } अन्यथा अगर (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } अन्यथा {
            var = "linux";
        }
#पूर्ण_अगर
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    वापस var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    स्थिर string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    अगर (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    अगर (var == "") {
        स्थिर string sc = getenvSYSTEMC();
        अगर (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    वापस V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    स्थिर string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    अगर (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    अगर (var == "") {
        स्थिर string sc = getenvSYSTEMC();
        स्थिर string arch = getenvSYSTEMC_ARCH();
        अगर (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    वापस V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_मूल() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    स्थिर string defenv = string{DEFENV_VERILATOR_मूल};
    अगर (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    अगर (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    वापस V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    स्थिर string defenv = string{DEFENV_VERILATOR_SOLVER};
    अगर (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    वापस var;
}

string V3Options::getStdPackagePath() {
    वापस V3Os::filenameJoin(getenv_VERILATOR_मूल(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    वापस V3Os::filenameJoin(getenv_VERILATOR_मूल(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(स्थिर string& var) {
    // If update below, also update V3Options::showVersion()
    अगर (var == "COROUTINES" && coroutineSupport()) {
        वापस "1";
    } अन्यथा अगर (var == "TSAN" && tsanSupport()) {
        वापस "1";
    } अन्यथा अगर (var == "DEV_ASAN" && devAsan()) {
        वापस "1";
    } अन्यथा अगर (var == "DEV_GCOV" && devGcov()) {
        वापस "1";
        // cppcheck-suppress knownConditionTrueFalse
    } अन्यथा अगर (var == "SYSTEMC" && systemCFound()) {
        वापस "1";
    } अन्यथा {
        वापस "";
    }
}

bool V3Options::systemCSystemWide() {
#अगर_घोषित HAVE_SYSTEMC
    वापस true;
#अन्यथा
    वापस false;
#पूर्ण_अगर
}

bool V3Options::systemCFound() {
    वापस (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

bool V3Options::coroutineSupport() {
#अगर_घोषित HAVE_COROUTINES
    वापस true;
#अन्यथा
    वापस false;
#पूर्ण_अगर
}

bool V3Options::devAsan() {
#अगर_घोषित HAVE_DEV_ASAN
    वापस true;
#अन्यथा
    वापस false;
#पूर्ण_अगर
}

bool V3Options::devGcov() {
#अगर_घोषित HAVE_DEV_GCOV
    वापस true;
#अन्यथा
    वापस false;
#पूर्ण_अगर
}

bool V3Options::tsanSupport() {
#अगर_घोषित HAVE_TSAN
    वापस true;
#अन्यथा
    वापस false;
#पूर्ण_अगर
}

//######################################################################
// V3 Options notification methods

व्योम V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_पंक्ति* स्थिर cmdfl = नया File_पंक्ति{File_पंक्ति::command_पंक्ति_Filename()};

    अगर (!outFormatOk() && v3Global.opt.मुख्य()) ccSet();  // --main implies --cc if not provided
    अगर (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    अगर (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    अगर (m_build) {
        अगर (m_द्विआधारी)
            backendFlags.emplace_back("--binary");
        अन्यथा
            backendFlags.emplace_back("--build");
    }
    अगर (m_preprocOnly) backendFlags.emplace_back("-E");
    अगर (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    अगर (m_lintOnly) backendFlags.emplace_back("--lint-only");
    अगर (m_jsonOnly) backendFlags.emplace_back("--json-only");
    अगर (backendFlags.आकार() > 1) {
        std::string backendFlagsString = backendFlags.front();
        क्रम (माप_प्रकार i = 1; i < backendFlags.आकार(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    अगर (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    अगर (!m_gmake && !m_makeJson) m_gmake = true;

    अगर (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    अगर (m_hierChild) {
        अगर (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = false;
    }

    अगर (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = true;  // No format, with --trace means wanted --trace-vcd
    }
    अगर (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = true;
    स्थिर पूर्णांक ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    अगर (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    अगर (protectIds()) {
        अगर (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        अगर (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        अगर (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    अगर (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    अगर (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    अगर (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(true);

    // Default split limits if not specified
    अगर (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    अगर (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    अगर (v3Global.opt.मुख्य() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    अगर (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        अगर (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    अगर (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    अगर (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    अगर (!m_dumpLevel.गिनती("tree") && m_dumpLevel.गिनती("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    अगर (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    अगर (m_buildJobs == -1) m_buildJobs = 1;
    अगर (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    अगर (timing().isSetTrue()) V3PreShell::define_Cmd_पंक्ति("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    अगर (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(false);

    // === Leave last
    // Mark options as available
    m_available = true;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    वापस ver;
}

string V3Options::protect_कुंजी_Defaulted() VL_MT_SAFE {
    अटल V3Mutex s_mutex;
    स्थिर V3LockGuard lock{s_mutex};
    अगर (m_protect_कुंजी.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_कुंजी = "VL-KEY-" + digest.digestBase64();
    }
    वापस m_protect_कुंजी;
}

व्योम V3Options::throwSigsegv() {  // LCOV_EXCL_START
#अगर !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<अस्थिर अक्षर*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#पूर्ण_अगर
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(स्थिर VTimescale& flag) स्थिर {
    अगर (!timeOverridePrec().isNone()) {
        वापस timeOverridePrec();
    } अन्यथा अगर (flag.isNone()) {
        वापस timeDefaultPrec();
    } अन्यथा {
        वापस flag;
    }
}

VTimescale V3Options::timeComputeUnit(स्थिर VTimescale& flag) स्थिर {
    अगर (!timeOverrideUnit().isNone()) {
        वापस timeOverrideUnit();
    } अन्यथा अगर (flag.isNone()) {
        वापस timeDefaultUnit();
    } अन्यथा {
        वापस flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(पूर्णांक argc, अक्षर** argv) {
    // Return list of arguments as simple string
    string opts;
    क्रम (पूर्णांक i = 0; i < argc; ++i) {
        अगर (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    वापस opts;
}

//######################################################################
// V3 Options Parsing

व्योम V3Options::parseOpts(File_पंक्ति* fl, पूर्णांक argc, अक्षर** argv) VL_MT_DISABLED {
    // Save command line options
    क्रम (पूर्णांक i = 0; i < argc; ++i) add_पंक्ति_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    अगर (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    अगर (prefix() == "" && शीर्ष_Module() != "") m_prefix = "V"s + शीर्ष_Module();  // Already encoded
    अगर (prefix() == "" && vFiles().आकार() >= 1)
        m_prefix
            = "V"s + Ast_गाँठ::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    अगर (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

bool V3Options::suffixed(स्थिर string& sw, स्थिर अक्षर* arg) {
    अगर (std::म_माप(arg) > sw.लंबाई()) वापस false;
    वापस (0 == std::म_भेद(sw.c_str() + sw.लंबाई() - std::म_माप(arg), arg));
}

व्योम V3Options::parseOptsList(File_पंक्ति* fl, स्थिर string& optdir, पूर्णांक argc,
                              अक्षर** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    स्थिर V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    स्थिर स्वतः callStrSetter = [यह](व्योम (V3Options::*cbStr)(स्थिर string&)) {
        वापस [यह, cbStr](स्थिर string& v) { (यह->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [यह](स्थिर अक्षर* optp) VL_MT_DISABLED {
        addDefine(optp, true);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [यह, &optdir](स्थिर अक्षर* optp) {
        string dirs = optp;
        string::आकार_type pos;
        जबतक ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        string exts = optp;
        string::आकार_type pos;
        जबतक ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](bool) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [यह](bool flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](स्थिर अक्षर*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [यह](bool flag) {
        m_bboxUnsup = flag;
        File_पंक्ति::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, true);
    });
    DECL_OPTION("-binary", CbCall, [यह]() {
        m_द्विआधारी = true;
        m_build = true;
        m_exe = true;
        m_main = true;
        अगर (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        पूर्णांक val = std::म_से_प(valp);
        अगर (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } अन्यथा अगर (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [यह]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](स्थिर std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](स्थिर std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](पूर्णांक val) { VName::max_लंबाई(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        अगर (!std::म_भेद(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } अन्यथा अगर (!std::म_भेद(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } अन्यथा अगर (!std::म_भेद(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } अन्यथा {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [यह](bool flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_व्यंजक);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_व्यंजक_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [यह, fl](स्थिर अक्षर* स्थिर valp) {
        अगर (!std::म_भेद(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } अन्यथा अगर (!std::म_भेद(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } अन्यथा अगर (!std::म_भेद(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } अन्यथा {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_पंक्ति);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    स्थिर स्वतः parseBinsLimit = [fl](स्थिर अक्षर* optp, स्थिर अक्षर* valp, uint32_t& limitr) {
        अक्षर* endp = nullptr;
        स्थिर अचिन्हित दीर्घ दीर्घ मूल्य = std::strtoull(valp, &endp, 10);
        अगर (*endp || मूल्य < 1 || मूल्य > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            वापस;
        }
        limitr = static_cast<uint32_t>(मूल्य);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [यह, parseBinsLimit](स्थिर अक्षर* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [यह, parseBinsLimit](स्थिर अक्षर* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [यह](स्थिर अक्षर* valp) VL_MT_DISABLED {
        addDefine(valp, false);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [यह]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [यह](पूर्णांक v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [यह](स्थिर अक्षर* optp, स्थिर अक्षर* valp) {
        m_debugLevel[optp] = std::म_से_प(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#अगर_घोषित VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", Set, &m_debugAllocRandom).undocumented();
#पूर्ण_अगर
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_ढेर_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [यह, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [यह, fl](स्थिर अक्षर* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [यह, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [यह](स्थिर अक्षर* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = true;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [यह](स्थिर अक्षर* optp, स्थिर अक्षर* valp) {
        m_dumpLevel[optp] = std::म_से_प(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [यह](bool flag) {
        अगर (flag) {
            m_stdPackage = false;
            m_stdWaiver = false;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<व्योम (*)(पूर्णांक)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [यह](स्थिर अक्षर* valp) { m_expandLimit = std::म_से_प(valp); });

    DECL_OPTION("-F", CbVal, [यह, fl, &optdir](स्थिर अक्षर* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), true);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [यह, &optdir](स्थिर अक्षर* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [यह, fl, &optdir](स्थिर अक्षर* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), false);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [यह](स्थिर अक्षर* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [यह](स्थिर अक्षर* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [यह](bool flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_वृक्ष = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_वृक्ष);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_वृक्ष);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [यह](bool flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [यह](bool flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [यह](स्थिर अक्षर* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [यह](स्थिर अक्षर* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_धकेलो_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [यह, fl](bool flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [यह](bool flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [यह](bool flag) {  //
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
    DECL_OPTION("-fslice-element-limit", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_fSliceElementLimit = std::म_से_प(valp);
        अगर (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](bool) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [यह](स्थिर अक्षर* optp) { addParameter(optp, false); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [यह]() {
        अ_बाहर << protect_कुंजी_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](स्थिर अक्षर* valp) {
        अ_बाहर << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](स्थिर अक्षर* valp) {
        अ_बाहर << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [यह](स्थिर अक्षर* valp) {
        स्थिर V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [यह](स्थिर अक्षर* optp) { m_hierParamsFile.धकेलो_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [यह, &optdir](स्थिर अक्षर* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [यह, fl](पूर्णांक val) {
        m_instr_गिनती_Dpi = val;
        अगर (m_instr_गिनती_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [यह](स्थिर अक्षर* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = true;
    });
    DECL_OPTION("-json-only-output", CbVal, [यह](स्थिर अक्षर* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = true;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [यह]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [यह]() { m_l2Name = "v"; }).undocumented();  // Historical
    स्थिर स्वतः setLang = [यह, fl](स्थिर अक्षर* valp) {
        स्थिर V3LangCode optval{valp};
        अगर (optval.legal()) {
            m_defaultLanguage = optval;
        } अन्यथा {
            VSpellCheck spell;
            क्रम (पूर्णांक i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.धकेलो_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_संदेश(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [यह](स्थिर अक्षर* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localize_Max_आकार);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [यह](स्थिर अक्षर* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_शीर्ष_Name);
    DECL_OPTION("-make", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        अगर (!std::म_भेद(valp, "gmake")) {
            m_gmake = true;
        } अन्यथा अगर (!std::म_भेद(valp, "json")) {
            m_makeJson = true;
        } अन्यथा {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_अंक_Width);
    DECL_OPTION("-mod-prefix", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [यह]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [यह]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [यह]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [यह]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](bool /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_outputGroups = std::म_से_प(valp);
        अगर (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_output_Groups_आकार = std::atoll(valp);
        अगर (m_output_Groups_आकार < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_outputSplitCFuncs = std::म_से_प(valp);
        अगर (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_outputSplitCTrace = std::म_से_प(valp);
        अगर (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_पंक्ति);
    DECL_OPTION("-pins64", CbCall, [यह]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [यह]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_pinsBv = std::म_से_प(valp);
        अगर (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [यह](bool flag) {
        m_pinsScUint = flag;
        अगर (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [यह](bool flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [यह](bool flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_preprocTokenLimit = std::म_से_प(valp);
        अगर (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [यह]() { m_public = false; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [यह]() { m_profC = m_profCFuncs = true; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [यह]() {
        m_profC = m_profCFuncs = true;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_कुंजी);
    DECL_OPTION("-protect-lib", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = true;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [यह](bool flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(true);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [यह](bool flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [यह](bool flag) {
        m_publicParams = flag;
        v3Global.dpi(true);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [यह](स्थिर अक्षर* varp) { addParameter(varp, false); });

    DECL_OPTION("-quiet", CbOnOff, [यह](bool flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_reloopLimit = std::म_से_प(valp);
        अगर (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_replicationLimit = std::म_से_प(valp);
        अगर (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [यह, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [यह]() {
        m_outFormatOk = true;
        m_systemC = true;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [यह](bool flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [यह](bool flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [यह]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [यह, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_threads = std::म_से_प(valp);
        अगर (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        अगर (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_hierThreads = std::म_से_प(valp);
        अगर (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        अगर (!std::म_भेद(valp, "all")) {
            m_threadsDpiPure = true;
            m_threadsDpiUnpure = true;
        } अन्यथा अगर (!std::म_भेद(valp, "none")) {
            m_threadsDpiPure = false;
            m_threadsDpiUnpure = false;
        } अन्यथा अगर (!std::म_भेद(valp, "pure")) {
            m_threadsDpiPure = true;
            m_threadsDpiUnpure = false;
        } अन्यथा {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        m_threadsMaxMTasks = std::म_से_प(valp);
        अगर (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        अगर (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        अगर (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, true);
        अगर (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        अगर (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [यह](स्थिर std::string& flag) { m_शीर्ष_Module = Ast_गाँठ::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [यह](स्थिर std::string& flag) { m_शीर्ष_Module = Ast_गाँठ::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_शीर्ष_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [यह]() { m_traceEnabledSaif = true; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [यह]() {
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
    DECL_OPTION("-trace-threads", CbVal, [fl](स्थिर अक्षर*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_शीर्ष);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [यह]() { m_traceEnabledVcd = true; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_गिनती).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [यह]() {
        showVersion(true);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [यह, &optdir](स्थिर अक्षर* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        पूर्णांक val = std::म_से_प(valp);
        अगर (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } अन्यथा अगर (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [यह]() {
        showVersion(false);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_पंक्ति::globalWarnOff(V3ErrorCode::I_LINT, false); });
    DECL_OPTION("-Werror-", CbPartialMatch, [यह, fl](स्थिर अक्षर* optp) {
        स्थिर V3ErrorCode code{optp};
        अगर (code == V3ErrorCode::EC_ERROR) {
            अगर (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } अन्यथा {
            V3Error::pretendError(code, true);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [यह](स्थिर अक्षर* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](स्थिर अक्षर* optp) VL_MT_DISABLED {
        स्थिर string err = File_पंक्ति::globalWarnOffParse(optp, true);
        अगर (!err.empty()) {
            स्थिर string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    क्रम (पूर्णांक i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        क्रम (स्थिर string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [यह]() { m_context = false; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(false); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_पंक्ति::globalWarnOff(V3ErrorCode::I_LINT, true); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_पंक्ति::globalWarnOff(V3ErrorCode::I_STYLE, true); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [यह]() {
        m_pedantic = true;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, false);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [यह, fl, &parser](स्थिर अक्षर* optp) VL_MT_DISABLED {
        स्थिर V3ErrorCode code{optp};
        अगर (code == V3ErrorCode::EC_ERROR) {
            अगर (!isFuture(optp)) {
                स्थिर string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } अन्यथा {
            File_पंक्ति::globalWarnOff(code, false);
            V3Error::pretendError(code, false);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_पंक्ति::globalWarnOff(V3ErrorCode::I_LINT, false); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_पंक्ति::globalWarnOff(V3ErrorCode::I_STYLE, false); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        अगर (!std::म_भेद(valp, "0")) {
            m_xAssign = "0";
        } अन्यथा अगर (!std::म_भेद(valp, "1")) {
            m_xAssign = "1";
        } अन्यथा अगर (!std::म_भेद(valp, "fast")) {
            m_xAssign = "fast";
        } अन्यथा अगर (!std::म_भेद(valp, "unique")) {
            m_xAssign = "unique";
        } अन्यथा {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [यह, fl](स्थिर अक्षर* valp) {
        अगर (!std::म_भेद(valp, "0")) {
            m_xInitial = "0";
        } अन्यथा अगर (!std::म_भेद(valp, "fast")) {
            m_xInitial = "fast";
        } अन्यथा अगर (!std::म_भेद(valp, "unique")) {
            m_xInitial = "unique";
        } अन्यथा {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [यह, &optdir](स्थिर अक्षर* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    क्रम (पूर्णांक i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        अगर (argv[i][0] == '-' || argv[i][0] == '+') {
            स्थिर std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            पूर्णांक consumed = 0;
            bool isForRerun = true;

            // Special case for -j which has an optional argument
            अगर (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                पूर्णांक val = 0;
                अगर (i + 1 < argc && std::है_अंक(argv[i + 1][0])) {
                    val = std::म_से_प(argv[i + 1]);  // Can't be negative due to isdigit above
                    अगर (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                अगर (m_buildJobs == -1) m_buildJobs = val;
                अगर (m_verilateJobs == -1) m_verilateJobs = val;
                अगर (m_outputGroups == -1) m_outputGroups = val;
            } अन्यथा {
                स्थिर स्वतः pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                अगर (consumed) {
                    // Already processed
                } अन्यथा अगर (isFuture0(argName)) {
                    consumed = 1;
                } अन्यथा अगर (isFuture1(argName)) {
                    consumed = 2;
                } अन्यथा {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            अगर (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            जारी;
        }

        // Filename
        स्थिर std::string filename = parseFileArg(optdir, argv[i]);
        bool isForRerun = true;
        अगर (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } अन्यथा अगर (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } अन्यथा अगर (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = false;
        } अन्यथा {
            V3Options::addVFile(filename, work());
            isForRerun = false;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    अगर (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

व्योम V3Options::parseOptsFile(File_पंक्ति* fl, स्थिर string& filename, bool rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    स्थिर std::unique_ptr<std::अख_प्रवाह> ifp{V3File::new_ifstream(filename)};
    अगर (ifp->हार()) {
        fl->v3error("Cannot open -f command file: " + filename);
        वापस;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    bool has_fargs = false;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    स्थिर स्वतः parse_fargs_cmt = [&](स्थिर अक्षर* p) -> व्योम {
        स्थिर bool blockCmt = p[1] == '*';
        p += 2;
        जबतक (std::है_खाली(*p)) ++p;
        अगर (VString::startsWith(p, "verilator")) {
            p += std::म_माप("verilator");
            अगर (std::है_खाली(*p)) {
                जबतक (std::है_खाली(*p)) ++p;
                अगर (VString::startsWith(p, "fargs")) {
                    p += std::म_माप("fargs");
                    अगर (std::है_खाली(*p)) {
                        जबतक (std::है_खाली(*p)) ++p;
                        अगर (!has_fargs) {
                            has_fargs = true;
                            whole_file.सफा();
                        }
                        whole_file += p;
                        अगर (blockCmt) {
                            स्थिर std::string::आकार_type pos = whole_file.find("*/");
                            अगर (pos == std::string::npos) {
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
    जबतक (!ifp->खप()) {
        स्थिर std::string पंक्ति = V3Os::लो_पंक्ति(*ifp);
        // Strip simple comments
        अक्षर lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        bool space_begin = true;  // At beginning or leading spaces only
        क्रम (स्थिर अक्षर* p = पंक्ति.c_str(); *p; lastch = *p++) {
            अगर (inCmt) {
                अगर (p[0] == '*' && p[1] == '/') {
                    inCmt = false;
                    ++p;
                }
            } अन्यथा अगर (p[0] == '/' && p[1] == '/' && std::है_खाली(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                अवरोध;  // Ignore to EOL
            } अन्यथा अगर (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                अवरोध;  // Ignore to EOL
            } अन्यथा अगर (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = true;
                space_begin = false;
                ++p;
            } अन्यथा {
                अगर (!std::है_खाली(*p)) space_begin = false;
                अगर (!has_fargs) whole_file += *p;
            }
        }
        अगर (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    अगर (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = नया File_पंक्ति{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    क्रमागत state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    क्रम (string::आकार_type pos = 0; pos < whole_file.लंबाई(); ++pos) {
        अक्षर curr_char = whole_file[pos];
        चयन (st) {
        हाल ST_IN_OPTION:  // Get all chars up to a white space or a "="
            अगर (std::है_खाली(curr_char)) {  // End of option
                अगर (!arg.empty()) {  // End of word
                    args.धकेलो_back(arg);
                }
                arg = "";
                अवरोध;
            }
            अगर (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                अवरोध;
            }
            अगर (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                अगर (pos < whole_file.लंबाई()) curr_char = whole_file[pos];
                अगर (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } अन्यथा {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                अवरोध;
            }
            अगर (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                अवरोध;
            }
            arg += curr_char;
            अवरोध;
        हाल ST_IN_QUOTED_STR:  // Just store all chars inside string
            अगर (curr_char != '\'') {
                arg += curr_char;
            } अन्यथा {  // End of quoted string
                st = ST_IN_OPTION;
            }
            अवरोध;
        हाल ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            अगर (curr_char != '"') {
                अगर (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } अन्यथा {
                    arg += curr_char;
                }
            } अन्यथा {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            अवरोध;
        हाल ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            अवरोध;
        }
    }
    अगर (!arg.empty()) {  // Add last word
        args.धकेलो_back(arg);
    }

    // Path
    स्थिर string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<अक्षर*> argv;
    argv.reserve(args.आकार() + 1);
    क्रम (स्थिर string& i : args) argv.धकेलो_back(const_cast<अक्षर*>(i.c_str()));
    argv.धकेलो_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<पूर्णांक>(argv.आकार() - 1), argv.तथ्य());
}

//======================================================================

string V3Options::parseFileArg(स्थिर string& optdir, स्थिर string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    अगर (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    वापस filename;
}

//======================================================================

व्योम V3Options::showVersion(bool verbose) {
    अ_बाहर << version();
    अ_बाहर << "\n";
    अगर (!verbose) वापस;

    अ_बाहर << "\n";
    अ_बाहर << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    अ_बाहर << "redistribute it and/or modify the Verilator internals under the terms of\n";
    अ_बाहर << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    अ_बाहर << "License Version 2.0.\n";

    अ_बाहर << "\n";
    अ_बाहर << "See https://verilator.org for documentation\n";

    अ_बाहर << "\n";
    अ_बाहर << "Summary of configuration:\n";
    अ_बाहर << "  Compiled in defaults if not in environment:\n";
    अ_बाहर << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    अ_बाहर << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    अ_बाहर << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    अ_बाहर << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    अ_बाहर << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_मूल << "\n";
    अ_बाहर << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    अ_बाहर << "    SystemC system-wide = " << cvtToStr(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    अ_बाहर << "\n";
    अ_बाहर << "Environment:\n";
    अ_बाहर << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    अ_बाहर << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    अ_बाहर << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    अ_बाहर << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    अ_बाहर << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    अ_बाहर << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    अ_बाहर << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    अ_बाहर << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    अ_बाहर << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    अ_बाहर << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    अ_बाहर << "\n";
    अ_बाहर << "Supported features (compiled-in or forced by environment):\n";
    अ_बाहर << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    अ_बाहर << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    अ_बाहर << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = नया V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_unusedRegexp = "*unused*";
    m_xAssign = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3LangCode::mostRecent();

    VName::max_लंबाई(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    addLibExtV("");  // So include "filename.v" will find the same file
    addLibExtV(".v");
    addLibExtV(".sv");
    // Default -I
    addIncDirFallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(मिटाओ m_impp, m_impp = nullptr); }

व्योम V3Options::setDebugMode(पूर्णांक level) {
    V3Error::debugDefault(level);
    अगर (!m_dumpLevel.गिनती("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = true;
    m_debugCheck = true;
    अगर (level) {
        अ_बाहर << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

अचिन्हित V3Options::debugLevel(स्थिर string& tag) स्थिर VL_MT_SAFE {
    स्थिर स्वतः iter = m_debugLevel.find(tag);
    वापस iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

अचिन्हित V3Options::debugSrcLevel(स्थिर string& srcfile_path) स्थिर VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    वापस debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

अचिन्हित V3Options::dumpLevel(स्थिर string& tag) स्थिर VL_MT_SAFE {
    स्थिर स्वतः iter = m_dumpLevel.find(tag);
    वापस iter != m_dumpLevel.end() ? iter->second : 0;
}

अचिन्हित V3Options::dumpSrcLevel(स्थिर string& srcfile_path) स्थिर VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    वापस dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

bool V3Options::dump_वृक्ष_Addrids() स्थिर VL_MT_SAFE {
    अटल पूर्णांक s_level = -1;
    अगर (VL_UNLIKELY(s_level < 0)) {
        स्थिर अचिन्हित मूल्य = dumpLevel("tree-addrids");
        अगर (!available()) वापस मूल्य > 0;
        s_level = static_cast<अचिन्हित>(मूल्य);
    }
    वापस s_level > 0;
}

व्योम V3Options::optimize(पूर्णांक level) {
    // Set all optimizations to on/off
    स्थिर bool flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_वृक्ष = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_वृक्ष = flag;
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
    अगर (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
