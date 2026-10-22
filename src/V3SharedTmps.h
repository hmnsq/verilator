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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Hash.h"
#अन्तर्भाव "V3HashTable.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <set>
#अन्तर्भाव <string>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    संरचना कुंजी_2 final {
        ध्रुव Ast_गाँठ_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ध्रुव Ast_गाँठ_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    संरचना कुंजी_Hash final {
        माप_प्रकार चालक()(ध्रुव Ast_गाँठ_Module* modp, ध्रुव std::string& name,
                          ध्रुव Ast_गाँठ_DType* dtypep) ध्रुव {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            प्रत्यागम hash.मूल्य();
        }
        माप_प्रकार चालक()(ध्रुव कुंजी_2& कुंजी) ध्रुव {
            प्रत्यागम (*this)(कुंजी.m_modp, कुंजी.m_name, कुंजी.m_dtypep);
        }
    };
    संरचना कुंजी_Equal final {
        bool चालक()(ध्रुव कुंजी_2& कुंजी, ध्रुव Ast_गाँठ_Module* modp, ध्रुव std::string& name,
                        ध्रुव Ast_गाँठ_DType* dtypep) ध्रुव {
            प्रत्यागम कुंजी.m_modp == modp && कुंजी.m_dtypep == dtypep && कुंजी.m_name == name;
        }
        bool चालक()(ध्रुव कुंजी_2& a, ध्रुव कुंजी_2& b) ध्रुव {
            प्रत्यागम (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    संरचना Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ध्रुव AstScope*, माप_प्रकार> m_nUsed;
    };

    // STATE
    ध्रुव std::string m_prefix;  // Prefix of all variable names
    ध्रुव VVarType m_varType;  // Type of all variables
    V3HashMap<कुंजी_2, Tmps, कुंजी_Hash, कुंजी_Equal> m_tmps;  // The temporaries created so far
    माप_प्रकार m_nVars = 0;  // Number of variables created, used to make unique names
    माप_प्रकार m_nReused = 0;  // Number of temporaries using an existing variable

सार्वजनिक:
    // CONSTRUCTORS
    V3SharedTmps(ध्रुव std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        स्थिर std::set<std::string> s_prefixes;
        ध्रुव bool isNew = s_prefixes.जोड़ो(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_पंक्ति* flp, AstScope* scopep, Ast_गाँठ_DType* dtypep,
                      ध्रुव std::string& name = "") {
        ध्रुव Ast_गाँठ_Module* ध्रुव modp = scopep->modp();
        ध्रुव स्वतः pair = m_tmps.जोड़ो_Lazy(modp, name, dtypep, [&]() {  //
            प्रत्यागम std::make_pair(कुंजी_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.मूल्य();
        ध्रुव माप_प्रकार n = tmps.m_nUsed[scopep]++;
        यदि (n < tmps.m_varps.size()) {
            ++m_nReused;
        } अन्यथा {
            std::string uniqueName = m_prefix + "_";
            यदि (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* ध्रुव varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* ध्रुव vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        प्रत्यागम vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_पंक्ति* flp, AstScope* scopep, अचिन्हित width,
                      ध्रुव std::string& name = "") {
        प्रत्यागम make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    माप_प्रकार nReused() ध्रुव { प्रत्यागम m_nReused; }
};

#endif  // Guard
