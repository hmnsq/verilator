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

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Options.h"

#ଅନ୍ତର୍ଭାଵ "V3Container.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Mutex.h"
#ଅନ୍ତର୍ଭାଵ "V3OptionParser.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3PreShell.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

// clang-format off
#ଅନ୍ତର୍ଭାଵ <sys/types.h>
#ଅନ୍ତର୍ଭାଵ <sys/stat.h>
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#ଅନ୍ତର୍ଭାଵ <algorithm>
#ଅନ୍ତର୍ଭାଵ <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#ଅନ୍ୟଥା
# include <dirent.h>
#endif
#ଅନ୍ତର୍ଭାଵ <fcntl.h>
#ଅନ୍ତର୍ଭାଵ <list>
#ଅନ୍ତର୍ଭାଵ <map>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <thread>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <string>

#ଅନ୍ତର୍ଭାଵ "config_rev.h"

#ଯଦି defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

ଶ୍ରେଣୀ V3OptionsImp final {
ସର୍ବସାଧାରଣ:
    // TYPES
    using DirMap = std::map<ଅଚଳ string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_ଧାଡ଼ି_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, ତାର୍କିକ>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<ଅଚଳ string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    ଶୂନ୍ୟ addIncDirUser(ଅଚଳ string& incdir) {
        ଅଚଳ string& dir = V3Os::filenameCleanup(incdir);
        ଅଚଳ ତାର୍କିକ inserted = m_incDirUsers.ଭର୍ତ୍ତି(dir);
        ଯଦି (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    ଶୂନ୍ୟ addIncDirFallback(ଅଚଳ string& incdir) {
        ଅଚଳ string& dir = V3Os::filenameCleanup(incdir);
        ଯଦି (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ଅଚଳ auto itFoundPair = m_incDirFallbackSet.ଭର୍ତ୍ତି(dir);
            ଯଦି (itFoundPair.second) m_incDirFallbacks.ଠେଲ_back(dir);
        }
    }
    ଶୂନ୍ୟ addLangExt(ଅଚଳ string& langext, ଅଚଳ V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        ଯଦି (addext[0] == '.') addext = addext.substr(1);
        (ଶୂନ୍ୟ)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    ଶୂନ୍ୟ addLibExtV(ଅଚଳ string& libext) { m_libExtVs.ଭର୍ତ୍ତି(libext); }
    V3OptionsImp() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~V3OptionsImp() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(ଅଚଳ ଅକ୍ଷର* textp) {
    // Return code for given string, or ERROR, which is a bad code
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        ଅଚଳ V3LangCode code{codei};
        ଯଦି (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            ଫେରନ୍ତୁ;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ଅଚଳ string& ମୂଲ୍ୟ, ତାର୍କିକ& badr)
    : m_e{VTimescale::NONE} {
    badr = ସତ୍ୟ;
    ଅଚଳ string spaceless = VString::removeWhitespace(ମୂଲ୍ୟ);
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = TS_100S; i < _ENUM_END; ++i) {
        ଅଚଳ VTimescale ts{i};
        ଯଦି (spaceless == ts.ascii()) {
            badr = ମିଥ୍ୟା;
            m_e = ts.m_e;
            ଭାଙ୍ଗ;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(ଅଚଳ string& opts) {
    VStringList vals;
    ତାର୍କିକ inStr = ମିଥ୍ୟା;
    string cur;
    ସ୍ଥିର ଅଚଳ string hierBlock("--hierarchical-block");
    File_ଧାଡ଼ି cmdfl(File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    ପାଇଁ (string::const_iterator it = opts.begin(); it != opts.end();) {
        ଯଦି (inStr) {
            ଯଦି (*it == '\\') {
                ++it;
                ଯଦି (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    ଭାଙ୍ଗ;
                }
                ଯଦି (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    ଭାଙ୍ଗ;
                }
                cur.ଠେଲ_back(*it);
                ++it;
            } ଅନ୍ୟଥା ଯଦି (*it == '"') {  // end of string
                cur.ଠେଲ_back(*it);
                vals.ଠେଲ_back(cur);
                cur.clear();
                ++it;
                ଯଦି (it != opts.end()) {
                    ଯଦି (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        ଭାଙ୍ଗ;
                    }
                    ++it;
                    ଯଦି (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        ଭାଙ୍ଗ;
                    }
                    inStr = *it == '"';
                    cur.ଠେଲ_back(*it);
                    ++it;
                }
            } ଅନ୍ୟଥା {
                cur.ଠେଲ_back(*it);
                ++it;
            }
        } ଅନ୍ୟଥା {
            ଯଦି (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                ଭାଙ୍ଗ;
            }
            ଯଦି (*it == ',') {  // end of this parameter
                vals.ଠେଲ_back(cur);
                cur.clear();
                ++it;
                ଯଦି (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    ଭାଙ୍ଗ;
                }
                inStr = *it == '"';
            }
            cur.ଠେଲ_back(*it);
            ++it;
        }
    }
    ଯଦି (!cur.empty()) vals.ଠେଲ_back(cur);
    ଯଦି (vals.size() >= 2) {
        ଯଦି (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } ଅନ୍ୟଥା {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    ପାଇଁ (size_t i = 2; i + 1 < vals.size(); i += 2) {
        ଅଚଳ ତାର୍କିକ inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        ଯଦି (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

ଶୂନ୍ୟ VTimescale::parseSlashed(File_ଧାଡ଼ି* fl, ଅଚଳ ଅକ୍ଷର* textp, VTimescale& unitr,
                              VTimescale& precr, ତାର୍କିକ allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ଅଚଳ ଅକ୍ଷର* cp = textp;
    ପାଇଁ (; std::isspace(*cp); ++cp) {}
    ଅଚଳ ଅକ୍ଷର* ଅଚଳ unitp = cp;
    ପାଇଁ (; *cp && *cp != '/'; ++cp) {}
    ଅଚଳ string unitStr(unitp, cp - unitp);
    ପାଇଁ (; std::isspace(*cp); ++cp) {}
    string precStr;
    ଯଦି (*cp == '/') {
        ++cp;
        ପାଇଁ (; std::isspace(*cp); ++cp) {}
        ଅଚଳ ଅକ୍ଷର* ଅଚଳ precp = cp;
        ପାଇଁ (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    ପାଇଁ (; std::isspace(*cp); ++cp) {}
    ଯଦି (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        ଫେରନ୍ତୁ;
    }

    ତାର୍କିକ unitbad = ମିଥ୍ୟା;
    ଅଚଳ VTimescale unit{unitStr, unitbad /*ref*/};
    ଯଦି (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        ଫେରନ୍ତୁ;
    }
    unitr = unit;

    ଯଦି (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        ତାର୍କିକ precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        ଯଦି (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            ଫେରନ୍ତୁ;
        }
        ଯଦି (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            ଫେରନ୍ତୁ;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

ଶୂନ୍ୟ V3Options::addIncDirUser(ଅଚଳ string& incdir) { m_impp->addIncDirUser(incdir); }
ଶୂନ୍ୟ V3Options::addIncDirFallback(ଅଚଳ string& incdir) { m_impp->addIncDirFallback(incdir); }
ଶୂନ୍ୟ V3Options::addLangExt(ଅଚଳ string& langext, ଅଚଳ V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
ଶୂନ୍ୟ V3Options::addLibExtV(ଅଚଳ string& libext) { m_impp->addLibExtV(libext); }
ଶୂନ୍ୟ V3Options::addDefine(ଅଚଳ string& defline, ତାର୍କିକ allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ବାମ = defline;
    ଯେତେବେଳେ (ବାମ != "") {
        string def = ବାମ;
        string::size_type pos;
        ଯଦି (allowPlus && ((pos = ବାମ.find('+')) != string::npos)) {
            ବାମ = ବାମ.substr(pos + 1);
            def.erase(pos);
        } ଅନ୍ୟଥା {
            ବାମ = "";
        }
        string ମୂଲ୍ୟ;
        ଯଦି ((pos = def.find('=')) != string::npos) {
            ମୂଲ୍ୟ = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_ଧାଡ଼ି(def, ମୂଲ୍ୟ);
    }
}
ଶୂନ୍ୟ V3Options::addParameter(ଅଚଳ string& paramline, ତାର୍କିକ allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ବାମ = paramline;
    ଯେତେବେଳେ (ବାମ != "") {
        string param = ବାମ;
        string::size_type pos;
        ଯଦି (allowPlus && ((pos = ବାମ.find('+')) != string::npos)) {
            ବାମ = ବାମ.substr(pos + 1);
            param.erase(pos);
        } ଅନ୍ୟଥା {
            ବାମ = "";
        }
        string ମୂଲ୍ୟ;
        ଯଦି ((pos = param.find('=')) != string::npos) {
            ମୂଲ୍ୟ = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << ମୂଲ୍ୟ);
        (ଶୂନ୍ୟ)m_parameters.erase(param);
        m_parameters[param] = ମୂଲ୍ୟ;
    }
}

ତାର୍କିକ V3Options::hasParameter(ଅଚଳ string& name) {
    ଫେରନ୍ତୁ m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(ଅଚଳ string& name) {
    string ମୂଲ୍ୟ = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    ଫେରନ୍ତୁ ମୂଲ୍ୟ;
}

ଶୂନ୍ୟ V3Options::checkParameters() {
    ଯଦି (!m_parameters.empty()) {
        std::stringstream ସନ୍ଦେଶ;
        ସନ୍ଦେଶ << "Parameters from the command line were not found in the design:";
        ପାଇଁ (ଅଚଳ auto& i : m_parameters) ସନ୍ଦେଶ << " " << i.first;
        v3error(ସନ୍ଦେଶ.str());
    }
}

ଶୂନ୍ୟ V3Options::addCppFile(ଅଚଳ string& filename) { m_cppFiles.ଭର୍ତ୍ତି(filename); }
ଶୂନ୍ୟ V3Options::addCFlags(ଅଚଳ string& filename) { m_cFlags.ଠେଲ_back(filename); }
ଶୂନ୍ୟ V3Options::addLibMapFile(ଅଚଳ string& filename) { m_libmapFiles.ଭର୍ତ୍ତି(filename); }
ଶୂନ୍ୟ V3Options::addCompilerIncludes(ଅଚଳ string& filename) {
    m_compilerIncludes.ଭର୍ତ୍ତି(filename);
}
ଶୂନ୍ୟ V3Options::addLdLibFile(ଅଚଳ string& filename) {
    m_ldLibFiles.ଠେଲ_back(filename);
    addLdLibs(filename);
}
ଶୂନ୍ୟ V3Options::addLdLibs(ଅଚଳ string& filename) { m_ldLibs.ଠେଲ_back(filename); }
ଶୂନ୍ୟ V3Options::addMakeFlags(ଅଚଳ string& filename) { m_makeFlags.ଠେଲ_back(filename); }
ଶୂନ୍ୟ V3Options::addFuture(ଅଚଳ string& flag) { m_futures.ଭର୍ତ୍ତି(flag); }
ଶୂନ୍ୟ V3Options::addFuture0(ଅଚଳ string& flag) { m_future0s.ଭର୍ତ୍ତି(flag); }
ଶୂନ୍ୟ V3Options::addFuture1(ଅଚଳ string& flag) { m_future1s.ଭର୍ତ୍ତି(flag); }
ତାର୍କିକ V3Options::isFuture(ଅଚଳ string& flag) ଅଚଳ {
    ଫେରନ୍ତୁ m_futures.find(flag) != m_futures.end();
}
ତାର୍କିକ V3Options::isFuture0(ଅଚଳ string& flag) ଅଚଳ {
    ଫେରନ୍ତୁ m_future0s.find(flag) != m_future0s.end();
}
ତାର୍କିକ V3Options::isFuture1(ଅଚଳ string& flag) ଅଚଳ {
    ଫେରନ୍ତୁ m_future1s.find(flag) != m_future1s.end();
}
ତାର୍କିକ V3Options::isLibraryFile(ଅଚଳ string& filename, ଅଚଳ string& libname) ଅଚଳ {
    ଫେରନ୍ତୁ m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
ଶୂନ୍ୟ V3Options::addLibraryFile(ଅଚଳ string& filename, ଅଚଳ string& libname) {
    m_libraryFiles.ଭର୍ତ୍ତି({filename, libname});
}
ଶୂନ୍ୟ V3Options::addVFile(ଅଚଳ string& filename, ଅଚଳ string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ଠେଲ_back({filename, libname});  // Not emplace
}
ଶୂନ୍ୟ V3Options::addVltFile(ଅଚଳ string& filename, ଅଚଳ string& libname) {
    m_vltFiles.ଭର୍ତ୍ତି({filename, libname});
}
ଶୂନ୍ୟ V3Options::addForceInc(ଅଚଳ string& filename) { m_forceIncs.ଠେଲ_back(filename); }

ଶୂନ୍ୟ V3Options::add_ଧାଡ଼ି_Arg(ଅଚଳ string& arg) { m_impp->m_ଧାଡ଼ି_Args.ଠେଲ_back(arg); }

ଶୂନ୍ୟ V3Options::addArg(ଅକ୍ଷର** argv, size_t ଗଣନା_2, ତାର୍କିକ isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, ତାର୍କିକ>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    ପାଇଁ (size_t n = 0; n < ଗଣନା_2; ++n) pair.first.emplace_back(argv[n]);
}

ଶୂନ୍ୟ V3Options::addArg(ଅଚଳ std::string& arg, ତାର୍କିକ isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, ତାର୍କିକ>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

ଅଚଳ std::list<std::pair<std::list<std::string>, ତାର୍କିକ>>& V3Options::allArgs() ଅଚଳ {
    ଫେରନ୍ତୁ m_impp->m_allArgs;
}

string V3Options::allArgsString() ଅଚଳ VL_MT_SAFE {
    string result;
    ପାଇଁ (ଅଚଳ auto& pair : m_impp->m_allArgs) {
        ପାଇଁ (ଅଚଳ string& arg : pair.first) {
            ଯଦି (!result.empty()) result += " ";
            result += arg;
        }
    }
    ଫେରନ୍ତୁ result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(ତାର୍କିକ for_ଶୀର୍ଷ) ଅଚଳ {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    ପାଇଁ (ଅଚଳ auto& vFile : m_vFiles) vFiles.ଭର୍ତ୍ତି(vFile.filename());
    string out;
    ତାର୍କିକ stripArg = ମିଥ୍ୟା;
    ତାର୍କିକ strip_Arg_If_ଅଙ୍କ = ମିଥ୍ୟା;
    ପାଇଁ (ଅଚଳ auto& pair : m_impp->m_allArgs) {
        ପାଇଁ (ଅଚଳ string& arg : pair.first) {
            ଯଦି (stripArg) {
                stripArg = ମିଥ୍ୟା;
                ଜାରି;
            }
            ଯଦି (strip_Arg_If_ଅଙ୍କ) {
                strip_Arg_If_ଅଙ୍କ = ମିଥ୍ୟା;
                ଯଦି (isdigit(arg[0])) ଜାରି;
            }
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା skip = 0;
            ଯଦି (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } ଅନ୍ୟଥା ଯଦି (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            ଯଦି (skip > 0) {  // arg is an option
                ଅଚଳ string opt = arg.substr(skip);  // Remove '-' in the beginning
                ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଅଙ୍କ_Strip = stripOptionsForChildRun(opt, for_ଶୀର୍ଷ);
                ଯଦି (ଅଙ୍କ_Strip) {
                    UASSERT(0 <= ଅଙ୍କ_Strip && ଅଙ୍କ_Strip <= 3, "should be one of 0, 1, 2, 3");
                    ଯଦି (ଅଙ୍କ_Strip == 2) stripArg = ସତ୍ୟ;
                    ଯଦି (ଅଙ୍କ_Strip == 3) strip_Arg_If_ଅଙ୍କ = ସତ୍ୟ;
                    ଜାରି;
                }
            } ଅନ୍ୟଥା {  // Not an option
                ଯଦି (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    ଜାରି;
                }
            }
            ଯଦି (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    ଫେରନ୍ତୁ out;
}

ଶୂନ୍ୟ V3Options::ccSet() {  // --cc
    m_outFormatOk = ସତ୍ୟ;
    m_systemC = ମିଥ୍ୟା;
}

ଶୂନ୍ୟ V3Options::decorations(File_ଧାଡ଼ି* fl, ଅଚଳ string& arg) {  // --decorations
    ଯଦି (arg == "none") {
        m_decoration = ମିଥ୍ୟା;
        m_decorationNodes = ମିଥ୍ୟା;
    } ଅନ୍ୟଥା ଯଦି (arg == "node") {
        m_decoration = ସତ୍ୟ;
        m_decorationNodes = ସତ୍ୟ;
    } ଅନ୍ୟଥା ଯଦି (arg == "medium") {
        m_decoration = ସତ୍ୟ;
        m_decorationNodes = ମିଥ୍ୟା;
    } ଅନ୍ୟଥା {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() ଅଚଳ VL_MT_SAFE {
    std::vector<std::string> result;
    ଯଦି (traceEnabledFst()) result.emplace_back("VerilatedFst");
    ଯଦି (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    ଯଦି (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    ଫେରନ୍ତୁ result;
}
std::vector<std::string> V3Options::traceClassLangs() ଅଚଳ VL_MT_SAFE {
    std::vector<std::string> result;
    ପାଇଁ (ଅଚଳ auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    ଫେରନ୍ତୁ result;
}
std::vector<std::string> V3Options::traceSourceBases() ଅଚଳ VL_MT_SAFE {
    std::vector<std::string> result;
    ଯଦି (traceEnabledFst()) result.emplace_back("verilated_fst");
    ଯଦି (traceEnabledSaif()) result.emplace_back("verilated_saif");
    ଯଦି (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    ଫେରନ୍ତୁ result;
}
std::vector<std::string> V3Options::traceSourceLangs() ଅଚଳ VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    ପାଇଁ (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    ଫେରନ୍ତୁ result;
}
std::string V3Options::traceClassBase() ଅଚଳ VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    ଫେରନ୍ତୁ traceClassBases().front();
}
std::string V3Options::traceClassLang() ଅଚଳ VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    ଫେରନ୍ତୁ traceClassLangs().front();
}

//######################################################################
// File searching

ତାର୍କିକ V3Options::fileStatNormal(ଅଚଳ string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ସଂରଚନା stat sstat;  // Stat information
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା err = stat(filename.c_str(), &sstat);
    ଯଦି (err != 0) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଯଦି (S_ISDIR(sstat.st_mode)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ ସତ୍ୟ;
}

string V3Options::fileExists(ଅଚଳ string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ଅଚଳ string dir = V3Os::filenameDir(filename);
    ଅଚଳ string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    ଯଦି (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* ଅଚଳ setp = &(diriter->second);

#ifdef _MSC_VER
        ଚେଷ୍ଟା {
            ପାଇଁ (ଅଚଳ auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->ଭର୍ତ୍ତି(dirEntry.path().filename().string());
        } ଧର (std::filesystem::filesystem_error ଅଚଳ& ex) {
            (ଶୂନ୍ୟ)ex;
            ଫେରନ୍ତୁ "";
        }
#ଅନ୍ୟଥା
        ଯଦି (DIR* ଅଚଳ dirp = opendir(dir.c_str())) {
            ଯେତେବେଳେ (ସଂରଚନା dirent* direntp = readdir(dirp)) setp->ଭର୍ତ୍ତି(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    ଅଚଳ std::set<string>* ଅଚଳ filesetp = &(diriter->second);
    ଅଚଳ auto fileiter = filesetp->find(basename);
    ଯଦି (fileiter == filesetp->end()) ଫେରନ୍ତୁ "";  // Not found
    // Check if it is a directory, ignore if so
    ଅଚଳ string filenameOut = V3Os::filenameJoin(dir, basename);
    ଯଦି (!fileStatNormal(filenameOut)) ଫେରନ୍ତୁ "";  // Directory
    ଫେରନ୍ତୁ filenameOut;
}

string V3Options::filePathCheckOneDir(ଅଚଳ string& modname, ଅଚଳ string& dirname) {
    ପାଇଁ (ଅଚଳ string& i : m_impp->m_libExtVs) {
        ଅଚଳ string fn = V3Os::filenameJoin(dirname, modname + i);
        ଅଚଳ string exists = fileExists(fn);
        ଯଦି (exists != "") ଫେରନ୍ତୁ exists;
    }
    ଫେରନ୍ତୁ "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3Options::stripOptionsForChildRun(ଅଚଳ string& opt, ତାର୍କିକ for_ଶୀର୍ଷ) {
    // Options to strip for both the top wrapper, and hier blocks
    ସ୍ଥିର ଅଚଳ std::unordered_map<std::string, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> commonOpts{
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
    ଯଦି (commonOpts.ଗଣନା_2(opt)) ଫେରନ୍ତୁ commonOpts.at(opt);

    // Options to strip only for hier blocks
    ଯଦି (!for_ଶୀର୍ଷ) {
        ସ୍ଥିର ଅଚଳ std::unordered_map<std::string, ପୂର୍ଣ୍ଣସଂଖ୍ୟା> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        ଯଦି (subOpts.ଗଣନା_2(opt)) ଫେରନ୍ତୁ subOpts.at(opt);
        ଯଦି (VString::startsWith(opt, "G")) ଫେରନ୍ତୁ 1;
        ଯଦି (VString::startsWith(opt, "pvalue")) ଫେରନ୍ତୁ 1;
    }

    // Do not strip
    ଫେରନ୍ତୁ 0;
}

ଶୂନ୍ୟ V3Options::validateIdentifier(File_ଧାଡ଼ି* fl, ଅଚଳ string& arg, ଅଚଳ string& opt) {
    ଯଦି (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_ଧାଡ଼ି* fl, ଅଚଳ string& modname, ଅଚଳ string& lastpath,
                           ଅଚଳ string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ଅଚଳ string filename = V3Os::filenameCleanup(VName::dehash(modname));
    ଯଦି (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ଅଚଳ string exists = filePathCheckOneDir(filename, "");
        ଯଦି (exists != "") ଫେରନ୍ତୁ exists;
    }
    ପାଇଁ (ଅଚଳ string& dir : m_impp->m_incDirUsers) {
        ଅଚଳ string exists = filePathCheckOneDir(filename, dir);
        ଯଦି (exists != "") ଫେରନ୍ତୁ exists;
    }
    ପାଇଁ (ଅଚଳ string& dir : m_impp->m_incDirFallbacks) {
        ଅଚଳ string exists = filePathCheckOneDir(filename, dir);
        ଯଦି (exists != "") ଫେରନ୍ତୁ exists;
    }

    ଯଦି (m_relativeIncludes) {
        ଅଚଳ string exists = filePathCheckOneDir(filename, lastpath);
        ଯଦି (exists != "") ଫେରନ୍ତୁ V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    ଯଦି (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_ସନ୍ଦେଶ(fl, filename));
    }
    ଫେରନ୍ତୁ "";
}

string V3Options::file_Path_Looked_ସନ୍ଦେଶ(File_ଧାଡ଼ି* /*fl*/, ଅଚଳ string& modname) {
    ସ୍ଥିର ତାର୍କିକ s_shown_notfound_ସନ୍ଦେଶ = ମିଥ୍ୟା;
    std::ostringstream ss;
    ଯଦି (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } ଅନ୍ୟଥା ଯଦି (!s_shown_notfound_ସନ୍ଦେଶ) {
        s_shown_notfound_ସନ୍ଦେଶ = ସତ୍ୟ;
        ଯଦି (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        ପାଇଁ (ଅଚଳ string& dir : m_impp->m_incDirUsers) {
            ପାଇଁ (ଅଚଳ string& ext : m_impp->m_libExtVs) {
                ଅଚଳ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ପାଇଁ (ଅଚଳ string& dir : m_impp->m_incDirFallbacks) {
            ପାଇଁ (ଅଚଳ string& ext : m_impp->m_libExtVs) {
                ଅଚଳ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    ଫେରନ୍ତୁ ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(ଅଚଳ string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    ଯଦି (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        ଫେରନ୍ତୁ V3LangCode::mostRecent();
    } ଅନ୍ୟଥା ଯଦି ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ଅଚଳ auto it = m_impp->m_langExts.find(ext);
        ଯଦି (it != m_impp->m_langExts.end()) ଫେରନ୍ତୁ it->second;
    }
    ଫେରନ୍ତୁ m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ଅଚଳ string& var) {
    // If update below, also update V3Options::showVersion()
    ଯଦି (var == "MAKE") {
        ଫେରନ୍ତୁ getenvMAKE();
    } ଅନ୍ୟଥା ଯଦି (var == "PERL") {
        ଫେରନ୍ତୁ getenvPERL();
    } ଅନ୍ୟଥା ଯଦି (var == "PYTHON3") {
        ଫେରନ୍ତୁ getenvPYTHON3();
    } ଅନ୍ୟଥା ଯଦି (var == "SYSTEMC") {
        ଫେରନ୍ତୁ getenvSYSTEMC();
    } ଅନ୍ୟଥା ଯଦି (var == "SYSTEMC_ARCH") {
        ଫେରନ୍ତୁ getenvSYSTEMC_ARCH();
    } ଅନ୍ୟଥା ଯଦି (var == "SYSTEMC_INCLUDE") {
        ଫେରନ୍ତୁ getenvSYSTEMC_INCLUDE();
    } ଅନ୍ୟଥା ଯଦି (var == "SYSTEMC_LIBDIR") {
        ଫେରନ୍ତୁ getenvSYSTEMC_LIBDIR();
    } ଅନ୍ୟଥା ଯଦି (var == "VERILATOR_ROOT") {
        ଫେରନ୍ତୁ getenv_VERILATOR_ମୂଳ();
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { ଫେରନ୍ତୁ V3Os::getenvStr("MAKE", "gmake"); }
#ଅନ୍ୟଥା
string V3Options::getenvMAKE() { ଫେରନ୍ତୁ V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    ଫେରନ୍ତୁ V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    ଫେରନ୍ତୁ V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    ଫେରନ୍ତୁ V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ଅଚଳ string defenv = string{DEFENV_SYSTEMC};
    ଯଦି (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    ଫେରନ୍ତୁ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ଅଚଳ string defenv = string{DEFENV_SYSTEMC_ARCH};
    ଯଦି (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    ଯଦି (var == "") {
#ଯଦି defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ଅଚଳ string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#ନଚେତଯଦି defined(_WIN32)
        ଅଚଳ string sysname = "WIN32";
        var = "win32";
#ଅନ୍ୟଥା
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ସଂରଚନା utsname uts;
        uname(&uts);
        ଅଚଳ string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        ଯଦି (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } ଅନ୍ୟଥା ଯଦି (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } ଅନ୍ୟଥା {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    ଫେରନ୍ତୁ var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ଅଚଳ string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    ଯଦି (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    ଯଦି (var == "") {
        ଅଚଳ string sc = getenvSYSTEMC();
        ଯଦି (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    ଫେରନ୍ତୁ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ଅଚଳ string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    ଯଦି (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    ଯଦି (var == "") {
        ଅଚଳ string sc = getenvSYSTEMC();
        ଅଚଳ string arch = getenvSYSTEMC_ARCH();
        ଯଦି (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    ଫେରନ୍ତୁ V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_ମୂଳ() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ଅଚଳ string defenv = string{DEFENV_VERILATOR_ମୂଳ};
    ଯଦି (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    ଯଦି (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    ଫେରନ୍ତୁ V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ଅଚଳ string defenv = string{DEFENV_VERILATOR_SOLVER};
    ଯଦି (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    ଫେରନ୍ତୁ var;
}

string V3Options::getStdPackagePath() {
    ଫେରନ୍ତୁ V3Os::filenameJoin(getenv_VERILATOR_ମୂଳ(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    ଫେରନ୍ତୁ V3Os::filenameJoin(getenv_VERILATOR_ମୂଳ(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ଅଚଳ string& var) {
    // If update below, also update V3Options::showVersion()
    ଯଦି (var == "COROUTINES" && coroutineSupport()) {
        ଫେରନ୍ତୁ "1";
    } ଅନ୍ୟଥା ଯଦି (var == "TSAN" && tsanSupport()) {
        ଫେରନ୍ତୁ "1";
    } ଅନ୍ୟଥା ଯଦି (var == "DEV_ASAN" && devAsan()) {
        ଫେରନ୍ତୁ "1";
    } ଅନ୍ୟଥା ଯଦି (var == "DEV_GCOV" && devGcov()) {
        ଫେରନ୍ତୁ "1";
        // cppcheck-suppress knownConditionTrueFalse
    } ଅନ୍ୟଥା ଯଦି (var == "SYSTEMC" && systemCFound()) {
        ଫେରନ୍ତୁ "1";
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ "";
    }
}

ତାର୍କିକ V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    ଫେରନ୍ତୁ ସତ୍ୟ;
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
#endif
}

ତାର୍କିକ V3Options::systemCFound() {
    ଫେରନ୍ତୁ (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

ତାର୍କିକ V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    ଫେରନ୍ତୁ ସତ୍ୟ;
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
#endif
}

ତାର୍କିକ V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    ଫେରନ୍ତୁ ସତ୍ୟ;
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
#endif
}

ତାର୍କିକ V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    ଫେରନ୍ତୁ ସତ୍ୟ;
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
#endif
}

ତାର୍କିକ V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    ଫେରନ୍ତୁ ସତ୍ୟ;
#ଅନ୍ୟଥା
    ଫେରନ୍ତୁ ମିଥ୍ୟା;
#endif
}

//######################################################################
// V3 Options notification methods

ଶୂନ୍ୟ V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_ଧାଡ଼ି* ଅଚଳ cmdfl = ନୂତନ File_ଧାଡ଼ି{File_ଧାଡ଼ି::command_ଧାଡ଼ି_Filename()};

    ଯଦି (!outFormatOk() && v3Global.opt.ମୁଖ୍ୟ()) ccSet();  // --main implies --cc if not provided
    ଯଦି (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    ଯଦି (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    ଯଦି (m_build) {
        ଯଦି (m_ଦ୍ୱିମିକ)
            backendFlags.emplace_back("--binary");
        ଅନ୍ୟଥା
            backendFlags.emplace_back("--build");
    }
    ଯଦି (m_preprocOnly) backendFlags.emplace_back("-E");
    ଯଦି (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    ଯଦି (m_lintOnly) backendFlags.emplace_back("--lint-only");
    ଯଦି (m_jsonOnly) backendFlags.emplace_back("--json-only");
    ଯଦି (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        ପାଇଁ (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    ଯଦି (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    ଯଦି (!m_gmake && !m_makeJson) m_gmake = ସତ୍ୟ;

    ଯଦି (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    ଯଦି (m_hierChild) {
        ଯଦି (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = ମିଥ୍ୟା;
    }

    ଯଦି (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = ସତ୍ୟ;  // No format, with --trace means wanted --trace-vcd
    }
    ଯଦି (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = ସତ୍ୟ;
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    ଯଦି (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    ଯଦି (protectIds()) {
        ଯଦି (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        ଯଦି (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        ଯଦି (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    ଯଦି (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    ଯଦି (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    ଯଦି (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(ସତ୍ୟ);

    // Default split limits if not specified
    ଯଦି (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    ଯଦି (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    ଯଦି (v3Global.opt.ମୁଖ୍ୟ() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    ଯଦି (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        ଯଦି (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    ଯଦି (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    ଯଦି (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    ଯଦି (!m_dumpLevel.ଗଣନା_2("tree") && m_dumpLevel.ଗଣନା_2("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    ଯଦି (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    ଯଦି (m_buildJobs == -1) m_buildJobs = 1;
    ଯଦି (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    ଯଦି (timing().isSetTrue()) V3PreShell::define_Cmd_ଧାଡ଼ି("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    ଯଦି (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(ମିଥ୍ୟା);

    // === Leave last
    // Mark options as available
    m_available = ସତ୍ୟ;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    ଫେରନ୍ତୁ ver;
}

string V3Options::protect_ଚାବି_Defaulted() VL_MT_SAFE {
    ସ୍ଥିର V3Mutex s_mutex;
    ଅଚଳ V3LockGuard lock{s_mutex};
    ଯଦି (m_protect_ଚାବି.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_ଚାବି = "VL-KEY-" + digest.digestBase64();
    }
    ଫେରନ୍ତୁ m_protect_ଚାବି;
}

ଶୂନ୍ୟ V3Options::throwSigsegv() {  // LCOV_EXCL_START
#ଯଦି !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<ଅସ୍ଥିର ଅକ୍ଷର*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(ଅଚଳ VTimescale& flag) ଅଚଳ {
    ଯଦି (!timeOverridePrec().isNone()) {
        ଫେରନ୍ତୁ timeOverridePrec();
    } ଅନ୍ୟଥା ଯଦି (flag.isNone()) {
        ଫେରନ୍ତୁ timeDefaultPrec();
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ flag;
    }
}

VTimescale V3Options::timeComputeUnit(ଅଚଳ VTimescale& flag) ଅଚଳ {
    ଯଦି (!timeOverrideUnit().isNone()) {
        ଫେରନ୍ତୁ timeOverrideUnit();
    } ଅନ୍ୟଥା ଯଦି (flag.isNone()) {
        ଫେରନ୍ତୁ timeDefaultUnit();
    } ଅନ୍ୟଥା {
        ଫେରନ୍ତୁ flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv) {
    // Return list of arguments as simple string
    string opts;
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < argc; ++i) {
        ଯଦି (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    ଫେରନ୍ତୁ opts;
}

//######################################################################
// V3 Options Parsing

ଶୂନ୍ୟ V3Options::parseOpts(File_ଧାଡ଼ି* fl, ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର** argv) VL_MT_DISABLED {
    // Save command line options
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < argc; ++i) add_ଧାଡ଼ି_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    ଯଦି (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    ଯଦି (prefix() == "" && ଶୀର୍ଷ_Module() != "") m_prefix = "V"s + ଶୀର୍ଷ_Module();  // Already encoded
    ଯଦି (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + Ast_ଗଣ୍ଠି::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    ଯଦି (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

ତାର୍କିକ V3Options::suffixed(ଅଚଳ string& sw, ଅଚଳ ଅକ୍ଷର* arg) {
    ଯଦି (std::strlen(arg) > sw.length()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

ଶୂନ୍ୟ V3Options::parseOptsList(File_ଧାଡ଼ି* fl, ଅଚଳ string& optdir, ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc,
                              ଅକ୍ଷର** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ଅଚଳ V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    ଅଚଳ auto callStrSetter = [ଏହି](ଶୂନ୍ୟ (V3Options::*cbStr)(ଅଚଳ string&)) {
        ଫେରନ୍ତୁ [ଏହି, cbStr](ଅଚଳ string& v) { (ଏହି->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) VL_MT_DISABLED {
        addDefine(optp, ସତ୍ୟ);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [ଏହି, &optdir](ଅଚଳ ଅକ୍ଷର* optp) {
        string dirs = optp;
        string::size_type pos;
        ଯେତେବେଳେ ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        string exts = optp;
        string::size_type pos;
        ଯେତେବେଳେ ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](ତାର୍କିକ) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ଅଚଳ ଅକ୍ଷର*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_bboxUnsup = flag;
        File_ଧାଡ଼ି::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, ସତ୍ୟ);
    });
    DECL_OPTION("-binary", CbCall, [ଏହି]() {
        m_ଦ୍ୱିମିକ = ସତ୍ୟ;
        m_build = ସତ୍ୟ;
        m_exe = ସତ୍ୟ;
        m_main = ସତ୍ୟ;
        ଯଦି (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା val = std::atoi(valp);
        ଯଦି (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } ଅନ୍ୟଥା ଯଦି (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [ଏହି]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](ଅଚଳ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ଅଚଳ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](ପୂର୍ଣ୍ଣସଂଖ୍ୟା val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        ଯଦି (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } ଅନ୍ୟଥା {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [ଏହି](ତାର୍କିକ flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_ପଦସମଷ୍ଟି);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_ପଦସମଷ୍ଟି_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* ଅଚଳ valp) {
        ଯଦି (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } ଅନ୍ୟଥା {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_ଧାଡ଼ି);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ଅଚଳ auto parseBinsLimit = [fl](ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* valp, uint32_t& limitr) {
        ଅକ୍ଷର* endp = nullptr;
        ଅଚଳ unsigned long long ମୂଲ୍ୟ = std::strtoull(valp, &endp, 10);
        ଯଦି (*endp || ମୂଲ୍ୟ < 1 || ମୂଲ୍ୟ > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            ଫେରନ୍ତୁ;
        }
        limitr = static_cast<uint32_t>(ମୂଲ୍ୟ);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [ଏହି, parseBinsLimit](ଅଚଳ ଅକ୍ଷର* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [ଏହି, parseBinsLimit](ଅଚଳ ଅକ୍ଷର* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) VL_MT_DISABLED {
        addDefine(valp, ମିଥ୍ୟା);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [ଏହି]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [ଏହି](ପୂର୍ଣ୍ଣସଂଖ୍ୟା v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* valp) {
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_ଗଦା_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [ଏହି, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [ଏହି, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = ସତ୍ୟ;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        ଯଦି (flag) {
            m_stdPackage = ମିଥ୍ୟା;
            m_stdWaiver = ମିଥ୍ୟା;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<ଶୂନ୍ୟ (*)(ପୂର୍ଣ୍ଣସଂଖ୍ୟା)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [ଏହି, fl, &optdir](ଅଚଳ ଅକ୍ଷର* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ସତ୍ୟ);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [ଏହି, &optdir](ଅଚଳ ଅକ୍ଷର* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [ଏହି, fl, &optdir](ଅଚଳ ଅକ୍ଷର* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ମିଥ୍ୟା);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_ବୃକ୍ଷ = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_ବୃକ୍ଷ);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_ବୃକ୍ଷ);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [ଏହି](ତାର୍କିକ flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](ତାର୍କିକ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](ତାର୍କିକ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](ତାର୍କିକ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ଠେଲ_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [ଏହି, fl](ତାର୍କିକ flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [ଏହି](ତାର୍କିକ flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [ଏହି](ତାର୍କିକ flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](ତାର୍କିକ) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](ତାର୍କିକ) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        ଯଦି (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](ତାର୍କିକ) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) { addParameter(optp, ମିଥ୍ୟା); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [ଏହି]() {
        cout << protect_ଚାବି_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ଅଚଳ ଅକ୍ଷର* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ଅଚଳ ଅକ୍ଷର* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) {
        ଅଚଳ V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) { m_hierParamsFile.ଠେଲ_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [ଏହି, &optdir](ଅଚଳ ଅକ୍ଷର* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [ଏହି, fl](ପୂର୍ଣ୍ଣସଂଖ୍ୟା val) {
        m_instr_ଗଣନା_Dpi = val;
        ଯଦି (m_instr_ଗଣନା_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = ସତ୍ୟ;
    });
    DECL_OPTION("-json-only-output", CbVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = ସତ୍ୟ;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [ଏହି]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [ଏହି]() { m_l2Name = "v"; }).undocumented();  // Historical
    ଅଚଳ auto setLang = [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        ଅଚଳ V3LangCode optval{valp};
        ଯଦି (optval.legal()) {
            m_defaultLanguage = optval;
        } ଅନ୍ୟଥା {
            VSpellCheck spell;
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.ଠେଲ_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_ସନ୍ଦେଶ(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [ଏହି](ଅଚଳ ଅକ୍ଷର* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_ଶୀର୍ଷ_Name);
    DECL_OPTION("-make", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        ଯଦି (!std::strcmp(valp, "gmake")) {
            m_gmake = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "json")) {
            m_makeJson = ସତ୍ୟ;
        } ଅନ୍ୟଥା {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_ଅଙ୍କ_Width);
    DECL_OPTION("-mod-prefix", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [ଏହି]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [ଏହି]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [ଏହି]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [ଏହି]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](ତାର୍କିକ /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_outputGroups = std::atoi(valp);
        ଯଦି (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_outputGroupsSize = std::atoll(valp);
        ଯଦି (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        ଯଦି (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        ଯଦି (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_ଧାଡ଼ି);
    DECL_OPTION("-pins64", CbCall, [ଏହି]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [ଏହି]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_pinsBv = std::atoi(valp);
        ଯଦି (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_pinsScUint = flag;
        ଯଦି (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [ଏହି](ତାର୍କିକ flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        ଯଦି (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [ଏହି]() { m_public = ମିଥ୍ୟା; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [ଏହି]() { m_profC = m_profCFuncs = ସତ୍ୟ; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [ଏହି]() {
        m_profC = m_profCFuncs = ସତ୍ୟ;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_ଚାବି);
    DECL_OPTION("-protect-lib", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = ସତ୍ୟ;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(ସତ୍ୟ);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [ଏହି](ତାର୍କିକ flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_publicParams = flag;
        v3Global.dpi(ସତ୍ୟ);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [ଏହି](ଅଚଳ ଅକ୍ଷର* varp) { addParameter(varp, ମିଥ୍ୟା); });

    DECL_OPTION("-quiet", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_reloopLimit = std::atoi(valp);
        ଯଦି (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_replicationLimit = std::atoi(valp);
        ଯଦି (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [ଏହି, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [ଏହି]() {
        m_outFormatOk = ସତ୍ୟ;
        m_systemC = ସତ୍ୟ;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [ଏହି](ତାର୍କିକ flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [ଏହି]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [ଏହି, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_threads = std::atoi(valp);
        ଯଦି (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        ଯଦି (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_hierThreads = std::atoi(valp);
        ଯଦି (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        ଯଦି (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = ସତ୍ୟ;
            m_threadsDpiUnpure = ସତ୍ୟ;
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = ମିଥ୍ୟା;
            m_threadsDpiUnpure = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = ସତ୍ୟ;
            m_threadsDpiUnpure = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        ଯଦି (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        ଯଦି (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        ଯଦି (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, ସତ୍ୟ);
        ଯଦି (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        ଯଦି (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [ଏହି](ଅଚଳ std::string& flag) { m_ଶୀର୍ଷ_Module = Ast_ଗଣ୍ଠି::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [ଏହି](ଅଚଳ std::string& flag) { m_ଶୀର୍ଷ_Module = Ast_ଗଣ୍ଠି::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_ଶୀର୍ଷ_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [ଏହି]() { m_traceEnabledSaif = ସତ୍ୟ; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [ଏହି]() {
        m_traceEnabledFst = ସତ୍ୟ;
        addLdLibs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", Set, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", Set, &m_traceMaxWidth);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ଅଚଳ ଅକ୍ଷର*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_ଶୀର୍ଷ);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [ଏହି]() { m_traceEnabledVcd = ସତ୍ୟ; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_ଗଣନା).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [ଏହି]() {
        showVersion(ସତ୍ୟ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [ଏହି, &optdir](ଅଚଳ ଅକ୍ଷର* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା val = std::atoi(valp);
        ଯଦି (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } ଅନ୍ୟଥା ଯଦି (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [ଏହି]() {
        showVersion(ମିଥ୍ୟା);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_ଧାଡ଼ି::globalWarnOff(V3ErrorCode::I_LINT, ମିଥ୍ୟା); });
    DECL_OPTION("-Werror-", CbPartialMatch, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* optp) {
        ଅଚଳ V3ErrorCode code{optp};
        ଯଦି (code == V3ErrorCode::EC_ERROR) {
            ଯଦି (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } ଅନ୍ୟଥା {
            V3Error::pretendError(code, ସତ୍ୟ);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [ଏହି](ଅଚଳ ଅକ୍ଷର* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](ଅଚଳ ଅକ୍ଷର* optp) VL_MT_DISABLED {
        ଅଚଳ string err = File_ଧାଡ଼ି::globalWarnOffParse(optp, ସତ୍ୟ);
        ଯଦି (!err.empty()) {
            ଅଚଳ string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        ପାଇଁ (ଅଚଳ string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [ଏହି]() { m_context = ମିଥ୍ୟା; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(ମିଥ୍ୟା); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_ଧାଡ଼ି::globalWarnOff(V3ErrorCode::I_LINT, ସତ୍ୟ); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_ଧାଡ଼ି::globalWarnOff(V3ErrorCode::I_STYLE, ସତ୍ୟ); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [ଏହି]() {
        m_pedantic = ସତ୍ୟ;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, ମିଥ୍ୟା);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [ଏହି, fl, &parser](ଅଚଳ ଅକ୍ଷର* optp) VL_MT_DISABLED {
        ଅଚଳ V3ErrorCode code{optp};
        ଯଦି (code == V3ErrorCode::EC_ERROR) {
            ଯଦି (!isFuture(optp)) {
                ଅଚଳ string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } ଅନ୍ୟଥା {
            File_ଧାଡ଼ି::globalWarnOff(code, ମିଥ୍ୟା);
            V3Error::pretendError(code, ମିଥ୍ୟା);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_ଧାଡ଼ି::globalWarnOff(V3ErrorCode::I_LINT, ମିଥ୍ୟା); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_ଧାଡ଼ି::globalWarnOff(V3ErrorCode::I_STYLE, ମିଥ୍ୟା); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        ଯଦି (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } ଅନ୍ୟଥା {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [ଏହି, fl](ଅଚଳ ଅକ୍ଷର* valp) {
        ଯଦି (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } ଅନ୍ୟଥା ଯଦି (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } ଅନ୍ୟଥା {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [ଏହି, &optdir](ଅଚଳ ଅକ୍ଷର* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        ଯଦି (argv[i][0] == '-' || argv[i][0] == '+') {
            ଅଚଳ std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା consumed = 0;
            ତାର୍କିକ isForRerun = ସତ୍ୟ;

            // Special case for -j which has an optional argument
            ଯଦି (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                ପୂର୍ଣ୍ଣସଂଖ୍ୟା val = 0;
                ଯଦି (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    ଯଦି (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                ଯଦି (m_buildJobs == -1) m_buildJobs = val;
                ଯଦି (m_verilateJobs == -1) m_verilateJobs = val;
                ଯଦି (m_outputGroups == -1) m_outputGroups = val;
            } ଅନ୍ୟଥା {
                ଅଚଳ auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                ଯଦି (consumed) {
                    // Already processed
                } ଅନ୍ୟଥା ଯଦି (isFuture0(argName)) {
                    consumed = 1;
                } ଅନ୍ୟଥା ଯଦି (isFuture1(argName)) {
                    consumed = 2;
                } ଅନ୍ୟଥା {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            ଯଦି (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            ଜାରି;
        }

        // Filename
        ଅଚଳ std::string filename = parseFileArg(optdir, argv[i]);
        ତାର୍କିକ isForRerun = ସତ୍ୟ;
        ଯଦି (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } ଅନ୍ୟଥା ଯଦି (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } ଅନ୍ୟଥା ଯଦି (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = ମିଥ୍ୟା;
        } ଅନ୍ୟଥା {
            V3Options::addVFile(filename, work());
            isForRerun = ମିଥ୍ୟା;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    ଯଦି (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

ଶୂନ୍ୟ V3Options::parseOptsFile(File_ଧାଡ଼ି* fl, ଅଚଳ string& filename, ତାର୍କିକ rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ଅଚଳ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    ଯଦି (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        ଫେରନ୍ତୁ;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    ତାର୍କିକ has_fargs = ମିଥ୍ୟା;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ଅଚଳ auto parse_fargs_cmt = [&](ଅଚଳ ଅକ୍ଷର* p) -> ଶୂନ୍ୟ {
        ଅଚଳ ତାର୍କିକ blockCmt = p[1] == '*';
        p += 2;
        ଯେତେବେଳେ (std::isspace(*p)) ++p;
        ଯଦି (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            ଯଦି (std::isspace(*p)) {
                ଯେତେବେଳେ (std::isspace(*p)) ++p;
                ଯଦି (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    ଯଦି (std::isspace(*p)) {
                        ଯେତେବେଳେ (std::isspace(*p)) ++p;
                        ଯଦି (!has_fargs) {
                            has_fargs = ସତ୍ୟ;
                            whole_file.clear();
                        }
                        whole_file += p;
                        ଯଦି (blockCmt) {
                            ଅଚଳ std::string::size_type pos = whole_file.find("*/");
                            ଯଦି (pos == std::string::npos) {
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

    ତାର୍କିକ inCmt = ମିଥ୍ୟା;
    ଯେତେବେଳେ (!ifp->eof()) {
        ଅଚଳ std::string ଧାଡ଼ି = V3Os::getline(*ifp);
        // Strip simple comments
        ଅକ୍ଷର lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        ତାର୍କିକ space_begin = ସତ୍ୟ;  // At beginning or leading spaces only
        ପାଇଁ (ଅଚଳ ଅକ୍ଷର* p = ଧାଡ଼ି.c_str(); *p; lastch = *p++) {
            ଯଦି (inCmt) {
                ଯଦି (p[0] == '*' && p[1] == '/') {
                    inCmt = ମିଥ୍ୟା;
                    ++p;
                }
            } ଅନ୍ୟଥା ଯଦି (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                ଭାଙ୍ଗ;  // Ignore to EOL
            } ଅନ୍ୟଥା ଯଦି (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                ଭାଙ୍ଗ;  // Ignore to EOL
            } ଅନ୍ୟଥା ଯଦି (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = ସତ୍ୟ;
                space_begin = ମିଥ୍ୟା;
                ++p;
            } ଅନ୍ୟଥା {
                ଯଦି (!std::isspace(*p)) space_begin = ମିଥ୍ୟା;
                ଯଦି (!has_fargs) whole_file += *p;
            }
        }
        ଯଦି (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    ଯଦି (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = ନୂତନ File_ଧାଡ଼ି{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    ଗଣନା state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    ପାଇଁ (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        ଅକ୍ଷର curr_char = whole_file[pos];
        ବାଛ (st) {
        ସ୍ଥିତି ST_IN_OPTION:  // Get all chars up to a white space or a "="
            ଯଦି (std::isspace(curr_char)) {  // End of option
                ଯଦି (!arg.empty()) {  // End of word
                    args.ଠେଲ_back(arg);
                }
                arg = "";
                ଭାଙ୍ଗ;
            }
            ଯଦି (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                ଭାଙ୍ଗ;
            }
            ଯଦି (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                ଯଦି (pos < whole_file.length()) curr_char = whole_file[pos];
                ଯଦି (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } ଅନ୍ୟଥା {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                ଭାଙ୍ଗ;
            }
            ଯଦି (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                ଭାଙ୍ଗ;
            }
            arg += curr_char;
            ଭାଙ୍ଗ;
        ସ୍ଥିତି ST_IN_QUOTED_STR:  // Just store all chars inside string
            ଯଦି (curr_char != '\'') {
                arg += curr_char;
            } ଅନ୍ୟଥା {  // End of quoted string
                st = ST_IN_OPTION;
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            ଯଦି (curr_char != '"') {
                ଯଦି (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } ଅନ୍ୟଥା {
                    arg += curr_char;
                }
            } ଅନ୍ୟଥା {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            ଭାଙ୍ଗ;
        ସ୍ଥିତି ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            ଭାଙ୍ଗ;
        }
    }
    ଯଦି (!arg.empty()) {  // Add last word
        args.ଠେଲ_back(arg);
    }

    // Path
    ଅଚଳ string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<ଅକ୍ଷର*> argv;
    argv.reserve(args.size() + 1);
    ପାଇଁ (ଅଚଳ string& i : args) argv.ଠେଲ_back(const_cast<ଅକ୍ଷର*>(i.c_str()));
    argv.ଠେଲ_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(ଅଚଳ string& optdir, ଅଚଳ string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    ଯଦି (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    ଫେରନ୍ତୁ filename;
}

//======================================================================

ଶୂନ୍ୟ V3Options::showVersion(ତାର୍କିକ verbose) {
    cout << version();
    cout << "\n";
    ଯଦି (!verbose) ଫେରନ୍ତୁ;

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
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_ମୂଳ << "\n";
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
    m_impp = ନୂତନ V3OptionsImp;

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

ଶୂନ୍ୟ V3Options::setDebugMode(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level) {
    V3Error::debugDefault(level);
    ଯଦି (!m_dumpLevel.ଗଣନା_2("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = ସତ୍ୟ;
    m_debugCheck = ସତ୍ୟ;
    ଯଦି (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(ଅଚଳ string& tag) ଅଚଳ VL_MT_SAFE {
    ଅଚଳ auto iter = m_debugLevel.find(tag);
    ଫେରନ୍ତୁ iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(ଅଚଳ string& srcfile_path) ଅଚଳ VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ଫେରନ୍ତୁ debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(ଅଚଳ string& tag) ଅଚଳ VL_MT_SAFE {
    ଅଚଳ auto iter = m_dumpLevel.find(tag);
    ଫେରନ୍ତୁ iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(ଅଚଳ string& srcfile_path) ଅଚଳ VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ଫେରନ୍ତୁ dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

ତାର୍କିକ V3Options::dump_ବୃକ୍ଷ_Addrids() ଅଚଳ VL_MT_SAFE {
    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା s_level = -1;
    ଯଦି (VL_UNLIKELY(s_level < 0)) {
        ଅଚଳ unsigned ମୂଲ୍ୟ = dumpLevel("tree-addrids");
        ଯଦି (!available()) ଫେରନ୍ତୁ ମୂଲ୍ୟ > 0;
        s_level = static_cast<unsigned>(ମୂଲ୍ୟ);
    }
    ଫେରନ୍ତୁ s_level > 0;
}

ଶୂନ୍ୟ V3Options::optimize(ପୂର୍ଣ୍ଣସଂଖ୍ୟା level) {
    // Set all optimizations to on/off
    ଅଚଳ ତାର୍କିକ flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_ବୃକ୍ଷ = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_ବୃକ୍ଷ = flag;
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
    ଯଦି (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
