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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Hash.h"
#অন্তর্ভাব "V3HashTable.h"
#অন্তর্ভাব "V3String.h"

#অন্তর্ভাব <set>
#অন্তর্ভাব <string>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <utility>
#অন্তর্ভাব <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    কাঠামো চাবি_2 final {
        ধ্রুবক Ast_গ্রন্থি_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ধ্রুবক Ast_গ্রন্থি_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    কাঠামো চাবি_Hash final {
        আকার_t operator()(ধ্রুবক Ast_গ্রন্থি_Module* modp, ধ্রুবক std::string& name,
                          ধ্রুবক Ast_গ্রন্থি_DType* dtypep) ধ্রুবক {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            ফেরত hash.মান();
        }
        আকার_t operator()(ধ্রুবক চাবি_2& চাবি) ধ্রুবক {
            ফেরত (*this)(চাবি.m_modp, চাবি.m_name, চাবি.m_dtypep);
        }
    };
    কাঠামো চাবি_Equal final {
        bool operator()(ধ্রুবক চাবি_2& চাবি, ধ্রুবক Ast_গ্রন্থি_Module* modp, ধ্রুবক std::string& name,
                        ধ্রুবক Ast_গ্রন্থি_DType* dtypep) ধ্রুবক {
            ফেরত চাবি.m_modp == modp && চাবি.m_dtypep == dtypep && চাবি.m_name == name;
        }
        bool operator()(ধ্রুবক চাবি_2& a, ধ্রুবক চাবি_2& b) ধ্রুবক {
            ফেরত (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    কাঠামো Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ধ্রুবক AstScope*, আকার_t> m_nUsed;
    };

    // STATE
    ধ্রুবক std::string m_prefix;  // Prefix of all variable names
    ধ্রুবক VVarType m_varType;  // Type of all variables
    V3HashMap<চাবি_2, Tmps, চাবি_Hash, চাবি_Equal> m_tmps;  // The temporaries created so far
    আকার_t m_nVars = 0;  // Number of variables created, used to make unique names
    আকার_t m_nReused = 0;  // Number of temporaries using an existing variable

সর্বজনীন:
    // CONSTRUCTORS
    V3SharedTmps(ধ্রুবক std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        স্থির std::set<std::string> s_prefixes;
        ধ্রুবক bool isNew = s_prefixes.ঢোকাও(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_সারি* flp, AstScope* scopep, Ast_গ্রন্থি_DType* dtypep,
                      ধ্রুবক std::string& name = "") {
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = scopep->modp();
        ধ্রুবক auto pair = m_tmps.ঢোকাও_Lazy(modp, name, dtypep, [&]() {  //
            ফেরত std::make_pair(চাবি_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.মান();
        ধ্রুবক আকার_t n = tmps.m_nUsed[scopep]++;
        যদি (n < tmps.m_varps.আকার_2()) {
            ++m_nReused;
        } অন্যথা {
            std::string uniqueName = m_prefix + "_";
            যদি (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* ধ্রুবক varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* ধ্রুবক vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        ফেরত vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_সারি* flp, AstScope* scopep, unsigned width,
                      ধ্রুবক std::string& name = "") {
        ফেরত make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    আকার_t nReused() ধ্রুবক { ফেরত m_nReused; }
};

#endif  // Guard
