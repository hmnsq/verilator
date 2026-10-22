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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Options.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Container.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Mutex.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OptionParser.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Os.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3PreShell.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

// clang-format off
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/types.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <sys/stat.h>
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ _WIN32
# include <sys/utsname.h>
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <cctype>
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ _MSC_VER
# include <filesystem> // C++17
# define S_ISDIR(ਬਹੁਲਕ) (((ਬਹੁਲਕ) & _S_IFMT) == _S_IFDIR)
#ਨਹੀਂਤਾਂ
# include <dirent.h>
#ਜੇ_ਅੰਤ
#ਅਨ੍ਤਰ੍ਭਾਵ <fcntl.h>
#ਅਨ੍ਤਰ੍ਭਾਵ <list>
#ਅਨ੍ਤਰ੍ਭਾਵ <map>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <thread>
#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>

#ਅਨ੍ਤਰ੍ਭਾਵ "config_rev.h"

#ਜੇ defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, close
#ਜੇ_ਅੰਤ
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// V3 Internal state

ਵਰਗ V3OptionsImp final {
ਜਨਤਕ:
    // TYPES
    ਵਰਤੋਂ DirMap = std::map<ਅਚਲ string, std::ਸੈੱਟ<std::string>>;  // Directory listing

    // STATE
    std::list<string> m_ਸਤਰ_ਦਲੀਲਾਂ;  // List of command line argument encountered
    // List of arguments encounterd, and a bool in needed for rerunning --dump-inputs
    std::list<std::pair<std::list<std::string>, ਤਾਰਕਿਕ>> m_all_ਦਲੀਲਾਂ;
    VInsertion_ਸੈੱਟ<std::string> m_incDirUsers;  // Include directories (ordered)
    std::list<string> m_incDirFallbacks;  // Include directories (ordered)
    std::ਸੈੱਟ<string> m_inc_Dir_Fallback_ਸੈੱਟ;  // Include directories (for removing duplicates)
    std::map<ਅਚਲ string, V3_Lang_ਕੋਡ> m_langExts;  // Language extension map
    VInsertion_ਸੈੱਟ<std::string> m_libExtVs;  // Library extensions (ordered)
    DirMap m_dirMap;  // Directory listing

    // ACCESSOR METHODS
    ਖਾਲੀ ਜੋੜੋ_Inc_Dir_User(ਅਚਲ string& incdir) {
        ਅਚਲ string& dir = V3Os::filenameCleanup(incdir);
        ਅਚਲ ਤਾਰਕਿਕ inserted = m_incDirUsers.ਪਾਓ(dir);
        ਜੇ (inserted) {
            // cppcheck-suppress stlFindInsert  // cppcheck 1.90 bug
            m_incDirFallbacks.remove(dir);  // User has priority over Fallback
            m_inc_Dir_Fallback_ਸੈੱਟ.erase(dir);  // User has priority over Fallback
        }
    }
    ਖਾਲੀ ਜੋੜੋ_Inc_Dir_Fallback(ਅਚਲ string& incdir) {
        ਅਚਲ string& dir = V3Os::filenameCleanup(incdir);
        ਜੇ (!m_incDirUsers.exists(dir)) {  // User has priority over Fallback
            ਅਚਲ ਸਵੈਚਾਲਿਤ it_ਮਿਲਿਆ_Pair = m_inc_Dir_Fallback_ਸੈੱਟ.ਪਾਓ(dir);
            ਜੇ (it_ਮਿਲਿਆ_Pair.ਦੂਜਾ) m_incDirFallbacks.ਧੱਕੋ_ਪਿੱਠ(dir);
        }
    }
    ਖਾਲੀ ਜੋੜੋ_Lang_Ext(ਅਚਲ string& langext, ਅਚਲ V3_Lang_ਕੋਡ& lc) {
        // New language extension replaces any pre-existing one.
        string addext = langext;
        ਜੇ (addext[0] == '.') addext = addext.substr(1);
        (ਖਾਲੀ)m_langExts.erase(addext);
        m_langExts[addext] = lc;
    }

    ਖਾਲੀ ਜੋੜੋ_Lib_Ext_V(ਅਚਲ string& libext) { m_libExtVs.ਪਾਓ(libext); }
    V3OptionsImp() = ਮੂਲ;
    ~V3OptionsImp() = ਮੂਲ;
};

//######################################################################
// V3LangCode class functions

V3_Lang_ਕੋਡ::V3_Lang_ਕੋਡ(ਅਚਲ ਅੱਖਰ* textp) {
    // Return code for given string, or ERROR, which is a bad code
    ਲਈ (ਪੂਰਨ codei = V3_Lang_ਕੋਡ::L_ERROR; codei < V3_Lang_ਕੋਡ::ENUM_ਸਿਰਾ; ++codei) {
        ਅਚਲ V3_Lang_ਕੋਡ ਕੋਡ{codei};
        ਜੇ (0 == VL_STRCASECMP(textp, ਕੋਡ.ascii())) {
            m_e = ਕੋਡ;
            ਵਾਪਸ;
        }
    }
    m_e = V3_Lang_ਕੋਡ::L_ERROR;
}

//######################################################################
// VTimescale class functions

VTimescale::VTimescale(ਅਚਲ string& ਮੁੱਲ, ਤਾਰਕਿਕ& badr)
    : m_e{VTimescale::NONE} {
    badr = ਸੱਚ;
    ਅਚਲ string spaceless = VString::removeWhitespace(ਮੁੱਲ);
    ਲਈ (ਪੂਰਨ i = TS_100S; i < ENUM_ਸਿਰਾ; ++i) {
        ਅਚਲ VTimescale ts{i};
        ਜੇ (spaceless == ts.ascii()) {
            badr = ਝੂਠ;
            m_e = ts.m_e;
            ਰੋਕੋ;
        }
    }
}

//######################################################################
// V3HierarchicalBlockOption class functions

// Parse "--hierarchical-block orig_name,mangled_name,param0_name,param0_value,... " option.
// The format of value is as same as -G option. (can be string literal surrounded by ")
V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option::V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option(ਅਚਲ string& opts) {
    VStringList vals;
    ਤਾਰਕਿਕ ਵਿੱਚ_Str = ਝੂਠ;
    string cur;
    ਸਥਿਰ ਅਚਲ string hier_ਖੰਡ("--hierarchical-block");
    File_ਸਤਰ cmdfl(File_ਸਤਰ::command_ਸਤਰ_Filename());
    // Split by ','. If ',' appears between "", that is not a separator.
    ਲਈ (string::const_iterator it = opts.begin(); it != opts.ਸਿਰਾ();) {
        ਜੇ (ਵਿੱਚ_Str) {
            ਜੇ (*it == '\\') {
                ++it;
                ਜੇ (it == opts.ਸਿਰਾ()) {
                    cmdfl.v3error(hier_ਖੰਡ + " must not end with \\");
                    ਰੋਕੋ;
                }
                ਜੇ (*it != '"' && *it != '\\') {
                    cmdfl.v3error(hier_ਖੰਡ + " does not allow '" + *it + "' after \\");
                    ਰੋਕੋ;
                }
                cur.ਧੱਕੋ_ਪਿੱਠ(*it);
                ++it;
            } ਨਹੀਂਤਾਂ ਜੇ (*it == '"') {  // end of string
                cur.ਧੱਕੋ_ਪਿੱਠ(*it);
                vals.ਧੱਕੋ_ਪਿੱਠ(cur);
                cur.clear();
                ++it;
                ਜੇ (it != opts.ਸਿਰਾ()) {
                    ਜੇ (*it != ',') {
                        cmdfl.v3error(hier_ਖੰਡ + " expects ',', but '" + *it + "' is passed");
                        ਰੋਕੋ;
                    }
                    ++it;
                    ਜੇ (it == opts.ਸਿਰਾ()) {
                        cmdfl.v3error(hier_ਖੰਡ + " must not end with ','");
                        ਰੋਕੋ;
                    }
                    ਵਿੱਚ_Str = *it == '"';
                    cur.ਧੱਕੋ_ਪਿੱਠ(*it);
                    ++it;
                }
            } ਨਹੀਂਤਾਂ {
                cur.ਧੱਕੋ_ਪਿੱਠ(*it);
                ++it;
            }
        } ਨਹੀਂਤਾਂ {
            ਜੇ (*it == '"') {
                cmdfl.v3error(hier_ਖੰਡ + " does not allow '\"' in the middle of literal");
                ਰੋਕੋ;
            }
            ਜੇ (*it == ',') {  // end of this parameter
                vals.ਧੱਕੋ_ਪਿੱਠ(cur);
                cur.clear();
                ++it;
                ਜੇ (it == opts.ਸਿਰਾ()) {
                    cmdfl.v3error(hier_ਖੰਡ + " must not end with ','");
                    ਰੋਕੋ;
                }
                ਵਿੱਚ_Str = *it == '"';
            }
            cur.ਧੱਕੋ_ਪਿੱਠ(*it);
            ++it;
        }
    }
    ਜੇ (!cur.ਖਾਲੀ_ਥਾਵਾਂ()) vals.ਧੱਕੋ_ਪਿੱਠ(cur);
    ਜੇ (vals.ਮਾਪ() >= 2) {
        ਜੇ (vals.ਮਾਪ() % 2) {
            cmdfl.v3error(hier_ਖੰਡ + " requires the number of entries to be even");
        }
        m_origName = vals[0];
        m_mangledName = vals[1];
    } ਨਹੀਂਤਾਂ {
        cmdfl.v3error(hier_ਖੰਡ + " requires at least two comma-separated values");
    }
    ਲਈ (ਮਾਪ_t i = 2; i + 1 < vals.ਮਾਪ(); i += 2) {
        ਅਚਲ ਤਾਰਕਿਕ inserted = m_parameters.emplace(vals[i], vals[i + 1]).ਦੂਜਾ;
        ਜੇ (!inserted) {
            cmdfl.v3error("Module name '" + vals[i] + "' is duplicated in " + hier_ਖੰਡ);
        }
    }
}

//######################################################################
// V3Options class functions

ਖਾਲੀ VTimescale::parseSlashed(File_ਸਤਰ* fl, ਅਚਲ ਅੱਖਰ* textp, VTimescale& unitr,
                              VTimescale& precr, ਤਾਰਕਿਕ allow_ਖਾਲੀ_ਥਾਵਾਂ) {
    // Parse `timescale of <number><units> / <number><units>
    unitr = VTimescale::NONE;
    precr = VTimescale::NONE;

    ਅਚਲ ਅੱਖਰ* cp = textp;
    ਲਈ (; std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp); ++cp) {}
    ਅਚਲ ਅੱਖਰ* ਅਚਲ unitp = cp;
    ਲਈ (; *cp && *cp != '/'; ++cp) {}
    ਅਚਲ string unitStr(unitp, cp - unitp);
    ਲਈ (; std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp); ++cp) {}
    string precStr;
    ਜੇ (*cp == '/') {
        ++cp;
        ਲਈ (; std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp); ++cp) {}
        ਅਚਲ ਅੱਖਰ* ਅਚਲ precp = cp;
        ਲਈ (; *cp && *cp != '/'; ++cp) {}
        precStr = string(precp, cp - precp);
    }
    ਲਈ (; std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*cp); ++cp) {}
    ਜੇ (*cp) {
        fl->v3error("`timescale syntax error: '" << textp << "'");
        ਵਾਪਸ;
    }

    ਤਾਰਕਿਕ unitbad = ਝੂਠ;
    ਅਚਲ VTimescale unit{unitStr, unitbad /*ref*/};
    ਜੇ (unitbad && !(unitStr.ਖਾਲੀ_ਥਾਵਾਂ() && allow_ਖਾਲੀ_ਥਾਵਾਂ)) {
        fl->v3error("`timescale timeunit syntax error: '" << unitStr << "'");
        ਵਾਪਸ;
    }
    unitr = unit;

    ਜੇ (!precStr.ਖਾਲੀ_ਥਾਵਾਂ()) {
        VTimescale prec{VTimescale::NONE};
        ਤਾਰਕਿਕ precbad;
        prec = VTimescale{precStr, precbad /*ref*/};
        ਜੇ (precbad) {
            fl->v3error("`timescale timeprecision syntax error: '" << precStr << "'");
            ਵਾਪਸ;
        }
        ਜੇ (!unit.isNone() && !prec.isNone() && unit < prec) {
            fl->v3error("`timescale timeunit '"
                        << unitStr << "' must be greater than or equal to timeprecision '"
                        << precStr << "'");
            ਵਾਪਸ;
        }
        precr = prec;
    }
}

//######################################################################
// V3Options class functions

