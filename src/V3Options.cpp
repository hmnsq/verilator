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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3Options.h"

#அந்தர்பாவ "V3Container.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3File.h"
#அந்தர்பாவ "V3Global.h"
#அந்தர்பாவ "V3Mutex.h"
#அந்தர்பாவ "V3OptionParser.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3PreShell.h"
#அந்தர்பாவ "V3String.h"

// clang-format off
#அந்தர்பாவ <sys/types.h>
#அந்தர்பாவ <sys/stat.h>
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#அந்தர்பாவ <algorithm>
#அந்தர்பாவ <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#இல்லையெனில்
# include <dirent.h>
#endif
#அந்தர்பாவ <fcntl.h>
#அந்தர்பாவ <list>
#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <thread>
#அந்தர்பாவ <set>
#அந்தர்பாவ <string>

#அந்தர்பாவ "config_rev.h"

#எனில் defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

வகுப்பு V3OptionsImp final {
பொது:
    // TYPES
    using DirMap = std::map<மாறிலி string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_வரி_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, தருக்க>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<மாறிலி string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    வெறுமை addIncDirUser(மாறிலி string& incdir) {
        மாறிலி string& dir = V3Os::filenameCleanup(incdir);
        மாறிலி தருக்க inserted = m_incDirUsers.செருகு(dir);
        எனில் (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    வெறுமை addIncDirFallback(மாறிலி string& incdir) {
        மாறிலி string& dir = V3Os::filenameCleanup(incdir);
        எனில் (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            மாறிலி auto itFoundPair = m_incDirFallbackSet.செருகு(dir);
            எனில் (itFoundPair.second) m_incDirFallbacks.தள்ளு_back(dir);
        }
    }
    வெறுமை addLangExt(மாறிலி string& langext, மாறிலி V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        எனில் (addext[0] == '.') addext = addext.substr(1);
        (வெறுமை)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    வெறுமை addLibExtV(மாறிலி string& libext) { m_libExtVs.செருகு(libext); }
    V3OptionsImp() = இயல்பு;
    ~V3OptionsImp() = இயல்பு;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(மாறிலி எழுத்து* textp) {
    // Return code for given string, or ERROR, which is a bad code
    க்கு (முழு codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        மாறிலி V3LangCode code{codei};
        எனில் (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            திரும்பு;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(மாறிலி string& மதிப்பு, தருக்க& badr)
    : m_e{VTimescale::NONE} {
    badr = உண்மை;
    மாறிலி string spaceless = VString::removeWhitespace(மதிப்பு);
    க்கு (முழு i = TS_100S; i < _ENUM_END; ++i) {
        மாறிலி VTimescale ts{i};
        எனில் (spaceless == ts.ascii()) {
            badr = பொய்;
            m_e = ts.m_e;
            நிறுத்து;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(மாறிலி string& opts) {
    VStringList vals;
    தருக்க inStr = பொய்;
    string cur;
    நிலையான மாறிலி string hierBlock("--hierarchical-block");
    File_வரி cmdfl(File_வரி::command_வரி_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    க்கு (string::const_iterator it = opts.begin(); it != opts.end();) {
        எனில் (inStr) {
            எனில் (*it == '\\') {
                ++it;
                எனில் (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    நிறுத்து;
                }
                எனில் (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    நிறுத்து;
                }
                cur.தள்ளு_back(*it);
                ++it;
            } இல்லையெனில் எனில் (*it == '"') {  // end of string
                cur.தள்ளு_back(*it);
                vals.தள்ளு_back(cur);
                cur.clear();
                ++it;
                எனில் (it != opts.end()) {
                    எனில் (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        நிறுத்து;
                    }
                    ++it;
                    எனில் (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        நிறுத்து;
                    }
                    inStr = *it == '"';
                    cur.தள்ளு_back(*it);
                    ++it;
                }
            } இல்லையெனில் {
                cur.தள்ளு_back(*it);
                ++it;
            }
        } இல்லையெனில் {
            எனில் (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                நிறுத்து;
            }
            எனில் (*it == ',') {  // end of this parameter
                vals.தள்ளு_back(cur);
                cur.clear();
                ++it;
                எனில் (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    நிறுத்து;
                }
                inStr = *it == '"';
            }
            cur.தள்ளு_back(*it);
            ++it;
        }
    }
    எனில் (!cur.empty()) vals.தள்ளு_back(cur);
    எனில் (vals.size() >= 2) {
        எனில் (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } இல்லையெனில் {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    க்கு (size_t i = 2; i + 1 < vals.size(); i += 2) {
        மாறிலி தருக்க inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        எனில் (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

வெறுமை VTimescale::parseSlashed(File_வரி* fl, மாறிலி எழுத்து* textp, VTimescale& unitr,
                              VTimescale& precr, தருக்க allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    மாறிலி எழுத்து* cp = textp;
    க்கு (; std::isspace(*cp); ++cp) {}
    மாறிலி எழுத்து* மாறிலி unitp = cp;
    க்கு (; *cp && *cp != '/'; ++cp) {}
    மாறிலி string unitStr(unitp, cp - unitp);
    க்கு (; std::isspace(*cp); ++cp) {}
    string precStr;
    எனில் (*cp == '/') {
        ++cp;
        க்கு (; std::isspace(*cp); ++cp) {}
        மாறிலி எழுத்து* மாறிலி precp = cp;
        க்கு (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    க்கு (; std::isspace(*cp); ++cp) {}
    எனில் (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        திரும்பு;
    }

    தருக்க unitbad = பொய்;
    மாறிலி VTimescale unit{unitStr, unitbad /*ref*/};
    எனில் (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        திரும்பு;
    }
    unitr = unit;

    எனில் (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        தருக்க precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        எனில் (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            திரும்பு;
        }
        எனில் (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            திரும்பு;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

வெறுமை V3Options::addIncDirUser(மாறிலி string& incdir) { m_impp->addIncDirUser(incdir); }
வெறுமை V3Options::addIncDirFallback(மாறிலி string& incdir) { m_impp->addIncDirFallback(incdir); }
வெறுமை V3Options::addLangExt(மாறிலி string& langext, மாறிலி V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
வெறுமை V3Options::addLibExtV(மாறிலி string& libext) { m_impp->addLibExtV(libext); }
வெறுமை V3Options::addDefine(மாறிலி string& defline, தருக்க allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string இடது = defline;
    போது (இடது != "") {
        string def = இடது;
        string::size_type pos;
        எனில் (allowPlus && ((pos = இடது.find('+')) != string::npos)) {
            இடது = இடது.substr(pos + 1);
            def.erase(pos);
        } இல்லையெனில் {
            இடது = "";
        }
        string மதிப்பு;
        எனில் ((pos = def.find('=')) != string::npos) {
            மதிப்பு = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_வரி(def, மதிப்பு);
    }
}
வெறுமை V3Options::addParameter(மாறிலி string& paramline, தருக்க allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string இடது = paramline;
    போது (இடது != "") {
        string param = இடது;
        string::size_type pos;
        எனில் (allowPlus && ((pos = இடது.find('+')) != string::npos)) {
            இடது = இடது.substr(pos + 1);
            param.erase(pos);
        } இல்லையெனில் {
            இடது = "";
        }
        string மதிப்பு;
        எனில் ((pos = param.find('=')) != string::npos) {
            மதிப்பு = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << மதிப்பு);
        (வெறுமை)m_parameters.erase(param);
        m_parameters[param] = மதிப்பு;
    }
}

தருக்க V3Options::hasParameter(மாறிலி string& name) {
    திரும்பு m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(மாறிலி string& name) {
    string மதிப்பு = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    திரும்பு மதிப்பு;
}

வெறுமை V3Options::checkParameters() {
    எனில் (!m_parameters.empty()) {
        std::stringstream செய்தி;
        செய்தி << "Parameters from the command line were not found in the design:";
        க்கு (மாறிலி auto& i : m_parameters) செய்தி << " " << i.first;
        v3error(செய்தி.str());
    }
}

வெறுமை V3Options::addCppFile(மாறிலி string& filename) { m_cppFiles.செருகு(filename); }
வெறுமை V3Options::addCFlags(மாறிலி string& filename) { m_cFlags.தள்ளு_back(filename); }
வெறுமை V3Options::addLibMapFile(மாறிலி string& filename) { m_libmapFiles.செருகு(filename); }
வெறுமை V3Options::addCompilerIncludes(மாறிலி string& filename) {
    m_compilerIncludes.செருகு(filename);
}
வெறுமை V3Options::addLdLibFile(மாறிலி string& filename) {
    m_ldLibFiles.தள்ளு_back(filename);
    addLdLibs(filename);
}
வெறுமை V3Options::addLdLibs(மாறிலி string& filename) { m_ldLibs.தள்ளு_back(filename); }
வெறுமை V3Options::addMakeFlags(மாறிலி string& filename) { m_makeFlags.தள்ளு_back(filename); }
வெறுமை V3Options::addFuture(மாறிலி string& flag) { m_futures.செருகு(flag); }
வெறுமை V3Options::addFuture0(மாறிலி string& flag) { m_future0s.செருகு(flag); }
வெறுமை V3Options::addFuture1(மாறிலி string& flag) { m_future1s.செருகு(flag); }
தருக்க V3Options::isFuture(மாறிலி string& flag) மாறிலி {
    திரும்பு m_futures.find(flag) != m_futures.end();
}
தருக்க V3Options::isFuture0(மாறிலி string& flag) மாறிலி {
    திரும்பு m_future0s.find(flag) != m_future0s.end();
}
தருக்க V3Options::isFuture1(மாறிலி string& flag) மாறிலி {
    திரும்பு m_future1s.find(flag) != m_future1s.end();
}
தருக்க V3Options::isLibraryFile(மாறிலி string& filename, மாறிலி string& libname) மாறிலி {
    திரும்பு m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
வெறுமை V3Options::addLibraryFile(மாறிலி string& filename, மாறிலி string& libname) {
    m_libraryFiles.செருகு({filename, libname});
}
வெறுமை V3Options::addVFile(மாறிலி string& filename, மாறிலி string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.தள்ளு_back({filename, libname});  // Not emplace
}
வெறுமை V3Options::addVltFile(மாறிலி string& filename, மாறிலி string& libname) {
    m_vltFiles.செருகு({filename, libname});
}
வெறுமை V3Options::addForceInc(மாறிலி string& filename) { m_forceIncs.தள்ளு_back(filename); }

வெறுமை V3Options::add_வரி_Arg(மாறிலி string& arg) { m_impp->m_வரி_Args.தள்ளு_back(arg); }

வெறுமை V3Options::addArg(எழுத்து** argv, size_t மொத்தம், தருக்க isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, தருக்க>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    க்கு (size_t n = 0; n < மொத்தம்; ++n) pair.first.emplace_back(argv[n]);
}

வெறுமை V3Options::addArg(மாறிலி std::string& arg, தருக்க isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, தருக்க>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

மாறிலி std::list<std::pair<std::list<std::string>, தருக்க>>& V3Options::allArgs() மாறிலி {
    திரும்பு m_impp->m_allArgs;
}

string V3Options::allArgsString() மாறிலி VL_MT_SAFE {
    string result;
    க்கு (மாறிலி auto& pair : m_impp->m_allArgs) {
        க்கு (மாறிலி string& arg : pair.first) {
            எனில் (!result.empty()) result += " ";
            result += arg;
        }
    }
    திரும்பு result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(தருக்க for_உச்சி) மாறிலி {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    க்கு (மாறிலி auto& vFile : m_vFiles) vFiles.செருகு(vFile.filename());
    string out;
    தருக்க stripArg = பொய்;
    தருக்க strip_Arg_If_இலக்கம் = பொய்;
    க்கு (மாறிலி auto& pair : m_impp->m_allArgs) {
        க்கு (மாறிலி string& arg : pair.first) {
            எனில் (stripArg) {
                stripArg = பொய்;
                தொடர்;
            }
            எனில் (strip_Arg_If_இலக்கம்) {
                strip_Arg_If_இலக்கம் = பொய்;
                எனில் (isdigit(arg[0])) தொடர்;
            }
            முழு skip = 0;
            எனில் (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } இல்லையெனில் எனில் (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            எனில் (skip > 0) {  // arg is an option
                மாறிலி string opt = arg.substr(skip);  // Remove '-' in the beginning
                மாறிலி முழு இலக்கம்_Strip = stripOptionsForChildRun(opt, for_உச்சி);
                எனில் (இலக்கம்_Strip) {
                    UASSERT(0 <= இலக்கம்_Strip && இலக்கம்_Strip <= 3, "should be one of 0, 1, 2, 3");
                    எனில் (இலக்கம்_Strip == 2) stripArg = உண்மை;
                    எனில் (இலக்கம்_Strip == 3) strip_Arg_If_இலக்கம் = உண்மை;
                    தொடர்;
                }
            } இல்லையெனில் {  // Not an option
                எனில் (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    தொடர்;
                }
            }
            எனில் (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    திரும்பு out;
}

வெறுமை V3Options::ccSet() {  // --cc
    m_outFormatOk = உண்மை;
    m_systemC = பொய்;
}

வெறுமை V3Options::decorations(File_வரி* fl, மாறிலி string& arg) {  // --decorations
    எனில் (arg == "none") {
        m_decoration = பொய்;
        m_decorationNodes = பொய்;
    } இல்லையெனில் எனில் (arg == "node") {
        m_decoration = உண்மை;
        m_decorationNodes = உண்மை;
    } இல்லையெனில் எனில் (arg == "medium") {
        m_decoration = உண்மை;
        m_decorationNodes = பொய்;
    } இல்லையெனில் {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() மாறிலி VL_MT_SAFE {
    std::vector<std::string> result;
    எனில் (traceEnabledFst()) result.emplace_back("VerilatedFst");
    எனில் (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    எனில் (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    திரும்பு result;
}
std::vector<std::string> V3Options::traceClassLangs() மாறிலி VL_MT_SAFE {
    std::vector<std::string> result;
    க்கு (மாறிலி auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    திரும்பு result;
}
std::vector<std::string> V3Options::traceSourceBases() மாறிலி VL_MT_SAFE {
    std::vector<std::string> result;
    எனில் (traceEnabledFst()) result.emplace_back("verilated_fst");
    எனில் (traceEnabledSaif()) result.emplace_back("verilated_saif");
    எனில் (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    திரும்பு result;
}
std::vector<std::string> V3Options::traceSourceLangs() மாறிலி VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    க்கு (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    திரும்பு result;
}
std::string V3Options::traceClassBase() மாறிலி VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    திரும்பு traceClassBases().front();
}
std::string V3Options::traceClassLang() மாறிலி VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    திரும்பு traceClassLangs().front();
}

//######################################################################
// File searching

தருக்க V3Options::fileStatNormal(மாறிலி string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    கட்டமைப்பு stat sstat;  // Stat information
    மாறிலி முழு err = stat(filename.c_str(), &sstat);
    எனில் (err != 0) திரும்பு பொய்;
    எனில் (S_ISDIR(sstat.st_mode)) திரும்பு பொய்;
    திரும்பு உண்மை;
}

string V3Options::fileExists(மாறிலி string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    மாறிலி string dir = V3Os::filenameDir(filename);
    மாறிலி string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    எனில் (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* மாறிலி setp = &(diriter->second);

#ifdef _MSC_VER
        முயல் {
            க்கு (மாறிலி auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->செருகு(dirEntry.path().filename().string());
        } பிடி (std::filesystem::filesystem_error மாறிலி& ex) {
            (வெறுமை)ex;
            திரும்பு "";
        }
#இல்லையெனில்
        எனில் (DIR* மாறிலி dirp = opendir(dir.c_str())) {
            போது (கட்டமைப்பு dirent* direntp = readdir(dirp)) setp->செருகு(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    மாறிலி std::set<string>* மாறிலி filesetp = &(diriter->second);
    மாறிலி auto fileiter = filesetp->find(basename);
    எனில் (fileiter == filesetp->end()) திரும்பு "";  // Not found
    // Check if it is a directory, ignore if so
    மாறிலி string filenameOut = V3Os::filenameJoin(dir, basename);
    எனில் (!fileStatNormal(filenameOut)) திரும்பு "";  // Directory
    திரும்பு filenameOut;
}

string V3Options::filePathCheckOneDir(மாறிலி string& modname, மாறிலி string& dirname) {
    க்கு (மாறிலி string& i : m_impp->m_libExtVs) {
        மாறிலி string fn = V3Os::filenameJoin(dirname, modname + i);
        மாறிலி string exists = fileExists(fn);
        எனில் (exists != "") திரும்பு exists;
    }
    திரும்பு "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
முழு V3Options::stripOptionsForChildRun(மாறிலி string& opt, தருக்க for_உச்சி) {
    // Options to strip for both the top wrapper, and hier blocks
    நிலையான மாறிலி std::unordered_map<std::string, முழு> commonOpts{
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
    எனில் (commonOpts.மொத்தம்(opt)) திரும்பு commonOpts.at(opt);

    // Options to strip only for hier blocks
    எனில் (!for_உச்சி) {
        நிலையான மாறிலி std::unordered_map<std::string, முழு> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        எனில் (subOpts.மொத்தம்(opt)) திரும்பு subOpts.at(opt);
        எனில் (VString::startsWith(opt, "G")) திரும்பு 1;
        எனில் (VString::startsWith(opt, "pvalue")) திரும்பு 1;
    }

    // Do not strip
    திரும்பு 0;
}

வெறுமை V3Options::validateIdentifier(File_வரி* fl, மாறிலி string& arg, மாறிலி string& opt) {
    எனில் (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_வரி* fl, மாறிலி string& modname, மாறிலி string& lastpath,
                           மாறிலி string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    மாறிலி string filename = V3Os::filenameCleanup(VName::dehash(modname));
    எனில் (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        மாறிலி string exists = filePathCheckOneDir(filename, "");
        எனில் (exists != "") திரும்பு exists;
    }
    க்கு (மாறிலி string& dir : m_impp->m_incDirUsers) {
        மாறிலி string exists = filePathCheckOneDir(filename, dir);
        எனில் (exists != "") திரும்பு exists;
    }
    க்கு (மாறிலி string& dir : m_impp->m_incDirFallbacks) {
        மாறிலி string exists = filePathCheckOneDir(filename, dir);
        எனில் (exists != "") திரும்பு exists;
    }

    எனில் (m_relativeIncludes) {
        மாறிலி string exists = filePathCheckOneDir(filename, lastpath);
        எனில் (exists != "") திரும்பு V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    எனில் (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_செய்தி(fl, filename));
    }
    திரும்பு "";
}

string V3Options::file_Path_Looked_செய்தி(File_வரி* /*fl*/, மாறிலி string& modname) {
    நிலையான தருக்க s_shown_notfound_செய்தி = பொய்;
    std::ostringstream ss;
    எனில் (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } இல்லையெனில் எனில் (!s_shown_notfound_செய்தி) {
        s_shown_notfound_செய்தி = உண்மை;
        எனில் (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        க்கு (மாறிலி string& dir : m_impp->m_incDirUsers) {
            க்கு (மாறிலி string& ext : m_impp->m_libExtVs) {
                மாறிலி string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        க்கு (மாறிலி string& dir : m_impp->m_incDirFallbacks) {
            க்கு (மாறிலி string& ext : m_impp->m_libExtVs) {
                மாறிலி string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    திரும்பு ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(மாறிலி string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    எனில் (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        திரும்பு V3LangCode::mostRecent();
    } இல்லையெனில் எனில் ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        மாறிலி auto it = m_impp->m_langExts.find(ext);
        எனில் (it != m_impp->m_langExts.end()) திரும்பு it->second;
    }
    திரும்பு m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(மாறிலி string& var) {
    // If update below, also update V3Options::showVersion()
    எனில் (var == "MAKE") {
        திரும்பு getenvMAKE();
    } இல்லையெனில் எனில் (var == "PERL") {
        திரும்பு getenvPERL();
    } இல்லையெனில் எனில் (var == "PYTHON3") {
        திரும்பு getenvPYTHON3();
    } இல்லையெனில் எனில் (var == "SYSTEMC") {
        திரும்பு getenvSYSTEMC();
    } இல்லையெனில் எனில் (var == "SYSTEMC_ARCH") {
        திரும்பு getenvSYSTEMC_ARCH();
    } இல்லையெனில் எனில் (var == "SYSTEMC_INCLUDE") {
        திரும்பு getenvSYSTEMC_INCLUDE();
    } இல்லையெனில் எனில் (var == "SYSTEMC_LIBDIR") {
        திரும்பு getenvSYSTEMC_LIBDIR();
    } இல்லையெனில் எனில் (var == "VERILATOR_ROOT") {
        திரும்பு getenv_VERILATOR_வேர்();
    } இல்லையெனில் {
        திரும்பு V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { திரும்பு V3Os::getenvStr("MAKE", "gmake"); }
#இல்லையெனில்
string V3Options::getenvMAKE() { திரும்பு V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    திரும்பு V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    திரும்பு V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    திரும்பு V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    மாறிலி string defenv = string{DEFENV_SYSTEMC};
    எனில் (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    திரும்பு V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    மாறிலி string defenv = string{DEFENV_SYSTEMC_ARCH};
    எனில் (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    எனில் (var == "") {
#எனில் defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        மாறிலி string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#அல்லதெனில் defined(_WIN32)
        மாறிலி string sysname = "WIN32";
        var = "win32";
#இல்லையெனில்
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        கட்டமைப்பு utsname uts;
        uname(&uts);
        மாறிலி string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        எனில் (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } இல்லையெனில் எனில் (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } இல்லையெனில் {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    திரும்பு var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    மாறிலி string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    எனில் (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    எனில் (var == "") {
        மாறிலி string sc = getenvSYSTEMC();
        எனில் (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    திரும்பு V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    மாறிலி string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    எனில் (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    எனில் (var == "") {
        மாறிலி string sc = getenvSYSTEMC();
        மாறிலி string arch = getenvSYSTEMC_ARCH();
        எனில் (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    திரும்பு V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_வேர்() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    மாறிலி string defenv = string{DEFENV_VERILATOR_வேர்};
    எனில் (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    எனில் (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    திரும்பு V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    மாறிலி string defenv = string{DEFENV_VERILATOR_SOLVER};
    எனில் (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    திரும்பு var;
}

string V3Options::getStdPackagePath() {
    திரும்பு V3Os::filenameJoin(getenv_VERILATOR_வேர்(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    திரும்பு V3Os::filenameJoin(getenv_VERILATOR_வேர்(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(மாறிலி string& var) {
    // If update below, also update V3Options::showVersion()
    எனில் (var == "COROUTINES" && coroutineSupport()) {
        திரும்பு "1";
    } இல்லையெனில் எனில் (var == "TSAN" && tsanSupport()) {
        திரும்பு "1";
    } இல்லையெனில் எனில் (var == "DEV_ASAN" && devAsan()) {
        திரும்பு "1";
    } இல்லையெனில் எனில் (var == "DEV_GCOV" && devGcov()) {
        திரும்பு "1";
        // cppcheck-suppress knownConditionTrueFalse
    } இல்லையெனில் எனில் (var == "SYSTEMC" && systemCFound()) {
        திரும்பு "1";
    } இல்லையெனில் {
        திரும்பு "";
    }
}

தருக்க V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    திரும்பு உண்மை;
#இல்லையெனில்
    திரும்பு பொய்;
#endif
}

தருக்க V3Options::systemCFound() {
    திரும்பு (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

தருக்க V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    திரும்பு உண்மை;
#இல்லையெனில்
    திரும்பு பொய்;
#endif
}

தருக்க V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    திரும்பு உண்மை;
#இல்லையெனில்
    திரும்பு பொய்;
#endif
}

தருக்க V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    திரும்பு உண்மை;
#இல்லையெனில்
    திரும்பு பொய்;
#endif
}

தருக்க V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    திரும்பு உண்மை;
#இல்லையெனில்
    திரும்பு பொய்;
#endif
}

//######################################################################
// V3 Options notification methods

வெறுமை V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_வரி* மாறிலி cmdfl = புதிய File_வரி{File_வரி::command_வரி_Filename()};

    எனில் (!outFormatOk() && v3Global.opt.முதன்மை()) ccSet();  // --main implies --cc if not provided
    எனில் (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    எனில் (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    எனில் (m_build) {
        எனில் (m_இருமம்)
            backendFlags.emplace_back("--binary");
        இல்லையெனில்
            backendFlags.emplace_back("--build");
    }
    எனில் (m_preprocOnly) backendFlags.emplace_back("-E");
    எனில் (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    எனில் (m_lintOnly) backendFlags.emplace_back("--lint-only");
    எனில் (m_jsonOnly) backendFlags.emplace_back("--json-only");
    எனில் (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        க்கு (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    எனில் (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    எனில் (!m_gmake && !m_makeJson) m_gmake = உண்மை;

    எனில் (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    எனில் (m_hierChild) {
        எனில் (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = பொய்;
    }

    எனில் (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = உண்மை;  // No format, with --trace means wanted --trace-vcd
    }
    எனில் (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = உண்மை;
    மாறிலி முழு ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    எனில் (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    எனில் (protectIds()) {
        எனில் (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        எனில் (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        எனில் (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    எனில் (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    எனில் (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    எனில் (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(உண்மை);

    // Default split limits if not specified
    எனில் (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    எனில் (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    எனில் (v3Global.opt.முதன்மை() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    எனில் (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        எனில் (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    எனில் (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    எனில் (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    எனில் (!m_dumpLevel.மொத்தம்("tree") && m_dumpLevel.மொத்தம்("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    எனில் (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    எனில் (m_buildJobs == -1) m_buildJobs = 1;
    எனில் (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    எனில் (timing().isSetTrue()) V3PreShell::define_Cmd_வரி("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    எனில் (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(பொய்);

    // === Leave last
    // Mark options as available
    m_available = உண்மை;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    திரும்பு ver;
}

string V3Options::protect_சாவி_Defaulted() VL_MT_SAFE {
    நிலையான V3Mutex s_mutex;
    மாறிலி V3LockGuard lock{s_mutex};
    எனில் (m_protect_சாவி.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_சாவி = "VL-KEY-" + digest.digestBase64();
    }
    திரும்பு m_protect_சாவி;
}

வெறுமை V3Options::throwSigsegv() {  // LCOV_EXCL_START
#எனில் !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<நிலையற்ற எழுத்து*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(மாறிலி VTimescale& flag) மாறிலி {
    எனில் (!timeOverridePrec().isNone()) {
        திரும்பு timeOverridePrec();
    } இல்லையெனில் எனில் (flag.isNone()) {
        திரும்பு timeDefaultPrec();
    } இல்லையெனில் {
        திரும்பு flag;
    }
}

VTimescale V3Options::timeComputeUnit(மாறிலி VTimescale& flag) மாறிலி {
    எனில் (!timeOverrideUnit().isNone()) {
        திரும்பு timeOverrideUnit();
    } இல்லையெனில் எனில் (flag.isNone()) {
        திரும்பு timeDefaultUnit();
    } இல்லையெனில் {
        திரும்பு flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(முழு argc, எழுத்து** argv) {
    // Return list of arguments as simple string
    string opts;
    க்கு (முழு i = 0; i < argc; ++i) {
        எனில் (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    திரும்பு opts;
}

//######################################################################
// V3 Options Parsing

வெறுமை V3Options::parseOpts(File_வரி* fl, முழு argc, எழுத்து** argv) VL_MT_DISABLED {
    // Save command line options
    க்கு (முழு i = 0; i < argc; ++i) add_வரி_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    எனில் (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    எனில் (prefix() == "" && உச்சி_Module() != "") m_prefix = "V"s + உச்சி_Module();  // Already encoded
    எனில் (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + Ast_முனை::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    எனில் (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

தருக்க V3Options::suffixed(மாறிலி string& sw, மாறிலி எழுத்து* arg) {
    எனில் (std::strlen(arg) > sw.length()) திரும்பு பொய்;
    திரும்பு (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

வெறுமை V3Options::parseOptsList(File_வரி* fl, மாறிலி string& optdir, முழு argc,
                              எழுத்து** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    மாறிலி V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    மாறிலி auto callStrSetter = [இது](வெறுமை (V3Options::*cbStr)(மாறிலி string&)) {
        திரும்பு [இது, cbStr](மாறிலி string& v) { (இது->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) VL_MT_DISABLED {
        addDefine(optp, உண்மை);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [இது, &optdir](மாறிலி எழுத்து* optp) {
        string dirs = optp;
        string::size_type pos;
        போது ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        string exts = optp;
        string::size_type pos;
        போது ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](தருக்க) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [இது](தருக்க flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](மாறிலி எழுத்து*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [இது](தருக்க flag) {
        m_bboxUnsup = flag;
        File_வரி::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, உண்மை);
    });
    DECL_OPTION("-binary", CbCall, [இது]() {
        m_இருமம் = உண்மை;
        m_build = உண்மை;
        m_exe = உண்மை;
        m_main = உண்மை;
        எனில் (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        முழு val = std::atoi(valp);
        எனில் (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } இல்லையெனில் எனில் (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [இது]() { ccSet(); });
    DECL_OPTION("-clk", CbVal, [fl](மாறிலி std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](மாறிலி std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", Set, &m_compLimitBlocks).undocumented();
    DECL_OPTION("-comp-limit-members", Set,
                &m_compLimitMembers)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", Set, &m_compLimitParens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](முழு val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        எனில் (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } இல்லையெனில் எனில் (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } இல்லையெனில் எனில் (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } இல்லையெனில் {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [இது](தருக்க flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_கோவை);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_கோவை_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [இது, fl](மாறிலி எழுத்து* மாறிலி valp) {
        எனில் (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } இல்லையெனில் எனில் (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } இல்லையெனில் எனில் (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } இல்லையெனில் {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_வரி);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    மாறிலி auto parseBinsLimit = [fl](மாறிலி எழுத்து* optp, மாறிலி எழுத்து* valp, uint32_t& limitr) {
        எழுத்து* endp = nullptr;
        மாறிலி unsigned long long மதிப்பு = std::strtoull(valp, &endp, 10);
        எனில் (*endp || மதிப்பு < 1 || மதிப்பு > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            திரும்பு;
        }
        limitr = static_cast<uint32_t>(மதிப்பு);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [இது, parseBinsLimit](மாறிலி எழுத்து* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [இது, parseBinsLimit](மாறிலி எழுத்து* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [இது](மாறிலி எழுத்து* valp) VL_MT_DISABLED {
        addDefine(valp, பொய்);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [இது]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [இது](முழு v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [இது](மாறிலி எழுத்து* optp, மாறிலி எழுத்து* valp) {
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_அடுக்கு_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [இது, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [இது, fl](மாறிலி எழுத்து* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [இது, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [இது](மாறிலி எழுத்து* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = உண்மை;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [இது](மாறிலி எழுத்து* optp, மாறிலி எழுத்து* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [இது](தருக்க flag) {
        எனில் (flag) {
            m_stdPackage = பொய்;
            m_stdWaiver = பொய்;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<வெறுமை (*)(முழு)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [இது](மாறிலி எழுத்து* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [இது, fl, &optdir](மாறிலி எழுத்து* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), உண்மை);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [இது, &optdir](மாறிலி எழுத்து* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [இது, fl, &optdir](மாறிலி எழுத்து* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), பொய்);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [இது](மாறிலி எழுத்து* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [இது](மாறிலி எழுத்து* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [இது](தருக்க flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_மரம் = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_மரம்);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_மரம்);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [இது](தருக்க flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [இது](தருக்க flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](தருக்க) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](தருக்க) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](தருக்க) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_தள்ளு_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [இது, fl](தருக்க flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [இது](தருக்க flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [இது](தருக்க flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](தருக்க) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](தருக்க) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        எனில் (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](தருக்க) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) { addParameter(optp, பொய்); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [இது]() {
        cout << protect_சாவி_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](மாறிலி எழுத்து* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](மாறிலி எழுத்து* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [இது](மாறிலி எழுத்து* valp) {
        மாறிலி V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [இது](மாறிலி எழுத்து* optp) { m_hierParamsFile.தள்ளு_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [இது, &optdir](மாறிலி எழுத்து* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [இது, fl](முழு val) {
        m_instr_மொத்தம்_Dpi = val;
        எனில் (m_instr_மொத்தம்_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [இது](மாறிலி எழுத்து* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = உண்மை;
    });
    DECL_OPTION("-json-only-output", CbVal, [இது](மாறிலி எழுத்து* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = உண்மை;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [இது]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [இது]() { m_l2Name = "v"; }).undocumented();  // Historical
    மாறிலி auto setLang = [இது, fl](மாறிலி எழுத்து* valp) {
        மாறிலி V3LangCode optval{valp};
        எனில் (optval.legal()) {
            m_defaultLanguage = optval;
        } இல்லையெனில் {
            VSpellCheck spell;
            க்கு (முழு i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.தள்ளு_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_செய்தி(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [இது](மாறிலி எழுத்து* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [இது](மாறிலி எழுத்து* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_உச்சி_Name);
    DECL_OPTION("-make", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        எனில் (!std::strcmp(valp, "gmake")) {
            m_gmake = உண்மை;
        } இல்லையெனில் எனில் (!std::strcmp(valp, "json")) {
            m_makeJson = உண்மை;
        } இல்லையெனில் {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_இலக்கம்_Width);
    DECL_OPTION("-mod-prefix", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [இது]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [இது]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [இது]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [இது]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](தருக்க /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_outputGroups = std::atoi(valp);
        எனில் (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_outputGroupsSize = std::atoll(valp);
        எனில் (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        எனில் (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        எனில் (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_வரி);
    DECL_OPTION("-pins64", CbCall, [இது]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [இது]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_pinsBv = std::atoi(valp);
        எனில் (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [இது](தருக்க flag) {
        m_pinsScUint = flag;
        எனில் (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [இது](தருக்க flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [இது](தருக்க flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        எனில் (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [இது]() { m_public = பொய்; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [இது]() { m_profC = m_profCFuncs = உண்மை; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [இது]() {
        m_profC = m_profCFuncs = உண்மை;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_சாவி);
    DECL_OPTION("-protect-lib", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = உண்மை;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [இது](தருக்க flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(உண்மை);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [இது](தருக்க flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [இது](தருக்க flag) {
        m_publicParams = flag;
        v3Global.dpi(உண்மை);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [இது](மாறிலி எழுத்து* varp) { addParameter(varp, பொய்); });

    DECL_OPTION("-quiet", CbOnOff, [இது](தருக்க flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_reloopLimit = std::atoi(valp);
        எனில் (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_replicationLimit = std::atoi(valp);
        எனில் (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [இது, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [இது]() {
        m_outFormatOk = உண்மை;
        m_systemC = உண்மை;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [இது](தருக்க flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [இது](தருக்க flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [இது]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [இது, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_threads = std::atoi(valp);
        எனில் (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        எனில் (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_hierThreads = std::atoi(valp);
        எனில் (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        எனில் (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = உண்மை;
            m_threadsDpiUnpure = உண்மை;
        } இல்லையெனில் எனில் (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = பொய்;
            m_threadsDpiUnpure = பொய்;
        } இல்லையெனில் எனில் (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = உண்மை;
            m_threadsDpiUnpure = பொய்;
        } இல்லையெனில் {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        எனில் (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        எனில் (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        எனில் (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, உண்மை);
        எனில் (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        எனில் (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [இது](மாறிலி std::string& flag) { m_உச்சி_Module = Ast_முனை::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [இது](மாறிலி std::string& flag) { m_உச்சி_Module = Ast_முனை::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_உச்சி_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [இது]() { m_traceEnabledSaif = உண்மை; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [இது]() {
        m_traceEnabledFst = உண்மை;
        addLdLibs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", Set, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", Set, &m_traceMaxWidth);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](மாறிலி எழுத்து*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_உச்சி);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [இது]() { m_traceEnabledVcd = உண்மை; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_மொத்தம்).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [இது]() {
        showVersion(உண்மை);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [இது, &optdir](மாறிலி எழுத்து* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        முழு val = std::atoi(valp);
        எனில் (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } இல்லையெனில் எனில் (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [இது]() {
        showVersion(பொய்);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_வரி::globalWarnOff(V3ErrorCode::I_LINT, பொய்); });
    DECL_OPTION("-Werror-", CbPartialMatch, [இது, fl](மாறிலி எழுத்து* optp) {
        மாறிலி V3ErrorCode code{optp};
        எனில் (code == V3ErrorCode::EC_ERROR) {
            எனில் (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } இல்லையெனில் {
            V3Error::pretendError(code, உண்மை);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [இது](மாறிலி எழுத்து* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](மாறிலி எழுத்து* optp) VL_MT_DISABLED {
        மாறிலி string err = File_வரி::globalWarnOffParse(optp, உண்மை);
        எனில் (!err.empty()) {
            மாறிலி string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    க்கு (முழு i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        க்கு (மாறிலி string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [இது]() { m_context = பொய்; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(பொய்); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_வரி::globalWarnOff(V3ErrorCode::I_LINT, உண்மை); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_வரி::globalWarnOff(V3ErrorCode::I_STYLE, உண்மை); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [இது]() {
        m_pedantic = உண்மை;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, பொய்);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [இது, fl, &parser](மாறிலி எழுத்து* optp) VL_MT_DISABLED {
        மாறிலி V3ErrorCode code{optp};
        எனில் (code == V3ErrorCode::EC_ERROR) {
            எனில் (!isFuture(optp)) {
                மாறிலி string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } இல்லையெனில் {
            File_வரி::globalWarnOff(code, பொய்);
            V3Error::pretendError(code, பொய்);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_வரி::globalWarnOff(V3ErrorCode::I_LINT, பொய்); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_வரி::globalWarnOff(V3ErrorCode::I_STYLE, பொய்); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        எனில் (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } இல்லையெனில் எனில் (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } இல்லையெனில் எனில் (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } இல்லையெனில் எனில் (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } இல்லையெனில் {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [இது, fl](மாறிலி எழுத்து* valp) {
        எனில் (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } இல்லையெனில் எனில் (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } இல்லையெனில் எனில் (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } இல்லையெனில் {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [இது, &optdir](மாறிலி எழுத்து* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    க்கு (முழு i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        எனில் (argv[i][0] == '-' || argv[i][0] == '+') {
            மாறிலி std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            முழு consumed = 0;
            தருக்க isForRerun = உண்மை;

            // Special case for -j which has an optional argument
            எனில் (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                முழு val = 0;
                எனில் (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    எனில் (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                எனில் (m_buildJobs == -1) m_buildJobs = val;
                எனில் (m_verilateJobs == -1) m_verilateJobs = val;
                எனில் (m_outputGroups == -1) m_outputGroups = val;
            } இல்லையெனில் {
                மாறிலி auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                எனில் (consumed) {
                    // Already processed
                } இல்லையெனில் எனில் (isFuture0(argName)) {
                    consumed = 1;
                } இல்லையெனில் எனில் (isFuture1(argName)) {
                    consumed = 2;
                } இல்லையெனில் {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            எனில் (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            தொடர்;
        }

        // Filename
        மாறிலி std::string filename = parseFileArg(optdir, argv[i]);
        தருக்க isForRerun = உண்மை;
        எனில் (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } இல்லையெனில் எனில் (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } இல்லையெனில் எனில் (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = பொய்;
        } இல்லையெனில் {
            V3Options::addVFile(filename, work());
            isForRerun = பொய்;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    எனில் (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

வெறுமை V3Options::parseOptsFile(File_வரி* fl, மாறிலி string& filename, தருக்க rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    மாறிலி std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    எனில் (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        திரும்பு;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    தருக்க has_fargs = பொய்;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    மாறிலி auto parse_fargs_cmt = [&](மாறிலி எழுத்து* p) -> வெறுமை {
        மாறிலி தருக்க blockCmt = p[1] == '*';
        p += 2;
        போது (std::isspace(*p)) ++p;
        எனில் (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            எனில் (std::isspace(*p)) {
                போது (std::isspace(*p)) ++p;
                எனில் (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    எனில் (std::isspace(*p)) {
                        போது (std::isspace(*p)) ++p;
                        எனில் (!has_fargs) {
                            has_fargs = உண்மை;
                            whole_file.clear();
                        }
                        whole_file += p;
                        எனில் (blockCmt) {
                            மாறிலி std::string::size_type pos = whole_file.find("*/");
                            எனில் (pos == std::string::npos) {
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

    தருக்க inCmt = பொய்;
    போது (!ifp->eof()) {
        மாறிலி std::string வரி = V3Os::getline(*ifp);
        // Strip simple comments
        எழுத்து lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        தருக்க space_begin = உண்மை;  // At beginning or leading spaces only
        க்கு (மாறிலி எழுத்து* p = வரி.c_str(); *p; lastch = *p++) {
            எனில் (inCmt) {
                எனில் (p[0] == '*' && p[1] == '/') {
                    inCmt = பொய்;
                    ++p;
                }
            } இல்லையெனில் எனில் (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                நிறுத்து;  // Ignore to EOL
            } இல்லையெனில் எனில் (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                நிறுத்து;  // Ignore to EOL
            } இல்லையெனில் எனில் (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = உண்மை;
                space_begin = பொய்;
                ++p;
            } இல்லையெனில் {
                எனில் (!std::isspace(*p)) space_begin = பொய்;
                எனில் (!has_fargs) whole_file += *p;
            }
        }
        எனில் (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    எனில் (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = புதிய File_வரி{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    எண்ணிக்கை state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    க்கு (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        எழுத்து curr_char = whole_file[pos];
        தேர்வு (st) {
        நிலை ST_IN_OPTION:  // Get all chars up to a white space or a "="
            எனில் (std::isspace(curr_char)) {  // End of option
                எனில் (!arg.empty()) {  // End of word
                    args.தள்ளு_back(arg);
                }
                arg = "";
                நிறுத்து;
            }
            எனில் (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                நிறுத்து;
            }
            எனில் (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                எனில் (pos < whole_file.length()) curr_char = whole_file[pos];
                எனில் (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } இல்லையெனில் {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                நிறுத்து;
            }
            எனில் (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                நிறுத்து;
            }
            arg += curr_char;
            நிறுத்து;
        நிலை ST_IN_QUOTED_STR:  // Just store all chars inside string
            எனில் (curr_char != '\'') {
                arg += curr_char;
            } இல்லையெனில் {  // End of quoted string
                st = ST_IN_OPTION;
            }
            நிறுத்து;
        நிலை ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            எனில் (curr_char != '"') {
                எனில் (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } இல்லையெனில் {
                    arg += curr_char;
                }
            } இல்லையெனில் {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            நிறுத்து;
        நிலை ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            நிறுத்து;
        }
    }
    எனில் (!arg.empty()) {  // Add last word
        args.தள்ளு_back(arg);
    }

    // Path
    மாறிலி string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<எழுத்து*> argv;
    argv.reserve(args.size() + 1);
    க்கு (மாறிலி string& i : args) argv.தள்ளு_back(const_cast<எழுத்து*>(i.c_str()));
    argv.தள்ளு_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<முழு>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(மாறிலி string& optdir, மாறிலி string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    எனில் (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    திரும்பு filename;
}

//======================================================================

வெறுமை V3Options::showVersion(தருக்க verbose) {
    cout << version();
    cout << "\n";
    எனில் (!verbose) திரும்பு;

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
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_வேர் << "\n";
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
    m_impp = புதிய V3OptionsImp;

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

வெறுமை V3Options::setDebugMode(முழு level) {
    V3Error::debugDefault(level);
    எனில் (!m_dumpLevel.மொத்தம்("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = உண்மை;
    m_debugCheck = உண்மை;
    எனில் (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(மாறிலி string& tag) மாறிலி VL_MT_SAFE {
    மாறிலி auto iter = m_debugLevel.find(tag);
    திரும்பு iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(மாறிலி string& srcfile_path) மாறிலி VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    திரும்பு debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(மாறிலி string& tag) மாறிலி VL_MT_SAFE {
    மாறிலி auto iter = m_dumpLevel.find(tag);
    திரும்பு iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(மாறிலி string& srcfile_path) மாறிலி VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    திரும்பு dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

தருக்க V3Options::dump_மரம்_Addrids() மாறிலி VL_MT_SAFE {
    நிலையான முழு s_level = -1;
    எனில் (VL_UNLIKELY(s_level < 0)) {
        மாறிலி unsigned மதிப்பு = dumpLevel("tree-addrids");
        எனில் (!available()) திரும்பு மதிப்பு > 0;
        s_level = static_cast<unsigned>(மதிப்பு);
    }
    திரும்பு s_level > 0;
}

வெறுமை V3Options::optimize(முழு level) {
    // Set all optimizations to on/off
    மாறிலி தருக்க flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_மரம் = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_மரம் = flag;
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
    எனில் (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
