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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Hash.h"
#ಅನ್ತರ್ಭಾವ "V3HashTable.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    ಸಂರಚನಾ ಕೀಲಿ_2 final {
        ಧ್ರುವ Ast_ಗಂಟು_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ಧ್ರುವ Ast_ಗಂಟು_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    ಸಂರಚನಾ ಕೀಲಿ_Hash final {
        size_t operator()(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, ಧ್ರುವ std::string& name,
                          ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) ಧ್ರುವ {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            ಪ್ರತ್ಯಾಗಮ hash.ಮೌಲ್ಯ();
        }
        size_t operator()(ಧ್ರುವ ಕೀಲಿ_2& ಕೀಲಿ) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ (*this)(ಕೀಲಿ.m_modp, ಕೀಲಿ.m_name, ಕೀಲಿ.m_dtypep);
        }
    };
    ಸಂರಚನಾ ಕೀಲಿ_Equal final {
        bool operator()(ಧ್ರುವ ಕೀಲಿ_2& ಕೀಲಿ, ಧ್ರುವ Ast_ಗಂಟು_Module* modp, ಧ್ರುವ std::string& name,
                        ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ.m_modp == modp && ಕೀಲಿ.m_dtypep == dtypep && ಕೀಲಿ.m_name == name;
        }
        bool operator()(ಧ್ರುವ ಕೀಲಿ_2& a, ಧ್ರುವ ಕೀಲಿ_2& b) ಧ್ರುವ {
            ಪ್ರತ್ಯಾಗಮ (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    ಸಂರಚನಾ Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ಧ್ರುವ AstScope*, size_t> m_nUsed;
    };

    // STATE
    ಧ್ರುವ std::string m_prefix;  // Prefix of all variable names
    ಧ್ರುವ VVarType m_varType;  // Type of all variables
    V3HashMap<ಕೀಲಿ_2, Tmps, ಕೀಲಿ_Hash, ಕೀಲಿ_Equal> m_tmps;  // The temporaries created so far
    size_t m_nVars = 0;  // Number of variables created, used to make unique names
    size_t m_nReused = 0;  // Number of temporaries using an existing variable

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3SharedTmps(ಧ್ರುವ std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        ಸ್ಥಿರ std::set<std::string> s_prefixes;
        ಧ್ರುವ bool isNew = s_prefixes.ಸೇರಿಸು(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_ಸಾಲು* flp, AstScope* scopep, Ast_ಗಂಟು_DType* dtypep,
                      ಧ್ರುವ std::string& name = "") {
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = scopep->modp();
        ಧ್ರುವ auto pair = m_tmps.ಸೇರಿಸು_Lazy(modp, name, dtypep, [&]() {  //
            ಪ್ರತ್ಯಾಗಮ std::make_pair(ಕೀಲಿ_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.ಮೌಲ್ಯ();
        ಧ್ರುವ size_t n = tmps.m_nUsed[scopep]++;
        ಯದಿ (n < tmps.m_varps.size()) {
            ++m_nReused;
        } ಅನ್ಯಥಾ {
            std::string uniqueName = m_prefix + "_";
            ಯದಿ (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* ಧ್ರುವ varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* ಧ್ರುವ vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        ಪ್ರತ್ಯಾಗಮ vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_ಸಾಲು* flp, AstScope* scopep, unsigned width,
                      ಧ್ರುವ std::string& name = "") {
        ಪ್ರತ್ಯಾಗಮ make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    size_t nReused() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_nReused; }
};

#endif  // Guard