ਖਾਲੀ V3Options::ਜੋੜੋ_Inc_Dir_User(ਅਚਲ string& incdir) { m_impp->ਜੋੜੋ_Inc_Dir_User(incdir); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Inc_Dir_Fallback(ਅਚਲ string& incdir) { m_impp->ਜੋੜੋ_Inc_Dir_Fallback(incdir); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Lang_Ext(ਅਚਲ string& langext, ਅਚਲ V3_Lang_ਕੋਡ& lc) {
    m_impp->ਜੋੜੋ_Lang_Ext(langext, lc);
}
ਖਾਲੀ V3Options::ਜੋੜੋ_Lib_Ext_V(ਅਚਲ string& libext) { m_impp->ਜੋੜੋ_Lib_Ext_V(libext); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Define(ਅਚਲ string& defline, ਤਾਰਕਿਕ allowPlus) VL_MT_DISABLED {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ਖੱਬੇ = defline;
    ਜਦੋਂ (ਖੱਬੇ != "") {
        string def = ਖੱਬੇ;
        string::ਮਾਪ_type pos;
        ਜੇ (allowPlus && ((pos = ਖੱਬੇ.ਲੱਭੋ('+')) != string::npos)) {
            ਖੱਬੇ = ਖੱਬੇ.substr(pos + 1);
            def.erase(pos);
        } ਨਹੀਂਤਾਂ {
            ਖੱਬੇ = "";
        }
        string ਮੁੱਲ;
        ਜੇ ((pos = def.ਲੱਭੋ('=')) != string::npos) {
            ਮੁੱਲ = def.substr(pos + 1);
            def.erase(pos);
        }
        V3PreShell::define_Cmd_ਸਤਰ(def, ਮੁੱਲ);
    }
}
ਖਾਲੀ V3Options::ਜੋੜੋ_Parameter(ਅਚਲ string& paramline, ਤਾਰਕਿਕ allowPlus) {
    // Split +define+foo=value into the appropriate parts and parse
    // Optional + says to allow multiple defines on the line
    // + is not quotable, as other simulators do not allow that
    string ਖੱਬੇ = paramline;
    ਜਦੋਂ (ਖੱਬੇ != "") {
        string param = ਖੱਬੇ;
        string::ਮਾਪ_type pos;
        ਜੇ (allowPlus && ((pos = ਖੱਬੇ.ਲੱਭੋ('+')) != string::npos)) {
            ਖੱਬੇ = ਖੱਬੇ.substr(pos + 1);
            param.erase(pos);
        } ਨਹੀਂਤਾਂ {
            ਖੱਬੇ = "";
        }
        string ਮੁੱਲ;
        ਜੇ ((pos = param.ਲੱਭੋ('=')) != string::npos) {
            ਮੁੱਲ = param.substr(pos + 1);
            param.erase(pos);
        }
        UINFO(4, "Add parameter" << param << "=" << ਮੁੱਲ);
        (ਖਾਲੀ)m_parameters.erase(param);
        m_parameters[param] = ਮੁੱਲ;
    }
}

ਤਾਰਕਿਕ V3Options::hasParameter(ਅਚਲ string& name) {
    ਵਾਪਸ m_parameters.ਲੱਭੋ(name) != m_parameters.ਸਿਰਾ();
}

string V3Options::parameter(ਅਚਲ string& name) {
    string ਮੁੱਲ = m_parameters.ਲੱਭੋ(name)->ਦੂਜਾ;
    m_parameters.erase(m_parameters.ਲੱਭੋ(name));
    ਵਾਪਸ ਮੁੱਲ;
}

ਖਾਲੀ V3Options::checkParameters() {
    ਜੇ (!m_parameters.ਖਾਲੀ_ਥਾਵਾਂ()) {
        std::stringstream ਸੁਨੇਹਾ_2;
        ਸੁਨੇਹਾ_2 << "Parameters from the command line were not found in the design:";
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& i : m_parameters) ਸੁਨੇਹਾ_2 << " " << i.ਪਹਿਲਾ;
        v3error(ਸੁਨੇਹਾ_2.str());
    }
}

ਖਾਲੀ V3Options::ਜੋੜੋ_Cpp_File(ਅਚਲ string& filename) { m_cppFiles.ਪਾਓ(filename); }
ਖਾਲੀ V3Options::ਜੋੜੋ_CFlags(ਅਚਲ string& filename) { m_cFlags.ਧੱਕੋ_ਪਿੱਠ(filename); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Lib_Map_File(ਅਚਲ string& filename) { m_libmapFiles.ਪਾਓ(filename); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Compiler_Includes(ਅਚਲ string& filename) {
    m_compilerIncludes.ਪਾਓ(filename);
}
ਖਾਲੀ V3Options::ਜੋੜੋ_Ld_Lib_File(ਅਚਲ string& filename) {
    m_ldLibFiles.ਧੱਕੋ_ਪਿੱਠ(filename);
    ਜੋੜੋ_Ld_Libs(filename);
}
ਖਾਲੀ V3Options::ਜੋੜੋ_Ld_Libs(ਅਚਲ string& filename) { m_ldLibs.ਧੱਕੋ_ਪਿੱਠ(filename); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Make_Flags(ਅਚਲ string& filename) { m_makeFlags.ਧੱਕੋ_ਪਿੱਠ(filename); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Future(ਅਚਲ string& flag) { m_futures.ਪਾਓ(flag); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Future0(ਅਚਲ string& flag) { m_future0s.ਪਾਓ(flag); }
ਖਾਲੀ V3Options::ਜੋੜੋ_Future1(ਅਚਲ string& flag) { m_future1s.ਪਾਓ(flag); }
ਤਾਰਕਿਕ V3Options::isFuture(ਅਚਲ string& flag) ਅਚਲ {
    ਵਾਪਸ m_futures.ਲੱਭੋ(flag) != m_futures.ਸਿਰਾ();
}
ਤਾਰਕਿਕ V3Options::isFuture0(ਅਚਲ string& flag) ਅਚਲ {
    ਵਾਪਸ m_future0s.ਲੱਭੋ(flag) != m_future0s.ਸਿਰਾ();
}
ਤਾਰਕਿਕ V3Options::isFuture1(ਅਚਲ string& flag) ਅਚਲ {
    ਵਾਪਸ m_future1s.ਲੱਭੋ(flag) != m_future1s.ਸਿਰਾ();
}
ਤਾਰਕਿਕ V3Options::isLibraryFile(ਅਚਲ string& filename, ਅਚਲ string& libname) ਅਚਲ {
    ਵਾਪਸ m_libraryFiles.ਲੱਭੋ({filename, libname}) != m_libraryFiles.ਸਿਰਾ();
}
ਖਾਲੀ V3Options::ਜੋੜੋ_Library_File(ਅਚਲ string& filename, ਅਚਲ string& libname) {
    m_libraryFiles.ਪਾਓ({filename, libname});
}
ਖਾਲੀ V3Options::ਜੋੜੋ_VFile(ਅਚਲ string& filename, ਅਚਲ string& libname) {
    // We use a list for v files, because it's legal to have includes
    // in a specific order and multiple of them.
    m_vFiles.ਧੱਕੋ_ਪਿੱਠ({filename, libname});  // Not emplace
}
ਖਾਲੀ V3Options::ਜੋੜੋ_Vlt_File(ਅਚਲ string& filename, ਅਚਲ string& libname) {
    m_vltFiles.ਪਾਓ({filename, libname});
}
ਖਾਲੀ V3Options::ਜੋੜੋ_Force_Inc(ਅਚਲ string& filename) { m_forceIncs.ਧੱਕੋ_ਪਿੱਠ(filename); }

ਖਾਲੀ V3Options::ਜੋੜੋ_ਸਤਰ_Arg(ਅਚਲ string& arg) { m_impp->m_ਸਤਰ_ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(arg); }

ਖਾਲੀ V3Options::ਜੋੜੋ_Arg(ਅੱਖਰ** argv, ਮਾਪ_t ਗਿਣਤੀ_2, ਤਾਰਕਿਕ isForRerun) {
    m_impp->m_all_ਦਲੀਲਾਂ.emplace_ਪਿੱਠ();
    std::pair<std::list<std::string>, ਤਾਰਕਿਕ>& pair = m_impp->m_all_ਦਲੀਲਾਂ.ਪਿੱਠ();
    pair.ਦੂਜਾ = isForRerun;
    ਲਈ (ਮਾਪ_t n = 0; n < ਗਿਣਤੀ_2; ++n) pair.ਪਹਿਲਾ.emplace_ਪਿੱਠ(argv[n]);
}

ਖਾਲੀ V3Options::ਜੋੜੋ_Arg(ਅਚਲ std::string& arg, ਤਾਰਕਿਕ isForRerun) {
    m_impp->m_all_ਦਲੀਲਾਂ.emplace_ਪਿੱਠ();
    std::pair<std::list<std::string>, ਤਾਰਕਿਕ>& pair = m_impp->m_all_ਦਲੀਲਾਂ.ਪਿੱਠ();
    pair.ਦੂਜਾ = isForRerun;
    pair.ਪਹਿਲਾ.emplace_ਪਿੱਠ(arg);
}

ਅਚਲ std::list<std::pair<std::list<std::string>, ਤਾਰਕਿਕ>>& V3Options::all_ਦਲੀਲਾਂ() ਅਚਲ {
    ਵਾਪਸ m_impp->m_all_ਦਲੀਲਾਂ;
}

string V3Options::all_ਦਲੀਲਾਂ_String() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    string ਨਤੀਜਾ;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_impp->m_all_ਦਲੀਲਾਂ) {
        ਲਈ (ਅਚਲ string& arg : pair.ਪਹਿਲਾ) {
            ਜੇ (!ਨਤੀਜਾ.ਖਾਲੀ_ਥਾਵਾਂ()) ਨਤੀਜਾ += " ";
            ਨਤੀਜਾ += arg;
        }
    }
    ਵਾਪਸ ਨਤੀਜਾ;
}

// Delete some options for Verilation of the hierarchical blocks.
string V3Options::all_ਦਲੀਲਾਂ_String_For_Hier_ਖੰਡ(ਤਾਰਕਿਕ for_ਸਿਖਰ) ਅਚਲ {
    // cppcheck-suppress shadowFunction
    std::ਸੈੱਟ<string> vFiles;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& vFile : m_vFiles) vFiles.ਪਾਓ(vFile.filename());
    string ਬਾਹਰਲਾ;
    ਤਾਰਕਿਕ stripArg = ਝੂਠ;
    ਤਾਰਕਿਕ strip_Arg_If_ਅੰਕ = ਝੂਠ;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& pair : m_impp->m_all_ਦਲੀਲਾਂ) {
        ਲਈ (ਅਚਲ string& arg : pair.ਪਹਿਲਾ) {
            ਜੇ (stripArg) {
                stripArg = ਝੂਠ;
                ਜਾਰੀ;
            }
            ਜੇ (strip_Arg_If_ਅੰਕ) {
                strip_Arg_If_ਅੰਕ = ਝੂਠ;
                ਜੇ (ਅੰਕ_ਹੈ(arg[0])) ਜਾਰੀ;
            }
            ਪੂਰਨ skip = 0;
            ਜੇ (arg.ਲੰਮਾਈ() >= 2 && arg[0] == '-' && arg[1] == '-') {
                skip = 2;
            } ਨਹੀਂਤਾਂ ਜੇ (arg.ਲੰਮਾਈ() >= 1 && arg[0] == '-') {
                skip = 1;
            }
            ਜੇ (skip > 0) {  // arg is an option
                ਅਚਲ string opt = arg.substr(skip);  // Remove '-' in the beginning
                ਅਚਲ ਪੂਰਨ ਅੰਕ_Strip = strip_Options_For_ਬੱਚਾ_ਲੜੀਵਾਰ(opt, for_ਸਿਖਰ);
                ਜੇ (ਅੰਕ_Strip) {
                    UASSERT(0 <= ਅੰਕ_Strip && ਅੰਕ_Strip <= 3, "should be one of 0, 1, 2, 3");
                    ਜੇ (ਅੰਕ_Strip == 2) stripArg = ਸੱਚ;
                    ਜੇ (ਅੰਕ_Strip == 3) strip_Arg_If_ਅੰਕ = ਸੱਚ;
                    ਜਾਰੀ;
                }
            } ਨਹੀਂਤਾਂ {  // Not an option
                ਜੇ (vFiles.ਲੱਭੋ(arg) != vFiles.ਸਿਰਾ()  // Remove HDL
                    || m_cppFiles.ਲੱਭੋ(arg) != m_cppFiles.ਸਿਰਾ()) {  // Remove C++
                    ਜਾਰੀ;
                }
            }
            ਜੇ (ਬਾਹਰਲਾ != "") ਬਾਹਰਲਾ += " ";
            // Don't use opt here because '-' is removed in arg
            // Use double quote because arg may contain whitespaces
            ਬਾਹਰਲਾ += '"' + VString::quoteAny(arg, '"', '\\') + '"';
        }
    }
    ਵਾਪਸ ਬਾਹਰਲਾ;
}

ਖਾਲੀ V3Options::cc_ਸੈੱਟ() {  // --cc
    m_ਬਾਹਰਲਾ_Format_Ok = ਸੱਚ;
    m_systemC = ਝੂਠ;
}

ਖਾਲੀ V3Options::decorations(File_ਸਤਰ* fl, ਅਚਲ string& arg) {  // --decorations
    ਜੇ (arg == "none") {
        m_decoration = ਝੂਠ;
        m_decorationNodes = ਝੂਠ;
    } ਨਹੀਂਤਾਂ ਜੇ (arg == "node") {
        m_decoration = ਸੱਚ;
        m_decorationNodes = ਸੱਚ;
    } ਨਹੀਂਤਾਂ ਜੇ (arg == "medium") {
        m_decoration = ਸੱਚ;
        m_decorationNodes = ਝੂਠ;
    } ਨਹੀਂਤਾਂ {
        fl->v3error("Unknown setting for --decorations: '"
                    << arg << "'\n"
                    << fl->warn_ਹੋਰ() << "... Suggest 'none', 'medium', or 'node'");
    }
}

std::ਸਦਿਸ਼<std::string> V3Options::traceClassBases() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    std::ਸਦਿਸ਼<std::string> ਨਤੀਜਾ;
    ਜੇ (traceEnabledFst()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("VerilatedFst");
    ਜੇ (traceEnabledSaif()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("VerilatedSaif");
    ਜੇ (traceEnabledVcd()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("VerilatedVcd");
    ਵਾਪਸ ਨਤੀਜਾ;
}
std::ਸਦਿਸ਼<std::string> V3Options::traceClassLangs() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    std::ਸਦਿਸ਼<std::string> ਨਤੀਜਾ;
    ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& cbase : traceClassBases())
        ਨਤੀਜਾ.emplace_ਪਿੱਠ(cbase + (systemC() ? "Sc" : "C"));
    ਵਾਪਸ ਨਤੀਜਾ;
}
std::ਸਦਿਸ਼<std::string> V3Options::trace_ਸਰੋਤ_Bases() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    std::ਸਦਿਸ਼<std::string> ਨਤੀਜਾ;
    ਜੇ (traceEnabledFst()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_fst");
    ਜੇ (traceEnabledSaif()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_saif");
    ਜੇ (traceEnabledVcd()) ਨਤੀਜਾ.emplace_ਪਿੱਠ("verilated_vcd");
    ਵਾਪਸ ਨਤੀਜਾ;
}
std::ਸਦਿਸ਼<std::string> V3Options::trace_ਸਰੋਤ_Langs() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    std::ਸਦਿਸ਼<std::string> ਨਤੀਜਾ = trace_ਸਰੋਤ_Bases();
    ਲਈ (std::string& str : ਨਤੀਜਾ) str += systemC() ? "_sc"s : "_c"s;
    ਵਾਪਸ ਨਤੀਜਾ;
}
std::string V3Options::trace_Class_ਅਧਾਰ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().ਖਾਲੀ_ਥਾਵਾਂ(), "Call traceClassBase only when trace() enabled");
    ਵਾਪਸ traceClassBases().front();
}
std::string V3Options::traceClassLang() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    // Deprecated - Needs to be fixed to support multiple trace, issue #5813
    UASSERT(!traceClassBases().ਖਾਲੀ_ਥਾਵਾਂ(), "Call traceClassLang only when trace() enabled");
    ਵਾਪਸ traceClassLangs().front();
}

//######################################################################
// File searching

ਤਾਰਕਿਕ V3Options::file_Stat_ਸਾਧਾਰਨ(ਅਚਲ string& filename) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ਬਣਤਰ stat sstat;  // Stat information
    ਅਚਲ ਪੂਰਨ err = stat(filename.c_str(), &sstat);
    ਜੇ (err != 0) ਵਾਪਸ ਝੂਠ;
    ਜੇ (S_ISDIR(sstat.st_ਬਹੁਲਕ)) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ ਸੱਚ;
}

string V3Options::fileExists(ਅਚਲ string& filename) {
    // Surprisingly, for some other simulators, this process
    // is quite slow; presumably because of re-reading each directory
    // many times.  So we read a whole dir at once and cache it

    ਅਚਲ string dir = V3Os::filenameDir(filename);
    ਅਚਲ string basename = V3Os::filenameNonDir(filename);

    ਸਵੈਚਾਲਿਤ diriter = m_impp->m_dirMap.ਲੱਭੋ(dir);
    ਜੇ (diriter == m_impp->m_dirMap.ਸਿਰਾ()) {
        // Read the listing
        m_impp->m_dirMap.emplace(dir, std::ਸੈੱਟ<string>());
        diriter = m_impp->m_dirMap.ਲੱਭੋ(dir);

        std::ਸੈੱਟ<string>* ਅਚਲ setp = &(diriter->ਦੂਜਾ);

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ _MSC_VER
        ਕੋਸ਼ਿਸ਼ {
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& dir_ਇੰਦਰਾਜ : std::filesystem::directory_iterator(dir.c_str()))
                setp->ਪਾਓ(dir_ਇੰਦਰਾਜ.ਰਸਤਾ().filename().string());
        } ਫੜੋ (std::filesystem::filesystem_error ਅਚਲ& ex) {
            (ਖਾਲੀ)ex;
            ਵਾਪਸ "";
        }
#ਨਹੀਂਤਾਂ
        ਜੇ (DIR* ਅਚਲ dirp = opendir(dir.c_str())) {
            ਜਦੋਂ (ਬਣਤਰ dirent* direntp = readdir(dirp)) setp->ਪਾਓ(direntp->d_name);
            closedir(dirp);
        }
#ਜੇ_ਅੰਤ
    }
    // Find it
    ਅਚਲ std::ਸੈੱਟ<string>* ਅਚਲ filesetp = &(diriter->ਦੂਜਾ);
    ਅਚਲ ਸਵੈਚਾਲਿਤ fileiter = filesetp->ਲੱਭੋ(basename);
    ਜੇ (fileiter == filesetp->ਸਿਰਾ()) ਵਾਪਸ "";  // Not found
    // Check if it is a directory, ignore if so
    ਅਚਲ string filename_ਬਾਹਰਲਾ = V3Os::filenameJoin(dir, basename);
    ਜੇ (!file_Stat_ਸਾਧਾਰਨ(filename_ਬਾਹਰਲਾ)) ਵਾਪਸ "";  // Directory
    ਵਾਪਸ filename_ਬਾਹਰਲਾ;
}

string V3Options::file_ਰਸਤਾ_Check_One_Dir(ਅਚਲ string& modname, ਅਚਲ string& dirname) {
    ਲਈ (ਅਚਲ string& i : m_impp->m_libExtVs) {
        ਅਚਲ string fn = V3Os::filenameJoin(dirname, modname + i);
        ਅਚਲ string exists = fileExists(fn);
        ਜੇ (exists != "") ਵਾਪਸ exists;
    }
    ਵਾਪਸ "";
}

