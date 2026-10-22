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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Options.h"

#ಅನ್ತರ್ಭಾವ "V3Container.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"
#ಅನ್ತರ್ಭಾವ "V3OptionParser.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3PreShell.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

// clang-format off
#ಅನ್ತರ್ಭಾವ <sys/types.h>
#ಅನ್ತರ್ಭಾವ <sys/stat.h>
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ _WIN32
# include <sys/utsname.h>
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ <algorithm>
#ಅನ್ತರ್ಭಾವ <cctype>
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(ಬಹುಲಕ) (((ಬಹುಲಕ) & _S_IFMT) == _S_IFDIR)
#ಇಲ್ಲದಿದ್ದರೆ
# include <dirent.h>
#ಆದರೆ_ಅಂತ್ಯ
#ಅನ್ತರ್ಭಾವ <fcntl.h>
#ಅನ್ತರ್ಭಾವ <list>
#ಅನ್ತರ್ಭಾವ <map>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <thread>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>

#ಅನ್ತರ್ಭಾವ "config_rev.h"

#ಆದರೆ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

ವರ್ಗ V3OptionsImp final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    ಬಳಸಿ DirMap = std::map<ಸ್ಥಿರಾಂಕ string, std::ಗಣ<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_ಸಾಲು_ವಾದಗಳು;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, ತಾರ್ಕಿಕ>> m_all_ವಾದಗಳು;
    VInsertion_ಗಣ<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::ಗಣ<string> m_inc_Dir_Fallback_ಗಣ;  // Include directories (for removing duplicates)
    std::map<ಸ್ಥಿರಾಂಕ string, V3_Lang_ಸಂಕೇತ> m_langExts;  // Language extension map
    VInsertion_ಗಣ<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_Inc_Dir_User(ಸ್ಥಿರಾಂಕ string& incdir) {
        ಸ್ಥಿರಾಂಕ string& dir = V3Os::filenameCleanup(incdir);
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ inserted = m_incDirUsers.ಸೇರಿಸು(dir);
        ಆದರೆ (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_inc_Dir_Fallback_ಗಣ.erase(dir);  // User has priority over Fallback
        }
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Inc_Dir_Fallback(ಸ್ಥಿರಾಂಕ string& incdir) {
        ಸ್ಥಿರಾಂಕ string& dir = V3Os::filenameCleanup(incdir);
        ಆದರೆ (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it_ಸಿಕ್ಕಿತು_Pair = m_inc_Dir_Fallback_ಗಣ.ಸೇರಿಸು(dir);
            ಆದರೆ (it_ಸಿಕ್ಕಿತು_Pair.ಎರಡನೇ) m_incDirFallbacks.ತಳ್ಳು_ಹಿಂಭಾಗ(dir);
        }
    }
    ಶೂನ್ಯ ಕೂಡಿಸು_Lang_Ext(ಸ್ಥಿರಾಂಕ string& langext, ಸ್ಥಿರಾಂಕ V3_Lang_ಸಂಕೇತ& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        ಆದರೆ (addext[0] == '.') addext = addext.substr(1);
        (ಶೂನ್ಯ)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Lib_Ext_V(ಸ್ಥಿರಾಂಕ string& libext) { m_libExtVs.ಸೇರಿಸು(libext); }
    V3OptionsImp() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~V3OptionsImp() = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// V3LangCode class functions

V3_Lang_ಸಂಕೇತ::V3_Lang_ಸಂಕೇತ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp) {
    // Return code for given string, or ERROR, which is a bad code
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ codei = V3_Lang_ಸಂಕೇತ::L_ERROR; codei < V3_Lang_ಸಂಕೇತ::ENUM_ಕೊನೆ; ++codei) {
        ಸ್ಥಿರಾಂಕ V3_Lang_ಸಂಕೇತ ಸಂಕೇತ_2{codei};
        ಆದರೆ (0 == VL_STRCASECMP(textp, ಸಂಕೇತ_2.ascii())) {
            m_e = ಸಂಕೇತ_2;
            ಹಿಂತಿರುಗಿ;
        }
    }
    m_e = V3_Lang_ಸಂಕೇತ::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ಸ್ಥಿರಾಂಕ string& ಮೌಲ್ಯ, ತಾರ್ಕಿಕ& badr)
    : m_e{VTimescale::NONE} {
    badr = ನಿಜ;
    ಸ್ಥಿರಾಂಕ string spaceless = VString::removeWhitespace(ಮೌಲ್ಯ);
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = TS_100S; i < ENUM_ಕೊನೆ; ++i) {
        ಸ್ಥಿರಾಂಕ VTimescale ts{i};
        ಆದರೆ (spaceless == ts.ascii()) {
            badr = ಸುಳ್ಳು;
            m_e = ts.m_e;
            ನಿಲ್ಲಿಸು;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option::V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option(ಸ್ಥಿರಾಂಕ string& opts) {
    VStringList vals;
    ತಾರ್ಕಿಕ ಒಳ_Str = ಸುಳ್ಳು;
    string cur;
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ string hier_ಖಂಡ("--hierarchical-block");
    File_ಸಾಲು cmdfl(File_ಸಾಲು::command_ಸಾಲು_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    ಗಾಗಿ (string::const_iterator it = opts.begin(); it != opts.ಕೊನೆ();) {
        ಆದರೆ (ಒಳ_Str) {
            ಆದರೆ (*it == '\\') {
                ++it;
                ಆದರೆ (it == opts.ಕೊನೆ()) {
                    cmdfl.v3error(hier_ಖಂಡ + " must not end with \\");
                    ನಿಲ್ಲಿಸು;
                }
                ಆದರೆ (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hier_ಖಂಡ + " does not allow '" + *it + "' after \\");
                    ನಿಲ್ಲಿಸು;
                }
                cur.ತಳ್ಳು_ಹಿಂಭಾಗ(*it);
                ++it;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (*it == '"') {  // end of string
                cur.ತಳ್ಳು_ಹಿಂಭಾಗ(*it);
                vals.ತಳ್ಳು_ಹಿಂಭಾಗ(cur);
                cur.clear();
                ++it;
                ಆದರೆ (it != opts.ಕೊನೆ()) {
                    ಆದರೆ (*it != ',') {
                        cmdfl.v3error(hier_ಖಂಡ + " expects ',', but '" + *it + "' is passed");
                        ನಿಲ್ಲಿಸು;
                    }
                    ++it;
                    ಆದರೆ (it == opts.ಕೊನೆ()) {
                        cmdfl.v3error(hier_ಖಂಡ + " must not end with ','");
                        ನಿಲ್ಲಿಸು;
                    }
                    ಒಳ_Str = *it == '"';
                    cur.ತಳ್ಳು_ಹಿಂಭಾಗ(*it);
                    ++it;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {
                cur.ತಳ್ಳು_ಹಿಂಭಾಗ(*it);
                ++it;
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಆದರೆ (*it == '"') {
                cmdfl.v3error(hier_ಖಂಡ + " does not allow '\"' in the middle of literal");
                ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (*it == ',') {  // end of this parameter
                vals.ತಳ್ಳು_ಹಿಂಭಾಗ(cur);
                cur.clear();
                ++it;
                ಆದರೆ (it == opts.ಕೊನೆ()) {
                    cmdfl.v3error(hier_ಖಂಡ + " must not end with ','");
                    ನಿಲ್ಲಿಸು;
                }
                ಒಳ_Str = *it == '"';
            }
            cur.ತಳ್ಳು_ಹಿಂಭಾಗ(*it);
            ++it;
        }
    }
    ಆದರೆ (!cur.ಖಾಲಿಗಳು()) vals.ತಳ್ಳು_ಹಿಂಭಾಗ(cur);
    ಆದರೆ (vals.ಗಾತ್ರ_2() >= 2) {
        ಆದರೆ (vals.ಗಾತ್ರ_2() % 2) {
            cmdfl.v3error(hier_ಖಂಡ + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } ಇಲ್ಲದಿದ್ದರೆ {
        cmdfl.v3error(hier_ಖಂಡ + " requires at least two comma-separated values");
    }
    ಗಾಗಿ (ಗಾತ್ರ_t i = 2; i + 1 < vals.ಗಾತ್ರ_2(); i += 2) {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ inserted = m_parameters.emplace(vals[i], vals[i + 1]).ಎರಡನೇ;
        ಆದರೆ (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hier_ಖಂಡ);
        }
    }
}

//######################################################################
// V3Options class functions

ಶೂನ್ಯ VTimescale::parseSlashed(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* textp, VTimescale& unitr,
                              VTimescale& precr, ತಾರ್ಕಿಕ allow_ಖಾಲಿಗಳು) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* cp = textp;
    ಗಾಗಿ (; std::ಖಾಲಿಯೇ(*cp); ++cp) {}
    ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ unitp = cp;
    ಗಾಗಿ (; *cp && *cp != '/'; ++cp) {}
    ಸ್ಥಿರಾಂಕ string unitStr(unitp, cp - unitp);
    ಗಾಗಿ (; std::ಖಾಲಿಯೇ(*cp); ++cp) {}
    string precStr;
    ಆದರೆ (*cp == '/') {
        ++cp;
        ಗಾಗಿ (; std::ಖಾಲಿಯೇ(*cp); ++cp) {}
        ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ precp = cp;
        ಗಾಗಿ (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    ಗಾಗಿ (; std::ಖಾಲಿಯೇ(*cp); ++cp) {}
    ಆದರೆ (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        ಹಿಂತಿರುಗಿ;
    }

    ತಾರ್ಕಿಕ unitbad = ಸುಳ್ಳು;
    ಸ್ಥಿರಾಂಕ VTimescale unit{unitStr, unitbad /*ref*/};
    ಆದರೆ (unitbad && !(unitStr.ಖಾಲಿಗಳು() && allow_ಖಾಲಿಗಳು)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        ಹಿಂತಿರುಗಿ;
    }
    unitr = unit;

    ಆದರೆ (!precStr.ಖಾಲಿಗಳು()) {
        VTimescale prec{VTimescale::NONE};
        ತಾರ್ಕಿಕ precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        ಆದರೆ (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            ಹಿಂತಿರುಗಿ;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Inc_Dir_User(ಸ್ಥಿರಾಂಕ string& incdir) { m_impp->ಕೂಡಿಸು_Inc_Dir_User(incdir); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Inc_Dir_Fallback(ಸ್ಥಿರಾಂಕ string& incdir) { m_impp->ಕೂಡಿಸು_Inc_Dir_Fallback(incdir); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Lang_Ext(ಸ್ಥಿರಾಂಕ string& langext, ಸ್ಥಿರಾಂಕ V3_Lang_ಸಂಕೇತ& lc) {
    m_impp->ಕೂಡಿಸು_Lang_Ext(langext, lc);
}
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Lib_Ext_V(ಸ್ಥಿರಾಂಕ string& libext) { m_impp->ಕೂಡಿಸು_Lib_Ext_V(libext); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Define(ಸ್ಥಿರಾಂಕ string& defline, ತಾರ್ಕಿಕ allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ಎಡ = defline;
    ಆಗ (ಎಡ != "") {
        string def = ಎಡ;
        string::ಗಾತ್ರ_type pos;
        ಆದರೆ (allowPlus && ((pos = ಎಡ.ಕಂಡುಹಿಡಿ('+')) != string::npos)) {
            ಎಡ = ಎಡ.substr(pos + 1);
            def.erase(pos);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಎಡ = "";
        }
        string ಮೌಲ್ಯ;
        ಆದರೆ ((pos = def.ಕಂಡುಹಿಡಿ('=')) != string::npos) {
            ಮೌಲ್ಯ = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_ಸಾಲು(def, ಮೌಲ್ಯ);
    }
}
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Parameter(ಸ್ಥಿರಾಂಕ string& paramline, ತಾರ್ಕಿಕ allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ಎಡ = paramline;
    ಆಗ (ಎಡ != "") {
        string param = ಎಡ;
        string::ಗಾತ್ರ_type pos;
        ಆದರೆ (allowPlus && ((pos = ಎಡ.ಕಂಡುಹಿಡಿ('+')) != string::npos)) {
            ಎಡ = ಎಡ.substr(pos + 1);
            param.erase(pos);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಎಡ = "";
        }
        string ಮೌಲ್ಯ;
        ಆದರೆ ((pos = param.ಕಂಡುಹಿಡಿ('=')) != string::npos) {
            ಮೌಲ್ಯ = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << ಮೌಲ್ಯ);
        (ಶೂನ್ಯ)m_parameters.erase(param);
        m_parameters[param] = ಮೌಲ್ಯ;
    }
}

ತಾರ್ಕಿಕ V3Options::hasParameter(ಸ್ಥಿರಾಂಕ string& name) {
    ಹಿಂತಿರುಗಿ m_parameters.ಕಂಡುಹಿಡಿ(name) != m_parameters.ಕೊನೆ();
}

string V3Options::parameter(ಸ್ಥಿರಾಂಕ string& name) {
    string ಮೌಲ್ಯ = m_parameters.ಕಂಡುಹಿಡಿ(name)->ಎರಡನೇ;
    m_parameters.erase(m_parameters.ಕಂಡುಹಿಡಿ(name));
    ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ;
}

ಶೂನ್ಯ V3Options::checkParameters() {
    ಆದರೆ (!m_parameters.ಖಾಲಿಗಳು()) {
        std::stringstream ಸಂದೇಶ_2;
        ಸಂದೇಶ_2 << "Parameters from the command line were not found in the design:";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& i : m_parameters) ಸಂದೇಶ_2 << " " << i.ಮೊದಲ;
        v3error(ಸಂದೇಶ_2.str());
    }
}

ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Cpp_File(ಸ್ಥಿರಾಂಕ string& filename) { m_cppFiles.ಸೇರಿಸು(filename); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_CFlags(ಸ್ಥಿರಾಂಕ string& filename) { m_cFlags.ತಳ್ಳು_ಹಿಂಭಾಗ(filename); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Lib_Map_File(ಸ್ಥಿರಾಂಕ string& filename) { m_libmapFiles.ಸೇರಿಸು(filename); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Compiler_Includes(ಸ್ಥಿರಾಂಕ string& filename) {
    m_compilerIncludes.ಸೇರಿಸು(filename);
}
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Ld_Lib_File(ಸ್ಥಿರಾಂಕ string& filename) {
    m_ldLibFiles.ತಳ್ಳು_ಹಿಂಭಾಗ(filename);
    ಕೂಡಿಸು_Ld_Libs(filename);
}
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Ld_Libs(ಸ್ಥಿರಾಂಕ string& filename) { m_ldLibs.ತಳ್ಳು_ಹಿಂಭಾಗ(filename); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Make_Flags(ಸ್ಥಿರಾಂಕ string& filename) { m_makeFlags.ತಳ್ಳು_ಹಿಂಭಾಗ(filename); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Future(ಸ್ಥಿರಾಂಕ string& flag) { m_futures.ಸೇರಿಸು(flag); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Future0(ಸ್ಥಿರಾಂಕ string& flag) { m_future0s.ಸೇರಿಸು(flag); }
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Future1(ಸ್ಥಿರಾಂಕ string& flag) { m_future1s.ಸೇರಿಸು(flag); }
ತಾರ್ಕಿಕ V3Options::isFuture(ಸ್ಥಿರಾಂಕ string& flag) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ m_futures.ಕಂಡುಹಿಡಿ(flag) != m_futures.ಕೊನೆ();
}
ತಾರ್ಕಿಕ V3Options::isFuture0(ಸ್ಥಿರಾಂಕ string& flag) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ m_future0s.ಕಂಡುಹಿಡಿ(flag) != m_future0s.ಕೊನೆ();
}
ತಾರ್ಕಿಕ V3Options::isFuture1(ಸ್ಥಿರಾಂಕ string& flag) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ m_future1s.ಕಂಡುಹಿಡಿ(flag) != m_future1s.ಕೊನೆ();
}
ತಾರ್ಕಿಕ V3Options::isLibraryFile(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname) ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ m_libraryFiles.ಕಂಡುಹಿಡಿ({filename, libname}) != m_libraryFiles.ಕೊನೆ();
}
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Library_File(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname) {
    m_libraryFiles.ಸೇರಿಸು({filename, libname});
}
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_VFile(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ತಳ್ಳು_ಹಿಂಭಾಗ({filename, libname});  // Not emplace
}
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Vlt_File(ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& libname) {
    m_vltFiles.ಸೇರಿಸು({filename, libname});
}
ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Force_Inc(ಸ್ಥಿರಾಂಕ string& filename) { m_forceIncs.ತಳ್ಳು_ಹಿಂಭಾಗ(filename); }

ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_ಸಾಲು_Arg(ಸ್ಥಿರಾಂಕ string& arg) { m_impp->m_ಸಾಲು_ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(arg); }

ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Arg(ಅಕ್ಷರ** argv, ಗಾತ್ರ_t ಎಣಿಕೆ_2, ತಾರ್ಕಿಕ isForRerun) {
    m_impp->m_all_ವಾದಗಳು.emplace_ಹಿಂಭಾಗ();
    std::pair<std::list<std::string>, ತಾರ್ಕಿಕ>& pair = m_impp->m_all_ವಾದಗಳು.ಹಿಂಭಾಗ();
    pair.ಎರಡನೇ = isForRerun;
    ಗಾಗಿ (ಗಾತ್ರ_t n = 0; n < ಎಣಿಕೆ_2; ++n) pair.ಮೊದಲ.emplace_ಹಿಂಭಾಗ(argv[n]);
}

ಶೂನ್ಯ V3Options::ಕೂಡಿಸು_Arg(ಸ್ಥಿರಾಂಕ std::string& arg, ತಾರ್ಕಿಕ isForRerun) {
    m_impp->m_all_ವಾದಗಳು.emplace_ಹಿಂಭಾಗ();
    std::pair<std::list<std::string>, ತಾರ್ಕಿಕ>& pair = m_impp->m_all_ವಾದಗಳು.ಹಿಂಭಾಗ();
    pair.ಎರಡನೇ = isForRerun;
    pair.ಮೊದಲ.emplace_ಹಿಂಭಾಗ(arg);
}

ಸ್ಥಿರಾಂಕ std::list<std::pair<std::list<std::string>, ತಾರ್ಕಿಕ>>& V3Options::all_ವಾದಗಳು() ಸ್ಥಿರಾಂಕ {
    ಹಿಂತಿರುಗಿ m_impp->m_all_ವಾದಗಳು;
}

string V3Options::all_ವಾದಗಳು_String() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    string ಫಲಿತಾಂಶ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_impp->m_all_ವಾದಗಳು) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& arg : pair.ಮೊದಲ) {
            ಆದರೆ (!ಫಲಿತಾಂಶ.ಖಾಲಿಗಳು()) ಫಲಿತಾಂಶ += " ";
            ಫಲಿತಾಂಶ += arg;
        }
    }
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::all_ವಾದಗಳು_String_For_Hier_ಖಂಡ(ತಾರ್ಕಿಕ for_ತುದಿ) ಸ್ಥಿರಾಂಕ {
    // cppcheck-suppress shadowFunction
    std::ಗಣ<string> vFiles;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& vFile : m_vFiles) vFiles.ಸೇರಿಸು(vFile.filename());
    string ಹೊರಹೋಗು;
    ತಾರ್ಕಿಕ stripArg = ಸುಳ್ಳು;
    ತಾರ್ಕಿಕ strip_Arg_If_ಅಂಕೆ = ಸುಳ್ಳು;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& pair : m_impp->m_all_ವಾದಗಳು) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& arg : pair.ಮೊದಲ) {
            ಆದರೆ (stripArg) {
                stripArg = ಸುಳ್ಳು;
                ಮುಂದುವರಿ;
            }
            ಆದರೆ (strip_Arg_If_ಅಂಕೆ) {
                strip_Arg_If_ಅಂಕೆ = ಸುಳ್ಳು;
                ಆದರೆ (ಅಂಕಿಯೇ(arg[0])) ಮುಂದುವರಿ;
            }
            ಪೂರ್ಣಾಂಕ skip = 0;
            ಆದರೆ (arg.ಉದ್ದಳತೆ() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (arg.ಉದ್ದಳತೆ() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            ಆದರೆ (skip > 0) {  // arg is an option
                ಸ್ಥಿರಾಂಕ string opt = arg.substr(skip);  // Remove '-' in the beginning
                ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಂಕೆ_Strip = strip_Options_For_ಮಗು_ಸತತ(opt, for_ತುದಿ);
                ಆದರೆ (ಅಂಕೆ_Strip) {
                    UASSERT(0 <= ಅಂಕೆ_Strip && ಅಂಕೆ_Strip <= 3, "should be one of 0, 1, 2, 3");
                    ಆದರೆ (ಅಂಕೆ_Strip == 2) stripArg = ನಿಜ;
                    ಆದರೆ (ಅಂಕೆ_Strip == 3) strip_Arg_If_ಅಂಕೆ = ನಿಜ;
                    ಮುಂದುವರಿ;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {  // Not an option
                ಆದರೆ (vFiles.ಕಂಡುಹಿಡಿ(arg) != vFiles.ಕೊನೆ()  // Remove HDL
                    || m_cppFiles.ಕಂಡುಹಿಡಿ(arg) != m_cppFiles.ಕೊನೆ()) {  // Remove C++
                    ಮುಂದುವರಿ;
                }
            }
            ಆದರೆ (ಹೊರಹೋಗು != "") ಹೊರಹೋಗು += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            ಹೊರಹೋಗು += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು;
}

ಶೂನ್ಯ V3Options::cc_ಗಣ() {  // --cc
    m_ಹೊರಹೋಗು_Format_Ok = ನಿಜ;
    m_systemC = ಸುಳ್ಳು;
}

ಶೂನ್ಯ V3Options::decorations(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& arg) {  // --decorations
    ಆದರೆ (arg == "none") {
        m_decoration = ಸುಳ್ಳು;
        m_decorationNodes = ಸುಳ್ಳು;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (arg == "node") {
        m_decoration = ನಿಜ;
        m_decorationNodes = ನಿಜ;
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (arg == "medium") {
        m_decoration = ನಿಜ;
        m_decorationNodes = ಸುಳ್ಳು;
    } ಇಲ್ಲದಿದ್ದರೆ {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warn_ಇನ್ನಷ್ಟು() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::ಸದಿಶ<std::string> V3Options::traceClassBases() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    std::ಸದಿಶ<std::string> ಫಲಿತಾಂಶ;
    ಆದರೆ (traceEnabledFst()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("VerilatedFst");
    ಆದರೆ (traceEnabledSaif()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("VerilatedSaif");
    ಆದರೆ (traceEnabledVcd()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("VerilatedVcd");
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
std::ಸದಿಶ<std::string> V3Options::traceClassLangs() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    std::ಸದಿಶ<std::string> ಫಲಿತಾಂಶ;
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& cbase : traceClassBases())
        ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ(cbase + (systemC() ? "Sc" : "C"));
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
std::ಸದಿಶ<std::string> V3Options::trace_ಮೂಲ_Bases() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    std::ಸದಿಶ<std::string> ಫಲಿತಾಂಶ;
    ಆದರೆ (traceEnabledFst()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_fst");
    ಆದರೆ (traceEnabledSaif()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_saif");
    ಆದರೆ (traceEnabledVcd()) ಫಲಿತಾಂಶ.emplace_ಹಿಂಭಾಗ("verilated_vcd");
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
std::ಸದಿಶ<std::string> V3Options::trace_ಮೂಲ_Langs() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    std::ಸದಿಶ<std::string> ಫಲಿತಾಂಶ = trace_ಮೂಲ_Bases();
    ಗಾಗಿ (std::string& str : ಫಲಿತಾಂಶ) str += systemC() ? "_sc"s : "_c"s;
    ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
}
std::string V3Options::trace_Class_ತಳ() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().ಖಾಲಿಗಳು(), "Call traceClassBase only when trace() enabled");
    ಹಿಂತಿರುಗಿ traceClassBases().front();
}
std::string V3Options::traceClassLang() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().ಖಾಲಿಗಳು(), "Call traceClassLang only when trace() enabled");
    ಹಿಂತಿರುಗಿ traceClassLangs().front();
}

//######################################################################
// File searching

ತಾರ್ಕಿಕ V3Options::file_Stat_ಸಾಮಾನ್ಯ(ಸ್ಥಿರಾಂಕ string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ರಚನೆ stat sstat;  // Stat information
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ err = stat(filename.c_str(), &sstat);
    ಆದರೆ (err != 0) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಆದರೆ (S_ISDIR(sstat.st_ಬಹುಲಕ)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ ನಿಜ;
}

string V3Options::fileExists(ಸ್ಥಿರಾಂಕ string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ಸ್ಥಿರಾಂಕ string dir = V3Os::filenameDir(filename);
    ಸ್ಥಿರಾಂಕ string basename = V3Os::filenameNonDir(filename);

    ಸ್ವಯಂಚಾಲಿತ diriter = m_impp->m_dirMap.ಕಂಡುಹಿಡಿ(dir);
    ಆದರೆ (diriter == m_impp->m_dirMap.ಕೊನೆ()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::ಗಣ<string>());
        diriter = m_impp->m_dirMap.ಕಂಡುಹಿಡಿ(dir);

        std::ಗಣ<string>* ಸ್ಥಿರಾಂಕ setp = &(diriter->ಎರಡನೇ);

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ _MSC_VER
        ಪ್ರಯತ್ನಿಸು {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& dir_ನಮೂದು : std::filesystem::directory_iterator(dir.c_str()))
                setp->ಸೇರಿಸು(dir_ನಮೂದು.ಮಾರ್ಗ().filename().string());
        } ಹಿಡಿ (std::filesystem::filesystem_error ಸ್ಥಿರಾಂಕ& ex) {
            (ಶೂನ್ಯ)ex;
            ಹಿಂತಿರುಗಿ "";
        }
#ಇಲ್ಲದಿದ್ದರೆ
        ಆದರೆ (DIR* ಸ್ಥಿರಾಂಕ dirp = opendir(dir.c_str())) {
            ಆಗ (ರಚನೆ dirent* direntp = readdir(dirp)) setp->ಸೇರಿಸು(direntp->d_name);
            closedir(dirp);
        }
#ಆದರೆ_ಅಂತ್ಯ
    }
    // Find it
    ಸ್ಥಿರಾಂಕ std::ಗಣ<string>* ಸ್ಥಿರಾಂಕ filesetp = &(diriter->ಎರಡನೇ);
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ fileiter = filesetp->ಕಂಡುಹಿಡಿ(basename);
    ಆದರೆ (fileiter == filesetp->ಕೊನೆ()) ಹಿಂತಿರುಗಿ "";  // Not found
    // Check if it is a directory, ignore if so
    ಸ್ಥಿರಾಂಕ string filename_ಹೊರಹೋಗು = V3Os::filenameJoin(dir, basename);
    ಆದರೆ (!file_Stat_ಸಾಮಾನ್ಯ(filename_ಹೊರಹೋಗು)) ಹಿಂತಿರುಗಿ "";  // Directory
    ಹಿಂತಿರುಗಿ filename_ಹೊರಹೋಗು;
}

string V3Options::file_ಮಾರ್ಗ_Check_One_Dir(ಸ್ಥಿರಾಂಕ string& modname, ಸ್ಥಿರಾಂಕ string& dirname) {
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : m_impp->m_libExtVs) {
        ಸ್ಥಿರಾಂಕ string fn = V3Os::filenameJoin(dirname, modname + i);
        ಸ್ಥಿರಾಂಕ string exists = fileExists(fn);
        ಆದರೆ (exists != "") ಹಿಂತಿರುಗಿ exists;
    }
    ಹಿಂತಿರುಗಿ "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
ಪೂರ್ಣಾಂಕ V3Options::strip_Options_For_ಮಗು_ಸತತ(ಸ್ಥಿರಾಂಕ string& opt, ತಾರ್ಕಿಕ for_ತುದಿ) {
    // Options to strip for both the top wrapper, and hier blocks
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ std::unordered_map<std::string, ಪೂರ್ಣಾಂಕ> commonOpts{
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
    ಆದರೆ (commonOpts.ಎಣಿಕೆ_2(opt)) ಹಿಂತಿರುಗಿ commonOpts.at(opt);

    // Options to strip only for hier blocks
    ಆದರೆ (!for_ತುದಿ) {
        ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ std::unordered_map<std::string, ಪೂರ್ಣಾಂಕ> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        ಆದರೆ (subOpts.ಎಣಿಕೆ_2(opt)) ಹಿಂತಿರುಗಿ subOpts.at(opt);
        ಆದರೆ (VString::startsWith(opt, "G")) ಹಿಂತಿರುಗಿ 1;
        ಆದರೆ (VString::startsWith(opt, "pvalue")) ಹಿಂತಿರುಗಿ 1;
    }

    // Do not strip
    ಹಿಂತಿರುಗಿ 0;
}

ಶೂನ್ಯ V3Options::validateIdentifier(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& arg, ಸ್ಥಿರಾಂಕ string& opt) {
    ಆದರೆ (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::file_ಮಾರ್ಗ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& modname, ಸ್ಥಿರಾಂಕ string& lastpath,
                           ಸ್ಥಿರಾಂಕ string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ಸ್ಥಿರಾಂಕ string filename = V3Os::filenameCleanup(VName::dehash(modname));
    ಆದರೆ (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ಸ್ಥಿರಾಂಕ string exists = file_ಮಾರ್ಗ_Check_One_Dir(filename, "");
        ಆದರೆ (exists != "") ಹಿಂತಿರುಗಿ exists;
    }
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& dir : m_impp->m_incDirUsers) {
        ಸ್ಥಿರಾಂಕ string exists = file_ಮಾರ್ಗ_Check_One_Dir(filename, dir);
        ಆದರೆ (exists != "") ಹಿಂತಿರುಗಿ exists;
    }
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& dir : m_impp->m_incDirFallbacks) {
        ಸ್ಥಿರಾಂಕ string exists = file_ಮಾರ್ಗ_Check_One_Dir(filename, dir);
        ಆದರೆ (exists != "") ಹಿಂತಿರುಗಿ exists;
    }

    ಆದರೆ (m_relativeIncludes) {
        ಸ್ಥಿರಾಂಕ string exists = file_ಮಾರ್ಗ_Check_One_Dir(filename, lastpath);
        ಆದರೆ (exists != "") ಹಿಂತಿರುಗಿ V3Os::filename_Real_ಮಾರ್ಗ(exists);
    }

    // Warn and return not found
    ಆದರೆ (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_ಮಾರ್ಗ_Looked_ಸಂದೇಶ(fl, filename));
    }
    ಹಿಂತಿರುಗಿ "";
}

string V3Options::file_ಮಾರ್ಗ_Looked_ಸಂದೇಶ(File_ಸಾಲು* /*fl*/, ಸ್ಥಿರಾಂಕ string& modname) {
    ಸ್ಥಿರ ತಾರ್ಕಿಕ s_shown_notfound_ಸಂದೇಶ = ಸುಳ್ಳು;
    std::ostringstream ss;
    ಆದರೆ (modname.ಕಂಡುಹಿಡಿ("__Vhsh") != string::npos) {
        ss << V3Error::warn_ಇನ್ನಷ್ಟು() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warn_ಇನ್ನಷ್ಟು() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!s_shown_notfound_ಸಂದೇಶ) {
        s_shown_notfound_ಸಂದೇಶ = ನಿಜ;
        ಆದರೆ (m_impp->m_incDirUsers.ಖಾಲಿಗಳು()) {
            ss << V3Error::warn_ಇನ್ನಷ್ಟು()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warn_ಇನ್ನಷ್ಟು() << "... Looked in:\n";
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& dir : m_impp->m_incDirUsers) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& ext : m_impp->m_libExtVs) {
                ಸ್ಥಿರಾಂಕ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_ಇನ್ನಷ್ಟು() << "     " << fn << "\n";
            }
        }
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& dir : m_impp->m_incDirFallbacks) {
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& ext : m_impp->m_libExtVs) {
                ಸ್ಥಿರಾಂಕ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_ಇನ್ನಷ್ಟು() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warn_ಇನ್ನಷ್ಟು() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    ಹಿಂತಿರುಗಿ ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3_Lang_ಸಂಕೇತ V3Options::fileLanguage(ಸ್ಥಿರಾಂಕ string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::ಗಾತ್ರ_type pos;
    ಆದರೆ (filename == V3Options::get_Std_Package_ಮಾರ್ಗ() || filename == V3Options::get_Std_Waiver_ಮಾರ್ಗ()) {
        ಹಿಂತಿರುಗಿ V3_Lang_ಸಂಕೇತ::mostRecent();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = m_impp->m_langExts.ಕಂಡುಹಿಡಿ(ext);
        ಆದರೆ (it != m_impp->m_langExts.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    }
    ಹಿಂತಿರುಗಿ m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ಸ್ಥಿರಾಂಕ string& var) {
    // If update below, also update V3Options::showVersion()
    ಆದರೆ (var == "MAKE") {
        ಹಿಂತಿರುಗಿ getenvMAKE();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "PERL") {
        ಹಿಂತಿರುಗಿ getenvPERL();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "PYTHON3") {
        ಹಿಂತಿರುಗಿ getenvPYTHON3();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "SYSTEMC") {
        ಹಿಂತಿರುಗಿ getenvSYSTEMC();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "SYSTEMC_ARCH") {
        ಹಿಂತಿರುಗಿ getenvSYSTEMC_ARCH();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "SYSTEMC_INCLUDE") {
        ಹಿಂತಿರುಗಿ getenvSYSTEMC_INCLUDE();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "SYSTEMC_LIBDIR") {
        ಹಿಂತಿರುಗಿ getenvSYSTEMC_LIBDIR();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "VERILATOR_ROOT") {
        ಹಿಂತಿರುಗಿ getenv_VERILATOR_ಬೇರು();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ V3Os::getenvStr(var, "");
    }
}

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ __FreeBSD__
string V3Options::getenvMAKE() { ಹಿಂತಿರುಗಿ V3Os::getenvStr("MAKE", "gmake"); }
#ಇಲ್ಲದಿದ್ದರೆ
string V3Options::getenvMAKE() { ಹಿಂತಿರುಗಿ V3Os::getenvStr("MAKE", "make"); }
#ಆದರೆ_ಅಂತ್ಯ

string V3Options::getenvMAKEFLAGS() {  //
    ಹಿಂತಿರುಗಿ V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    ಹಿಂತಿರುಗಿ V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    ಹಿಂತಿರುಗಿ V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಸ್ಥಿರಾಂಕ string defenv = string{DEFENV_SYSTEMC};
    ಆದರೆ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    ಹಿಂತಿರುಗಿ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಸ್ಥಿರಾಂಕ string defenv = string{DEFENV_SYSTEMC_ARCH};
    ಆದರೆ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    ಆದರೆ (var == "") {
#ಆದರೆ defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ಸ್ಥಿರಾಂಕ string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#ಅಥವಾಆದರೆ defined(_WIN32)
        ಸ್ಥಿರಾಂಕ string sysname = "WIN32";
        var = "win32";
#ಇಲ್ಲದಿದ್ದರೆ
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ರಚನೆ utsname uts;
        uname(&uts);
        ಸ್ಥಿರಾಂಕ string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        ಆದರೆ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } ಇಲ್ಲದಿದ್ದರೆ {
            var = "linux";
        }
#ಆದರೆ_ಅಂತ್ಯ
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    ಹಿಂತಿರುಗಿ var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಸ್ಥಿರಾಂಕ string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    ಆದರೆ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    ಆದರೆ (var == "") {
        ಸ್ಥಿರಾಂಕ string sc = getenvSYSTEMC();
        ಆದರೆ (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    ಹಿಂತಿರುಗಿ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಸ್ಥಿರಾಂಕ string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    ಆದರೆ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    ಆದರೆ (var == "") {
        ಸ್ಥಿರಾಂಕ string sc = getenvSYSTEMC();
        ಸ್ಥಿರಾಂಕ string arch = getenvSYSTEMC_ARCH();
        ಆದರೆ (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    ಹಿಂತಿರುಗಿ V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_ಬೇರು() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಸ್ಥಿರಾಂಕ string defenv = string{DEFENV_VERILATOR_ಬೇರು};
    ಆದರೆ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    ಆದರೆ (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    ಹಿಂತಿರುಗಿ V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ಸ್ಥಿರಾಂಕ string defenv = string{DEFENV_VERILATOR_SOLVER};
    ಆದರೆ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    ಹಿಂತಿರುಗಿ var;
}

string V3Options::get_Std_Package_ಮಾರ್ಗ() {
    ಹಿಂತಿರುಗಿ V3Os::filenameJoin(getenv_VERILATOR_ಬೇರು(), "include", "verilated_std.sv");
}
string V3Options::get_Std_Waiver_ಮಾರ್ಗ() {
    ಹಿಂತಿರುಗಿ V3Os::filenameJoin(getenv_VERILATOR_ಬೇರು(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ಸ್ಥಿರಾಂಕ string& var) {
    // If update below, also update V3Options::showVersion()
    ಆದರೆ (var == "COROUTINES" && coroutineSupport()) {
        ಹಿಂತಿರುಗಿ "1";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "TSAN" && tsanSupport()) {
        ಹಿಂತಿರುಗಿ "1";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "DEV_ASAN" && devAsan()) {
        ಹಿಂತಿರುಗಿ "1";
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "DEV_GCOV" && devGcov()) {
        ಹಿಂತಿರುಗಿ "1";
        // cppcheck-suppress knownConditionTrueFalse
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (var == "SYSTEMC" && systemCFound()) {
        ಹಿಂತಿರುಗಿ "1";
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ "";
    }
}

ತಾರ್ಕಿಕ V3Options::systemCSystemWide() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_SYSTEMC
    ಹಿಂತಿರುಗಿ ನಿಜ;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
#ಆದರೆ_ಅಂತ್ಯ
}

ತಾರ್ಕಿಕ V3Options::systemCFound() {
    ಹಿಂತಿರುಗಿ (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().ಖಾಲಿಗಳು() && !getenvSYSTEMC_LIBDIR().ಖಾಲಿಗಳು()));
}

ತಾರ್ಕಿಕ V3Options::coroutineSupport() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_COROUTINES
    ಹಿಂತಿರುಗಿ ನಿಜ;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
#ಆದರೆ_ಅಂತ್ಯ
}

ತಾರ್ಕಿಕ V3Options::devAsan() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_DEV_ASAN
    ಹಿಂತಿರುಗಿ ನಿಜ;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
#ಆದರೆ_ಅಂತ್ಯ
}

ತಾರ್ಕಿಕ V3Options::devGcov() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_DEV_GCOV
    ಹಿಂತಿರುಗಿ ನಿಜ;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
#ಆದರೆ_ಅಂತ್ಯ
}

ತಾರ್ಕಿಕ V3Options::tsanSupport() {
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ HAVE_TSAN
    ಹಿಂತಿರುಗಿ ನಿಜ;
#ಇಲ್ಲದಿದ್ದರೆ
    ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
#ಆದರೆ_ಅಂತ್ಯ
}

//######################################################################
// V3 Options notification methods

ಶೂನ್ಯ V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_ಸಾಲು* ಸ್ಥಿರಾಂಕ cmdfl = ಹೊಸ File_ಸಾಲು{File_ಸಾಲು::command_ಸಾಲು_Filename()};

    ಆದರೆ (!ಹೊರಹೋಗು_Format_Ok() && v3Global.opt.ಮುಖ್ಯ()) cc_ಗಣ();  // --main implies --cc if not provided
    ಆದರೆ (!ಹೊರಹೋಗು_Format_Ok() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    ಆದರೆ (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::ಸದಿಶ<std::string> backendFlags;
    ಆದರೆ (m_build) {
        ಆದರೆ (m_ದ್ವಿಮಾನ)
            backendFlags.emplace_ಹಿಂಭಾಗ("--binary");
        ಇಲ್ಲದಿದ್ದರೆ
            backendFlags.emplace_ಹಿಂಭಾಗ("--build");
    }
    ಆದರೆ (m_preprocOnly) backendFlags.emplace_ಹಿಂಭಾಗ("-E");
    ಆದರೆ (m_dpiHdrOnly) backendFlags.emplace_ಹಿಂಭಾಗ("--dpi-hdr-only");
    ಆದರೆ (m_lintOnly) backendFlags.emplace_ಹಿಂಭಾಗ("--lint-only");
    ಆದರೆ (m_jsonOnly) backendFlags.emplace_ಹಿಂಭಾಗ("--json-only");
    ಆದರೆ (backendFlags.ಗಾತ್ರ_2() > 1) {
        std::string backendFlagsString = backendFlags.front();
        ಗಾಗಿ (ಗಾತ್ರ_t i = 1; i < backendFlags.ಗಾತ್ರ_2(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    ಆದರೆ (m_exe && !v3Global.opt.libCreate().ಖಾಲಿಗಳು()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    ಆದರೆ (!m_gmake && !m_makeJson) m_gmake = ನಿಜ;

    ಆದರೆ (m_ಶ್ರೇಣೀಕೃತ && (m_hier_ಮಗು || !m_hier_ಖಂಡಗಳು.ಖಾಲಿಗಳು())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    ಆದರೆ (m_hier_ಮಗು) {
        ಆದರೆ (m_hier_ಖಂಡಗಳು.ಖಾಲಿಗಳು()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = ಸುಳ್ಳು;
    }

    ಆದರೆ (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = ನಿಜ;  // No format, with --trace means wanted --trace-vcd
    }
    ಆದರೆ (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = ನಿಜ;
    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    ಆದರೆ (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    ಆದರೆ (protectIds()) {
        ಆದರೆ (all_ಬಹಿರಂಗ()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warn_ಇನ್ನಷ್ಟು() + "... Suggest remove --public.");
        }
        ಆದರೆ (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warn_ಇನ್ನಷ್ಟು() + "... Suggest remove --trace.");
        }
        ಆದರೆ (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warn_ಇನ್ನಷ್ಟು() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    ಆದರೆ (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.ಗಣ_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    ಆದರೆ (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.ಗಣ_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    ಆದರೆ (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.ಗಣ_True_Or_False(ನಿಜ);

    // Default split limits if not specified
    ಆದರೆ (m_output_ವಿಭಜಿಸು_CFuncs < 0) m_output_ವಿಭಜಿಸು_CFuncs = m_output_ವಿಭಜಿಸು;
    ಆದರೆ (m_output_ವಿಭಜಿಸು_CTrace < 0) m_output_ವಿಭಜಿಸು_CTrace = m_output_ವಿಭಜಿಸು;

    ಆದರೆ (v3Global.opt.ಮುಖ್ಯ() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    ಆದರೆ (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        ಆದರೆ (ಶ್ರೇಣೀಕೃತ()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    ಆದರೆ (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    ಆದರೆ (v3Global.opt.timing().is_ಗಣ_True() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    ಆದರೆ (!m_dumpLevel.ಎಣಿಕೆ_2("tree") && m_dumpLevel.ಎಣಿಕೆ_2("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(ಎಳೆಗಳು() >= 1, "'threads()' must return a value >= 1");
    ಆದರೆ (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    ಆದರೆ (m_buildJobs == -1) m_buildJobs = 1;
    ಆದರೆ (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    ಆದರೆ (timing().is_ಗಣ_True()) V3PreShell::define_Cmd_ಸಾಲು("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    ಆದರೆ (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.ಗಣ_True_Or_False(ಸುಳ್ಳು);

    // === Leave last
    // Mark options as available
    m_ಲಭ್ಯ = ನಿಜ;
}

//######################################################################
// V3 Options accessors

string V3Options::ಆವೃತ್ತಿ() VL_ಶುದ್ಧ {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_ಗಮ್ಯ_Str(DTVERSION_rev);
    ಹಿಂತಿರುಗಿ ver;
}

string V3Options::protect_ಕೀಲಿ_Defaulted() VL_MT_ಸುರಕ್ಷಿತ {
    ಸ್ಥಿರ V3Mutex s_mutex;
    ಸ್ಥಿರಾಂಕ V3_ಬೀಗ_Guard ಬೀಗ_2{s_mutex};
    ಆದರೆ (m_protect_ಕೀಲಿ.ಖಾಲಿಗಳು()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_ಕೀಲಿ = "VL-KEY-" + digest.digestBase64();
    }
    ಹಿಂತಿರುಗಿ m_protect_ಕೀಲಿ;
}

ಶೂನ್ಯ V3Options::throwSigsegv() {  // LCOV_EXCL_START
#ಆದರೆ !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<ಅಸ್ಥಿರ ಅಕ್ಷರ*>(ಶೂನ್ಯಸೂಚಕ) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#ಆದರೆ_ಅಂತ್ಯ
}  // LCOV_EXCL_STOP

VTimescale V3Options::ಸಮಯ_Compute_Prec(ಸ್ಥಿರಾಂಕ VTimescale& flag) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!ಸಮಯ_Override_Prec().isNone()) {
        ಹಿಂತಿರುಗಿ ಸಮಯ_Override_Prec();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (flag.isNone()) {
        ಹಿಂತಿರುಗಿ ಸಮಯ_Default_Prec();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ flag;
    }
}

VTimescale V3Options::ಸಮಯ_Compute_Unit(ಸ್ಥಿರಾಂಕ VTimescale& flag) ಸ್ಥಿರಾಂಕ {
    ಆದರೆ (!ಸಮಯ_Override_Unit().isNone()) {
        ಹಿಂತಿರುಗಿ ಸಮಯ_Override_Unit();
    } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (flag.isNone()) {
        ಹಿಂತಿರುಗಿ ಸಮಯ_Default_Unit();
    } ಇಲ್ಲದಿದ್ದರೆ {
        ಹಿಂತಿರುಗಿ flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ** argv) {
    // Return list of arguments as simple string
    string opts;
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < argc; ++i) {
        ಆದರೆ (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    ಹಿಂತಿರುಗಿ opts;
}

//######################################################################
// V3 Options Parsing

ಶೂನ್ಯ V3Options::parseOpts(File_ಸಾಲು* fl, ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ** argv) VL_MT_DISABLED {
    // Save command line options
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < argc; ++i) ಕೂಡಿಸು_ಸಾಲು_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    ಆದರೆ (vFiles().ಖಾಲಿಗಳು()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    ಆದರೆ (ಪೂರ್ವಪ್ರತ್ಯಯ() == "" && ತುದಿ_Module() != "") m_ಪೂರ್ವಪ್ರತ್ಯಯ = "V"s + ತುದಿ_Module();  // Already encoded
    ಆದರೆ (ಪೂರ್ವಪ್ರತ್ಯಯ() == "" && vFiles().ಗಾತ್ರ_2() >= 1)
        m_ಪೂರ್ವಪ್ರತ್ಯಯ
            = "V"s + Ast_ಗಂಟು::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    ಆದರೆ (mod_ಪೂರ್ವಪ್ರತ್ಯಯ() == "") m_mod_ಪೂರ್ವಪ್ರತ್ಯಯ = ಪೂರ್ವಪ್ರತ್ಯಯ();

    // Find files in makedir
    ಕೂಡಿಸು_Inc_Dir_Fallback(makeDir());
}

//======================================================================

ತಾರ್ಕಿಕ V3Options::suffixed(ಸ್ಥಿರಾಂಕ string& sw, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* arg) {
    ಆದರೆ (std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(arg) > sw.ಉದ್ದಳತೆ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    ಹಿಂತಿರುಗಿ (0 == std::strcmp(sw.c_str() + sw.ಉದ್ದಳತೆ() - std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ(arg), arg));
}

ಶೂನ್ಯ V3Options::parseOptsList(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& optdir, ಪೂರ್ಣಾಂಕ argc,
                              ಅಕ್ಷರ** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ಸ್ಥಿರಾಂಕ V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_ಟ್ಯಾಗುಗಳು;

    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ callStrSetter = [ಇದು](ಶೂನ್ಯ (V3Options::*cbStr)(ಸ್ಥಿರಾಂಕ string&)) {
        ಹಿಂತಿರುಗಿ [ಇದು, cbStr](ಸ್ಥಿರಾಂಕ string& v) { (ಇದು->*cbStr)(v); };
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
    DECL_OPTION("+define+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) VL_MT_DISABLED {
        ಕೂಡಿಸು_Define(optp, ನಿಜ);
    }).notForRerun();
    DECL_OPTION("+incdir+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು, &optdir](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        string dirs = optp;
        string::ಗಾತ್ರ_type pos;
        ಆಗ ((pos = dirs.ಕಂಡುಹಿಡಿ('+')) != string::npos) {
            ಕೂಡಿಸು_Inc_Dir_User(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        ಕೂಡಿಸು_Inc_Dir_User(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        string exts = optp;
        string::ಗಾತ್ರ_type pos;
        ಆಗ ((pos = exts.ಕಂಡುಹಿಡಿ('+')) != string::npos) {
            ಕೂಡಿಸು_Lib_Ext_V(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        ಕೂಡಿಸು_Lib_Ext_V(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Lang_Ext(optp, V3_Lang_ಸಂಕೇತ::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](ತಾರ್ಕಿಕ) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_bboxUnsup = flag;
        File_ಸಾಲು::globalWarnOff(V3_Error_ಸಂಕೇತ::E_UNSUPPORTED, ನಿಜ);
    });
    DECL_OPTION("-binary", CbCall, [ಇದು]() {
        m_ದ್ವಿಮಾನ = ನಿಜ;
        m_build = ನಿಜ;
        m_exe = ನಿಜ;
        m_main = ನಿಜ;
        ಆದರೆ (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", ಗಣ_2, &m_build);
    DECL_OPTION("-build-dep-bin", ಗಣ_2, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಪೂರ್ಣಾಂಕ val = std::atoi(valp);
        ಆದರೆ (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::ಕೂಡಿಸು_CFlags));
    DECL_OPTION("-cc", CbCall, [ಇದು]() { cc_ಗಣ(); });
    DECL_OPTION("-clk", CbVal, [fl](ಸ್ಥಿರಾಂಕ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ಸ್ಥಿರಾಂಕ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", ಗಣ_2, &m_comp_ಎಲ್ಲೆ_ಖಂಡಗಳು).undocumented();
    DECL_OPTION("-comp-limit-members", ಗಣ_2,
                &m_comp_ಎಲ್ಲೆ_Members)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", ಗಣ_2, &m_comp_ಎಲ್ಲೆ_Parens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](ಪೂರ್ಣಾಂಕ val) { VName::max_ಉದ್ದಳತೆ(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಆದರೆ (!std::strcmp(valp, "clang")) {
            m_comp_ಎಲ್ಲೆ_ಖಂಡಗಳು = 80;  // limit unknown
            m_comp_ಎಲ್ಲೆ_Members = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_comp_ಎಲ್ಲೆ_Parens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "gcc")) {
            m_comp_ಎಲ್ಲೆ_ಖಂಡಗಳು = 0;  // Bug free
            m_comp_ಎಲ್ಲೆ_Members = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_comp_ಎಲ್ಲೆ_Parens = 240;  // Unlimited, but generate same code as for clang
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "msvc")) {
            m_comp_ಎಲ್ಲೆ_ಖಂಡಗಳು = 80;  // 128, but allow some room
            m_comp_ಎಲ್ಲೆ_Members = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_comp_ಎಲ್ಲೆ_Parens = 80;  // 128, but allow some room
        } ಇಲ್ಲದಿದ್ದರೆ {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warn_ಇನ್ನಷ್ಟು() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::ಕೂಡಿಸು_Compiler_Includes));
    DECL_OPTION("-converge-limit", ಗಣ_2, &m_converge_ಎಲ್ಲೆ);
    DECL_OPTION("-coverage", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_ಅಭಿವ್ಯಕ್ತಿ);
    DECL_OPTION("-coverage-expr-max", ಗಣ_2, &m_coverage_ಅಭಿವ್ಯಕ್ತಿ_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ valp) {
        ಆದರೆ (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::ತುಂಬಿದ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warn_ಇನ್ನಷ್ಟು() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", ಗಣ_2, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_ಸಾಲು);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ parse_Bins_ಎಲ್ಲೆ = [fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp, uint32_t& limitr) {
        ಅಕ್ಷರ* endp = ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ದೀರ್ಘ ದೀರ್ಘ ಮೌಲ್ಯ = std::strtoull(valp, &endp, 10);
        ಆದರೆ (*endp || ಮೌಲ್ಯ < 1 || ಮೌಲ್ಯ > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            ಹಿಂತಿರುಗಿ;
        }
        limitr = static_cast<uint32_t>(ಮೌಲ್ಯ);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [ಇದು, parse_Bins_ಎಲ್ಲೆ](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        parse_Bins_ಎಲ್ಲೆ("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [ಇದು, parse_Bins_ಎಲ್ಲೆ](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        parse_Bins_ಎಲ್ಲೆ("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", ಗಣ_2, &m_coverage_Max_ಅಗಲ);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverage_ವಿಲೀನ_Instances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coverage_ಪ್ರತಿ_Instance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) VL_MT_DISABLED {
        ಕೂಡಿಸು_Define(valp, ಸುಳ್ಳು);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [ಇದು]() { ಗಣ_Debug_ಬಹುಲಕ(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [ಇದು](ಪೂರ್ಣಾಂಕ v) { ಗಣ_Debug_ಬಹುಲಕ(v); }).notForRerun();
    DECL_OPTION("-debugi-", Cb_Partial_ಹೊಂದಿಕೆ_Val, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_debugLevel[optp] = std::atoi(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", ಗಣ_2, &m_debugAllocRandom).undocumented();
#ಆದರೆ_ಅಂತ್ಯ
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
    DECL_OPTION("-debug-partition", OnOff, &m_debug_ವಿಭಜನೆ).undocumented();
    DECL_OPTION("-debug-preproc-passthru", OnOff, &m_debugPreprocPassthru).undocumented();
    DECL_OPTION("-debug-protect", OnOff, &m_debugProtect).undocumented();
    DECL_OPTION("-debug-runtime-timeout", ಗಣ_2, &m_debugRuntimeTimeout).undocumented();
    DECL_OPTION("-debug-self-test", OnOff, &m_debugSelfTest).undocumented();
    DECL_OPTION("-debug-sigsegv", CbCall, throwSigsegv).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_ರಾಶಿ_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debug_ಅಗಲ).undocumented();
    DECL_OPTION("-decoration", CbCall, [ಇದು, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [ಇದು, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = ನಿಜ;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", Cb_Partial_ಹೊಂದಿಕೆ_Val, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        ಆದರೆ (flag) {
            m_stdPackage = ಸುಳ್ಳು;
            m_stdWaiver = ಸುಳ್ಳು;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<ಶೂನ್ಯ (*)(ಪೂರ್ಣಾಂಕ)>(&V3Error::error_ಎಲ್ಲೆ));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) { m_expand_ಎಲ್ಲೆ = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [ಇದು, fl, &optdir](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ನಿಜ);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [ಇದು, &optdir](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) { ಕೂಡಿಸು_Force_Inc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [ಇದು, fl, &optdir](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ಸುಳ್ಳು);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) { ಕೂಡಿಸು_Future0(valp); });
    DECL_OPTION("-future1", CbVal, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) { ಕೂಡಿಸು_Future1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_f_ಬಿಟ್_ಪರಿಶೀಲನೆ_Loops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_fCaseDecoder = flag;
        m_f_Case_ಕೋಷ್ಟಕ = flag;
        m_f_Case_ವೃಕ್ಷ = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_f_Case_ಕೋಷ್ಟಕ);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_ವೃಕ್ಷ);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_ಬಿಟ್_Op_ವೃಕ್ಷ);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ತಳ್ಳು_ಕೆಳಕ್ಕೆ_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [ಇದು, fl](ತಾರ್ಕಿಕ flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {  //
        m_f_Func_ವಿಭಜಿಸು_Cat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_f_Func_ವಿಭಜಿಸು_Cat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {  //
        m_fIcoChangeDetect.ಗಣ_True_Or_False(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_f_ವಿಲೀನ_Cond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_f_ವಿಲೀನ_Cond_Motion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_f_ತುಂಡು);
    DECL_OPTION("-fslice-element-limit", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_f_ತುಂಡು_Element_ಎಲ್ಲೆ = std::atoi(valp);
        ಆದರೆ (m_f_ತುಂಡು_Element_ಎಲ್ಲೆ < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_f_ವಿಭಜಿಸು);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_f_ಕೋಷ್ಟಕ);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](ತಾರ್ಕಿಕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) { ಕೂಡಿಸು_Parameter(optp, ಸುಳ್ಳು); });
    DECL_OPTION("-gate-stmts", ಗಣ_2, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [ಇದು]() {
        ಹೊರಗೆ << protect_ಕೀಲಿ_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಹೊರಗೆ << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಹೊರಗೆ << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_ಶ್ರೇಣೀಕೃತ);
    DECL_OPTION("-hierarchical-block", CbVal, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಸ್ಥಿರಾಂಕ V3_ಶ್ರೇಣೀಕೃತ_ಖಂಡ_Option opt{valp};
        m_hier_ಖಂಡಗಳು.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", ಗಣ_2, &m_hier_ಮಗು);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) { m_hierParamsFile.ತಳ್ಳು_ಹಿಂಭಾಗ({optp, work()}); });

    DECL_OPTION("-I", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು, &optdir](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಕೂಡಿಸು_Inc_Dir_User(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", ಗಣ_2, &m_if_ಆಳ);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", ಗಣ_2, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", ಗಣ_2, &m_inline_CFuncs_ಗುಣಲಬ್ಧ);
    DECL_OPTION("-inline-mult", ಗಣ_2, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [ಇದು, fl](ಪೂರ್ಣಾಂಕ val) {
        m_instr_ಎಣಿಕೆ_Dpi = val;
        ಆದರೆ (m_instr_ಎಣಿಕೆ_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = ನಿಜ;
    });
    DECL_OPTION("-json-only-output", CbVal, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = ನಿಜ;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::ಕೂಡಿಸು_Ld_Libs));
    DECL_OPTION("-l2-name", ಗಣ_2, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [ಇದು]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [ಇದು]() { m_l2Name = "v"; }).undocumented();  // Historical
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಗಣ_Lang = [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಸ್ಥಿರಾಂಕ V3_Lang_ಸಂಕೇತ optval{valp};
        ಆದರೆ (optval.legal()) {
            m_defaultLanguage = optval;
        } ಇಲ್ಲದಿದ್ದರೆ {
            VSpellCheck spell;
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = V3_Lang_ಸಂಕೇತ::L_ERROR + 1; i < V3_Lang_ಸಂಕೇತ::ENUM_ಕೊನೆ; ++i) {
                spell.ತಳ್ಳು_Candidate(V3_Lang_ಸಂಕೇತ{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.ಉತ್ತಮ_Candidate_ಸಂದೇಶ(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, ಗಣ_Lang).notForRerun();
    DECL_OPTION("-language", CbVal, ಗಣ_Lang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) { ಕೂಡಿಸು_Lib_Map_File(valp); });
    DECL_OPTION("-lib-create", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", ಗಣ_2, &m_localize_Max_ಗಾತ್ರ);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::ಕೂಡಿಸು_Make_Flags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_makeDir = valp;
        ಕೂಡಿಸು_Inc_Dir_Fallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", ಗಣ_2, &m_main_ತುದಿ_Name);
    DECL_OPTION("-make", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಆದರೆ (!std::strcmp(valp, "gmake")) {
            m_gmake = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "json")) {
            m_makeJson = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", ಗಣ_2, &m_funcRecursion);
    DECL_OPTION("-max-num-width", ಗಣ_2, &m_max_ಅಂಕೆ_ಅಗಲ);
    DECL_OPTION("-mod-prefix", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_mod_ಪೂರ್ವಪ್ರತ್ಯಯ = valp;
    });

    DECL_OPTION("-O0", CbCall, [ಇದು]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [ಇದು]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [ಇದು]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [ಇದು]() { optimize(3); });

    DECL_OPTION("-o", ಗಣ_2, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](ತಾರ್ಕಿಕ /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_outputGroups = std::atoi(valp);
        ಆದರೆ (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_output_Groups_ಗಾತ್ರ = std::atoll(valp);
        ಆದರೆ (m_output_Groups_ಗಾತ್ರ < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", ಗಣ_2, &m_output_ವಿಭಜಿಸು);
    DECL_OPTION("-output-split-cfuncs", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_output_ವಿಭಜಿಸು_CFuncs = std::atoi(valp);
        ಆದರೆ (m_output_ವಿಭಜಿಸು_CFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_output_ವಿಭಜಿಸು_CTrace = std::atoi(valp);
        ಆದರೆ (m_output_ವಿಭಜಿಸು_CTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", ಗಣ_2, &m_preproc_No_ಸಾಲು);
    DECL_OPTION("-pins64", CbCall, [ಇದು]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [ಇದು]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_pinsBv = std::atoi(valp);
        ಆದರೆ (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_pinsScUint = flag;
        ಆದರೆ (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", ಗಣ_2, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_ಪೂರ್ವಪ್ರತ್ಯಯ = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preproc_ನಿರೂಪಿಸುತ್ತದೆ);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_preproc_Token_ಎಲ್ಲೆ = std::atoi(valp);
        ಆದರೆ (m_preproc_Token_ಎಲ್ಲೆ <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [ಇದು]() { m_ಬಹಿರಂಗ = ಸುಳ್ಳು; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [ಇದು]() { m_profC = m_profCFuncs = ನಿಜ; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [ಇದು]() {
        m_profC = m_profCFuncs = ನಿಜ;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", ಗಣ_2, &m_protect_ಕೀಲಿ);
    DECL_OPTION("-protect-lib", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = ನಿಜ;
    });
    DECL_OPTION("-public", OnOff, &m_ಬಹಿರಂಗ);
    DECL_OPTION("-public-depth", ಗಣ_2, &m_ಬಹಿರಂಗ_ಆಳ);
    DECL_OPTION("-public-flat-rw", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_ಬಹಿರಂಗ_Flat_RW = flag;
        v3Global.dpi(ನಿಜ);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) { m_ಬಹಿರಂಗ_Ignore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_ಬಹಿರಂಗ_Params = flag;
        v3Global.dpi(ನಿಜ);
    });
    DECL_OPTION("-pvalue+", Cb_Partial_ಹೊಂದಿಕೆ,
                [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* varp) { ಕೂಡಿಸು_Parameter(varp, ಸುಳ್ಳು); });

    DECL_OPTION("-quiet", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_reloop_ಎಲ್ಲೆ = std::atoi(valp);
        ಆದರೆ (m_reloop_ಎಲ್ಲೆ < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_replication_ಎಲ್ಲೆ = std::atoi(valp);
        ಆದರೆ (m_replication_ಎಲ್ಲೆ < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [ಇದು, fl]() {
        decorations(fl, "node");
        ಕೂಡಿಸು_CFlags("-ggdb");
        ಕೂಡಿಸು_Ld_Libs("-ggdb");
        ಕೂಡಿಸು_CFlags("-fsanitize=address,undefined");
        ಕೂಡಿಸು_Ld_Libs("-fsanitize=address,undefined");
        ಕೂಡಿಸು_CFlags("-D_GLIBCXX_DEBUG");
        ಕೂಡಿಸು_CFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [ಇದು]() {
        m_ಹೊರಹೋಗು_Format_Ok = ನಿಜ;
        m_systemC = ನಿಜ;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [ಇದು](ತಾರ್ಕಿಕ flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [ಇದು]() { m_defaultLanguage = V3_Lang_ಸಂಕೇತ::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [ಇದು, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_ಎಳೆಗಳು = 1;
    });
    DECL_OPTION("-threads", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_ಎಳೆಗಳು = std::atoi(valp);
        ಆದರೆ (m_ಎಳೆಗಳು < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        ಆದರೆ (m_ಎಳೆಗಳು == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_ಎಳೆಗಳು = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_hier_ಎಳೆಗಳು = std::atoi(valp);
        ಆದರೆ (m_hier_ಎಳೆಗಳು < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_ಎಳೆಗಳು_Coarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಆದರೆ (!std::strcmp(valp, "all")) {
            m_ಎಳೆಗಳು_Dpi_ಶುದ್ಧ = ನಿಜ;
            m_ಎಳೆಗಳು_Dpi_Unpure = ನಿಜ;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "none")) {
            m_ಎಳೆಗಳು_Dpi_ಶುದ್ಧ = ಸುಳ್ಳು;
            m_ಎಳೆಗಳು_Dpi_Unpure = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "pure")) {
            m_ಎಳೆಗಳು_Dpi_ಶುದ್ಧ = ನಿಜ;
            m_ಎಳೆಗಳು_Dpi_Unpure = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warn_ಇನ್ನಷ್ಟು() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        m_ಎಳೆಗಳು_Max_MTasks = std::atoi(valp);
        ಆದರೆ (m_ಎಳೆಗಳು_Max_MTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        ಆದರೆ (!unit.isNone() && ಸಮಯ_Override_Unit().isNone()) m_ಸಮಯ_Default_Unit = unit;
        ಆದರೆ (!prec.isNone() && ಸಮಯ_Override_Prec().isNone()) m_ಸಮಯ_Default_Prec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, ನಿಜ);
        ಆದರೆ (!unit.isNone()) {
            m_ಸಮಯ_Default_Unit = unit;
            m_ಸಮಯ_Override_Unit = unit;
        }
        ಆದರೆ (!prec.isNone()) {
            m_ಸಮಯ_Default_Prec = prec;
            m_ಸಮಯ_Override_Prec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [ಇದು](ಸ್ಥಿರಾಂಕ std::string& flag) { m_ತುದಿ_Module = Ast_ಗಂಟು::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [ಇದು](ಸ್ಥಿರಾಂಕ std::string& flag) { m_ತುದಿ_Module = Ast_ಗಂಟು::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", ಗಣ_2, &m_ತುದಿ_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [ಇದು]() { m_traceEnabledSaif = ನಿಜ; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", ಗಣ_2, &m_trace_ಆಳ);
    DECL_OPTION("-trace-fst", CbCall, [ಇದು]() {
        m_traceEnabledFst = ನಿಜ;
        ಕೂಡಿಸು_Ld_Libs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", ಗಣ_2, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", ಗಣ_2, &m_trace_Max_ಅಗಲ);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", ಗಣ_2, &m_no_Trace_ತುದಿ);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [ಇದು]() { m_traceEnabledVcd = ನಿಜ; });

    DECL_OPTION("-U", Cb_Partial_ಹೊಂದಿಕೆ, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", ಗಣ_2, &m_constraint_Array_ಎಲ್ಲೆ);
    DECL_OPTION("-unroll-count", ಗಣ_2, &m_unroll_ಎಣಿಕೆ).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", ಗಣ_2, &m_unroll_ಎಲ್ಲೆ);
    DECL_OPTION("-unroll-stmts", ಗಣ_2, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", ಗಣ_2, &m_ಬಳಸದ_Regexp);

    DECL_OPTION("-V", CbCall, [ಇದು]() {
        show_ಆವೃತ್ತಿ(ನಿಜ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [ಇದು, &optdir](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        V3Options::ಕೂಡಿಸು_Library_File(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಪೂರ್ಣಾಂಕ val = std::atoi(valp);
        ಆದರೆ (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [ಇದು]() {
        show_ಆವೃತ್ತಿ(ಸುಳ್ಳು);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_ಸಾಲು::globalWarnOff(V3_Error_ಸಂಕೇತ::I_LINT, ಸುಳ್ಳು); });
    DECL_OPTION("-Werror-", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2{optp};
        ಆದರೆ (ಸಂಕೇತ_2 == V3_Error_ಸಂಕೇತ::EC_ERROR) {
            ಆದರೆ (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            V3Error::pretendError(ಸಂಕೇತ_2, ನಿಜ);
        }
    });
    DECL_OPTION("-Wfuture-", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        ಕೂಡಿಸು_Future(optp);
    });
    DECL_OPTION("-Wno-", Cb_Partial_ಹೊಂದಿಕೆ, [fl, &parser](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) VL_MT_DISABLED {
        ಸ್ಥಿರಾಂಕ string err = File_ಸಾಲು::globalWarnOffParse(optp, ನಿಜ);
        ಆದರೆ (!err.ಖಾಲಿಗಳು()) {
            ಸ್ಥಿರಾಂಕ string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = V3_Error_ಸಂಕೇತ::EC_ಮೊದಲ_WARN; i < V3_Error_ಸಂಕೇತ::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string ಪೂರ್ವಪ್ರತ್ಯಯ : {"-Wno-", "-Wwarn-"})
            parser.ಕೂಡಿಸು_Suggestion_Candidate(ಪೂರ್ವಪ್ರತ್ಯಯ + V3_Error_ಸಂಕೇತ{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [ಇದು]() { m_context = ಸುಳ್ಳು; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(ಸುಳ್ಳು); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_ಸಾಲು::globalWarnOff(V3_Error_ಸಂಕೇತ::I_LINT, ನಿಜ); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_ಸಾಲು::globalWarnOff(V3_Error_ಸಂಕೇತ::I_STYLE, ನಿಜ); });
    DECL_OPTION("-work", ಗಣ_2, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [ಇದು]() {
        m_pedantic = ನಿಜ;
        V3Error::pretendError(V3_Error_ಸಂಕೇತ::ASSIGNIN, ಸುಳ್ಳು);
    });
    DECL_OPTION("-Wwarn-", Cb_Partial_ಹೊಂದಿಕೆ, [ಇದು, fl, &parser](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) VL_MT_DISABLED {
        ಸ್ಥಿರಾಂಕ V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2{optp};
        ಆದರೆ (ಸಂಕೇತ_2 == V3_Error_ಸಂಕೇತ::EC_ERROR) {
            ಆದರೆ (!isFuture(optp)) {
                ಸ್ಥಿರಾಂಕ string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            File_ಸಾಲು::globalWarnOff(ಸಂಕೇತ_2, ಸುಳ್ಳು);
            V3Error::pretendError(ಸಂಕೇತ_2, ಸುಳ್ಳು);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_ಸಾಲು::globalWarnOff(V3_Error_ಸಂಕೇತ::I_LINT, ಸುಳ್ಳು); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_ಸಾಲು::globalWarnOff(V3_Error_ಸಂಕೇತ::I_STYLE, ಸುಳ್ಳು); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", ಗಣ_2, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಆದರೆ (!std::strcmp(valp, "0")) {
            m_x_ನಿಯೋಜಿಸು = "0";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "1")) {
            m_x_ನಿಯೋಜಿಸು = "1";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "fast")) {
            m_x_ನಿಯೋಜಿಸು = "fast";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "unique")) {
            m_x_ನಿಯೋಜಿಸು = "unique";
        } ಇಲ್ಲದಿದ್ದರೆ {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warn_ಇನ್ನಷ್ಟು() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [ಇದು, fl](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಆದರೆ (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } ಇಲ್ಲದಿದ್ದರೆ {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warn_ಇನ್ನಷ್ಟು() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_x_Initial_ಅಂಚು);

    DECL_OPTION("-y", CbVal, [ಇದು, &optdir](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) {
        ಕೂಡಿಸು_Inc_Dir_User(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        ಆದರೆ (argv[i][0] == '-' || argv[i][0] == '+') {
            ಸ್ಥಿರಾಂಕ std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            ಪೂರ್ಣಾಂಕ consumed = 0;
            ತಾರ್ಕಿಕ isForRerun = ನಿಜ;

            // Special case for -j which has an optional argument
            ಆದರೆ (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                ಪೂರ್ಣಾಂಕ val = 0;
                ಆದರೆ (i + 1 < argc && std::ಅಂಕಿಯೇ(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    ಆದರೆ (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                ಆದರೆ (m_buildJobs == -1) m_buildJobs = val;
                ಆದರೆ (m_verilateJobs == -1) m_verilateJobs = val;
                ಆದರೆ (m_outputGroups == -1) m_outputGroups = val;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = parser.parse(i, argc, argv);
                consumed = pair.ಮೊದಲ;
                isForRerun = pair.ಎರಡನೇ;
                ಆದರೆ (consumed) {
                    // Already processed
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isFuture0(argName)) {
                    consumed = 1;
                } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (isFuture1(argName)) {
                    consumed = 2;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            ಆದರೆ (argName != "f" && argName != "F") ಕೂಡಿಸು_Arg(argv + i, consumed, isForRerun);
            i += consumed;
            ಮುಂದುವರಿ;
        }

        // Filename
        ಸ್ಥಿರಾಂಕ std::string filename = parseFileArg(optdir, argv[i]);
        ತಾರ್ಕಿಕ isForRerun = ನಿಜ;
        ಆದರೆ (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::ಕೂಡಿಸು_Cpp_File(filename);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::ಕೂಡಿಸು_Ld_Lib_File(filename);
        } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (suffixed(filename, ".vlt")) {
            V3Options::ಕೂಡಿಸು_Vlt_File(filename, work());
            isForRerun = ಸುಳ್ಳು;
        } ಇಲ್ಲದಿದ್ದರೆ {
            V3Options::ಕೂಡಿಸು_VFile(filename, work());
            isForRerun = ಸುಳ್ಳು;
        }
        ಕೂಡಿಸು_Arg(filename, isForRerun);
        ++i;
    }

    ಆದರೆ (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

ಶೂನ್ಯ V3Options::parseOptsFile(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& filename, ತಾರ್ಕಿಕ rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ಸ್ಥಿರಾಂಕ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    ಆದರೆ (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        ಹಿಂತಿರುಗಿ;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    ತಾರ್ಕಿಕ has_fargs = ಸುಳ್ಳು;
    std::string ಸಂಪೂರ್ಣ_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ parse_fargs_cmt = [&](ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* p) -> ಶೂನ್ಯ {
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಖಂಡ_Cmt = p[1] == '*';
        p += 2;
        ಆಗ (std::ಖಾಲಿಯೇ(*p)) ++p;
        ಆದರೆ (VString::startsWith(p, "verilator")) {
            p += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("verilator");
            ಆದರೆ (std::ಖಾಲಿಯೇ(*p)) {
                ಆಗ (std::ಖಾಲಿಯೇ(*p)) ++p;
                ಆದರೆ (VString::startsWith(p, "fargs")) {
                    p += std::ಅಕ್ಷರಸರಣಿ_ಉದ್ದ("fargs");
                    ಆದರೆ (std::ಖಾಲಿಯೇ(*p)) {
                        ಆಗ (std::ಖಾಲಿಯೇ(*p)) ++p;
                        ಆದರೆ (!has_fargs) {
                            has_fargs = ನಿಜ;
                            ಸಂಪೂರ್ಣ_file.clear();
                        }
                        ಸಂಪೂರ್ಣ_file += p;
                        ಆದರೆ (ಖಂಡ_Cmt) {
                            ಸ್ಥಿರಾಂಕ std::string::ಗಾತ್ರ_type pos = ಸಂಪೂರ್ಣ_file.ಕಂಡುಹಿಡಿ("*/");
                            ಆದರೆ (pos == std::string::npos) {
                                fl->v3fatal("/* verilator fargs */ must be on single line");
                            }
                            ಸಂಪೂರ್ಣ_file.resize(pos);
                        }
                        ಸಂಪೂರ್ಣ_file += ' ';
                    }
                }
            }
        }
    };

    ತಾರ್ಕಿಕ ಒಳ_Cmt = ಸುಳ್ಳು;
    ಆಗ (!ifp->eof()) {
        ಸ್ಥಿರಾಂಕ std::string ಸಾಲು = V3Os::getline(*ifp);
        // Strip simple comments
        ಅಕ್ಷರ lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        ತಾರ್ಕಿಕ space_begin = ನಿಜ;  // At beginning or leading spaces only
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* p = ಸಾಲು.c_str(); *p; lastch = *p++) {
            ಆದರೆ (ಒಳ_Cmt) {
                ಆದರೆ (p[0] == '*' && p[1] == '/') {
                    ಒಳ_Cmt = ಸುಳ್ಳು;
                    ++p;
                }
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (p[0] == '/' && p[1] == '/' && std::ಖಾಲಿಯೇ(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                ನಿಲ್ಲಿಸು;  // Ignore to EOL
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                ನಿಲ್ಲಿಸು;  // Ignore to EOL
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                ಒಳ_Cmt = ನಿಜ;
                space_begin = ಸುಳ್ಳು;
                ++p;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ಆದರೆ (!std::ಖಾಲಿಯೇ(*p)) space_begin = ಸುಳ್ಳು;
                ಆದರೆ (!has_fargs) ಸಂಪೂರ್ಣ_file += *p;
            }
        }
        ಆದರೆ (!has_fargs) ಸಂಪೂರ್ಣ_file += " ";
    }
    ಸಂಪೂರ್ಣ_file += "\n";  // So string match below is simplified
    ಆದರೆ (ಒಳ_Cmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = ಹೊಸ File_ಸಾಲು{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::ಸದಿಶ<string> ವಾದಗಳು;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    ಎಣಿಕೆ state : uint8_t {
        ST_ಒಳ_OPTION,
        ST_ESCAPED_CHAR,
        ST_ಒಳ_QUOTED_STR,
        ST_ಒಳ_DOUBLE_QUOTED_STR
    };

    state st = ST_ಒಳ_OPTION;
    state ಕೊನೆಯ_st = ST_ಒಳ_OPTION;
    string arg;
    ಗಾಗಿ (string::ಗಾತ್ರ_type pos = 0; pos < ಸಂಪೂರ್ಣ_file.ಉದ್ದಳತೆ(); ++pos) {
        ಅಕ್ಷರ curr_char = ಸಂಪೂರ್ಣ_file[pos];
        ಆಯ್ಕೆ (st) {
        ಸ್ಥಿತಿ ST_ಒಳ_OPTION:  // Get all chars up to a white space or a "="
            ಆದರೆ (std::ಖಾಲಿಯೇ(curr_char)) {  // End of option
                ಆದರೆ (!arg.ಖಾಲಿಗಳು()) {  // End of word
                    ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(arg);
                }
                arg = "";
                ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (curr_char == '\\') {  // Escape char, we wait for next char
                ಕೊನೆಯ_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                ಆದರೆ (pos < ಸಂಪೂರ್ಣ_file.ಉದ್ದಳತೆ()) curr_char = ಸಂಪೂರ್ಣ_file[pos];
                ಆದರೆ (curr_char == '"') {  // String
                    st = ST_ಒಳ_QUOTED_STR;
                } ಇಲ್ಲದಿದ್ದರೆ {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                ನಿಲ್ಲಿಸು;
            }
            ಆದರೆ (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_ಒಳ_DOUBLE_QUOTED_STR;
                ನಿಲ್ಲಿಸು;
            }
            arg += curr_char;
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ ST_ಒಳ_QUOTED_STR:  // Just store all chars inside string
            ಆದರೆ (curr_char != '\'') {
                arg += curr_char;
            } ಇಲ್ಲದಿದ್ದರೆ {  // End of quoted string
                st = ST_ಒಳ_OPTION;
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ ST_ಒಳ_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            ಆದರೆ (curr_char != '"') {
                ಆದರೆ (curr_char == '\\') {
                    ಕೊನೆಯ_st = st;
                    st = ST_ESCAPED_CHAR;
                } ಇಲ್ಲದಿದ್ದರೆ {
                    arg += curr_char;
                }
            } ಇಲ್ಲದಿದ್ದರೆ {  // End of double quoted string
                st = ST_ಒಳ_OPTION;
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = ಕೊನೆಯ_st;
            ನಿಲ್ಲಿಸು;
        }
    }
    ಆದರೆ (!arg.ಖಾಲಿಗಳು()) {  // Add last word
        ವಾದಗಳು.ತಳ್ಳು_ಹಿಂಭಾಗ(arg);
    }

    //ಮಾರ್ಗ
    ಸ್ಥಿರಾಂಕ string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::ಸದಿಶ<ಅಕ್ಷರ*> argv;
    argv.reserve(ವಾದಗಳು.ಗಾತ್ರ_2() + 1);
    ಗಾಗಿ (ಸ್ಥಿರಾಂಕ string& i : ವಾದಗಳು) argv.ತಳ್ಳು_ಹಿಂಭಾಗ(const_cast<ಅಕ್ಷರ*>(i.c_str()));
    argv.ತಳ್ಳು_ಹಿಂಭಾಗ(ಶೂನ್ಯಸೂಚಕ);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<ಪೂರ್ಣಾಂಕ>(argv.ಗಾತ್ರ_2() - 1), argv.ದತ್ತಾಂಶ());
}

//======================================================================

string V3Options::parseFileArg(ಸ್ಥಿರಾಂಕ string& optdir, ಸ್ಥಿರಾಂಕ string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    ಆದರೆ (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    ಹಿಂತಿರುಗಿ filename;
}

//======================================================================

ಶೂನ್ಯ V3Options::show_ಆವೃತ್ತಿ(ತಾರ್ಕಿಕ verbose) {
    ಹೊರಗೆ << ಆವೃತ್ತಿ();
    ಹೊರಗೆ << "\n";
    ಆದರೆ (!verbose) ಹಿಂತಿರುಗಿ;

    ಹೊರಗೆ << "\n";
    ಹೊರಗೆ << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    ಹೊರಗೆ << "redistribute it and/or modify the Verilator internals under the terms of\n";
    ಹೊರಗೆ << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    ಹೊರಗೆ << "License Version 2.0.\n";

    ಹೊರಗೆ << "\n";
    ಹೊರಗೆ << "See https://verilator.org for documentation\n";

    ಹೊರಗೆ << "\n";
    ಹೊರಗೆ << "Summary of configuration:\n";
    ಹೊರಗೆ << "  Compiled in defaults if not in environment:\n";
    ಹೊರಗೆ << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    ಹೊರಗೆ << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    ಹೊರಗೆ << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    ಹೊರಗೆ << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    ಹೊರಗೆ << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_ಬೇರು << "\n";
    ಹೊರಗೆ << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    ಹೊರಗೆ << "    SystemC system-wide = " << cvt_ಗಮ್ಯ_Str(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    ಹೊರಗೆ << "\n";
    ಹೊರಗೆ << "Environment:\n";
    ಹೊರಗೆ << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    ಹೊರಗೆ << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    ಹೊರಗೆ << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    ಹೊರಗೆ << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    ಹೊರಗೆ << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    ಹೊರಗೆ << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    ಹೊರಗೆ << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    ಹೊರಗೆ << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    ಹೊರಗೆ << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    ಹೊರಗೆ << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    ಹೊರಗೆ << "\n";
    ಹೊರಗೆ << "Supported features (compiled-in or forced by environment):\n";
    ಹೊರಗೆ << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    ಹೊರಗೆ << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    ಹೊರಗೆ << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = ಹೊಸ V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_ಬಳಸದ_Regexp = "*unused*";
    m_x_ನಿಯೋಜಿಸು = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3_Lang_ಸಂಕೇತ::mostRecent();

    VName::max_ಉದ್ದಳತೆ(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    ಕೂಡಿಸು_Lib_Ext_V("");  // So include "filename.v" will find the same file
    ಕೂಡಿಸು_Lib_Ext_V(".v");
    ಕೂಡಿಸು_Lib_Ext_V(".sv");
    // Default -I
    ಕೂಡಿಸು_Inc_Dir_Fallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(ಅಳಿಸು m_impp, m_impp = ಶೂನ್ಯಸೂಚಕ); }

ಶೂನ್ಯ V3Options::ಗಣ_Debug_ಬಹುಲಕ(ಪೂರ್ಣಾಂಕ level) {
    V3Error::debugDefault(level);
    ಆದರೆ (!m_dumpLevel.ಎಣಿಕೆ_2("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = ನಿಜ;
    m_debugCheck = ನಿಜ;
    ಆದರೆ (level) {
        ಹೊರಗೆ << "- Starting " << ಆವೃತ್ತಿ() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

ಚಿಹ್ನೆರಹಿತ V3Options::debugLevel(ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_debugLevel.ಕಂಡುಹಿಡಿ(ಟ್ಯಾಗ್);
    ಹಿಂತಿರುಗಿ iter != m_debugLevel.ಕೊನೆ() ? iter->ಎರಡನೇ : V3Error::debugDefault();
}

ಚಿಹ್ನೆರಹಿತ V3Options::debugSrcLevel(ಸ್ಥಿರಾಂಕ string& srcfile_ಮಾರ್ಗ) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ಹಿಂತಿರುಗಿ debugLevel(V3Os::filenameNonDirExt(srcfile_ಮಾರ್ಗ));
}

ಚಿಹ್ನೆರಹಿತ V3Options::dumpLevel(ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ iter = m_dumpLevel.ಕಂಡುಹಿಡಿ(ಟ್ಯಾಗ್);
    ಹಿಂತಿರುಗಿ iter != m_dumpLevel.ಕೊನೆ() ? iter->ಎರಡನೇ : 0;
}

ಚಿಹ್ನೆರಹಿತ V3Options::dumpSrcLevel(ಸ್ಥಿರಾಂಕ string& srcfile_ಮಾರ್ಗ) ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ಹಿಂತಿರುಗಿ dumpLevel(V3Os::filenameNonDirExt(srcfile_ಮಾರ್ಗ));
}

ತಾರ್ಕಿಕ V3Options::dump_ವೃಕ್ಷ_Addrids() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {
    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ s_level = -1;
    ಆದರೆ (VL_UNLIKELY(s_level < 0)) {
        ಸ್ಥಿರಾಂಕ ಚಿಹ್ನೆರಹಿತ ಮೌಲ್ಯ = dumpLevel("tree-addrids");
        ಆದರೆ (!ಲಭ್ಯ()) ಹಿಂತಿರುಗಿ ಮೌಲ್ಯ > 0;
        s_level = static_cast<ಚಿಹ್ನೆರಹಿತ>(ಮೌಲ್ಯ);
    }
    ಹಿಂತಿರುಗಿ s_level > 0;
}

ಶೂನ್ಯ V3Options::optimize(ಪೂರ್ಣಾಂಕ level) {
    // Set all optimizations to on/off
    ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_f_ಬಿಟ್_ಪರಿಶೀಲನೆ_Loops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_f_Case_ಕೋಷ್ಟಕ = flag;
    m_f_Case_ವೃಕ್ಷ = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_ಬಿಟ್_Op_ವೃಕ್ಷ = flag;
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
    m_f_ವಿಲೀನ_Cond = flag;
    m_fReloop = flag;
    m_fReorder = flag;
    m_f_ವಿಭಜಿಸು = flag;
    m_fSubst = flag;
    m_fSubstConst = flag;
    m_f_ಕೋಷ್ಟಕ = flag;
    // And set specific optimization levels
    ಆದರೆ (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
