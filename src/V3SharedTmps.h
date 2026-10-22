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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Hash.h"
#ଅନ୍ତର୍ଭାଵ "V3HashTable.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    ସଂରଚନା ଚାବି_2 final {
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    ସଂରଚନା ଚାବି_Hash final {
        size_t operator()(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ std::string& name,
                          ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) ଅଚଳ {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            ଫେରନ୍ତୁ hash.ମୂଲ୍ୟ();
        }
        size_t operator()(ଅଚଳ ଚାବି_2& ଚାବି) ଅଚଳ {
            ଫେରନ୍ତୁ (*this)(ଚାବି.m_modp, ଚାବି.m_name, ଚାବି.m_dtypep);
        }
    };
    ସଂରଚନା ଚାବି_Equal final {
        bool operator()(ଅଚଳ ଚାବି_2& ଚାବି, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ std::string& name,
                        ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep) ଅଚଳ {
            ଫେରନ୍ତୁ ଚାବି.m_modp == modp && ଚାବି.m_dtypep == dtypep && ଚାବି.m_name == name;
        }
        bool operator()(ଅଚଳ ଚାବି_2& a, ଅଚଳ ଚାବି_2& b) ଅଚଳ {
            ଫେରନ୍ତୁ (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    ସଂରଚନା Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<ଅଚଳ AstScope*, size_t> m_nUsed;
    };

    // STATE
    ଅଚଳ std::string m_prefix;  // Prefix of all variable names
    ଅଚଳ VVarType m_varType;  // Type of all variables
    V3HashMap<ଚାବି_2, Tmps, ଚାବି_Hash, ଚାବି_Equal> m_tmps;  // The temporaries created so far
    size_t m_nVars = 0;  // Number of variables created, used to make unique names
    size_t m_nReused = 0;  // Number of temporaries using an existing variable

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    V3SharedTmps(ଅଚଳ std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        ସ୍ଥିର std::set<std::string> s_prefixes;
        ଅଚଳ bool isNew = s_prefixes.ଭର୍ତ୍ତି(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_ଧାଡ଼ି* flp, AstScope* scopep, Ast_ଗଣ୍ଠି_DType* dtypep,
                      ଅଚଳ std::string& name = "") {
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = scopep->modp();
        ଅଚଳ auto pair = m_tmps.ଭର୍ତ୍ତି_Lazy(modp, name, dtypep, [&]() {  //
            ଫେରନ୍ତୁ std::make_pair(ଚାବି_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.ମୂଲ୍ୟ();
        ଅଚଳ size_t n = tmps.m_nUsed[scopep]++;
        ଯଦି (n < tmps.m_varps.size()) {
            ++m_nReused;
        } ଅନ୍ୟଥା {
            std::string uniqueName = m_prefix + "_";
            ଯଦି (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* ଅଚଳ varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* ଅଚଳ vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        ଫେରନ୍ତୁ vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_ଧାଡ଼ି* flp, AstScope* scopep, unsigned width,
                      ଅଚଳ std::string& name = "") {
        ଫେରନ୍ତୁ make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    size_t nReused() ଅଚଳ { ଫେରନ୍ତୁ m_nReused; }
};

#endif  // Guard
