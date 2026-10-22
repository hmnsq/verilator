// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Block code ordering
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//
//  Initial graph dependency builder for ordering
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderGraph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

ਵਰਗ V3_ਕ੍ਰਮ_Process_Domains final {
    // STATE
    ਕ੍ਰਮ_ਗ੍ਰਾਫ& m_ਗ੍ਰਾਫ;  // The ordering graph
    ਅਚਲ string m_ਟੈਗ;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ਅਚਲ V3_ਕ੍ਰਮ::ExternalTriggersProvider m_externalTriggers;
    std::ਸਦਿਸ਼<ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    ਖਾਲੀ processDomains() {
        // The triggers of the current vertex
        std::ਸੈੱਟ<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        ਲਈ (V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼& it : m_ਗ੍ਰਾਫ.vertices()) {
            ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼* ਅਚਲ vtxp = it.as<ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼>();
            // Sequential logic already has its domain set
            ਜੇ (vtxp->domainp()) ਜਾਰੀ;

            // Reset state
            triggers.clear();

            ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lvtxp = vtxp->cast<ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            ਜੇ (ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ hybridp = lvtxp ? lvtxp->hybridp() : ਖਾਲੀ_ਸੂਚਕ) {
                ਅਚਲ std::ਸੈੱਟ<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.ਪਾਓ(hybridTriggers.begin(), hybridTriggers.ਸਿਰਾ());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vtxp->ਵਿੱਚ_ਕਿਨਾਰੇ()) {
                ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼* ਅਚਲ fromVtxp = ਕਿਨਾਰਾ.fromp()->as<ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼>();
                // Cut edge
                ਜੇ (!ਕਿਨਾਰਾ.ਭਾਰ_2()) ਜਾਰੀ;
                // some edges can be ignored as they only encode ordering constraints
                ਜੇ (!fromVtxp->domainMatters()) ਜਾਰੀ;

                // Add driver's domains
                ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ fromDomainp = fromVtxp->domainp();
                ਅਚਲ std::ਸੈੱਟ<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.ਪਾਓ(fromTriggers.begin(), fromTriggers.ਸਿਰਾ());

                // Add in any external domains of variables
                ਜੇ (ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਅਚਲ varVtxp = fromVtxp->cast<ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ domainp = m_ਗ੍ਰਾਫ.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            ਜੇ (domainp->triggers().ਖਾਲੀ_ਥਾਵਾਂ() && lvtxp) m_untriggeredps.ਧੱਕੋ_ਪਿੱਠ(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3_ਕ੍ਰਮ_Process_Domains(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਅਚਲ string& ਟੈਗ,
                          ਅਚਲ V3_ਕ੍ਰਮ::ExternalTriggersProvider& externalTriggers)
        : m_ਗ੍ਰਾਫ{ਗ੍ਰਾਫ}
        , m_ਟੈਗ{ਟੈਗ}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        ਜੇ (dump_ਗ੍ਰਾਫ_Level()) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed(m_ਟੈਗ + "_orderg_domain");

        // Delete logic that is never triggered
        ਲਈ (ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਅਚਲ lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().ਖਾਲੀ_ਥਾਵਾਂ(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
            lVtxp->unlinkDelete(&m_ਗ੍ਰਾਫ);
        }
    }

    ~V3_ਕ੍ਰਮ_Process_Domains() = ਮੂਲ;

ਜਨਤਕ:
    // Order the logic
    ਸਥਿਰ ਖਾਲੀ apply(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ, ਅਚਲ string& ਟੈਗ,
                      ਅਚਲ V3_ਕ੍ਰਮ::ExternalTriggersProvider& externalTriggers) {
        V3_ਕ੍ਰਮ_Process_Domains{ਗ੍ਰਾਫ, ਟੈਗ, externalTriggers};
    }
};

ਖਾਲੀ V3_ਕ੍ਰਮ::processDomains(ਕ੍ਰਮ_ਗ੍ਰਾਫ& ਗ੍ਰਾਫ,  //
                             ਅਚਲ std::string& ਟੈਗ,  //
                             ਅਚਲ ExternalTriggersProvider& externalTriggers) {
    V3_ਕ੍ਰਮ_Process_Domains::apply(ਗ੍ਰਾਫ, ਟੈਗ, externalTriggers);
}
