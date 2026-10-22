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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Hash.h"
#ضمن "V3HashTable.h"
#ضمن "V3String.h"

#ضمن <set>
#ضمن <string>
#ضمن <unordered_map>
#ضمن <utility>
#ضمن <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    بنية مبودي_2 final {
        ثابت Ast_كلي_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ثابت Ast_كلي_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    بنية مبودي_Hash final {
        size_t operator()(ثابت Ast_كلي_Module* modp, ثابت std::string& name,
                          ثابت Ast_كلي_DType* dtypep) ثابت {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            أرجع hash.درج();
        }
        size_t operator()(ثابت مبودي_2& مبودي) ثابت {
            أرجع (*this)(مبودي.m_modp, مبودي.m_name, مبودي.m_dtypep);
        }
    };
    بنية مبودي_Equal final {
        bool operator()(ثابت مبودي_2& مبودي, ثابت Ast_كلي_Module* modp, ثابت std::string& name,
                        ثابت Ast_كلي_DType* dtypep) ثابت {
            أرجع مبودي.m_modp == modp && مبودي.m_dtypep == dtypep && مبودي.m_name == name;
        }
        bool operator()(ثابت مبودي_2& a, ثابت مبودي_2& b) ثابت {
            أرجع (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    بنية Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ثابت AstScope*, size_t> m_nUsed;
    };

    // STATE
    ثابت std::string m_prefix;  // Prefix of all variable names
    ثابت VVarType m_varType;  // Type of all variables
    V3HashMap<مبودي_2, Tmps, مبودي_Hash, مبودي_Equal> m_tmps;  // The temporaries created so far
    size_t m_nVars = 0;  // Number of variables created, used to make unique names
    size_t m_nReused = 0;  // Number of temporaries using an existing variable

عام:
    // CONSTRUCTORS
    V3SharedTmps(ثابت std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        ساكن std::set<std::string> s_prefixes;
        ثابت bool isNew = s_prefixes.سك(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_لي* flp, AstScope* scopep, Ast_كلي_DType* dtypep,
                      ثابت std::string& name = "") {
        ثابت Ast_كلي_Module* ثابت modp = scopep->modp();
        ثابت auto pair = m_tmps.سك_Lazy(modp, name, dtypep, [&]() {  //
            أرجع std::make_pair(مبودي_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.درج();
        ثابت size_t n = tmps.m_nUsed[scopep]++;
        إذا (n < tmps.m_varps.size()) {
            ++m_nReused;
        } وإلا {
            std::string uniqueName = m_prefix + "_";
            إذا (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* ثابت varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* ثابت vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        أرجع vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_لي* flp, AstScope* scopep, unsigned width,
                      ثابت std::string& name = "") {
        أرجع make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    size_t nReused() ثابت { أرجع m_nReused; }
};

#endif  // Guard
