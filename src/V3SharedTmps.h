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

#که_نه_تعریف_شوی VERILATOR_V3SHAREDTMPS_H_
#تعریف_کړه VERILATOR_V3SHAREDTMPS_H_

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
    بنية کلی_2 final {
        ثابت Ast_غوټه_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ثابت Ast_غوټه_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    بنية کلی_Hash final {
        اندازه_t چلوونکی()(ثابت Ast_غوټه_Module* modp, ثابت std::string& name,
                          ثابت Ast_غوټه_DType* dtypep) ثابت {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            أرجع hash.ارزښت();
        }
        اندازه_t چلوونکی()(ثابت کلی_2& کلی) ثابت {
            أرجع (*this)(کلی.m_modp, کلی.m_name, کلی.m_dtypep);
        }
    };
    بنية کلی_Equal final {
        bool چلوونکی()(ثابت کلی_2& کلی, ثابت Ast_غوټه_Module* modp, ثابت std::string& name,
                        ثابت Ast_غوټه_DType* dtypep) ثابت {
            أرجع کلی.m_modp == modp && کلی.m_dtypep == dtypep && کلی.m_name == name;
        }
        bool چلوونکی()(ثابت کلی_2& a, ثابت کلی_2& b) ثابت {
            أرجع (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    بنية Tmps final {
        std::ویکټور<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ثابت AstScope*, اندازه_t> m_n_کارول_شوی;
    };

    // STATE
    ثابت std::string m_مختاړی;  // Prefix of all variable names
    ثابت VVarType m_varType;  // Type of all variables
    V3HashMap<کلی_2, Tmps, کلی_Hash, کلی_Equal> m_tmps;  // The temporaries created so far
    اندازه_t m_nVars = 0;  // Number of variables created, used to make unique names
    اندازه_t m_nReused = 0;  // Number of temporaries using an existing variable

عام:
    // CONSTRUCTORS
    V3SharedTmps(ثابت std::string& مختاړی, VVarType varType)
        : m_مختاړی{مختاړی}
        , m_varType{varType} {
        UASSERT(VString::startsWith(مختاړی, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::پایونه_With(مختاړی, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        ساكن std::سېټ<std::string> s_prefixes;
        ثابت bool isNew = s_prefixes.ورننباسه(m_مختاړی).دوهم;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << مختاړی << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_کرښه* flp, AstScope* scopep, Ast_غوټه_DType* dtypep,
                      ثابت std::string& name = "") {
        ثابت Ast_غوټه_Module* ثابت modp = scopep->modp();
        ثابت خپلکاری pair = m_tmps.ورننباسه_Lazy(modp, name, dtypep, [&]() {  //
            أرجع std::make_pair(کلی_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.لومړی.ارزښت();
        ثابت اندازه_t n = tmps.m_n_کارول_شوی[scopep]++;
        إذا (n < tmps.m_varps.اندازه()) {
            ++m_nReused;
        } وإلا {
            std::string uniqueName = m_مختاړی + "_";
            إذا (!name.تشه()) uniqueName += name + "_";
            uniqueName += std::منزل_string(m_nVars++);
            AstVar* ثابت varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->جمع_کړه_Stmtsp(varp);
            tmps.m_varps.emplace_شا(varp);
        }
        AstVarScope* ثابت vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->جمع_کړه_Varsp(vscp);
        أرجع vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_کرښه* flp, AstScope* scopep, بینښه پلنوالی,
                      ثابت std::string& name = "") {
        أرجع make(flp, scopep, scopep->پیدا_کړه_بټ_DType(پلنوالی, پلنوالی, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    اندازه_t nReused() ثابت { أرجع m_nReused; }
};

#د_که_پای  // Guard