// Checks if a option needs to be stripped for child run of hierarchical Verilation.
// 0: Keep the option including its argument
// 1: Delete the option which has no argument
// 2: Delete the option and its argument
// 3: Delete the option and its argument if it is a number
ਪੂਰਨ V3Options::strip_Options_For_ਬੱਚਾ_ਲੜੀਵਾਰ(ਅਚਲ string& opt, ਤਾਰਕਿਕ for_ਸਿਖਰ) {
    // Options to strip for both the top wrapper, and hier blocks
    ਸਥਿਰ ਅਚਲ std::unordered_map<std::string, ਪੂਰਨ> commonOpts{
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
    ਜੇ (commonOpts.ਗਿਣਤੀ_2(opt)) ਵਾਪਸ commonOpts.at(opt);

    // Options to strip only for hier blocks
    ਜੇ (!for_ਸਿਖਰ) {
        ਸਥਿਰ ਅਚਲ std::unordered_map<std::string, ਪੂਰਨ> subOpts{
            {"cc", 1},
            {"exe", 1},
            {"sc", 1},
            {"binary", 1},
        };
        ਜੇ (subOpts.ਗਿਣਤੀ_2(opt)) ਵਾਪਸ subOpts.at(opt);
        ਜੇ (VString::startsWith(opt, "G")) ਵਾਪਸ 1;
        ਜੇ (VString::startsWith(opt, "pvalue")) ਵਾਪਸ 1;
    }

    // Do not strip
    ਵਾਪਸ 0;
}

ਖਾਲੀ V3Options::validateIdentifier(File_ਸਤਰ* fl, ਅਚਲ string& arg, ਅਚਲ string& opt) {
    ਜੇ (!VString::isIdentifier(arg)) {
        fl->v3error(opt << " argument must be a legal C++ identifier: '" << arg << "'");
    }
}

string V3Options::file_ਰਸਤਾ(File_ਸਤਰ* fl, ਅਚਲ string& modname, ਅਚਲ string& lastpath,
                           ਅਚਲ string& errmsg) {  // Error prefix or "" to suppress error
    // Find a filename to read the specified module name,
    // using the incdir and libext's.
    // Return "" if not found.
    ਅਚਲ string filename = V3Os::filenameCleanup(VName::dehash(modname));
    ਜੇ (!V3Os::filenameIsRel(filename)) {
        // filename is an absolute path, so can find getStdPackagePath()/getStdWaiverPath()
        ਅਚਲ string exists = file_ਰਸਤਾ_Check_One_Dir(filename, "");
        ਜੇ (exists != "") ਵਾਪਸ exists;
    }
    ਲਈ (ਅਚਲ string& dir : m_impp->m_incDirUsers) {
        ਅਚਲ string exists = file_ਰਸਤਾ_Check_One_Dir(filename, dir);
        ਜੇ (exists != "") ਵਾਪਸ exists;
    }
    ਲਈ (ਅਚਲ string& dir : m_impp->m_incDirFallbacks) {
        ਅਚਲ string exists = file_ਰਸਤਾ_Check_One_Dir(filename, dir);
        ਜੇ (exists != "") ਵਾਪਸ exists;
    }

    ਜੇ (m_relativeIncludes) {
        ਅਚਲ string exists = file_ਰਸਤਾ_Check_One_Dir(filename, lastpath);
        ਜੇ (exists != "") ਵਾਪਸ V3Os::filename_Real_ਰਸਤਾ(exists);
    }

    // Warn and return not found
    ਜੇ (errmsg != "") {
        fl->v3error(errmsg << "'"s << filename << "'\n"s << fl->warnContextPrimary()
                           << V3Error::warnAdditionalInfo() << file_ਰਸਤਾ_Looked_ਸੁਨੇਹਾ(fl, filename));
    }
    ਵਾਪਸ "";
}

string V3Options::file_ਰਸਤਾ_Looked_ਸੁਨੇਹਾ(File_ਸਤਰ* /*fl*/, ਅਚਲ string& modname) {
    ਸਥਿਰ ਤਾਰਕਿਕ s_shown_notfound_ਸੁਨੇਹਾ = ਝੂਠ;
    std::ostringstream ss;
    ਜੇ (modname.ਲੱਭੋ("__Vhsh") != string::npos) {
        ss << V3Error::warn_ਹੋਰ() << "... Note: Name is longer than 127 characters; automatic"
           << " file lookup may have failed due to OS filename length limits.\n";
        ss << V3Error::warn_ਹੋਰ() << "... Suggest putting filename with this module/package"
           << " onto command line instead.\n";
    } ਨਹੀਂਤਾਂ ਜੇ (!s_shown_notfound_ਸੁਨੇਹਾ) {
        s_shown_notfound_ਸੁਨੇਹਾ = ਸੱਚ;
        ਜੇ (m_impp->m_incDirUsers.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ss << V3Error::warn_ਹੋਰ()
               << "... This may be because there's no search path specified with -I<dir>.\n";
        }
        ss << V3Error::warn_ਹੋਰ() << "... Looked in:\n";
        ਲਈ (ਅਚਲ string& dir : m_impp->m_incDirUsers) {
            ਲਈ (ਅਚਲ string& ext : m_impp->m_libExtVs) {
                ਅਚਲ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_ਹੋਰ() << "     " << fn << "\n";
            }
        }
        ਲਈ (ਅਚਲ string& dir : m_impp->m_incDirFallbacks) {
            ਲਈ (ਅਚਲ string& ext : m_impp->m_libExtVs) {
                ਅਚਲ string fn = V3Os::filenameJoin(dir, modname + ext);
                ss << V3Error::warn_ਹੋਰ() << "     " << fn << "\n";
            }
        }
        ss << V3Error::warn_ਹੋਰ() << "... With current working directory '" << V3Os::cwd()
           << "'\n";
    }
    ਵਾਪਸ ss.str();
}

//! Determine what language is associated with a filename

//! If we recognize the extension, use its language, otherwise, use the
//! default language.
V3_Lang_ਕੋਡ V3Options::fileLanguage(ਅਚਲ string& filename) {
    string ext = V3Os::filenameNonDir(filename);
    string::ਮਾਪ_type pos;
    ਜੇ (filename == V3Options::get_Std_Package_ਰਸਤਾ() || filename == V3Options::get_Std_Waiver_ਰਸਤਾ()) {
        ਵਾਪਸ V3_Lang_ਕੋਡ::mostRecent();
    } ਨਹੀਂਤਾਂ ਜੇ ((pos = ext.rfind('.')) != string::npos) {
        ext.erase(0, pos + 1);
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_impp->m_langExts.ਲੱਭੋ(ext);
        ਜੇ (it != m_impp->m_langExts.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;
    }
    ਵਾਪਸ m_defaultLanguage;
}

//######################################################################
// Environment

string V3Options::getenvBuiltins(ਅਚਲ string& var) {
    // If update below, also update V3Options::showVersion()
    ਜੇ (var == "MAKE") {
        ਵਾਪਸ getenvMAKE();
    } ਨਹੀਂਤਾਂ ਜੇ (var == "PERL") {
        ਵਾਪਸ getenvPERL();
    } ਨਹੀਂਤਾਂ ਜੇ (var == "PYTHON3") {
        ਵਾਪਸ getenvPYTHON3();
    } ਨਹੀਂਤਾਂ ਜੇ (var == "SYSTEMC") {
        ਵਾਪਸ getenvSYSTEMC();
    } ਨਹੀਂਤਾਂ ਜੇ (var == "SYSTEMC_ARCH") {
        ਵਾਪਸ getenvSYSTEMC_ARCH();
    } ਨਹੀਂਤਾਂ ਜੇ (var == "SYSTEMC_INCLUDE") {
        ਵਾਪਸ getenvSYSTEMC_INCLUDE();
    } ਨਹੀਂਤਾਂ ਜੇ (var == "SYSTEMC_LIBDIR") {
        ਵਾਪਸ getenvSYSTEMC_LIBDIR();
    } ਨਹੀਂਤਾਂ ਜੇ (var == "VERILATOR_ROOT") {
        ਵਾਪਸ getenv_VERILATOR_ਜੜ੍ਹ();
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ V3Os::getenvStr(var, "");
    }
}

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ __FreeBSD__
string V3Options::getenvMAKE() { ਵਾਪਸ V3Os::getenvStr("MAKE", "gmake"); }
#ਨਹੀਂਤਾਂ
string V3Options::getenvMAKE() { ਵਾਪਸ V3Os::getenvStr("MAKE", "make"); }
#ਜੇ_ਅੰਤ

string V3Options::getenvMAKEFLAGS() {  //
    ਵਾਪਸ V3Os::getenvStr("MAKEFLAGS", "");
}

string V3Options::getenvPERL() {  //
    ਵਾਪਸ V3Os::filenameCleanup(V3Os::getenvStr("PERL", "perl"));
}

string V3Options::getenvPYTHON3() {  //
    ਵਾਪਸ V3Os::filenameCleanup(V3Os::getenvStr("PYTHON3", "python3"));
}

string V3Options::getenvSYSTEMC() {
    string var = V3Os::getenvStr("SYSTEMC", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ਅਚਲ string defenv = string{DEFENV_SYSTEMC};
    ਜੇ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC", var, "Hardcoded at build time");
    }
    ਵਾਪਸ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_ARCH() {
    string var = V3Os::getenvStr("SYSTEMC_ARCH", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ਅਚਲ string defenv = string{DEFENV_SYSTEMC_ARCH};
    ਜੇ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_ARCH", var, "Hardcoded at build time");
    }
    ਜੇ (var == "") {
#ਜੇ defined(__MINGW32__)
        // Hardcoded with MINGW current version. Would like a better way.
        ਅਚਲ string sysname = "MINGW32_NT-5.0";
        var = "mingw32";
#ਜਾਂਜੇ defined(_WIN32)
        ਅਚਲ string sysname = "WIN32";
        var = "win32";
#ਨਹੀਂਤਾਂ
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        ਬਣਤਰ utsname uts;
        uname(&uts);
        ਅਚਲ string sysname = VString::downcase(uts.sysname);  // aka  'uname -s'
        ਜੇ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*solaris*"))) {
            var = "gccsparcOS5";  // LCOV_EXCL_LINE
        } ਨਹੀਂਤਾਂ ਜੇ (VL_UNCOVERABLE(VString::wildmatch(sysname.c_str(), "*cygwin*"))) {
            var = "cygwin";  // LCOV_EXCL_LINE
        } ਨਹੀਂਤਾਂ {
            var = "linux";
        }
#ਜੇ_ਅੰਤ
        V3Os::setenvStr("SYSTEMC_ARCH", var, "From sysname '" + sysname + "'");
    }
    ਵਾਪਸ var;
}

string V3Options::getenvSYSTEMC_INCLUDE() {
    string var = V3Os::getenvStr("SYSTEMC_INCLUDE", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ਅਚਲ string defenv = string{DEFENV_SYSTEMC_INCLUDE};
    ਜੇ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_INCLUDE", var, "Hardcoded at build time");
    }
    ਜੇ (var == "") {
        ਅਚਲ string sc = getenvSYSTEMC();
        ਜੇ (sc != "") var = V3Os::filenameJoin(sc, "include");
    }
    ਵਾਪਸ V3Os::filenameCleanup(var);
}

string V3Options::getenvSYSTEMC_LIBDIR() {
    string var = V3Os::getenvStr("SYSTEMC_LIBDIR", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ਅਚਲ string defenv = string{DEFENV_SYSTEMC_LIBDIR};
    ਜੇ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("SYSTEMC_LIBDIR", var, "Hardcoded at build time");
    }
    ਜੇ (var == "") {
        ਅਚਲ string sc = getenvSYSTEMC();
        ਅਚਲ string arch = getenvSYSTEMC_ARCH();
        ਜੇ (sc != "" && arch != "") var = V3Os::filenameJoin(sc, "lib-" + arch);
    }
    ਵਾਪਸ V3Os::filenameCleanup(var);
}

string V3Options::getenv_VERILATOR_ਜੜ੍ਹ() {
    string var = V3Os::getenvStr("VERILATOR_ROOT", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ਅਚਲ string defenv = string{DEFENV_VERILATOR_ਜੜ੍ਹ};
    ਜੇ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_ROOT", var, "Hardcoded at build time");
    }
    ਜੇ (var == "") v3fatal("$VERILATOR_ROOT needs to be in environment\n");
    ਵਾਪਸ V3Os::filenameCleanup(var);
}

string V3Options::getenvVERILATOR_SOLVER() {
    string var = V3Os::getenvStr("VERILATOR_SOLVER", "");
    // Treat compiled-in DEFENV string literals as C-strings to enable
    // binary patching for relocatable installs (e.g. conda)
    ਅਚਲ string defenv = string{DEFENV_VERILATOR_SOLVER};
    ਜੇ (var == "" && defenv != "") {
        var = defenv;
        V3Os::setenvStr("VERILATOR_SOLVER", var, "Hardcoded at build time");
    }
    ਵਾਪਸ var;
}

string V3Options::get_Std_Package_ਰਸਤਾ() {
    ਵਾਪਸ V3Os::filenameJoin(getenv_VERILATOR_ਜੜ੍ਹ(), "include", "verilated_std.sv");
}
string V3Options::get_Std_Waiver_ਰਸਤਾ() {
    ਵਾਪਸ V3Os::filenameJoin(getenv_VERILATOR_ਜੜ੍ਹ(), "include", "verilated_std_waiver.vlt");
}

string V3Options::getSupported(ਅਚਲ string& var) {
    // If update below, also update V3Options::showVersion()
    ਜੇ (var == "COROUTINES" && coroutineSupport()) {
        ਵਾਪਸ "1";
    } ਨਹੀਂਤਾਂ ਜੇ (var == "TSAN" && tsanSupport()) {
        ਵਾਪਸ "1";
    } ਨਹੀਂਤਾਂ ਜੇ (var == "DEV_ASAN" && devAsan()) {
        ਵਾਪਸ "1";
    } ਨਹੀਂਤਾਂ ਜੇ (var == "DEV_GCOV" && devGcov()) {
        ਵਾਪਸ "1";
        // cppcheck-suppress knownConditionTrueFalse
    } ਨਹੀਂਤਾਂ ਜੇ (var == "SYSTEMC" && systemCFound()) {
        ਵਾਪਸ "1";
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ "";
    }
}

ਤਾਰਕਿਕ V3Options::systemCSystemWide() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_SYSTEMC
    ਵਾਪਸ ਸੱਚ;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ ਝੂਠ;
#ਜੇ_ਅੰਤ
}

ਤਾਰਕਿਕ V3Options::systemCFound() {
    ਵਾਪਸ (systemCSystemWide()
            || (!getenvSYSTEMC_INCLUDE().ਖਾਲੀ_ਥਾਵਾਂ() && !getenvSYSTEMC_LIBDIR().ਖਾਲੀ_ਥਾਵਾਂ()));
}

ਤਾਰਕਿਕ V3Options::coroutineSupport() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_COROUTINES
    ਵਾਪਸ ਸੱਚ;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ ਝੂਠ;
#ਜੇ_ਅੰਤ
}

ਤਾਰਕਿਕ V3Options::devAsan() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_DEV_ASAN
    ਵਾਪਸ ਸੱਚ;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ ਝੂਠ;
#ਜੇ_ਅੰਤ
}

ਤਾਰਕਿਕ V3Options::devGcov() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_DEV_GCOV
    ਵਾਪਸ ਸੱਚ;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ ਝੂਠ;
#ਜੇ_ਅੰਤ
}

ਤਾਰਕਿਕ V3Options::tsanSupport() {
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ HAVE_TSAN
    ਵਾਪਸ ਸੱਚ;
#ਨਹੀਂਤਾਂ
    ਵਾਪਸ ਝੂਠ;
#ਜੇ_ਅੰਤ
}

//######################################################################
// V3 Options notification methods

