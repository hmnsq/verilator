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

#അന്തര്ഭാവ "V3PchAstMT.h"

#അന്തര്ഭാവ "V3Options.h"

#അന്തര്ഭാവ "V3Container.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3File.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Mutex.h"
#അന്തര്ഭാവ "V3OptionParser.h"
#അന്തര്ഭാവ "V3Os.h"
#അന്തര്ഭാവ "V3PreShell.h"
#അന്തര്ഭാവ "V3String.h"

// clang-format off
#അന്തര്ഭാവ <sys/types.h>
#അന്തര്ഭാവ <sys/stat.h>
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#അല്ലെങ്കിൽ
# include <dirent.h>
#endif
#അന്തര്ഭാവ <fcntl.h>
#അന്തര്ഭാവ <list>
#അന്തര്ഭാവ <map>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <thread>
#അന്തര്ഭാവ <set>
#അന്തര്ഭാവ <string>

#അന്തര്ഭാവ "config_rev.h"

#എങ്കിൽ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

വർഗ്ഗം V3OptionsImp final {
പൊതു:
    // TYPES
    using DirMap = std::map<സ്ഥിരം string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_വരി_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, തർക്കം>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<സ്ഥിരം string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    ശൂന്യം addIncDirUser(സ്ഥിരം string& incdir) {
        സ്ഥിരം string& dir = V3Os::filenameCleanup(incdir);
        സ്ഥിരം തർക്കം inserted = m_incDirUsers.തിരുകുക(dir);
        എങ്കിൽ (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    ശൂന്യം addIncDirFallback(സ്ഥിരം string& incdir) {
        സ്ഥിരം string& dir = V3Os::filenameCleanup(incdir);
        എങ്കിൽ (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            സ്ഥിരം auto itFoundPair = m_incDirFallbackSet.തിരുകുക(dir);
            എങ്കിൽ (itFoundPair.second) m_incDirFallbacks.തള്ളുക_back(dir);
        }
    }
    ശൂന്യം addLangExt(സ്ഥിരം string& langext, സ്ഥിരം V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        എങ്കിൽ (addext[0] == '.') addext = addext.substr(1);
        (ശൂന്യം)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    ശൂന്യം addLibExtV(സ്ഥിരം string& libext) { m_libExtVs.തിരുകുക(libext); }
    V3OptionsImp() = സ്ഥിരസ്ഥിതി;
    ~V3OptionsImp() = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(സ്ഥിരം അക്ഷരം* textp) {
    // Return code for given string, or ERROR, which is a bad code
    വേണ്ടി (പൂർണ്ണസംഖ്യ codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        സ്ഥിരം V3LangCode code{codei};
        എങ്കിൽ (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            തിരികെ;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(സ്ഥിരം string& മൂല്യം, തർക്കം& badr)
    : m_e{VTimescale::NONE} {
    badr = സത്യം;
    സ്ഥിരം string spaceless = VString::removeWhitespace(മൂല്യം);
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = TS_100S; i < _ENUM_END; ++i) {
        സ്ഥിരം VTimescale ts{i};
        എങ്കിൽ (spaceless == ts.ascii()) {
            badr = അസത്യം;
            m_e = ts.m_e;
            നിർത്തുക;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(സ്ഥിരം string& opts) {
    VStringList vals;
    തർക്കം inStr = അസത്യം;
    string cur;
    സ്ഥിതം സ്ഥിരം string hierBlock("--hierarchical-block");
    File_വരി cmdfl(File_വരി::command_വരി_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    വേണ്ടി (string::const_iterator it = opts.begin(); it != opts.end();) {
        എങ്കിൽ (inStr) {
            എങ്കിൽ (*it == '\\') {
                ++it;
                എങ്കിൽ (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    നിർത്തുക;
                }
                എങ്കിൽ (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    നിർത്തുക;
                }
                cur.തള്ളുക_back(*it);
                ++it;
            } അല്ലെങ്കിൽ എങ്കിൽ (*it == '"') {  // end of string
                cur.തള്ളുക_back(*it);
                vals.തള്ളുക_back(cur);
                cur.clear();
                ++it;
                എങ്കിൽ (it != opts.end()) {
                    എങ്കിൽ (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        നിർത്തുക;
                    }
                    ++it;
                    എങ്കിൽ (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        നിർത്തുക;
                    }
                    inStr = *it == '"';
                    cur.തള്ളുക_back(*it);
                    ++it;
                }
            } അല്ലെങ്കിൽ {
                cur.തള്ളുക_back(*it);
                ++it;
            }
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                നിർത്തുക;
            }
            എങ്കിൽ (*it == ',') {  // end of this parameter
                vals.തള്ളുക_back(cur);
                cur.clear();
                ++it;
                എങ്കിൽ (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    നിർത്തുക;
                }
                inStr = *it == '"';
            }
            cur.തള്ളുക_back(*it);
            ++it;
        }
    }
    എങ്കിൽ (!cur.empty()) vals.തള്ളുക_back(cur);
    എങ്കിൽ (vals.size() >= 2) {
        എങ്കിൽ (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } അല്ലെങ്കിൽ {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    വേണ്ടി (size_t i = 2; i + 1 < vals.size(); i += 2) {
        സ്ഥിരം തർക്കം inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        എങ്കിൽ (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

ശൂന്യം VTimescale::parseSlashed(File_വരി* fl, സ്ഥിരം അക്ഷരം* textp, VTimescale& unitr,
                              VTimescale& precr, തർക്കം allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    സ്ഥിരം അക്ഷരം* cp = textp;
    വേണ്ടി (; std::isspace(*cp); ++cp) {}
    സ്ഥിരം അക്ഷരം* സ്ഥിരം unitp = cp;
    വേണ്ടി (; *cp && *cp != '/'; ++cp) {}
    സ്ഥിരം string unitStr(unitp, cp - unitp);
    വേണ്ടി (; std::isspace(*cp); ++cp) {}
    string precStr;
    എങ്കിൽ (*cp == '/') {
        ++cp;
        വേണ്ടി (; std::isspace(*cp); ++cp) {}
        സ്ഥിരം അക്ഷരം* സ്ഥിരം precp = cp;
        വേണ്ടി (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    വേണ്ടി (; std::isspace(*cp); ++cp) {}
    എങ്കിൽ (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        തിരികെ;
    }

    തർക്കം unitbad = അസത്യം;
    സ്ഥിരം VTimescale unit{unitStr, unitbad /*ref*/};
    എങ്കിൽ (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        തിരികെ;
    }
    unitr = unit;

    എങ്കിൽ (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        തർക്കം precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        എങ്കിൽ (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            തിരികെ;
        }
        എങ്കിൽ (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            തിരികെ;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

ശൂന്യം V3Options::addIncDirUser(സ്ഥിരം string& incdir) { m_impp->addIncDirUser(incdir); }
ശൂന്യം V3Options::addIncDirFallback(സ്ഥിരം string& incdir) { m_impp->addIncDirFallback(incdir); }
ശൂന്യം V3Options::addLangExt(സ്ഥിരം string& langext, സ്ഥിരം V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
ശൂന്യം V3Options::addLibExtV(സ്ഥിരം string& libext) { m_impp->addLibExtV(libext); }
ശൂന്യം V3Options::addDefine(സ്ഥിരം string& defline, തർക്കം allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ഇടത് = defline;
    ആകുമ്പോൾ (ഇടത് != "") {
        string def = ഇടത്;
        string::size_type pos;
        എങ്കിൽ (allowPlus && ((pos = ഇടത്.find('+')) != string::npos)) {
            ഇടത് = ഇടത്.substr(pos + 1);
            def.erase(pos);
        } അല്ലെങ്കിൽ {
            ഇടത് = "";
        }
        string മൂല്യം;
        എങ്കിൽ ((pos = def.find('=')) != string::npos) {
            മൂല്യം = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_വരി(def, മൂല്യം);
    }
}
ശൂന്യം V3Options::addParameter(സ്ഥിരം string& paramline, തർക്കം allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ഇടത് = paramline;
    ആകുമ്പോൾ (ഇടത് != "") {
        string param = ഇടത്;
        string::size_type pos;
        എങ്കിൽ (allowPlus && ((pos = ഇടത്.find('+')) != string::npos)) {
            ഇടത് = ഇടത്.substr(pos + 1);
            param.erase(pos);
        } അല്ലെങ്കിൽ {
            ഇടത് = "";
        }
        string മൂല്യം;
        എങ്കിൽ ((pos = param.find('=')) != string::npos) {
            മൂല്യം = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << മൂല്യം);
        (ശൂന്യം)m_parameters.erase(param);
        m_parameters[param] = മൂല്യം;
    }
}

തർക്കം V3Options::hasParameter(സ്ഥിരം string& name) {
    തിരികെ m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(സ്ഥിരം string& name) {
    string മൂല്യം = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    തിരികെ മൂല്യം;
}

ശൂന്യം V3Options::checkParameters() {
    എങ്കിൽ (!m_parameters.empty()) {
        std::stringstream സന്ദേശം;
        സന്ദേശം << "Parameters from the command line were not found in the design:";
        വേണ്ടി (സ്ഥിരം auto& i : m_parameters) സന്ദേശം << " " << i.first;
        v3error(സന്ദേശം.str());
    }
}

ശൂന്യം V3Options::addCppFile(സ്ഥിരം string& filename) { m_cppFiles.തിരുകുക(filename); }
ശൂന്യം V3Options::addCFlags(സ്ഥിരം string& filename) { m_cFlags.തള്ളുക_back(filename); }
ശൂന്യം V3Options::addLibMapFile(സ്ഥിരം string& filename) { m_libmapFiles.തിരുകുക(filename); }
ശൂന്യം V3Options::addCompilerIncludes(സ്ഥിരം string& filename) {
    m_compilerIncludes.തിരുകുക(filename);
}
ശൂന്യം V3Options::addLdLibFile(സ്ഥിരം string& filename) {
    m_ldLibFiles.തള്ളുക_back(filename);
    addLdLibs(filename);
}
ശൂന്യം V3Options::addLdLibs(സ്ഥിരം string& filename) { m_ldLibs.തള്ളുക_back(filename); }
ശൂന്യം V3Options::addMakeFlags(സ്ഥിരം string& filename) { m_makeFlags.തള്ളുക_back(filename); }
ശൂന്യം V3Options::addFuture(സ്ഥിരം string& flag) { m_futures.തിരുകുക(flag); }
ശൂന്യം V3Options::addFuture0(സ്ഥിരം string& flag) { m_future0s.തിരുകുക(flag); }
ശൂന്യം V3Options::addFuture1(സ്ഥിരം string& flag) { m_future1s.തിരുകുക(flag); }
തർക്കം V3Options::isFuture(സ്ഥിരം string& flag) സ്ഥിരം {
    തിരികെ m_futures.find(flag) != m_futures.end();
}
തർക്കം V3Options::isFuture0(സ്ഥിരം string& flag) സ്ഥിരം {
    തിരികെ m_future0s.find(flag) != m_future0s.end();
}
തർക്കം V3Options::isFuture1(സ്ഥിരം string& flag) സ്ഥിരം {
    തിരികെ m_future1s.find(flag) != m_future1s.end();
}
തർക്കം V3Options::isLibraryFile(സ്ഥിരം string& filename, സ്ഥിരം string& libname) സ്ഥിരം {
    തിരികെ m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
ശൂന്യം V3Options::addLibraryFile(സ്ഥിരം string& filename, സ്ഥിരം string& libname) {
    m_libraryFiles.തിരുകുക({filename, libname});
}
ശൂന്യം V3Options::addVFile(സ്ഥിരം string& filename, സ്ഥിരം string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.തള്ളുക_back({filename, libname});  // Not emplace
}
ശൂന്യം V3Options::addVltFile(സ്ഥിരം string& filename, സ്ഥിരം string& libname) {
    m_vltFiles.തിരുകുക({filename, libname});
}
ശൂന്യം V3Options::addForceInc(സ്ഥിരം string& filename) { m_forceIncs.തള്ളുക_back(filename); }

ശൂന്യം V3Options::add_വരി_Arg(സ്ഥിരം string& arg) { m_impp->m_വരി_Args.തള്ളുക_back(arg); }

ശൂന്യം V3Options::addArg(അക്ഷരം** argv, size_t എണ്ണം, തർക്കം isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, തർക്കം>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    വേണ്ടി (size_t n = 0; n < എണ്ണം; ++n) pair.first.emplace_back(argv[n]);
}

ശൂന്യം V3Options::addArg(സ്ഥിരം std::string& arg, തർക്കം isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, തർക്കം>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

സ്ഥിരം std::list<std::pair<std::list<std::string>, തർക്കം>>& V3Options::allArgs() സ്ഥിരം {
    തിരികെ m_impp->m_allArgs;
}

string V3Options::allArgsString() സ്ഥിരം VL_MT_SAFE {
    string result;
    വേണ്ടി (സ്ഥിരം auto& pair : m_impp->m_allArgs) {
        വേണ്ടി (സ്ഥിരം string& arg : pair.first) {
            എങ്കിൽ (!result.empty()) result += " ";
            result += arg;
        }
    }
    തിരികെ result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(തർക്കം for_മുകൾ) സ്ഥിരം {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    വേണ്ടി (സ്ഥിരം auto& vFile : m_vFiles) vFiles.തിരുകുക(vFile.filename());
    string out;
    തർക്കം stripArg = അസത്യം;
    തർക്കം strip_Arg_If_അക്കം = അസത്യം;
    വേണ്ടി (സ്ഥിരം auto& pair : m_impp->m_allArgs) {
        വേണ്ടി (സ്ഥിരം string& arg : pair.first) {
            എങ്കിൽ (stripArg) {
                stripArg = അസത്യം;
                തുടരുക;
            }
            എങ്കിൽ (strip_Arg_If_അക്കം) {
                strip_Arg_If_അക്കം = അസത്യം;
                എങ്കിൽ (isdigit(arg[0])) തുടരുക;
            }
            പൂർണ്ണസംഖ്യ skip = 0;
            എങ്കിൽ (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } അല്ലെങ്കിൽ എങ്കിൽ (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            എങ്കിൽ (skip > 0) {  // arg is an option
                സ്ഥിരം string opt = arg.substr(skip);  // Remove '-' in the beginning
                സ്ഥിരം പൂർണ്ണസംഖ്യ അക്കം_Strip = stripOptionsForChildRun(opt, for_മുകൾ);
                എങ്കിൽ (അക്കം_Strip) {
                    UASSERT(0 <= അക്കം_Strip && അക്കം_Strip <= 3, "should be one of 0, 1, 2, 3");
                    എങ്കിൽ (അക്കം_Strip == 2) stripArg = സത്യം;
                    എങ്കിൽ (അക്കം_Strip == 3) strip_Arg_If_അക്കം = സത്യം;
                    തുടരുക;
                }
            } അല്ലെങ്കിൽ {  // Not an option
                എങ്കിൽ (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    തുടരുക;
                }
            }
            എങ്കിൽ (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    തിരികെ out;
}

ശൂന്യം V3Options::ccSet() {  // --cc
    m_outFormatOk = സത്യം;
    m_systemC = അസത്യം;
}

ശൂന്യം V3Options::decorations(File_വരി* fl, സ്ഥിരം string& arg) {  // --decorations
    എങ്കിൽ (arg == "none") {
        m_decoration = അസത്യം;
        m_decorationNodes = അസത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (arg == "node") {
        m_decoration = സത്യം;
        m_decorationNodes = സത്യം;
    } അല്ലെങ്കിൽ എങ്കിൽ (arg == "medium") {
        m_decoration = സത്യം;
        m_decorationNodes = അസത്യം;
    } അല്ലെങ്കിൽ {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() സ്ഥിരം VL_MT_SAFE {
    std::vector<std::string> result;
    എങ്കിൽ (traceEnabledFst()) result.emplace_back("VerilatedFst");
    എങ്കിൽ (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    എങ്കിൽ (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    തിരികെ result;
}
std::vector<std::string> V3Options::traceClassLangs() സ്ഥിരം VL_MT_SAFE {
    std::vector<std::string> result;
    വേണ്ടി (സ്ഥിരം auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    തിരികെ result;
}
std::vector<std::string> V3Options::traceSourceBases() സ്ഥിരം VL_MT_SAFE {
    std::vector<std::string> result;
    എങ്കിൽ (traceEnabledFst()) result.emplace_back("verilated_fst");
    എങ്കിൽ (traceEnabledSaif()) result.emplace_back("verilated_saif");
    എങ്കിൽ (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    തിരികെ result;
}
std::vector<std::string> V3Options::traceSourceLangs() സ്ഥിരം VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    വേണ്ടി (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    തിരികെ result;
}
std::string V3Options::traceClassBase() സ്ഥിരം VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    തിരികെ traceClassBases().front();
}
std::string V3Options::traceClassLang() സ്ഥിരം VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    തിരികെ traceClassLangs().front();
}

//######################################################################
// File searching

തർക്കം V3Options::fileStatNormal(സ്ഥിരം string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ഘടന stat sstat;  // Stat information
    സ്ഥിരം പൂർണ്ണസംഖ്യ err = stat(filename.c_str(), &sstat);
    എങ്കിൽ (err != 0) തിരികെ അസത്യം;
    എങ്കിൽ (S_ISDIR(sstat.st_mode)) തിരികെ അസത്യം;
    തിരികെ സത്യം;
}

string V3Options::fileExists(സ്ഥിരം string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    സ്ഥിരം string dir = V3Os::filenameDir(filename);
    സ്ഥിരം string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    എങ്കിൽ (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* സ്ഥിരം setp = &(diriter->second);

#ifdef _MSC_VER
        ശ്രമിക്കുക {
            വേണ്ടി (സ്ഥിരം auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->തിരുകുക(dirEntry.path().filename().string());
        } പിടിക്കുക (std::filesystem::filesystem_error സ്ഥിരം& ex) {
            (ശൂന്യം)ex;
            തിരികെ "";
        }
#അല്ലെങ്കിൽ
        എങ്കിൽ (DIR* സ്ഥിരം dirp = opendir(dir.c_str())) {
            ആകുമ്പോൾ (ഘടന dirent* direntp = readdir(dirp)) setp->തിരുകുക(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    സ്ഥിരം std::set<string>* സ്ഥിരം filesetp = &(diriter->second);
    സ്ഥിരം auto fileiter = filesetp->find(basename);
    എങ്കിൽ (fileiter == filesetp->end()) തിരികെ "";  // Not found
    // Check if it is a directory, ignore if so
    സ്ഥിരം string filenameOut = V3Os::filenameJoin(dir, basename);
    എങ്കിൽ (!fileStatNormal(filenameOut)) തിരികെ "";  // Directory
    തിരികെ filenameOut;
}

string V3Options::filePathCheckOneDir(സ്ഥിരം string& modname, സ്ഥിരം string& dirname) {
    വേണ്ടി (സ്ഥിരം string& i : m_impp->m_libExtVs) {
        സ്ഥിരം string fn = V3Os::filenameJoin(dirname, modname + i);
        സ്ഥിരം string exists = fileExists(fn);
        എങ്കിൽ (exists != "") തിരികെ exists;
    }
    തിരികെ "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
പൂർണ്ണസംഖ്യ V3Options::stripOptionsForChildRun(സ്ഥിരം string& opt, തർക്കം for_മുകൾ) {
    // Options to strip for both the top wrapper, and hier blocks
    സ്ഥിതം സ്ഥിരം std::unordered_map<std::string, പൂർണ്ണസംഖ്യ> commonOpts{
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
    എങ്കിൽ (commonOpts.എണ്ണം(opt)) തിരികെ commonOpts.at(opt);

    // Options to strip only for hier blocks
    എങ്കിൽ (!for_മുകൾ) {
        സ്ഥിതം സ്ഥിരം std::unordered_map<std::string, പൂർണ്ണസംഖ്യ> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        എങ്കിൽ (subOpts.എണ്ണം(opt)) തിരികെ subOpts.at(opt);
        എങ്കിൽ (VString::startsWith(opt, "G")) തിരികെ 1;
        എങ്കിൽ (VString::startsWith(opt, "pvalue")) തിരികെ 1;
    }

    // Do not strip
    തിരികെ 0;
}

ശൂന്യം V3Options::validateIdentifier(File_വരി* fl, സ്ഥിരം string& arg, സ്ഥിരം string& opt) {
    എങ്കിൽ (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_വരി* fl, സ്ഥിരം string& modname, സ്ഥിരം string& lastpath,
                           സ്ഥിരം string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    സ്ഥിരം string filename = V3Os::filenameCleanup(VName::dehash(modname));
    എങ്കിൽ (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        സ്ഥിരം string exists = filePathCheckOneDir(filename, "");
        എങ്കിൽ (exists != "") തിരികെ exists;
    }
    വേണ്ടി (സ്ഥിരം string& dir : m_impp->m_incDirUsers) {
        സ്ഥിരം string exists = filePathCheckOneDir(filename, dir);
        എങ്കിൽ (exists != "") തിരികെ exists;
    }
    വേണ്ടി (സ്ഥിരം string& dir : m_impp->m_incDirFallbacks) {
        സ്ഥിരം string exists = filePathCheckOneDir(filename, dir);
        എങ്കിൽ (exists != "") തിരികെ exists;
    }

    എങ്കിൽ (m_relativeIncludes) {
        സ്ഥിരം string exists = filePathCheckOneDir(filename, lastpath);
        എങ്കിൽ (exists != "") തിരികെ V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    എങ്കിൽ (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_സന്ദേശം(fl, filename));
    }
    തിരികെ "";
}

string V3Options::file_Path_Looked_സന്ദേശം(File_വരി* /*fl*/, സ്ഥിരം string& modname) {
    സ്ഥിതം തർക്കം s_shown_notfound_സന്ദേശം = അസത്യം;
    std::ostringstream ss;
    എങ്കിൽ (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } അല്ലെങ്കിൽ എങ്കിൽ (!s_shown_notfound_സന്ദേശം) {
        s_shown_notfound_സന്ദേശം = സത്യം;
        എങ്കിൽ (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        വേണ്ടി (സ്ഥിരം string& dir : m_impp->m_incDirUsers) {
            വേണ്ടി (സ്ഥിരം string& ext : m_impp->m_libExtVs) {
                സ്ഥിരം string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        വേണ്ടി (സ്ഥിരം string& dir : m_impp->m_incDirFallbacks) {
            വേണ്ടി (സ്ഥിരം string& ext : m_impp->m_libExtVs) {
                സ്ഥിരം string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    തിരികെ ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(സ്ഥിരം string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    എങ്കിൽ (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        തിരികെ V3LangCode::mostRecent();
    } അല്ലെങ്കിൽ എങ്കിൽ ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        സ്ഥിരം auto it = m_impp->m_langExts.find(ext);
        എങ്കിൽ (it != m_impp->m_langExts.end()) തിരികെ it->second;
    }
    തിരികെ m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(സ്ഥിരം string& var) {
    // If update below, also update V3Options::showVersion()
    എങ്കിൽ (var == "MAKE") {
        തിരികെ getenvMAKE();
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "PERL") {
        തിരികെ getenvPERL();
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "PYTHON3") {
        തിരികെ getenvPYTHON3();
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "SYSTEMC") {
        തിരികെ getenvSYSTEMC();
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "SYSTEMC_ARCH") {
        തിരികെ getenvSYSTEMC_ARCH();
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "SYSTEMC_INCLUDE") {
        തിരികെ getenvSYSTEMC_INCLUDE();
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "SYSTEMC_LIBDIR") {
        തിരികെ getenvSYSTEMC_LIBDIR();
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "VERILATOR_ROOT") {
        തിരികെ getenv_VERILATOR_വേര്();
    } അല്ലെങ്കിൽ {
        തിരികെ V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { തിരികെ V3Os::getenvStr("MAKE", "gmake"); }
#അല്ലെങ്കിൽ
string V3Options::getenvMAKE() { തിരികെ V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    തിരികെ V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    തിരികെ V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    തിരികെ V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    സ്ഥിരം string defenv = string{DEFENV_SYSTEMC};
    എങ്കിൽ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    തിരികെ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    സ്ഥിരം string defenv = string{DEFENV_SYSTEMC_ARCH};
    എങ്കിൽ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    എങ്കിൽ (var == "") {
#എങ്കിൽ defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        സ്ഥിരം string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#അല്ലെങ്കിലെങ്കിൽ defined(_WIN32)
        സ്ഥിരം string sysname = "WIN32";
        var = "win32";
#അല്ലെങ്കിൽ
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ഘടന utsname uts;
        uname(&uts);
        സ്ഥിരം string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        എങ്കിൽ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } അല്ലെങ്കിൽ എങ്കിൽ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } അല്ലെങ്കിൽ {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    തിരികെ var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    സ്ഥിരം string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    എങ്കിൽ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    എങ്കിൽ (var == "") {
        സ്ഥിരം string sc = getenvSYSTEMC();
        എങ്കിൽ (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    തിരികെ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    സ്ഥിരം string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    എങ്കിൽ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    എങ്കിൽ (var == "") {
        സ്ഥിരം string sc = getenvSYSTEMC();
        സ്ഥിരം string arch = getenvSYSTEMC_ARCH();
        എങ്കിൽ (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    തിരികെ V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_വേര്() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    സ്ഥിരം string defenv = string{DEFENV_VERILATOR_വേര്};
    എങ്കിൽ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    എങ്കിൽ (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    തിരികെ V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    സ്ഥിരം string defenv = string{DEFENV_VERILATOR_SOLVER};
    എങ്കിൽ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    തിരികെ var;
}

string V3Options::getStdPackagePath() {
    തിരികെ V3Os::filenameJoin(getenv_VERILATOR_വേര്(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    തിരികെ V3Os::filenameJoin(getenv_VERILATOR_വേര്(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(സ്ഥിരം string& var) {
    // If update below, also update V3Options::showVersion()
    എങ്കിൽ (var == "COROUTINES" && coroutineSupport()) {
        തിരികെ "1";
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "TSAN" && tsanSupport()) {
        തിരികെ "1";
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "DEV_ASAN" && devAsan()) {
        തിരികെ "1";
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "DEV_GCOV" && devGcov()) {
        തിരികെ "1";
        // cppcheck-suppress knownConditionTrueFalse
    } അല്ലെങ്കിൽ എങ്കിൽ (var == "SYSTEMC" && systemCFound()) {
        തിരികെ "1";
    } അല്ലെങ്കിൽ {
        തിരികെ "";
    }
}

തർക്കം V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    തിരികെ സത്യം;
#അല്ലെങ്കിൽ
    തിരികെ അസത്യം;
#endif
}

തർക്കം V3Options::systemCFound() {
    തിരികെ (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

തർക്കം V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    തിരികെ സത്യം;
#അല്ലെങ്കിൽ
    തിരികെ അസത്യം;
#endif
}

തർക്കം V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    തിരികെ സത്യം;
#അല്ലെങ്കിൽ
    തിരികെ അസത്യം;
#endif
}

തർക്കം V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    തിരികെ സത്യം;
#അല്ലെങ്കിൽ
    തിരികെ അസത്യം;
#endif
}

തർക്കം V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    തിരികെ സത്യം;
#അല്ലെങ്കിൽ
    തിരികെ അസത്യം;
#endif
}

//######################################################################
// V3 Options notification methods

ശൂന്യം V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_വരി* സ്ഥിരം cmdfl = പുതിയ File_വരി{File_വരി::command_വരി_Filename()};

    എങ്കിൽ (!outFormatOk() && v3Global.opt.പ്രധാന()) ccSet();  // --main implies --cc if not provided
    എങ്കിൽ (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    എങ്കിൽ (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    എങ്കിൽ (m_build) {
        എങ്കിൽ (m_ദ്വിമാന)
            backendFlags.emplace_back("--binary");
        അല്ലെങ്കിൽ
            backendFlags.emplace_back("--build");
    }
    എങ്കിൽ (m_preprocOnly) backendFlags.emplace_back("-E");
    എങ്കിൽ (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    എങ്കിൽ (m_lintOnly) backendFlags.emplace_back("--lint-only");
    എങ്കിൽ (m_jsonOnly) backendFlags.emplace_back("--json-only");
    എങ്കിൽ (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        വേണ്ടി (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    എങ്കിൽ (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    എങ്കിൽ (!m_gmake && !m_makeJson) m_gmake = സത്യം;

    എങ്കിൽ (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    എങ്കിൽ (m_hierChild) {
        എങ്കിൽ (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = അസത്യം;
    }

    എങ്കിൽ (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = സത്യം;  // No format, with --trace means wanted --trace-vcd
    }
    എങ്കിൽ (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = സത്യം;
    സ്ഥിരം പൂർണ്ണസംഖ്യ ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    എങ്കിൽ (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    എങ്കിൽ (protectIds()) {
        എങ്കിൽ (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        എങ്കിൽ (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        എങ്കിൽ (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    എങ്കിൽ (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    എങ്കിൽ (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    എങ്കിൽ (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(സത്യം);

    // Default split limits if not specified
    എങ്കിൽ (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    എങ്കിൽ (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    എങ്കിൽ (v3Global.opt.പ്രധാന() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    എങ്കിൽ (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        എങ്കിൽ (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    എങ്കിൽ (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    എങ്കിൽ (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    എങ്കിൽ (!m_dumpLevel.എണ്ണം("tree") && m_dumpLevel.എണ്ണം("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    എങ്കിൽ (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    എങ്കിൽ (m_buildJobs == -1) m_buildJobs = 1;
    എങ്കിൽ (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    എങ്കിൽ (timing().isSetTrue()) V3PreShell::define_Cmd_വരി("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    എങ്കിൽ (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(അസത്യം);

    // === Leave last
    // Mark options as available
    m_available = സത്യം;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    തിരികെ ver;
}

string V3Options::protect_താക്കോൽ_Defaulted() VL_MT_SAFE {
    സ്ഥിതം V3Mutex s_mutex;
    സ്ഥിരം V3LockGuard lock{s_mutex};
    എങ്കിൽ (m_protect_താക്കോൽ.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_താക്കോൽ = "VL-KEY-" + digest.digestBase64();
    }
    തിരികെ m_protect_താക്കോൽ;
}

ശൂന്യം V3Options::throwSigsegv() {  // LCOV_EXCL_START
#എങ്കിൽ !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<അസ്ഥിര അക്ഷരം*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(സ്ഥിരം VTimescale& flag) സ്ഥിരം {
    എങ്കിൽ (!timeOverridePrec().isNone()) {
        തിരികെ timeOverridePrec();
    } അല്ലെങ്കിൽ എങ്കിൽ (flag.isNone()) {
        തിരികെ timeDefaultPrec();
    } അല്ലെങ്കിൽ {
        തിരികെ flag;
    }
}

VTimescale V3Options::timeComputeUnit(സ്ഥിരം VTimescale& flag) സ്ഥിരം {
    എങ്കിൽ (!timeOverrideUnit().isNone()) {
        തിരികെ timeOverrideUnit();
    } അല്ലെങ്കിൽ എങ്കിൽ (flag.isNone()) {
        തിരികെ timeDefaultUnit();
    } അല്ലെങ്കിൽ {
        തിരികെ flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv) {
    // Return list of arguments as simple string
    string opts;
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < argc; ++i) {
        എങ്കിൽ (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    തിരികെ opts;
}

//######################################################################
// V3 Options Parsing

ശൂന്യം V3Options::parseOpts(File_വരി* fl, പൂർണ്ണസംഖ്യ argc, അക്ഷരം** argv) VL_MT_DISABLED {
    // Save command line options
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < argc; ++i) add_വരി_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    എങ്കിൽ (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    എങ്കിൽ (prefix() == "" && മുകൾ_Module() != "") m_prefix = "V"s + മുകൾ_Module();  // Already encoded
    എങ്കിൽ (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + Ast_കണ്ണി::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    എങ്കിൽ (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

തർക്കം V3Options::suffixed(സ്ഥിരം string& sw, സ്ഥിരം അക്ഷരം* arg) {
    എങ്കിൽ (std::strlen(arg) > sw.length()) തിരികെ അസത്യം;
    തിരികെ (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

ശൂന്യം V3Options::parseOptsList(File_വരി* fl, സ്ഥിരം string& optdir, പൂർണ്ണസംഖ്യ argc,
                              അക്ഷരം** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    സ്ഥിരം V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    സ്ഥിരം auto callStrSetter = [ഇത്](ശൂന്യം (V3Options::*cbStr)(സ്ഥിരം string&)) {
        തിരികെ [ഇത്, cbStr](സ്ഥിരം string& v) { (ഇത്->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) VL_MT_DISABLED {
        addDefine(optp, സത്യം);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [ഇത്, &optdir](സ്ഥിരം അക്ഷരം* optp) {
        string dirs = optp;
        string::size_type pos;
        ആകുമ്പോൾ ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        string exts = optp;
        string::size_type pos;
        ആകുമ്പോൾ ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](തർക്കം) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [ഇത്](തർക്കം flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](സ്ഥിരം അക്ഷരം*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [ഇത്](തർക്കം flag) {
        m_bboxUnsup = flag;
        File_വരി::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, സത്യം);
    });
    DECL_OPTION("-binary", CbCall, [ഇത്]() {
        m_ദ്വിമാന = സത്യം;
        m_build = സത്യം;
        m_exe = സത്യം;
        m_main = സത്യം;
        എങ്കിൽ (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        പൂർണ്ണസംഖ്യ val = std::atoi(valp);
        എങ്കിൽ (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [ഇത്]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](സ്ഥിരം std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](സ്ഥിരം std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](പൂർണ്ണസംഖ്യ val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        എങ്കിൽ (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } അല്ലെങ്കിൽ {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [ഇത്](തർക്കം flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_വ്യഞ്ജകം);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_വ്യഞ്ജകം_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* സ്ഥിരം valp) {
        എങ്കിൽ (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } അല്ലെങ്കിൽ {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_വരി);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    സ്ഥിരം auto parseBinsLimit = [fl](സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* valp, uint32_t& limitr) {
        അക്ഷരം* endp = nullptr;
        സ്ഥിരം unsigned long long മൂല്യം = std::strtoull(valp, &endp, 10);
        എങ്കിൽ (*endp || മൂല്യം < 1 || മൂല്യം > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            തിരികെ;
        }
        limitr = static_cast<uint32_t>(മൂല്യം);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [ഇത്, parseBinsLimit](സ്ഥിരം അക്ഷരം* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [ഇത്, parseBinsLimit](സ്ഥിരം അക്ഷരം* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* valp) VL_MT_DISABLED {
        addDefine(valp, അസത്യം);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [ഇത്]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [ഇത്](പൂർണ്ണസംഖ്യ v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [ഇത്](സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* valp) {
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_അട്ടി_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [ഇത്, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [ഇത്, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = സത്യം;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [ഇത്](സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [ഇത്](തർക്കം flag) {
        എങ്കിൽ (flag) {
            m_stdPackage = അസത്യം;
            m_stdWaiver = അസത്യം;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<ശൂന്യം (*)(പൂർണ്ണസംഖ്യ)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [ഇത്](സ്ഥിരം അക്ഷരം* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [ഇത്, fl, &optdir](സ്ഥിരം അക്ഷരം* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), സത്യം);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [ഇത്, &optdir](സ്ഥിരം അക്ഷരം* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [ഇത്, fl, &optdir](സ്ഥിരം അക്ഷരം* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), അസത്യം);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [ഇത്](സ്ഥിരം അക്ഷരം* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [ഇത്](സ്ഥിരം അക്ഷരം* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [ഇത്](തർക്കം flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_വൃക്ഷം = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_വൃക്ഷം);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_വൃക്ഷം);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [ഇത്](തർക്കം flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [ഇത്](തർക്കം flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](തർക്കം) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](തർക്കം) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](തർക്കം) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_തള്ളുക_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [ഇത്, fl](തർക്കം flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [ഇത്](തർക്കം flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [ഇത്](തർക്കം flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](തർക്കം) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](തർക്കം) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        എങ്കിൽ (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](തർക്കം) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) { addParameter(optp, അസത്യം); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [ഇത്]() {
        cout << protect_താക്കോൽ_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](സ്ഥിരം അക്ഷരം* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](സ്ഥിരം അക്ഷരം* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [ഇത്](സ്ഥിരം അക്ഷരം* valp) {
        സ്ഥിരം V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [ഇത്](സ്ഥിരം അക്ഷരം* optp) { m_hierParamsFile.തള്ളുക_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [ഇത്, &optdir](സ്ഥിരം അക്ഷരം* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [ഇത്, fl](പൂർണ്ണസംഖ്യ val) {
        m_instr_എണ്ണം_Dpi = val;
        എങ്കിൽ (m_instr_എണ്ണം_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [ഇത്](സ്ഥിരം അക്ഷരം* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = സത്യം;
    });
    DECL_OPTION("-json-only-output", CbVal, [ഇത്](സ്ഥിരം അക്ഷരം* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = സത്യം;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [ഇത്]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [ഇത്]() { m_l2Name = "v"; }).undocumented();  // Historical
    സ്ഥിരം auto setLang = [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        സ്ഥിരം V3LangCode optval{valp};
        എങ്കിൽ (optval.legal()) {
            m_defaultLanguage = optval;
        } അല്ലെങ്കിൽ {
            VSpellCheck spell;
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.തള്ളുക_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_സന്ദേശം(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [ഇത്](സ്ഥിരം അക്ഷരം* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [ഇത്](സ്ഥിരം അക്ഷരം* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_മുകൾ_Name);
    DECL_OPTION("-make", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        എങ്കിൽ (!std::strcmp(valp, "gmake")) {
            m_gmake = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "json")) {
            m_makeJson = സത്യം;
        } അല്ലെങ്കിൽ {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_അക്കം_Width);
    DECL_OPTION("-mod-prefix", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [ഇത്]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [ഇത്]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [ഇത്]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [ഇത്]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](തർക്കം /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_outputGroups = std::atoi(valp);
        എങ്കിൽ (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_outputGroupsSize = std::atoll(valp);
        എങ്കിൽ (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        എങ്കിൽ (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        എങ്കിൽ (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_വരി);
    DECL_OPTION("-pins64", CbCall, [ഇത്]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [ഇത്]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_pinsBv = std::atoi(valp);
        എങ്കിൽ (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [ഇത്](തർക്കം flag) {
        m_pinsScUint = flag;
        എങ്കിൽ (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [ഇത്](തർക്കം flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [ഇത്](തർക്കം flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        എങ്കിൽ (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [ഇത്]() { m_public = അസത്യം; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [ഇത്]() { m_profC = m_profCFuncs = സത്യം; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [ഇത്]() {
        m_profC = m_profCFuncs = സത്യം;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_താക്കോൽ);
    DECL_OPTION("-protect-lib", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = സത്യം;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [ഇത്](തർക്കം flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(സത്യം);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [ഇത്](തർക്കം flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [ഇത്](തർക്കം flag) {
        m_publicParams = flag;
        v3Global.dpi(സത്യം);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [ഇത്](സ്ഥിരം അക്ഷരം* varp) { addParameter(varp, അസത്യം); });

    DECL_OPTION("-quiet", CbOnOff, [ഇത്](തർക്കം flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_reloopLimit = std::atoi(valp);
        എങ്കിൽ (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_replicationLimit = std::atoi(valp);
        എങ്കിൽ (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [ഇത്, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [ഇത്]() {
        m_outFormatOk = സത്യം;
        m_systemC = സത്യം;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [ഇത്](തർക്കം flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [ഇത്](തർക്കം flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [ഇത്]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [ഇത്, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_threads = std::atoi(valp);
        എങ്കിൽ (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        എങ്കിൽ (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_hierThreads = std::atoi(valp);
        എങ്കിൽ (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        എങ്കിൽ (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = സത്യം;
            m_threadsDpiUnpure = സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = അസത്യം;
            m_threadsDpiUnpure = അസത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = സത്യം;
            m_threadsDpiUnpure = അസത്യം;
        } അല്ലെങ്കിൽ {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        എങ്കിൽ (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        എങ്കിൽ (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        എങ്കിൽ (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, സത്യം);
        എങ്കിൽ (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        എങ്കിൽ (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [ഇത്](സ്ഥിരം std::string& flag) { m_മുകൾ_Module = Ast_കണ്ണി::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [ഇത്](സ്ഥിരം std::string& flag) { m_മുകൾ_Module = Ast_കണ്ണി::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_മുകൾ_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [ഇത്]() { m_traceEnabledSaif = സത്യം; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [ഇത്]() {
        m_traceEnabledFst = സത്യം;
        addLdLibs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", Set, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", Set, &m_traceMaxWidth);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](സ്ഥിരം അക്ഷരം*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_മുകൾ);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [ഇത്]() { m_traceEnabledVcd = സത്യം; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_എണ്ണം).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [ഇത്]() {
        showVersion(സത്യം);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [ഇത്, &optdir](സ്ഥിരം അക്ഷരം* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        പൂർണ്ണസംഖ്യ val = std::atoi(valp);
        എങ്കിൽ (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } അല്ലെങ്കിൽ എങ്കിൽ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [ഇത്]() {
        showVersion(അസത്യം);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_വരി::globalWarnOff(V3ErrorCode::I_LINT, അസത്യം); });
    DECL_OPTION("-Werror-", CbPartialMatch, [ഇത്, fl](സ്ഥിരം അക്ഷരം* optp) {
        സ്ഥിരം V3ErrorCode code{optp};
        എങ്കിൽ (code == V3ErrorCode::EC_ERROR) {
            എങ്കിൽ (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } അല്ലെങ്കിൽ {
            V3Error::pretendError(code, സത്യം);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [ഇത്](സ്ഥിരം അക്ഷരം* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](സ്ഥിരം അക്ഷരം* optp) VL_MT_DISABLED {
        സ്ഥിരം string err = File_വരി::globalWarnOffParse(optp, സത്യം);
        എങ്കിൽ (!err.empty()) {
            സ്ഥിരം string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    വേണ്ടി (പൂർണ്ണസംഖ്യ i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        വേണ്ടി (സ്ഥിരം string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [ഇത്]() { m_context = അസത്യം; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(അസത്യം); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_വരി::globalWarnOff(V3ErrorCode::I_LINT, സത്യം); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_വരി::globalWarnOff(V3ErrorCode::I_STYLE, സത്യം); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [ഇത്]() {
        m_pedantic = സത്യം;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, അസത്യം);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [ഇത്, fl, &parser](സ്ഥിരം അക്ഷരം* optp) VL_MT_DISABLED {
        സ്ഥിരം V3ErrorCode code{optp};
        എങ്കിൽ (code == V3ErrorCode::EC_ERROR) {
            എങ്കിൽ (!isFuture(optp)) {
                സ്ഥിരം string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } അല്ലെങ്കിൽ {
            File_വരി::globalWarnOff(code, അസത്യം);
            V3Error::pretendError(code, അസത്യം);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_വരി::globalWarnOff(V3ErrorCode::I_LINT, അസത്യം); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_വരി::globalWarnOff(V3ErrorCode::I_STYLE, അസത്യം); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        എങ്കിൽ (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } അല്ലെങ്കിൽ {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [ഇത്, fl](സ്ഥിരം അക്ഷരം* valp) {
        എങ്കിൽ (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } അല്ലെങ്കിൽ എങ്കിൽ (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } അല്ലെങ്കിൽ {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [ഇത്, &optdir](സ്ഥിരം അക്ഷരം* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        എങ്കിൽ (argv[i][0] == '-' || argv[i][0] == '+') {
            സ്ഥിരം std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            പൂർണ്ണസംഖ്യ consumed = 0;
            തർക്കം isForRerun = സത്യം;

            // Special case for -j which has an optional argument
            എങ്കിൽ (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                പൂർണ്ണസംഖ്യ val = 0;
                എങ്കിൽ (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    എങ്കിൽ (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                എങ്കിൽ (m_buildJobs == -1) m_buildJobs = val;
                എങ്കിൽ (m_verilateJobs == -1) m_verilateJobs = val;
                എങ്കിൽ (m_outputGroups == -1) m_outputGroups = val;
            } അല്ലെങ്കിൽ {
                സ്ഥിരം auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                എങ്കിൽ (consumed) {
                    // Already processed
                } അല്ലെങ്കിൽ എങ്കിൽ (isFuture0(argName)) {
                    consumed = 1;
                } അല്ലെങ്കിൽ എങ്കിൽ (isFuture1(argName)) {
                    consumed = 2;
                } അല്ലെങ്കിൽ {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            എങ്കിൽ (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            തുടരുക;
        }

        // Filename
        സ്ഥിരം std::string filename = parseFileArg(optdir, argv[i]);
        തർക്കം isForRerun = സത്യം;
        എങ്കിൽ (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } അല്ലെങ്കിൽ എങ്കിൽ (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } അല്ലെങ്കിൽ എങ്കിൽ (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = അസത്യം;
        } അല്ലെങ്കിൽ {
            V3Options::addVFile(filename, work());
            isForRerun = അസത്യം;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    എങ്കിൽ (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

ശൂന്യം V3Options::parseOptsFile(File_വരി* fl, സ്ഥിരം string& filename, തർക്കം rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    സ്ഥിരം std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    എങ്കിൽ (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        തിരികെ;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    തർക്കം has_fargs = അസത്യം;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    സ്ഥിരം auto parse_fargs_cmt = [&](സ്ഥിരം അക്ഷരം* p) -> ശൂന്യം {
        സ്ഥിരം തർക്കം blockCmt = p[1] == '*';
        p += 2;
        ആകുമ്പോൾ (std::isspace(*p)) ++p;
        എങ്കിൽ (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            എങ്കിൽ (std::isspace(*p)) {
                ആകുമ്പോൾ (std::isspace(*p)) ++p;
                എങ്കിൽ (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    എങ്കിൽ (std::isspace(*p)) {
                        ആകുമ്പോൾ (std::isspace(*p)) ++p;
                        എങ്കിൽ (!has_fargs) {
                            has_fargs = സത്യം;
                            whole_file.clear();
                        }
                        whole_file += p;
                        എങ്കിൽ (blockCmt) {
                            സ്ഥിരം std::string::size_type pos = whole_file.find("*/");
                            എങ്കിൽ (pos == std::string::npos) {
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

    തർക്കം inCmt = അസത്യം;
    ആകുമ്പോൾ (!ifp->eof()) {
        സ്ഥിരം std::string വരി = V3Os::getline(*ifp);
        // Strip simple comments
        അക്ഷരം lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        തർക്കം space_begin = സത്യം;  // At beginning or leading spaces only
        വേണ്ടി (സ്ഥിരം അക്ഷരം* p = വരി.c_str(); *p; lastch = *p++) {
            എങ്കിൽ (inCmt) {
                എങ്കിൽ (p[0] == '*' && p[1] == '/') {
                    inCmt = അസത്യം;
                    ++p;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                നിർത്തുക;  // Ignore to EOL
            } അല്ലെങ്കിൽ എങ്കിൽ (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                നിർത്തുക;  // Ignore to EOL
            } അല്ലെങ്കിൽ എങ്കിൽ (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = സത്യം;
                space_begin = അസത്യം;
                ++p;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (!std::isspace(*p)) space_begin = അസത്യം;
                എങ്കിൽ (!has_fargs) whole_file += *p;
            }
        }
        എങ്കിൽ (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    എങ്കിൽ (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = പുതിയ File_വരി{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    എണ്ണൽ state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    വേണ്ടി (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        അക്ഷരം curr_char = whole_file[pos];
        തിരഞ്ഞെടുപ്പ് (st) {
        അവസ്ഥ ST_IN_OPTION:  // Get all chars up to a white space or a "="
            എങ്കിൽ (std::isspace(curr_char)) {  // End of option
                എങ്കിൽ (!arg.empty()) {  // End of word
                    args.തള്ളുക_back(arg);
                }
                arg = "";
                നിർത്തുക;
            }
            എങ്കിൽ (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                നിർത്തുക;
            }
            എങ്കിൽ (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                എങ്കിൽ (pos < whole_file.length()) curr_char = whole_file[pos];
                എങ്കിൽ (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } അല്ലെങ്കിൽ {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                നിർത്തുക;
            }
            എങ്കിൽ (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                നിർത്തുക;
            }
            arg += curr_char;
            നിർത്തുക;
        അവസ്ഥ ST_IN_QUOTED_STR:  // Just store all chars inside string
            എങ്കിൽ (curr_char != '\'') {
                arg += curr_char;
            } അല്ലെങ്കിൽ {  // End of quoted string
                st = ST_IN_OPTION;
            }
            നിർത്തുക;
        അവസ്ഥ ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            എങ്കിൽ (curr_char != '"') {
                എങ്കിൽ (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } അല്ലെങ്കിൽ {
                    arg += curr_char;
                }
            } അല്ലെങ്കിൽ {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            നിർത്തുക;
        അവസ്ഥ ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            നിർത്തുക;
        }
    }
    എങ്കിൽ (!arg.empty()) {  // Add last word
        args.തള്ളുക_back(arg);
    }

    // Path
    സ്ഥിരം string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<അക്ഷരം*> argv;
    argv.reserve(args.size() + 1);
    വേണ്ടി (സ്ഥിരം string& i : args) argv.തള്ളുക_back(const_cast<അക്ഷരം*>(i.c_str()));
    argv.തള്ളുക_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<പൂർണ്ണസംഖ്യ>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(സ്ഥിരം string& optdir, സ്ഥിരം string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    എങ്കിൽ (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    തിരികെ filename;
}

//======================================================================

ശൂന്യം V3Options::showVersion(തർക്കം verbose) {
    cout << version();
    cout << "\n";
    എങ്കിൽ (!verbose) തിരികെ;

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
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_വേര് << "\n";
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
    m_impp = പുതിയ V3OptionsImp;

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

ശൂന്യം V3Options::setDebugMode(പൂർണ്ണസംഖ്യ level) {
    V3Error::debugDefault(level);
    എങ്കിൽ (!m_dumpLevel.എണ്ണം("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = സത്യം;
    m_debugCheck = സത്യം;
    എങ്കിൽ (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(സ്ഥിരം string& tag) സ്ഥിരം VL_MT_SAFE {
    സ്ഥിരം auto iter = m_debugLevel.find(tag);
    തിരികെ iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(സ്ഥിരം string& srcfile_path) സ്ഥിരം VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    തിരികെ debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(സ്ഥിരം string& tag) സ്ഥിരം VL_MT_SAFE {
    സ്ഥിരം auto iter = m_dumpLevel.find(tag);
    തിരികെ iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(സ്ഥിരം string& srcfile_path) സ്ഥിരം VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    തിരികെ dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

തർക്കം V3Options::dump_വൃക്ഷം_Addrids() സ്ഥിരം VL_MT_SAFE {
    സ്ഥിതം പൂർണ്ണസംഖ്യ s_level = -1;
    എങ്കിൽ (VL_UNLIKELY(s_level < 0)) {
        സ്ഥിരം unsigned മൂല്യം = dumpLevel("tree-addrids");
        എങ്കിൽ (!available()) തിരികെ മൂല്യം > 0;
        s_level = static_cast<unsigned>(മൂല്യം);
    }
    തിരികെ s_level > 0;
}

ശൂന്യം V3Options::optimize(പൂർണ്ണസംഖ്യ level) {
    // Set all optimizations to on/off
    സ്ഥിരം തർക്കം flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_വൃക്ഷം = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_വൃക്ഷം = flag;
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
    എങ്കിൽ (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
