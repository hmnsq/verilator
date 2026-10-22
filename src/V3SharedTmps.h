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

#ifndef VERILATOR_V3SHAREDTMPS_H_
#define VERILATOR_V3SHAREDTMPS_H_

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Hash.h"
#અન્તર્ભાવ "V3HashTable.h"
#અન્તર્ભાવ "V3String.h"

#અન્તર્ભાવ <set>
#અન્તર્ભાવ <string>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <utility>
#અન્તર્ભાવ <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    સંરચના ચાવી_2 final {
        અચલ Ast_ગાંઠ_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        અચલ Ast_ગાંઠ_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    સંરચના ચાવી_Hash final {
        size_t operator()(અચલ Ast_ગાંઠ_Module* modp, અચલ std::string& name,
                          અચલ Ast_ગાંઠ_DType* dtypep) અચલ {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            પાછા hash.કિંમત();
        }
        size_t operator()(અચલ ચાવી_2& ચાવી) અચલ {
            પાછા (*this)(ચાવી.m_modp, ચાવી.m_name, ચાવી.m_dtypep);
        }
    };
    સંરચના ચાવી_Equal final {
        bool operator()(અચલ ચાવી_2& ચાવી, અચલ Ast_ગાંઠ_Module* modp, અચલ std::string& name,
                        અચલ Ast_ગાંઠ_DType* dtypep) અચલ {
            પાછા ચાવી.m_modp == modp && ચાવી.m_dtypep == dtypep && ચાવી.m_name == name;
        }
        bool operator()(અચલ ચાવી_2& a, અચલ ચાવી_2& b) અચલ {
            પાછા (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    સંરચના Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<અચલ AstScope*, size_t> m_nUsed;
    };

    // STATE
    અચલ std::string m_prefix;  // Prefix of all variable names
    અચલ VVarType m_varType;  // Type of all variables
    V3HashMap<ચાવી_2, Tmps, ચાવી_Hash, ચાવી_Equal> m_tmps;  // The temporaries created so far
    size_t m_nVars = 0;  // Number of variables created, used to make unique names
    size_t m_nReused = 0;  // Number of temporaries using an existing variable

જાહેર:
    // CONSTRUCTORS
    V3SharedTmps(અચલ std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        સ્થિર std::set<std::string> s_prefixes;
        અચલ bool isNew = s_prefixes.ઉમેરો(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_લીટી* flp, AstScope* scopep, Ast_ગાંઠ_DType* dtypep,
                      અચલ std::string& name = "") {
        અચલ Ast_ગાંઠ_Module* અચલ modp = scopep->modp();
        અચલ auto pair = m_tmps.ઉમેરો_Lazy(modp, name, dtypep, [&]() {  //
            પાછા std::make_pair(ચાવી_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.કિંમત();
        અચલ size_t n = tmps.m_nUsed[scopep]++;
        જો (n < tmps.m_varps.size()) {
            ++m_nReused;
        } નહીંતર {
            std::string uniqueName = m_prefix + "_";
            જો (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* અચલ varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* અચલ vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        પાછા vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_લીટી* flp, AstScope* scopep, unsigned width,
                      અચલ std::string& name = "") {
        પાછા make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    size_t nReused() અચલ { પાછા m_nReused; }
};

#endif  // Guard
