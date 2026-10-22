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

#جے_بنا_تعریف VERILATOR_V3SHAREDTMPS_H_
#تعریف_کرو VERILATOR_V3SHAREDTMPS_H_

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
    بنية کلید_2 final {
        ثابت Ast_گرہ_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ثابت Ast_گرہ_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    بنية کلید_Hash final {
        ناپ_t عامل()(ثابت Ast_گرہ_Module* modp, ثابت std::string& name,
                          ثابت Ast_گرہ_DType* dtypep) ثابت {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            أرجع hash.قدر();
        }
        ناپ_t عامل()(ثابت کلید_2& کلید) ثابت {
            أرجع (*this)(کلید.m_modp, کلید.m_name, کلید.m_dtypep);
        }
    };
    بنية کلید_Equal final {
        bool عامل()(ثابت کلید_2& کلید, ثابت Ast_گرہ_Module* modp, ثابت std::string& name,
                        ثابت Ast_گرہ_DType* dtypep) ثابت {
            أرجع کلید.m_modp == modp && کلید.m_dtypep == dtypep && کلید.m_name == name;
        }
        bool عامل()(ثابت کلید_2& a, ثابت کلید_2& b) ثابت {
            أرجع (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    بنية Tmps final {
        std::سمتیہ<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ثابت AstScope*, ناپ_t> m_n_ورتیا;
    };

    // STATE
    ثابت std::string m_اگیتر;  // Prefix of all variable names
    ثابت VVarType m_varType;  // Type of all variables
    V3HashMap<کلید_2, Tmps, کلید_Hash, کلید_Equal> m_tmps;  // The temporaries created so far
    ناپ_t m_nVars = 0;  // Number of variables created, used to make unique names
    ناپ_t m_nReused = 0;  // Number of temporaries using an existing variable

عام:
    // CONSTRUCTORS
    V3SharedTmps(ثابت std::string& اگیتر, VVarType varType)
        : m_اگیتر{اگیتر}
        , m_varType{varType} {
        UASSERT(VString::startsWith(اگیتر, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::سرے_With(اگیتر, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        ساكن std::سیٹ<std::string> s_prefixes;
        ثابت bool isNew = s_prefixes.داخل_کرو(m_اگیتر).دوجا;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << اگیتر << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_سطر* flp, AstScope* scopep, Ast_گرہ_DType* dtypep,
                      ثابت std::string& name = "") {
        ثابت Ast_گرہ_Module* ثابت modp = scopep->modp();
        ثابت خودکار pair = m_tmps.داخل_کرو_Lazy(modp, name, dtypep, [&]() {  //
            أرجع std::make_pair(کلید_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.پہلا.قدر();
        ثابت ناپ_t n = tmps.m_n_ورتیا[scopep]++;
        إذا (n < tmps.m_varps.ناپ()) {
            ++m_nReused;
        } وإلا {
            std::string uniqueName = m_اگیتر + "_";
            إذا (!name.خالی_تھاواں()) uniqueName += name + "_";
            uniqueName += std::منزل_string(m_nVars++);
            AstVar* ثابت varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->جوڑو_Stmtsp(varp);
            tmps.m_varps.emplace_پٹھ(varp);
        }
        AstVarScope* ثابت vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->جوڑو_Varsp(vscp);
        أرجع vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_سطر* flp, AstScope* scopep, بنانشانی چوڑائی,
                      ثابت std::string& name = "") {
        أرجع make(flp, scopep, scopep->لبھو_ثنائی_ہندسہ_DType(چوڑائی, چوڑائی, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    ناپ_t nReused() ثابت { أرجع m_nReused; }
};

#جے_انت  // Guard
