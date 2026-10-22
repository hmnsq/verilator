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
    संरचना साँचो_2 final {
        अचर Ast_गाँठो_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        अचर Ast_गाँठो_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    संरचना साँचो_Hash final {
        माप_प्रकार चालक()(अचर Ast_गाँठो_Module* modp, अचर std::string& name,
                          अचर Ast_गाँठो_DType* dtypep) अचर {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            फर्काऊ hash.मान();
        }
        माप_प्रकार चालक()(अचर साँचो_2& साँचो) अचर {
            फर्काऊ (*this)(साँचो.m_modp, साँचो.m_name, साँचो.m_dtypep);
        }
    };
    संरचना साँचो_Equal final {
        bool चालक()(अचर साँचो_2& साँचो, अचर Ast_गाँठो_Module* modp, अचर std::string& name,
                        अचर Ast_गाँठो_DType* dtypep) अचर {
            फर्काऊ साँचो.m_modp == modp && साँचो.m_dtypep == dtypep && साँचो.m_name == name;
        }
        bool चालक()(अचर साँचो_2& a, अचर साँचो_2& b) अचर {
            फर्काऊ (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    संरचना Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<अचर AstScope*, माप_प्रकार> m_nUsed;
    };

    // STATE
    अचर std::string m_prefix;  // Prefix of all variable names
    अचर VVarType m_varType;  // Type of all variables
    V3HashMap<साँचो_2, Tmps, साँचो_Hash, साँचो_Equal> m_tmps;  // The temporaries created so far
    माप_प्रकार m_nVars = 0;  // Number of variables created, used to make unique names
    माप_प्रकार m_nReused = 0;  // Number of temporaries using an existing variable

सार्वजनिक:
    // CONSTRUCTORS
    V3SharedTmps(अचर std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        स्थिर std::set<std::string> s_prefixes;
        अचर bool isNew = s_prefixes.घुसाउ(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_पङ्क्ति* flp, AstScope* scopep, Ast_गाँठो_DType* dtypep,
                      अचर std::string& name = "") {
        अचर Ast_गाँठो_Module* अचर modp = scopep->modp();
        अचर स्वतः pair = m_tmps.घुसाउ_Lazy(modp, name, dtypep, [&]() {  //
            फर्काऊ std::make_pair(साँचो_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.मान();
        अचर माप_प्रकार n = tmps.m_nUsed[scopep]++;
        यदि (n < tmps.m_varps.size()) {
            ++m_nReused;
        } अन्यथा {
            std::string uniqueName = m_prefix + "_";
            यदि (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* अचर varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* अचर vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        फर्काऊ vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_पङ्क्ति* flp, AstScope* scopep, अचिन्हित width,
                      अचर std::string& name = "") {
        फर्काऊ make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    माप_प्रकार nReused() अचर { फर्काऊ m_nReused; }
};

#endif  // Guard
