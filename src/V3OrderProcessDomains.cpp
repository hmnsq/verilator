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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3OrderGraph.h"
#ضمن "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

صنف V3_ترتيب_Process_Domains final {
    // STATE
    ترتيب_گراف& m_گراف;  // The ordering graph
    ثابت string m_ٽيگ;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ثابت V3_ترتيب::ExternalTriggersProvider m_externalTriggers;
    std::ويڪٽر<ترتيب_Logic_راس*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    فراغ processDomains() {
        // The triggers of the current vertex
        std::سيٽ<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        لكل (V3_گراف_راس& it : m_گراف.vertices()) {
            ترتيب_Either_راس* ثابت vtxp = it.as<ترتيب_Either_راس>();
            // Sequential logic already has its domain set
            إذا (vtxp->domainp()) استمر;

            // Reset state
            triggers.clear();

            ترتيب_Logic_راس* ثابت lvtxp = vtxp->cast<ترتيب_Logic_راس>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            إذا (ثابت ترتيب_Domain* ثابت hybridp = lvtxp ? lvtxp->hybridp() : خالي_اشارو) {
                ثابت std::سيٽ<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.وجهو(hybridTriggers.begin(), hybridTriggers.پڇاڙي());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            لكل (V3_گراف_ڪنارو& ڪنارو : vtxp->اندريون_ڪنارا()) {
                ترتيب_Either_راس* ثابت fromVtxp = ڪنارو.fromp()->as<ترتيب_Either_راس>();
                // Cut edge
                إذا (!ڪنارو.وزن_2()) استمر;
                // some edges can be ignored as they only encode ordering constraints
                إذا (!fromVtxp->domainMatters()) استمر;

                // Add driver's domains
                ثابت ترتيب_Domain* ثابت fromDomainp = fromVtxp->domainp();
                ثابت std::سيٽ<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.وجهو(fromTriggers.begin(), fromTriggers.پڇاڙي());

                // Add in any external domains of variables
                إذا (ترتيب_Var_راس* ثابت varVtxp = fromVtxp->cast<ترتيب_Var_راس>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ثابت ترتيب_Domain* ثابت domainp = m_گراف.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            إذا (domainp->triggers().خالي_جايون() && lvtxp) m_untriggeredps.ڌڪو_پٺ(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3_ترتيب_Process_Domains(ترتيب_گراف& گراف, ثابت string& ٽيگ,
                          ثابت V3_ترتيب::ExternalTriggersProvider& externalTriggers)
        : m_گراف{گراف}
        , m_ٽيگ{ٽيگ}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        إذا (dump_گراف_Level()) m_گراف.dumpDotFilePrefixed(m_ٽيگ + "_orderg_domain");

        // Delete logic that is never triggered
        لكل (ترتيب_Logic_راس* ثابت lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().خالي_جايون(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlink_Fr_پٺ()->delete_وڻ();
            lVtxp->unlinkDelete(&m_گراف);
        }
    }

    ~V3_ترتيب_Process_Domains() = افتراضي;

عام:
    // Order the logic
    ساكن فراغ apply(ترتيب_گراف& گراف, ثابت string& ٽيگ,
                      ثابت V3_ترتيب::ExternalTriggersProvider& externalTriggers) {
        V3_ترتيب_Process_Domains{گراف, ٽيگ, externalTriggers};
    }
};

فراغ V3_ترتيب::processDomains(ترتيب_گراف& گراف,  //
                             ثابت std::string& ٽيگ,  //
                             ثابت ExternalTriggersProvider& externalTriggers) {
    V3_ترتيب_Process_Domains::apply(گراف, ٽيگ, externalTriggers);
}
