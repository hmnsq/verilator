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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Options.h"

#अन्तर्भाव "V3Container.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Mutex.h"
#अन्तर्भाव "V3OptionParser.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3PreShell.h"
#अन्तर्भाव "V3String.h"

// clang-format off
#अन्तर्भाव <sys/types.h>
#अन्तर्भाव <sys/stat.h>
#यदि_अपरिभाषितम् _WIN32
# include <sys/utsname.h>
#यदि_अन्तः
#अन्तर्भाव <algorithm>
#अन्तर्भाव <cctype>
#यदि_परिभाषितम् _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(बहुलकम्) (((बहुलकम्) & _S_IFMT) == _S_IFDIR)
#अन्यथा
# include <dirent.h>
#यदि_अन्तः
#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <list>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>
#अन्तर्भाव <thread>
#अन्तर्भाव <set>
#अन्तर्भाव <string>

#अन्तर्भाव "config_rev.h"

#यदि defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#यदि_अन्तः
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

वर्ग V3OptionsImp final {
सार्वजनिक:
    // TYPES
    उपयुज्य DirMap = std::map<ध्रुव string, std::समुच्चयः<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_पङ्क्तिः_तर्काः;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, तार्किक>> m_all_तर्काः;
    VInsertion_समुच्चयः<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::समुच्चयः<string> m_inc_Dir_Fallback_समुच्चयः;  // Include directories (for removing duplicates)
    std::map<ध्रुव string, V3_Lang_संहिता> m_langExts;  // Language extension map
    VInsertion_समुच्चयः<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    शून्य सङ्कलय_Inc_Dir_User(ध्रुव string& incdir) {
        ध्रुव string& dir = V3Os::filenameCleanup(incdir);
        ध्रुव तार्किक inserted = m_incDirUsers.निवेशय(dir);
        यदि (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_inc_Dir_Fallback_समुच्चयः.erase(dir);  // User has priority over Fallback
        }
    }
    शून्य सङ्कलय_Inc_Dir_Fallback(ध्रुव string& incdir) {
        ध्रुव string& dir = V3Os::filenameCleanup(incdir);
        यदि (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ध्रुव स्वचालित it_प्राप्तम्_Pair = m_inc_Dir_Fallback_समुच्चयः.निवेशय(dir);
            यदि (it_प्राप्तम्_Pair.second) m_incDirFallbacks.प्रक्षिप_पृष्ठभागः(dir);
        }
    }
    शून्य सङ्कलय_Lang_Ext(ध्रुव string& langext, ध्रुव V3_Lang_संहिता& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        यदि (addext[0] == '.') addext = addext.substr(1);
        (शून्य)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    शून्य सङ्कलय_Lib_Ext_V(ध्रुव string& libext) { m_libExtVs.निवेशय(libext); }
    V3OptionsImp() = स्वभाव;
    ~V3OptionsImp() = स्वभाव;
};

//######################################################################
// V3LangCode class functions

V3_Lang_संहिता::V3_Lang_संहिता(ध्रुव वर्ण* textp) {
    // Return code for given string, or ERROR, which is a bad code
    प्रति (पूर्णाङ्क codei = V3_Lang_संहिता::L_ERROR; codei < V3_Lang_संहिता::ENUM_अन्तः; ++codei) {
        ध्रुव V3_Lang_संहिता संहिता{codei};
        यदि (0 == VL_STRCASECMP(textp, संहिता.ascii())) {
            m_e = संहिता;
            प्रत्यागम;
        }
    }
    m_e = V3_Lang_संहिता::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ध्रुव string& मानम्, तार्किक& badr)
    : m_e{VTimescale::NONE} {
    badr = सत्य;
    ध्रुव string spaceless = VString::removeWhitespace(मानम्);
    प्रति (पूर्णाङ्क i = TS_100S; i < ENUM_अन्तः; ++i) {
        ध्रुव VTimescale ts{i};
        यदि (spaceless == ts.ascii()) {
            badr = असत्य;
            m_e = ts.m_e;
            भङ्ग;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3_श्रेणीबद्धम्_खण्डम्_Option::V3_श्रेणीबद्धम्_खण्डम्_Option(ध्रुव string& opts) {
    VStringList vals;
    तार्किक अन्तर्_Str = असत्य;
    string cur;
    स्थिर ध्रुव string hier_खण्डम्("--hierarchical-block");
    File_पङ्क्तिः cmdfl(File_पङ्क्तिः::command_पङ्क्तिः_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    प्रति (string::const_iterator it = opts.begin(); it != opts.अन्तः_2();) {
        यदि (अन्तर्_Str) {
            यदि (*it == '\\') {
                ++it;
                यदि (it == opts.अन्तः_2()) {
                    cmdfl.v3error(hier_खण्डम् + " must not end with \\");
                    भङ्ग;
                }
                यदि (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hier_खण्डम् + " does not allow '" + *it + "' after \\");
                    भङ्ग;
                }
                cur.प्रक्षिप_पृष्ठभागः(*it);
                ++it;
            } अन्यथा यदि (*it == '"') {  // end of string
                cur.प्रक्षिप_पृष्ठभागः(*it);
                vals.प्रक्षिप_पृष्ठभागः(cur);
                cur.clear();
                ++it;
                यदि (it != opts.अन्तः_2()) {
                    यदि (*it != ',') {
                        cmdfl.v3error(hier_खण्डम् + " expects ',', but '" + *it + "' is passed");
                        भङ्ग;
                    }
                    ++it;
                    यदि (it == opts.अन्तः_2()) {
                        cmdfl.v3error(hier_खण्डम् + " must not end with ','");
                        भङ्ग;
                    }
                    अन्तर्_Str = *it == '"';
                    cur.प्रक्षिप_पृष्ठभागः(*it);
                    ++it;
                }
            } अन्यथा {
                cur.प्रक्षिप_पृष्ठभागः(*it);
                ++it;
            }
        } अन्यथा {
            यदि (*it == '"') {
                cmdfl.v3error(hier_खण्डम् + " does not allow '\"' in the middle of literal");
                भङ्ग;
            }
            यदि (*it == ',') {  // end of this parameter
                vals.प्रक्षिप_पृष्ठभागः(cur);
                cur.clear();
                ++it;
                यदि (it == opts.अन्तः_2()) {
                    cmdfl.v3error(hier_खण्डम् + " must not end with ','");
                    भङ्ग;
                }
                अन्तर्_Str = *it == '"';
            }
            cur.प्रक्षिप_पृष्ठभागः(*it);
            ++it;
        }
    }
    यदि (!cur.रिक्तानि()) vals.प्रक्षिप_पृष्ठभागः(cur);
    यदि (vals.आकार() >= 2) {
        यदि (vals.आकार() % 2) {
            cmdfl.v3error(hier_खण्डम् + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } अन्यथा {
        cmdfl.v3error(hier_खण्डम् + " requires at least two comma-separated values");
    }
    प्रति (आकार_t i = 2; i + 1 < vals.आकार(); i += 2) {
        ध्रुव तार्किक inserted = m_parameters.emplace(vals[i], vals[i + 1]).second;
        यदि (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hier_खण्डम्);
        }
    }
}

//######################################################################
// V3Options class functions

शून्य VTimescale::parseSlashed(File_पङ्क्तिः* fl, ध्रुव वर्ण* textp, VTimescale& unitr,
                              VTimescale& precr, तार्किक allow_रिक्तानि) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ध्रुव वर्ण* cp = textp;
    प्रति (; std::रिक्तम्_वा(*cp); ++cp) {}
    ध्रुव वर्ण* ध्रुव unitp = cp;
    प्रति (; *cp && *cp != '/'; ++cp) {}
    ध्रुव string unitStr(unitp, cp - unitp);
    प्रति (; std::रिक्तम्_वा(*cp); ++cp) {}
    string precStr;
    यदि (*cp == '/') {
        ++cp;
        प्रति (; std::रिक्तम्_वा(*cp); ++cp) {}
        ध्रुव वर्ण* ध्रुव precp = cp;
        प्रति (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    प्रति (; std::रिक्तम्_वा(*cp); ++cp) {}
    यदि (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        प्रत्यागम;
    }

    तार्किक unitbad = असत्य;
    ध्रुव VTimescale unit{unitStr, unitbad /*ref*/};
    यदि (unitbad && !(unitStr.रिक्तानि() && allow_रिक्तानि)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        प्रत्यागम;
    }
    unitr = unit;

    यदि (!precStr.रिक्तानि()) {
        VTimescale prec{VTimescale::NONE};
        तार्किक precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        यदि (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            प्रत्यागम;
        }
        यदि (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            प्रत्यागम;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

शून्य V3Options::सङ्कलय_Inc_Dir_User(ध्रुव string& incdir) { m_impp->सङ्कलय_Inc_Dir_User(incdir); }
शून्य V3Options::सङ्कलय_Inc_Dir_Fallback(ध्रुव string& incdir) { m_impp->सङ्कलय_Inc_Dir_Fallback(incdir); }
शून्य V3Options::सङ्कलय_Lang_Ext(ध्रुव string& langext, ध्रुव V3_Lang_संहिता& lc) {
    m_impp->सङ्कलय_Lang_Ext(langext, lc);
}
शून्य V3Options::सङ्कलय_Lib_Ext_V(ध्रुव string& libext) { m_impp->सङ्कलय_Lib_Ext_V(libext); }
शून्य V3Options::सङ्कलय_Define(ध्रुव string& defline, तार्किक allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string वामतः = defline;
    यावत् (वामतः != "") {
        string def = वामतः;
        string::आकार_type pos;
        यदि (allowPlus && ((pos = वामतः.find('+')) != string::npos)) {
            वामतः = वामतः.substr(pos + 1);
            def.erase(pos);
        } अन्यथा {
            वामतः = "";
        }
        string मानम्;
        यदि ((pos = def.find('=')) != string::npos) {
            मानम् = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_पङ्क्तिः(def, मानम्);
    }
}
शून्य V3Options::सङ्कलय_Parameter(ध्रुव string& paramline, तार्किक allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string वामतः = paramline;
    यावत् (वामतः != "") {
        string param = वामतः;
        string::आकार_type pos;
        यदि (allowPlus && ((pos = वामतः.find('+')) != string::npos)) {
            वामतः = वामतः.substr(pos + 1);
            param.erase(pos);
        } अन्यथा {
            वामतः = "";
        }
        string मानम्;
        यदि ((pos = param.find('=')) != string::npos) {
            मानम् = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << मानम्);
        (शून्य)m_parameters.erase(param);
        m_parameters[param] = मानम्;
    }
}

तार्किक V3Options::hasParameter(ध्रुव string& name) {
    प्रत्यागम m_parameters.find(name) != m_parameters.अन्तः_2();
}

string V3Options::parameter(ध्रुव string& name) {
    string मानम् = m_parameters.find(name)->second;
    m_parameters.erase(m_parameters.find(name));
    प्रत्यागम मानम्;
}

शून्य V3Options::checkParameters() {
    यदि (!m_parameters.रिक्तानि()) {
        std::stringstream सन्देशः_2;
        सन्देशः_2 << "Parameters from the command line were not found in the design:";
        प्रति (ध्रुव स्वचालित& i : m_parameters) सन्देशः_2 << " " << i.first;
        v3error(सन्देशः_2.str());
    }
}

शून्य V3Options::सङ्कलय_Cpp_File(ध्रुव string& filename) { m_cppFiles.निवेशय(filename); }
शून्य V3Options::सङ्कलय_CFlags(ध्रुव string& filename) { m_cFlags.प्रक्षिप_पृष्ठभागः(filename); }
शून्य V3Options::सङ्कलय_Lib_Map_File(ध्रुव string& filename) { m_libmapFiles.निवेशय(filename); }
शून्य V3Options::सङ्कलय_Compiler_Includes(ध्रुव string& filename) {
    m_compilerIncludes.निवेशय(filename);
}
शून्य V3Options::सङ्कलय_Ld_Lib_File(ध्रुव string& filename) {
    m_ldLibFiles.प्रक्षिप_पृष्ठभागः(filename);
    सङ्कलय_Ld_Libs(filename);
}
शून्य V3Options::सङ्कलय_Ld_Libs(ध्रुव string& filename) { m_ldLibs.प्रक्षिप_पृष्ठभागः(filename); }
शून्य V3Options::सङ्कलय_Make_Flags(ध्रुव string& filename) { m_makeFlags.प्रक्षिप_पृष्ठभागः(filename); }
शून्य V3Options::सङ्कलय_Future(ध्रुव string& flag) { m_futures.निवेशय(flag); }
शून्य V3Options::सङ्कलय_Future0(ध्रुव string& flag) { m_future0s.निवेशय(flag); }
शून्य V3Options::सङ्कलय_Future1(ध्रुव string& flag) { m_future1s.निवेशय(flag); }
तार्किक V3Options::isFuture(ध्रुव string& flag) ध्रुव {
    प्रत्यागम m_futures.find(flag) != m_futures.अन्तः_2();
}
तार्किक V3Options::isFuture0(ध्रुव string& flag) ध्रुव {
    प्रत्यागम m_future0s.find(flag) != m_future0s.अन्तः_2();
}
तार्किक V3Options::isFuture1(ध्रुव string& flag) ध्रुव {
    प्रत्यागम m_future1s.find(flag) != m_future1s.अन्तः_2();
}
तार्किक V3Options::isLibraryFile(ध्रुव string& filename, ध्रुव string& libname) ध्रुव {
    प्रत्यागम m_libraryFiles.find({filename, libname}) != m_libraryFiles.अन्तः_2();
}
शून्य V3Options::सङ्कलय_Library_File(ध्रुव string& filename, ध्रुव string& libname) {
    m_libraryFiles.निवेशय({filename, libname});
}
शून्य V3Options::सङ्कलय_VFile(ध्रुव string& filename, ध्रुव string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.प्रक्षिप_पृष्ठभागः({filename, libname});  // Not emplace
}
शून्य V3Options::सङ्कलय_Vlt_File(ध्रुव string& filename, ध्रुव string& libname) {
    m_vltFiles.निवेशय({filename, libname});
}
शून्य V3Options::सङ्कलय_Force_Inc(ध्रुव string& filename) { m_forceIncs.प्रक्षिप_पृष्ठभागः(filename); }

शून्य V3Options::सङ्कलय_पङ्क्तिः_Arg(ध्रुव string& arg) { m_impp->m_पङ्क्तिः_तर्काः.प्रक्षिप_पृष्ठभागः(arg); }

शून्य V3Options::सङ्कलय_Arg(वर्ण** argv, आकार_t गणनम्, तार्किक isForRerun) {
    m_impp->m_all_तर्काः.emplace_पृष्ठभागः();
    std::pair<std::list<std::string>, तार्किक>& pair = m_impp->m_all_तर्काः.पृष्ठभागः();
    pair.second = isForRerun;
    प्रति (आकार_t n = 0; n < गणनम्; ++n) pair.first.emplace_पृष्ठभागः(argv[n]);
}

शून्य V3Options::सङ्कलय_Arg(ध्रुव std::string& arg, तार्किक isForRerun) {
    m_impp->m_all_तर्काः.emplace_पृष्ठभागः();
    std::pair<std::list<std::string>, तार्किक>& pair = m_impp->m_all_तर्काः.पृष्ठभागः();
    pair.second = isForRerun;
    pair.first.emplace_पृष्ठभागः(arg);
}

ध्रुव std::list<std::pair<std::list<std::string>, तार्किक>>& V3Options::all_तर्काः() ध्रुव {
    प्रत्यागम m_impp->m_all_तर्काः;
}

string V3Options::all_तर्काः_String() ध्रुव VL_MT_SAFE {
    string परिणाम;
    प्रति (ध्रुव स्वचालित& pair : m_impp->m_all_तर्काः) {
        प्रति (ध्रुव string& arg : pair.first) {
            यदि (!परिणाम.रिक्तानि()) परिणाम += " ";
            परिणाम += arg;
        }
    }
    प्रत्यागम परिणाम;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::all_तर्काः_String_For_Hier_खण्डम्(तार्किक for_शिखरम्) ध्रुव {
    // cppcheck-suppress shadowFunction
    std::समुच्चयः<string> vFiles;
    प्रति (ध्रुव स्वचालित& vFile : m_vFiles) vFiles.निवेशय(vFile.filename());
    string बहिर्गमनम्;
    तार्किक stripArg = असत्य;
    तार्किक strip_Arg_If_अङ्कः = असत्य;
    प्रति (ध्रुव स्वचालित& pair : m_impp->m_all_तर्काः) {
        प्रति (ध्रुव string& arg : pair.first) {
            यदि (stripArg) {
                stripArg = असत्य;
                अनुवर्त;
            }
            यदि (strip_Arg_If_अङ्कः) {
                strip_Arg_If_अङ्कः = असत्य;
                यदि (अङ्कः_वा(arg[0])) अनुवर्त;
            }
            पूर्णाङ्क skip = 0;
            यदि (arg.दीर्घता_2() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } अन्यथा यदि (arg.दीर्घता_2() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            यदि (skip > 0) {  // arg is an option
                ध्रुव string opt = arg.substr(skip);  // Remove '-' in the beginning
                ध्रुव पूर्णाङ्क अङ्कः_Strip = strip_Options_For_Child_सततम्(opt, for_शिखरम्);
                यदि (अङ्कः_Strip) {
                    UASSERT(0 <= अङ्कः_Strip && अङ्कः_Strip <= 3, "should be one of 0, 1, 2, 3");
                    यदि (अङ्कः_Strip == 2) stripArg = सत्य;
                    यदि (अङ्कः_Strip == 3) strip_Arg_If_अङ्कः = सत्य;
                    अनुवर्त;
                }
            } अन्यथा {  // Not an option
                यदि (vFiles.find(arg) != vFiles.अन्तः_2()  // Remove HDL
                    || m_cppFiles.find(arg) != m_cppFiles.अन्तः_2()) {  // Remove C++
                    अनुवर्त;
                }
            }
            यदि (बहिर्गमनम् != "") बहिर्गमनम् += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            बहिर्गमनम् += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    प्रत्यागम बहिर्गमनम्;
}

शून्य V3Options::cc_समुच्चयः() {  // --cc
    m_बहिर्गमनम्_Format_Ok = सत्य;
    m_systemC = असत्य;
}

शून्य V3Options::decorations(File_पङ्क्तिः* fl, ध्रुव string& arg) {  // --decorations
    यदि (arg == "none") {
        m_decoration = असत्य;
        m_decorationNodes = असत्य;
    } अन्यथा यदि (arg == "node") {
        m_decoration = सत्य;
        m_decorationNodes = सत्य;
    } अन्यथा यदि (arg == "medium") {
        m_decoration = सत्य;
        m_decorationNodes = असत्य;
    } अन्यथा {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warn_अधिकम्() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::सदिशम्<std::string> V3Options::traceClassBases() ध्रुव VL_MT_SAFE {
    std::सदिशम्<std::string> परिणाम;
    यदि (traceEnabledFst()) परिणाम.emplace_पृष्ठभागः("VerilatedFst");
    यदि (traceEnabledSaif()) परिणाम.emplace_पृष्ठभागः("VerilatedSaif");
    यदि (traceEnabledVcd()) परिणाम.emplace_पृष्ठभागः("VerilatedVcd");
    प्रत्यागम परिणाम;
}
std::सदिशम्<std::string> V3Options::traceClassLangs() ध्रुव VL_MT_SAFE {
    std::सदिशम्<std::string> परिणाम;
    प्रति (ध्रुव स्वचालित& cbase : traceClassBases())
        परिणाम.emplace_पृष्ठभागः(cbase + (systemC() ? "Sc" : "C"));
    प्रत्यागम परिणाम;
}
std::सदिशम्<std::string> V3Options::traceSourceBases() ध्रुव VL_MT_SAFE {
    std::सदिशम्<std::string> परिणाम;
    यदि (traceEnabledFst()) परिणाम.emplace_पृष्ठभागः("verilated_fst");
    यदि (traceEnabledSaif()) परिणाम.emplace_पृष्ठभागः("verilated_saif");
    यदि (traceEnabledVcd()) परिणाम.emplace_पृष्ठभागः("verilated_vcd");
    प्रत्यागम परिणाम;
}
std::सदिशम्<std::string> V3Options::traceSourceLangs() ध्रुव VL_MT_SAFE {
    std::सदिशम्<std::string> परिणाम = traceSourceBases();
    प्रति (std::string& str : परिणाम) str += systemC() ? "_sc"s : "_c"s;
    प्रत्यागम परिणाम;
}
std::string V3Options::trace_Class_आधारः() ध्रुव VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().रिक्तानि(), "Call traceClassBase only when trace() enabled");
    प्रत्यागम traceClassBases().front();
}
std::string V3Options::traceClassLang() ध्रुव VL_MT_SAFE {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().रिक्तानि(), "Call traceClassLang only when trace() enabled");
    प्रत्यागम traceClassLangs().front();
}

//######################################################################
// File searching

तार्किक V3Options::file_Stat_प्रसामान्यम्(ध्रुव string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    संरचना stat sstat;  // Stat information
    ध्रुव पूर्णाङ्क err = stat(filename.c_str(), &sstat);
    यदि (err != 0) प्रत्यागम असत्य;
    यदि (S_ISDIR(sstat.st_बहुलकम्)) प्रत्यागम असत्य;
    प्रत्यागम सत्य;
}

string V3Options::fileExists(ध्रुव string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ध्रुव string dir = V3Os::filenameDir(filename);
    ध्रुव string basename = V3Os::filenameNonDir(filename);

    स्वचालित diriter = m_impp->m_dirMap.find(dir);
    यदि (diriter == m_impp->m_dirMap.अन्तः_2()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::समुच्चयः<string>());
        diriter = m_impp->m_dirMap.find(dir);

        std::समुच्चयः<string>* ध्रुव setp = &(diriter->second);

#यदि_परिभाषितम् _MSC_VER
        यत्न {
            प्रति (ध्रुव स्वचालित& dir_प्रविष्टिः : std::filesystem::directory_iterator(dir.c_str()))
                setp->निवेशय(dir_प्रविष्टिः.मार्गः().filename().string());
        } गृहाण (std::filesystem::filesystem_error ध्रुव& ex) {
            (शून्य)ex;
            प्रत्यागम "";
        }
#अन्यथा
        यदि (DIR* ध्रुव dirp = opendir(dir.c_str())) {
            यावत् (संरचना dirent* direntp = readdir(dirp)) setp->निवेशय(direntp->d_name);
            closedir(dirp);
        }
#यदि_अन्तः
    }
    // Find it
    ध्रुव std::समुच्चयः<string>* ध्रुव filesetp = &(diriter->second);
    ध्रुव स्वचालित fileiter = filesetp->find(basename);
    यदि (fileiter == filesetp->अन्तः_2()) प्रत्यागम "";  // Not found
    // Check if it is a directory, ignore if so
    ध्रुव string filename_बहिर्गमनम् = V3Os::filenameJoin(dir, basename);
    यदि (!file_Stat_प्रसामान्यम्(filename_बहिर्गमनम्)) प्रत्यागम "";  // Directory
    प्रत्यागम filename_बहिर्गमनम्;
}

string V3Options::file_मार्गः_Check_One_Dir(ध्रुव string& modname, ध्रुव string& dirname) {
    प्रति (ध्रुव string& i : m_impp->m_libExtVs) {
        ध्रुव string fn = V3Os::filenameJoin(dirname, modname + i);
        ध्रुव string exists = fileExists(fn);
        यदि (exists != "") प्रत्यागम exists;
    }
    प्रत्यागम "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
पूर्णाङ्क V3Options::strip_Options_For_Child_सततम्(ध्रुव string& opt, तार्किक for_शिखरम्) {
    // Options to strip for both the top wrapper, and hier blocks
    स्थिर ध्रुव std::unordered_map<std::string, पूर्णाङ्क> commonOpts{
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
    यदि (commonOpts.गणनम्(opt)) प्रत्यागम commonOpts.at(opt);

    // Options to strip only for hier blocks
    यदि (!for_शिखरम्) {
        स्थिर ध्रुव std::unordered_map<std::string, पूर्णाङ्क> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        यदि (subOpts.गणनम्(opt)) प्रत्यागम subOpts.at(opt);
        यदि (VString::startsWith(opt, "G")) प्रत्यागम 1;
        यदि (VString::startsWith(opt, "pvalue")) प्रत्यागम 1;
    }

    // Do not strip
    प्रत्यागम 0;
}

शून्य V3Options::validateIdentifier(File_पङ्क्तिः* fl, ध्रुव string& arg, ध्रुव string& opt) {
    यदि (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::file_मार्गः(File_पङ्क्तिः* fl, ध्रुव string& modname, ध्रुव string& lastpath,
                           ध्रुव string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ध्रुव string filename = V3Os::filenameCleanup(VName::dehash(modname));
    यदि (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ध्रुव string exists = file_मार्गः_Check_One_Dir(filename, "");
        यदि (exists != "") प्रत्यागम exists;
    }
    प्रति (ध्रुव string& dir : m_impp->m_incDirUsers) {
        ध्रुव string exists = file_मार्गः_Check_One_Dir(filename, dir);
        यदि (exists != "") प्रत्यागम exists;
    }
    प्रति (ध्रुव string& dir : m_impp->m_incDirFallbacks) {
        ध्रुव string exists = file_मार्गः_Check_One_Dir(filename, dir);
        यदि (exists != "") प्रत्यागम exists;
    }

    यदि (m_relativeIncludes) {
        ध्रुव string exists = file_मार्गः_Check_One_Dir(filename, lastpath);
        यदि (exists != "") प्रत्यागम V3Os::filename_Real_मार्गः(exists);
    }

    // Warn and return not found
    यदि (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_मार्गः_Looked_सन्देशः(fl, filename));
    }
    प्रत्यागम "";
}

string V3Options::file_मार्गः_Looked_सन्देशः(File_पङ्क्तिः* /*fl*/, ध्रुव string& modname) {
    स्थिर तार्किक s_shown_notfound_सन्देशः = असत्य;
    std::ostringstream ss;
    यदि (modname.find("__Vhsh") != string::npos) {
        ss << V3Error::warn_अधिकम्() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warn_अधिकम्() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } अन्यथा यदि (!s_shown_notfound_सन्देशः) {
        s_shown_notfound_सन्देशः = सत्य;
        यदि (m_impp->m_incDirUsers.रिक्तानि()) {
            ss << V3Error::warn_अधिकम्()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warn_अधिकम्() << "... Looked in:\n";
        प्रति (ध्रुव string& dir : m_impp->m_incDirUsers) {
            प्रति (ध्रुव string& ext : m_impp->m_libExtVs) {
                ध्रुव string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_अधिकम्() << "     " << fn << "\n";
            }
        }
        प्रति (ध्रुव string& dir : m_impp->m_incDirFallbacks) {
            प्रति (ध्रुव string& ext : m_impp->m_libExtVs) {
                ध्रुव string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_अधिकम्() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warn_अधिकम्() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    प्रत्यागम ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3_Lang_संहिता V3Options::fileLanguage(ध्रुव string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::आकार_type pos;
    यदि (filename == V3Options::get_Std_Package_मार्गः() || filename == V3Options::get_Std_Waiver_मार्गः()) {
        प्रत्यागम V3_Lang_संहिता::mostRecent();
    } अन्यथा यदि ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ध्रुव स्वचालित it = m_impp->m_langExts.find(ext);
        यदि (it != m_impp->m_langExts.अन्तः_2()) प्रत्यागम it->second;
    }
    प्रत्यागम m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ध्रुव string& var) {
    // If update below, also update V3Options::showVersion()
    यदि (var == "MAKE") {
        प्रत्यागम getenvMAKE();
    } अन्यथा यदि (var == "PERL") {
        प्रत्यागम getenvPERL();
    } अन्यथा यदि (var == "PYTHON3") {
        प्रत्यागम getenvPYTHON3();
    } अन्यथा यदि (var == "SYSTEMC") {
        प्रत्यागम getenvSYSTEMC();
    } अन्यथा यदि (var == "SYSTEMC_ARCH") {
        प्रत्यागम getenvSYSTEMC_ARCH();
    } अन्यथा यदि (var == "SYSTEMC_INCLUDE") {
        प्रत्यागम getenvSYSTEMC_INCLUDE();
    } अन्यथा यदि (var == "SYSTEMC_LIBDIR") {
        प्रत्यागम getenvSYSTEMC_LIBDIR();
    } अन्यथा यदि (var == "VERILATOR_ROOT") {
        प्रत्यागम getenv_VERILATOR_मूलम्();
    } अन्यथा {
        प्रत्यागम V3Os::getenvStr(var, "");
    }
}

#यदि_परिभाषितम् __FreeBSD__
string V3Options::getenvMAKE() { प्रत्यागम V3Os::getenvStr("MAKE", "gmake"); }
#अन्यथा
string V3Options::getenvMAKE() { प्रत्यागम V3Os::getenvStr("MAKE", "make"); }
#यदि_अन्तः

string V3Options::getenvMAKEFLAGS() {  //
    प्रत्यागम V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    प्रत्यागम V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    प्रत्यागम V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ध्रुव string defenv = string{DEFENV_SYSTEMC};
    यदि (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    प्रत्यागम V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ध्रुव string defenv = string{DEFENV_SYSTEMC_ARCH};
    यदि (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    यदि (var == "") {
#यदि defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ध्रुव string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#नोचेद्यदि defined(_WIN32)
        ध्रुव string sysname = "WIN32";
        var = "win32";
#अन्यथा
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        संरचना utsname uts;
        uname(&uts);
        ध्रुव string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        यदि (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } अन्यथा यदि (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } अन्यथा {
            var = "linux";
        }
#यदि_अन्तः
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    प्रत्यागम var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ध्रुव string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    यदि (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    यदि (var == "") {
        ध्रुव string sc = getenvSYSTEMC();
        यदि (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    प्रत्यागम V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ध्रुव string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    यदि (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    यदि (var == "") {
        ध्रुव string sc = getenvSYSTEMC();
        ध्रुव string arch = getenvSYSTEMC_ARCH();
        यदि (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    प्रत्यागम V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_मूलम्() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ध्रुव string defenv = string{DEFENV_VERILATOR_मूलम्};
    यदि (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    यदि (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    प्रत्यागम V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ध्रुव string defenv = string{DEFENV_VERILATOR_SOLVER};
    यदि (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    प्रत्यागम var;
}

string V3Options::get_Std_Package_मार्गः() {
    प्रत्यागम V3Os::filenameJoin(getenv_VERILATOR_मूलम्(), "include", "verilated_std.sv");
}
string V3Options::get_Std_Waiver_मार्गः() {
    प्रत्यागम V3Os::filenameJoin(getenv_VERILATOR_मूलम्(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ध्रुव string& var) {
    // If update below, also update V3Options::showVersion()
    यदि (var == "COROUTINES" && coroutineSupport()) {
        प्रत्यागम "1";
    } अन्यथा यदि (var == "TSAN" && tsanSupport()) {
        प्रत्यागम "1";
    } अन्यथा यदि (var == "DEV_ASAN" && devAsan()) {
        प्रत्यागम "1";
    } अन्यथा यदि (var == "DEV_GCOV" && devGcov()) {
        प्रत्यागम "1";
        // cppcheck-suppress knownConditionTrueFalse
    } अन्यथा यदि (var == "SYSTEMC" && systemCFound()) {
        प्रत्यागम "1";
    } अन्यथा {
        प्रत्यागम "";
    }
}

तार्किक V3Options::systemCSystemWide() {
#यदि_परिभाषितम् HAVE_SYSTEMC
    प्रत्यागम सत्य;
#अन्यथा
    प्रत्यागम असत्य;
#यदि_अन्तः
}

तार्किक V3Options::systemCFound() {
    प्रत्यागम (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().रिक्तानि() && !getenvSYSTEMC_LIBDIR().रिक्तानि()));
}

तार्किक V3Options::coroutineSupport() {
#यदि_परिभाषितम् HAVE_COROUTINES
    प्रत्यागम सत्य;
#अन्यथा
    प्रत्यागम असत्य;
#यदि_अन्तः
}

तार्किक V3Options::devAsan() {
#यदि_परिभाषितम् HAVE_DEV_ASAN
    प्रत्यागम सत्य;
#अन्यथा
    प्रत्यागम असत्य;
#यदि_अन्तः
}

तार्किक V3Options::devGcov() {
#यदि_परिभाषितम् HAVE_DEV_GCOV
    प्रत्यागम सत्य;
#अन्यथा
    प्रत्यागम असत्य;
#यदि_अन्तः
}

तार्किक V3Options::tsanSupport() {
#यदि_परिभाषितम् HAVE_TSAN
    प्रत्यागम सत्य;
#अन्यथा
    प्रत्यागम असत्य;
#यदि_अन्तः
}

//######################################################################
// V3 Options notification methods

शून्य V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_पङ्क्तिः* ध्रुव cmdfl = नूतन File_पङ्क्तिः{File_पङ्क्तिः::command_पङ्क्तिः_Filename()};

    यदि (!बहिर्गमनम्_Format_Ok() && v3Global.opt.मुख्यम्()) cc_समुच्चयः();  // --main implies --cc if not provided
    यदि (!बहिर्गमनम्_Format_Ok() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    यदि (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::सदिशम्<std::string> backendFlags;
    यदि (m_build) {
        यदि (m_द्विमान)
            backendFlags.emplace_पृष्ठभागः("--binary");
        अन्यथा
            backendFlags.emplace_पृष्ठभागः("--build");
    }
    यदि (m_preprocOnly) backendFlags.emplace_पृष्ठभागः("-E");
    यदि (m_dpiHdrOnly) backendFlags.emplace_पृष्ठभागः("--dpi-hdr-only");
    यदि (m_lintOnly) backendFlags.emplace_पृष्ठभागः("--lint-only");
    यदि (m_jsonOnly) backendFlags.emplace_पृष्ठभागः("--json-only");
    यदि (backendFlags.आकार() > 1) {
        std::string backendFlagsString = backendFlags.front();
        प्रति (आकार_t i = 1; i < backendFlags.आकार(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    यदि (m_exe && !v3Global.opt.libCreate().रिक्तानि()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    यदि (!m_gmake && !m_makeJson) m_gmake = सत्य;

    यदि (m_श्रेणीबद्धम् && (m_hierChild || !m_hier_खण्डानि.रिक्तानि())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    यदि (m_hierChild) {
        यदि (m_hier_खण्डानि.रिक्तानि()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = असत्य;
    }

    यदि (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = सत्य;  // No format, with --trace means wanted --trace-vcd
    }
    यदि (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = सत्य;
    ध्रुव पूर्णाङ्क ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    यदि (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    यदि (protectIds()) {
        यदि (all_लौकिकम्()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warn_अधिकम्() + "... Suggest remove --public.");
        }
        यदि (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warn_अधिकम्() + "... Suggest remove --trace.");
        }
        यदि (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warn_अधिकम्() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    यदि (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.समुच्चयः_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    यदि (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.समुच्चयः_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    यदि (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.समुच्चयः_True_Or_False(सत्य);

    // Default split limits if not specified
    यदि (m_output_विभज_CFuncs < 0) m_output_विभज_CFuncs = m_output_विभज;
    यदि (m_output_विभज_CTrace < 0) m_output_विभज_CTrace = m_output_विभज;

    यदि (v3Global.opt.मुख्यम्() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    यदि (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        यदि (श्रेणीबद्धम्()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    यदि (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    यदि (v3Global.opt.timing().is_समुच्चयः_True() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    यदि (!m_dumpLevel.गणनम्("tree") && m_dumpLevel.गणनम्("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(तन्तवः() >= 1, "'threads()' must return a value >= 1");
    यदि (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    यदि (m_buildJobs == -1) m_buildJobs = 1;
    यदि (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    यदि (timing().is_समुच्चयः_True()) V3PreShell::define_Cmd_पङ्क्तिः("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    यदि (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.समुच्चयः_True_Or_False(असत्य);

    // === Leave last
    // Mark options as available
    m_उपलब्धम् = सत्य;
}

//######################################################################
// V3 Options accessors

string V3Options::संस्करणम्() VL_विशुद्धम् {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_गन्तव्यम्_Str(DTVERSION_rev);
    प्रत्यागम ver;
}

string V3Options::protect_कुञ्चिका_Defaulted() VL_MT_SAFE {
    स्थिर V3Mutex s_mutex;
    ध्रुव V3_तालकम्_Guard तालकम्_2{s_mutex};
    यदि (m_protect_कुञ्चिका.रिक्तानि()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_कुञ्चिका = "VL-KEY-" + digest.digestBase64();
    }
    प्रत्यागम m_protect_कुञ्चिका;
}

शून्य V3Options::throwSigsegv() {  // LCOV_EXCL_START
#यदि !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<चञ्चल वर्ण*>(शून्यसूचकः) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#यदि_अन्तः
}  // LCOV_EXCL_STOP

VTimescale V3Options::timeComputePrec(ध्रुव VTimescale& flag) ध्रुव {
    यदि (!timeOverridePrec().isNone()) {
        प्रत्यागम timeOverridePrec();
    } अन्यथा यदि (flag.isNone()) {
        प्रत्यागम timeDefaultPrec();
    } अन्यथा {
        प्रत्यागम flag;
    }
}

VTimescale V3Options::timeComputeUnit(ध्रुव VTimescale& flag) ध्रुव {
    यदि (!timeOverrideUnit().isNone()) {
        प्रत्यागम timeOverrideUnit();
    } अन्यथा यदि (flag.isNone()) {
        प्रत्यागम timeDefaultUnit();
    } अन्यथा {
        प्रत्यागम flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(पूर्णाङ्क argc, वर्ण** argv) {
    // Return list of arguments as simple string
    string opts;
    प्रति (पूर्णाङ्क i = 0; i < argc; ++i) {
        यदि (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    प्रत्यागम opts;
}

//######################################################################
// V3 Options Parsing

शून्य V3Options::parseOpts(File_पङ्क्तिः* fl, पूर्णाङ्क argc, वर्ण** argv) VL_MT_DISABLED {
    // Save command line options
    प्रति (पूर्णाङ्क i = 0; i < argc; ++i) सङ्कलय_पङ्क्तिः_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    यदि (vFiles().रिक्तानि()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    यदि (उपसर्गः() == "" && शिखरम्_Module() != "") m_उपसर्गः = "V"s + शिखरम्_Module();  // Already encoded
    यदि (उपसर्गः() == "" && vFiles().आकार() >= 1)
        m_उपसर्गः
            = "V"s + Ast_ग्रन्थिः::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    यदि (mod_उपसर्गः() == "") m_mod_उपसर्गः = उपसर्गः();

    // Find files in makedir
    सङ्कलय_Inc_Dir_Fallback(makeDir());
}

//======================================================================

तार्किक V3Options::suffixed(ध्रुव string& sw, ध्रुव वर्ण* arg) {
    यदि (std::वाक्यदैर्घ्यम्(arg) > sw.दीर्घता_2()) प्रत्यागम असत्य;
    प्रत्यागम (0 == std::वाक्यं_तुलय(sw.c_str() + sw.दीर्घता_2() - std::वाक्यदैर्घ्यम्(arg), arg));
}

शून्य V3Options::parseOptsList(File_पङ्क्तिः* fl, ध्रुव string& optdir, पूर्णाङ्क argc,
                              वर्ण** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ध्रुव V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_चिह्नकानि;

    ध्रुव स्वचालित callStrSetter = [एतत्](शून्य (V3Options::*cbStr)(ध्रुव string&)) {
        प्रत्यागम [एतत्, cbStr](ध्रुव string& v) { (एतत्->*cbStr)(v); };
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
    DECL_OPTION("+define+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) VL_MT_DISABLED {
        सङ्कलय_Define(optp, सत्य);
    }).notForRerun();
    DECL_OPTION("+incdir+", Cb_Partial_मेलनम्, [एतत्, &optdir](ध्रुव वर्ण* optp) {
        string dirs = optp;
        string::आकार_type pos;
        यावत् ((pos = dirs.find('+')) != string::npos) {
            सङ्कलय_Inc_Dir_User(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        सङ्कलय_Inc_Dir_User(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        string exts = optp;
        string::आकार_type pos;
        यावत् ((pos = exts.find('+')) != string::npos) {
            सङ्कलय_Lib_Ext_V(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        सङ्कलय_Lib_Ext_V(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        सङ्कलय_Lang_Ext(optp, V3_Lang_संहिता::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](तार्किक) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [एतत्](तार्किक flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ध्रुव वर्ण*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [एतत्](तार्किक flag) {
        m_bboxUnsup = flag;
        File_पङ्क्तिः::globalWarnOff(V3_Error_संहिता::E_UNSUPPORTED, सत्य);
    });
    DECL_OPTION("-binary", CbCall, [एतत्]() {
        m_द्विमान = सत्य;
        m_build = सत्य;
        m_exe = सत्य;
        m_main = सत्य;
        यदि (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", समुच्चयः_2, &m_build);
    DECL_OPTION("-build-dep-bin", समुच्चयः_2, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        पूर्णाङ्क val = std::atoi(valp);
        यदि (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } अन्यथा यदि (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::सङ्कलय_CFlags));
    DECL_OPTION("-cc", CbCall, [एतत्]() { cc_समुच्चयः(); });
    DECL_OPTION("-clk", CbVal, [fl](ध्रुव std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ध्रुव std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", समुच्चयः_2, &m_comp_सीमा_खण्डानि).undocumented();
    DECL_OPTION("-comp-limit-members", समुच्चयः_2,
                &m_comp_सीमा_Members)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", समुच्चयः_2, &m_comp_सीमा_Parens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](पूर्णाङ्क val) { VName::max_दीर्घता(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        यदि (!std::वाक्यं_तुलय(valp, "clang")) {
            m_comp_सीमा_खण्डानि = 80;  // limit unknown
            m_comp_सीमा_Members = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_comp_सीमा_Parens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "gcc")) {
            m_comp_सीमा_खण्डानि = 0;  // Bug free
            m_comp_सीमा_Members = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_comp_सीमा_Parens = 240;  // Unlimited, but generate same code as for clang
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "msvc")) {
            m_comp_सीमा_खण्डानि = 80;  // 128, but allow some room
            m_comp_सीमा_Members = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_comp_सीमा_Parens = 80;  // 128, but allow some room
        } अन्यथा {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warn_अधिकम्() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::सङ्कलय_Compiler_Includes));
    DECL_OPTION("-converge-limit", समुच्चयः_2, &m_converge_सीमा);
    DECL_OPTION("-coverage", CbOnOff, [एतत्](तार्किक flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_व्यञ्जकम्);
    DECL_OPTION("-coverage-expr-max", समुच्चयः_2, &m_coverage_व्यञ्जकम्_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [एतत्, fl](ध्रुव वर्ण* ध्रुव valp) {
        यदि (!std::वाक्यं_तुलय(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::पूर्णम्;
        } अन्यथा {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warn_अधिकम्() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", समुच्चयः_2, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_पङ्क्तिः);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ध्रुव स्वचालित parse_Bins_सीमा = [fl](ध्रुव वर्ण* optp, ध्रुव वर्ण* valp, uint32_t& limitr) {
        वर्ण* endp = शून्यसूचकः;
        ध्रुव अचिह्नित दीर्घ दीर्घ मानम् = std::strtoull(valp, &endp, 10);
        यदि (*endp || मानम् < 1 || मानम् > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            प्रत्यागम;
        }
        limitr = static_cast<uint32_t>(मानम्);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [एतत्, parse_Bins_सीमा](ध्रुव वर्ण* valp) {
        parse_Bins_सीमा("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [एतत्, parse_Bins_सीमा](ध्रुव वर्ण* valp) {
        parse_Bins_सीमा("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", समुच्चयः_2, &m_coverage_Max_विस्तारः);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverageMergeInstances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coverage_प्रत्येकम्_Instance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* valp) VL_MT_DISABLED {
        सङ्कलय_Define(valp, असत्य);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [एतत्]() { समुच्चयः_Debug_बहुलकम्(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [एतत्](पूर्णाङ्क v) { समुच्चयः_Debug_बहुलकम्(v); }).notForRerun();
    DECL_OPTION("-debugi-", Cb_Partial_मेलनम्_Val, [एतत्](ध्रुव वर्ण* optp, ध्रुव वर्ण* valp) {
        m_debugLevel[optp] = std::atoi(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#यदि_परिभाषितम् VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", समुच्चयः_2, &m_debugAllocRandom).undocumented();
#यदि_अन्तः
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
    DECL_OPTION("-debug-runtime-timeout", समुच्चयः_2, &m_debugRuntimeTimeout).undocumented();
    DECL_OPTION("-debug-self-test", OnOff, &m_debugSelfTest).undocumented();
    DECL_OPTION("-debug-sigsegv", CbCall, throwSigsegv).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_राशिः_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debug_विस्तारः).undocumented();
    DECL_OPTION("-decoration", CbCall, [एतत्, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [एतत्, fl](ध्रुव वर्ण* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [एतत्, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [एतत्](ध्रुव वर्ण* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = सत्य;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", Cb_Partial_मेलनम्_Val, [एतत्](ध्रुव वर्ण* optp, ध्रुव वर्ण* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [एतत्](तार्किक flag) {
        यदि (flag) {
            m_stdPackage = असत्य;
            m_stdWaiver = असत्य;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<शून्य (*)(पूर्णाङ्क)>(&V3Error::error_सीमा));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [एतत्](ध्रुव वर्ण* valp) { m_expand_सीमा = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [एतत्, fl, &optdir](ध्रुव वर्ण* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), सत्य);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [एतत्, &optdir](ध्रुव वर्ण* valp) { सङ्कलय_Force_Inc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [एतत्, fl, &optdir](ध्रुव वर्ण* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), असत्य);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [एतत्](ध्रुव वर्ण* valp) { सङ्कलय_Future0(valp); });
    DECL_OPTION("-future1", CbVal, [एतत्](ध्रुव वर्ण* valp) { सङ्कलय_Future1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_f_Bit_अवलोकनम्_Loops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [एतत्](तार्किक flag) {
        m_fCaseDecoder = flag;
        m_f_Case_सारणी = flag;
        m_f_Case_वृक्षः = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_f_Case_सारणी);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_वृक्षः);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_Bit_Op_वृक्षः);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [एतत्](तार्किक flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [एतत्](तार्किक flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](तार्किक) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](तार्किक) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](तार्किक) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_प्रक्षिप_अधस्तात्_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [एतत्, fl](तार्किक flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [एतत्](तार्किक flag) {  //
        m_f_Func_विभज_Cat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_f_Func_विभज_Cat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [एतत्](तार्किक flag) {  //
        m_fIcoChangeDetect.समुच्चयः_True_Or_False(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](तार्किक) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_fMergeCond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_fMergeCondMotion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](तार्किक) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_f_खण्डिका);
    DECL_OPTION("-fslice-element-limit", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_f_खण्डिका_Element_सीमा = std::atoi(valp);
        यदि (m_f_खण्डिका_Element_सीमा < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_f_विभज);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_f_सारणी);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](तार्किक) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) { सङ्कलय_Parameter(optp, असत्य); });
    DECL_OPTION("-gate-stmts", समुच्चयः_2, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [एतत्]() {
        निर्गमः << protect_कुञ्चिका_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ध्रुव वर्ण* valp) {
        निर्गमः << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ध्रुव वर्ण* valp) {
        निर्गमः << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_श्रेणीबद्धम्);
    DECL_OPTION("-hierarchical-block", CbVal, [एतत्](ध्रुव वर्ण* valp) {
        ध्रुव V3_श्रेणीबद्धम्_खण्डम्_Option opt{valp};
        m_hier_खण्डानि.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", समुच्चयः_2, &m_hierChild);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [एतत्](ध्रुव वर्ण* optp) { m_hierParamsFile.प्रक्षिप_पृष्ठभागः({optp, work()}); });

    DECL_OPTION("-I", Cb_Partial_मेलनम्, [एतत्, &optdir](ध्रुव वर्ण* optp) {
        सङ्कलय_Inc_Dir_User(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", समुच्चयः_2, &m_if_गभीरता);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", समुच्चयः_2, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", समुच्चयः_2, &m_inline_CFuncs_गुणनफलम्);
    DECL_OPTION("-inline-mult", समुच्चयः_2, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [एतत्, fl](पूर्णाङ्क val) {
        m_instr_गणनम्_Dpi = val;
        यदि (m_instr_गणनम्_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [एतत्](ध्रुव वर्ण* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = सत्य;
    });
    DECL_OPTION("-json-only-output", CbVal, [एतत्](ध्रुव वर्ण* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = सत्य;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::सङ्कलय_Ld_Libs));
    DECL_OPTION("-l2-name", समुच्चयः_2, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [एतत्]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [एतत्]() { m_l2Name = "v"; }).undocumented();  // Historical
    ध्रुव स्वचालित समुच्चयः_Lang = [एतत्, fl](ध्रुव वर्ण* valp) {
        ध्रुव V3_Lang_संहिता optval{valp};
        यदि (optval.legal()) {
            m_defaultLanguage = optval;
        } अन्यथा {
            VSpellCheck spell;
            प्रति (पूर्णाङ्क i = V3_Lang_संहिता::L_ERROR + 1; i < V3_Lang_संहिता::ENUM_अन्तः; ++i) {
                spell.प्रक्षिप_Candidate(V3_Lang_संहिता{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.best_Candidate_सन्देशः(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, समुच्चयः_Lang).notForRerun();
    DECL_OPTION("-language", CbVal, समुच्चयः_Lang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [एतत्](ध्रुव वर्ण* valp) { सङ्कलय_Lib_Map_File(valp); });
    DECL_OPTION("-lib-create", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", समुच्चयः_2, &m_localize_Max_आकार);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::सङ्कलय_Make_Flags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [एतत्](ध्रुव वर्ण* valp) {
        m_makeDir = valp;
        सङ्कलय_Inc_Dir_Fallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", समुच्चयः_2, &m_main_शिखरम्_Name);
    DECL_OPTION("-make", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        यदि (!std::वाक्यं_तुलय(valp, "gmake")) {
            m_gmake = सत्य;
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "json")) {
            m_makeJson = सत्य;
        } अन्यथा {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", समुच्चयः_2, &m_funcRecursion);
    DECL_OPTION("-max-num-width", समुच्चयः_2, &m_max_अङ्कः_विस्तारः);
    DECL_OPTION("-mod-prefix", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_mod_उपसर्गः = valp;
    });

    DECL_OPTION("-O0", CbCall, [एतत्]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [एतत्]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [एतत्]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [एतत्]() { optimize(3); });

    DECL_OPTION("-o", समुच्चयः_2, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](तार्किक /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_outputGroups = std::atoi(valp);
        यदि (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_output_Groups_आकार = std::atoll(valp);
        यदि (m_output_Groups_आकार < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", समुच्चयः_2, &m_output_विभज);
    DECL_OPTION("-output-split-cfuncs", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_output_विभज_CFuncs = std::atoi(valp);
        यदि (m_output_विभज_CFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_output_विभज_CTrace = std::atoi(valp);
        यदि (m_output_विभज_CTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", समुच्चयः_2, &m_preproc_No_पङ्क्तिः);
    DECL_OPTION("-pins64", CbCall, [एतत्]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [एतत्]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_pinsBv = std::atoi(valp);
        यदि (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [एतत्](तार्किक flag) {
        m_pinsScUint = flag;
        यदि (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [एतत्](तार्किक flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [एतत्](तार्किक flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", समुच्चयः_2, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_उपसर्गः = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preproc_परिभाषते);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_preproc_Token_सीमा = std::atoi(valp);
        यदि (m_preproc_Token_सीमा <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [एतत्]() { m_लौकिकम् = असत्य; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [एतत्]() { m_profC = m_profCFuncs = सत्य; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [एतत्]() {
        m_profC = m_profCFuncs = सत्य;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", समुच्चयः_2, &m_protect_कुञ्चिका);
    DECL_OPTION("-protect-lib", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = सत्य;
    });
    DECL_OPTION("-public", OnOff, &m_लौकिकम्);
    DECL_OPTION("-public-depth", समुच्चयः_2, &m_लौकिकम्_गभीरता);
    DECL_OPTION("-public-flat-rw", CbOnOff, [एतत्](तार्किक flag) {
        m_लौकिकम्_Flat_RW = flag;
        v3Global.dpi(सत्य);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [एतत्](तार्किक flag) { m_लौकिकम्_Ignore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [एतत्](तार्किक flag) {
        m_लौकिकम्_Params = flag;
        v3Global.dpi(सत्य);
    });
    DECL_OPTION("-pvalue+", Cb_Partial_मेलनम्,
                [एतत्](ध्रुव वर्ण* varp) { सङ्कलय_Parameter(varp, असत्य); });

    DECL_OPTION("-quiet", CbOnOff, [एतत्](तार्किक flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_reloop_सीमा = std::atoi(valp);
        यदि (m_reloop_सीमा < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_replication_सीमा = std::atoi(valp);
        यदि (m_replication_सीमा < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [एतत्, fl]() {
        decorations(fl, "node");
        सङ्कलय_CFlags("-ggdb");
        सङ्कलय_Ld_Libs("-ggdb");
        सङ्कलय_CFlags("-fsanitize=address,undefined");
        सङ्कलय_Ld_Libs("-fsanitize=address,undefined");
        सङ्कलय_CFlags("-D_GLIBCXX_DEBUG");
        सङ्कलय_CFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [एतत्]() {
        m_बहिर्गमनम्_Format_Ok = सत्य;
        m_systemC = सत्य;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [एतत्](तार्किक flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [एतत्](तार्किक flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [एतत्]() { m_defaultLanguage = V3_Lang_संहिता::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [एतत्, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_तन्तवः = 1;
    });
    DECL_OPTION("-threads", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_तन्तवः = std::atoi(valp);
        यदि (m_तन्तवः < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        यदि (m_तन्तवः == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_तन्तवः = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_hier_तन्तवः = std::atoi(valp);
        यदि (m_hier_तन्तवः < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_तन्तवः_Coarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        यदि (!std::वाक्यं_तुलय(valp, "all")) {
            m_तन्तवः_Dpi_विशुद्धम् = सत्य;
            m_तन्तवः_Dpi_Unpure = सत्य;
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "none")) {
            m_तन्तवः_Dpi_विशुद्धम् = असत्य;
            m_तन्तवः_Dpi_Unpure = असत्य;
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "pure")) {
            m_तन्तवः_Dpi_विशुद्धम् = सत्य;
            m_तन्तवः_Dpi_Unpure = असत्य;
        } अन्यथा {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warn_अधिकम्() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        m_तन्तवः_Max_MTasks = std::atoi(valp);
        यदि (m_तन्तवः_Max_MTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        यदि (!unit.isNone() && timeOverrideUnit().isNone()) m_timeDefaultUnit = unit;
        यदि (!prec.isNone() && timeOverridePrec().isNone()) m_timeDefaultPrec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, सत्य);
        यदि (!unit.isNone()) {
            m_timeDefaultUnit = unit;
            m_timeOverrideUnit = unit;
        }
        यदि (!prec.isNone()) {
            m_timeDefaultPrec = prec;
            m_timeOverridePrec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [एतत्](ध्रुव std::string& flag) { m_शिखरम्_Module = Ast_ग्रन्थिः::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [एतत्](ध्रुव std::string& flag) { m_शिखरम्_Module = Ast_ग्रन्थिः::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", समुच्चयः_2, &m_शिखरम्_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [एतत्]() { m_traceEnabledSaif = सत्य; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", समुच्चयः_2, &m_trace_गभीरता);
    DECL_OPTION("-trace-fst", CbCall, [एतत्]() {
        m_traceEnabledFst = सत्य;
        सङ्कलय_Ld_Libs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", समुच्चयः_2, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", समुच्चयः_2, &m_trace_Max_विस्तारः);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ध्रुव वर्ण*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", समुच्चयः_2, &m_no_Trace_शिखरम्);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [एतत्]() { m_traceEnabledVcd = सत्य; });

    DECL_OPTION("-U", Cb_Partial_मेलनम्, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", समुच्चयः_2, &m_constraint_Array_सीमा);
    DECL_OPTION("-unroll-count", समुच्चयः_2, &m_unroll_गणनम्).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", समुच्चयः_2, &m_unroll_सीमा);
    DECL_OPTION("-unroll-stmts", समुच्चयः_2, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", समुच्चयः_2, &m_अप्रयुक्तम्_Regexp);

    DECL_OPTION("-V", CbCall, [एतत्]() {
        show_संस्करणम्(सत्य);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [एतत्, &optdir](ध्रुव वर्ण* valp) {
        V3Options::सङ्कलय_Library_File(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        पूर्णाङ्क val = std::atoi(valp);
        यदि (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } अन्यथा यदि (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [एतत्]() {
        show_संस्करणम्(असत्य);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_पङ्क्तिः::globalWarnOff(V3_Error_संहिता::I_LINT, असत्य); });
    DECL_OPTION("-Werror-", Cb_Partial_मेलनम्, [एतत्, fl](ध्रुव वर्ण* optp) {
        ध्रुव V3_Error_संहिता संहिता{optp};
        यदि (संहिता == V3_Error_संहिता::EC_ERROR) {
            यदि (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } अन्यथा {
            V3Error::pretendError(संहिता, सत्य);
        }
    });
    DECL_OPTION("-Wfuture-", Cb_Partial_मेलनम्, [एतत्](ध्रुव वर्ण* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        सङ्कलय_Future(optp);
    });
    DECL_OPTION("-Wno-", Cb_Partial_मेलनम्, [fl, &parser](ध्रुव वर्ण* optp) VL_MT_DISABLED {
        ध्रुव string err = File_पङ्क्तिः::globalWarnOffParse(optp, सत्य);
        यदि (!err.रिक्तानि()) {
            ध्रुव string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    प्रति (पूर्णाङ्क i = V3_Error_संहिता::EC_FIRST_WARN; i < V3_Error_संहिता::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        प्रति (ध्रुव string उपसर्गः : {"-Wno-", "-Wwarn-"})
            parser.सङ्कलय_Suggestion_Candidate(उपसर्गः + V3_Error_संहिता{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [एतत्]() { m_context = असत्य; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(असत्य); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_पङ्क्तिः::globalWarnOff(V3_Error_संहिता::I_LINT, सत्य); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_पङ्क्तिः::globalWarnOff(V3_Error_संहिता::I_STYLE, सत्य); });
    DECL_OPTION("-work", समुच्चयः_2, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [एतत्]() {
        m_pedantic = सत्य;
        V3Error::pretendError(V3_Error_संहिता::ASSIGNIN, असत्य);
    });
    DECL_OPTION("-Wwarn-", Cb_Partial_मेलनम्, [एतत्, fl, &parser](ध्रुव वर्ण* optp) VL_MT_DISABLED {
        ध्रुव V3_Error_संहिता संहिता{optp};
        यदि (संहिता == V3_Error_संहिता::EC_ERROR) {
            यदि (!isFuture(optp)) {
                ध्रुव string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } अन्यथा {
            File_पङ्क्तिः::globalWarnOff(संहिता, असत्य);
            V3Error::pretendError(संहिता, असत्य);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_पङ्क्तिः::globalWarnOff(V3_Error_संहिता::I_LINT, असत्य); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_पङ्क्तिः::globalWarnOff(V3_Error_संहिता::I_STYLE, असत्य); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", समुच्चयः_2, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        यदि (!std::वाक्यं_तुलय(valp, "0")) {
            m_x_नियोजय = "0";
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "1")) {
            m_x_नियोजय = "1";
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "fast")) {
            m_x_नियोजय = "fast";
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "unique")) {
            m_x_नियोजय = "unique";
        } अन्यथा {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warn_अधिकम्() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [एतत्, fl](ध्रुव वर्ण* valp) {
        यदि (!std::वाक्यं_तुलय(valp, "0")) {
            m_xInitial = "0";
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "fast")) {
            m_xInitial = "fast";
        } अन्यथा यदि (!std::वाक्यं_तुलय(valp, "unique")) {
            m_xInitial = "unique";
        } अन्यथा {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warn_अधिकम्() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_xInitialEdge);

    DECL_OPTION("-y", CbVal, [एतत्, &optdir](ध्रुव वर्ण* valp) {
        सङ्कलय_Inc_Dir_User(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    प्रति (पूर्णाङ्क i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        यदि (argv[i][0] == '-' || argv[i][0] == '+') {
            ध्रुव std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            पूर्णाङ्क consumed = 0;
            तार्किक isForRerun = सत्य;

            // Special case for -j which has an optional argument
            यदि (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                पूर्णाङ्क val = 0;
                यदि (i + 1 < argc && std::अङ्कः_वा(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    यदि (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                यदि (m_buildJobs == -1) m_buildJobs = val;
                यदि (m_verilateJobs == -1) m_verilateJobs = val;
                यदि (m_outputGroups == -1) m_outputGroups = val;
            } अन्यथा {
                ध्रुव स्वचालित pair = parser.parse(i, argc, argv);
                consumed = pair.first;
                isForRerun = pair.second;
                यदि (consumed) {
                    // Already processed
                } अन्यथा यदि (isFuture0(argName)) {
                    consumed = 1;
                } अन्यथा यदि (isFuture1(argName)) {
                    consumed = 2;
                } अन्यथा {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            यदि (argName != "f" && argName != "F") सङ्कलय_Arg(argv + i, consumed, isForRerun);
            i += consumed;
            अनुवर्त;
        }

        // Filename
        ध्रुव std::string filename = parseFileArg(optdir, argv[i]);
        तार्किक isForRerun = सत्य;
        यदि (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::सङ्कलय_Cpp_File(filename);
        } अन्यथा यदि (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::सङ्कलय_Ld_Lib_File(filename);
        } अन्यथा यदि (suffixed(filename, ".vlt")) {
            V3Options::सङ्कलय_Vlt_File(filename, work());
            isForRerun = असत्य;
        } अन्यथा {
            V3Options::सङ्कलय_VFile(filename, work());
            isForRerun = असत्य;
        }
        सङ्कलय_Arg(filename, isForRerun);
        ++i;
    }

    यदि (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

शून्य V3Options::parseOptsFile(File_पङ्क्तिः* fl, ध्रुव string& filename, तार्किक rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ध्रुव std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    यदि (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        प्रत्यागम;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    तार्किक has_fargs = असत्य;
    std::string सम्पूर्णम्_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ध्रुव स्वचालित parse_fargs_cmt = [&](ध्रुव वर्ण* p) -> शून्य {
        ध्रुव तार्किक खण्डम्_Cmt = p[1] == '*';
        p += 2;
        यावत् (std::रिक्तम्_वा(*p)) ++p;
        यदि (VString::startsWith(p, "verilator")) {
            p += std::वाक्यदैर्घ्यम्("verilator");
            यदि (std::रिक्तम्_वा(*p)) {
                यावत् (std::रिक्तम्_वा(*p)) ++p;
                यदि (VString::startsWith(p, "fargs")) {
                    p += std::वाक्यदैर्घ्यम्("fargs");
                    यदि (std::रिक्तम्_वा(*p)) {
                        यावत् (std::रिक्तम्_वा(*p)) ++p;
                        यदि (!has_fargs) {
                            has_fargs = सत्य;
                            सम्पूर्णम्_file.clear();
                        }
                        सम्पूर्णम्_file += p;
                        यदि (खण्डम्_Cmt) {
                            ध्रुव std::string::आकार_type pos = सम्पूर्णम्_file.find("*/");
                            यदि (pos == std::string::npos) {
                                fl->v3fatal("/* verilator fargs */ must be on single line");
                            }
                            सम्पूर्णम्_file.resize(pos);
                        }
                        सम्पूर्णम्_file += ' ';
                    }
                }
            }
        }
    };

    तार्किक अन्तर्_Cmt = असत्य;
    यावत् (!ifp->eof()) {
        ध्रुव std::string पङ्क्तिः = V3Os::getline(*ifp);
        // Strip simple comments
        वर्ण lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        तार्किक space_begin = सत्य;  // At beginning or leading spaces only
        प्रति (ध्रुव वर्ण* p = पङ्क्तिः.c_str(); *p; lastch = *p++) {
            यदि (अन्तर्_Cmt) {
                यदि (p[0] == '*' && p[1] == '/') {
                    अन्तर्_Cmt = असत्य;
                    ++p;
                }
            } अन्यथा यदि (p[0] == '/' && p[1] == '/' && std::रिक्तम्_वा(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                भङ्ग;  // Ignore to EOL
            } अन्यथा यदि (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                भङ्ग;  // Ignore to EOL
            } अन्यथा यदि (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                अन्तर्_Cmt = सत्य;
                space_begin = असत्य;
                ++p;
            } अन्यथा {
                यदि (!std::रिक्तम्_वा(*p)) space_begin = असत्य;
                यदि (!has_fargs) सम्पूर्णम्_file += *p;
            }
        }
        यदि (!has_fargs) सम्पूर्णम्_file += " ";
    }
    सम्पूर्णम्_file += "\n";  // So string match below is simplified
    यदि (अन्तर्_Cmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = नूतन File_पङ्क्तिः{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::सदिशम्<string> तर्काः;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    गणना state : uint8_t {
        ST_अन्तर्_OPTION,
        ST_ESCAPED_CHAR,
        ST_अन्तर्_QUOTED_STR,
        ST_अन्तर्_DOUBLE_QUOTED_STR
    };

    state st = ST_अन्तर्_OPTION;
    state चरमः_st = ST_अन्तर्_OPTION;
    string arg;
    प्रति (string::आकार_type pos = 0; pos < सम्पूर्णम्_file.दीर्घता_2(); ++pos) {
        वर्ण curr_char = सम्पूर्णम्_file[pos];
        विकल्प (st) {
        पक्ष ST_अन्तर्_OPTION:  // Get all chars up to a white space or a "="
            यदि (std::रिक्तम्_वा(curr_char)) {  // End of option
                यदि (!arg.रिक्तानि()) {  // End of word
                    तर्काः.प्रक्षिप_पृष्ठभागः(arg);
                }
                arg = "";
                भङ्ग;
            }
            यदि (curr_char == '\\') {  // Escape char, we wait for next char
                चरमः_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                भङ्ग;
            }
            यदि (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                यदि (pos < सम्पूर्णम्_file.दीर्घता_2()) curr_char = सम्पूर्णम्_file[pos];
                यदि (curr_char == '"') {  // String
                    st = ST_अन्तर्_QUOTED_STR;
                } अन्यथा {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                भङ्ग;
            }
            यदि (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_अन्तर्_DOUBLE_QUOTED_STR;
                भङ्ग;
            }
            arg += curr_char;
            भङ्ग;
        पक्ष ST_अन्तर्_QUOTED_STR:  // Just store all chars inside string
            यदि (curr_char != '\'') {
                arg += curr_char;
            } अन्यथा {  // End of quoted string
                st = ST_अन्तर्_OPTION;
            }
            भङ्ग;
        पक्ष ST_अन्तर्_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            यदि (curr_char != '"') {
                यदि (curr_char == '\\') {
                    चरमः_st = st;
                    st = ST_ESCAPED_CHAR;
                } अन्यथा {
                    arg += curr_char;
                }
            } अन्यथा {  // End of double quoted string
                st = ST_अन्तर्_OPTION;
            }
            भङ्ग;
        पक्ष ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = चरमः_st;
            भङ्ग;
        }
    }
    यदि (!arg.रिक्तानि()) {  // Add last word
        तर्काः.प्रक्षिप_पृष्ठभागः(arg);
    }

    //मार्गः
    ध्रुव string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::सदिशम्<वर्ण*> argv;
    argv.reserve(तर्काः.आकार() + 1);
    प्रति (ध्रुव string& i : तर्काः) argv.प्रक्षिप_पृष्ठभागः(const_cast<वर्ण*>(i.c_str()));
    argv.प्रक्षिप_पृष्ठभागः(शून्यसूचकः);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<पूर्णाङ्क>(argv.आकार() - 1), argv.दत्त());
}

//======================================================================

string V3Options::parseFileArg(ध्रुव string& optdir, ध्रुव string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    यदि (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    प्रत्यागम filename;
}

//======================================================================

शून्य V3Options::show_संस्करणम्(तार्किक verbose) {
    निर्गमः << संस्करणम्();
    निर्गमः << "\n";
    यदि (!verbose) प्रत्यागम;

    निर्गमः << "\n";
    निर्गमः << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    निर्गमः << "redistribute it and/or modify the Verilator internals under the terms of\n";
    निर्गमः << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    निर्गमः << "License Version 2.0.\n";

    निर्गमः << "\n";
    निर्गमः << "See https://verilator.org for documentation\n";

    निर्गमः << "\n";
    निर्गमः << "Summary of configuration:\n";
    निर्गमः << "  Compiled in defaults if not in environment:\n";
    निर्गमः << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    निर्गमः << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    निर्गमः << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    निर्गमः << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    निर्गमः << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_मूलम् << "\n";
    निर्गमः << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    निर्गमः << "    SystemC system-wide = " << cvt_गन्तव्यम्_Str(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    निर्गमः << "\n";
    निर्गमः << "Environment:\n";
    निर्गमः << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    निर्गमः << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    निर्गमः << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    निर्गमः << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    निर्गमः << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    निर्गमः << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    निर्गमः << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    निर्गमः << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    निर्गमः << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    निर्गमः << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    निर्गमः << "\n";
    निर्गमः << "Supported features (compiled-in or forced by environment):\n";
    निर्गमः << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    निर्गमः << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    निर्गमः << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = नूतन V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_अप्रयुक्तम्_Regexp = "*unused*";
    m_x_नियोजय = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3_Lang_संहिता::mostRecent();

    VName::max_दीर्घता(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    सङ्कलय_Lib_Ext_V("");  // So include "filename.v" will find the same file
    सङ्कलय_Lib_Ext_V(".v");
    सङ्कलय_Lib_Ext_V(".sv");
    // Default -I
    सङ्कलय_Inc_Dir_Fallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(विलोपय m_impp, m_impp = शून्यसूचकः); }

शून्य V3Options::समुच्चयः_Debug_बहुलकम्(पूर्णाङ्क level) {
    V3Error::debugDefault(level);
    यदि (!m_dumpLevel.गणनम्("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = सत्य;
    m_debugCheck = सत्य;
    यदि (level) {
        निर्गमः << "- Starting " << संस्करणम्() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

अचिह्नित V3Options::debugLevel(ध्रुव string& चिह्नकम्) ध्रुव VL_MT_SAFE {
    ध्रुव स्वचालित iter = m_debugLevel.find(चिह्नकम्);
    प्रत्यागम iter != m_debugLevel.अन्तः_2() ? iter->second : V3Error::debugDefault();
}

अचिह्नित V3Options::debugSrcLevel(ध्रुव string& srcfile_मार्गः) ध्रुव VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    प्रत्यागम debugLevel(V3Os::filenameNonDirExt(srcfile_मार्गः));
}

अचिह्नित V3Options::dumpLevel(ध्रुव string& चिह्नकम्) ध्रुव VL_MT_SAFE {
    ध्रुव स्वचालित iter = m_dumpLevel.find(चिह्नकम्);
    प्रत्यागम iter != m_dumpLevel.अन्तः_2() ? iter->second : 0;
}

अचिह्नित V3Options::dumpSrcLevel(ध्रुव string& srcfile_मार्गः) ध्रुव VL_MT_SAFE {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    प्रत्यागम dumpLevel(V3Os::filenameNonDirExt(srcfile_मार्गः));
}

तार्किक V3Options::dump_वृक्षः_Addrids() ध्रुव VL_MT_SAFE {
    स्थिर पूर्णाङ्क s_level = -1;
    यदि (VL_UNLIKELY(s_level < 0)) {
        ध्रुव अचिह्नित मानम् = dumpLevel("tree-addrids");
        यदि (!उपलब्धम्()) प्रत्यागम मानम् > 0;
        s_level = static_cast<अचिह्नित>(मानम्);
    }
    प्रत्यागम s_level > 0;
}

शून्य V3Options::optimize(पूर्णाङ्क level) {
    // Set all optimizations to on/off
    ध्रुव तार्किक flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_f_Bit_अवलोकनम्_Loops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_f_Case_सारणी = flag;
    m_f_Case_वृक्षः = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_Bit_Op_वृक्षः = flag;
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
    m_f_विभज = flag;
    m_fSubst = flag;
    m_fSubstConst = flag;
    m_f_सारणी = flag;
    // And set specific optimization levels
    यदि (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
