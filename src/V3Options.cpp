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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3Options.h"

#అన్తర్భావ "V3Container.h"
#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3File.h"
#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Mutex.h"
#అన్తర్భావ "V3OptionParser.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3PreShell.h"
#అన్తర్భావ "V3String.h"

// clang-format off
#అన్తర్భావ <sys/types.h>
#అన్తర్భావ <sys/stat.h>
#నిర్వచితం_కాకపోతే _WIN32
# include <sys/utsname.h>
#అయితే_ముగింపు
#అన్తర్భావ <algorithm>
#అన్తర్భావ <cctype>
#నిర్వచితమైతే _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(బహుళకం) (((బహుళకం) & _S_IFMT) == _S_IFDIR)
#లేకపోతే
# include <dirent.h>
#అయితే_ముగింపు
#అన్తర్భావ <fcntl.h>
#అన్తర్భావ <list>
#అన్తర్భావ <map>
#అన్తర్భావ <memory>
#అన్తర్భావ <thread>
#అన్తర్భావ <set>
#అన్తర్భావ <string>

#అన్తర్భావ "config_rev.h"

#ఒకవేళ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#అయితే_ముగింపు
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

తరగతి V3OptionsImp final {
బహిరంగ:
    // TYPES
    ఉపయోగించి DirMap = std::map<స్థిరరాశి string, std::సమితి<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_పంక్తి_వాదనలు;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, తార్కిక>> m_all_వాదనలు;
    VInsertion_సమితి<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::సమితి<string> m_inc_Dir_Fallback_సమితి;  // Include directories (for removing duplicates)
    std::map<స్థిరరాశి string, V3_Lang_సంకేతం> m_langExts;  // Language extension map
    VInsertion_సమితి<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    శూన్య కూడు_Inc_Dir_User(స్థిరరాశి string& incdir) {
        స్థిరరాశి string& dir = V3Os::filenameCleanup(incdir);
        స్థిరరాశి తార్కిక inserted = m_incDirUsers.చొప్పించు(dir);
        ఒకవేళ (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_inc_Dir_Fallback_సమితి.erase(dir);  // User has priority over Fallback
        }
    }
    శూన్య కూడు_Inc_Dir_Fallback(స్థిరరాశి string& incdir) {
        స్థిరరాశి string& dir = V3Os::filenameCleanup(incdir);
        ఒకవేళ (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            స్థిరరాశి స్వయంచాలక it_దొరికింది_Pair = m_inc_Dir_Fallback_సమితి.చొప్పించు(dir);
            ఒకవేళ (it_దొరికింది_Pair.రెండవ) m_incDirFallbacks.నెట్టు_వెనుక(dir);
        }
    }
    శూన్య కూడు_Lang_Ext(స్థిరరాశి string& langext, స్థిరరాశి V3_Lang_సంకేతం& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        ఒకవేళ (addext[0] == '.') addext = addext.substr(1);
        (శూన్య)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    శూన్య కూడు_Lib_Ext_V(స్థిరరాశి string& libext) { m_libExtVs.చొప్పించు(libext); }
    V3OptionsImp() = అప్రమేయ;
    ~V3OptionsImp() = అప్రమేయ;
};

//######################################################################
// V3LangCode class functions

V3_Lang_సంకేతం::V3_Lang_సంకేతం(స్థిరరాశి అక్షరం* textp) {
    // Return code for given string, or ERROR, which is a bad code
    కొరకు (పూర్ణాంకం codei = V3_Lang_సంకేతం::L_ERROR; codei < V3_Lang_సంకేతం::ENUM_చివర; ++codei) {
        స్థిరరాశి V3_Lang_సంకేతం సంకేతం_2{codei};
        ఒకవేళ (0 == VL_STRCASECMP(textp, సంకేతం_2.ascii())) {
            m_e = సంకేతం_2;
            తిరిగి;
        }
    }
    m_e = V3_Lang_సంకేతం::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(స్థిరరాశి string& విలువ, తార్కిక& badr)
    : m_e{VTimescale::NONE} {
    badr = నిజం;
    స్థిరరాశి string spaceless = VString::removeWhitespace(విలువ);
    కొరకు (పూర్ణాంకం i = TS_100S; i < ENUM_చివర; ++i) {
        స్థిరరాశి VTimescale ts{i};
        ఒకవేళ (spaceless == ts.ascii()) {
            badr = అబద్ధం;
            m_e = ts.m_e;
            ఆపు;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3_క్రమానుగత_ఖండం_Option::V3_క్రమానుగత_ఖండం_Option(స్థిరరాశి string& opts) {
    VStringList vals;
    తార్కిక లోపల_Str = అబద్ధం;
    string cur;
    స్థిర స్థిరరాశి string hier_ఖండం("--hierarchical-block");
    File_పంక్తి cmdfl(File_పంక్తి::command_పంక్తి_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    కొరకు (string::const_iterator it = opts.begin(); it != opts.చివర();) {
        ఒకవేళ (లోపల_Str) {
            ఒకవేళ (*it == '\\') {
                ++it;
                ఒకవేళ (it == opts.చివర()) {
                    cmdfl.v3error(hier_ఖండం + " must not end with \\");
                    ఆపు;
                }
                ఒకవేళ (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hier_ఖండం + " does not allow '" + *it + "' after \\");
                    ఆపు;
                }
                cur.నెట్టు_వెనుక(*it);
                ++it;
            } లేకపోతే ఒకవేళ (*it == '"') {  // end of string
                cur.నెట్టు_వెనుక(*it);
                vals.నెట్టు_వెనుక(cur);
                cur.clear();
                ++it;
                ఒకవేళ (it != opts.చివర()) {
                    ఒకవేళ (*it != ',') {
                        cmdfl.v3error(hier_ఖండం + " expects ',', but '" + *it + "' is passed");
                        ఆపు;
                    }
                    ++it;
                    ఒకవేళ (it == opts.చివర()) {
                        cmdfl.v3error(hier_ఖండం + " must not end with ','");
                        ఆపు;
                    }
                    లోపల_Str = *it == '"';
                    cur.నెట్టు_వెనుక(*it);
                    ++it;
                }
            } లేకపోతే {
                cur.నెట్టు_వెనుక(*it);
                ++it;
            }
        } లేకపోతే {
            ఒకవేళ (*it == '"') {
                cmdfl.v3error(hier_ఖండం + " does not allow '\"' in the middle of literal");
                ఆపు;
            }
            ఒకవేళ (*it == ',') {  // end of this parameter
                vals.నెట్టు_వెనుక(cur);
                cur.clear();
                ++it;
                ఒకవేళ (it == opts.చివర()) {
                    cmdfl.v3error(hier_ఖండం + " must not end with ','");
                    ఆపు;
                }
                లోపల_Str = *it == '"';
            }
            cur.నెట్టు_వెనుక(*it);
            ++it;
        }
    }
    ఒకవేళ (!cur.ఖాళీలు()) vals.నెట్టు_వెనుక(cur);
    ఒకవేళ (vals.కొలత() >= 2) {
        ఒకవేళ (vals.కొలత() % 2) {
            cmdfl.v3error(hier_ఖండం + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } లేకపోతే {
        cmdfl.v3error(hier_ఖండం + " requires at least two comma-separated values");
    }
    కొరకు (కొలత_t i = 2; i + 1 < vals.కొలత(); i += 2) {
        స్థిరరాశి తార్కిక inserted = m_parameters.emplace(vals[i], vals[i + 1]).రెండవ;
        ఒకవేళ (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hier_ఖండం);
        }
    }
}

//######################################################################
// V3Options class functions

శూన్య VTimescale::parseSlashed(File_పంక్తి* fl, స్థిరరాశి అక్షరం* textp, VTimescale& unitr,
                              VTimescale& precr, తార్కిక allow_ఖాళీలు) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    స్థిరరాశి అక్షరం* cp = textp;
    కొరకు (; std::ఖాళీనా(*cp); ++cp) {}
    స్థిరరాశి అక్షరం* స్థిరరాశి unitp = cp;
    కొరకు (; *cp && *cp != '/'; ++cp) {}
    స్థిరరాశి string unitStr(unitp, cp - unitp);
    కొరకు (; std::ఖాళీనా(*cp); ++cp) {}
    string precStr;
    ఒకవేళ (*cp == '/') {
        ++cp;
        కొరకు (; std::ఖాళీనా(*cp); ++cp) {}
        స్థిరరాశి అక్షరం* స్థిరరాశి precp = cp;
        కొరకు (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    కొరకు (; std::ఖాళీనా(*cp); ++cp) {}
    ఒకవేళ (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        తిరిగి;
    }

    తార్కిక unitbad = అబద్ధం;
    స్థిరరాశి VTimescale unit{unitStr, unitbad /*ref*/};
    ఒకవేళ (unitbad && !(unitStr.ఖాళీలు() && allow_ఖాళీలు)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        తిరిగి;
    }
    unitr = unit;

    ఒకవేళ (!precStr.ఖాళీలు()) {
        VTimescale prec{VTimescale::NONE};
        తార్కిక precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        ఒకవేళ (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            తిరిగి;
        }
        ఒకవేళ (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            తిరిగి;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

శూన్య V3Options::కూడు_Inc_Dir_User(స్థిరరాశి string& incdir) { m_impp->కూడు_Inc_Dir_User(incdir); }
శూన్య V3Options::కూడు_Inc_Dir_Fallback(స్థిరరాశి string& incdir) { m_impp->కూడు_Inc_Dir_Fallback(incdir); }
శూన్య V3Options::కూడు_Lang_Ext(స్థిరరాశి string& langext, స్థిరరాశి V3_Lang_సంకేతం& lc) {
    m_impp->కూడు_Lang_Ext(langext, lc);
}
శూన్య V3Options::కూడు_Lib_Ext_V(స్థిరరాశి string& libext) { m_impp->కూడు_Lib_Ext_V(libext); }
శూన్య V3Options::కూడు_Define(స్థిరరాశి string& defline, తార్కిక allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ఎడమ = defline;
    అయితే (ఎడమ != "") {
        string def = ఎడమ;
        string::కొలత_type pos;
        ఒకవేళ (allowPlus && ((pos = ఎడమ.కనుగొను('+')) != string::npos)) {
            ఎడమ = ఎడమ.substr(pos + 1);
            def.erase(pos);
        } లేకపోతే {
            ఎడమ = "";
        }
        string విలువ;
        ఒకవేళ ((pos = def.కనుగొను('=')) != string::npos) {
            విలువ = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_పంక్తి(def, విలువ);
    }
}
శూన్య V3Options::కూడు_Parameter(స్థిరరాశి string& paramline, తార్కిక allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ఎడమ = paramline;
    అయితే (ఎడమ != "") {
        string param = ఎడమ;
        string::కొలత_type pos;
        ఒకవేళ (allowPlus && ((pos = ఎడమ.కనుగొను('+')) != string::npos)) {
            ఎడమ = ఎడమ.substr(pos + 1);
            param.erase(pos);
        } లేకపోతే {
            ఎడమ = "";
        }
        string విలువ;
        ఒకవేళ ((pos = param.కనుగొను('=')) != string::npos) {
            విలువ = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << విలువ);
        (శూన్య)m_parameters.erase(param);
        m_parameters[param] = విలువ;
    }
}

తార్కిక V3Options::hasParameter(స్థిరరాశి string& name) {
    తిరిగి m_parameters.కనుగొను(name) != m_parameters.చివర();
}

string V3Options::parameter(స్థిరరాశి string& name) {
    string విలువ = m_parameters.కనుగొను(name)->రెండవ;
    m_parameters.erase(m_parameters.కనుగొను(name));
    తిరిగి విలువ;
}

శూన్య V3Options::checkParameters() {
    ఒకవేళ (!m_parameters.ఖాళీలు()) {
        std::stringstream సందేశం_2;
        సందేశం_2 << "Parameters from the command line were not found in the design:";
        కొరకు (స్థిరరాశి స్వయంచాలక& i : m_parameters) సందేశం_2 << " " << i.మొదటి;
        v3error(సందేశం_2.str());
    }
}

శూన్య V3Options::కూడు_Cpp_File(స్థిరరాశి string& filename) { m_cppFiles.చొప్పించు(filename); }
శూన్య V3Options::కూడు_CFlags(స్థిరరాశి string& filename) { m_cFlags.నెట్టు_వెనుక(filename); }
శూన్య V3Options::కూడు_Lib_Map_File(స్థిరరాశి string& filename) { m_libmapFiles.చొప్పించు(filename); }
శూన్య V3Options::కూడు_Compiler_Includes(స్థిరరాశి string& filename) {
    m_compilerIncludes.చొప్పించు(filename);
}
శూన్య V3Options::కూడు_Ld_Lib_File(స్థిరరాశి string& filename) {
    m_ldLibFiles.నెట్టు_వెనుక(filename);
    కూడు_Ld_Libs(filename);
}
శూన్య V3Options::కూడు_Ld_Libs(స్థిరరాశి string& filename) { m_ldLibs.నెట్టు_వెనుక(filename); }
శూన్య V3Options::కూడు_Make_Flags(స్థిరరాశి string& filename) { m_makeFlags.నెట్టు_వెనుక(filename); }
శూన్య V3Options::కూడు_Future(స్థిరరాశి string& flag) { m_futures.చొప్పించు(flag); }
శూన్య V3Options::కూడు_Future0(స్థిరరాశి string& flag) { m_future0s.చొప్పించు(flag); }
శూన్య V3Options::కూడు_Future1(స్థిరరాశి string& flag) { m_future1s.చొప్పించు(flag); }
తార్కిక V3Options::isFuture(స్థిరరాశి string& flag) స్థిరరాశి {
    తిరిగి m_futures.కనుగొను(flag) != m_futures.చివర();
}
తార్కిక V3Options::isFuture0(స్థిరరాశి string& flag) స్థిరరాశి {
    తిరిగి m_future0s.కనుగొను(flag) != m_future0s.చివర();
}
తార్కిక V3Options::isFuture1(స్థిరరాశి string& flag) స్థిరరాశి {
    తిరిగి m_future1s.కనుగొను(flag) != m_future1s.చివర();
}
తార్కిక V3Options::isLibraryFile(స్థిరరాశి string& filename, స్థిరరాశి string& libname) స్థిరరాశి {
    తిరిగి m_libraryFiles.కనుగొను({filename, libname}) != m_libraryFiles.చివర();
}
శూన్య V3Options::కూడు_Library_File(స్థిరరాశి string& filename, స్థిరరాశి string& libname) {
    m_libraryFiles.చొప్పించు({filename, libname});
}
శూన్య V3Options::కూడు_VFile(స్థిరరాశి string& filename, స్థిరరాశి string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.నెట్టు_వెనుక({filename, libname});  // Not emplace
}
శూన్య V3Options::కూడు_Vlt_File(స్థిరరాశి string& filename, స్థిరరాశి string& libname) {
    m_vltFiles.చొప్పించు({filename, libname});
}
శూన్య V3Options::కూడు_Force_Inc(స్థిరరాశి string& filename) { m_forceIncs.నెట్టు_వెనుక(filename); }

శూన్య V3Options::కూడు_పంక్తి_Arg(స్థిరరాశి string& arg) { m_impp->m_పంక్తి_వాదనలు.నెట్టు_వెనుక(arg); }

శూన్య V3Options::కూడు_Arg(అక్షరం** argv, కొలత_t లెక్క, తార్కిక isForRerun) {
    m_impp->m_all_వాదనలు.emplace_వెనుక();
    std::pair<std::list<std::string>, తార్కిక>& pair = m_impp->m_all_వాదనలు.వెనుక();
    pair.రెండవ = isForRerun;
    కొరకు (కొలత_t n = 0; n < లెక్క; ++n) pair.మొదటి.emplace_వెనుక(argv[n]);
}

శూన్య V3Options::కూడు_Arg(స్థిరరాశి std::string& arg, తార్కిక isForRerun) {
    m_impp->m_all_వాదనలు.emplace_వెనుక();
    std::pair<std::list<std::string>, తార్కిక>& pair = m_impp->m_all_వాదనలు.వెనుక();
    pair.రెండవ = isForRerun;
    pair.మొదటి.emplace_వెనుక(arg);
}

స్థిరరాశి std::list<std::pair<std::list<std::string>, తార్కిక>>& V3Options::all_వాదనలు() స్థిరరాశి {
    తిరిగి m_impp->m_all_వాదనలు;
}

string V3Options::all_వాదనలు_String() స్థిరరాశి VL_MT_సురక్షిత {
    string ఫలితం;
    కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_impp->m_all_వాదనలు) {
        కొరకు (స్థిరరాశి string& arg : pair.మొదటి) {
            ఒకవేళ (!ఫలితం.ఖాళీలు()) ఫలితం += " ";
            ఫలితం += arg;
        }
    }
    తిరిగి ఫలితం;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::all_వాదనలు_String_For_Hier_ఖండం(తార్కిక for_శిఖరం) స్థిరరాశి {
    // cppcheck-suppress shadowFunction
    std::సమితి<string> vFiles;
    కొరకు (స్థిరరాశి స్వయంచాలక& vFile : m_vFiles) vFiles.చొప్పించు(vFile.filename());
    string బహిర్గతం;
    తార్కిక stripArg = అబద్ధం;
    తార్కిక strip_Arg_If_అంకె = అబద్ధం;
    కొరకు (స్థిరరాశి స్వయంచాలక& pair : m_impp->m_all_వాదనలు) {
        కొరకు (స్థిరరాశి string& arg : pair.మొదటి) {
            ఒకవేళ (stripArg) {
                stripArg = అబద్ధం;
                కొనసాగు;
            }
            ఒకవేళ (strip_Arg_If_అంకె) {
                strip_Arg_If_అంకె = అబద్ధం;
                ఒకవేళ (అంకెనా(arg[0])) కొనసాగు;
            }
            పూర్ణాంకం skip = 0;
            ఒకవేళ (arg.నిడివి() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } లేకపోతే ఒకవేళ (arg.నిడివి() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            ఒకవేళ (skip > 0) {  // arg is an option
                స్థిరరాశి string opt = arg.substr(skip);  // Remove '-' in the beginning
                స్థిరరాశి పూర్ణాంకం అంకె_Strip = strip_Options_For_బిడ్డ_నిరంతరం(opt, for_శిఖరం);
                ఒకవేళ (అంకె_Strip) {
                    UASSERT(0 <= అంకె_Strip && అంకె_Strip <= 3, "should be one of 0, 1, 2, 3");
                    ఒకవేళ (అంకె_Strip == 2) stripArg = నిజం;
                    ఒకవేళ (అంకె_Strip == 3) strip_Arg_If_అంకె = నిజం;
                    కొనసాగు;
                }
            } లేకపోతే {  // Not an option
                ఒకవేళ (vFiles.కనుగొను(arg) != vFiles.చివర()  // Remove HDL
                    || m_cppFiles.కనుగొను(arg) != m_cppFiles.చివర()) {  // Remove C++
                    కొనసాగు;
                }
            }
            ఒకవేళ (బహిర్గతం != "") బహిర్గతం += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            బహిర్గతం += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    తిరిగి బహిర్గతం;
}

శూన్య V3Options::cc_సమితి() {  // --cc
    m_బహిర్గతం_Format_Ok = నిజం;
    m_systemC = అబద్ధం;
}

శూన్య V3Options::decorations(File_పంక్తి* fl, స్థిరరాశి string& arg) {  // --decorations
    ఒకవేళ (arg == "none") {
        m_decoration = అబద్ధం;
        m_decorationNodes = అబద్ధం;
    } లేకపోతే ఒకవేళ (arg == "node") {
        m_decoration = నిజం;
        m_decorationNodes = నిజం;
    } లేకపోతే ఒకవేళ (arg == "medium") {
        m_decoration = నిజం;
        m_decorationNodes = అబద్ధం;
    } లేకపోతే {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warn_మరిన్ని() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::సదిశ<std::string> V3Options::traceClassBases() స్థిరరాశి VL_MT_సురక్షిత {
    std::సదిశ<std::string> ఫలితం;
    ఒకవేళ (traceEnabledFst()) ఫలితం.emplace_వెనుక("VerilatedFst");
    ఒకవేళ (traceEnabledSaif()) ఫలితం.emplace_వెనుక("VerilatedSaif");
    ఒకవేళ (traceEnabledVcd()) ఫలితం.emplace_వెనుక("VerilatedVcd");
    తిరిగి ఫలితం;
}
std::సదిశ<std::string> V3Options::traceClassLangs() స్థిరరాశి VL_MT_సురక్షిత {
    std::సదిశ<std::string> ఫలితం;
    కొరకు (స్థిరరాశి స్వయంచాలక& cbase : traceClassBases())
        ఫలితం.emplace_వెనుక(cbase + (systemC() ? "Sc" : "C"));
    తిరిగి ఫలితం;
}
std::సదిశ<std::string> V3Options::trace_ఆధారం_Bases() స్థిరరాశి VL_MT_సురక్షిత {
    std::సదిశ<std::string> ఫలితం;
    ఒకవేళ (traceEnabledFst()) ఫలితం.emplace_వెనుక("verilated_fst");
    ఒకవేళ (traceEnabledSaif()) ఫలితం.emplace_వెనుక("verilated_saif");
    ఒకవేళ (traceEnabledVcd()) ఫలితం.emplace_వెనుక("verilated_vcd");
    తిరిగి ఫలితం;
}
std::సదిశ<std::string> V3Options::trace_ఆధారం_Langs() స్థిరరాశి VL_MT_సురక్షిత {
    std::సదిశ<std::string> ఫలితం = trace_ఆధారం_Bases();
    కొరకు (std::string& str : ఫలితం) str += systemC() ? "_sc"s : "_c"s;
    తిరిగి ఫలితం;
}
std::string V3Options::trace_Class_పునాది() స్థిరరాశి VL_MT_సురక్షిత {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().ఖాళీలు(), "Call traceClassBase only when trace() enabled");
    తిరిగి traceClassBases().front();
}
std::string V3Options::traceClassLang() స్థిరరాశి VL_MT_సురక్షిత {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().ఖాళీలు(), "Call traceClassLang only when trace() enabled");
    తిరిగి traceClassLangs().front();
}

//######################################################################
// File searching

తార్కిక V3Options::file_Stat_సాధారణ(స్థిరరాశి string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    నిర్మాణం stat sstat;  // Stat information
    స్థిరరాశి పూర్ణాంకం err = stat(filename.c_str(), &sstat);
    ఒకవేళ (err != 0) తిరిగి అబద్ధం;
    ఒకవేళ (S_ISDIR(sstat.st_బహుళకం)) తిరిగి అబద్ధం;
    తిరిగి నిజం;
}

string V3Options::fileExists(స్థిరరాశి string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    స్థిరరాశి string dir = V3Os::filenameDir(filename);
    స్థిరరాశి string basename = V3Os::filenameNonDir(filename);

    స్వయంచాలక diriter = m_impp->m_dirMap.కనుగొను(dir);
    ఒకవేళ (diriter == m_impp->m_dirMap.చివర()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::సమితి<string>());
        diriter = m_impp->m_dirMap.కనుగొను(dir);

        std::సమితి<string>* స్థిరరాశి setp = &(diriter->రెండవ);

#నిర్వచితమైతే _MSC_VER
        ప్రయత్నించు {
            కొరకు (స్థిరరాశి స్వయంచాలక& dir_నమోదు : std::filesystem::directory_iterator(dir.c_str()))
                setp->చొప్పించు(dir_నమోదు.మార్గం().filename().string());
        } పట్టుకో (std::filesystem::filesystem_error స్థిరరాశి& ex) {
            (శూన్య)ex;
            తిరిగి "";
        }
#లేకపోతే
        ఒకవేళ (DIR* స్థిరరాశి dirp = opendir(dir.c_str())) {
            అయితే (నిర్మాణం dirent* direntp = readdir(dirp)) setp->చొప్పించు(direntp->d_name);
            closedir(dirp);
        }
#అయితే_ముగింపు
    }
    // Find it
    స్థిరరాశి std::సమితి<string>* స్థిరరాశి filesetp = &(diriter->రెండవ);
    స్థిరరాశి స్వయంచాలక fileiter = filesetp->కనుగొను(basename);
    ఒకవేళ (fileiter == filesetp->చివర()) తిరిగి "";  // Not found
    // Check if it is a directory, ignore if so
    స్థిరరాశి string filename_బహిర్గతం = V3Os::filenameJoin(dir, basename);
    ఒకవేళ (!file_Stat_సాధారణ(filename_బహిర్గతం)) తిరిగి "";  // Directory
    తిరిగి filename_బహిర్గతం;
}

string V3Options::file_మార్గం_Check_One_Dir(స్థిరరాశి string& modname, స్థిరరాశి string& dirname) {
    కొరకు (స్థిరరాశి string& i : m_impp->m_libExtVs) {
        స్థిరరాశి string fn = V3Os::filenameJoin(dirname, modname + i);
        స్థిరరాశి string exists = fileExists(fn);
        ఒకవేళ (exists != "") తిరిగి exists;
    }
    తిరిగి "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
పూర్ణాంకం V3Options::strip_Options_For_బిడ్డ_నిరంతరం(స్థిరరాశి string& opt, తార్కిక for_శిఖరం) {
    // Options to strip for both the top wrapper, and hier blocks
    స్థిర స్థిరరాశి std::unordered_map<std::string, పూర్ణాంకం> commonOpts{
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
    ఒకవేళ (commonOpts.లెక్క(opt)) తిరిగి commonOpts.at(opt);

    // Options to strip only for hier blocks
    ఒకవేళ (!for_శిఖరం) {
        స్థిర స్థిరరాశి std::unordered_map<std::string, పూర్ణాంకం> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        ఒకవేళ (subOpts.లెక్క(opt)) తిరిగి subOpts.at(opt);
        ఒకవేళ (VString::startsWith(opt, "G")) తిరిగి 1;
        ఒకవేళ (VString::startsWith(opt, "pvalue")) తిరిగి 1;
    }

    // Do not strip
    తిరిగి 0;
}

శూన్య V3Options::validateIdentifier(File_పంక్తి* fl, స్థిరరాశి string& arg, స్థిరరాశి string& opt) {
    ఒకవేళ (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::file_మార్గం(File_పంక్తి* fl, స్థిరరాశి string& modname, స్థిరరాశి string& lastpath,
                           స్థిరరాశి string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    స్థిరరాశి string filename = V3Os::filenameCleanup(VName::dehash(modname));
    ఒకవేళ (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        స్థిరరాశి string exists = file_మార్గం_Check_One_Dir(filename, "");
        ఒకవేళ (exists != "") తిరిగి exists;
    }
    కొరకు (స్థిరరాశి string& dir : m_impp->m_incDirUsers) {
        స్థిరరాశి string exists = file_మార్గం_Check_One_Dir(filename, dir);
        ఒకవేళ (exists != "") తిరిగి exists;
    }
    కొరకు (స్థిరరాశి string& dir : m_impp->m_incDirFallbacks) {
        స్థిరరాశి string exists = file_మార్గం_Check_One_Dir(filename, dir);
        ఒకవేళ (exists != "") తిరిగి exists;
    }

    ఒకవేళ (m_relativeIncludes) {
        స్థిరరాశి string exists = file_మార్గం_Check_One_Dir(filename, lastpath);
        ఒకవేళ (exists != "") తిరిగి V3Os::filename_Real_మార్గం(exists);
    }

    // Warn and return not found
    ఒకవేళ (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_మార్గం_Looked_సందేశం(fl, filename));
    }
    తిరిగి "";
}

string V3Options::file_మార్గం_Looked_సందేశం(File_పంక్తి* /*fl*/, స్థిరరాశి string& modname) {
    స్థిర తార్కిక s_shown_notfound_సందేశం = అబద్ధం;
    std::ostringstream ss;
    ఒకవేళ (modname.కనుగొను("__Vhsh") != string::npos) {
        ss << V3Error::warn_మరిన్ని() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warn_మరిన్ని() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } లేకపోతే ఒకవేళ (!s_shown_notfound_సందేశం) {
        s_shown_notfound_సందేశం = నిజం;
        ఒకవేళ (m_impp->m_incDirUsers.ఖాళీలు()) {
            ss << V3Error::warn_మరిన్ని()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warn_మరిన్ని() << "... Looked in:\n";
        కొరకు (స్థిరరాశి string& dir : m_impp->m_incDirUsers) {
            కొరకు (స్థిరరాశి string& ext : m_impp->m_libExtVs) {
                స్థిరరాశి string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_మరిన్ని() << "     " << fn << "\n";
            }
        }
        కొరకు (స్థిరరాశి string& dir : m_impp->m_incDirFallbacks) {
            కొరకు (స్థిరరాశి string& ext : m_impp->m_libExtVs) {
                స్థిరరాశి string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_మరిన్ని() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warn_మరిన్ని() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    తిరిగి ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3_Lang_సంకేతం V3Options::fileLanguage(స్థిరరాశి string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::కొలత_type pos;
    ఒకవేళ (filename == V3Options::get_Std_Package_మార్గం() || filename == V3Options::get_Std_Waiver_మార్గం()) {
        తిరిగి V3_Lang_సంకేతం::mostRecent();
    } లేకపోతే ఒకవేళ ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        స్థిరరాశి స్వయంచాలక it = m_impp->m_langExts.కనుగొను(ext);
        ఒకవేళ (it != m_impp->m_langExts.చివర()) తిరిగి it->రెండవ;
    }
    తిరిగి m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(స్థిరరాశి string& var) {
    // If update below, also update V3Options::showVersion()
    ఒకవేళ (var == "MAKE") {
        తిరిగి getenvMAKE();
    } లేకపోతే ఒకవేళ (var == "PERL") {
        తిరిగి getenvPERL();
    } లేకపోతే ఒకవేళ (var == "PYTHON3") {
        తిరిగి getenvPYTHON3();
    } లేకపోతే ఒకవేళ (var == "SYSTEMC") {
        తిరిగి getenvSYSTEMC();
    } లేకపోతే ఒకవేళ (var == "SYSTEMC_ARCH") {
        తిరిగి getenvSYSTEMC_ARCH();
    } లేకపోతే ఒకవేళ (var == "SYSTEMC_INCLUDE") {
        తిరిగి getenvSYSTEMC_INCLUDE();
    } లేకపోతే ఒకవేళ (var == "SYSTEMC_LIBDIR") {
        తిరిగి getenvSYSTEMC_LIBDIR();
    } లేకపోతే ఒకవేళ (var == "VERILATOR_ROOT") {
        తిరిగి getenv_VERILATOR_మూలం();
    } లేకపోతే {
        తిరిగి V3Os::getenvStr(var, "");
    }
}

#నిర్వచితమైతే __FreeBSD__
string V3Options::getenvMAKE() { తిరిగి V3Os::getenvStr("MAKE", "gmake"); }
#లేకపోతే
string V3Options::getenvMAKE() { తిరిగి V3Os::getenvStr("MAKE", "make"); }
#అయితే_ముగింపు

string V3Options::getenvMAKEFLAGS() {  //
    తిరిగి V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    తిరిగి V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    తిరిగి V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    స్థిరరాశి string defenv = string{DEFENV_SYSTEMC};
    ఒకవేళ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    తిరిగి V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    స్థిరరాశి string defenv = string{DEFENV_SYSTEMC_ARCH};
    ఒకవేళ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    ఒకవేళ (var == "") {
#ఒకవేళ defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        స్థిరరాశి string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#లేదావేళ defined(_WIN32)
        స్థిరరాశి string sysname = "WIN32";
        var = "win32";
#లేకపోతే
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        నిర్మాణం utsname uts;
        uname(&uts);
        స్థిరరాశి string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        ఒకవేళ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } లేకపోతే ఒకవేళ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } లేకపోతే {
            var = "linux";
        }
#అయితే_ముగింపు
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    తిరిగి var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    స్థిరరాశి string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    ఒకవేళ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    ఒకవేళ (var == "") {
        స్థిరరాశి string sc = getenvSYSTEMC();
        ఒకవేళ (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    తిరిగి V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    స్థిరరాశి string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    ఒకవేళ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    ఒకవేళ (var == "") {
        స్థిరరాశి string sc = getenvSYSTEMC();
        స్థిరరాశి string arch = getenvSYSTEMC_ARCH();
        ఒకవేళ (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    తిరిగి V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_మూలం() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    స్థిరరాశి string defenv = string{DEFENV_VERILATOR_మూలం};
    ఒకవేళ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    ఒకవేళ (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    తిరిగి V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    స్థిరరాశి string defenv = string{DEFENV_VERILATOR_SOLVER};
    ఒకవేళ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    తిరిగి var;
}

string V3Options::get_Std_Package_మార్గం() {
    తిరిగి V3Os::filenameJoin(getenv_VERILATOR_మూలం(), "include", "verilated_std.sv");
}
string V3Options::get_Std_Waiver_మార్గం() {
    తిరిగి V3Os::filenameJoin(getenv_VERILATOR_మూలం(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(స్థిరరాశి string& var) {
    // If update below, also update V3Options::showVersion()
    ఒకవేళ (var == "COROUTINES" && coroutineSupport()) {
        తిరిగి "1";
    } లేకపోతే ఒకవేళ (var == "TSAN" && tsanSupport()) {
        తిరిగి "1";
    } లేకపోతే ఒకవేళ (var == "DEV_ASAN" && devAsan()) {
        తిరిగి "1";
    } లేకపోతే ఒకవేళ (var == "DEV_GCOV" && devGcov()) {
        తిరిగి "1";
        // cppcheck-suppress knownConditionTrueFalse
    } లేకపోతే ఒకవేళ (var == "SYSTEMC" && systemCFound()) {
        తిరిగి "1";
    } లేకపోతే {
        తిరిగి "";
    }
}

తార్కిక V3Options::systemCSystemWide() {
#నిర్వచితమైతే HAVE_SYSTEMC
    తిరిగి నిజం;
#లేకపోతే
    తిరిగి అబద్ధం;
#అయితే_ముగింపు
}

తార్కిక V3Options::systemCFound() {
    తిరిగి (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().ఖాళీలు() && !getenvSYSTEMC_LIBDIR().ఖాళీలు()));
}

తార్కిక V3Options::coroutineSupport() {
#నిర్వచితమైతే HAVE_COROUTINES
    తిరిగి నిజం;
#లేకపోతే
    తిరిగి అబద్ధం;
#అయితే_ముగింపు
}

తార్కిక V3Options::devAsan() {
#నిర్వచితమైతే HAVE_DEV_ASAN
    తిరిగి నిజం;
#లేకపోతే
    తిరిగి అబద్ధం;
#అయితే_ముగింపు
}

తార్కిక V3Options::devGcov() {
#నిర్వచితమైతే HAVE_DEV_GCOV
    తిరిగి నిజం;
#లేకపోతే
    తిరిగి అబద్ధం;
#అయితే_ముగింపు
}

తార్కిక V3Options::tsanSupport() {
#నిర్వచితమైతే HAVE_TSAN
    తిరిగి నిజం;
#లేకపోతే
    తిరిగి అబద్ధం;
#అయితే_ముగింపు
}

//######################################################################
// V3 Options notification methods

శూన్య V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_పంక్తి* స్థిరరాశి cmdfl = కొత్త File_పంక్తి{File_పంక్తి::command_పంక్తి_Filename()};

    ఒకవేళ (!బహిర్గతం_Format_Ok() && v3Global.opt.ప్రధాన()) cc_సమితి();  // --main implies --cc if not provided
    ఒకవేళ (!బహిర్గతం_Format_Ok() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    ఒకవేళ (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::సదిశ<std::string> backendFlags;
    ఒకవేళ (m_build) {
        ఒకవేళ (m_ద్విమాన)
            backendFlags.emplace_వెనుక("--binary");
        లేకపోతే
            backendFlags.emplace_వెనుక("--build");
    }
    ఒకవేళ (m_preprocOnly) backendFlags.emplace_వెనుక("-E");
    ఒకవేళ (m_dpiHdrOnly) backendFlags.emplace_వెనుక("--dpi-hdr-only");
    ఒకవేళ (m_lintOnly) backendFlags.emplace_వెనుక("--lint-only");
    ఒకవేళ (m_jsonOnly) backendFlags.emplace_వెనుక("--json-only");
    ఒకవేళ (backendFlags.కొలత() > 1) {
        std::string backendFlagsString = backendFlags.front();
        కొరకు (కొలత_t i = 1; i < backendFlags.కొలత(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    ఒకవేళ (m_exe && !v3Global.opt.libCreate().ఖాళీలు()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    ఒకవేళ (!m_gmake && !m_makeJson) m_gmake = నిజం;

    ఒకవేళ (m_క్రమానుగత && (m_hier_బిడ్డ || !m_hier_ఖండాలు.ఖాళీలు())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    ఒకవేళ (m_hier_బిడ్డ) {
        ఒకవేళ (m_hier_ఖండాలు.ఖాళీలు()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = అబద్ధం;
    }

    ఒకవేళ (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = నిజం;  // No format, with --trace means wanted --trace-vcd
    }
    ఒకవేళ (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = నిజం;
    స్థిరరాశి పూర్ణాంకం ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    ఒకవేళ (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    ఒకవేళ (protectIds()) {
        ఒకవేళ (all_ప్రజా()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warn_మరిన్ని() + "... Suggest remove --public.");
        }
        ఒకవేళ (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warn_మరిన్ని() + "... Suggest remove --trace.");
        }
        ఒకవేళ (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warn_మరిన్ని() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    ఒకవేళ (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.సమితి_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    ఒకవేళ (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.సమితి_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    ఒకవేళ (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.సమితి_True_Or_False(నిజం);

    // Default split limits if not specified
    ఒకవేళ (m_output_విభజించు_CFuncs < 0) m_output_విభజించు_CFuncs = m_output_విభజించు;
    ఒకవేళ (m_output_విభజించు_CTrace < 0) m_output_విభజించు_CTrace = m_output_విభజించు;

    ఒకవేళ (v3Global.opt.ప్రధాన() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    ఒకవేళ (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        ఒకవేళ (క్రమానుగత()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    ఒకవేళ (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    ఒకవేళ (v3Global.opt.timing().is_సమితి_True() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    ఒకవేళ (!m_dumpLevel.లెక్క("tree") && m_dumpLevel.లెక్క("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(దారాలు() >= 1, "'threads()' must return a value >= 1");
    ఒకవేళ (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    ఒకవేళ (m_buildJobs == -1) m_buildJobs = 1;
    ఒకవేళ (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    ఒకవేళ (timing().is_సమితి_True()) V3PreShell::define_Cmd_పంక్తి("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    ఒకవేళ (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.సమితి_True_Or_False(అబద్ధం);

    // === Leave last
    // Mark options as available
    m_అందుబాటు = నిజం;
}

//######################################################################
// V3 Options accessors

string V3Options::సంస్కరణ() VL_శుద్ధ {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_గమ్యం_Str(DTVERSION_rev);
    తిరిగి ver;
}

string V3Options::protect_కీలకం_Defaulted() VL_MT_సురక్షిత {
    స్థిర V3Mutex s_mutex;
    స్థిరరాశి V3_తాళం_Guard తాళం_2{s_mutex};
    ఒకవేళ (m_protect_కీలకం.ఖాళీలు()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_కీలకం = "VL-KEY-" + digest.digestBase64();
    }
    తిరిగి m_protect_కీలకం;
}

శూన్య V3Options::throwSigsegv() {  // LCOV_EXCL_START
#ఒకవేళ !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<అస్థిర అక్షరం*>(శూన్యసూచిక) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#అయితే_ముగింపు
}  // LCOV_EXCL_STOP

VTimescale V3Options::సమయం_Compute_Prec(స్థిరరాశి VTimescale& flag) స్థిరరాశి {
    ఒకవేళ (!సమయం_Override_Prec().isNone()) {
        తిరిగి సమయం_Override_Prec();
    } లేకపోతే ఒకవేళ (flag.isNone()) {
        తిరిగి సమయం_Default_Prec();
    } లేకపోతే {
        తిరిగి flag;
    }
}

VTimescale V3Options::సమయం_Compute_Unit(స్థిరరాశి VTimescale& flag) స్థిరరాశి {
    ఒకవేళ (!సమయం_Override_Unit().isNone()) {
        తిరిగి సమయం_Override_Unit();
    } లేకపోతే ఒకవేళ (flag.isNone()) {
        తిరిగి సమయం_Default_Unit();
    } లేకపోతే {
        తిరిగి flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(పూర్ణాంకం argc, అక్షరం** argv) {
    // Return list of arguments as simple string
    string opts;
    కొరకు (పూర్ణాంకం i = 0; i < argc; ++i) {
        ఒకవేళ (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    తిరిగి opts;
}

//######################################################################
// V3 Options Parsing

శూన్య V3Options::parseOpts(File_పంక్తి* fl, పూర్ణాంకం argc, అక్షరం** argv) VL_MT_DISABLED {
    // Save command line options
    కొరకు (పూర్ణాంకం i = 0; i < argc; ++i) కూడు_పంక్తి_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    ఒకవేళ (vFiles().ఖాళీలు()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    ఒకవేళ (ఉపసర్గ() == "" && శిఖరం_Module() != "") m_ఉపసర్గ = "V"s + శిఖరం_Module();  // Already encoded
    ఒకవేళ (ఉపసర్గ() == "" && vFiles().కొలత() >= 1)
        m_ఉపసర్గ
            = "V"s + Ast_కణుపు::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    ఒకవేళ (mod_ఉపసర్గ() == "") m_mod_ఉపసర్గ = ఉపసర్గ();

    // Find files in makedir
    కూడు_Inc_Dir_Fallback(makeDir());
}

//======================================================================

తార్కిక V3Options::suffixed(స్థిరరాశి string& sw, స్థిరరాశి అక్షరం* arg) {
    ఒకవేళ (std::అక్షరశ్రేణి_పొడవు(arg) > sw.నిడివి()) తిరిగి అబద్ధం;
    తిరిగి (0 == std::strcmp(sw.c_str() + sw.నిడివి() - std::అక్షరశ్రేణి_పొడవు(arg), arg));
}

శూన్య V3Options::parseOptsList(File_పంక్తి* fl, స్థిరరాశి string& optdir, పూర్ణాంకం argc,
                              అక్షరం** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    స్థిరరాశి V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_ట్యాగులు;

    స్థిరరాశి స్వయంచాలక callStrSetter = [ఇది](శూన్య (V3Options::*cbStr)(స్థిరరాశి string&)) {
        తిరిగి [ఇది, cbStr](స్థిరరాశి string& v) { (ఇది->*cbStr)(v); };
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
    DECL_OPTION("+define+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) VL_MT_DISABLED {
        కూడు_Define(optp, నిజం);
    }).notForRerun();
    DECL_OPTION("+incdir+", Cb_Partial_సరిపోలిక, [ఇది, &optdir](స్థిరరాశి అక్షరం* optp) {
        string dirs = optp;
        string::కొలత_type pos;
        అయితే ((pos = dirs.కనుగొను('+')) != string::npos) {
            కూడు_Inc_Dir_User(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        కూడు_Inc_Dir_User(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        string exts = optp;
        string::కొలత_type pos;
        అయితే ((pos = exts.కనుగొను('+')) != string::npos) {
            కూడు_Lib_Ext_V(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        కూడు_Lib_Ext_V(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        కూడు_Lang_Ext(optp, V3_Lang_సంకేతం::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](తార్కిక) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [ఇది](తార్కిక flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](స్థిరరాశి అక్షరం*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [ఇది](తార్కిక flag) {
        m_bboxUnsup = flag;
        File_పంక్తి::globalWarnOff(V3_Error_సంకేతం::E_UNSUPPORTED, నిజం);
    });
    DECL_OPTION("-binary", CbCall, [ఇది]() {
        m_ద్విమాన = నిజం;
        m_build = నిజం;
        m_exe = నిజం;
        m_main = నిజం;
        ఒకవేళ (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", సమితి_2, &m_build);
    DECL_OPTION("-build-dep-bin", సమితి_2, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        పూర్ణాంకం val = std::atoi(valp);
        ఒకవేళ (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } లేకపోతే ఒకవేళ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::కూడు_CFlags));
    DECL_OPTION("-cc", CbCall, [ఇది]() { cc_సమితి(); });
    DECL_OPTION("-clk", CbVal, [fl](స్థిరరాశి std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](స్థిరరాశి std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", సమితి_2, &m_comp_హద్దు_ఖండాలు).undocumented();
    DECL_OPTION("-comp-limit-members", సమితి_2,
                &m_comp_హద్దు_Members)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", సమితి_2, &m_comp_హద్దు_Parens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](పూర్ణాంకం val) { VName::max_నిడివి(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        ఒకవేళ (!std::strcmp(valp, "clang")) {
            m_comp_హద్దు_ఖండాలు = 80;  // limit unknown
            m_comp_హద్దు_Members = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_comp_హద్దు_Parens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "gcc")) {
            m_comp_హద్దు_ఖండాలు = 0;  // Bug free
            m_comp_హద్దు_Members = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_comp_హద్దు_Parens = 240;  // Unlimited, but generate same code as for clang
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "msvc")) {
            m_comp_హద్దు_ఖండాలు = 80;  // 128, but allow some room
            m_comp_హద్దు_Members = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_comp_హద్దు_Parens = 80;  // 128, but allow some room
        } లేకపోతే {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warn_మరిన్ని() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::కూడు_Compiler_Includes));
    DECL_OPTION("-converge-limit", సమితి_2, &m_converge_హద్దు);
    DECL_OPTION("-coverage", CbOnOff, [ఇది](తార్కిక flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_సమాసం);
    DECL_OPTION("-coverage-expr-max", సమితి_2, &m_coverage_సమాసం_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* స్థిరరాశి valp) {
        ఒకవేళ (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::నిండిన;
        } లేకపోతే {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warn_మరిన్ని() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", సమితి_2, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_పంక్తి);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    స్థిరరాశి స్వయంచాలక parse_Bins_హద్దు = [fl](స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* valp, uint32_t& limitr) {
        అక్షరం* endp = శూన్యసూచిక;
        స్థిరరాశి గుర్తులేని దీర్ఘ దీర్ఘ విలువ = std::strtoull(valp, &endp, 10);
        ఒకవేళ (*endp || విలువ < 1 || విలువ > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            తిరిగి;
        }
        limitr = static_cast<uint32_t>(విలువ);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [ఇది, parse_Bins_హద్దు](స్థిరరాశి అక్షరం* valp) {
        parse_Bins_హద్దు("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [ఇది, parse_Bins_హద్దు](స్థిరరాశి అక్షరం* valp) {
        parse_Bins_హద్దు("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", సమితి_2, &m_coverage_Max_వెడల్పు);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverage_విలీనం_Instances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coverage_చొప్పున_Instance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* valp) VL_MT_DISABLED {
        కూడు_Define(valp, అబద్ధం);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [ఇది]() { సమితి_Debug_బహుళకం(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [ఇది](పూర్ణాంకం v) { సమితి_Debug_బహుళకం(v); }).notForRerun();
    DECL_OPTION("-debugi-", Cb_Partial_సరిపోలిక_Val, [ఇది](స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* valp) {
        m_debugLevel[optp] = std::atoi(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#నిర్వచితమైతే VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", సమితి_2, &m_debugAllocRandom).undocumented();
#అయితే_ముగింపు
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
    DECL_OPTION("-debug-partition", OnOff, &m_debug_విభజన).undocumented();
    DECL_OPTION("-debug-preproc-passthru", OnOff, &m_debugPreprocPassthru).undocumented();
    DECL_OPTION("-debug-protect", OnOff, &m_debugProtect).undocumented();
    DECL_OPTION("-debug-runtime-timeout", సమితి_2, &m_debugRuntimeTimeout).undocumented();
    DECL_OPTION("-debug-self-test", OnOff, &m_debugSelfTest).undocumented();
    DECL_OPTION("-debug-sigsegv", CbCall, throwSigsegv).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_దొంతర_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debug_వెడల్పు).undocumented();
    DECL_OPTION("-decoration", CbCall, [ఇది, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [ఇది, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [ఇది](స్థిరరాశి అక్షరం* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = నిజం;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", Cb_Partial_సరిపోలిక_Val, [ఇది](స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [ఇది](తార్కిక flag) {
        ఒకవేళ (flag) {
            m_stdPackage = అబద్ధం;
            m_stdWaiver = అబద్ధం;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<శూన్య (*)(పూర్ణాంకం)>(&V3Error::error_హద్దు));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [ఇది](స్థిరరాశి అక్షరం* valp) { m_expand_హద్దు = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [ఇది, fl, &optdir](స్థిరరాశి అక్షరం* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), నిజం);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [ఇది, &optdir](స్థిరరాశి అక్షరం* valp) { కూడు_Force_Inc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [ఇది, fl, &optdir](స్థిరరాశి అక్షరం* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), అబద్ధం);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [ఇది](స్థిరరాశి అక్షరం* valp) { కూడు_Future0(valp); });
    DECL_OPTION("-future1", CbVal, [ఇది](స్థిరరాశి అక్షరం* valp) { కూడు_Future1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_f_బిట్టు_పరిశీలన_Loops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [ఇది](తార్కిక flag) {
        m_fCaseDecoder = flag;
        m_f_Case_పట్టిక = flag;
        m_f_Case_వృక్షం = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_f_Case_పట్టిక);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_వృక్షం);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_బిట్టు_Op_వృక్షం);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [ఇది](తార్కిక flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [ఇది](తార్కిక flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](తార్కిక) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](తార్కిక) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](తార్కిక) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_నెట్టు_కిందికి_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [ఇది, fl](తార్కిక flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [ఇది](తార్కిక flag) {  //
        m_f_Func_విభజించు_Cat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_f_Func_విభజించు_Cat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [ఇది](తార్కిక flag) {  //
        m_fIcoChangeDetect.సమితి_True_Or_False(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](తార్కిక) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_f_విలీనం_Cond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_f_విలీనం_Cond_Motion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](తార్కిక) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_f_ముక్క);
    DECL_OPTION("-fslice-element-limit", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_f_ముక్క_Element_హద్దు = std::atoi(valp);
        ఒకవేళ (m_f_ముక్క_Element_హద్దు < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_f_విభజించు);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_f_పట్టిక);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](తార్కిక) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) { కూడు_Parameter(optp, అబద్ధం); });
    DECL_OPTION("-gate-stmts", సమితి_2, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [ఇది]() {
        బయటకు << protect_కీలకం_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](స్థిరరాశి అక్షరం* valp) {
        బయటకు << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](స్థిరరాశి అక్షరం* valp) {
        బయటకు << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_క్రమానుగత);
    DECL_OPTION("-hierarchical-block", CbVal, [ఇది](స్థిరరాశి అక్షరం* valp) {
        స్థిరరాశి V3_క్రమానుగత_ఖండం_Option opt{valp};
        m_hier_ఖండాలు.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", సమితి_2, &m_hier_బిడ్డ);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [ఇది](స్థిరరాశి అక్షరం* optp) { m_hierParamsFile.నెట్టు_వెనుక({optp, work()}); });

    DECL_OPTION("-I", Cb_Partial_సరిపోలిక, [ఇది, &optdir](స్థిరరాశి అక్షరం* optp) {
        కూడు_Inc_Dir_User(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", సమితి_2, &m_if_లోతు);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", సమితి_2, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", సమితి_2, &m_inline_CFuncs_లబ్ధం);
    DECL_OPTION("-inline-mult", సమితి_2, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [ఇది, fl](పూర్ణాంకం val) {
        m_instr_లెక్క_Dpi = val;
        ఒకవేళ (m_instr_లెక్క_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [ఇది](స్థిరరాశి అక్షరం* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = నిజం;
    });
    DECL_OPTION("-json-only-output", CbVal, [ఇది](స్థిరరాశి అక్షరం* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = నిజం;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::కూడు_Ld_Libs));
    DECL_OPTION("-l2-name", సమితి_2, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [ఇది]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [ఇది]() { m_l2Name = "v"; }).undocumented();  // Historical
    స్థిరరాశి స్వయంచాలక సమితి_Lang = [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        స్థిరరాశి V3_Lang_సంకేతం optval{valp};
        ఒకవేళ (optval.legal()) {
            m_defaultLanguage = optval;
        } లేకపోతే {
            VSpellCheck spell;
            కొరకు (పూర్ణాంకం i = V3_Lang_సంకేతం::L_ERROR + 1; i < V3_Lang_సంకేతం::ENUM_చివర; ++i) {
                spell.నెట్టు_Candidate(V3_Lang_సంకేతం{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.ఉత్తమ_Candidate_సందేశం(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, సమితి_Lang).notForRerun();
    DECL_OPTION("-language", CbVal, సమితి_Lang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [ఇది](స్థిరరాశి అక్షరం* valp) { కూడు_Lib_Map_File(valp); });
    DECL_OPTION("-lib-create", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", సమితి_2, &m_localize_Max_కొలత);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::కూడు_Make_Flags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [ఇది](స్థిరరాశి అక్షరం* valp) {
        m_makeDir = valp;
        కూడు_Inc_Dir_Fallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", సమితి_2, &m_main_శిఖరం_Name);
    DECL_OPTION("-make", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        ఒకవేళ (!std::strcmp(valp, "gmake")) {
            m_gmake = నిజం;
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "json")) {
            m_makeJson = నిజం;
        } లేకపోతే {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", సమితి_2, &m_funcRecursion);
    DECL_OPTION("-max-num-width", సమితి_2, &m_max_అంకె_వెడల్పు);
    DECL_OPTION("-mod-prefix", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_mod_ఉపసర్గ = valp;
    });

    DECL_OPTION("-O0", CbCall, [ఇది]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [ఇది]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [ఇది]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [ఇది]() { optimize(3); });

    DECL_OPTION("-o", సమితి_2, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](తార్కిక /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_outputGroups = std::atoi(valp);
        ఒకవేళ (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_output_Groups_కొలత = std::atoll(valp);
        ఒకవేళ (m_output_Groups_కొలత < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", సమితి_2, &m_output_విభజించు);
    DECL_OPTION("-output-split-cfuncs", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_output_విభజించు_CFuncs = std::atoi(valp);
        ఒకవేళ (m_output_విభజించు_CFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_output_విభజించు_CTrace = std::atoi(valp);
        ఒకవేళ (m_output_విభజించు_CTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", సమితి_2, &m_preproc_No_పంక్తి);
    DECL_OPTION("-pins64", CbCall, [ఇది]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [ఇది]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_pinsBv = std::atoi(valp);
        ఒకవేళ (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [ఇది](తార్కిక flag) {
        m_pinsScUint = flag;
        ఒకవేళ (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [ఇది](తార్కిక flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [ఇది](తార్కిక flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", సమితి_2, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_ఉపసర్గ = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preproc_నిర్వచిస్తుంది);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_preproc_Token_హద్దు = std::atoi(valp);
        ఒకవేళ (m_preproc_Token_హద్దు <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [ఇది]() { m_ప్రజా = అబద్ధం; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [ఇది]() { m_profC = m_profCFuncs = నిజం; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [ఇది]() {
        m_profC = m_profCFuncs = నిజం;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", సమితి_2, &m_protect_కీలకం);
    DECL_OPTION("-protect-lib", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = నిజం;
    });
    DECL_OPTION("-public", OnOff, &m_ప్రజా);
    DECL_OPTION("-public-depth", సమితి_2, &m_ప్రజా_లోతు);
    DECL_OPTION("-public-flat-rw", CbOnOff, [ఇది](తార్కిక flag) {
        m_ప్రజా_Flat_RW = flag;
        v3Global.dpi(నిజం);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [ఇది](తార్కిక flag) { m_ప్రజా_Ignore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [ఇది](తార్కిక flag) {
        m_ప్రజా_Params = flag;
        v3Global.dpi(నిజం);
    });
    DECL_OPTION("-pvalue+", Cb_Partial_సరిపోలిక,
                [ఇది](స్థిరరాశి అక్షరం* varp) { కూడు_Parameter(varp, అబద్ధం); });

    DECL_OPTION("-quiet", CbOnOff, [ఇది](తార్కిక flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_reloop_హద్దు = std::atoi(valp);
        ఒకవేళ (m_reloop_హద్దు < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_replication_హద్దు = std::atoi(valp);
        ఒకవేళ (m_replication_హద్దు < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [ఇది, fl]() {
        decorations(fl, "node");
        కూడు_CFlags("-ggdb");
        కూడు_Ld_Libs("-ggdb");
        కూడు_CFlags("-fsanitize=address,undefined");
        కూడు_Ld_Libs("-fsanitize=address,undefined");
        కూడు_CFlags("-D_GLIBCXX_DEBUG");
        కూడు_CFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [ఇది]() {
        m_బహిర్గతం_Format_Ok = నిజం;
        m_systemC = నిజం;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [ఇది](తార్కిక flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [ఇది](తార్కిక flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [ఇది]() { m_defaultLanguage = V3_Lang_సంకేతం::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [ఇది, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_దారాలు = 1;
    });
    DECL_OPTION("-threads", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_దారాలు = std::atoi(valp);
        ఒకవేళ (m_దారాలు < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        ఒకవేళ (m_దారాలు == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_దారాలు = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_hier_దారాలు = std::atoi(valp);
        ఒకవేళ (m_hier_దారాలు < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_దారాలు_Coarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        ఒకవేళ (!std::strcmp(valp, "all")) {
            m_దారాలు_Dpi_శుద్ధ = నిజం;
            m_దారాలు_Dpi_Unpure = నిజం;
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "none")) {
            m_దారాలు_Dpi_శుద్ధ = అబద్ధం;
            m_దారాలు_Dpi_Unpure = అబద్ధం;
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "pure")) {
            m_దారాలు_Dpi_శుద్ధ = నిజం;
            m_దారాలు_Dpi_Unpure = అబద్ధం;
        } లేకపోతే {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warn_మరిన్ని() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        m_దారాలు_Max_MTasks = std::atoi(valp);
        ఒకవేళ (m_దారాలు_Max_MTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        ఒకవేళ (!unit.isNone() && సమయం_Override_Unit().isNone()) m_సమయం_Default_Unit = unit;
        ఒకవేళ (!prec.isNone() && సమయం_Override_Prec().isNone()) m_సమయం_Default_Prec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, నిజం);
        ఒకవేళ (!unit.isNone()) {
            m_సమయం_Default_Unit = unit;
            m_సమయం_Override_Unit = unit;
        }
        ఒకవేళ (!prec.isNone()) {
            m_సమయం_Default_Prec = prec;
            m_సమయం_Override_Prec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [ఇది](స్థిరరాశి std::string& flag) { m_శిఖరం_Module = Ast_కణుపు::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [ఇది](స్థిరరాశి std::string& flag) { m_శిఖరం_Module = Ast_కణుపు::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", సమితి_2, &m_శిఖరం_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [ఇది]() { m_traceEnabledSaif = నిజం; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", సమితి_2, &m_trace_లోతు);
    DECL_OPTION("-trace-fst", CbCall, [ఇది]() {
        m_traceEnabledFst = నిజం;
        కూడు_Ld_Libs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", సమితి_2, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", సమితి_2, &m_trace_Max_వెడల్పు);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](స్థిరరాశి అక్షరం*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", సమితి_2, &m_no_Trace_శిఖరం);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [ఇది]() { m_traceEnabledVcd = నిజం; });

    DECL_OPTION("-U", Cb_Partial_సరిపోలిక, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", సమితి_2, &m_constraint_Array_హద్దు);
    DECL_OPTION("-unroll-count", సమితి_2, &m_unroll_లెక్క).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", సమితి_2, &m_unroll_హద్దు);
    DECL_OPTION("-unroll-stmts", సమితి_2, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", సమితి_2, &m_వాడని_Regexp);

    DECL_OPTION("-V", CbCall, [ఇది]() {
        show_సంస్కరణ(నిజం);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [ఇది, &optdir](స్థిరరాశి అక్షరం* valp) {
        V3Options::కూడు_Library_File(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        పూర్ణాంకం val = std::atoi(valp);
        ఒకవేళ (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } లేకపోతే ఒకవేళ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [ఇది]() {
        show_సంస్కరణ(అబద్ధం);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_పంక్తి::globalWarnOff(V3_Error_సంకేతం::I_LINT, అబద్ధం); });
    DECL_OPTION("-Werror-", Cb_Partial_సరిపోలిక, [ఇది, fl](స్థిరరాశి అక్షరం* optp) {
        స్థిరరాశి V3_Error_సంకేతం సంకేతం_2{optp};
        ఒకవేళ (సంకేతం_2 == V3_Error_సంకేతం::EC_ERROR) {
            ఒకవేళ (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } లేకపోతే {
            V3Error::pretendError(సంకేతం_2, నిజం);
        }
    });
    DECL_OPTION("-Wfuture-", Cb_Partial_సరిపోలిక, [ఇది](స్థిరరాశి అక్షరం* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        కూడు_Future(optp);
    });
    DECL_OPTION("-Wno-", Cb_Partial_సరిపోలిక, [fl, &parser](స్థిరరాశి అక్షరం* optp) VL_MT_DISABLED {
        స్థిరరాశి string err = File_పంక్తి::globalWarnOffParse(optp, నిజం);
        ఒకవేళ (!err.ఖాళీలు()) {
            స్థిరరాశి string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    కొరకు (పూర్ణాంకం i = V3_Error_సంకేతం::EC_మొదటి_WARN; i < V3_Error_సంకేతం::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        కొరకు (స్థిరరాశి string ఉపసర్గ : {"-Wno-", "-Wwarn-"})
            parser.కూడు_Suggestion_Candidate(ఉపసర్గ + V3_Error_సంకేతం{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [ఇది]() { m_context = అబద్ధం; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(అబద్ధం); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_పంక్తి::globalWarnOff(V3_Error_సంకేతం::I_LINT, నిజం); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_పంక్తి::globalWarnOff(V3_Error_సంకేతం::I_STYLE, నిజం); });
    DECL_OPTION("-work", సమితి_2, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [ఇది]() {
        m_pedantic = నిజం;
        V3Error::pretendError(V3_Error_సంకేతం::ASSIGNIN, అబద్ధం);
    });
    DECL_OPTION("-Wwarn-", Cb_Partial_సరిపోలిక, [ఇది, fl, &parser](స్థిరరాశి అక్షరం* optp) VL_MT_DISABLED {
        స్థిరరాశి V3_Error_సంకేతం సంకేతం_2{optp};
        ఒకవేళ (సంకేతం_2 == V3_Error_సంకేతం::EC_ERROR) {
            ఒకవేళ (!isFuture(optp)) {
                స్థిరరాశి string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } లేకపోతే {
            File_పంక్తి::globalWarnOff(సంకేతం_2, అబద్ధం);
            V3Error::pretendError(సంకేతం_2, అబద్ధం);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_పంక్తి::globalWarnOff(V3_Error_సంకేతం::I_LINT, అబద్ధం); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_పంక్తి::globalWarnOff(V3_Error_సంకేతం::I_STYLE, అబద్ధం); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", సమితి_2, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        ఒకవేళ (!std::strcmp(valp, "0")) {
            m_x_కేటాయించు = "0";
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "1")) {
            m_x_కేటాయించు = "1";
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "fast")) {
            m_x_కేటాయించు = "fast";
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "unique")) {
            m_x_కేటాయించు = "unique";
        } లేకపోతే {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warn_మరిన్ని() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [ఇది, fl](స్థిరరాశి అక్షరం* valp) {
        ఒకవేళ (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } లేకపోతే ఒకవేళ (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } లేకపోతే {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warn_మరిన్ని() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_x_Initial_అంచు);

    DECL_OPTION("-y", CbVal, [ఇది, &optdir](స్థిరరాశి అక్షరం* valp) {
        కూడు_Inc_Dir_User(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    కొరకు (పూర్ణాంకం i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        ఒకవేళ (argv[i][0] == '-' || argv[i][0] == '+') {
            స్థిరరాశి std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            పూర్ణాంకం consumed = 0;
            తార్కిక isForRerun = నిజం;

            // Special case for -j which has an optional argument
            ఒకవేళ (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                పూర్ణాంకం val = 0;
                ఒకవేళ (i + 1 < argc && std::అంకెనా(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    ఒకవేళ (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                ఒకవేళ (m_buildJobs == -1) m_buildJobs = val;
                ఒకవేళ (m_verilateJobs == -1) m_verilateJobs = val;
                ఒకవేళ (m_outputGroups == -1) m_outputGroups = val;
            } లేకపోతే {
                స్థిరరాశి స్వయంచాలక pair = parser.parse(i, argc, argv);
                consumed = pair.మొదటి;
                isForRerun = pair.రెండవ;
                ఒకవేళ (consumed) {
                    // Already processed
                } లేకపోతే ఒకవేళ (isFuture0(argName)) {
                    consumed = 1;
                } లేకపోతే ఒకవేళ (isFuture1(argName)) {
                    consumed = 2;
                } లేకపోతే {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            ఒకవేళ (argName != "f" && argName != "F") కూడు_Arg(argv + i, consumed, isForRerun);
            i += consumed;
            కొనసాగు;
        }

        // Filename
        స్థిరరాశి std::string filename = parseFileArg(optdir, argv[i]);
        తార్కిక isForRerun = నిజం;
        ఒకవేళ (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::కూడు_Cpp_File(filename);
        } లేకపోతే ఒకవేళ (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::కూడు_Ld_Lib_File(filename);
        } లేకపోతే ఒకవేళ (suffixed(filename, ".vlt")) {
            V3Options::కూడు_Vlt_File(filename, work());
            isForRerun = అబద్ధం;
        } లేకపోతే {
            V3Options::కూడు_VFile(filename, work());
            isForRerun = అబద్ధం;
        }
        కూడు_Arg(filename, isForRerun);
        ++i;
    }

    ఒకవేళ (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

శూన్య V3Options::parseOptsFile(File_పంక్తి* fl, స్థిరరాశి string& filename, తార్కిక rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    స్థిరరాశి std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    ఒకవేళ (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        తిరిగి;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    తార్కిక has_fargs = అబద్ధం;
    std::string సంపూర్ణ_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    స్థిరరాశి స్వయంచాలక parse_fargs_cmt = [&](స్థిరరాశి అక్షరం* p) -> శూన్య {
        స్థిరరాశి తార్కిక ఖండం_Cmt = p[1] == '*';
        p += 2;
        అయితే (std::ఖాళీనా(*p)) ++p;
        ఒకవేళ (VString::startsWith(p, "verilator")) {
            p += std::అక్షరశ్రేణి_పొడవు("verilator");
            ఒకవేళ (std::ఖాళీనా(*p)) {
                అయితే (std::ఖాళీనా(*p)) ++p;
                ఒకవేళ (VString::startsWith(p, "fargs")) {
                    p += std::అక్షరశ్రేణి_పొడవు("fargs");
                    ఒకవేళ (std::ఖాళీనా(*p)) {
                        అయితే (std::ఖాళీనా(*p)) ++p;
                        ఒకవేళ (!has_fargs) {
                            has_fargs = నిజం;
                            సంపూర్ణ_file.clear();
                        }
                        సంపూర్ణ_file += p;
                        ఒకవేళ (ఖండం_Cmt) {
                            స్థిరరాశి std::string::కొలత_type pos = సంపూర్ణ_file.కనుగొను("*/");
                            ఒకవేళ (pos == std::string::npos) {
                                fl->v3fatal("/* verilator fargs */ must be on single line");
                            }
                            సంపూర్ణ_file.resize(pos);
                        }
                        సంపూర్ణ_file += ' ';
                    }
                }
            }
        }
    };

    తార్కిక లోపల_Cmt = అబద్ధం;
    అయితే (!ifp->eof()) {
        స్థిరరాశి std::string పంక్తి = V3Os::getline(*ifp);
        // Strip simple comments
        అక్షరం lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        తార్కిక space_begin = నిజం;  // At beginning or leading spaces only
        కొరకు (స్థిరరాశి అక్షరం* p = పంక్తి.c_str(); *p; lastch = *p++) {
            ఒకవేళ (లోపల_Cmt) {
                ఒకవేళ (p[0] == '*' && p[1] == '/') {
                    లోపల_Cmt = అబద్ధం;
                    ++p;
                }
            } లేకపోతే ఒకవేళ (p[0] == '/' && p[1] == '/' && std::ఖాళీనా(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                ఆపు;  // Ignore to EOL
            } లేకపోతే ఒకవేళ (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                ఆపు;  // Ignore to EOL
            } లేకపోతే ఒకవేళ (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                లోపల_Cmt = నిజం;
                space_begin = అబద్ధం;
                ++p;
            } లేకపోతే {
                ఒకవేళ (!std::ఖాళీనా(*p)) space_begin = అబద్ధం;
                ఒకవేళ (!has_fargs) సంపూర్ణ_file += *p;
            }
        }
        ఒకవేళ (!has_fargs) సంపూర్ణ_file += " ";
    }
    సంపూర్ణ_file += "\n";  // So string match below is simplified
    ఒకవేళ (లోపల_Cmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = కొత్త File_పంక్తి{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::సదిశ<string> వాదనలు;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    గణన state : uint8_t {
        ST_లోపల_OPTION,
        ST_ESCAPED_CHAR,
        ST_లోపల_QUOTED_STR,
        ST_లోపల_DOUBLE_QUOTED_STR
    };

    state st = ST_లోపల_OPTION;
    state చివరి_st = ST_లోపల_OPTION;
    string arg;
    కొరకు (string::కొలత_type pos = 0; pos < సంపూర్ణ_file.నిడివి(); ++pos) {
        అక్షరం curr_char = సంపూర్ణ_file[pos];
        ఎంపిక (st) {
        స్థితి ST_లోపల_OPTION:  // Get all chars up to a white space or a "="
            ఒకవేళ (std::ఖాళీనా(curr_char)) {  // End of option
                ఒకవేళ (!arg.ఖాళీలు()) {  // End of word
                    వాదనలు.నెట్టు_వెనుక(arg);
                }
                arg = "";
                ఆపు;
            }
            ఒకవేళ (curr_char == '\\') {  // Escape char, we wait for next char
                చివరి_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                ఆపు;
            }
            ఒకవేళ (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                ఒకవేళ (pos < సంపూర్ణ_file.నిడివి()) curr_char = సంపూర్ణ_file[pos];
                ఒకవేళ (curr_char == '"') {  // String
                    st = ST_లోపల_QUOTED_STR;
                } లేకపోతే {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                ఆపు;
            }
            ఒకవేళ (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_లోపల_DOUBLE_QUOTED_STR;
                ఆపు;
            }
            arg += curr_char;
            ఆపు;
        స్థితి ST_లోపల_QUOTED_STR:  // Just store all chars inside string
            ఒకవేళ (curr_char != '\'') {
                arg += curr_char;
            } లేకపోతే {  // End of quoted string
                st = ST_లోపల_OPTION;
            }
            ఆపు;
        స్థితి ST_లోపల_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            ఒకవేళ (curr_char != '"') {
                ఒకవేళ (curr_char == '\\') {
                    చివరి_st = st;
                    st = ST_ESCAPED_CHAR;
                } లేకపోతే {
                    arg += curr_char;
                }
            } లేకపోతే {  // End of double quoted string
                st = ST_లోపల_OPTION;
            }
            ఆపు;
        స్థితి ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = చివరి_st;
            ఆపు;
        }
    }
    ఒకవేళ (!arg.ఖాళీలు()) {  // Add last word
        వాదనలు.నెట్టు_వెనుక(arg);
    }

    //మార్గం
    స్థిరరాశి string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::సదిశ<అక్షరం*> argv;
    argv.reserve(వాదనలు.కొలత() + 1);
    కొరకు (స్థిరరాశి string& i : వాదనలు) argv.నెట్టు_వెనుక(const_cast<అక్షరం*>(i.c_str()));
    argv.నెట్టు_వెనుక(శూన్యసూచిక);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<పూర్ణాంకం>(argv.కొలత() - 1), argv.దత్తాంశం());
}

//======================================================================

string V3Options::parseFileArg(స్థిరరాశి string& optdir, స్థిరరాశి string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    ఒకవేళ (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    తిరిగి filename;
}

//======================================================================

శూన్య V3Options::show_సంస్కరణ(తార్కిక verbose) {
    బయటకు << సంస్కరణ();
    బయటకు << "\n";
    ఒకవేళ (!verbose) తిరిగి;

    బయటకు << "\n";
    బయటకు << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    బయటకు << "redistribute it and/or modify the Verilator internals under the terms of\n";
    బయటకు << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    బయటకు << "License Version 2.0.\n";

    బయటకు << "\n";
    బయటకు << "See https://verilator.org for documentation\n";

    బయటకు << "\n";
    బయటకు << "Summary of configuration:\n";
    బయటకు << "  Compiled in defaults if not in environment:\n";
    బయటకు << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    బయటకు << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    బయటకు << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    బయటకు << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    బయటకు << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_మూలం << "\n";
    బయటకు << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    బయటకు << "    SystemC system-wide = " << cvt_గమ్యం_Str(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    బయటకు << "\n";
    బయటకు << "Environment:\n";
    బయటకు << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    బయటకు << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    బయటకు << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    బయటకు << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    బయటకు << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    బయటకు << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    బయటకు << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    బయటకు << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    బయటకు << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    బయటకు << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    బయటకు << "\n";
    బయటకు << "Supported features (compiled-in or forced by environment):\n";
    బయటకు << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    బయటకు << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    బయటకు << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = కొత్త V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_వాడని_Regexp = "*unused*";
    m_x_కేటాయించు = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3_Lang_సంకేతం::mostRecent();

    VName::max_నిడివి(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    కూడు_Lib_Ext_V("");  // So include "filename.v" will find the same file
    కూడు_Lib_Ext_V(".v");
    కూడు_Lib_Ext_V(".sv");
    // Default -I
    కూడు_Inc_Dir_Fallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(తొలగించు m_impp, m_impp = శూన్యసూచిక); }

శూన్య V3Options::సమితి_Debug_బహుళకం(పూర్ణాంకం level) {
    V3Error::debugDefault(level);
    ఒకవేళ (!m_dumpLevel.లెక్క("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = నిజం;
    m_debugCheck = నిజం;
    ఒకవేళ (level) {
        బయటకు << "- Starting " << సంస్కరణ() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

గుర్తులేని V3Options::debugLevel(స్థిరరాశి string& ట్యాగ్) స్థిరరాశి VL_MT_సురక్షిత {
    స్థిరరాశి స్వయంచాలక iter = m_debugLevel.కనుగొను(ట్యాగ్);
    తిరిగి iter != m_debugLevel.చివర() ? iter->రెండవ : V3Error::debugDefault();
}

గుర్తులేని V3Options::debugSrcLevel(స్థిరరాశి string& srcfile_మార్గం) స్థిరరాశి VL_MT_సురక్షిత {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    తిరిగి debugLevel(V3Os::filenameNonDirExt(srcfile_మార్గం));
}

గుర్తులేని V3Options::dumpLevel(స్థిరరాశి string& ట్యాగ్) స్థిరరాశి VL_MT_సురక్షిత {
    స్థిరరాశి స్వయంచాలక iter = m_dumpLevel.కనుగొను(ట్యాగ్);
    తిరిగి iter != m_dumpLevel.చివర() ? iter->రెండవ : 0;
}

గుర్తులేని V3Options::dumpSrcLevel(స్థిరరాశి string& srcfile_మార్గం) స్థిరరాశి VL_MT_సురక్షిత {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    తిరిగి dumpLevel(V3Os::filenameNonDirExt(srcfile_మార్గం));
}

తార్కిక V3Options::dump_వృక్షం_Addrids() స్థిరరాశి VL_MT_సురక్షిత {
    స్థిర పూర్ణాంకం s_level = -1;
    ఒకవేళ (VL_UNLIKELY(s_level < 0)) {
        స్థిరరాశి గుర్తులేని విలువ = dumpLevel("tree-addrids");
        ఒకవేళ (!అందుబాటు()) తిరిగి విలువ > 0;
        s_level = static_cast<గుర్తులేని>(విలువ);
    }
    తిరిగి s_level > 0;
}

శూన్య V3Options::optimize(పూర్ణాంకం level) {
    // Set all optimizations to on/off
    స్థిరరాశి తార్కిక flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_f_బిట్టు_పరిశీలన_Loops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_f_Case_పట్టిక = flag;
    m_f_Case_వృక్షం = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_బిట్టు_Op_వృక్షం = flag;
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
    m_f_విలీనం_Cond = flag;
    m_fReloop = flag;
    m_fReorder = flag;
    m_f_విభజించు = flag;
    m_fSubst = flag;
    m_fSubstConst = flag;
    m_f_పట్టిక = flag;
    // And set specific optimization levels
    ఒకవేళ (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
