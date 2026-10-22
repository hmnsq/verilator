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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3OrderGraph.h"
#అన్తర్భావ "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

తరగతి V3_క్రమం_Process_Domains final {
    // STATE
    క్రమం_లేఖాచిత్రం& m_లేఖాచిత్రం;  // The ordering graph
    స్థిరరాశి string m_ట్యాగ్;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    స్థిరరాశి V3_క్రమం::ExternalTriggersProvider m_externalTriggers;
    std::సదిశ<క్రమం_Logic_శీర్షం*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    శూన్య processDomains() {
        // The triggers of the current vertex
        std::సమితి<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        కొరకు (V3_లేఖాచిత్రం_శీర్షం& it : m_లేఖాచిత్రం.vertices()) {
            క్రమం_Either_శీర్షం* స్థిరరాశి vtxp = it.as<క్రమం_Either_శీర్షం>();
            // Sequential logic already has its domain set
            ఒకవేళ (vtxp->domainp()) కొనసాగు;

            // Reset state
            triggers.clear();

            క్రమం_Logic_శీర్షం* స్థిరరాశి lvtxp = vtxp->cast<క్రమం_Logic_శీర్షం>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            ఒకవేళ (స్థిరరాశి క్రమం_Domain* స్థిరరాశి hybridp = lvtxp ? lvtxp->hybridp() : శూన్యసూచిక) {
                స్థిరరాశి std::సమితి<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.చొప్పించు(hybridTriggers.begin(), hybridTriggers.చివర());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            కొరకు (V3_లేఖాచిత్రం_అంచు& అంచు : vtxp->లోపల_అంచులు()) {
                క్రమం_Either_శీర్షం* స్థిరరాశి fromVtxp = అంచు.fromp()->as<క్రమం_Either_శీర్షం>();
                // Cut edge
                ఒకవేళ (!అంచు.బరువు_2()) కొనసాగు;
                // some edges can be ignored as they only encode ordering constraints
                ఒకవేళ (!fromVtxp->domainMatters()) కొనసాగు;

                // Add driver's domains
                స్థిరరాశి క్రమం_Domain* స్థిరరాశి fromDomainp = fromVtxp->domainp();
                స్థిరరాశి std::సమితి<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.చొప్పించు(fromTriggers.begin(), fromTriggers.చివర());

                // Add in any external domains of variables
                ఒకవేళ (క్రమం_Var_శీర్షం* స్థిరరాశి varVtxp = fromVtxp->cast<క్రమం_Var_శీర్షం>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            స్థిరరాశి క్రమం_Domain* స్థిరరాశి domainp = m_లేఖాచిత్రం.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            ఒకవేళ (domainp->triggers().ఖాళీలు() && lvtxp) m_untriggeredps.నెట్టు_వెనుక(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3_క్రమం_Process_Domains(క్రమం_లేఖాచిత్రం& లేఖాచిత్రం, స్థిరరాశి string& ట్యాగ్,
                          స్థిరరాశి V3_క్రమం::ExternalTriggersProvider& externalTriggers)
        : m_లేఖాచిత్రం{లేఖాచిత్రం}
        , m_ట్యాగ్{ట్యాగ్}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        ఒకవేళ (dump_లేఖాచిత్రం_Level()) m_లేఖాచిత్రం.dumpDotFilePrefixed(m_ట్యాగ్ + "_orderg_domain");

        // Delete logic that is never triggered
        కొరకు (క్రమం_Logic_శీర్షం* స్థిరరాశి lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().ఖాళీలు(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlink_Fr_వెనుక()->delete_వృక్షం();
            lVtxp->unlinkDelete(&m_లేఖాచిత్రం);
        }
    }

    ~V3_క్రమం_Process_Domains() = అప్రమేయ;

బహిరంగ:
    // Order the logic
    స్థిర శూన్య apply(క్రమం_లేఖాచిత్రం& లేఖాచిత్రం, స్థిరరాశి string& ట్యాగ్,
                      స్థిరరాశి V3_క్రమం::ExternalTriggersProvider& externalTriggers) {
        V3_క్రమం_Process_Domains{లేఖాచిత్రం, ట్యాగ్, externalTriggers};
    }
};

శూన్య V3_క్రమం::processDomains(క్రమం_లేఖాచిత్రం& లేఖాచిత్రం,  //
                             స్థిరరాశి std::string& ట్యాగ్,  //
                             స్థిరరాశి ExternalTriggersProvider& externalTriggers) {
    V3_క్రమం_Process_Domains::apply(లేఖాచిత్రం, ట్యాగ్, externalTriggers);
}
