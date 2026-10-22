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

#నిర్వచితం_కాకపోతే VERILATOR_V3SHAREDTMPS_H_
#నిర్వచించు VERILATOR_V3SHAREDTMPS_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Hash.h"
#అన్తర్భావ "V3HashTable.h"
#అన్తర్భావ "V3String.h"

#అన్తర్భావ <set>
#అన్తర్భావ <string>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

class V3SharedTmps final {
    // TYPES
    // Temporaries are identified by their module, name and data type
    నిర్మాణం కీలకం_2 final {
        స్థిరరాశి Ast_కణుపు_Module* m_modp;  // The module of the scopes
        std::string m_name;  // Name of the temporary
        స్థిరరాశి Ast_కణుపు_DType* m_dtypep;  // Data type of the temporary
    };
    // Hash and equality of Keys, which can be looked up by their parts, without making one
    నిర్మాణం కీలకం_Hash final {
        కొలత_t క్రియాకారకం()(స్థిరరాశి Ast_కణుపు_Module* modp, స్థిరరాశి std::string& name,
                          స్థిరరాశి Ast_కణుపు_DType* dtypep) స్థిరరాశి {
            V3Hash hash{modp};
            hash += name;
            hash += dtypep;
            తిరిగి hash.విలువ();
        }
        కొలత_t క్రియాకారకం()(స్థిరరాశి కీలకం_2& కీలకం) స్థిరరాశి {
            తిరిగి (*this)(కీలకం.m_modp, కీలకం.m_name, కీలకం.m_dtypep);
        }
    };
    నిర్మాణం కీలకం_Equal final {
        bool క్రియాకారకం()(స్థిరరాశి కీలకం_2& కీలకం, స్థిరరాశి Ast_కణుపు_Module* modp, స్థిరరాశి std::string& name,
                        స్థిరరాశి Ast_కణుపు_DType* dtypep) స్థిరరాశి {
            తిరిగి కీలకం.m_modp == modp && కీలకం.m_dtypep == dtypep && కీలకం.m_name == name;
        }
        bool క్రియాకారకం()(స్థిరరాశి కీలకం_2& a, స్థిరరాశి కీలకం_2& b) స్థిరరాశి {
            తిరిగి (*this)(a, b.m_modp, b.m_name, b.m_dtypep);
        }
    };
    నిర్మాణం Tmps final {
        std::సదిశ<AstVar*> m_varps;  // Variables created so far
        // Number of variables used by each scope
        std::unordered_map<స్థిరరాశి AstScope*, కొలత_t> m_n_వాడిన;
    };

    // STATE
    స్థిరరాశి std::string m_ఉపసర్గ;  // Prefix of all variable names
    స్థిరరాశి VVarType m_varType;  // Type of all variables
    V3HashMap<కీలకం_2, Tmps, కీలకం_Hash, కీలకం_Equal> m_tmps;  // The temporaries created so far
    కొలత_t m_nVars = 0;  // Number of variables created, used to make unique names
    కొలత_t m_nReused = 0;  // Number of temporaries using an existing variable

బహిరంగ:
    // CONSTRUCTORS
    V3SharedTmps(స్థిరరాశి std::string& ఉపసర్గ, VVarType varType)
        : m_ఉపసర్గ{ఉపసర్గ}
        , m_varType{varType} {
        UASSERT(VString::startsWith(ఉపసర్గ, "__V"), "Prefix must start with '__V'");
        UASSERT(!VString::చివర్లు_With(ఉపసర్గ, "_"), "Prefix must not end with '_'");
        // Prefixes used so far during the whole of compilation. Must be unique.
        స్థిర std::సమితి<std::string> s_prefixes;
        స్థిరరాశి bool isNew = s_prefixes.చొప్పించు(m_ఉపసర్గ).రెండవ;
        UASSERT(isNew, "V3SharedTmps prefix is not unique: '" << ఉపసర్గ << "'");
    }

    // METHODS
    // Create a new temporary of type 'dtypep' in 'scopep', and return its AstVarScope
    AstVarScope* make(File_పంక్తి* flp, AstScope* scopep, Ast_కణుపు_DType* dtypep,
                      స్థిరరాశి std::string& name = "") {
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = scopep->modp();
        స్థిరరాశి స్వయంచాలక pair = m_tmps.చొప్పించు_Lazy(modp, name, dtypep, [&]() {  //
            తిరిగి std::make_pair(కీలకం_2{modp, name, dtypep}, Tmps{});
        });
        Tmps& tmps = pair.మొదటి.విలువ();
        స్థిరరాశి కొలత_t n = tmps.m_n_వాడిన[scopep]++;
        ఒకవేళ (n < tmps.m_varps.కొలత()) {
            ++m_nReused;
        } లేకపోతే {
            std::string uniqueName = m_ఉపసర్గ + "_";
            ఒకవేళ (!name.ఖాళీలు()) uniqueName += name + "_";
            uniqueName += std::గమ్యం_string(m_nVars++);
            AstVar* స్థిరరాశి varp = new AstVar{flp, m_varType, uniqueName, dtypep};
            scopep->modp()->కూడు_Stmtsp(varp);
            tmps.m_varps.emplace_వెనుక(varp);
        }
        AstVarScope* స్థిరరాశి vscp = new AstVarScope{flp, scopep, tmps.m_varps[n]};
        scopep->కూడు_Varsp(vscp);
        తిరిగి vscp;
    }
    // Same as above, but with a 2-state unsigned packed type of the given 'width'
    AstVarScope* make(File_పంక్తి* flp, AstScope* scopep, గుర్తులేని వెడల్పు,
                      స్థిరరాశి std::string& name = "") {
        తిరిగి make(flp, scopep, scopep->కనుగొను_బిట్టు_DType(వెడల్పు, వెడల్పు, VSigning::UNSIGNED), name);
    }

    // Number of temporaries using an existing variable
    కొలత_t nReused() స్థిరరాశి { తిరిగి m_nReused; }
};

#అయితే_ముగింపు  // Guard
