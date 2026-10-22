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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3OrderGraph.h"
#ಅನ್ತರ್ಭಾವ "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

ವರ್ಗ V3_ಕ್ರಮ_Process_Domains final {
    // STATE
    ಕ್ರಮ_ನಕ್ಷೆ& m_ನಕ್ಷೆ;  // The ordering graph
    ಸ್ಥಿರಾಂಕ string m_ಟ್ಯಾಗ್;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ಸ್ಥಿರಾಂಕ V3_ಕ್ರಮ::ExternalTriggersProvider m_externalTriggers;
    std::ಸದಿಶ<ಕ್ರಮ_Logic_ಶೃಂಗ*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    ಶೂನ್ಯ processDomains() {
        // The triggers of the current vertex
        std::ಗಣ<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        ಗಾಗಿ (V3_ನಕ್ಷೆ_ಶೃಂಗ& it : m_ನಕ್ಷೆ.vertices()) {
            ಕ್ರಮ_Either_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vtxp = it.as<ಕ್ರಮ_Either_ಶೃಂಗ>();
            // Sequential logic already has its domain set
            ಆದರೆ (vtxp->domainp()) ಮುಂದುವರಿ;

            // Reset state
            triggers.clear();

            ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lvtxp = vtxp->cast<ಕ್ರಮ_Logic_ಶೃಂಗ>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            ಆದರೆ (ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ hybridp = lvtxp ? lvtxp->hybridp() : ಶೂನ್ಯಸೂಚಕ) {
                ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.ಸೇರಿಸು(hybridTriggers.begin(), hybridTriggers.ಕೊನೆ());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vtxp->ಒಳ_ಅಂಚುಗಳು()) {
                ಕ್ರಮ_Either_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ fromVtxp = ಅಂಚು.fromp()->as<ಕ್ರಮ_Either_ಶೃಂಗ>();
                // Cut edge
                ಆದರೆ (!ಅಂಚು.ತೂಕ_2()) ಮುಂದುವರಿ;
                // some edges can be ignored as they only encode ordering constraints
                ಆದರೆ (!fromVtxp->domainMatters()) ಮುಂದುವರಿ;

                // Add driver's domains
                ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ fromDomainp = fromVtxp->domainp();
                ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.ಸೇರಿಸು(fromTriggers.begin(), fromTriggers.ಕೊನೆ());

                // Add in any external domains of variables
                ಆದರೆ (ಕ್ರಮ_Var_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ varVtxp = fromVtxp->cast<ಕ್ರಮ_Var_ಶೃಂಗ>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ domainp = m_ನಕ್ಷೆ.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            ಆದರೆ (domainp->triggers().ಖಾಲಿಗಳು() && lvtxp) m_untriggeredps.ತಳ್ಳು_ಹಿಂಭಾಗ(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3_ಕ್ರಮ_Process_Domains(ಕ್ರಮ_ನಕ್ಷೆ& ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್,
                          ಸ್ಥಿರಾಂಕ V3_ಕ್ರಮ::ExternalTriggersProvider& externalTriggers)
        : m_ನಕ್ಷೆ{ನಕ್ಷೆ}
        , m_ಟ್ಯಾಗ್{ಟ್ಯಾಗ್}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        ಆದರೆ (dump_ನಕ್ಷೆ_Level()) m_ನಕ್ಷೆ.dumpDotFilePrefixed(m_ಟ್ಯಾಗ್ + "_orderg_domain");

        // Delete logic that is never triggered
        ಗಾಗಿ (ಕ್ರಮ_Logic_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().ಖಾಲಿಗಳು(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ();
            lVtxp->unlinkDelete(&m_ನಕ್ಷೆ);
        }
    }

    ~V3_ಕ್ರಮ_Process_Domains() = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    // Order the logic
    ಸ್ಥಿರ ಶೂನ್ಯ apply(ಕ್ರಮ_ನಕ್ಷೆ& ನಕ್ಷೆ, ಸ್ಥಿರಾಂಕ string& ಟ್ಯಾಗ್,
                      ಸ್ಥಿರಾಂಕ V3_ಕ್ರಮ::ExternalTriggersProvider& externalTriggers) {
        V3_ಕ್ರಮ_Process_Domains{ನಕ್ಷೆ, ಟ್ಯಾಗ್, externalTriggers};
    }
};

ಶೂನ್ಯ V3_ಕ್ರಮ::processDomains(ಕ್ರಮ_ನಕ್ಷೆ& ನಕ್ಷೆ,  //
                             ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್,  //
                             ಸ್ಥಿರಾಂಕ ExternalTriggersProvider& externalTriggers) {
    V3_ಕ್ರಮ_Process_Domains::apply(ನಕ್ಷೆ, ಟ್ಯಾಗ್, externalTriggers);
}
