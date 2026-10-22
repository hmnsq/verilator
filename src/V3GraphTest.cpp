// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph tests
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

#define VL_MT_DISABLED_CODE_UNIT 1

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

वर्ग V3GraphTest VL_NOT_FINAL {
संरक्षित:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    आभासी शून्य runTest() = 0;  // Run the test
    आभासी string name() = 0;  // Name of the test

    // Utilities
    शून्य dumpSelf() {
        जर (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

सार्वजनिक:
    V3GraphTest() = पूर्वनिर्धारित;
    आभासी ~V3GraphTest() = पूर्वनिर्धारित;

    शून्य run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

वर्ग V3GraphTestVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    स्थिरांक string m_name;

सार्वजनिक:
    V3GraphTestVertex(V3Graph* graphp, स्थिरांक string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = पूर्वनिर्धारित;
    // ACCESSORS
    string name() स्थिरांक override VL_MT_STABLE { परत m_name; }
};

वर्ग V3GraphTestVarVertex final : सार्वजनिक V3GraphTestVertex {
सार्वजनिक:
    V3GraphTestVarVertex(V3Graph* graphp, स्थिरांक string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = पूर्वनिर्धारित;
    // ACCESSORS
    string dotColor() स्थिरांक override { परत "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

वर्ग V3GraphTestStrong final : सार्वजनिक V3GraphTest {
सार्वजनिक:
    string name() override { परत "strong"; }
    शून्य runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = नवीन V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = नवीन V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = नवीन V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = नवीन V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = नवीन V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = नवीन V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = नवीन V3GraphTestVarVertex{gp, "q"};
        नवीन V3GraphEdge{gp, i, a, 2, सत्य};
        नवीन V3GraphEdge{gp, a, b, 2, सत्य};
        नवीन V3GraphEdge{gp, b, g1, 2, सत्य};
        नवीन V3GraphEdge{gp, b, g2, 2, सत्य};
        नवीन V3GraphEdge{gp, b, g3, 2, सत्य};
        नवीन V3GraphEdge{gp, g1, a, 2, सत्य};
        नवीन V3GraphEdge{gp, g3, g2, 2, सत्य};
        नवीन V3GraphEdge{gp, g2, g3, 2, सत्य};
        नवीन V3GraphEdge{gp, g1, q, 2, सत्य};
        नवीन V3GraphEdge{gp, g2, q, 2, सत्य};
        नवीन V3GraphEdge{gp, g3, q, 2, सत्य};

        gp->stronglyConnected(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

वर्ग V3GraphTestAcyc final : सार्वजनिक V3GraphTest {
सार्वजनिक:
    string name() override { परत "acyc"; }
    शून्य runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = नवीन V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = नवीन V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = नवीन V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = नवीन V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = नवीन V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = नवीन V3GraphTestVarVertex{gp, "g3"};
        नवीन V3GraphEdge{gp, i, a, 2, सत्य};
        नवीन V3GraphEdge{gp, a, b, 2, सत्य};
        नवीन V3GraphEdge{gp, b, g1, 2, सत्य};
        नवीन V3GraphEdge{gp, b, g2, 2, सत्य};
        नवीन V3GraphEdge{gp, b, g3, 2, सत्य};
        नवीन V3GraphEdge{gp, g1, a, 2, सत्य};
        नवीन V3GraphEdge{gp, g2, a, 2, सत्य};
        नवीन V3GraphEdge{gp, g3, a, 2, सत्य};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

वर्ग V3GraphTestVars final : सार्वजनिक V3GraphTest {
सार्वजनिक:
    string name() override { परत "vars"; }
    शून्य runTest() override {
        V3Graph* स्थिरांक gp = &m_graph;

        V3GraphTestVertex* स्थिरांक clk = नवीन V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* स्थिरांक a = नवीन V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* स्थिरांक a_dly = नवीन V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* स्थिरांक a_dlyblk = नवीन V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* स्थिरांक b = नवीन V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* स्थिरांक b_dly = नवीन V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* स्थिरांक b_dlyblk = नवीन V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* स्थिरांक c = नवीन V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* स्थिरांक i = नवीन V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* स्थिरांक ap = नवीन V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* स्थिरांक bp = नवीन V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* स्थिरांक cp = नवीन V3GraphTestVarVertex{gp, "$c_pre"};

        V3GraphTestVertex* n;

        // Logical order between clk, and posedge blocks
        //   implemented by special CLK prod/cons?
        // Required order between first x_DLY<=x_pre and final x<=x_DLY
        //   implemented by producer/consumer on a_dly signals
        // Required order between first x_DLY<=x_pre and x_DLY<=setters
        //   implemented by fake dependency on _dlyblk
        // Required order between x_DLY<=setters and final x<=x_DLY
        //   implemented by producer/consumer on a_dly signals
        // Desired order between different _DLY blocks so we can elim temporaries
        //   implemented by cutable "pre" signal dependencies

        n = नवीन V3GraphTestVertex{gp, "*INPUTS*"};
        {
            नवीन V3GraphEdge{gp, n, clk, 2};
            नवीन V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* स्थिरांक posedge = n = नवीन V3GraphTestVertex{gp, "*posedge clk*"};
        { नवीन V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = नवीन V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            नवीन V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            नवीन V3GraphEdge{gp, n, a_dly, 2};
            नवीन V3GraphEdge{gp, ap, n, 2, सत्य};  // DESIRED delayed ordering (inp is required)
            नवीन V3GraphEdge{gp, posedge, n, 2};
        }
        n = नवीन V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            नवीन V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            नवीन V3GraphEdge{gp, n, b_dly, 2};
            नवीन V3GraphEdge{gp, bp, n, 2, सत्य};  // DESIRED delayed ordering
            नवीन V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = नवीन V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            नवीन V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            नवीन V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            नवीन V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            नवीन V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            नवीन V3GraphEdge{gp, posedge, n, 2};
        }
        n = नवीन V3GraphTestVertex{gp, "b_dly<=a"};
        {
            नवीन V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            नवीन V3GraphEdge{gp, n, b_dly, 2};
            नवीन V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            नवीन V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = नवीन V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            नवीन V3GraphEdge{gp, n, a, 3};
            नवीन V3GraphEdge{gp, a_dly, n, 3};
            नवीन V3GraphEdge{gp, posedge, n, 2};
        }
        n = नवीन V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            नवीन V3GraphEdge{gp, n, b, 3};
            नवीन V3GraphEdge{gp, b_dly, n, 3};
            नवीन V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = नवीन V3GraphTestVertex{gp, "c=a|b|i"};
            नवीन V3GraphEdge{gp, n, c, 1, सत्य};
            नवीन V3GraphEdge{gp, a, n, 1, असत्य};
            नवीन V3GraphEdge{gp, b, n, 1, असत्य};
            नवीन V3GraphEdge{gp, i, n, 1, असत्य};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::cout, *ap);

        dumpSelf();
    }
};

//======================================================================

वर्ग V3GraphTestImport final : सार्वजनिक V3GraphTest {

#ifdef GRAPH_IMPORT
    शून्य dotImport();
#अन्यथा
    शून्य dotImport() {}
#endif

सार्वजनिक:
    string name() override { परत "import"; }
    शून्य runTest() override {
        V3Graph* स्थिरांक gp = &m_graph;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#ifdef GRAPH_IMPORT
# include "graph_export.cpp"
#endif
// clang-format on

//======================================================================

शून्य V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