ਖਾਲੀ V3Options::notify() VL_MT_DISABLED {
    // Notify that all arguments have been passed and final modification can be made.
    File_ਸਤਰ* ਅਚਲ cmdfl = ਨਵਾਂ File_ਸਤਰ{File_ਸਤਰ::command_ਸਤਰ_Filename()};

    ਜੇ (!ਬਾਹਰਲਾ_Format_Ok() && v3Global.opt.ਮੁੱਖ()) cc_ਸੈੱਟ();  // --main implies --cc if not provided
    ਜੇ (!ਬਾਹਰਲਾ_Format_Ok() && !dpiHdrOnly() && !lintOnly() && !preprocOnly() && !serializeOnly()) {
        v3fatal("verilator: Need --binary, --cc, --sc, --dpi-hdr-only, --lint-only, "
                "--json-only or --E option");
    }

    ਜੇ (m_build && (m_gmake || m_makeJson)) {
        cmdfl->v3error("--make cannot be used together with --build. Suggest see manual");
    }

    // m_build, m_preprocOnly, m_dpiHdrOnly, m_lintOnly, and m_jsonOnly are mutually
    // exclusive
    std::ਸਦਿਸ਼<std::string> backendFlags;
    ਜੇ (m_build) {
        ਜੇ (m_ਦੁਆਧਾਰੀ)
            backendFlags.emplace_ਪਿੱਠ("--binary");
        ਨਹੀਂਤਾਂ
            backendFlags.emplace_ਪਿੱਠ("--build");
    }
    ਜੇ (m_preprocOnly) backendFlags.emplace_ਪਿੱਠ("-E");
    ਜੇ (m_dpiHdrOnly) backendFlags.emplace_ਪਿੱਠ("--dpi-hdr-only");
    ਜੇ (m_lintOnly) backendFlags.emplace_ਪਿੱਠ("--lint-only");
    ਜੇ (m_jsonOnly) backendFlags.emplace_ਪਿੱਠ("--json-only");
    ਜੇ (backendFlags.ਮਾਪ() > 1) {
        std::string backendFlagsString = backendFlags.front();
        ਲਈ (ਮਾਪ_t i = 1; i < backendFlags.ਮਾਪ(); i++) {
            backendFlagsString += ", " + backendFlags[i];
        }
        v3error("The following cannot be used together: " + backendFlagsString
                + ". Suggest see manual");
    }

    ਜੇ (m_exe && !v3Global.opt.libCreate().ਖਾਲੀ_ਥਾਵਾਂ()) {
        cmdfl->v3error("--exe cannot be used together with --lib-create. Suggest see manual");
    }

    // Make sure at least one make system is enabled
    ਜੇ (!m_gmake && !m_makeJson) m_gmake = ਸੱਚ;

    ਜੇ (m_ਦਰਜਾਵਾਰ && (m_hier_ਬੱਚਾ || !m_hier_ਖੰਡਾਂ.ਖਾਲੀ_ਥਾਵਾਂ())) {
        cmdfl->v3error(
            "--hierarchical must not be set with --hierarchical-child or --hierarchical-block");
    }
    ਜੇ (m_hier_ਬੱਚਾ) {
        ਜੇ (m_hier_ਖੰਡਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            cmdfl->v3error("--hierarchical-block must be set when --hierarchical-child is set");
        }
        m_main = ਝੂਠ;
    }

    ਜੇ (trace() && !traceEnabledFst() && !traceEnabledSaif() && !traceEnabledVcd()) {
        m_traceEnabledVcd = ਸੱਚ;  // No format, with --trace means wanted --trace-vcd
    }
    ਜੇ (traceEnabledFst() || traceEnabledSaif() || traceEnabledVcd()) m_trace = ਸੱਚ;
    ਅਚਲ ਪੂਰਨ ntraces = traceEnabledFst() + traceEnabledSaif() + traceEnabledVcd();
    ਜੇ (ntraces > 1)  // Issue #5813
        cmdfl->v3error("Only one of --trace-fst, --trace-saif or --trace--vcd may be used");

    ਜੇ (protectIds()) {
        ਜੇ (all_ਜਨਤਕ()) {
            // We always call protect() on names, we don't check if public or not
            // Hence any external references wouldn't be able to find the refed public object.
            cmdfl->v3warn(E_UNSUPPORTED, "Unsupported: Using --protect-ids with --public\n"  //
                                             + cmdfl->warn_ਹੋਰ() + "... Suggest remove --public.");
        }
        ਜੇ (trace()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --trace may expose private design details\n"
                              + cmdfl->warn_ਹੋਰ() + "... Suggest remove --trace.");
        }
        ਜੇ (vpi()) {
            cmdfl->v3warn(INSECURE,
                          "Using --protect-ids with --vpi may expose private design details\n"
                              + cmdfl->warn_ਹੋਰ() + "... Suggest remove --vpi.");
        }
    }

    // Default some options if not turned on or off
    ਜੇ (v3Global.opt.skipIdentical().isDefault()) {
        v3Global.opt.m_skipIdentical.ਸੈੱਟ_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }
    ਜੇ (v3Global.opt.makeDepend().isDefault()) {
        v3Global.opt.m_makeDepend.ਸੈੱਟ_True_Or_False(  //
            !v3Global.opt.dpiHdrOnly()  //
            && !v3Global.opt.lintOnly()  //
            && !v3Global.opt.preprocOnly()  //
            && !v3Global.opt.serializeOnly());
    }

    ਜੇ (m_timing.isDefault() && (v3Global.opt.jsonOnly() || v3Global.opt.lintOnly()))
        v3Global.opt.m_timing.ਸੈੱਟ_True_Or_False(ਸੱਚ);

    // Default split limits if not specified
    ਜੇ (m_output_ਵੰਡੋ_CFuncs < 0) m_output_ਵੰਡੋ_CFuncs = m_output_ਵੰਡੋ;
    ਜੇ (m_output_ਵੰਡੋ_CTrace < 0) m_output_ਵੰਡੋ_CTrace = m_output_ਵੰਡੋ;

    ਜੇ (v3Global.opt.ਮੁੱਖ() && v3Global.opt.systemC()) {
        cmdfl->v3warn(E_UNSUPPORTED,
                      "--main not usable with SystemC. Suggest see examples for sc_main().");
    }

    ਜੇ (fourstate()) {
        cmdfl->v3warn(FUTURE, "--fourstate is not supported as is under development");
        ਜੇ (ਦਰਜਾਵਾਰ()) {
            cmdfl->v3warn(E_UNSUPPORTED,
                          "--fourstate is not supported with hierarchical Verilation");
        }
    }

    ਜੇ (coverage() && savable()) {
        cmdfl->v3error("Unsupported: --coverage and --savable not supported together");
    }
    ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True() && savable()) {
        cmdfl->v3error("Unsupported: --timing and --savable not supported together");
    }

    // --dump-tree-dot will turn on tree dumping.
    ਜੇ (!m_dumpLevel.ਗਿਣਤੀ_2("tree") && m_dumpLevel.ਗਿਣਤੀ_2("tree-dot")) {
        m_dumpLevel["tree"] = m_dumpLevel["tree-dot"];
    }

    // Sanity check of expected configuration
    UASSERT(ਧਾਗੇ() >= 1, "'threads()' must return a value >= 1");
    ਜੇ (m_outputGroups == -1) m_outputGroups = (m_buildJobs != -1) ? m_buildJobs : 0;
    ਜੇ (m_buildJobs == -1) m_buildJobs = 1;
    ਜੇ (m_verilateJobs == -1) m_verilateJobs = 1;

    // Preprocessor defines based on options used
    ਜੇ (timing().is_ਸੈੱਟ_True()) V3PreShell::define_Cmd_ਸਤਰ("VERILATOR_TIMING", "1");

    // If VPI is used, and no explicit ico change detect option was passed, disable it by default
    ਜੇ (m_vpi.isTrue() && m_fIcoChangeDetect.isDefault()) m_fIcoChangeDetect.ਸੈੱਟ_True_Or_False(ਝੂਠ);

    // === Leave last
    // Mark options as available
    m_ਉਪਲਬਧ = ਸੱਚ;
}

//######################################################################
// V3 Options accessors

string V3Options::ਸੰਸਕਰਣ() VL_ਸ਼ੁੱਧ {
    string ver = PACKAGE_STRING;
    ver += " rev " + cvt_ਮੰਜ਼ਿਲ_Str(DTVERSION_rev);
    ਵਾਪਸ ver;
}

string V3Options::protect_ਕੁੰਜੀ_Defaulted() VL_MT_ਸੁਰੱਖਿਅਤ {
    ਸਥਿਰ V3Mutex s_mutex;
    ਅਚਲ V3_ਤਾਲਾ_Guard ਤਾਲਾ_2{s_mutex};
    ਜੇ (m_protect_ਕੁੰਜੀ.ਖਾਲੀ_ਥਾਵਾਂ()) {
        // Create a key with a human-readable symbol-like name.
        // This conversion drops ~2 bits of entropy out of 256, shouldn't matter.
        VHashSha512 digest{V3Os::trueRandom(64)};
        m_protect_ਕੁੰਜੀ = "VL-KEY-" + digest.digestBase64();
    }
    ਵਾਪਸ m_protect_ਕੁੰਜੀ;
}

ਖਾਲੀ V3Options::throwSigsegv() {  // LCOV_EXCL_START
#ਜੇ !(defined(VL_CPPCHECK) || defined(__clang_analyzer__))
    // clang-format off
    *static_cast<ਅਸਥਿਰ ਅੱਖਰ*>(ਖਾਲੀ_ਸੂਚਕ) = 0;  // Intentional core dump, ignore warnings here
    // clang-format on
#ਜੇ_ਅੰਤ
}  // LCOV_EXCL_STOP

VTimescale V3Options::ਸਮਾਂ_Compute_Prec(ਅਚਲ VTimescale& flag) ਅਚਲ {
    ਜੇ (!ਸਮਾਂ_Override_Prec().isNone()) {
        ਵਾਪਸ ਸਮਾਂ_Override_Prec();
    } ਨਹੀਂਤਾਂ ਜੇ (flag.isNone()) {
        ਵਾਪਸ ਸਮਾਂ_Default_Prec();
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ flag;
    }
}

VTimescale V3Options::ਸਮਾਂ_Compute_Unit(ਅਚਲ VTimescale& flag) ਅਚਲ {
    ਜੇ (!ਸਮਾਂ_Override_Unit().isNone()) {
        ਵਾਪਸ ਸਮਾਂ_Override_Unit();
    } ਨਹੀਂਤਾਂ ਜੇ (flag.isNone()) {
        ਵਾਪਸ ਸਮਾਂ_Default_Unit();
    } ਨਹੀਂਤਾਂ {
        ਵਾਪਸ flag;
    }
}

//######################################################################
// V3 Options utilities

string V3Options::argString(ਪੂਰਨ argc, ਅੱਖਰ** argv) {
    // Return list of arguments as simple string
    string opts;
    ਲਈ (ਪੂਰਨ i = 0; i < argc; ++i) {
        ਜੇ (i != 0) opts += " ";
        opts += string{argv[i]};
    }
    ਵਾਪਸ opts;
}

//######################################################################
// V3 Options Parsing

ਖਾਲੀ V3Options::parseOpts(File_ਸਤਰ* fl, ਪੂਰਨ argc, ਅੱਖਰ** argv) VL_MT_DISABLED {
    // Save command line options
    ਲਈ (ਪੂਰਨ i = 0; i < argc; ++i) ਜੋੜੋ_ਸਤਰ_Arg(argv[i]);

    // Parse all options
    // Initial entry point from Verilator.cpp
    parseOptsList(fl, ".", argc, argv);

    // Default certain options and error check
    // Detailed error, since this is what we often get when run with minimal arguments
    ਜੇ (vFiles().ਖਾਲੀ_ਥਾਵਾਂ()) {
        v3fatal("verilator: No input Verilog file specified on command line, "
                "see 'verilator --help' for more information\n");
    }

    // Default prefix to the filename
    ਜੇ (ਅਗੇਤਰ() == "" && ਸਿਖਰ_Module() != "") m_ਅਗੇਤਰ = "V"s + ਸਿਖਰ_Module();  // Already encoded
    ਜੇ (ਅਗੇਤਰ() == "" && vFiles().ਮਾਪ() >= 1)
        m_ਅਗੇਤਰ
            = "V"s + Ast_ਗੰਢ::encodeName(V3Os::filenameNonDirExt(vFiles().begin()->filename()));
    ਜੇ (mod_ਅਗੇਤਰ() == "") m_mod_ਅਗੇਤਰ = ਅਗੇਤਰ();

    // Find files in makedir
    ਜੋੜੋ_Inc_Dir_Fallback(makeDir());
}

//======================================================================

ਤਾਰਕਿਕ V3Options::suffixed(ਅਚਲ string& sw, ਅਚਲ ਅੱਖਰ* arg) {
    ਜੇ (std::ਲੜੀ_ਲੰਬਾਈ(arg) > sw.ਲੰਮਾਈ()) ਵਾਪਸ ਝੂਠ;
    ਵਾਪਸ (0 == std::strcmp(sw.c_str() + sw.ਲੰਮਾਈ() - std::ਲੜੀ_ਲੰਬਾਈ(arg), arg));
}

ਖਾਲੀ V3Options::parseOptsList(File_ਸਤਰ* fl, ਅਚਲ string& optdir, ਪੂਰਨ argc,
                              ਅੱਖਰ** argv) VL_MT_DISABLED {
    // Parse parameters
    // Note argc and argv DO NOT INCLUDE the filename in [0]!!!
    // May be called recursively when there are -f files.

    V3OptionParser parser;
    ਅਚਲ V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3_OPTION_PARSER_DECL_ਟੈਗ_ਸੂਚੀ;

    ਅਚਲ ਸਵੈਚਾਲਿਤ callStrSetter = [ਇਹ](ਖਾਲੀ (V3Options::*cbStr)(ਅਚਲ string&)) {
        ਵਾਪਸ [ਇਹ, cbStr](ਅਚਲ string& v) { (ਇਹ->*cbStr)(v); };
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
    DECL_OPTION("+define+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) VL_MT_DISABLED {
        ਜੋੜੋ_Define(optp, ਸੱਚ);
    }).notForRerun();
    DECL_OPTION("+incdir+", Cb_Partial_ਮੇਲ, [ਇਹ, &optdir](ਅਚਲ ਅੱਖਰ* optp) {
        string dirs = optp;
        string::ਮਾਪ_type pos;
        ਜਦੋਂ ((pos = dirs.ਲੱਭੋ('+')) != string::npos) {
            ਜੋੜੋ_Inc_Dir_User(parseFileArg(optdir, dirs.substr(0, pos)));
            dirs = dirs.substr(pos + 1);
        }
        ਜੋੜੋ_Inc_Dir_User(parseFileArg(optdir, dirs));
    }).notForRerun();
    DECL_OPTION("+libext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        string exts = optp;
        string::ਮਾਪ_type pos;
        ਜਦੋਂ ((pos = exts.ਲੱਭੋ('+')) != string::npos) {
            ਜੋੜੋ_Lib_Ext_V(exts.substr(0, pos));
            exts = exts.substr(pos + 1);
        }
        ਜੋੜੋ_Lib_Ext_V(exts);
    }).notForRerun();
    DECL_OPTION("+librescan", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+notimingchecks", CbCall, []() {}).notForRerun();  // NOP
    DECL_OPTION("+systemverilogext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+verilog1995ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+verilog2001ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-1995ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1364_1995);
    }).notForRerun();
    DECL_OPTION("+1364-2001ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1364_2001);
    }).notForRerun();
    DECL_OPTION("+1364-2005ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1364_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2005ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1800_2005);
    }).notForRerun();
    DECL_OPTION("+1800-2009ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1800_2009);
    }).notForRerun();
    DECL_OPTION("+1800-2012ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1800_2012);
    }).notForRerun();
    DECL_OPTION("+1800-2017ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1800_2017);
    }).notForRerun();
    DECL_OPTION("+1800-2023ext+", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Lang_Ext(optp, V3_Lang_ਕੋਡ::L1800_2023);
    }).notForRerun();

    // Minus options
    DECL_OPTION("-aslr", CbOnOff, [](ਤਾਰਕਿਕ) {});  // Processed only in bin/verilator shell
    DECL_OPTION("-assert", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_assert = flag;
        m_assertCase = flag;
    });
    DECL_OPTION("-assert-case", OnOff, &m_assertCase);
    DECL_OPTION("-assert-unroll-limit", CbVal, [fl](ਅਚਲ ਅੱਖਰ*) {
        fl->v3warn(DEPRECATED, "Option '--assert-unroll-limit' is deprecated and has no effect.");
    }).notForRerun();
    DECL_OPTION("-autoflush", OnOff, &m_autoflush);

    DECL_OPTION("-bbox-sys", OnOff, &m_bboxSys);
    DECL_OPTION("-bbox-unsup", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_bboxUnsup = flag;
        File_ਸਤਰ::globalWarnOff(V3_Error_ਕੋਡ::E_UNSUPPORTED, ਸੱਚ);
    });
    DECL_OPTION("-binary", CbCall, [ਇਹ]() {
        m_ਦੁਆਧਾਰੀ = ਸੱਚ;
        m_build = ਸੱਚ;
        m_exe = ਸੱਚ;
        m_main = ਸੱਚ;
        ਜੇ (m_timing.isDefault()) m_timing = VOptionBool::OPT_TRUE;
    });
    DECL_OPTION("-build", ਸੈੱਟ_2, &m_build);
    DECL_OPTION("-build-dep-bin", ਸੈੱਟ_2, &m_buildDepBin);
    DECL_OPTION("-build-jobs", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        ਪੂਰਨ val = std::atoi(valp);
        ਜੇ (val < 0) {
            fl->v3error("--build-jobs requires a non-negative integer, but '" << valp
                                                                              << "' was passed");
            val = 1;
        } ਨਹੀਂਤਾਂ ਜੇ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_buildJobs = val;
    });

    DECL_OPTION("-CFLAGS", CbVal, callStrSetter(&V3Options::ਜੋੜੋ_CFlags));
    DECL_OPTION("-cc", CbCall, [ਇਹ]() { cc_ਸੈੱਟ(); });
    DECL_OPTION("-clk", CbVal, [fl](ਅਚਲ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-no-clk", CbVal, [fl](ਅਚਲ std::string&) {
        fl->v3warn(DEPRECATED, "Option '--no-clk' is deprecated and has no effect.");
    });
    DECL_OPTION("-comp-limit-blocks", ਸੈੱਟ_2, &m_comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ).undocumented();
    DECL_OPTION("-comp-limit-members", ਸੈੱਟ_2,
                &m_comp_ਹੱਦਬੰਦੀ_Members)
        .undocumented();  // Ideally power-of-two so structs stay aligned
    DECL_OPTION("-comp-limit-parens", ਸੈੱਟ_2, &m_comp_ਹੱਦਬੰਦੀ_Parens).undocumented();
    DECL_OPTION("-comp-limit-syms", CbVal, [](ਪੂਰਨ val) { VName::max_ਲੰਮਾਈ(val); }).undocumented();
    DECL_OPTION("-compiler", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        ਜੇ (!std::strcmp(valp, "clang")) {
            m_comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ = 80;  // limit unknown
            m_comp_ਹੱਦਬੰਦੀ_Members = 64;  // soft limit, has slowdown bug as of clang++ 3.8
            m_comp_ਹੱਦਬੰਦੀ_Parens = 240;  // controlled by -fbracket-depth, which defaults to 256
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "gcc")) {
            m_comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ = 0;  // Bug free
            m_comp_ਹੱਦਬੰਦੀ_Members = 64;  // soft limit, has slowdown bug as of g++ 7.1
            m_comp_ਹੱਦਬੰਦੀ_Parens = 240;  // Unlimited, but generate same code as for clang
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "msvc")) {
            m_comp_ਹੱਦਬੰਦੀ_ਖੰਡਾਂ = 80;  // 128, but allow some room
            m_comp_ਹੱਦਬੰਦੀ_Members = 0;  // probably ok, and AFAIK doesn't support anon structs
            m_comp_ਹੱਦਬੰਦੀ_Parens = 80;  // 128, but allow some room
        } ਨਹੀਂਤਾਂ {
            fl->v3error("Unknown setting for --compiler: '"
                        << valp << "'\n"
                        << fl->warn_ਹੋਰ() << "... Suggest 'clang', 'gcc', or 'msvc'");
        }
    });
    DECL_OPTION("-compiler-include", CbVal, callStrSetter(&V3Options::ਜੋੜੋ_Compiler_Includes));
    DECL_OPTION("-converge-limit", ਸੈੱਟ_2, &m_converge_ਹੱਦਬੰਦੀ);
    DECL_OPTION("-coverage", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) { coverage(flag); });
    DECL_OPTION("-coverage-branch", OnOff, &m_coverageBranch);
    DECL_OPTION("-coverage-expr", OnOff, &m_coverage_ਵਿਅੰਜਕ);
    DECL_OPTION("-coverage-expr-max", ਸੈੱਟ_2, &m_coverage_ਵਿਅੰਜਕ_Max);
    DECL_OPTION("-coverage-fsm", OnOff, &m_coverageFsm);
    DECL_OPTION("-coverage-fsm-expand", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* ਅਚਲ valp) {
        ਜੇ (!std::strcmp(valp, "auto")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO;
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "auto-expand")) {
            m_coverageFsmExpand = VFsmExpandType::AUTO_EXPAND;
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "full")) {
            m_coverageFsmExpand = VFsmExpandType::ਭਰਿਆ;
        } ਨਹੀਂਤਾਂ {
            fl->v3error("Unknown setting for --coverage-fsm-expand: '"
                        << valp << "'\n"
                        << fl->warn_ਹੋਰ() << "... Suggest 'auto', 'auto-expand', or 'full'");
        }
    });
    DECL_OPTION("-coverage-fsm-max-arcs", ਸੈੱਟ_2, &m_coverageFsmMaxArcs);
    DECL_OPTION("-coverage-line", OnOff, &m_coverage_ਸਤਰ);
    // Covergroup bins limits; the runtime indexes bins with 32 bits
    ਅਚਲ ਸਵੈਚਾਲਿਤ parse_Bins_ਹੱਦਬੰਦੀ = [fl](ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* valp, uint32_t& limitr) {
        ਅੱਖਰ* endp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਲੰਮਾ ਲੰਮਾ ਮੁੱਲ = std::strtoull(valp, &endp, 10);
        ਜੇ (*endp || ਮੁੱਲ < 1 || ਮੁੱਲ > std::numeric_limits<uint32_t>::max()) {
            fl->v3error(optp << " requires an integer from 1 to "
                             << std::numeric_limits<uint32_t>::max() << ", but '" << valp
                             << "' was passed");
            ਵਾਪਸ;
        }
        limitr = static_cast<uint32_t>(ਮੁੱਲ);
    };
    DECL_OPTION("-coverage-max-bins", CbVal, [ਇਹ, parse_Bins_ਹੱਦਬੰਦੀ](ਅਚਲ ਅੱਖਰ* valp) {
        parse_Bins_ਹੱਦਬੰਦੀ("--coverage-max-bins", valp, m_coverageMaxBins);
    });
    DECL_OPTION("-coverage-max-real-bins", CbVal, [ਇਹ, parse_Bins_ਹੱਦਬੰਦੀ](ਅਚਲ ਅੱਖਰ* valp) {
        parse_Bins_ਹੱਦਬੰਦੀ("--coverage-max-real-bins", valp, m_coverageMaxRealBins);
    });
    DECL_OPTION("-coverage-max-width", ਸੈੱਟ_2, &m_coverage_Max_ਚੌੜਾਈ);
    DECL_OPTION("-coverage-merge-instances", OnOff, &m_coverage_ਮਿਲਾਓ_Instances);
    DECL_OPTION("-coverage-per-instance", OnOff, &m_coverage_ਪ੍ਰਤੀ_Instance);
    DECL_OPTION("-coverage-toggle", OnOff, &m_coverageToggle);
    DECL_OPTION("-coverage-underscore", OnOff, &m_coverageUnderscore);
    DECL_OPTION("-coverage-user", OnOff, &m_coverageUser);

    DECL_OPTION("-D", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) VL_MT_DISABLED {
        ਜੋੜੋ_Define(valp, ਝੂਠ);
    }).notForRerun();
    DECL_OPTION("-debug", CbCall, [ਇਹ]() { ਸੈੱਟ_Debug_ਬਹੁਲਕ(3); }).notForRerun();
    DECL_OPTION("-debugi", CbVal, [ਇਹ](ਪੂਰਨ v) { ਸੈੱਟ_Debug_ਬਹੁਲਕ(v); }).notForRerun();
    DECL_OPTION("-debugi-", Cb_Partial_ਮੇਲ_Val, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* valp) {
        m_debugLevel[optp] = std::atoi(valp);
    });
    DECL_OPTION("-debug-abort", CbCall, []() {
        V3Error::vlAbort();  // LCOV_EXCL_LINE
    }).undocumented();  // See also --debug-sigseg
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ VL_ALLOC_RANDOM_CHECKS
    DECL_OPTION("-debug-alloc-random", ਸੈੱਟ_2, &m_debugAllocRandom).undocumented();
