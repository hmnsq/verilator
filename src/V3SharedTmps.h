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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3SHAREDTMPS_H_
#وصف_ڪريو VERILATOR_V3SHAREDTMPS_H_

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
    بنية ڪنجي_2 final {
        ثابت Ast_ڳنڍ_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ثابت Ast_ڳنڍ_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    بنية ڪنجي_Hash final {
        سائيز_t عامل()(ثابت Ast_ڳنڍ_Module* modp, ثابت std::string& name,
                          ثابت Ast_ڳنڍ_DType* dtypep) ثابت {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            أرجع hash.قدر();
        }
        سائيز_t عامل()(ثابت ڪنجي_2& ڪنجي) ثابت {
            أرجع (*this)(ڪنجي.m_modp, ڪنجي.m_name, ڪنجي.m_dtypep);
        }
    };
    بنية ڪنجي_Equal final {
        bool عامل()(ثابت ڪنجي_2& ڪنجي, ثابت Ast_ڳنڍ_Module* modp, ثابت std::string& name,
                        ثابت Ast_ڳنڍ_DType* dtypep) ثابت {
            أرجع ڪنجي.m_modp == modp && ڪنجي.m_dtypep == dtypep && ڪنجي.m_name == name;
        }
        bool عامل()(ثابت ڪنجي_2& a, ثابت ڪنجي_2& b) ثابت {
            أرجع (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    بنية Tmps final {
        std::ويڪٽر<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ثابت AstScope*, سائيز_t> m_n_استعمال_ٿيل;
    };

    // STATE
    ثابت std::string m_اڳياڙي;  // Prefix of all variable names
    ثابت VVarType m_varType;  // Type of all variables
    V3HashMap<ڪنجي_2, Tmps, ڪنجي_Hash, ڪنجي_Equal> m_tmps;  // The temporaries created so far
    سائيز_t m_nVars = 0;  // Number of variables created, used to make unique names
    سائيز_t m_nReused = 0;  // Number of temporaries using an existing variable

عام:
    // CONSTRUCTORS
    V3SharedTmps(ثابت std::string& اڳياڙي, VVarType varType)
        : m_اڳياڙي{اڳياڙي}
        , m_varType{varType} {
        UASSERT(VString::startsWith(اڳياڙي, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::پڇاڙيون_With(اڳياڙي, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        ساكن std::سيٽ<std::string> s_prefixes;
        ثابت bool isNew = s_prefixes.وجهو(m_اڳياڙي).ٻيو;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << اڳياڙي << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_سٽ* flp, AstScope* scopep, Ast_ڳنڍ_DType* dtypep,
                      ثابت std::string& name = "") {
        ثابت Ast_ڳنڍ_Module* ثابت modp = scopep->modp();
        ثابت خودڪار pair = m_tmps.وجهو_Lazy(modp, name, dtypep, [&]() {  //
            أرجع std::make_pair(ڪنجي_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.پهريون.قدر();
        ثابت سائيز_t n = tmps.m_n_استعمال_ٿيل[scopep]++;
        إذا (n < tmps.m_varps.سائيز()) {
            ++m_nReused;
        } وإلا {
            std::string uniqueName = m_اڳياڙي + "_";
            إذا (!name.خالي_جايون()) uniqueName += name + "_";
            uniqueName += std::منزل_string(m_nVars++);
            AstVar* ثابت varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->جوڙيو_Stmtsp(varp);
            tmps.m_varps.emplace_پٺ(varp);
        }
        AstVarScope* ثابت vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->جوڙيو_Varsp(vscp);
        أرجع vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_سٽ* flp, AstScope* scopep, بنانشان ويڪر,
                      ثابت std::string& name = "") {
        أرجع make(flp, scopep, scopep->ڳوليو_ٻٽو_انگ_DType(ويڪر, ويڪر, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    سائيز_t nReused() ثابت { أرجع m_nReused; }
};

#جيڪڏهن_پورو  // Guard
