// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Temporary variables shared between instances
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
// Creates temporary variables in scopes, such that the scopes of the same
// module (instances) use the same AstVar declarations, each with their own
// AstVarScope. Without this, identical instances would reference different
// variables, which prevents V3Combine from sharing their logic.
//
// Variables are named '<prefix>_<name>_<n>', with the prefix given to the
// constructor, which must start with '__V', and must be unique among all
// instances created during the run, so names never clash.
//
// Variables are requested with a name and a data type. The n-th request with
// the same name and type from each scope of a module yields the same AstVar,
// so sharing works if all scopes of a module request their temporaries in the
// same order, which holds as they contain the same code. A scope never gets
// the same AstVar twice. As the AstVar is shared, callers must set identical
// AstVar attributes on all variables created with the same name. Data types
// are compared by identity, so callers must pass shared (e.g. type table) data
// types, not ones created anew for each scope, otherwise no variables are
// shared.
//
//*************************************************************************

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3SHAREDTMPS_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3SHAREDTMPS_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hash.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3HashTable.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    ਬਣਤਰ ਕੁੰਜੀ_2 final {
        ਅਚਲ Ast_ਗੰਢ_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ਅਚਲ Ast_ਗੰਢ_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    ਬਣਤਰ ਕੁੰਜੀ_Hash final {
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ Ast_ਗੰਢ_Module* modp, ਅਚਲ std::string& name,
                          ਅਚਲ Ast_ਗੰਢ_DType* dtypep) ਅਚਲ {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            ਵਾਪਸ hash.ਮੁੱਲ();
        }
        ਮਾਪ_t ਸੰਚਾਲਕ()(ਅਚਲ ਕੁੰਜੀ_2& ਕੁੰਜੀ) ਅਚਲ {
            ਵਾਪਸ (*this)(ਕੁੰਜੀ.m_modp, ਕੁੰਜੀ.m_name, ਕੁੰਜੀ.m_dtypep);
        }
    };
    ਬਣਤਰ ਕੁੰਜੀ_Equal final {
        bool ਸੰਚਾਲਕ()(ਅਚਲ ਕੁੰਜੀ_2& ਕੁੰਜੀ, ਅਚਲ Ast_ਗੰਢ_Module* modp, ਅਚਲ std::string& name,
                        ਅਚਲ Ast_ਗੰਢ_DType* dtypep) ਅਚਲ {
            ਵਾਪਸ ਕੁੰਜੀ.m_modp == modp && ਕੁੰਜੀ.m_dtypep == dtypep && ਕੁੰਜੀ.m_name == name;
        }
        bool ਸੰਚਾਲਕ()(ਅਚਲ ਕੁੰਜੀ_2& a, ਅਚਲ ਕੁੰਜੀ_2& b) ਅਚਲ {
            ਵਾਪਸ (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    ਬਣਤਰ Tmps final {
        std::ਸਦਿਸ਼<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ਅਚਲ AstScope*, ਮਾਪ_t> m_n_ਵਰਤਿਆ;
    };

    // STATE
    ਅਚਲ std::string m_ਅਗੇਤਰ;  // Prefix of all variable names
    ਅਚਲ VVarType m_varType;  // Type of all variables
    V3HashMap<ਕੁੰਜੀ_2, Tmps, ਕੁੰਜੀ_Hash, ਕੁੰਜੀ_Equal> m_tmps;  // The temporaries created so far
    ਮਾਪ_t m_nVars = 0;  // Number of variables created, used to make unique names
    ਮਾਪ_t m_nReused = 0;  // Number of temporaries using an existing variable

ਜਨਤਕ:
    // CONSTRUCTORS
    V3SharedTmps(ਅਚਲ std::string& ਅਗੇਤਰ, VVarType varType)
        : m_ਅਗੇਤਰ{ਅਗੇਤਰ}
        , m_varType{varType} {
        UASSERT(VString::startsWith(ਅਗੇਤਰ, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::ਸਿਰੇ_With(ਅਗੇਤਰ, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        ਸਥਿਰ std::ਸੈੱਟ<std::string> s_prefixes;
        ਅਚਲ bool isNew = s_prefixes.ਪਾਓ(m_ਅਗੇਤਰ).ਦੂਜਾ;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << ਅਗੇਤਰ << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_ਸਤਰ* flp, AstScope* scopep, Ast_ਗੰਢ_DType* dtypep,
                      ਅਚਲ std::string& name = "") {
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = scopep->modp();
        ਅਚਲ ਸਵੈਚਾਲਿਤ pair = m_tmps.ਪਾਓ_Lazy(modp, name, dtypep, [&]() {  //
            ਵਾਪਸ std::make_pair(ਕੁੰਜੀ_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.ਪਹਿਲਾ.ਮੁੱਲ();
        ਅਚਲ ਮਾਪ_t n = tmps.m_n_ਵਰਤਿਆ[scopep]++;
        ਜੇ (n < tmps.m_varps.ਮਾਪ()) {
            ++m_nReused;
        } ਨਹੀਂਤਾਂ {
            std::string uniqueName = m_ਅਗੇਤਰ + "_";
            ਜੇ (!name.ਖਾਲੀ_ਥਾਵਾਂ()) uniqueName += name + "_";
            uniqueName += std::ਮੰਜ਼ਿਲ_string(m_nVars++);
            AstVar* ਅਚਲ varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->ਜੋੜੋ_Stmtsp(varp);
            tmps.m_varps.emplace_ਪਿੱਠ(varp);
        }
        AstVarScope* ਅਚਲ vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->ਜੋੜੋ_Varsp(vscp);
        ਵਾਪਸ vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_ਸਤਰ* flp, AstScope* scopep, ਬਿਨਾਨਿਸ਼ਾਨੀ ਚੌੜਾਈ,
                      ਅਚਲ std::string& name = "") {
        ਵਾਪਸ make(flp, scopep, scopep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    ਮਾਪ_t nReused() ਅਚਲ { ਵਾਪਸ m_nReused; }
};

#ਜੇ_ਅੰਤ  // Guard
