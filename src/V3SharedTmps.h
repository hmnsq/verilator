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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Hash.h"
#அந்தர்பாவ "V3HashTable.h"
#அந்தர்பாவ "V3String.h"

#அந்தர்பாவ <set>
#அந்தர்பாவ <string>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    கட்டமைப்பு சாவி_2 final {
        மாறிலி Ast_முனை_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        மாறிலி Ast_முனை_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    கட்டமைப்பு சாவி_Hash final {
        size_t operator()(மாறிலி Ast_முனை_Module* modp, மாறிலி std::string& name,
                          மாறிலி Ast_முனை_DType* dtypep) மாறிலி {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            திரும்பு hash.மதிப்பு();
        }
        size_t operator()(மாறிலி சாவி_2& சாவி) மாறிலி {
            திரும்பு (*this)(சாவி.m_modp, சாவி.m_name, சாவி.m_dtypep);
        }
    };
    கட்டமைப்பு சாவி_Equal final {
        bool operator()(மாறிலி சாவி_2& சாவி, மாறிலி Ast_முனை_Module* modp, மாறிலி std::string& name,
                        மாறிலி Ast_முனை_DType* dtypep) மாறிலி {
            திரும்பு சாவி.m_modp == modp && சாவி.m_dtypep == dtypep && சாவி.m_name == name;
        }
        bool operator()(மாறிலி சாவி_2& a, மாறிலி சாவி_2& b) மாறிலி {
            திரும்பு (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    கட்டமைப்பு Tmps final {
        std::vector<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<மாறிலி AstScope*, size_t> m_nUsed;
    };

    // STATE
    மாறிலி std::string m_prefix;  // Prefix of all variable names
    மாறிலி VVarType m_varType;  // Type of all variables
    V3HashMap<சாவி_2, Tmps, சாவி_Hash, சாவி_Equal> m_tmps;  // The temporaries created so far
    size_t m_nVars = 0;  // Number of variables created, used to make unique names
    size_t m_nReused = 0;  // Number of temporaries using an existing variable

பொது:
    // CONSTRUCTORS
    V3SharedTmps(மாறிலி std::string& prefix, VVarType varType)
        : m_prefix{prefix}
        , m_varType{varType} {
        UASSERT(VString::startsWith(prefix, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::endsWith(prefix, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        நிலையான std::set<std::string> s_prefixes;
        மாறிலி bool isNew = s_prefixes.செருகு(m_prefix).second;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << prefix << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_வரி* flp, AstScope* scopep, Ast_முனை_DType* dtypep,
                      மாறிலி std::string& name = "") {
        மாறிலி Ast_முனை_Module* மாறிலி modp = scopep->modp();
        மாறிலி auto pair = m_tmps.செருகு_Lazy(modp, name, dtypep, [&]() {  //
            திரும்பு std::make_pair(சாவி_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.first.மதிப்பு();
        மாறிலி size_t n = tmps.m_nUsed[scopep]++;
        எனில் (n < tmps.m_varps.size()) {
            ++m_nReused;
        } இல்லையெனில் {
            std::string uniqueName = m_prefix + "_";
            எனில் (!name.empty()) uniqueName += name + "_";
            uniqueName += std::to_string(m_nVars++);
            AstVar* மாறிலி varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->addStmtsp(varp);
            tmps.m_varps.emplace_back(varp);
        }
        AstVarScope* மாறிலி vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->addVarsp(vscp);
        திரும்பு vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_வரி* flp, AstScope* scopep, unsigned width,
                      மாறிலி std::string& name = "") {
        திரும்பு make(flp, scopep, scopep->findBitDType(width, width, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    size_t nReused() மாறிலி { திரும்பு m_nReused; }
};

#endif  // Guard