#ਜੇ_ਅੰਤ
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
    DECL_OPTION("-debug-partition", OnOff, &m_debug_ਹਿੱਸੇ_ਕਰੋ).undocumented();
    DECL_OPTION("-debug-preproc-passthru", OnOff, &m_debugPreprocPassthru).undocumented();
    DECL_OPTION("-debug-protect", OnOff, &m_debugProtect).undocumented();
    DECL_OPTION("-debug-runtime-timeout", ਸੈੱਟ_2, &m_debugRuntimeTimeout).undocumented();
    DECL_OPTION("-debug-self-test", OnOff, &m_debugSelfTest).undocumented();
    DECL_OPTION("-debug-sigsegv", CbCall, throwSigsegv).undocumented();  // See also --debug-abort
    DECL_OPTION("-debug-stack-check", OnOff, &m_debug_ਢੇਰ_Check).undocumented();
    DECL_OPTION("-debug-width", OnOff, &m_debug_ਚੌੜਾਈ).undocumented();
    DECL_OPTION("-decoration", CbCall, [ਇਹ, fl]() { decorations(fl, "medium"); });
    DECL_OPTION("-decorations", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* optp) { decorations(fl, optp); });
    DECL_OPTION("-no-decoration", CbCall, [ਇਹ, fl]() { decorations(fl, "none"); });
    DECL_OPTION("-diagnostics-sarif", OnOff, &m_diagnosticsSarif);
    DECL_OPTION("-diagnostics-sarif-output", CbVal, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        m_diagnosticsSarifOutput = optp;
        m_diagnosticsSarif = ਸੱਚ;
    });
    DECL_OPTION("-dpi-hdr-only", OnOff, &m_dpiHdrOnly);
    DECL_OPTION("-dump-", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        m_dumpLevel[optp] = 3;
    }).notForRerun();
    DECL_OPTION("-no-dump-", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        m_dumpLevel[optp] = 0;
    }).notForRerun();
    DECL_OPTION("-dumpi-", Cb_Partial_ਮੇਲ_Val, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* valp) {
        m_dumpLevel[optp] = std::atoi(valp);
    }).notForRerun();

    DECL_OPTION("-E", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        ਜੇ (flag) {
            m_stdPackage = ਝੂਠ;
            m_stdWaiver = ਝੂਠ;
        }
        m_preprocOnly = flag;
    });
    DECL_OPTION("-emit-accessors", OnOff, &m_emitAccessors);
    DECL_OPTION("-error-limit", CbVal, static_cast<ਖਾਲੀ (*)(ਪੂਰਨ)>(&V3Error::error_ਹੱਦਬੰਦੀ));
    DECL_OPTION("-exe", OnOff, &m_exe);
    DECL_OPTION("-expand-limit", CbVal,
                [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) { m_expand_ਹੱਦਬੰਦੀ = std::atoi(valp); });

    DECL_OPTION("-F", CbVal, [ਇਹ, fl, &optdir](ਅਚਲ ਅੱਖਰ* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ਸੱਚ);
    }).notForRerun();
    DECL_OPTION("-FI", CbVal,
                [ਇਹ, &optdir](ਅਚਲ ਅੱਖਰ* valp) { ਜੋੜੋ_Force_Inc(parseFileArg(optdir, valp)); });
    DECL_OPTION("-f", CbVal, [ਇਹ, fl, &optdir](ਅਚਲ ਅੱਖਰ* valp) VL_MT_DISABLED {
        parseOptsFile(fl, parseFileArg(optdir, valp), ਝੂਠ);
    }).notForRerun();
    DECL_OPTION("-flatten", OnOff, &m_flatten);
    DECL_OPTION("-fourstate", OnOff, &m_fourstate).undocumented();
    DECL_OPTION("-future0", CbVal, [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) { ਜੋੜੋ_Future0(valp); });
    DECL_OPTION("-future1", CbVal, [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) { ਜੋੜੋ_Future1(valp); });

    DECL_OPTION("-facyc-simp", FOnOff, &m_fAcycSimp);
    DECL_OPTION("-fassemble", FOnOff, &m_fAssemble);
    DECL_OPTION("-fbit-scan-loops", FOnOff, &m_f_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Loops);
    DECL_OPTION("-fbitblast", FOnOff, &m_fBitblast);
    DECL_OPTION("-fcase", CbFOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_fCaseDecoder = flag;
        m_f_Case_ਸਾਰਣੀ = flag;
        m_f_Case_ਰੁੱਖ = flag;
    });
    DECL_OPTION("-fcase-decoder", FOnOff, &m_fCaseDecoder);
    DECL_OPTION("-fcase-table", FOnOff, &m_f_Case_ਸਾਰਣੀ);
    DECL_OPTION("-fcase-tree", FOnOff, &m_f_Case_ਰੁੱਖ);
    DECL_OPTION("-fcombine", FOnOff, &m_fCombine);
    DECL_OPTION("-fconst", FOnOff, &m_fConst);
    DECL_OPTION("-fconst-before-dfg", FOnOff, &m_fConstBeforeDfg);
    DECL_OPTION("-fconst-bit-op-tree", FOnOff, &m_f_Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ);
    DECL_OPTION("-fconst-eager", FOnOff, &m_fConstEager);
    DECL_OPTION("-fdead-assigns", FOnOff, &m_fDeadAssigns);
    DECL_OPTION("-fdead-cells", FOnOff, &m_fDeadCells);
    DECL_OPTION("-fdead-methods", FOnOff, &m_fDeadMethods);
    DECL_OPTION("-fdecompose", CbFOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_fDecomposePacked = flag;
        m_fDecomposeUnpacked = flag;
    });
    DECL_OPTION("-fdecompose-packed", FOnOff, &m_fDecomposePacked);
    DECL_OPTION("-fdecompose-unpacked", FOnOff, &m_fDecomposeUnpacked);
    DECL_OPTION("-fdedup", FOnOff, &m_fDedupe);
    DECL_OPTION("-fdfg", CbFOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) { m_fDfg = flag; });
    DECL_OPTION("-fdfg-break-cycles", CbFOnOff, [fl](ਤਾਰਕਿਕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-break-cycles' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-peephole", FOnOff, &m_fDfgPeephole);
    DECL_OPTION("-fdfg-peephole-", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {  //
        m_fDfgPeepholeDisabled.erase(optp);
    });
    DECL_OPTION("-fno-dfg-peephole-", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {  //
        m_fDfgPeepholeDisabled.emplace(optp);
    });
    DECL_OPTION("-fdfg-pre-inline", CbFOnOff, [fl](ਤਾਰਕਿਕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-pre-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-post-inline", CbFOnOff, [fl](ਤਾਰਕਿਕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-post-inline' is deprecated and has no effect");
    });
    DECL_OPTION("-fdfg-push-down-sels", FOnOff, &m_f_Dfg_ਧੱਕੋ_ਥੱਲੇ_Sels);
    DECL_OPTION("-fdfg-synthesize-all", FOnOff, &m_fDfgSynthesizeAll);
    DECL_OPTION("-fdfg-scoped", CbFOnOff, [ਇਹ, fl](ਤਾਰਕਿਕ flag) {
        fl->v3warn(DEPRECATED, "Option '-fno-dfg-scoped' is deprecated, use '-fno-dfg' instead.");
        m_fDfg = flag;
    });
    DECL_OPTION("-fexpand", FOnOff, &m_fExpand);
    DECL_OPTION("-ffunc-opt", CbFOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {  //
        m_f_Func_ਵੰਡੋ_Cat = flag;
        m_fFuncBalanceCat = flag;
    });
    DECL_OPTION("-ffunc-opt-balance-cat", FOnOff, &m_fFuncBalanceCat);
    DECL_OPTION("-ffunc-opt-split-cat", FOnOff, &m_f_Func_ਵੰਡੋ_Cat);
    DECL_OPTION("-fgate", FOnOff, &m_fGate);
    DECL_OPTION("-fico-change-detect", CbFOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {  //
        m_fIcoChangeDetect.ਸੈੱਟ_True_Or_False(flag);
    });
    DECL_OPTION("-finline", FOnOff, &m_fInline);
    DECL_OPTION("-finline-cfuncs", FOnOff, &m_fInlineCFuncs);
    DECL_OPTION("-finline-funcs", FOnOff, &m_fInlineFuncs);
    DECL_OPTION("-finline-funcs-eager", FOnOff, &m_fInlineFuncsEager);
    DECL_OPTION("-flife", FOnOff, &m_fLife);
    DECL_OPTION("-flife-post", FOnOff, &m_fLifePost);
    DECL_OPTION("-flift-expr", CbFOnOff, [fl](ਤਾਰਕਿਕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-lift-expr' is deprecated and has no effect");
    });
    DECL_OPTION("-flocalize", FOnOff, &m_fLocalize);
    DECL_OPTION("-fmerge-cond", FOnOff, &m_f_ਮਿਲਾਓ_Cond);
    DECL_OPTION("-fmerge-cond-motion", FOnOff, &m_f_ਮਿਲਾਓ_Cond_Motion);
    DECL_OPTION("-fmerge-const-pool", CbFOnOff, [fl](ਤਾਰਕਿਕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-merge-const-pool' is deprecated and has no effect");
    });
    DECL_OPTION("-freloop", FOnOff, &m_fReloop);
    DECL_OPTION("-freorder", FOnOff, &m_fReorder);
    DECL_OPTION("-fslice", FOnOff, &m_f_ਟੁਕੜਾ);
    DECL_OPTION("-fslice-element-limit", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_f_ਟੁਕੜਾ_Element_ਹੱਦਬੰਦੀ = std::atoi(valp);
        ਜੇ (m_f_ਟੁਕੜਾ_Element_ਹੱਦਬੰਦੀ < 0) fl->v3fatal("--fslice-element-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-fsplit", FOnOff, &m_f_ਵੰਡੋ);
    DECL_OPTION("-fsubst", FOnOff, &m_fSubst);
    DECL_OPTION("-fsubst-const", FOnOff, &m_fSubstConst);
    DECL_OPTION("-ftable", FOnOff, &m_f_ਸਾਰਣੀ);
    DECL_OPTION("-ftaskify-all-forked", FOnOff, &m_fTaskifyAll).undocumented();  // Debug
    DECL_OPTION("-fvar-split", CbFOnOff, [fl](ਤਾਰਕਿਕ) {
        fl->v3warn(DEPRECATED, "Option '-fno-var-split' is deprecated and has no effect");
    });
    DECL_OPTION("-G", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) { ਜੋੜੋ_Parameter(optp, ਝੂਠ); });
    DECL_OPTION("-gate-stmts", ਸੈੱਟ_2, &m_gateStmts);
    DECL_OPTION("-gdb", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-gdbbt", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-generate-key", CbCall, [ਇਹ]() {
        ਬਾਹਰ << protect_ਕੁੰਜੀ_Defaulted() << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-getenv", CbVal, [](ਅਚਲ ਅੱਖਰ* valp) {
        ਬਾਹਰ << V3Options::getenvBuiltins(valp) << '\n';
        v3Global.vlExit(0);
    });
    DECL_OPTION("-get-supported", CbVal, [](ਅਚਲ ਅੱਖਰ* valp) {
        ਬਾਹਰ << V3Options::getSupported(valp) << '\n';
        v3Global.vlExit(0);
    });

    DECL_OPTION("-hierarchical", OnOff, &m_ਦਰਜਾਵਾਰ);
    DECL_OPTION("-hierarchical-block", CbVal, [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) {
        ਅਚਲ V3_ਦਰਜਾਵਾਰ_ਖੰਡ_Option opt{valp};
        m_hier_ਖੰਡਾਂ.emplace(opt.mangledName(), opt);
    });
    DECL_OPTION("-hierarchical-child", ਸੈੱਟ_2, &m_hier_ਬੱਚਾ);
    DECL_OPTION("-hierarchical-params-file", CbVal,
                [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) { m_hierParamsFile.ਧੱਕੋ_ਪਿੱਠ({optp, work()}); });

    DECL_OPTION("-I", Cb_Partial_ਮੇਲ, [ਇਹ, &optdir](ਅਚਲ ਅੱਖਰ* optp) {
        ਜੋੜੋ_Inc_Dir_User(parseFileArg(optdir, optp));
    }).notForRerun();
    DECL_OPTION("-if-depth", ਸੈੱਟ_2, &m_if_ਡੂੰਘਾਈ);
    DECL_OPTION("-ignc", OnOff, &m_ignc).undocumented();
    DECL_OPTION("-inline-cfuncs", ਸੈੱਟ_2, &m_inlineCFuncs);
    DECL_OPTION("-inline-cfuncs-product", ਸੈੱਟ_2, &m_inline_CFuncs_ਗੁਣਨਫਲ);
    DECL_OPTION("-inline-mult", ਸੈੱਟ_2, &m_inlineMult);
    DECL_OPTION("-instr-count-dpi", CbVal, [ਇਹ, fl](ਪੂਰਨ val) {
        m_instr_ਗਿਣਤੀ_Dpi = val;
        ਜੇ (m_instr_ਗਿਣਤੀ_Dpi < 0) fl->v3fatal("--instr-count-dpi must be non-negative: " << val);
    });

    DECL_OPTION("-json-edit-nums", OnOff, &m_jsonEditNums);
    DECL_OPTION("-json-ids", OnOff, &m_jsonIds);
    DECL_OPTION("-json-only", OnOff, &m_jsonOnly);
    DECL_OPTION("-json-only-meta-output", CbVal, [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) {
        m_jsonOnlyMetaOutput = valp;
        m_jsonOnly = ਸੱਚ;
    });
    DECL_OPTION("-json-only-output", CbVal, [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) {
        m_jsonOnlyOutput = valp;
        m_jsonOnly = ਸੱਚ;
    });

    DECL_OPTION("-LDFLAGS", CbVal, callStrSetter(&V3Options::ਜੋੜੋ_Ld_Libs));
    DECL_OPTION("-l2-name", ਸੈੱਟ_2, &m_l2Name);
    DECL_OPTION("-no-l2name", CbCall, [ਇਹ]() { m_l2Name = ""; }).undocumented();  // Historical
    DECL_OPTION("-l2name", CbCall, [ਇਹ]() { m_l2Name = "v"; }).undocumented();  // Historical
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਸੈੱਟ_Lang = [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        ਅਚਲ V3_Lang_ਕੋਡ optval{valp};
        ਜੇ (optval.legal()) {
            m_defaultLanguage = optval;
        } ਨਹੀਂਤਾਂ {
            VSpellCheck spell;
            ਲਈ (ਪੂਰਨ i = V3_Lang_ਕੋਡ::L_ERROR + 1; i < V3_Lang_ਕੋਡ::ENUM_ਸਿਰਾ; ++i) {
                spell.ਧੱਕੋ_Candidate(V3_Lang_ਕੋਡ{i}.ascii());
            }
            fl->v3error("Unknown language specified: " << valp << spell.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(valp));
        }
    };
    DECL_OPTION("-default-language", CbVal, ਸੈੱਟ_Lang).notForRerun();
    DECL_OPTION("-language", CbVal, ਸੈੱਟ_Lang).notForRerun();
    DECL_OPTION("-libmap", CbVal, [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) { ਜੋੜੋ_Lib_Map_File(valp); });
    DECL_OPTION("-lib-create", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        validateIdentifier(fl, valp, "--lib-create");
        m_libCreate = valp;
    });
    DECL_OPTION("-lint-only", OnOff, &m_lintOnly);
    DECL_OPTION("-localize-max-size", ਸੈੱਟ_2, &m_localize_Max_ਮਾਪ);

    DECL_OPTION("-MAKEFLAGS", CbVal, callStrSetter(&V3Options::ਜੋੜੋ_Make_Flags));
    DECL_OPTION("-MMD", OnOff, &m_makeDepend);
    DECL_OPTION("-MP", OnOff, &m_makePhony);
    DECL_OPTION("-Mdir", CbVal, [ਇਹ](ਅਚਲ ਅੱਖਰ* valp) {
        m_makeDir = valp;
        ਜੋੜੋ_Inc_Dir_Fallback(m_makeDir);  // Need to find generated files there too
    }).notForRerun();
    DECL_OPTION("-main", OnOff, &m_main);
    DECL_OPTION("-main-top-name", ਸੈੱਟ_2, &m_main_ਸਿਖਰ_Name);
    DECL_OPTION("-make", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        ਜੇ (!std::strcmp(valp, "gmake")) {
            m_gmake = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "json")) {
            m_makeJson = ਸੱਚ;
        } ਨਹੀਂਤਾਂ {
            fl->v3error("Unknown --make system specified: '" << valp << "'");
        }
    });
    DECL_OPTION("-func-recursion-depth", ਸੈੱਟ_2, &m_funcRecursion);
    DECL_OPTION("-max-num-width", ਸੈੱਟ_2, &m_max_ਅੰਕ_ਚੌੜਾਈ);
    DECL_OPTION("-mod-prefix", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        validateIdentifier(fl, valp, "--mod-prefix");
        m_mod_ਅਗੇਤਰ = valp;
    });

    DECL_OPTION("-O0", CbCall, [ਇਹ]() { optimize(0); });
    DECL_OPTION("-O1", CbCall, [ਇਹ]() { optimize(1); });
    DECL_OPTION("-O2", CbCall, [ਇਹ]() { optimize(2); });
    DECL_OPTION("-O3", CbCall, [ਇਹ]() { optimize(3); });

    DECL_OPTION("-o", ਸੈੱਟ_2, &m_exeName);
    DECL_OPTION("-order-clock-delay", CbOnOff, [fl](ਤਾਰਕਿਕ /*flag*/) {
        fl->v3warn(DEPRECATED, "Option order-clock-delay is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-output-groups", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_outputGroups = std::atoi(valp);
        ਜੇ (m_outputGroups < -1) fl->v3error("--output-groups must be >= -1: " << valp);
    });
    DECL_OPTION("-output-groups-size", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_output_Groups_ਮਾਪ = std::atoll(valp);
        ਜੇ (m_output_Groups_ਮਾਪ < 0) fl->v3error("--output-groups-size must be >= 0: " << valp);
    });
    DECL_OPTION("-output-split", ਸੈੱਟ_2, &m_output_ਵੰਡੋ);
    DECL_OPTION("-output-split-cfuncs", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_output_ਵੰਡੋ_CFuncs = std::atoi(valp);
        ਜੇ (m_output_ਵੰਡੋ_CFuncs < 0) {
            fl->v3error("--output-split-cfuncs must be >= 0: " << valp);
        }
    });
    DECL_OPTION("-output-split-ctrace", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_output_ਵੰਡੋ_CTrace = std::atoi(valp);
        ਜੇ (m_output_ਵੰਡੋ_CTrace < 0) {
            fl->v3error("--output-split-ctrace must be >= 0: " << valp);
        }
    });

    DECL_OPTION("-P", ਸੈੱਟ_2, &m_preproc_No_ਸਤਰ);
    DECL_OPTION("-pins64", CbCall, [ਇਹ]() { m_pinsBv = 65; });
    DECL_OPTION("-no-pins64", CbCall, [ਇਹ]() { m_pinsBv = 33; });
    DECL_OPTION("-pins-bv", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_pinsBv = std::atoi(valp);
        ਜੇ (m_pinsBv > 65) fl->v3error("--pins-bv maximum is 65: " << valp);
    });
    DECL_OPTION("-pins-inout-enables", OnOff, &m_pinsInoutEnables);
    DECL_OPTION("-pins-sc-uint", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_pinsScUint = flag;
        ਜੇ (!m_pinsScBigUint) m_pinsBv = 65;
    });
    DECL_OPTION("-pins-sc-uint-bool", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) { m_pinsScUintBool = flag; });
    DECL_OPTION("-pins-sc-biguint", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_pinsScBigUint = flag;
        m_pinsBv = 513;
    });
    DECL_OPTION("-pins-uint8", OnOff, &m_pinsUint8);
    DECL_OPTION("-pipe-filter", ਸੈੱਟ_2, &m_pipeFilter);
    DECL_OPTION("-pp-comments", OnOff, &m_preprocComments).undocumented();  // Renamed
    DECL_OPTION("-prefix", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        validateIdentifier(fl, valp, "--prefix");
        m_ਅਗੇਤਰ = valp;
    });
    DECL_OPTION("-preproc-comments", OnOff, &m_preprocComments);
    DECL_OPTION("-preproc-defines", OnOff, &m_preproc_ਨਿਰਧਾਰਤ_ਕਰਦਾ);
    DECL_OPTION("-preproc-resolve", OnOff, &m_preprocResolve);
    DECL_OPTION("-preproc-token-limit", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_preproc_Token_ਹੱਦਬੰਦੀ = std::atoi(valp);
        ਜੇ (m_preproc_Token_ਹੱਦਬੰਦੀ <= 0) fl->v3error("--preproc-token-limit must be > 0: " << valp);
    });
    DECL_OPTION("-private", CbCall, [ਇਹ]() { m_ਜਨਤਕ = ਝੂਠ; });
    DECL_OPTION("-prof-c", OnOff, &m_profC);
    DECL_OPTION("-prof-cfuncs", CbCall, [ਇਹ]() { m_profC = m_profCFuncs = ਸੱਚ; });
    DECL_OPTION("-prof-exec", OnOff, &m_profExec);
    DECL_OPTION("-prof-pgo", OnOff, &m_profPgo);
    DECL_OPTION("-profile-cfuncs", CbCall, [ਇਹ]() {
        m_profC = m_profCFuncs = ਸੱਚ;
    }).undocumented();  // Renamed
    DECL_OPTION("-protect-ids", OnOff, &m_protectIds);
    DECL_OPTION("-protect-key", ਸੈੱਟ_2, &m_protect_ਕੁੰਜੀ);
    DECL_OPTION("-protect-lib", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        validateIdentifier(fl, valp, "--protect-lib");
        m_libCreate = valp;
        m_protectIds = ਸੱਚ;
    });
    DECL_OPTION("-public", OnOff, &m_ਜਨਤਕ);
    DECL_OPTION("-public-depth", ਸੈੱਟ_2, &m_ਜਨਤਕ_ਡੂੰਘਾਈ);
    DECL_OPTION("-public-flat-rw", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_ਜਨਤਕ_Flat_RW = flag;
        v3Global.dpi(ਸੱਚ);
    });
    DECL_OPTION("-public-ignore", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) { m_ਜਨਤਕ_Ignore = flag; });
    DECL_OPTION("-public-params", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_ਜਨਤਕ_Params = flag;
        v3Global.dpi(ਸੱਚ);
    });
    DECL_OPTION("-pvalue+", Cb_Partial_ਮੇਲ,
                [ਇਹ](ਅਚਲ ਅੱਖਰ* varp) { ਜੋੜੋ_Parameter(varp, ਝੂਠ); });

    DECL_OPTION("-quiet", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_quietBuild = flag;
        m_quietExit = flag;
        m_quietStats = flag;
    });
    DECL_OPTION("-quiet-build", OnOff, &m_quietBuild);
    DECL_OPTION("-quiet-exit", OnOff, &m_quietExit);
    DECL_OPTION("-quiet-stats", OnOff, &m_quietStats);

    DECL_OPTION("-relative-includes", OnOff, &m_relativeIncludes);
    DECL_OPTION("-reloop-limit", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_reloop_ਹੱਦਬੰਦੀ = std::atoi(valp);
        ਜੇ (m_reloop_ਹੱਦਬੰਦੀ < 2) fl->v3error("--reloop-limit must be >= 2: " << valp);
    });
    DECL_OPTION("-report-unoptflat", OnOff, &m_reportUnoptflat);
    DECL_OPTION("-replication-limit", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_replication_ਹੱਦਬੰਦੀ = std::atoi(valp);
        ਜੇ (m_replication_ਹੱਦਬੰਦੀ < 0) fl->v3error("--replication-limit must be >= 0: " << valp);
    });
    DECL_OPTION("-rr", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-runtime-debug", CbCall, [ਇਹ, fl]() {
        decorations(fl, "node");
        ਜੋੜੋ_CFlags("-ggdb");
        ਜੋੜੋ_Ld_Libs("-ggdb");
        ਜੋੜੋ_CFlags("-fsanitize=address,undefined");
        ਜੋੜੋ_Ld_Libs("-fsanitize=address,undefined");
        ਜੋੜੋ_CFlags("-D_GLIBCXX_DEBUG");
        ਜੋੜੋ_CFlags("-DVL_DEBUG=1");
    });

    DECL_OPTION("-savable", OnOff, &m_savable);
    DECL_OPTION("-sc", CbCall, [ਇਹ]() {
        m_ਬਾਹਰਲਾ_Format_Ok = ਸੱਚ;
        m_systemC = ਸੱਚ;
    });
    DECL_OPTION("-sched-zero-delay", OnOff, &m_schedZeroDelay);
    DECL_OPTION("-skip-identical", OnOff, &m_skipIdentical);
    DECL_OPTION("-stats", OnOff, &m_stats);
    DECL_OPTION("-stats-vars", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_statsVars = flag;
        m_stats |= flag;
    });
    DECL_OPTION("-std", CbOnOff, [ਇਹ](ਤਾਰਕਿਕ flag) {
        m_stdPackage = flag;
        m_stdWaiver = flag;
    });
    DECL_OPTION("-std-package", OnOff, &m_stdPackage);
    DECL_OPTION("-std-waiver", OnOff, &m_stdWaiver);
    DECL_OPTION("-stop-fail", OnOff, &m_stopFail);
    DECL_OPTION("-sv", CbCall, [ਇਹ]() { m_defaultLanguage = V3_Lang_ਕੋਡ::L1800_2023; });

    DECL_OPTION("-no-threads", CbCall, [ਇਹ, fl]() {
        fl->v3warn(DEPRECATED, "Option --no-threads is deprecated, use '--threads 1' instead");
        m_ਧਾਗੇ = 1;
    });
    DECL_OPTION("-threads", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_ਧਾਗੇ = std::atoi(valp);
        ਜੇ (m_ਧਾਗੇ < 0) fl->v3fatal("--threads must be >= 0: " << valp);
        ਜੇ (m_ਧਾਗੇ == 0) {
            fl->v3warn(DEPRECATED, "Option --threads 0 is deprecated, use '--threads 1' instead");
            m_ਧਾਗੇ = 1;
        }
    });
    DECL_OPTION("-hierarchical-threads", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_hier_ਧਾਗੇ = std::atoi(valp);
        ਜੇ (m_hier_ਧਾਗੇ < 0) fl->v3fatal("--hierarchical-threads must be >= 0: " << valp);
    });
    DECL_OPTION("-threads-coarsen", OnOff, &m_ਧਾਗੇ_Coarsen).undocumented();  // Debug
    DECL_OPTION("-threads-dpi", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        ਜੇ (!std::strcmp(valp, "all")) {
            m_ਧਾਗੇ_Dpi_ਸ਼ੁੱਧ = ਸੱਚ;
            m_ਧਾਗੇ_Dpi_Unpure = ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "none")) {
            m_ਧਾਗੇ_Dpi_ਸ਼ੁੱਧ = ਝੂਠ;
            m_ਧਾਗੇ_Dpi_Unpure = ਝੂਠ;
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "pure")) {
            m_ਧਾਗੇ_Dpi_ਸ਼ੁੱਧ = ਸੱਚ;
            m_ਧਾਗੇ_Dpi_Unpure = ਝੂਠ;
        } ਨਹੀਂਤਾਂ {
            fl->v3error("Unknown setting for --threads-dpi: '"
                        << valp << "'\n"
                        << fl->warn_ਹੋਰ() << "... Suggest 'all', 'none', or 'pure'");
        }
    });
    DECL_OPTION("-threads-max-mtasks", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        m_ਧਾਗੇ_Max_MTasks = std::atoi(valp);
        ਜੇ (m_ਧਾਗੇ_Max_MTasks < 1) fl->v3fatal("--threads-max-mtasks must be >= 1: " << valp);
    });
    DECL_OPTION("-timescale", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/);
        ਜੇ (!unit.isNone() && ਸਮਾਂ_Override_Unit().isNone()) m_ਸਮਾਂ_Default_Unit = unit;
        ਜੇ (!prec.isNone() && ਸਮਾਂ_Override_Prec().isNone()) m_ਸਮਾਂ_Default_Prec = prec;
    });
    DECL_OPTION("-timescale-override", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        VTimescale unit;
        VTimescale prec;
        VTimescale::parseSlashed(fl, valp, unit /*ref*/, prec /*ref*/, ਸੱਚ);
        ਜੇ (!unit.isNone()) {
            m_ਸਮਾਂ_Default_Unit = unit;
            m_ਸਮਾਂ_Override_Unit = unit;
        }
        ਜੇ (!prec.isNone()) {
            m_ਸਮਾਂ_Default_Prec = prec;
            m_ਸਮਾਂ_Override_Prec = prec;
        }
    });
    DECL_OPTION("-timing", OnOff, &m_timing);
    DECL_OPTION("-top", CbVal,
                [ਇਹ](ਅਚਲ std::string& flag) { m_ਸਿਖਰ_Module = Ast_ਗੰਢ::encodeName(flag); });
    DECL_OPTION("-top-module", CbVal,
                [ਇਹ](ਅਚਲ std::string& flag) { m_ਸਿਖਰ_Module = Ast_ਗੰਢ::encodeName(flag); });
    DECL_OPTION("-top-module-encoded", ਸੈੱਟ_2, &m_ਸਿਖਰ_Module).undocumented();
    DECL_OPTION("-trace", OnOff, &m_trace);
    DECL_OPTION("-trace-saif", CbCall, [ਇਹ]() { m_traceEnabledSaif = ਸੱਚ; });
    DECL_OPTION("-trace-coverage", OnOff, &m_traceCoverage);
    DECL_OPTION("-trace-depth", ਸੈੱਟ_2, &m_trace_ਡੂੰਘਾਈ);
    DECL_OPTION("-trace-fst", CbCall, [ਇਹ]() {
        m_traceEnabledFst = ਸੱਚ;
        ਜੋੜੋ_Ld_Libs("-lz");
    });
    DECL_OPTION("-trace-fst-thread", CbCall, [fl]() {
        fl->v3warn(DEPRECATED, "Option '--trace-fst-thread' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-trace-max-array", ਸੈੱਟ_2, &m_traceMaxArray);
    DECL_OPTION("-trace-max-width", ਸੈੱਟ_2, &m_trace_Max_ਚੌੜਾਈ);
    DECL_OPTION("-trace-params", OnOff, &m_traceParams);
    DECL_OPTION("-trace-structs", OnOff, &m_traceStructs);
    DECL_OPTION("-trace-threads", CbVal, [fl](ਅਚਲ ਅੱਖਰ*) {
        fl->v3warn(DEPRECATED, "Option '--trace-threads' is deprecated and has no effect.");
    }).undocumented();
    DECL_OPTION("-no-trace-top", ਸੈੱਟ_2, &m_no_Trace_ਸਿਖਰ);
    DECL_OPTION("-trace-underscore", OnOff, &m_traceUnderscore);
    DECL_OPTION("-trace-vcd", CbCall, [ਇਹ]() { m_traceEnabledVcd = ਸੱਚ; });

    DECL_OPTION("-U", Cb_Partial_ਮੇਲ, &V3PreShell::undef);
    DECL_OPTION("-underline-zero", OnOff, &m_underlineZero).undocumented();  // Deprecated
    DECL_OPTION("-no-unlimited-stack", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-constraint-array-limit", ਸੈੱਟ_2, &m_constraint_Array_ਹੱਦਬੰਦੀ);
    DECL_OPTION("-unroll-count", ਸੈੱਟ_2, &m_unroll_ਗਿਣਤੀ).undocumented();  // Optimization tweak
    DECL_OPTION("-unroll-limit", ਸੈੱਟ_2, &m_unroll_ਹੱਦਬੰਦੀ);
    DECL_OPTION("-unroll-stmts", ਸੈੱਟ_2, &m_unrollStmts).undocumented();  // Optimization tweak
    DECL_OPTION("-unused-regexp", ਸੈੱਟ_2, &m_ਅਣਵਰਤਿਆ_Regexp);

    DECL_OPTION("-V", CbCall, [ਇਹ]() {
        show_ਸੰਸਕਰਣ(ਸੱਚ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-v", CbVal, [ਇਹ, &optdir](ਅਚਲ ਅੱਖਰ* valp) {
        V3Options::ਜੋੜੋ_Library_File(parseFileArg(optdir, valp), work());
    }).notForRerun();
    DECL_OPTION("-valgrind", CbCall, []() {});  // Processed only in bin/verilator shell
    DECL_OPTION("-verilate", OnOff, &m_verilate);
    DECL_OPTION("-verilate-jobs", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        ਪੂਰਨ val = std::atoi(valp);
        ਜੇ (val < 0) {
            fl->v3error("--verilate-jobs requires a non-negative integer, but '"
                        << valp << "' was passed");
            val = 1;
        } ਨਹੀਂਤਾਂ ਜੇ (val == 0) {
            val = VlOs::getProcessDefaultParallelism();
        }
        m_verilateJobs = val;
    });
    DECL_OPTION("-version", CbCall, [ਇਹ]() {
        show_ਸੰਸਕਰਣ(ਝੂਠ);
        v3Global.vlExit(0);
    });
    DECL_OPTION("-vpi", OnOff, &m_vpi);

    DECL_OPTION("-Wall", CbCall, []() { File_ਸਤਰ::globalWarnOff(V3_Error_ਕੋਡ::I_LINT, ਝੂਠ); });
    DECL_OPTION("-Werror-", Cb_Partial_ਮੇਲ, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* optp) {
        ਅਚਲ V3_Error_ਕੋਡ ਕੋਡ{optp};
        ਜੇ (ਕੋਡ == V3_Error_ਕੋਡ::EC_ERROR) {
            ਜੇ (!isFuture(optp)) fl->v3fatal("Unknown warning specified: -Werror-" << optp);
        } ਨਹੀਂਤਾਂ {
            V3Error::pretendError(ਕੋਡ, ਸੱਚ);
        }
    });
    DECL_OPTION("-Wfuture-", Cb_Partial_ਮੇਲ, [ਇਹ](ਅਚਲ ਅੱਖਰ* optp) {
        // Note it may not be a future option, but one that is currently implemented.
        ਜੋੜੋ_Future(optp);
    });
    DECL_OPTION("-Wno-", Cb_Partial_ਮੇਲ, [fl, &parser](ਅਚਲ ਅੱਖਰ* optp) VL_MT_DISABLED {
        ਅਚਲ string err = File_ਸਤਰ::globalWarnOffParse(optp, ਸੱਚ);
        ਜੇ (!err.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ string fullopt = "-Wno-"s + optp;
            fl->v3fatal("Unknown warning specified: " << fullopt
                                                      << parser.getSuggestion(fullopt.c_str()));
        }
    });
    ਲਈ (ਪੂਰਨ i = V3_Error_ਕੋਡ::EC_ਪਹਿਲਾ_WARN; i < V3_Error_ਕੋਡ::_ENUM_MAX; ++i) {
        // cppcheck-suppress shadowFunction
        ਲਈ (ਅਚਲ string ਅਗੇਤਰ : {"-Wno-", "-Wwarn-"})
            parser.ਜੋੜੋ_Suggestion_Candidate(ਅਗੇਤਰ + V3_Error_ਕੋਡ{i}.ascii());
    }
    DECL_OPTION("-Wno-context", CbCall, [ਇਹ]() { m_context = ਝੂਠ; });
    DECL_OPTION("-Wno-fatal", CbCall, []() { V3Error::warnFatal(ਝੂਠ); });
    DECL_OPTION("-Wno-lint", CbCall, []() { File_ਸਤਰ::globalWarnOff(V3_Error_ਕੋਡ::I_LINT, ਸੱਚ); });
    DECL_OPTION("-Wno-style", CbCall,
                []() { File_ਸਤਰ::globalWarnOff(V3_Error_ਕੋਡ::I_STYLE, ਸੱਚ); });
    DECL_OPTION("-work", ਸੈੱਟ_2, &m_work);
    DECL_OPTION("-Wpedantic", CbCall, [ਇਹ]() {
        m_pedantic = ਸੱਚ;
        V3Error::pretendError(V3_Error_ਕੋਡ::ASSIGNIN, ਝੂਠ);
    });
    DECL_OPTION("-Wwarn-", Cb_Partial_ਮੇਲ, [ਇਹ, fl, &parser](ਅਚਲ ਅੱਖਰ* optp) VL_MT_DISABLED {
        ਅਚਲ V3_Error_ਕੋਡ ਕੋਡ{optp};
        ਜੇ (ਕੋਡ == V3_Error_ਕੋਡ::EC_ERROR) {
            ਜੇ (!isFuture(optp)) {
                ਅਚਲ string fullopt = "-Wwarn-"s + optp;
                fl->v3fatal("Unknown warning specified: "
                            << fullopt << parser.getSuggestion(fullopt.c_str()));
            }
        } ਨਹੀਂਤਾਂ {
            File_ਸਤਰ::globalWarnOff(ਕੋਡ, ਝੂਠ);
            V3Error::pretendError(ਕੋਡ, ਝੂਠ);
        }
    });
    DECL_OPTION("-Wwarn-lint", CbCall,
                []() { File_ਸਤਰ::globalWarnOff(V3_Error_ਕੋਡ::I_LINT, ਝੂਠ); });
    DECL_OPTION("-Wwarn-style", CbCall,
                []() { File_ਸਤਰ::globalWarnOff(V3_Error_ਕੋਡ::I_STYLE, ਝੂਠ); });
    DECL_OPTION("-waiver-multiline", OnOff, &m_waiverMultiline);
    DECL_OPTION("-waiver-output", ਸੈੱਟ_2, &m_waiverOutput);

    DECL_OPTION("-x-assign", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        ਜੇ (!std::strcmp(valp, "0")) {
            m_x_ਸੌਂਪੋ = "0";
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "1")) {
            m_x_ਸੌਂਪੋ = "1";
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "fast")) {
            m_x_ਸੌਂਪੋ = "fast";
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "unique")) {
            m_x_ਸੌਂਪੋ = "unique";
        } ਨਹੀਂਤਾਂ {
            fl->v3error("Unknown setting for --x-assign: '"
                        << valp << "'\n"
                        << fl->warn_ਹੋਰ() << "... Suggest '0', '1', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial", CbVal, [ਇਹ, fl](ਅਚਲ ਅੱਖਰ* valp) {
        ਜੇ (!std::strcmp(valp, "0")) {
            m_xInitial = "0";
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "fast")) {
            m_xInitial = "fast";
        } ਨਹੀਂਤਾਂ ਜੇ (!std::strcmp(valp, "unique")) {
            m_xInitial = "unique";
        } ਨਹੀਂਤਾਂ {
            fl->v3error("Unknown setting for --x-initial: '"
                        << valp << "'\n"
                        << fl->warn_ਹੋਰ() << "... Suggest '0', 'fast', or 'unique'");
        }
    });
    DECL_OPTION("-x-initial-edge", OnOff, &m_x_Initial_ਕਿਨਾਰਾ);

    DECL_OPTION("-y", CbVal, [ਇਹ, &optdir](ਅਚਲ ਅੱਖਰ* valp) {
        ਜੋੜੋ_Inc_Dir_User(parseFileArg(optdir, string{valp}));
    }).notForRerun();

    parser.finalize();

    ਲਈ (ਪੂਰਨ i = 0; i < argc;) {
        UINFO(9, " Option: " << argv[i]);

        // Option
        ਜੇ (argv[i][0] == '-' || argv[i][0] == '+') {
            ਅਚਲ std::string argName = argv[i] + (argv[i][0] == argv[i][1] ? 2 : 1);
            ਪੂਰਨ consumed = 0;
            ਤਾਰਕਿਕ isForRerun = ਸੱਚ;

            // Special case for -j which has an optional argument
            ਜੇ (argv[i][0] == '-' && argName == "j") {
                consumed = 1;
                ਪੂਰਨ val = 0;
                ਜੇ (i + 1 < argc && std::ਅੰਕ_ਹੈ(argv[i + 1][0])) {
                    val = std::atoi(argv[i + 1]);  // Can't be negative due to isdigit above
                    ਜੇ (val == 0) val = VlOs::getProcessDefaultParallelism();
                    consumed = 2;
                }
                ਜੇ (m_buildJobs == -1) m_buildJobs = val;
                ਜੇ (m_verilateJobs == -1) m_verilateJobs = val;
                ਜੇ (m_outputGroups == -1) m_outputGroups = val;
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ ਸਵੈਚਾਲਿਤ pair = parser.parse(i, argc, argv);
                consumed = pair.ਪਹਿਲਾ;
                isForRerun = pair.ਦੂਜਾ;
                ਜੇ (consumed) {
                    // Already processed
                } ਨਹੀਂਤਾਂ ਜੇ (isFuture0(argName)) {
                    consumed = 1;
                } ਨਹੀਂਤਾਂ ਜੇ (isFuture1(argName)) {
                    consumed = 2;
                } ਨਹੀਂਤਾਂ {
                    fl->v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
                    consumed = 1;
                }
            }
            UASSERT(consumed, "Failed to consume argument: " << argv[i]);
            // Record all arguments except for -f and -F (their contents will be recorded instead)
            ਜੇ (argName != "f" && argName != "F") ਜੋੜੋ_Arg(argv + i, consumed, isForRerun);
            i += consumed;
            ਜਾਰੀ;
        }

        // Filename
        ਅਚਲ std::string filename = parseFileArg(optdir, argv[i]);
        ਤਾਰਕਿਕ isForRerun = ਸੱਚ;
        ਜੇ (suffixed(filename, ".cpp")  //
            || suffixed(filename, ".cxx")  //
            || suffixed(filename, ".cc")  //
            || suffixed(filename, ".c")  //
            || suffixed(filename, ".sp")) {
            V3Options::ਜੋੜੋ_Cpp_File(filename);
        } ਨਹੀਂਤਾਂ ਜੇ (suffixed(filename, ".a")  //
                   || suffixed(filename, ".o")  //
                   || suffixed(filename, ".so")) {
            V3Options::ਜੋੜੋ_Ld_Lib_File(filename);
        } ਨਹੀਂਤਾਂ ਜੇ (suffixed(filename, ".vlt")) {
            V3Options::ਜੋੜੋ_Vlt_File(filename, work());
            isForRerun = ਝੂਠ;
        } ਨਹੀਂਤਾਂ {
            V3Options::ਜੋੜੋ_VFile(filename, work());
            isForRerun = ਝੂਠ;
        }
        ਜੋੜੋ_Arg(filename, isForRerun);
        ++i;
    }

    ਜੇ (m_debugOptions) {
        parser.dumpOptions();
        v3Global.vlExit(0);
    }
}

