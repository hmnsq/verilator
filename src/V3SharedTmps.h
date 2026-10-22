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

#अगर_अघोषित VERILATOR_V3SHAREDTMPS_H_
#घोषणा VERILATOR_V3SHAREDTMPS_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Hash.h"
#समावेश "V3HashTable.h"
#समावेश "V3String.h"

#समावेश <set>
#समावेश <string>
#समावेश <unordered_map>
#समावेश <utility>
#समावेश <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    काष्ठा कुंजी_2 final {
        स्थिर Ast_गाँठ_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        स्थिर Ast_गाँठ_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    काष्ठा कुंजी_Hash final {
        माप_प्रकार चालक()(स्थिर Ast_गाँठ_Module* modp, स्थिर std::string& name,
                          स्थिर Ast_गाँठ_DType* dtypep) स्थिर {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            वापस hash.मूल्य();
        }
        माप_प्रकार चालक()(स्थिर कुंजी_2& कुंजी) स्थिर {
            वापस (*this)(कुंजी.m_modp, कुंजी.m_name, कुंजी.m_dtypep);
        }
    };
    काष्ठा कुंजी_Equal final {
        bool चालक()(स्थिर कुंजी_2& कुंजी, स्थिर Ast_गाँठ_Module* modp, स्थिर std::string& name,
                        स्थिर Ast_गाँठ_DType* dtypep) स्थिर {
            वापस कुंजी.m_modp == modp && कुंजी.m_dtypep == dtypep && कुंजी.m_name == name;
        }
        bool चालक()(स्थिर कुंजी_2& a, स्थिर कुंजी_2& b) स्थिर {
            वापस (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    काष्ठा Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<स्थिर AstScope*, माप_प्रकार> m_nUsed;
    };

    // STATE
    स्थिर std::string m_prefix;  // Prefix of all variable names
    स्थिर VVarType m_varType;  // Type of all variables
    V3HashMap<कुंजी_2, Tmps, कुंजी_Hash, कुंजी_Equal> m_tmps;  // The temporaries created so far
    माप_प्रकार m_nVars = 0;  // Number of variables created, used to make unique names
    माप_प्रकार m_nReused = 0;  // Number of temporaries using an existing variable

खुला:
    // CONSTRUCTORS
    V3SharedTmps(स्थिर std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        अटल std::set<std::string> s_prefixes;
        स्थिर bool isNew = s_prefixes.जोड़ो(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_पंक्ति* flp, AstScope* scopep, Ast_गाँठ_DType* dtypep,
                      स्थिर std::string& name = "") {
        स्थिर Ast_गाँठ_Module* स्थिर modp = scopep->modp();
        स्थिर स्वतः pair = m_tmps.जोड़ो_Lazy(modp, name, dtypep, [&]() {  //
            वापस std::make_pair(कुंजी_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.मूल्य();
        स्थिर माप_प्रकार n = tmps.m_nUsed[scopep]++;
        अगर (n < tmps.m_varps.आकार()) {
            ++m_nReused;
        } अन्यथा {
            std::string uniqueName = m_prefix + "_";
            अगर (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* स्थिर varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* स्थिर vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        वापस vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_पंक्ति* flp, AstScope* scopep, अचिन्हित width,
                      स्थिर std::string& name = "") {
        वापस make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    माप_प्रकार nReused() स्थिर { वापस m_nReused; }
};

#पूर्ण_अगर  // Guard
