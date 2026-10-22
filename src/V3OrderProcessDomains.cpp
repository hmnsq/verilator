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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3OrderGraph.h"
#شامل "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

جماعت V3_ترتیب_Process_Domains final {
    // STATE
    ترتیب_گراف& m_گراف;  // The ordering graph
    ثابت string m_لیبل;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ثابت V3_ترتیب::ExternalTriggersProvider m_externalTriggers;
    std::سمتیہ<ترتیب_Logic_راس*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    خالی processDomains() {
        // The triggers of the current vertex
        std::سیٹ<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        برائے (V3_گراف_راس& it : m_گراف.vertices()) {
            ترتیب_Either_راس* ثابت vtxp = it.as<ترتیب_Either_راس>();
            // Sequential logic already has its domain set
            اگر (vtxp->domainp()) جاری;

            // Reset state
            triggers.clear();

            ترتیب_Logic_راس* ثابت lvtxp = vtxp->cast<ترتیب_Logic_راس>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            اگر (ثابت ترتیب_Domain* ثابت hybridp = lvtxp ? lvtxp->hybridp() : خالی_اشاریہ) {
                ثابت std::سیٹ<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.داخل_کرو(hybridTriggers.begin(), hybridTriggers.انتہا());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            برائے (V3_گراف_کنارہ& کنارہ : vtxp->اندر_کنارے()) {
                ترتیب_Either_راس* ثابت fromVtxp = کنارہ.fromp()->as<ترتیب_Either_راس>();
                // Cut edge
                اگر (!کنارہ.وزن_2()) جاری;
                // some edges can be ignored as they only encode ordering constraints
                اگر (!fromVtxp->domainMatters()) جاری;

                // Add driver's domains
                ثابت ترتیب_Domain* ثابت fromDomainp = fromVtxp->domainp();
                ثابت std::سیٹ<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.داخل_کرو(fromTriggers.begin(), fromTriggers.انتہا());

                // Add in any external domains of variables
                اگر (ترتیب_Var_راس* ثابت varVtxp = fromVtxp->cast<ترتیب_Var_راس>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ثابت ترتیب_Domain* ثابت domainp = m_گراف.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            اگر (domainp->triggers().خالی_خانے() && lvtxp) m_untriggeredps.ڈالو_عقب(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3_ترتیب_Process_Domains(ترتیب_گراف& گراف, ثابت string& لیبل,
                          ثابت V3_ترتیب::ExternalTriggersProvider& externalTriggers)
        : m_گراف{گراف}
        , m_لیبل{لیبل}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        اگر (dump_گراف_Level()) m_گراف.dumpDotFilePrefixed(m_لیبل + "_orderg_domain");

        // Delete logic that is never triggered
        برائے (ترتیب_Logic_راس* ثابت lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().خالی_خانے(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlink_Fr_عقب()->delete_درخت();
            lVtxp->unlinkDelete(&m_گراف);
        }
    }

    ~V3_ترتیب_Process_Domains() = طےشدہ;

عوامی:
    // Order the logic
    ساکن خالی apply(ترتیب_گراف& گراف, ثابت string& لیبل,
                      ثابت V3_ترتیب::ExternalTriggersProvider& externalTriggers) {
        V3_ترتیب_Process_Domains{گراف, لیبل, externalTriggers};
    }
};

خالی V3_ترتیب::processDomains(ترتیب_گراف& گراف,  //
                             ثابت std::string& لیبل,  //
                             ثابت ExternalTriggersProvider& externalTriggers) {
    V3_ترتیب_Process_Domains::apply(گراف, لیبل, externalTriggers);
}
