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

#অন্তর্ভাৱ "V3PchAstMT.h"

#অন্তর্ভাৱ "V3Options.h"

#অন্তর্ভাৱ "V3Container.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Mutex.h"
#অন্তর্ভাৱ "V3OptionParser.h"
#অন্তর্ভাৱ "V3Os.h"
#অন্তর্ভাৱ "V3PreShell.h"
#অন্তর্ভাৱ "V3String.h"

// clang-format off
#অন্তর্ভাৱ <sys/types.h>
#অন্তর্ভাৱ <sys/stat.h>
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#অন্তর্ভাৱ <algorithm>
#অন্তর্ভাৱ <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#অন্যথা
# include <dirent.h>
#endif
#অন্তর্ভাৱ <fcntl.h>
#অন্তর্ভাৱ <list>
#অন্তর্ভাৱ <map>
#অন্তর্ভাৱ <memory>
#অন্তর্ভাৱ <thread>
#অন্তর্ভাৱ <set>
#অন্তর্ভাৱ <string>

#অন্তর্ভাৱ "config_rev.h"

#যদি defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

শ্ৰেণী V3OptionsImp final {
সার্ৱজনিক:
    // TYPES
    using DirMap = std::map<ধ্রুৱ string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_সাৰি_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, তার্কিক>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<ধ্রুৱ string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    শূন্য addIncDirUser(ধ্রুৱ string& incdir) {
        ধ্রুৱ string& dir = V3Os::filenameCleanup(incdir);
        ধ্রুৱ তার্কিক inserted = m_incDirUsers.ঢোকাও(dir);
        যদি (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    শূন্য addIncDirFallback(ধ্রুৱ string& incdir) {
        ধ্রুৱ string& dir = V3Os::filenameCleanup(incdir);
        যদি (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ধ্রুৱ auto itFoundPair = m_incDirFallbackSet.ঢোকাও(dir);
            যদি (itFoundPair.second) m_incDirFallbacks.ঠেলো_back(dir);
        }
    }
    শূন্য addLangExt(ধ্রুৱ string& langext, ধ্রুৱ V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        যদি (addext[0] == '.') addext = addext.substr(1);
        (শূন্য)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    শূন্য addLibExtV(ধ্রুৱ string& libext) { m_libExtVs.ঢোকাও(libext); }
    V3OptionsImp() = স্ৱভাৱ;
    ~V3OptionsImp() = স্ৱভাৱ;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(ধ্রুৱ ৱর্ণ* textp) {
    // Return code for given string, or ERROR, which is a bad code
    বাবে (পূর্ণাঙ্ক codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        ধ্রুৱ V3LangCode code{codei};
        যদি (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            ঘূৰাই;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ধ্রুৱ string& মান, তার্কিক& badr)
    : m_e{VTimescale::NONE} {
    badr = সঁচা;
    ধ্রুৱ string spaceless = VString::removeWhitespace(মান);
    বাবে (পূর্ণাঙ্ক i = TS_100S; i < _ENUM_END; ++i) {
        ধ্রুৱ VTimescale ts{i};
        যদি (spaceless == ts.ascii()) {
            badr = মিছা;
            m_e = ts.m_e;
            ভঙ্গ;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(ধ্রুৱ string& opts) {
    VStringList vals;
    তার্কিক inStr = মিছা;
    string cur;
    স্থির ধ্রুৱ string hierBlock("--hierarchical-block");
    File_সাৰি cmdfl(File_সাৰি::command_সাৰি_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    বাবে (string::const_iterator it = opts.begin(); it != opts.end();) {
        যদি (inStr) {
            যদি (*it == '\\') {
                ++it;
                যদি (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    ভঙ্গ;
                }
                যদি (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    ভঙ্গ;
                }
                cur.ঠেলো_back(*it);
                ++it;
            } অন্যথা যদি (*it == '"') {  // end of string
                cur.ঠেলো_back(*it);
                vals.ঠেলো_back(cur);
                cur.clear();
                ++it;
                যদি (it != opts.end()) {
                    যদি (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        ভঙ্গ;
                    }
                    ++it;
                    যদি (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        ভঙ্গ;
                    }
                    inStr = *it == '"';
                    cur.ঠেলো_back(*it);
                    ++it;
                }
            } অন্যথা {
                cur.ঠেলো_back(*it);
                ++it;
            }
        } অন্যথা {
            যদি (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                ভঙ্গ;
            }
            যদি (*it == ',') {  // end of this parameter
                vals.ঠেলো_back(cur);
                cur.clear();
                ++it;
                যদি (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    ভঙ্গ;
                }
                inStr = *it == '"';
            }
            cur.ঠেলো_back(*it);
            ++it;
        }
    }
    যদি (!cur.empty()) vals.ঠেলো_back(cur);
    যদি (vals.size() >= 2) {
        যদি (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } অন্যথা {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    বাবে (size_t i = 2; i + 1 < vals.size(); i += 2) {
        ধ্রুৱ তার্কিক inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        যদি (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

শূন্য VTimescale::parseSlashed(File_সাৰি* fl, ধ্রুৱ ৱর্ণ* textp, VTimescale& unitr,
                              VTimescale& precr, তার্কিক allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ধ্রুৱ ৱর্ণ* cp = textp;
    বাবে (; std::isspace(*cp); ++cp) {}
    ধ্রুৱ ৱর্ণ* ধ্রুৱ unitp = cp;
    বাবে (; *cp && *cp != '/'; ++cp) {}
    ধ্রুৱ string unitStr(unitp, cp - unitp);
    বাবে (; std::isspace(*cp); ++cp) {}
    string precStr;
    যদি (*cp == '/') {
        ++cp;
        বাবে (; std::isspace(*cp); ++cp) {}
        ধ্রুৱ ৱর্ণ* ধ্রুৱ precp = cp;
        বাবে (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    বাবে (; std::isspace(*cp); ++cp) {}
    যদি (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        ঘূৰাই;
    }

    তার্কিক unitbad = মিছা;
    ধ্রুৱ VTimescale unit{unitStr, unitbad /*ref*/};
    যদি (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        ঘূৰাই;
    }
    unitr = unit;

    যদি (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        তার্কিক precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        যদি (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            ঘূৰাই;
        }
        যদি (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            ঘূৰাই;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

শূন্য V3Options::addIncDirUser(ধ্রুৱ string& incdir) { m_impp->addIncDirUser(incdir); }
শূন্য V3Options::addIncDirFallback(ধ্রুৱ string& incdir) { m_impp->addIncDirFallback(incdir); }
শূন্য V3Options::addLangExt(ধ্রুৱ string& langext, ধ্রুৱ V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
শূন্য V3Options::addLibExtV(ধ্রুৱ string& libext) { m_impp->addLibExtV(libext); }
শূন্য V3Options::addDefine(ধ্রুৱ string& defline, তার্কিক allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string বাম = defline;
    যেতিয়ালৈ (বাম != "") {
        string def = বাম;
        string::size_type pos;
        যদি (allowPlus && ((pos = বাম.find('+')) != string::npos)) {
            বাম = বাম.substr(pos + 1);
            def.erase(pos);
        } অন্যথা {
            বাম = "";
        }
        string মান;
        যদি ((pos = def.find('=')) != string::npos) {
            মান = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_সাৰি(def, মান);
    }
}
শূন্য V3Options::addParameter(ধ্রুৱ string& paramline, তার্কিক allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string বাম = paramline;
    যেতিয়ালৈ (বাম != "") {
        string param = বাম;
        string::size_type pos;
        যদি (allowPlus && ((pos = বাম.find('+')) != string::npos)) {
            বাম = বাম.substr(pos + 1);
            param.erase(pos);
        } অন্যথা {
            বাম = "";
        }
        string মান;
        যদি ((pos = param.find('=')) != string::npos) {
            মান = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << মান);
        (শূন্য)m_parameters.erase(param);
        m_parameters[param] = মান;
    }
}

তার্কিক V3Options::hasParameter(ধ্রুৱ string& name) {
    ঘূৰাই m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(ধ্রুৱ string& name) {
    string মান = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    ঘূৰাই মান;
}

শূন্য V3Options::checkParameters() {
    যদি (!m_parameters.empty()) {
        std::stringstream বাৰ্তা;
        বাৰ্তা << "Parameters from the command line were not found in the design:";
        বাবে (ধ্রুৱ auto& i : m_parameters) বাৰ্তা << " " << i.first;
        v3error(বাৰ্তা.str());
    }
}

শূন্য V3Options::addCppFile(ধ্রুৱ string& filename) { m_cppFiles.ঢোকাও(filename); }
শূন্য V3Options::addCFlags(ধ্রুৱ string& filename) { m_cFlags.ঠেলো_back(filename); }
শূন্য V3Options::addLibMapFile(ধ্রুৱ string& filename) { m_libmapFiles.ঢোকাও(filename); }
শূন্য V3Options::addCompilerIncludes(ধ্রুৱ string& filename) {
    m_compilerIncludes.ঢোকাও(filename);
}
শূন্য V3Options::addLdLibFile(ধ্রুৱ string& filename) {
    m_ldLibFiles.ঠেলো_back(filename);
    addLdLibs(filename);
}
শূন্য V3Options::addLdLibs(ধ্রুৱ string& filename) { m_ldLibs.ঠেলো_back(filename); }
শূন্য V3Options::addMakeFlags(ধ্রুৱ string& filename) { m_makeFlags.ঠেলো_back(filename); }
শূন্য V3Options::addFuture(ধ্রুৱ string& flag) { m_futures.ঢোকাও(flag); }
শূন্য V3Options::addFuture0(ধ্রুৱ string& flag) { m_future0s.ঢোকাও(flag); }
শূন্য V3Options::addFuture1(ধ্রুৱ string& flag) { m_future1s.ঢোকাও(flag); }
তার্কিক V3Options::isFuture(ধ্রুৱ string& flag) ধ্রুৱ {
    ঘূৰাই m_futures.find(flag) != m_futures.end();
}
তার্কিক V3Options::isFuture0(ধ্রুৱ string& flag) ধ্রুৱ {
    ঘূৰাই m_future0s.find(flag) != m_future0s.end();
}
তার্কিক V3Options::isFuture1(ধ্রুৱ string& flag) ধ্রুৱ {
    ঘূৰাই m_future1s.find(flag) != m_future1s.end();
}
তার্কিক V3Options::isLibraryFile(ধ্রুৱ string& filename, ধ্রুৱ string& libname) ধ্রুৱ {
    ঘূৰাই m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
শূন্য V3Options::addLibraryFile(ধ্রুৱ string& filename, ধ্রুৱ string& libname) {
    m_libraryFiles.ঢোকাও({filename, libname});
}
শূন্য V3Options::addVFile(ধ্রুৱ string& filename, ধ্রুৱ string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ঠেলো_back({filename, libname});  // Not emplace
}
শূন্য V3Options::addVltFile(ধ্রুৱ string& filename, ধ্রুৱ string& libname) {
    m_vltFiles.ঢোকাও({filename, libname});
}
শূন্য V3Options::addForceInc(ধ্রুৱ string& filename) { m_forceIncs.ঠেলো_back(filename); }

শূন্য V3Options::add_সাৰি_Arg(ধ্রুৱ string& arg) { m_impp->m_সাৰি_Args.ঠেলো_back(arg); }

শূন্য V3Options::addArg(ৱর্ণ** argv, size_t গুনতি, তার্কিক isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, তার্কিক>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    বাবে (size_t n = 0; n < গুনতি; ++n) pair.first.emplace_back(argv[n]);
}

শূন্য V3Options::addArg(ধ্রুৱ std::string& arg, তার্কিক isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, তার্কিক>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

ধ্রুৱ std::list<std::pair<std::list<std::string>, তার্কিক>>& V3Options::allArgs() ধ্রুৱ {
    ঘূৰাই m_impp->m_allArgs;
}

string V3Options::allArgsString() ধ্রুৱ VL_MT_SAFE {
    string result;
    বাবে (ধ্রুৱ auto& pair : m_impp->m_allArgs) {
        বাবে (ধ্রুৱ string& arg : pair.first) {
            যদি (!result.empty()) result += " ";
            result += arg;
        }
    }
    ঘূৰাই result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(তার্কিক for_শীৰ্ষ) ধ্রুৱ {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    বাবে (ধ্রুৱ auto& vFile : m_vFiles) vFiles.ঢোকাও(vFile.filename());
    string out;
    তার্কিক stripArg = মিছা;
    তার্কিক strip_Arg_If_অঙ্ক = মিছা;
    বাবে (ধ্রুৱ auto& pair : m_impp->m_allArgs) {
        বাবে (ধ্রুৱ string& arg : pair.first) {
            যদি (stripArg) {
                stripArg = মিছা;
                অনুৱর্ত;
            }
            যদি (strip_Arg_If_অঙ্ক) {
                strip_Arg_If_অঙ্ক = মিছা;
                যদি (isdigit(arg[0])) অনুৱর্ত;
            }
            পূর্ণাঙ্ক skip = 0;
            যদি (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } অন্যথা যদি (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            যদি (skip > 0) {  // arg is an option
                ধ্রুৱ string opt = arg.substr(skip);  // Remove '-' in the beginning
                ধ্রুৱ পূর্ণাঙ্ক অঙ্ক_Strip = stripOptionsForChildRun(opt, for_শীৰ্ষ);
                যদি (অঙ্ক_Strip) {
                    UASSERT(0 <= অঙ্ক_Strip && অঙ্ক_Strip <= 3, "should be one of 0, 1, 2, 3");
                    যদি (অঙ্ক_Strip == 2) stripArg = সঁচা;
                    যদি (অঙ্ক_Strip == 3) strip_Arg_If_অঙ্ক = সঁচা;
                    অনুৱর্ত;
                }
            } অন্যথা {  // Not an option
                যদি (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    অনুৱর্ত;
                }
            }
            যদি (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    ঘূৰাই out;
}

শূন্য V3Options::ccSet() {  // --cc
    m_outFormatOk = সঁচা;
    m_systemC = মিছা;
}

শূন্য V3Options::decorations(File_সাৰি* fl, ধ্রুৱ string& arg) {  // --decorations
    যদি (arg == "none") {
        m_decoration = মিছা;
        m_decorationNodes = মিছা;
    } অন্যথা যদি (arg == "node") {
        m_decoration = সঁচা;
        m_decorationNodes = সঁচা;
    } অন্যথা যদি (arg == "medium") {
        m_decoration = সঁচা;
        m_decorationNodes = মিছা;
    } অন্যথা {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() ধ্রুৱ VL_MT_SAFE {
    std::vector<std::string> result;
    যদি (traceEnabledFst()) result.emplace_back("VerilatedFst");
    যদি (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    যদি (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    ঘূৰাই result;
}
std::vector<std::string> V3Options::traceClassLangs() ধ্রুৱ VL_MT_SAFE {
    std::vector<std::string> result;
    বাবে (ধ্রুৱ auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    ঘূৰাই result;
}
std::vector<std::string> V3Options::traceSourceBases() ধ্রুৱ VL_MT_SAFE {
    std::vector<std::string> result;
    যদি (traceEnabledFst()) result.emplace_back("verilated_fst");
    যদি (traceEnabledSaif()) result.emplace_back("verilated_saif");
    যদি (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    ঘূৰাই result;
}
std::vector<std::string> V3Options::traceSourceLangs() ধ্রুৱ VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    বাবে (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    ঘূৰাই result;
}
std::string V3Options::traceClassBase() ধ্রুৱ VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    ঘূৰাই traceClassBases().front();
}
std::string V3Options::traceClassLang() ধ্রুৱ VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    ঘূৰাই traceClassLangs().front();
}

//######################################################################
// File searching

তার্কিক V3Options::fileStatNormal(ধ্রুৱ string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    সংরচনা stat sstat;  // Stat information
    ধ্রুৱ পূর্ণাঙ্ক err = stat(filename.c_str(), &sstat);
    যদি (err != 0) ঘূৰাই মিছা;
    যদি (S_ISDIR(sstat.st_mode)) ঘূৰাই মিছা;
    ঘূৰাই সঁচা;
}

string V3Options::fileExists(ধ্রুৱ string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ধ্রুৱ string dir = V3Os::filenameDir(filename);
    ধ্রুৱ string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    যদি (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* ধ্রুৱ setp = &(diriter->second);

#ifdef _MSC_VER
        যত্ন {
            বাবে (ধ্রুৱ auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->ঢোকাও(dirEntry.path().filename().string());
        } গৃহাণ (std::filesystem::filesystem_error ধ্রুৱ& ex) {
            (শূন্য)ex;
            ঘূৰাই "";
        }
#অন্যথা
        যদি (DIR* ধ্রুৱ dirp = opendir(dir.c_str())) {
            যেতিয়ালৈ (সংরচনা dirent* direntp = readdir(dirp)) setp->ঢোকাও(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    ধ্রুৱ std::set<string>* ধ্রুৱ filesetp = &(diriter->second);
    ধ্রুৱ auto fileiter = filesetp->find(basename);
    যদি (fileiter == filesetp->end()) ঘূৰাই "";  // Not found
    // Check if it is a directory, ignore if so
    ধ্রুৱ string filenameOut = V3Os::filenameJoin(dir, basename);
    যদি (!fileStatNormal(filenameOut)) ঘূৰাই "";  // Directory
    ঘূৰাই filenameOut;
}

string V3Options::filePathCheckOneDir(ধ্রুৱ string& modname, ধ্রুৱ string& dirname) {
    বাবে (ধ্রুৱ string& i : m_impp->m_libExtVs) {
        ধ্রুৱ string fn = V3Os::filenameJoin(dirname, modname + i);
        ধ্রুৱ string exists = fileExists(fn);
        যদি (exists != "") ঘূৰাই exists;
    }
    ঘূৰাই "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
পূর্ণাঙ্ক V3Options::stripOptionsForChildRun(ধ্রুৱ string& opt, তার্কিক for_শীৰ্ষ) {
    // Options to strip for both the top wrapper, and hier blocks
    স্থির ধ্রুৱ std::unordered_map<std::string, পূর্ণাঙ্ক> commonOpts{
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
    যদি (commonOpts.গুনতি(opt)) ঘূৰাই commonOpts.at(opt);

    // Options to strip only for hier blocks
    যদি (!for_শীৰ্ষ) {
        স্থির ধ্রুৱ std::unordered_map<std::string, পূর্ণাঙ্ক> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        যদি (subOpts.গুনতি(opt)) ঘূৰাই subOpts.at(opt);
        যদি (VString::startsWith(opt, "G")) ঘূৰাই 1;
        যদি (VString::startsWith(opt, "pvalue")) ঘূৰাই 1;
    }

    // Do not strip
    ঘূৰাই 0;
}

শূন্য V3Options::validateIdentifier(File_সাৰি* fl, ধ্রুৱ string& arg, ধ্রুৱ string& opt) {
    যদি (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_সাৰি* fl, ধ্রুৱ string& modname, ধ্রুৱ string& lastpath,
                           ধ্রুৱ string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ধ্রুৱ string filename = V3Os::filenameCleanup(VName::dehash(modname));
    যদি (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ধ্রুৱ string exists = filePathCheckOneDir(filename, "");
        যদি (exists != "") ঘূৰাই exists;
    }
    বাবে (ধ্রুৱ string& dir : m_impp->m_incDirUsers) {
        ধ্রুৱ string exists = filePathCheckOneDir(filename, dir);
        যদি (exists != "") ঘূৰাই exists;
    }
    বাবে (ধ্রুৱ string& dir : m_impp->m_incDirFallbacks) {
        ধ্রুৱ string exists = filePathCheckOneDir(filename, dir);
        যদি (exists != "") ঘূৰাই exists;
    }

    যদি (m_relativeIncludes) {
        ধ্রুৱ string exists = filePathCheckOneDir(filename, lastpath);
        যদি (exists != "") ঘূৰাই V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    যদি (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_বাৰ্তা(fl, filename));
    }
    ঘূৰাই "";
}

string V3Options::file_Path_Looked_বাৰ্তা(File_সাৰি* /*fl*/, ধ্রুৱ string& modname) {
    স্থির তার্কিক s_shown_notfound_বাৰ্তা = মিছা;
    std::ostringstream ss;
    যদি (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } অন্যথা যদি (!s_shown_notfound_বাৰ্তা) {
        s_shown_notfound_বাৰ্তা = সঁচা;
        যদি (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        বাবে (ধ্রুৱ string& dir : m_impp->m_incDirUsers) {
            বাবে (ধ্রুৱ string& ext : m_impp->m_libExtVs) {
                ধ্রুৱ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        বাবে (ধ্রুৱ string& dir : m_impp->m_incDirFallbacks) {
            বাবে (ধ্রুৱ string& ext : m_impp->m_libExtVs) {
                ধ্রুৱ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    ঘূৰাই ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(ধ্রুৱ string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    যদি (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        ঘূৰাই V3LangCode::mostRecent();
    } অন্যথা যদি ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ধ্রুৱ auto it = m_impp->m_langExts.find(ext);
        যদি (it != m_impp->m_langExts.end()) ঘূৰাই it->second;
    }
    ঘূৰাই m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ধ্রুৱ string& var) {
    // If update below, also update V3Options::showVersion()
    যদি (var == "MAKE") {
        ঘূৰাই getenvMAKE();
    } অন্যথা যদি (var == "PERL") {
        ঘূৰাই getenvPERL();
    } অন্যথা যদি (var == "PYTHON3") {
        ঘূৰাই getenvPYTHON3();
    } অন্যথা যদি (var == "SYSTEMC") {
        ঘূৰাই getenvSYSTEMC();
    } অন্যথা যদি (var == "SYSTEMC_ARCH") {
        ঘূৰাই getenvSYSTEMC_ARCH();
    } অন্যথা যদি (var == "SYSTEMC_INCLUDE") {
        ঘূৰাই getenvSYSTEMC_INCLUDE();
    } অন্যথা যদি (var == "SYSTEMC_LIBDIR") {
        ঘূৰাই getenvSYSTEMC_LIBDIR();
    } অন্যথা যদি (var == "VERILATOR_ROOT") {
        ঘূৰাই getenv_VERILATOR_মূল();
    } অন্যথা {
        ঘূৰাই V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { ঘূৰাই V3Os::getenvStr("MAKE", "gmake"); }
#অন্যথা
string V3Options::getenvMAKE() { ঘূৰাই V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    ঘূৰাই V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    ঘূৰাই V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    ঘূৰাই V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ধ্রুৱ string defenv = string{DEFENV_SYSTEMC};
    যদি (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    ঘূৰাই V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ধ্রুৱ string defenv = string{DEFENV_SYSTEMC_ARCH};
    যদি (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    যদি (var == "") {
#যদি defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ধ্রুৱ string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#নোচেদ্যদি defined(_WIN32)
        ধ্রুৱ string sysname = "WIN32";
        var = "win32";
#অন্যথা
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        সংরচনা utsname uts;
        uname(&uts);
        ধ্রুৱ string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        যদি (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } অন্যথা যদি (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } অন্যথা {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    ঘূৰাই var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ধ্রুৱ string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    যদি (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    যদি (var == "") {
        ধ্রুৱ string sc = getenvSYSTEMC();
        যদি (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    ঘূৰাই V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ধ্রুৱ string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    যদি (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    যদি (var == "") {
        ধ্রুৱ string sc = getenvSYSTEMC();
        ধ্রুৱ string arch = getenvSYSTEMC_ARCH();
        যদি (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    ঘূৰাই V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_মূল() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ধ্রুৱ string defenv = string{DEFENV_VERILATOR_মূল};
    যদি (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    যদি (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    ঘূৰাই V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ধ্রুৱ string defenv = string{DEFENV_VERILATOR_SOLVER};
    যদি (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    ঘূৰাই var;
}

string V3Options::getStdPackagePath() {
    ঘূৰাই V3Os::filenameJoin(getenv_VERILATOR_মূল(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    ঘূৰাই V3Os::filenameJoin(getenv_VERILATOR_মূল(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ধ্রুৱ string& var) {
    // If update below, also update V3Options::showVersion()
    যদি (var == "COROUTINES" && coroutineSupport()) {
        ঘূৰাই "1";
    } অন্যথা যদি (var == "TSAN" && tsanSupport()) {
        ঘূৰাই "1";
    } অন্যথা যদি (var == "DEV_ASAN" && devAsan()) {
        ঘূৰাই "1";
    } অন্যথা যদি (var == "DEV_GCOV" && devGcov()) {
        ঘূৰাই "1";
        // cppcheck-suppress knownConditionTrueFalse
    } অন্যথা যদি (var == "SYSTEMC" && systemCFound()) {
        ঘূৰাই "1";
    } অন্যথা {
        ঘূৰাই "";
    }
}

তার্কিক V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    ঘূৰাই সঁচা;
#অন্যথা
    ঘূৰাই মিছা;
#endif
}

তার্কিক V3Options::systemCFound() {
    ঘূৰাই (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

তার্কিক V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    ঘূৰাই সঁচা;
#অন্যথা
    ঘূৰাই মিছা;
#endif
}

তার্কিক V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    ঘূৰাই সঁচা;
#অন্যথা
    ঘূৰাই মিছা;
#endif
}

তার্কিক V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    ঘূৰাই সঁচা;
#অন্যথা
    ঘূৰাই মিছা;
#endif
}

তার্কিক V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    ঘূৰাই সঁচা;
#অন্যথা
    ঘূৰাই মিছা;
#endif
}

//######################################################################
// V3 Options notification methods

শূন্য V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_সাৰি* ধ্রুৱ cmdfl = নূতন File_সাৰি{File_সাৰি::command_সাৰি_Filename()};

    যদি (!outFormatOk() && v3Global.opt.main()) ccSet();  // --main implies --cc if not provided
    যদি (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    যদি (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    যদি (m_build) {
        যদি (m_দ্বিমিক)
            backendFlags.emplace_back("--binary");
        অন্যথা
            backendFlags.emplace_back("--build");
    }
    যদি (m_preprocOnly) backendFlags.emplace_back("-E");
    যদি (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    যদি (m_lintOnly) backendFlags.emplace_back("--lint-only");
    যদি (m_jsonOnly) backendFlags.emplace_back("--json-only");
    যদি (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        বাবে (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    যদি (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    যদি (!m_gmake && !m_makeJson) m_gmake = সঁচা;

    যদি (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    যদি (m_hierChild) {
        যদি (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = মিছা;
    }

    যদি (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = সঁচা;  // No format, with --trace means wanted --trace-vcd
    }
    যদি (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = সঁচা;
    ধ্রুৱ পূর্ণাঙ্ক ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    যদি (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    যদি (protectIds()) {
        যদি (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        যদি (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        যদি (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    যদি (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    যদি (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    যদি (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(সঁচা);

    // Default split limits if not specified
    যদি (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    যদি (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    যদি (v3Global.opt.main() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    যদি (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        যদি (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    যদি (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    যদি (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    যদি (!m_dumpLevel.গুনতি("tree") && m_dumpLevel.গুনতি("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    যদি (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    যদি (m_buildJobs == -1) m_buildJobs = 1;
    যদি (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    যদি (timing().isSetTrue()) V3PreShell::define_Cmd_সাৰি("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    যদি (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(মিছা);

    // === Leave last
    // Mark options as available
    m_available = সঁচা;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    ঘূৰাই ver;
}

string V3Options::protect_চাবি_Defaulted() VL_MT_SAFE {
    স্থির V3Mutex s_mutex;
    ধ্রুৱ V3LockGuard lock{s_mutex};
    যদি (m_protect_চাবি.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_চাবি = "VL-KEY-" + digest.digestBase64();
    }
    ঘূৰাই m_protect_চাবি;
}

শূন্য V3Options::throwSigsegv() {  // LCOV_EXCL_START
#যদি !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<চঞ্চল ৱর্ণ*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(ধ্রুৱ VTimescale& flag) ধ্রুৱ {
    যদি (!timeOverridePrec().isNone()) {
        ঘূৰাই timeOverridePrec();
    } অন্যথা যদি (flag.isNone()) {
        ঘূৰাই timeDefaultPrec();
    } অন্যথা {
        ঘূৰাই flag;
    }
}

VTimescale V3Options::timeComputeUnit(ধ্রুৱ VTimescale& flag) ধ্রুৱ {
    যদি (!timeOverrideUnit().isNone()) {
        ঘূৰাই timeOverrideUnit();
    } অন্যথা যদি (flag.isNone()) {
        ঘূৰাই timeDefaultUnit();
    } অন্যথা {
        ঘূৰাই flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(পূর্ণাঙ্ক argc, ৱর্ণ** argv) {
    // Return list of arguments as simple string
    string opts;
    বাবে (পূর্ণাঙ্ক i = 0; i < argc; ++i) {
        যদি (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    ঘূৰাই opts;
}

//######################################################################
// V3 Options Parsing

শূন্য V3Options::parseOpts(File_সাৰি* fl, পূর্ণাঙ্ক argc, ৱর্ণ** argv) VL_MT_DISABLED {
    // Save command line options
    বাবে (পূর্ণাঙ্ক i = 0; i < argc; ++i) add_সাৰি_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    যদি (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    যদি (prefix() == "" && শীৰ্ষ_Module() != "") m_prefix = "V"s + শীৰ্ষ_Module();  // Already encoded
    যদি (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + Ast_গ্ৰন্থি::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    যদি (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

তার্কিক V3Options::suffixed(ধ্রুৱ string& sw, ধ্রুৱ ৱর্ণ* arg) {
    যদি (std::strlen(arg) > sw.length()) ঘূৰাই মিছা;
    ঘূৰাই (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

শূন্য V3Options::parseOptsList(File_সাৰি* fl, ধ্রুৱ string& optdir, পূর্ণাঙ্ক argc,
                              ৱর্ণ** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ধ্রুৱ V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    ধ্রুৱ auto callStrSetter = [এতত্](শূন্য (V3Options::*cbStr)(ধ্রুৱ string&)) {
        ঘূৰাই [এতত্, cbStr](ধ্রুৱ string& v) { (এতত্->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) VL_MT_DISABLED {
        addDefine(optp, সঁচা);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [এতত্, &optdir](ধ্রুৱ ৱর্ণ* optp) {
        string dirs = optp;
        string::size_type pos;
        যেতিয়ালৈ ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        string exts = optp;
        string::size_type pos;
        যেতিয়ালৈ ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](তার্কিক) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [এতত্](তার্কিক flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ধ্রুৱ ৱর্ণ*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [এতত্](তার্কিক flag) {
        m_bboxUnsup = flag;
        File_সাৰি::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, সঁচা);
    });
    DECL_OPTION("-binary", CbCall, [এতত্]() {
        m_দ্বিমিক = সঁচা;
        m_build = সঁচা;
        m_exe = সঁচা;
        m_main = সঁচা;
        যদি (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        পূর্ণাঙ্ক val = std::atoi(valp);
        যদি (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } অন্যথা যদি (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [এতত্]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](ধ্রুৱ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ধ্রুৱ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](পূর্ণাঙ্ক val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        যদি (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } অন্যথা যদি (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } অন্যথা যদি (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } অন্যথা {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [এতত্](তার্কিক flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_ৰাশিমালা);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_ৰাশিমালা_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* ধ্রুৱ valp) {
        যদি (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } অন্যথা যদি (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } অন্যথা যদি (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } অন্যথা {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_সাৰি);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ধ্রুৱ auto parseBinsLimit = [fl](ধ্রুৱ ৱর্ণ* optp, ধ্রুৱ ৱর্ণ* valp, uint32_t& limitr) {
        ৱর্ণ* endp = nullptr;
        ধ্রুৱ unsigned long long মান = std::strtoull(valp, &endp, 10);
        যদি (*endp || মান < 1 || মান > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            ঘূৰাই;
        }
        limitr = static_cast<uint32_t>(মান);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [এতত্, parseBinsLimit](ধ্রুৱ ৱর্ণ* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [এতত্, parseBinsLimit](ধ্রুৱ ৱর্ণ* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* valp) VL_MT_DISABLED {
        addDefine(valp, মিছা);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [এতত্]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [এতত্](পূর্ণাঙ্ক v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [এতত্](ধ্রুৱ ৱর্ণ* optp, ধ্রুৱ ৱর্ণ* valp) {
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_স্তূপ_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [এতত্, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [এতত্, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = সঁচা;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [এতত্](ধ্রুৱ ৱর্ণ* optp, ধ্রুৱ ৱর্ণ* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [এতত্](তার্কিক flag) {
        যদি (flag) {
            m_stdPackage = মিছা;
            m_stdWaiver = মিছা;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<শূন্য (*)(পূর্ণাঙ্ক)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [এতত্](ধ্রুৱ ৱর্ণ* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [এতত্, fl, &optdir](ধ্রুৱ ৱর্ণ* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), সঁচা);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [এতত্, &optdir](ধ্রুৱ ৱর্ণ* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [এতত্, fl, &optdir](ধ্রুৱ ৱর্ণ* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), মিছা);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [এতত্](ধ্রুৱ ৱর্ণ* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [এতত্](ধ্রুৱ ৱর্ণ* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [এতত্](তার্কিক flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_বৃক্ষ = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_বৃক্ষ);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_বৃক্ষ);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [এতত্](তার্কিক flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [এতত্](তার্কিক flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](তার্কিক) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](তার্কিক) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](তার্কিক) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ঠেলো_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [এতত্, fl](তার্কিক flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [এতত্](তার্কিক flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [এতত্](তার্কিক flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](তার্কিক) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](তার্কিক) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        যদি (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](তার্কিক) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) { addParameter(optp, মিছা); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [এতত্]() {
        cout << protect_চাবি_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ধ্রুৱ ৱর্ণ* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ধ্রুৱ ৱর্ণ* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [এতত্](ধ্রুৱ ৱর্ণ* valp) {
        ধ্রুৱ V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [এতত্](ধ্রুৱ ৱর্ণ* optp) { m_hierParamsFile.ঠেলো_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [এতত্, &optdir](ধ্রুৱ ৱর্ণ* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [এতত্, fl](পূর্ণাঙ্ক val) {
        m_instr_গুনতি_Dpi = val;
        যদি (m_instr_গুনতি_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [এতত্](ধ্রুৱ ৱর্ণ* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = সঁচা;
    });
    DECL_OPTION("-json-only-output", CbVal, [এতত্](ধ্রুৱ ৱর্ণ* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = সঁচা;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [এতত্]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [এতত্]() { m_l2Name = "v"; }).undocumented();  // Historical
    ধ্রুৱ auto setLang = [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        ধ্রুৱ V3LangCode optval{valp};
        যদি (optval.legal()) {
            m_defaultLanguage = optval;
        } অন্যথা {
            VSpellCheck spell;
            বাবে (পূর্ণাঙ্ক i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.ঠেলো_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_বাৰ্তা(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [এতত্](ধ্রুৱ ৱর্ণ* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [এতত্](ধ্রুৱ ৱর্ণ* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_শীৰ্ষ_Name);
    DECL_OPTION("-make", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        যদি (!std::strcmp(valp, "gmake")) {
            m_gmake = সঁচা;
        } অন্যথা যদি (!std::strcmp(valp, "json")) {
            m_makeJson = সঁচা;
        } অন্যথা {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_অঙ্ক_Width);
    DECL_OPTION("-mod-prefix", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [এতত্]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [এতত্]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [এতত্]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [এতত্]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](তার্কিক /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_outputGroups = std::atoi(valp);
        যদি (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_outputGroupsSize = std::atoll(valp);
        যদি (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        যদি (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        যদি (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_সাৰি);
    DECL_OPTION("-pins64", CbCall, [এতত্]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [এতত্]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_pinsBv = std::atoi(valp);
        যদি (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [এতত্](তার্কিক flag) {
        m_pinsScUint = flag;
        যদি (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [এতত্](তার্কিক flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [এতত্](তার্কিক flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        যদি (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [এতত্]() { m_public = মিছা; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [এতত্]() { m_profC = m_profCFuncs = সঁচা; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [এতত্]() {
        m_profC = m_profCFuncs = সঁচা;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_চাবি);
    DECL_OPTION("-protect-lib", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = সঁচা;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [এতত্](তার্কিক flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(সঁচা);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [এতত্](তার্কিক flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [এতত্](তার্কিক flag) {
        m_publicParams = flag;
        v3Global.dpi(সঁচা);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [এতত্](ধ্রুৱ ৱর্ণ* varp) { addParameter(varp, মিছা); });

    DECL_OPTION("-quiet", CbOnOff, [এতত্](তার্কিক flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_reloopLimit = std::atoi(valp);
        যদি (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_replicationLimit = std::atoi(valp);
        যদি (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [এতত্, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [এতত্]() {
        m_outFormatOk = সঁচা;
        m_systemC = সঁচা;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [এতত্](তার্কিক flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [এতত্](তার্কিক flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [এতত্]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [এতত্, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_threads = std::atoi(valp);
        যদি (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        যদি (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_hierThreads = std::atoi(valp);
        যদি (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        যদি (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = সঁচা;
            m_threadsDpiUnpure = সঁচা;
        } অন্যথা যদি (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = মিছা;
            m_threadsDpiUnpure = মিছা;
        } অন্যথা যদি (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = সঁচা;
            m_threadsDpiUnpure = মিছা;
        } অন্যথা {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        যদি (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        যদি (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        যদি (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, সঁচা);
        যদি (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        যদি (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [এতত্](ধ্রুৱ std::string& flag) { m_শীৰ্ষ_Module = Ast_গ্ৰন্থি::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [এতত্](ধ্রুৱ std::string& flag) { m_শীৰ্ষ_Module = Ast_গ্ৰন্থি::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_শীৰ্ষ_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [এতত্]() { m_traceEnabledSaif = সঁচা; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [এতত্]() {
        m_traceEnabledFst = সঁচা;
        addLdLibs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", Set, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", Set, &m_traceMaxWidth);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ধ্রুৱ ৱর্ণ*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_শীৰ্ষ);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [এতত্]() { m_traceEnabledVcd = সঁচা; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_গুনতি).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [এতত্]() {
        showVersion(সঁচা);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [এতত্, &optdir](ধ্রুৱ ৱর্ণ* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        পূর্ণাঙ্ক val = std::atoi(valp);
        যদি (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } অন্যথা যদি (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [এতত্]() {
        showVersion(মিছা);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_সাৰি::globalWarnOff(V3ErrorCode::I_LINT, মিছা); });
    DECL_OPTION("-Werror-", CbPartialMatch, [এতত্, fl](ধ্রুৱ ৱর্ণ* optp) {
        ধ্রুৱ V3ErrorCode code{optp};
        যদি (code == V3ErrorCode::EC_ERROR) {
            যদি (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } অন্যথা {
            V3Error::pretendError(code, সঁচা);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [এতত্](ধ্রুৱ ৱর্ণ* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](ধ্রুৱ ৱর্ণ* optp) VL_MT_DISABLED {
        ধ্রুৱ string err = File_সাৰি::globalWarnOffParse(optp, সঁচা);
        যদি (!err.empty()) {
            ধ্রুৱ string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    বাবে (পূর্ণাঙ্ক i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        বাবে (ধ্রুৱ string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [এতত্]() { m_context = মিছা; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(মিছা); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_সাৰি::globalWarnOff(V3ErrorCode::I_LINT, সঁচা); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_সাৰি::globalWarnOff(V3ErrorCode::I_STYLE, সঁচা); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [এতত্]() {
        m_pedantic = সঁচা;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, মিছা);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [এতত্, fl, &parser](ধ্রুৱ ৱর্ণ* optp) VL_MT_DISABLED {
        ধ্রুৱ V3ErrorCode code{optp};
        যদি (code == V3ErrorCode::EC_ERROR) {
            যদি (!isFuture(optp)) {
                ধ্রুৱ string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } অন্যথা {
            File_সাৰি::globalWarnOff(code, মিছা);
            V3Error::pretendError(code, মিছা);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_সাৰি::globalWarnOff(V3ErrorCode::I_LINT, মিছা); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_সাৰি::globalWarnOff(V3ErrorCode::I_STYLE, মিছা); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        যদি (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } অন্যথা যদি (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } অন্যথা যদি (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } অন্যথা যদি (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } অন্যথা {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [এতত্, fl](ধ্রুৱ ৱর্ণ* valp) {
        যদি (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } অন্যথা যদি (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } অন্যথা যদি (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } অন্যথা {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [এতত্, &optdir](ধ্রুৱ ৱর্ণ* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    বাবে (পূর্ণাঙ্ক i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        যদি (argv[i][0] == '-' || argv[i][0] == '+') {
            ধ্রুৱ std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            পূর্ণাঙ্ক consumed = 0;
            তার্কিক isForRerun = সঁচা;

            // Special case for -j which has an optional argument
            যদি (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                পূর্ণাঙ্ক val = 0;
                যদি (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    যদি (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                যদি (m_buildJobs == -1) m_buildJobs = val;
                যদি (m_verilateJobs == -1) m_verilateJobs = val;
                যদি (m_outputGroups == -1) m_outputGroups = val;
            } অন্যথা {
                ধ্রুৱ auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                যদি (consumed) {
                    // Already processed
                } অন্যথা যদি (isFuture0(argName)) {
                    consumed = 1;
                } অন্যথা যদি (isFuture1(argName)) {
                    consumed = 2;
                } অন্যথা {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            যদি (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            অনুৱর্ত;
        }

        // Filename
        ধ্রুৱ std::string filename = parseFileArg(optdir, argv[i]);
        তার্কিক isForRerun = সঁচা;
        যদি (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } অন্যথা যদি (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } অন্যথা যদি (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = মিছা;
        } অন্যথা {
            V3Options::addVFile(filename, work());
            isForRerun = মিছা;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    যদি (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

শূন্য V3Options::parseOptsFile(File_সাৰি* fl, ধ্রুৱ string& filename, তার্কিক rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ধ্রুৱ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    যদি (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        ঘূৰাই;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    তার্কিক has_fargs = মিছা;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ধ্রুৱ auto parse_fargs_cmt = [&](ধ্রুৱ ৱর্ণ* p) -> শূন্য {
        ধ্রুৱ তার্কিক blockCmt = p[1] == '*';
        p += 2;
        যেতিয়ালৈ (std::isspace(*p)) ++p;
        যদি (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            যদি (std::isspace(*p)) {
                যেতিয়ালৈ (std::isspace(*p)) ++p;
                যদি (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    যদি (std::isspace(*p)) {
                        যেতিয়ালৈ (std::isspace(*p)) ++p;
                        যদি (!has_fargs) {
                            has_fargs = সঁচা;
                            whole_file.clear();
                        }
                        whole_file += p;
                        যদি (blockCmt) {
                            ধ্রুৱ std::string::size_type pos = whole_file.find("*/");
                            যদি (pos == std::string::npos) {
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

    তার্কিক inCmt = মিছা;
    যেতিয়ালৈ (!ifp->eof()) {
        ধ্রুৱ std::string সাৰি = V3Os::getline(*ifp);
        // Strip simple comments
        ৱর্ণ lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        তার্কিক space_begin = সঁচা;  // At beginning or leading spaces only
        বাবে (ধ্রুৱ ৱর্ণ* p = সাৰি.c_str(); *p; lastch = *p++) {
            যদি (inCmt) {
                যদি (p[0] == '*' && p[1] == '/') {
                    inCmt = মিছা;
                    ++p;
                }
            } অন্যথা যদি (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                ভঙ্গ;  // Ignore to EOL
            } অন্যথা যদি (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                ভঙ্গ;  // Ignore to EOL
            } অন্যথা যদি (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = সঁচা;
                space_begin = মিছা;
                ++p;
            } অন্যথা {
                যদি (!std::isspace(*p)) space_begin = মিছা;
                যদি (!has_fargs) whole_file += *p;
            }
        }
        যদি (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    যদি (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = নূতন File_সাৰি{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    গণনা state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    বাবে (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        ৱর্ণ curr_char = whole_file[pos];
        ৱিকল্প (st) {
        পক্ষ ST_IN_OPTION:  // Get all chars up to a white space or a "="
            যদি (std::isspace(curr_char)) {  // End of option
                যদি (!arg.empty()) {  // End of word
                    args.ঠেলো_back(arg);
                }
                arg = "";
                ভঙ্গ;
            }
            যদি (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                ভঙ্গ;
            }
            যদি (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                যদি (pos < whole_file.length()) curr_char = whole_file[pos];
                যদি (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } অন্যথা {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                ভঙ্গ;
            }
            যদি (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                ভঙ্গ;
            }
            arg += curr_char;
            ভঙ্গ;
        পক্ষ ST_IN_QUOTED_STR:  // Just store all chars inside string
            যদি (curr_char != '\'') {
                arg += curr_char;
            } অন্যথা {  // End of quoted string
                st = ST_IN_OPTION;
            }
            ভঙ্গ;
        পক্ষ ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            যদি (curr_char != '"') {
                যদি (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } অন্যথা {
                    arg += curr_char;
                }
            } অন্যথা {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            ভঙ্গ;
        পক্ষ ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            ভঙ্গ;
        }
    }
    যদি (!arg.empty()) {  // Add last word
        args.ঠেলো_back(arg);
    }

    // Path
    ধ্রুৱ string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<ৱর্ণ*> argv;
    argv.reserve(args.size() + 1);
    বাবে (ধ্রুৱ string& i : args) argv.ঠেলো_back(const_cast<ৱর্ণ*>(i.c_str()));
    argv.ঠেলো_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<পূর্ণাঙ্ক>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(ধ্রুৱ string& optdir, ধ্রুৱ string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    যদি (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    ঘূৰাই filename;
}

//======================================================================

শূন্য V3Options::showVersion(তার্কিক verbose) {
    cout << version();
    cout << "\n";
    যদি (!verbose) ঘূৰাই;

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
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_মূল << "\n";
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
    m_impp = নূতন V3OptionsImp;

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

শূন্য V3Options::setDebugMode(পূর্ণাঙ্ক level) {
    V3Error::debugDefault(level);
    যদি (!m_dumpLevel.গুনতি("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = সঁচা;
    m_debugCheck = সঁচা;
    যদি (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(ধ্রুৱ string& tag) ধ্রুৱ VL_MT_SAFE {
    ধ্রুৱ auto iter = m_debugLevel.find(tag);
    ঘূৰাই iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(ধ্রুৱ string& srcfile_path) ধ্রুৱ VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ঘূৰাই debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(ধ্রুৱ string& tag) ধ্রুৱ VL_MT_SAFE {
    ধ্রুৱ auto iter = m_dumpLevel.find(tag);
    ঘূৰাই iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(ধ্রুৱ string& srcfile_path) ধ্রুৱ VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ঘূৰাই dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

তার্কিক V3Options::dump_বৃক্ষ_Addrids() ধ্রুৱ VL_MT_SAFE {
    স্থির পূর্ণাঙ্ক s_level = -1;
    যদি (VL_UNLIKELY(s_level < 0)) {
        ধ্রুৱ unsigned মান = dumpLevel("tree-addrids");
        যদি (!available()) ঘূৰাই মান > 0;
        s_level = static_cast<unsigned>(মান);
    }
    ঘূৰাই s_level > 0;
}

শূন্য V3Options::optimize(পূর্ণাঙ্ক level) {
    // Set all optimizations to on/off
    ধ্রুৱ তার্কিক flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_বৃক্ষ = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_বৃক্ষ = flag;
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
    যদি (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