//======================================================================

ਖਾਲੀ V3Options::parseOptsFile(File_ਸਤਰ* fl, ਅਚਲ string& filename, ਤਾਰਕਿਕ rel) VL_MT_DISABLED {
    // Read the specified -f filename and process as arguments
    UINFO(1, "Reading Options File " << filename);

    ਅਚਲ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream(filename)};
    ਜੇ (ifp->fail()) {
        fl->v3error("Cannot open -f command file: " + filename);
        ਵਾਪਸ;
    }

    // If we find any '// verilator fargs' line in the file, we will only use these,
    // otherwise we will use the file contents. This enables rerunning the output
    // produced by --dump-inptus with:
    //   verilator -f <prefix>__inputs.vpp <prefix>__inputs.vpp
    ਤਾਰਕਿਕ has_fargs = ਝੂਠ;
    std::string ਸਾਰਾ_file;

    // Parses a '// verilator fargs' or '/* verilator fargs */' comment
    ਅਚਲ ਸਵੈਚਾਲਿਤ parse_fargs_cmt = [&](ਅਚਲ ਅੱਖਰ* p) -> ਖਾਲੀ {
        ਅਚਲ ਤਾਰਕਿਕ ਖੰਡ_Cmt = p[1] == '*';
        p += 2;
        ਜਦੋਂ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*p)) ++p;
        ਜੇ (VString::startsWith(p, "verilator")) {
            p += std::ਲੜੀ_ਲੰਬਾਈ("verilator");
            ਜੇ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*p)) {
                ਜਦੋਂ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*p)) ++p;
                ਜੇ (VString::startsWith(p, "fargs")) {
                    p += std::ਲੜੀ_ਲੰਬਾਈ("fargs");
                    ਜੇ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*p)) {
                        ਜਦੋਂ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*p)) ++p;
                        ਜੇ (!has_fargs) {
                            has_fargs = ਸੱਚ;
                            ਸਾਰਾ_file.clear();
                        }
                        ਸਾਰਾ_file += p;
                        ਜੇ (ਖੰਡ_Cmt) {
                            ਅਚਲ std::string::ਮਾਪ_type pos = ਸਾਰਾ_file.ਲੱਭੋ("*/");
                            ਜੇ (pos == std::string::npos) {
                                fl->v3fatal("/* verilator fargs */ must be on single line");
                            }
                            ਸਾਰਾ_file.resize(pos);
                        }
                        ਸਾਰਾ_file += ' ';
                    }
                }
            }
        }
    };

    ਤਾਰਕਿਕ ਵਿੱਚ_Cmt = ਝੂਠ;
    ਜਦੋਂ (!ifp->eof()) {
        ਅਚਲ std::string ਸਤਰ = V3Os::getline(*ifp);
        // Strip simple comments
        ਅੱਖਰ lastch = ' ';  // 'std::isspace(lastch)' is true at the start of the line
        ਤਾਰਕਿਕ space_begin = ਸੱਚ;  // At beginning or leading spaces only
        ਲਈ (ਅਚਲ ਅੱਖਰ* p = ਸਤਰ.c_str(); *p; lastch = *p++) {
            ਜੇ (ਵਿੱਚ_Cmt) {
                ਜੇ (p[0] == '*' && p[1] == '/') {
                    ਵਿੱਚ_Cmt = ਝੂਠ;
                    ++p;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (p[0] == '/' && p[1] == '/' && std::ਖਾਲੀ_ਥਾਂ_ਹੈ(lastch)) {  // Allow /file//path
                parse_fargs_cmt(p);
                ਰੋਕੋ;  // Ignore to EOL
            } ਨਹੀਂਤਾਂ ਜੇ (p[0] == '#' && space_begin) {  // Only # at [spaced] begin of line
                ਰੋਕੋ;  // Ignore to EOL
            } ਨਹੀਂਤਾਂ ਜੇ (p[0] == '/' && p[1] == '*') {
                parse_fargs_cmt(p);
                ਵਿੱਚ_Cmt = ਸੱਚ;
                space_begin = ਝੂਠ;
                ++p;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (!std::ਖਾਲੀ_ਥਾਂ_ਹੈ(*p)) space_begin = ਝੂਠ;
                ਜੇ (!has_fargs) ਸਾਰਾ_file += *p;
            }
        }
        ਜੇ (!has_fargs) ਸਾਰਾ_file += " ";
    }
    ਸਾਰਾ_file += "\n";  // So string match below is simplified
    ਜੇ (ਵਿੱਚ_Cmt) fl->v3error("Unterminated /* comment inside -f file.");

    fl = ਨਵਾਂ File_ਸਤਰ{filename};

    // Split into argument list and process
    // Note we try to respect escaped char, double/simple quoted strings
    // Other simulators don't respect a common syntax...

    // Strip off arguments and parse into words
    std::ਸਦਿਸ਼<string> ਦਲੀਲਾਂ;

    // Parse file using a state machine, taking into account quoted strings and escaped chars
    ਗਿਣਤੀ state : uint8_t {
        ST_ਵਿੱਚ_OPTION,
        ST_ESCAPED_CHAR,
        ST_ਵਿੱਚ_QUOTED_STR,
        ST_ਵਿੱਚ_DOUBLE_QUOTED_STR
    };

    state st = ST_ਵਿੱਚ_OPTION;
    state ਆਖਰੀ_st = ST_ਵਿੱਚ_OPTION;
    string arg;
    ਲਈ (string::ਮਾਪ_type pos = 0; pos < ਸਾਰਾ_file.ਲੰਮਾਈ(); ++pos) {
        ਅੱਖਰ curr_char = ਸਾਰਾ_file[pos];
        ਚੋਣ (st) {
        ਹਾਲਤ ST_ਵਿੱਚ_OPTION:  // Get all chars up to a white space or a "="
            ਜੇ (std::ਖਾਲੀ_ਥਾਂ_ਹੈ(curr_char)) {  // End of option
                ਜੇ (!arg.ਖਾਲੀ_ਥਾਵਾਂ()) {  // End of word
                    ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(arg);
                }
                arg = "";
                ਰੋਕੋ;
            }
            ਜੇ (curr_char == '\\') {  // Escape char, we wait for next char
                ਆਖਰੀ_st = st;  // Memorize current state
                st = ST_ESCAPED_CHAR;
                ਰੋਕੋ;
            }
            ਜੇ (curr_char == '\'') {  // Find begin of quoted string
                // Examine next char in order to decide between
                // a string or a base specifier for integer literal
                ++pos;
                ਜੇ (pos < ਸਾਰਾ_file.ਲੰਮਾਈ()) curr_char = ਸਾਰਾ_file[pos];
                ਜੇ (curr_char == '"') {  // String
                    st = ST_ਵਿੱਚ_QUOTED_STR;
                } ਨਹੀਂਤਾਂ {  // Base specifier
                    arg += '\'';
                }
                arg += curr_char;
                ਰੋਕੋ;
            }
            ਜੇ (curr_char == '"') {  // Find begin of double quoted string
                // Doesn't insert the quote
                st = ST_ਵਿੱਚ_DOUBLE_QUOTED_STR;
                ਰੋਕੋ;
            }
            arg += curr_char;
            ਰੋਕੋ;
        ਹਾਲਤ ST_ਵਿੱਚ_QUOTED_STR:  // Just store all chars inside string
            ਜੇ (curr_char != '\'') {
                arg += curr_char;
            } ਨਹੀਂਤਾਂ {  // End of quoted string
                st = ST_ਵਿੱਚ_OPTION;
            }
            ਰੋਕੋ;
        ਹਾਲਤ ST_ਵਿੱਚ_DOUBLE_QUOTED_STR:  // Take into account escaped chars
            ਜੇ (curr_char != '"') {
                ਜੇ (curr_char == '\\') {
                    ਆਖਰੀ_st = st;
                    st = ST_ESCAPED_CHAR;
                } ਨਹੀਂਤਾਂ {
                    arg += curr_char;
                }
            } ਨਹੀਂਤਾਂ {  // End of double quoted string
                st = ST_ਵਿੱਚ_OPTION;
            }
            ਰੋਕੋ;
        ਹਾਲਤ ST_ESCAPED_CHAR:  // Just add the escaped char
            arg += curr_char;
            st = ਆਖਰੀ_st;
            ਰੋਕੋ;
        }
    }
    ਜੇ (!arg.ਖਾਲੀ_ਥਾਵਾਂ()) {  // Add last word
        ਦਲੀਲਾਂ.ਧੱਕੋ_ਪਿੱਠ(arg);
    }

    //ਰਸਤਾ
    ਅਚਲ string optdir = (rel ? V3Os::filenameDir(filename) : ".");

    // Convert to argv style arg list and parse them
    std::ਸਦਿਸ਼<ਅੱਖਰ*> argv;
    argv.reserve(ਦਲੀਲਾਂ.ਮਾਪ() + 1);
    ਲਈ (ਅਚਲ string& i : ਦਲੀਲਾਂ) argv.ਧੱਕੋ_ਪਿੱਠ(const_cast<ਅੱਖਰ*>(i.c_str()));
    argv.ਧੱਕੋ_ਪਿੱਠ(ਖਾਲੀ_ਸੂਚਕ);  // argv is nullptr-terminated
    parseOptsList(fl, optdir, static_cast<ਪੂਰਨ>(argv.ਮਾਪ() - 1), argv.ਅੰਕੜੇ());
}

//======================================================================

string V3Options::parseFileArg(ਅਚਲ string& optdir, ਅਚਲ string& relfilename) {
    string filename = V3Os::filenameSubstitute(relfilename);
    ਜੇ (optdir != "." && V3Os::filenameIsRel(filename))
        filename = V3Os::filenameJoin(optdir, filename);
    ਵਾਪਸ filename;
}

//======================================================================

ਖਾਲੀ V3Options::show_ਸੰਸਕਰਣ(ਤਾਰਕਿਕ verbose) {
    ਬਾਹਰ << ਸੰਸਕਰਣ();
    ਬਾਹਰ << "\n";
    ਜੇ (!verbose) ਵਾਪਸ;

    ਬਾਹਰ << "\n";
    ਬਾਹਰ << "Copyright 2003-2026 by Wilson Snyder.  Verilator is free software; you can\n";
    ਬਾਹਰ << "redistribute it and/or modify the Verilator internals under the terms of\n";
    ਬਾਹਰ << "either the GNU Lesser General Public License Version 3 or the Perl Artistic\n";
    ਬਾਹਰ << "License Version 2.0.\n";

    ਬਾਹਰ << "\n";
    ਬਾਹਰ << "See https://verilator.org for documentation\n";

    ਬਾਹਰ << "\n";
    ਬਾਹਰ << "Summary of configuration:\n";
    ਬਾਹਰ << "  Compiled in defaults if not in environment:\n";
    ਬਾਹਰ << "    SYSTEMC            = " << DEFENV_SYSTEMC << "\n";
    ਬਾਹਰ << "    SYSTEMC_ARCH       = " << DEFENV_SYSTEMC_ARCH << "\n";
    ਬਾਹਰ << "    SYSTEMC_INCLUDE    = " << DEFENV_SYSTEMC_INCLUDE << "\n";
    ਬਾਹਰ << "    SYSTEMC_LIBDIR     = " << DEFENV_SYSTEMC_LIBDIR << "\n";
    ਬਾਹਰ << "    VERILATOR_ROOT     = " << DEFENV_VERILATOR_ਜੜ੍ਹ << "\n";
    ਬਾਹਰ << "    VERILATOR_SOLVER   = " << DEFENV_VERILATOR_SOLVER << "\n";
    ਬਾਹਰ << "    SystemC system-wide = " << cvt_ਮੰਜ਼ਿਲ_Str(systemCSystemWide()) << "\n";

    // If update below, also update V3Options::getenvBuiltins()
    ਬਾਹਰ << "\n";
    ਬਾਹਰ << "Environment:\n";
    ਬਾਹਰ << "    MAKE               = " << V3Os::getenvStr("MAKE", "") << "\n";
    ਬਾਹਰ << "    PERL               = " << V3Os::getenvStr("PERL", "") << "\n";
    ਬਾਹਰ << "    PYTHON3            = " << V3Os::getenvStr("PYTHON3", "") << "\n";
    ਬਾਹਰ << "    SYSTEMC            = " << V3Os::getenvStr("SYSTEMC", "") << "\n";
    ਬਾਹਰ << "    SYSTEMC_ARCH       = " << V3Os::getenvStr("SYSTEMC_ARCH", "") << "\n";
    ਬਾਹਰ << "    SYSTEMC_INCLUDE    = " << V3Os::getenvStr("SYSTEMC_INCLUDE", "") << "\n";
    ਬਾਹਰ << "    SYSTEMC_LIBDIR     = " << V3Os::getenvStr("SYSTEMC_LIBDIR", "") << "\n";
    // wrapper uses VERILATOR_BIN
    ਬਾਹਰ << "    VERILATOR_BIN      = " << V3Os::getenvStr("VERILATOR_BIN", "") << "\n";
    ਬਾਹਰ << "    VERILATOR_ROOT     = " << V3Os::getenvStr("VERILATOR_ROOT", "") << "\n";
    ਬਾਹਰ << "    VERILATOR_SOLVER   = " << V3Os::getenvStr("VERILATOR_SOLVER", "") << "\n";

    // If update below, also update V3Options::getSupported()
    ਬਾਹਰ << "\n";
    ਬਾਹਰ << "Supported features (compiled-in or forced by environment):\n";
    ਬਾਹਰ << "    COROUTINES         = " << getSupported("COROUTINES") << "\n";
    ਬਾਹਰ << "    SYSTEMC            = " << getSupported("SYSTEMC") << "\n";
    ਬਾਹਰ << "    TSAN               = " << getSupported("TSAN") << "\n";
}

//======================================================================

V3Options::V3Options() {
    m_impp = ਨਵਾਂ V3OptionsImp;

    m_coverageFsmExpand = VFsmExpandType::AUTO;  // Auto is a default global default
    m_makeDir = "obj_dir";
    m_ਅਣਵਰਤਿਆ_Regexp = "*unused*";
    m_x_ਸੌਂਪੋ = "fast";
    m_xInitial = "unique";

    m_defaultLanguage = V3_Lang_ਕੋਡ::mostRecent();

    VName::max_ਲੰਮਾਈ(128);  // Linux filename limits 256; leave half for prefix

    optimize(1);
    // Default +libext+
    ਜੋੜੋ_Lib_Ext_V("");  // So include "filename.v" will find the same file
    ਜੋੜੋ_Lib_Ext_V(".v");
    ਜੋੜੋ_Lib_Ext_V(".sv");
    // Default -I
    ਜੋੜੋ_Inc_Dir_Fallback(".");  // Looks better than {long_cwd_path}/...
}

V3Options::~V3Options() { VL_DO_CLEAR(ਮਿਟਾਓ m_impp, m_impp = ਖਾਲੀ_ਸੂਚਕ); }

ਖਾਲੀ V3Options::ਸੈੱਟ_Debug_ਬਹੁਲਕ(ਪੂਰਨ level) {
    V3Error::debugDefault(level);
    ਜੇ (!m_dumpLevel.ਗਿਣਤੀ_2("tree")) m_dumpLevel["tree"] = 3;  // Don't override if already set.
    m_stats = ਸੱਚ;
    m_debugCheck = ਸੱਚ;
    ਜੇ (level) {
        ਬਾਹਰ << "- Starting " << ਸੰਸਕਰਣ() << "\n";
        UINFO(1, "Current working directory (CWD) is " << V3Os::cwd());
    }
}

ਬਿਨਾਨਿਸ਼ਾਨੀ V3Options::debugLevel(ਅਚਲ string& ਟੈਗ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_debugLevel.ਲੱਭੋ(ਟੈਗ);
    ਵਾਪਸ iter != m_debugLevel.ਸਿਰਾ() ? iter->ਦੂਜਾ : V3Error::debugDefault();
}

ਬਿਨਾਨਿਸ਼ਾਨੀ V3Options::debugSrcLevel(ਅਚਲ string& srcfile_ਰਸਤਾ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ਵਾਪਸ debugLevel(V3Os::filenameNonDirExt(srcfile_ਰਸਤਾ));
}

ਬਿਨਾਨਿਸ਼ਾਨੀ V3Options::dumpLevel(ਅਚਲ string& ਟੈਗ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਅਚਲ ਸਵੈਚਾਲਿਤ iter = m_dumpLevel.ਲੱਭੋ(ਟੈਗ);
    ਵਾਪਸ iter != m_dumpLevel.ਸਿਰਾ() ? iter->ਦੂਜਾ : 0;
}

ਬਿਨਾਨਿਸ਼ਾਨੀ V3Options::dumpSrcLevel(ਅਚਲ string& srcfile_ਰਸਤਾ) ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    // For simplicity, calling functions can just use __FILE__ for srcfile.
    // That means we need to strip the filenames: ../Foo.cpp -> Foo
    ਵਾਪਸ dumpLevel(V3Os::filenameNonDirExt(srcfile_ਰਸਤਾ));
}

