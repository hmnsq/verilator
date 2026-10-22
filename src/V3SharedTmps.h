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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3SHAREDTMPS_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3SHAREDTMPS_H_

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
    ರಚನೆ ಕೀಲಿ_2 final {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    ರಚನೆ ಕೀಲಿ_Hash final {
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ std::string& name,
                          ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) ಸ್ಥಿರಾಂಕ {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            ಹಿಂತಿರುಗಿ hash.ಮೌಲ್ಯ();
        }
        ಗಾತ್ರ_t ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಕೀಲಿ_2& ಕೀಲಿ) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ (*this)(ಕೀಲಿ.m_modp, ಕೀಲಿ.m_name, ಕೀಲಿ.m_dtypep);
        }
    };
    ರಚನೆ ಕೀಲಿ_Equal final {
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಕೀಲಿ_2& ಕೀಲಿ, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, ಸ್ಥಿರಾಂಕ std::string& name,
                        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ ಕೀಲಿ.m_modp == modp && ಕೀಲಿ.m_dtypep == dtypep && ಕೀಲಿ.m_name == name;
        }
        bool ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಕೀಲಿ_2& a, ಸ್ಥಿರಾಂಕ ಕೀಲಿ_2& b) ಸ್ಥಿರಾಂಕ {
            ಹಿಂತಿರುಗಿ (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    ರಚನೆ Tmps final {
        std::ಸದಿಶ<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ಸ್ಥಿರಾಂಕ AstScope*, ಗಾತ್ರ_t> m_n_ಬಳಸಿದ;
    };

    // STATE
    ಸ್ಥಿರಾಂಕ std::string m_ಪೂರ್ವಪ್ರತ್ಯಯ;  // Prefix of all variable names
    ಸ್ಥಿರಾಂಕ VVarType m_varType;  // Type of all variables
    V3HashMap<ಕೀಲಿ_2, Tmps, ಕೀಲಿ_Hash, ಕೀಲಿ_Equal> m_tmps;  // The temporaries created so far
    ಗಾತ್ರ_t m_nVars = 0;  // Number of variables created, used to make unique names
    ಗಾತ್ರ_t m_nReused = 0;  // Number of temporaries using an existing variable

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    V3SharedTmps(ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ, VVarType varType)
        : m_ಪೂರ್ವಪ್ರತ್ಯಯ{ಪೂರ್ವಪ್ರತ್ಯಯ}
        , m_varType{varType} {
        UASSERT(VString::startsWith(ಪೂರ್ವಪ್ರತ್ಯಯ, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::ತುದಿಗಳು_With(ಪೂರ್ವಪ್ರತ್ಯಯ, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        ಸ್ಥಿರ std::ಗಣ<std::string> s_prefixes;
        ಸ್ಥಿರಾಂಕ bool isNew = s_prefixes.ಸೇರಿಸು(m_ಪೂರ್ವಪ್ರತ್ಯಯ).ಎರಡನೇ;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << ಪೂರ್ವಪ್ರತ್ಯಯ << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_ಸಾಲು* flp, AstScope* scopep, Ast_ಗಂಟು_DType* dtypep,
                      ಸ್ಥಿರಾಂಕ std::string& name = "") {
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = scopep->modp();
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ pair = m_tmps.ಸೇರಿಸು_Lazy(modp, name, dtypep, [&]() {  //
            ಹಿಂತಿರುಗಿ std::make_pair(ಕೀಲಿ_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.ಮೊದಲ.ಮೌಲ್ಯ();
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t n = tmps.m_n_ಬಳಸಿದ[scopep]++;
        ಆದರೆ (n < tmps.m_varps.ಗಾತ್ರ_2()) {
            ++m_nReused;
        } ಇಲ್ಲದಿದ್ದರೆ {
            std::string uniqueName = m_ಪೂರ್ವಪ್ರತ್ಯಯ + "_";
            ಆದರೆ (!name.ಖಾಲಿಗಳು()) uniqueName += name + "_";
            uniqueName += std::ಗಮ್ಯ_string(m_nVars++);
            AstVar* ಸ್ಥಿರಾಂಕ varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->ಕೂಡಿಸು_Stmtsp(varp);
            tmps.m_varps.emplace_ಹಿಂಭಾಗ(varp);
        }
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->ಕೂಡಿಸು_Varsp(vscp);
        ಹಿಂತಿರುಗಿ vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_ಸಾಲು* flp, AstScope* scopep, ಚಿಹ್ನೆರಹಿತ ಅಗಲ,
                      ಸ್ಥಿರಾಂಕ std::string& name = "") {
        ಹಿಂತಿರುಗಿ make(flp, scopep, scopep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(ಅಗಲ, ಅಗಲ, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    ಗಾತ್ರ_t nReused() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_nReused; }
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
