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

#شامل "V3PchAstMT.h"

#شامل "V3Options.h"

#شامل "V3Container.h"
#شامل "V3Error.h"
#شامل "V3File.h"
#شامل "V3Global.h"
#شامل "V3Mutex.h"
#شامل "V3OptionParser.h"
#شامل "V3Os.h"
#شامل "V3PreShell.h"
#شامل "V3String.h"

// clang-format off
#شامل <sys/types.h>
#شامل <sys/stat.h>
#ifndef _WIN32
# include <sys/utsname.h>
#endif
#شامل <algorithm>
#شامل <cctype>
#ifdef _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#وگرنه
# include <dirent.h>
#endif
#شامل <fcntl.h>
#شامل <list>
#شامل <map>
#شامل <memory>
#شامل <thread>
#شامل <set>
#شامل <string>

#شامل "config_rev.h"

#اگر defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

رده V3OptionsImp final {
عمومی:
    // TYPES
    using DirMap = std::map<ثابت string, std::set<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_سطر_Args;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, منطقی>> m_allArgs;
    VInsertionSet<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::set<string> m_incDirFallbackSet;  // Include directories (for removing duplicates)
    std::map<ثابت string, V3LangCode> m_langExts;  // Language extension map
    VInsertionSet<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    تهی addIncDirUser(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        ثابت منطقی inserted = m_incDirUsers.درج(dir);
        اگر (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_incDirFallbackSet.erase(dir);  // User has priority over Fallback
        }
    }
    تهی addIncDirFallback(ثابت string& incdir) {
        ثابت string& dir = V3Os::filenameCleanup(incdir);
        اگر (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ثابت auto itFoundPair = m_incDirFallbackSet.درج(dir);
            اگر (itFoundPair.second) m_incDirFallbacks.افزودن_back(dir);
        }
    }
    تهی addLangExt(ثابت string& langext, ثابت V3LangCode& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        اگر (addext[0] == '.') addext = addext.substr(1);
        (تهی)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    تهی addLibExtV(ثابت string& libext) { m_libExtVs.درج(libext); }
    V3OptionsImp() = پیشفرض;
    ~V3OptionsImp() = پیشفرض;
};

//######################################################################
// V3LangCode class functions

V3LangCode::V3LangCode(ثابت نویسه* textp) {
    // Return code for given string, or ERROR, which is a bad code
    برای (صحیح codei = V3LangCode::L_ERROR; codei < V3LangCode::_ENUM_END; ++codei) {
        ثابت V3LangCode code{codei};
        اگر (0 == VL_STRCASECMP(textp, code.ascii())) {
            m_e = code;
            بازگشت;
        }
    }
    m_e = V3LangCode::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ثابت string& مقدار, منطقی& badr)
    : m_e{VTimescale::NONE} {
    badr = درست;
    ثابت string spaceless = VString::removeWhitespace(مقدار);
    برای (صحیح i = TS_100S; i < _ENUM_END; ++i) {
        ثابت VTimescale ts{i};
        اگر (spaceless == ts.ascii()) {
            badr = نادرست;
            m_e = ts.m_e;
            بشکن;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3HierarchicalBlockOption::V3HierarchicalBlockOption(ثابت string& opts) {
    VStringList vals;
    منطقی inStr = نادرست;
    string cur;
    ایستا ثابت string hierBlock("--hierarchical-block");
    File_سطر cmdfl(File_سطر::command_سطر_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    برای (string::const_iterator it = opts.begin(); it != opts.end();) {
        اگر (inStr) {
            اگر (*it == '\\') {
                ++it;
                اگر (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with \\");
                    بشکن;
                }
                اگر (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hierBlock + " does not allow '" + *it + "' after \\");
                    بشکن;
                }
                cur.افزودن_back(*it);
                ++it;
            } وگرنه اگر (*it == '"') {  // end of string
                cur.افزودن_back(*it);
                vals.افزودن_back(cur);
                cur.clear();
                ++it;
                اگر (it != opts.end()) {
                    اگر (*it != ',') {
                        cmdfl.v3error(hierBlock + " expects ',', but '" + *it + "' is passed");
                        بشکن;
                    }
                    ++it;
                    اگر (it == opts.end()) {
                        cmdfl.v3error(hierBlock + " must not end with ','");
                        بشکن;
                    }
                    inStr = *it == '"';
                    cur.افزودن_back(*it);
                    ++it;
                }
            } وگرنه {
                cur.افزودن_back(*it);
                ++it;
            }
        } وگرنه {
            اگر (*it == '"') {
                cmdfl.v3error(hierBlock + " does not allow '\"' in the middle of literal");
                بشکن;
            }
            اگر (*it == ',') {  // end of this parameter
                vals.افزودن_back(cur);
                cur.clear();
                ++it;
                اگر (it == opts.end()) {
                    cmdfl.v3error(hierBlock + " must not end with ','");
                    بشکن;
                }
                inStr = *it == '"';
            }
            cur.افزودن_back(*it);
            ++it;
        }
    }
    اگر (!cur.empty()) vals.افزودن_back(cur);
    اگر (vals.size() >= 2) {
        اگر (vals.size() % 2) {
            cmdfl.v3error(hierBlock + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } وگرنه {
        cmdfl.v3error(hierBlock + " requires at least two comma-separated values");
    }
    برای (size_t i = 2; i + 1 < vals.size(); i += 2) {
        ثابت منطقی inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        اگر (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hierBlock);
        }
    }
}

//######################################################################
// V3Options class functions

تهی VTimescale::parseSlashed(File_سطر* fl, ثابت نویسه* textp, VTimescale& unitr,
                              VTimescale& precr, منطقی allowEmpty) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ثابت نویسه* cp = textp;
    برای (; std::isspace(*cp); ++cp) {}
    ثابت نویسه* ثابت unitp = cp;
    برای (; *cp && *cp != '/'; ++cp) {}
    ثابت string unitStr(unitp, cp - unitp);
    برای (; std::isspace(*cp); ++cp) {}
    string precStr;
    اگر (*cp == '/') {
        ++cp;
        برای (; std::isspace(*cp); ++cp) {}
        ثابت نویسه* ثابت precp = cp;
        برای (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    برای (; std::isspace(*cp); ++cp) {}
    اگر (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        بازگشت;
    }

    منطقی unitbad = نادرست;
    ثابت VTimescale unit{unitStr, unitbad /*ref*/};
    اگر (unitbad && !(unitStr.empty() && allowEmpty)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        بازگشت;
    }
    unitr = unit;

    اگر (!precStr.empty()) {
        VTimescale prec{VTimescale::NONE};
        منطقی precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        اگر (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            بازگشت;
        }
        اگر (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            بازگشت;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

تهی V3Options::addIncDirUser(ثابت string& incdir) { m_impp->addIncDirUser(incdir); }
تهی V3Options::addIncDirFallback(ثابت string& incdir) { m_impp->addIncDirFallback(incdir); }
تهی V3Options::addLangExt(ثابت string& langext, ثابت V3LangCode& lc) {
    m_impp->addLangExt(langext, lc);
}
تهی V3Options::addLibExtV(ثابت string& libext) { m_impp->addLibExtV(libext); }
تهی V3Options::addDefine(ثابت string& defline, منطقی allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string چپ = defline;
    وقتی (چپ != "") {
        string def = چپ;
        string::size_type pos;
        اگر (allowPlus && ((pos = چپ.find('+')) != string::npos)) {
            چپ = چپ.substr(pos + 1);
            def.erase(pos);
        } وگرنه {
            چپ = "";
        }
        string مقدار;
        اگر ((pos = def.find('=')) != string::npos) {
            مقدار = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_سطر(def, مقدار);
    }
}
تهی V3Options::addParameter(ثابت string& paramline, منطقی allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string چپ = paramline;
    وقتی (چپ != "") {
        string param = چپ;
        string::size_type pos;
        اگر (allowPlus && ((pos = چپ.find('+')) != string::npos)) {
            چپ = چپ.substr(pos + 1);
            param.erase(pos);
        } وگرنه {
            چپ = "";
        }
        string مقدار;
        اگر ((pos = param.find('=')) != string::npos) {
            مقدار = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << مقدار);
        (تهی)m_parameters.erase(param);
        m_parameters[param] = مقدار;
    }
}

منطقی V3Options::hasParameter(ثابت string& name) {
    بازگشت m_parameters.find(name) != m_parameters.end();
}

string V3Options::parameter(ثابت string& name) {
    string مقدار = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    بازگشت مقدار;
}

تهی V3Options::checkParameters() {
    اگر (!m_parameters.empty()) {
        std::stringstream پیام;
        پیام << "Parameters from the command line were not found in the design:";
        برای (ثابت auto& i : m_parameters) پیام << " " << i.first;
        v3error(پیام.str());
    }
}

تهی V3Options::addCppFile(ثابت string& filename) { m_cppFiles.درج(filename); }
تهی V3Options::addCFlags(ثابت string& filename) { m_cFlags.افزودن_back(filename); }
تهی V3Options::addLibMapFile(ثابت string& filename) { m_libmapFiles.درج(filename); }
تهی V3Options::addCompilerIncludes(ثابت string& filename) {
    m_compilerIncludes.درج(filename);
}
تهی V3Options::addLdLibFile(ثابت string& filename) {
    m_ldLibFiles.افزودن_back(filename);
    addLdLibs(filename);
}
تهی V3Options::addLdLibs(ثابت string& filename) { m_ldLibs.افزودن_back(filename); }
تهی V3Options::addMakeFlags(ثابت string& filename) { m_makeFlags.افزودن_back(filename); }
تهی V3Options::addFuture(ثابت string& flag) { m_futures.درج(flag); }
تهی V3Options::addFuture0(ثابت string& flag) { m_future0s.درج(flag); }
تهی V3Options::addFuture1(ثابت string& flag) { m_future1s.درج(flag); }
منطقی V3Options::isFuture(ثابت string& flag) ثابت {
    بازگشت m_futures.find(flag) != m_futures.end();
}
منطقی V3Options::isFuture0(ثابت string& flag) ثابت {
    بازگشت m_future0s.find(flag) != m_future0s.end();
}
منطقی V3Options::isFuture1(ثابت string& flag) ثابت {
    بازگشت m_future1s.find(flag) != m_future1s.end();
}
منطقی V3Options::isLibraryFile(ثابت string& filename, ثابت string& libname) ثابت {
    بازگشت m_libraryFiles.find({filename, libname}) != m_libraryFiles.end();
}
تهی V3Options::addLibraryFile(ثابت string& filename, ثابت string& libname) {
    m_libraryFiles.درج({filename, libname});
}
تهی V3Options::addVFile(ثابت string& filename, ثابت string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.افزودن_back({filename, libname});  // Not emplace
}
تهی V3Options::addVltFile(ثابت string& filename, ثابت string& libname) {
    m_vltFiles.درج({filename, libname});
}
تهی V3Options::addForceInc(ثابت string& filename) { m_forceIncs.افزودن_back(filename); }

تهی V3Options::add_سطر_Arg(ثابت string& arg) { m_impp->m_سطر_Args.افزودن_back(arg); }

تهی V3Options::addArg(نویسه** argv, size_t شمار, منطقی isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, منطقی>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    برای (size_t n = 0; n < شمار; ++n) pair.first.emplace_back(argv[n]);
}

تهی V3Options::addArg(ثابت std::string& arg, منطقی isForRerun) {
    m_impp->m_allArgs.emplace_back();
    std::pair<std::list<std::string>, منطقی>& pair = m_impp->m_allArgs.back();
    pair.second = isForRerun;
    pair.first.emplace_back(arg);
}

ثابت std::list<std::pair<std::list<std::string>, منطقی>>& V3Options::allArgs() ثابت {
    بازگشت m_impp->m_allArgs;
}

string V3Options::allArgsString() ثابت VL_MT_SAFE {
    string result;
    برای (ثابت auto& pair : m_impp->m_allArgs) {
        برای (ثابت string& arg : pair.first) {
            اگر (!result.empty()) result += " ";
            result += arg;
        }
    }
    بازگشت result;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::allArgsStringForHierBlock(منطقی for_بالاترین) ثابت {
    // cppcheck-suppress shadowFunction
    std::set<string> vFiles;
    برای (ثابت auto& vFile : m_vFiles) vFiles.درج(vFile.filename());
    string out;
    منطقی stripArg = نادرست;
    منطقی strip_Arg_If_رقم = نادرست;
    برای (ثابت auto& pair : m_impp->m_allArgs) {
        برای (ثابت string& arg : pair.first) {
            اگر (stripArg) {
                stripArg = نادرست;
                ادامه;
            }
            اگر (strip_Arg_If_رقم) {
                strip_Arg_If_رقم = نادرست;
                اگر (isdigit(arg[0])) ادامه;
            }
            صحیح skip = 0;
            اگر (arg.length() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } وگرنه اگر (arg.length() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            اگر (skip > 0) {  // arg is an option
                ثابت string opt = arg.substr(skip);  // Remove '-' in the beginning
                ثابت صحیح رقم_Strip = stripOptionsForChildRun(opt, for_بالاترین);
                اگر (رقم_Strip) {
                    UASSERT(0 <= رقم_Strip && رقم_Strip <= 3, "should be one of 0, 1, 2, 3");
                    اگر (رقم_Strip == 2) stripArg = درست;
                    اگر (رقم_Strip == 3) strip_Arg_If_رقم = درست;
                    ادامه;
                }
            } وگرنه {  // Not an option
                اگر (vFiles.find(arg) != vFiles.end()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.end()) {  // Remove C++
                    ادامه;
                }
            }
            اگر (out != "") out += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            out += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    بازگشت out;
}

تهی V3Options::ccSet() {  // --cc
    m_outFormatOk = درست;
    m_systemC = نادرست;
}

تهی V3Options::decorations(File_سطر* fl, ثابت string& arg) {  // --decorations
    اگر (arg == "none") {
        m_decoration = نادرست;
        m_decorationNodes = نادرست;
    } وگرنه اگر (arg == "node") {
        m_decoration = درست;
        m_decorationNodes = درست;
    } وگرنه اگر (arg == "medium") {
        m_decoration = درست;
        m_decorationNodes = نادرست;
    } وگرنه {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warnMore() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::vector<std::string> V3Options::traceClassBases() ثابت VL_MT_SAFE {
    std::vector<std::string> result;
    اگر (traceEnabledFst()) result.emplace_back("VerilatedFst");
    اگر (traceEnabledSaif()) result.emplace_back("VerilatedSaif");
    اگر (traceEnabledVcd()) result.emplace_back("VerilatedVcd");
    بازگشت result;
}
std::vector<std::string> V3Options::traceClassLangs() ثابت VL_MT_SAFE {
    std::vector<std::string> result;
    برای (ثابت auto& cbase : traceClassBases())
        result.emplace_back(cbase + (systemC() ? "Sc" : "C"));
    بازگشت result;
}
std::vector<std::string> V3Options::traceSourceBases() ثابت VL_MT_SAFE {
    std::vector<std::string> result;
    اگر (traceEnabledFst()) result.emplace_back("verilated_fst");
    اگر (traceEnabledSaif()) result.emplace_back("verilated_saif");
    اگر (traceEnabledVcd()) result.emplace_back("verilated_vcd");
    بازگشت result;
}
std::vector<std::string> V3Options::traceSourceLangs() ثابت VL_MT_SAFE {
    std::vector<std::string> result = traceSourceBases();
    برای (std::string& str : result) str += systemC() ? "_sc"s : "_c"s;
    بازگشت result;
}
std::string V3Options::traceClassBase() ثابت VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassBase only when trace() enabled");
    بازگشت traceClassBases().front();
}
std::string V3Options::traceClassLang() ثابت VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().empty(), "Call traceClassLang only when trace() enabled");
    بازگشت traceClassLangs().front();
}

//######################################################################
// File searching

منطقی V3Options::fileStatNormal(ثابت string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ساختار stat sstat;  // Stat information
    ثابت صحیح err = stat(filename.c_str(), &sstat);
    اگر (err != 0) بازگشت نادرست;
    اگر (S_ISDIR(sstat.st_mode)) بازگشت نادرست;
    بازگشت درست;
}

string V3Options::fileExists(ثابت string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ثابت string dir = V3Os::filenameDir(filename);
    ثابت string basename = V3Os::filenameNonDir(filename);

    auto diriter = m_impp->m_dirMap.find(dir);
    اگر (diriter == m_impp->m_dirMap.end()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::set<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::set<string>* ثابت setp = &(diriter->second);

#ifdef _MSC_VER
        تلاش {
            برای (ثابت auto& dirEntry : std::filesystem::directory_iterator(dir.c_str()))
                setp->درج(dirEntry.path().filename().string());
        } بگیر (std::filesystem::filesystem_error ثابت& ex) {
            (تهی)ex;
            بازگشت "";
        }
#وگرنه
        اگر (DIR* ثابت dirp = opendir(dir.c_str())) {
            وقتی (ساختار dirent* direntp = readdir(dirp)) setp->درج(direntp->d_name);
            closedir(dirp);
        }
#endif
    }
    // Find it
    ثابت std::set<string>* ثابت filesetp = &(diriter->second);
    ثابت auto fileiter = filesetp->find(basename);
    اگر (fileiter == filesetp->end()) بازگشت "";  // Not found
    // Check if it is a directory, ignore if so
    ثابت string filenameOut = V3Os::filenameJoin(dir, basename);
    اگر (!fileStatNormal(filenameOut)) بازگشت "";  // Directory
    بازگشت filenameOut;
}

string V3Options::filePathCheckOneDir(ثابت string& modname, ثابت string& dirname) {
    برای (ثابت string& i : m_impp->m_libExtVs) {
        ثابت string fn = V3Os::filenameJoin(dirname, modname + i);
        ثابت string exists = fileExists(fn);
        اگر (exists != "") بازگشت exists;
    }
    بازگشت "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
صحیح V3Options::stripOptionsForChildRun(ثابت string& opt, منطقی for_بالاترین) {
    // Options to strip for both the top wrapper, and hier blocks
    ایستا ثابت std::unordered_map<std::string, صحیح> commonOpts{
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
    اگر (commonOpts.شمار(opt)) بازگشت commonOpts.at(opt);

    // Options to strip only for hier blocks
    اگر (!for_بالاترین) {
        ایستا ثابت std::unordered_map<std::string, صحیح> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        اگر (subOpts.شمار(opt)) بازگشت subOpts.at(opt);
        اگر (VString::startsWith(opt, "G")) بازگشت 1;
        اگر (VString::startsWith(opt, "pvalue")) بازگشت 1;
    }

    // Do not strip
    بازگشت 0;
}

تهی V3Options::validateIdentifier(File_سطر* fl, ثابت string& arg, ثابت string& opt) {
    اگر (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::filePath(File_سطر* fl, ثابت string& modname, ثابت string& lastpath,
                           ثابت string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ثابت string filename = V3Os::filenameCleanup(VName::dehash(modname));
    اگر (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ثابت string exists = filePathCheckOneDir(filename, "");
        اگر (exists != "") بازگشت exists;
    }
    برای (ثابت string& dir : m_impp->m_incDirUsers) {
        ثابت string exists = filePathCheckOneDir(filename, dir);
        اگر (exists != "") بازگشت exists;
    }
    برای (ثابت string& dir : m_impp->m_incDirFallbacks) {
        ثابت string exists = filePathCheckOneDir(filename, dir);
        اگر (exists != "") بازگشت exists;
    }

    اگر (m_relativeIncludes) {
        ثابت string exists = filePathCheckOneDir(filename, lastpath);
        اگر (exists != "") بازگشت V3Os::filenameRealPath(exists);
    }

    // Warn and return not found
    اگر (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_Path_Looked_پیام(fl, filename));
    }
    بازگشت "";
}

string V3Options::file_Path_Looked_پیام(File_سطر* /*fl*/, ثابت string& modname) {
    ایستا منطقی s_shown_notfound_پیام = نادرست;
    std::ostringstream ss;
    اگر (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warnMore() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warnMore() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } وگرنه اگر (!s_shown_notfound_پیام) {
        s_shown_notfound_پیام = درست;
        اگر (m_impp->m_incDirUsers.empty()) {
            ss << V3Error::warnMore()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warnMore() << "... Looked in:\n";
        برای (ثابت string& dir : m_impp->m_incDirUsers) {
            برای (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        برای (ثابت string& dir : m_impp->m_incDirFallbacks) {
            برای (ثابت string& ext : m_impp->m_libExtVs) {
                ثابت string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warnMore() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warnMore() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    بازگشت ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3LangCode V3Options::fileLanguage(ثابت string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::size_type pos;
    اگر (filename == V3Options::getStdPackagePath() || filename == V3Options::getStdWaiverPath()) {
        بازگشت V3LangCode::mostRecent();
    } وگرنه اگر ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ثابت auto it = m_impp->m_langExts.find(ext);
        اگر (it != m_impp->m_langExts.end()) بازگشت it->second;
    }
    بازگشت m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ثابت string& var) {
    // If update below, also update V3Options::showVersion()
    اگر (var == "MAKE") {
        بازگشت getenvMAKE();
    } وگرنه اگر (var == "PERL") {
        بازگشت getenvPERL();
    } وگرنه اگر (var == "PYTHON3") {
        بازگشت getenvPYTHON3();
    } وگرنه اگر (var == "SYSTEMC") {
        بازگشت getenvSYSTEMC();
    } وگرنه اگر (var == "SYSTEMC_ARCH") {
        بازگشت getenvSYSTEMC_ARCH();
    } وگرنه اگر (var == "SYSTEMC_INCLUDE") {
        بازگشت getenvSYSTEMC_INCLUDE();
    } وگرنه اگر (var == "SYSTEMC_LIBDIR") {
        بازگشت getenvSYSTEMC_LIBDIR();
    } وگرنه اگر (var == "VERILATOR_ROOT") {
        بازگشت getenv_VERILATOR_ریشه();
    } وگرنه {
        بازگشت V3Os::getenvStr(var, "");
    }
}

#ifdef __FreeBSD__
string V3Options::getenvMAKE() { بازگشت V3Os::getenvStr("MAKE", "gmake"); }
#وگرنه
string V3Options::getenvMAKE() { بازگشت V3Os::getenvStr("MAKE", "make"); }
#endif

string V3Options::getenvMAKEFLAGS() {  //
    بازگشت V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    بازگشت V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    بازگشت V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    بازگشت V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_ARCH};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    اگر (var == "") {
#اگر defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ثابت string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#وگرنهاگر defined(_WIN32)
        ثابت string sysname = "WIN32";
        var = "win32";
#وگرنه
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ساختار utsname uts;
        uname(&uts);
        ثابت string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        اگر (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } وگرنه اگر (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } وگرنه {
            var = "linux";
        }
#endif
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    بازگشت var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    اگر (var == "") {
        ثابت string sc = getenvSYSTEMC();
        اگر (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    بازگشت V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    اگر (var == "") {
        ثابت string sc = getenvSYSTEMC();
        ثابت string arch = getenvSYSTEMC_ARCH();
        اگر (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    بازگشت V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_ریشه() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_VERILATOR_ریشه};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    اگر (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    بازگشت V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ثابت string defenv = string{DEFENV_VERILATOR_SOLVER};
    اگر (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    بازگشت var;
}

string V3Options::getStdPackagePath() {
    بازگشت V3Os::filenameJoin(getenv_VERILATOR_ریشه(), "include", "verilated_std.sv");
}
string V3Options::getStdWaiverPath() {
    بازگشت V3Os::filenameJoin(getenv_VERILATOR_ریشه(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ثابت string& var) {
    // If update below, also update V3Options::showVersion()
    اگر (var == "COROUTINES" && coroutineSupport()) {
        بازگشت "1";
    } وگرنه اگر (var == "TSAN" && tsanSupport()) {
        بازگشت "1";
    } وگرنه اگر (var == "DEV_ASAN" && devAsan()) {
        بازگشت "1";
    } وگرنه اگر (var == "DEV_GCOV" && devGcov()) {
        بازگشت "1";
        // cppcheck-suppress knownConditionTrueFalse
    } وگرنه اگر (var == "SYSTEMC" && systemCFound()) {
        بازگشت "1";
    } وگرنه {
        بازگشت "";
    }
}

منطقی V3Options::systemCSystemWide() {
#ifdef HAVE_SYSTEMC
    بازگشت درست;
#وگرنه
    بازگشت نادرست;
#endif
}

منطقی V3Options::systemCFound() {
    بازگشت (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().empty() && !getenvSYSTEMC_LIBDIR().empty()));
}

منطقی V3Options::coroutineSupport() {
#ifdef HAVE_COROUTINES
    بازگشت درست;
#وگرنه
    بازگشت نادرست;
#endif
}

منطقی V3Options::devAsan() {
#ifdef HAVE_DEV_ASAN
    بازگشت درست;
#وگرنه
    بازگشت نادرست;
#endif
}

منطقی V3Options::devGcov() {
#ifdef HAVE_DEV_GCOV
    بازگشت درست;
#وگرنه
    بازگشت نادرست;
#endif
}

منطقی V3Options::tsanSupport() {
#ifdef HAVE_TSAN
    بازگشت درست;
#وگرنه
    بازگشت نادرست;
#endif
}

//######################################################################
// V3 Options notification methods

تهی V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_سطر* ثابت cmdfl = جدید File_سطر{File_سطر::command_سطر_Filename()};

    اگر (!outFormatOk() && v3Global.opt.اصلی()) ccSet();  // --main implies --cc if not provided
    اگر (!outFormatOk() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    اگر (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::vector<std::string> backendFlags;
    اگر (m_build) {
        اگر (m_دودویی)
            backendFlags.emplace_back("--binary");
        وگرنه
            backendFlags.emplace_back("--build");
    }
    اگر (m_preprocOnly) backendFlags.emplace_back("-E");
    اگر (m_dpiHdrOnly) backendFlags.emplace_back("--dpi-hdr-only");
    اگر (m_lintOnly) backendFlags.emplace_back("--lint-only");
    اگر (m_jsonOnly) backendFlags.emplace_back("--json-only");
    اگر (backendFlags.size() > 1) {
        std::string backendFlagsString = backendFlags.front();
        برای (size_t i = 1; i < backendFlags.size(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    اگر (m_exe && !v3Global.opt.libCreate().empty()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    اگر (!m_gmake && !m_makeJson) m_gmake = درست;

    اگر (m_hierarchical && (m_hierChild || !m_hierBlocks.empty())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    اگر (m_hierChild) {
        اگر (m_hierBlocks.empty()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = نادرست;
    }

    اگر (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = درست;  // No format, with --trace means wanted --trace-vcd
    }
    اگر (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = درست;
    ثابت صحیح ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    اگر (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    اگر (protectIds()) {
        اگر (allPublic()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warnMore() + "... Suggest remove --public.");
        }
        اگر (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --trace.");
        }
        اگر (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warnMore() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    اگر (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    اگر (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.setTrueOrFalse(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    اگر (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.setTrueOrFalse(درست);

    // Default split limits if not specified
    اگر (m_outputSplitCFuncs < 0) m_outputSplitCFuncs = m_outputSplit;
    اگر (m_outputSplitCTrace < 0) m_outputSplitCTrace = m_outputSplit;

    اگر (v3Global.opt.اصلی() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    اگر (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        اگر (hierarchical()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    اگر (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    اگر (v3Global.opt.timing().isSetTrue() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    اگر (!m_dumpLevel.شمار("tree") && m_dumpLevel.شمار("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(threads() >= 1, "'threads()' must return a value >= 1");
    اگر (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    اگر (m_buildJobs == -1) m_buildJobs = 1;
    اگر (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    اگر (timing().isSetTrue()) V3PreShell::define_Cmd_سطر("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    اگر (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.setTrueOrFalse(نادرست);

    // === Leave last
    // Mark options as available
    m_available = درست;
}

//######################################################################
// V3 Options accessors

string V3Options::version() VL_PURE {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvtToStr(DTVERSION_rev);
    بازگشت ver;
}

string V3Options::protect_کلید_Defaulted() VL_MT_SAFE {
    ایستا V3Mutex s_mutex;
    ثابت V3LockGuard lock{s_mutex};
    اگر (m_protect_کلید.empty()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_کلید = "VL-KEY-" + digest.digestBase64();
    }
    بازگشت m_protect_کلید;
}

تهی V3Options::throwSigsegv() {  // LCOV_EXCL_START
#اگر !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<فرار نویسه*>(nullptr) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#endif
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(ثابت VTimescale& flag) ثابت {
    اگر (!timeOverridePrec().isNone()) {
        بازگشت timeOverridePrec();
    } وگرنه اگر (flag.isNone()) {
        بازگشت timeDefaultPrec();
    } وگرنه {
        بازگشت flag;
    }
}

VTimescale V3Options::timeComputeUnit(ثابت VTimescale& flag) ثابت {
    اگر (!timeOverrideUnit().isNone()) {
        بازگشت timeOverrideUnit();
    } وگرنه اگر (flag.isNone()) {
        بازگشت timeDefaultUnit();
    } وگرنه {
        بازگشت flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(صحیح argc, نویسه** argv) {
    // Return list of arguments as simple string
    string opts;
    برای (صحیح i = 0; i < argc; ++i) {
        اگر (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    بازگشت opts;
}

//######################################################################
// V3 Options Parsing

تهی V3Options::parseOpts(File_سطر* fl, صحیح argc, نویسه** argv) VL_MT_DISABLED {
    // Save command line options
    برای (صحیح i = 0; i < argc; ++i) add_سطر_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    اگر (vFiles().empty()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    اگر (prefix() == "" && بالاترین_Module() != "") m_prefix = "V"s + بالاترین_Module();  // Already encoded
    اگر (prefix() == "" && vFiles().size() >= 1)
        m_prefix
            = "V"s + Ast_گره::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    اگر (modPrefix() == "") m_modPrefix = prefix();

    // Find files in makedir
    addIncDirFallback(makeDir());
}

//======================================================================

منطقی V3Options::suffixed(ثابت string& sw, ثابت نویسه* arg) {
    اگر (std::strlen(arg) > sw.length()) بازگشت نادرست;
    بازگشت (0 == std::strcmp(sw.c_str() + sw.length() - std::strlen(arg), arg));
}

تهی V3Options::parseOptsList(File_سطر* fl, ثابت string& optdir, صحیح argc,
                              نویسه** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ثابت V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    ثابت auto callStrSetter = [این](تهی (V3Options::*cbStr)(ثابت string&)) {
        بازگشت [این, cbStr](ثابت string& v) { (این->*cbStr)(v); };
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
    DECL_OPTION("+define+", CbPartialMatch, [این](ثابت نویسه* optp) VL_MT_DISABLED {
        addDefine(optp, درست);
    }).notForRerun();
    DECL_OPTION("+incdir+", CbPartialMatch, [این, &optdir](ثابت نویسه* optp) {
        string dirs = optp;
        string::size_type pos;
        وقتی ((pos = dirs.find('+')) != string::npos) {
            addIncDirUser(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        addIncDirUser(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        string exts = optp;
        string::size_type pos;
        وقتی ((pos = exts.find('+')) != string::npos) {
            addLibExtV(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        addLibExtV(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", CbPartialMatch, [این](ثابت نویسه* optp) {
        addLangExt(optp, V3LangCode::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](منطقی) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [این](منطقی flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ثابت نویسه*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [این](منطقی flag) {
        m_bboxUnsup = flag;
        File_سطر::globalWarnOff(V3ErrorCode::E_UNSUPPORTED, درست);
    });
    DECL_OPTION("-binary", CbCall, [این]() {
        m_دودویی = درست;
        m_build = درست;
        m_exe = درست;
        m_main = درست;
        اگر (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", Set, &m_build);
    DECL_OPTION("-build-dep-bin", Set, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [این, fl](ثابت نویسه* valp) {
        صحیح val = std::atoi(valp);
        اگر (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } وگرنه اگر (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::addCFlags));
    DECL_OPTION("-cc", CbCall, [این]() { ccSet(); });
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
    DECL_OPTION("-comp-limit-syms", CbVal, [](صحیح val) { VName::maxLength(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [این, fl](ثابت نویسه* valp) {
        اگر (!std::strcmp(valp, "clang")) {
            m_compLimitBlocks = 80;  // limit unknown
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_compLimitParens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } وگرنه اگر (!std::strcmp(valp, "gcc")) {
            m_compLimitBlocks = 0;  // Bug free
            m_compLimitMembers = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_compLimitParens = 240;  // Unlimited, but generate same code as for clang
        } وگرنه اگر (!std::strcmp(valp, "msvc")) {
            m_compLimitBlocks = 80;  // 128, but allow some room
            m_compLimitMembers = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_compLimitParens = 80;  // 128, but allow some room
        } وگرنه {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::addCompilerIncludes));
    DECL_OPTION("-converge-limit", Set, &m_convergeLimit);
    DECL_OPTION("-coverage", CbOnOff, [این](منطقی flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_عبارت);
    DECL_OPTION("-coverage-expr-max", Set, &m_coverage_عبارت_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [این, fl](ثابت نویسه* ثابت valp) {
        اگر (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } وگرنه اگر (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } وگرنه اگر (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::FULL;
        } وگرنه {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", Set, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_سطر);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ثابت auto parseBinsLimit = [fl](ثابت نویسه* optp, ثابت نویسه* valp, uint32_t& limitr) {
        نویسه* endp = nullptr;
        ثابت unsigned long long مقدار = std::strtoull(valp, &endp, 10);
        اگر (*endp || مقدار < 1 || مقدار > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            بازگشت;
        }
        limitr = static_cast<uint32_t>(مقدار);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [این, parseBinsLimit](ثابت نویسه* valp) {
        parseBinsLimit("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [این, parseBinsLimit](ثابت نویسه* valp) {
        parseBinsLimit("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", Set, &m_coverageMaxWidth);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coveragePerInstance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", CbPartialMatch, [این](ثابت نویسه* valp) VL_MT_DISABLED {
        addDefine(valp, نادرست);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [این]() { setDebugMode(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [این](صحیح v) { setDebugMode(v); }).notForRerun();
    DECL_OPTION("-debugi-", CbPartialMatchVal, [این](ثابت نویسه* optp, ثابت نویسه* valp) {
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
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_پشته_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debugWidth).undocumented();
    DECL_OPTION("-decoration", CbCall, [این, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [این, fl](ثابت نویسه* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [این, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [این](ثابت نویسه* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = درست;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", CbPartialMatch, [این](ثابت نویسه* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", CbPartialMatch, [این](ثابت نویسه* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", CbPartialMatchVal, [این](ثابت نویسه* optp, ثابت نویسه* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [این](منطقی flag) {
        اگر (flag) {
            m_stdPackage = نادرست;
            m_stdWaiver = نادرست;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<تهی (*)(صحیح)>(&V3Error::errorLimit));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [این](ثابت نویسه* valp) { m_expandLimit = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [این, fl, &optdir](ثابت نویسه* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), درست);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [این, &optdir](ثابت نویسه* valp) { addForceInc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [این, fl, &optdir](ثابت نویسه* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), نادرست);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [این](ثابت نویسه* valp) { addFuture0(valp); });
    DECL_OPTION("-future1", CbVal, [این](ثابت نویسه* valp) { addFuture1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_fBitScanLoops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [این](منطقی flag) {
        m_fCaseDecoder = flag;
        m_fCaseTable = flag;
        m_f_Case_درخت = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_fCaseTable);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_درخت);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_درخت);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [این](منطقی flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [این](منطقی flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", CbPartialMatch, [این](ثابت نویسه* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", CbPartialMatch, [این](ثابت نویسه* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_افزودن_Down_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [این, fl](منطقی flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [این](منطقی flag) {  //
        m_fFuncSplitCat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_fFuncSplitCat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [این](منطقی flag) {  //
        m_fIcoChangeDetect.setTrueOrFalse(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_fSlice);
    DECL_OPTION("-fslice-element-limit", CbVal, [این, fl](ثابت نویسه* valp) {
        m_fSliceElementLimit = std::atoi(valp);
        اگر (m_fSliceElementLimit < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_fSplit);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_fTable);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](منطقی) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", CbPartialMatch, [این](ثابت نویسه* optp) { addParameter(optp, نادرست); });
    DECL_OPTION("-gate-stmts", Set, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [این]() {
        cout << protect_کلید_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ثابت نویسه* valp) {
        cout << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ثابت نویسه* valp) {
        cout << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_hierarchical);
    DECL_OPTION("-hierarchical-block", CbVal, [این](ثابت نویسه* valp) {
        ثابت V3HierarchicalBlockOption opt{valp};
        m_hierBlocks.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", Set, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [این](ثابت نویسه* optp) { m_hierParamsFile.افزودن_back({optp, work()}); });

    DECL_OPTION("-I", CbPartialMatch, [این, &optdir](ثابت نویسه* optp) {
        addIncDirUser(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", Set, &m_ifDepth);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", Set, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", Set, &m_inlineCFuncsProduct);
    DECL_OPTION("-inline-mult", Set, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [این, fl](صحیح val) {
        m_instr_شمار_Dpi = val;
        اگر (m_instr_شمار_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [این](ثابت نویسه* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = درست;
    });
    DECL_OPTION("-json-only-output", CbVal, [این](ثابت نویسه* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = درست;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::addLdLibs));
    DECL_OPTION("-l2-name", Set, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [این]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [این]() { m_l2Name = "v"; }).undocumented();  // Historical
    ثابت auto setLang = [این, fl](ثابت نویسه* valp) {
        ثابت V3LangCode optval{valp};
        اگر (optval.legal()) {
            m_defaultLanguage = optval;
        } وگرنه {
            VSpellCheck spell;
            برای (صحیح i = V3LangCode::L_ERROR + 1; i < V3LangCode::_ENUM_END; ++i) {
                spell.افزودن_Candidate(V3LangCode{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_پیام(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-language", CbVal, setLang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [این](ثابت نویسه* valp) { addLibMapFile(valp); });
    DECL_OPTION("-lib-create", CbVal, [این, fl](ثابت نویسه* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", Set, &m_localizeMaxSize);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::addMakeFlags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [این](ثابت نویسه* valp) {
        m_makeDir = valp;
        addIncDirFallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", Set, &m_main_بالاترین_Name);
    DECL_OPTION("-make", CbVal, [این, fl](ثابت نویسه* valp) {
        اگر (!std::strcmp(valp, "gmake")) {
            m_gmake = درست;
        } وگرنه اگر (!std::strcmp(valp, "json")) {
            m_makeJson = درست;
        } وگرنه {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", Set, &m_funcRecursion);
    DECL_OPTION("-max-num-width", Set, &m_max_رقم_Width);
    DECL_OPTION("-mod-prefix", CbVal, [این, fl](ثابت نویسه* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_modPrefix = valp;
    });

    DECL_OPTION("-O0", CbCall, [این]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [این]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [این]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [این]() { optimize(3); });

    DECL_OPTION("-o", Set, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](منطقی /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [این, fl](ثابت نویسه* valp) {
        m_outputGroups = std::atoi(valp);
        اگر (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [این, fl](ثابت نویسه* valp) {
        m_outputGroupsSize = std::atoll(valp);
        اگر (m_outputGroupsSize < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", Set, &m_outputSplit);
    DECL_OPTION("-output-split-cfuncs", CbVal, [این, fl](ثابت نویسه* valp) {
        m_outputSplitCFuncs = std::atoi(valp);
        اگر (m_outputSplitCFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [این, fl](ثابت نویسه* valp) {
        m_outputSplitCTrace = std::atoi(valp);
        اگر (m_outputSplitCTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", Set, &m_preproc_No_سطر);
    DECL_OPTION("-pins64", CbCall, [این]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [این]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [این, fl](ثابت نویسه* valp) {
        m_pinsBv = std::atoi(valp);
        اگر (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [این](منطقی flag) {
        m_pinsScUint = flag;
        اگر (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [این](منطقی flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [این](منطقی flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", Set, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [این, fl](ثابت نویسه* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_prefix = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preprocDefines);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [این, fl](ثابت نویسه* valp) {
        m_preprocTokenLimit = std::atoi(valp);
        اگر (m_preprocTokenLimit <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [این]() { m_public = نادرست; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [این]() { m_profC = m_profCFuncs = درست; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [این]() {
        m_profC = m_profCFuncs = درست;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", Set, &m_protect_کلید);
    DECL_OPTION("-protect-lib", CbVal, [این, fl](ثابت نویسه* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = درست;
    });
    DECL_OPTION("-public", OnOff, &m_public);
    DECL_OPTION("-public-depth", Set, &m_publicDepth);
    DECL_OPTION("-public-flat-rw", CbOnOff, [این](منطقی flag) {
        m_publicFlatRW = flag;
        v3Global.dpi(درست);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [این](منطقی flag) { m_publicIgnore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [این](منطقی flag) {
        m_publicParams = flag;
        v3Global.dpi(درست);
    });
    DECL_OPTION("-pvalue+", CbPartialMatch,
                [این](ثابت نویسه* varp) { addParameter(varp, نادرست); });

    DECL_OPTION("-quiet", CbOnOff, [این](منطقی flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [این, fl](ثابت نویسه* valp) {
        m_reloopLimit = std::atoi(valp);
        اگر (m_reloopLimit < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [این, fl](ثابت نویسه* valp) {
        m_replicationLimit = std::atoi(valp);
        اگر (m_replicationLimit < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [این, fl]() {
        decorations(fl, "node");
        addCFlags("-ggdb");
        addLdLibs("-ggdb");
        addCFlags("-fsanitize=address,undefined");
        addLdLibs("-fsanitize=address,undefined");
        addCFlags("-D_GLIBCXX_DEBUG");
        addCFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [این]() {
        m_outFormatOk = درست;
        m_systemC = درست;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [این](منطقی flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [این](منطقی flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [این]() { m_defaultLanguage = V3LangCode::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [این, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_threads = 1;
    });
    DECL_OPTION("-threads", CbVal, [این, fl](ثابت نویسه* valp) {
        m_threads = std::atoi(valp);
        اگر (m_threads < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        اگر (m_threads == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_threads = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [این, fl](ثابت نویسه* valp) {
        m_hierThreads = std::atoi(valp);
        اگر (m_hierThreads < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_threadsCoarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [این, fl](ثابت نویسه* valp) {
        اگر (!std::strcmp(valp, "all")) {
            m_threadsDpiPure = درست;
            m_threadsDpiUnpure = درست;
        } وگرنه اگر (!std::strcmp(valp, "none")) {
            m_threadsDpiPure = نادرست;
            m_threadsDpiUnpure = نادرست;
        } وگرنه اگر (!std::strcmp(valp, "pure")) {
            m_threadsDpiPure = درست;
            m_threadsDpiUnpure = نادرست;
        } وگرنه {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [این, fl](ثابت نویسه* valp) {
        m_threadsMaxMTasks = std::atoi(valp);
        اگر (m_threadsMaxMTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [این, fl](ثابت نویسه* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        اگر (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        اگر (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [این, fl](ثابت نویسه* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, درست);
        اگر (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        اگر (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [این](ثابت std::string& flag) { m_بالاترین_Module = Ast_گره::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [این](ثابت std::string& flag) { m_بالاترین_Module = Ast_گره::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", Set, &m_بالاترین_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [این]() { m_traceEnabledSaif = درست; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", Set, &m_traceDepth);
    DECL_OPTION("-trace-fst", CbCall, [این]() {
        m_traceEnabledFst = درست;
        addLdLibs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", Set, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", Set, &m_traceMaxWidth);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ثابت نویسه*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", Set, &m_no_Trace_بالاترین);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [این]() { m_traceEnabledVcd = درست; });

    DECL_OPTION("-U", CbPartialMatch, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", Set, &m_constraintArrayLimit);
    DECL_OPTION("-unroll-count", Set, &m_unroll_شمار).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", Set, &m_unrollLimit);
    DECL_OPTION("-unroll-stmts", Set, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", Set, &m_unusedRegexp);

    DECL_OPTION("-V", CbCall, [این]() {
        showVersion(درست);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [این, &optdir](ثابت نویسه* valp) {
        V3Options::addLibraryFile(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [این, fl](ثابت نویسه* valp) {
        صحیح val = std::atoi(valp);
        اگر (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } وگرنه اگر (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [این]() {
        showVersion(نادرست);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_سطر::globalWarnOff(V3ErrorCode::I_LINT, نادرست); });
    DECL_OPTION("-Werror-", CbPartialMatch, [این, fl](ثابت نویسه* optp) {
        ثابت V3ErrorCode code{optp};
        اگر (code == V3ErrorCode::EC_ERROR) {
            اگر (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } وگرنه {
            V3Error::pretendError(code, درست);
        }
    });
    DECL_OPTION("-Wfuture-", CbPartialMatch, [این](ثابت نویسه* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        addFuture(optp);
    });
    DECL_OPTION("-Wno-", CbPartialMatch, [fl, &parser](ثابت نویسه* optp) VL_MT_DISABLED {
        ثابت string err = File_سطر::globalWarnOffParse(optp, درست);
        اگر (!err.empty()) {
            ثابت string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    برای (صحیح i = V3ErrorCode::EC_FIRST_WARN; i < V3ErrorCode::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        برای (ثابت string prefix : {"-Wno-", "-Wwarn-"})
            parser.addSuggestionCandidate(prefix + V3ErrorCode{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [این]() { m_context = نادرست; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(نادرست); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_سطر::globalWarnOff(V3ErrorCode::I_LINT, درست); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_سطر::globalWarnOff(V3ErrorCode::I_STYLE, درست); });
    DECL_OPTION("-work", Set, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [این]() {
        m_pedantic = درست;
        V3Error::pretendError(V3ErrorCode::ASSIGNIN, نادرست);
    });
    DECL_OPTION("-Wwarn-", CbPartialMatch, [این, fl, &parser](ثابت نویسه* optp) VL_MT_DISABLED {
        ثابت V3ErrorCode code{optp};
        اگر (code == V3ErrorCode::EC_ERROR) {
            اگر (!isFuture(optp)) {
                ثابت string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } وگرنه {
            File_سطر::globalWarnOff(code, نادرست);
            V3Error::pretendError(code, نادرست);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_سطر::globalWarnOff(V3ErrorCode::I_LINT, نادرست); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_سطر::globalWarnOff(V3ErrorCode::I_STYLE, نادرست); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", Set, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [این, fl](ثابت نویسه* valp) {
        اگر (!std::strcmp(valp, "0")) {
            m_xAssign = "0";
        } وگرنه اگر (!std::strcmp(valp, "1")) {
            m_xAssign = "1";
        } وگرنه اگر (!std::strcmp(valp, "fast")) {
            m_xAssign = "fast";
        } وگرنه اگر (!std::strcmp(valp, "unique")) {
            m_xAssign = "unique";
        } وگرنه {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [این, fl](ثابت نویسه* valp) {
        اگر (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } وگرنه اگر (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } وگرنه اگر (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } وگرنه {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warnMore() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [این, &optdir](ثابت نویسه* valp) {
        addIncDirUser(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    برای (صحیح i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        اگر (argv[i][0] == '-' || argv[i][0] == '+') {
            ثابت std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            صحیح consumed = 0;
            منطقی isForRerun = درست;

            // Special case for -j which has an optional argument
            اگر (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                صحیح val = 0;
                اگر (i + 1 < argc && std::isdigit(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    اگر (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                اگر (m_buildJobs == -1) m_buildJobs = val;
                اگر (m_verilateJobs == -1) m_verilateJobs = val;
                اگر (m_outputGroups == -1) m_outputGroups = val;
            } وگرنه {
                ثابت auto pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                اگر (consumed) {
                    // Already processed
                } وگرنه اگر (isFuture0(argName)) {
                    consumed = 1;
                } وگرنه اگر (isFuture1(argName)) {
                    consumed = 2;
                } وگرنه {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            اگر (argName != "f" && argName != "F") addArg(argv + i, consumed, isForRerun);
            i += consumed;
            ادامه;
        }

        // Filename
        ثابت std::string filename = parseFileArg(optdir, argv[i]);
        منطقی isForRerun = درست;
        اگر (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::addCppFile(filename);
        } وگرنه اگر (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::addLdLibFile(filename);
        } وگرنه اگر (suffixed(filename, ".vlt")) {
            V3Options::addVltFile(filename, work());
            isForRerun = نادرست;
        } وگرنه {
            V3Options::addVFile(filename, work());
            isForRerun = نادرست;
        }
        addArg(filename, isForRerun);
        ++i;
    }

    اگر (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

تهی V3Options::parseOptsFile(File_سطر* fl, ثابت string& filename, منطقی rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    اگر (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        بازگشت;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    منطقی has_fargs = نادرست;
    std::string whole_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ثابت auto parse_fargs_cmt = [&](ثابت نویسه* p) -> تهی {
        ثابت منطقی blockCmt = p[1] == '*';
        p += 2;
        وقتی (std::isspace(*p)) ++p;
        اگر (VString::startsWith(p, "verilator")) {
            p += std::strlen("verilator");
            اگر (std::isspace(*p)) {
                وقتی (std::isspace(*p)) ++p;
                اگر (VString::startsWith(p, "fargs")) {
                    p += std::strlen("fargs");
                    اگر (std::isspace(*p)) {
                        وقتی (std::isspace(*p)) ++p;
                        اگر (!has_fargs) {
                            has_fargs = درست;
                            whole_file.clear();
                        }
                        whole_file += p;
                        اگر (blockCmt) {
                            ثابت std::string::size_type pos = whole_file.find("*/");
                            اگر (pos == std::string::npos) {
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

    منطقی inCmt = نادرست;
    وقتی (!ifp->eof()) {
        ثابت std::string سطر = V3Os::getline(*ifp);
        // Strip simple comments
        نویسه lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        منطقی space_begin = درست;  // At beginning or leading spaces only
        برای (ثابت نویسه* p = سطر.c_str(); *p; lastch = *p++) {
            اگر (inCmt) {
                اگر (p[0] == '*' && p[1] == '/') {
                    inCmt = نادرست;
                    ++p;
                }
            } وگرنه اگر (p[0] == '/' && p[1] == '/' && std::isspace(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                بشکن;  // Ignore to EOL
            } وگرنه اگر (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                بشکن;  // Ignore to EOL
            } وگرنه اگر (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                inCmt = درست;
                space_begin = نادرست;
                ++p;
            } وگرنه {
                اگر (!std::isspace(*p)) space_begin = نادرست;
                اگر (!has_fargs) whole_file += *p;
            }
        }
        اگر (!has_fargs) whole_file += " ";
    }
    whole_file += "\n";  // So string match below is simplified
    اگر (inCmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = جدید File_سطر{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::vector<string> args;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    شمارش state : uint8_t {
        ST_IN_OPTION,
        ST_ESCAPED_CHAR,
        ST_IN_QUOTED_STR,
        ST_IN_DOUBLE_QUOTED_STR
    };

    state st = ST_IN_OPTION;
    state last_st = ST_IN_OPTION;
    string arg;
    برای (string::size_type pos = 0; pos < whole_file.length(); ++pos) {
        نویسه curr_char = whole_file[pos];
        انتخاب (st) {
        حالت ST_IN_OPTION:  // Get all chars up to a white space or a "="
            اگر (std::isspace(curr_char)) {  // End of option
                اگر (!arg.empty()) {  // End of word
                    args.افزودن_back(arg);
                }
                arg = "";
                بشکن;
            }
            اگر (curr_char == '\\') {  // Escape char, we wait for next char
                last_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                بشکن;
            }
            اگر (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                اگر (pos < whole_file.length()) curr_char = whole_file[pos];
                اگر (curr_char == '"') {  // String
                    st = ST_IN_QUOTED_STR;
                } وگرنه {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                بشکن;
            }
            اگر (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_IN_DOUBLE_QUOTED_STR;
                بشکن;
            }
            arg += curr_char;
            بشکن;
        حالت ST_IN_QUOTED_STR:  // Just store all chars inside string
            اگر (curr_char != '\'') {
                arg += curr_char;
            } وگرنه {  // End of quoted string
                st = ST_IN_OPTION;
            }
            بشکن;
        حالت ST_IN_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            اگر (curr_char != '"') {
                اگر (curr_char == '\\') {
                    last_st = st;
                    st = ST_ESCAPED_CHAR;
                } وگرنه {
                    arg += curr_char;
                }
            } وگرنه {  // End of double quoted string
                st = ST_IN_OPTION;
            }
            بشکن;
        حالت ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = last_st;
            بشکن;
        }
    }
    اگر (!arg.empty()) {  // Add last word
        args.افزودن_back(arg);
    }

    // Path
    ثابت string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::vector<نویسه*> argv;
    argv.reserve(args.size() + 1);
    برای (ثابت string& i : args) argv.افزودن_back(const_cast<نویسه*>(i.c_str()));
    argv.افزودن_back(nullptr);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<صحیح>(argv.size() - 1), argv.data());
}

//======================================================================

string V3Options::parseFileArg(ثابت string& optdir, ثابت string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    اگر (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    بازگشت filename;
}

//======================================================================

تهی V3Options::showVersion(منطقی verbose) {
    cout << version();
    cout << "\n";
    اگر (!verbose) بازگشت;

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
    cout << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_ریشه << "\n";
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
    m_impp = جدید V3OptionsImp;

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

تهی V3Options::setDebugMode(صحیح level) {
    V3Error::debugDefault(level);
    اگر (!m_dumpLevel.شمار("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = درست;
    m_debugCheck = درست;
    اگر (level) {
        cout << "- Starting " << version() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

unsigned V3Options::debugLevel(ثابت string& tag) ثابت VL_MT_SAFE {
    ثابت auto iter = m_debugLevel.find(tag);
    بازگشت iter != m_debugLevel.end() ? iter->second : V3Error::debugDefault();
}

unsigned V3Options::debugSrcLevel(ثابت string& srcfile_path) ثابت VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    بازگشت debugLevel(V3Os::filenameNonDirExt(srcfile_path));
}

unsigned V3Options::dumpLevel(ثابت string& tag) ثابت VL_MT_SAFE {
    ثابت auto iter = m_dumpLevel.find(tag);
    بازگشت iter != m_dumpLevel.end() ? iter->second : 0;
}

unsigned V3Options::dumpSrcLevel(ثابت string& srcfile_path) ثابت VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    بازگشت dumpLevel(V3Os::filenameNonDirExt(srcfile_path));
}

منطقی V3Options::dump_درخت_Addrids() ثابت VL_MT_SAFE {
    ایستا صحیح s_level = -1;
    اگر (VL_UNLIKELY(s_level < 0)) {
        ثابت unsigned مقدار = dumpLevel("tree-addrids");
        اگر (!available()) بازگشت مقدار > 0;
        s_level = static_cast<unsigned>(مقدار);
    }
    بازگشت s_level > 0;
}

تهی V3Options::optimize(صحیح level) {
    // Set all optimizations to on/off
    ثابت منطقی flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_fBitScanLoops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_fCaseTable = flag;
    m_f_Case_درخت = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_درخت = flag;
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
    اگر (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