ਤਾਰਕਿਕ V3Options::dump_ਰੁੱਖ_Addrids() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {
    ਸਥਿਰ ਪੂਰਨ s_level = -1;
    ਜੇ (VL_UNLIKELY(s_level < 0)) {
        ਅਚਲ ਬਿਨਾਨਿਸ਼ਾਨੀ ਮੁੱਲ = dumpLevel("tree-addrids");
        ਜੇ (!ਉਪਲਬਧ()) ਵਾਪਸ ਮੁੱਲ > 0;
        s_level = static_cast<ਬਿਨਾਨਿਸ਼ਾਨੀ>(ਮੁੱਲ);
    }
    ਵਾਪਸ s_level > 0;
}

ਖਾਲੀ V3Options::optimize(ਪੂਰਨ level) {
    // Set all optimizations to on/off
    ਅਚਲ ਤਾਰਕਿਕ flag = level > 0;
    m_fAcycSimp = flag;
    m_fAssemble = flag;
    m_f_ਦੋਆਧਾਰੀ_ਅੰਕ_ਜਾਂਚ_Loops = flag;
    m_fBitblast = flag;
    m_fCaseDecoder = flag;
    m_f_Case_ਸਾਰਣੀ = flag;
    m_f_Case_ਰੁੱਖ = flag;
    m_fCombine = flag;
    m_fConst = flag;
    m_f_Const_ਦੋਆਧਾਰੀ_ਅੰਕ_Op_ਰੁੱਖ = flag;
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
    m_f_ਮਿਲਾਓ_Cond = flag;
    m_fReloop = flag;
    m_fReorder = flag;
    m_f_ਵੰਡੋ = flag;
    m_fSubst = flag;
    m_fSubstConst = flag;
    m_f_ਸਾਰਣੀ = flag;
    // And set specific optimization levels
    ਜੇ (level >= 3) {
        m_inlineMult = -1;  // Maximum inlining
    }
}
