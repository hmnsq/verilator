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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3OrderGraph.h"
#अन्तर्भाव "V3OrderInternal.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProcessDomains class

वर्ग V3_क्रमः_Process_Domains final {
    // STATE
    क्रमः_Graph& m_graph;  // The ordering graph
    ध्रुव string m_चिह्नकम्;  // Substring to add to generated names
    // This is a function provided by the invoker of the ordering that can provide additional
    // trigger indices that when set indicate the passed AstVarScope might have changed external
    // to the code being ordered, hence dependent logic needs to be evaluated.
    ध्रुव V3_क्रमः::ExternalTriggersProvider m_externalTriggers;
    std::सदिशम्<क्रमः_Logic_Vertex*> m_untriggeredps;  // Logic never triggered and can be deleted

    // METHODS

    // Compute the OrderDomain for combinational and hybrid logic vertices.
    शून्य processDomains() {
        // The triggers of the current vertex
        std::समुच्चयः<uint32_t> triggers;

        // The graph is already in topological order, so can walk vertices once
        प्रति (V3GraphVertex& it : m_graph.vertices()) {
            क्रमः_Either_Vertex* ध्रुव vtxp = it.as<क्रमः_Either_Vertex>();
            // Sequential logic already has its domain set
            यदि (vtxp->domainp()) अनुवर्त;

            // Reset state
            triggers.clear();

            क्रमः_Logic_Vertex* ध्रुव lvtxp = vtxp->cast<क्रमः_Logic_Vertex>();

            // For logic vertices, start with the explicit hybrid sensitivities, if any
            यदि (ध्रुव क्रमः_Domain* ध्रुव hybridp = lvtxp ? lvtxp->hybridp() : शून्यसूचकः) {
                ध्रुव std::समुच्चयः<uint32_t>& hybridTriggers = hybridp->triggers();
                triggers.निवेशय(hybridTriggers.begin(), hybridTriggers.अन्तः_2());
            }

            // For each incoming edge, union the source vertex's domain into the current domain
            प्रति (V3GraphEdge& edge : vtxp->अन्तर्_धाराः()) {
                क्रमः_Either_Vertex* ध्रुव fromVtxp = edge.fromp()->as<क्रमः_Either_Vertex>();
                // Cut edge
                यदि (!edge.weight()) अनुवर्त;
                // some edges can be ignored as they only encode ordering constraints
                यदि (!fromVtxp->domainMatters()) अनुवर्त;

                // Add driver's domains
                ध्रुव क्रमः_Domain* ध्रुव fromDomainp = fromVtxp->domainp();
                ध्रुव std::समुच्चयः<uint32_t>& fromTriggers = fromDomainp->triggers();
                triggers.निवेशय(fromTriggers.begin(), fromTriggers.अन्तः_2());

                // Add in any external domains of variables
                यदि (क्रमः_Var_Vertex* ध्रुव varVtxp = fromVtxp->cast<क्रमः_Var_Vertex>()) {
                    m_externalTriggers(varVtxp->vscp(), triggers);
                }
            }

            // Set the domain of the vertex
            ध्रुव क्रमः_Domain* ध्रुव domainp = m_graph.domainp(triggers);
            vtxp->domainp(domainp);

            // If nothing triggers this vertex, we can delete the corresponding logic
            यदि (domainp->triggers().रिक्तानि() && lvtxp) m_untriggeredps.प्रक्षिप_पृष्ठभागः(lvtxp);
        }
    }

    // CONSTRUCTOR
    V3_क्रमः_Process_Domains(क्रमः_Graph& graph, ध्रुव string& चिह्नकम्,
                          ध्रुव V3_क्रमः::ExternalTriggersProvider& externalTriggers)
        : m_graph{graph}
        , m_चिह्नकम्{चिह्नकम्}
        , m_externalTriggers{externalTriggers} {

        // Assign vertices to their sensitivity domains
        processDomains();
        यदि (dumpGraphLevel()) m_graph.dumpDotFilePrefixed(m_चिह्नकम् + "_orderg_domain");

        // Delete logic that is never triggered
        प्रति (क्रमः_Logic_Vertex* ध्रुव lVtxp : m_untriggeredps) {
            UASSERT_OBJ(lVtxp->domainp()->triggers().रिक्तानि(), lVtxp,
                        "Deleted logic should not be triggered");
            lVtxp->nodep()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
            lVtxp->unlinkDelete(&m_graph);
        }
    }

    ~V3_क्रमः_Process_Domains() = स्वभाव;

सार्वजनिक:
    // Order the logic
    स्थिर शून्य apply(क्रमः_Graph& graph, ध्रुव string& चिह्नकम्,
                      ध्रुव V3_क्रमः::ExternalTriggersProvider& externalTriggers) {
        V3_क्रमः_Process_Domains{graph, चिह्नकम्, externalTriggers};
    }
};

शून्य V3_क्रमः::processDomains(क्रमः_Graph& graph,  //
                             ध्रुव std::string& चिह्नकम्,  //
                             ध्रुव ExternalTriggersProvider& externalTriggers) {
    V3_क्रमः_Process_Domains::apply(graph, चिह्नकम्, externalTriggers);
}
