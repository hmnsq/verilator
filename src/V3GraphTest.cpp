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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

વર્ગ V3GraphTest VL_NOT_FINAL {
સુરક્ષિત:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    આભાસી શૂન્ય runTest() = 0;  // Run the test
    આભાસી string name() = 0;  // Name of the test

    // Utilities
    શૂન્ય dumpSelf() {
        જો (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

જાહેર:
    V3GraphTest() = મૂળભૂત;
    આભાસી ~V3GraphTest() = મૂળભૂત;

    શૂન્ય run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

વર્ગ V3GraphTestVertex VL_NOT_FINAL : જાહેર V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    અચલ string m_name;

જાહેર:
    V3GraphTestVertex(V3Graph* graphp, અચલ string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = મૂળભૂત;
    // ACCESSORS
    string name() અચલ override VL_MT_STABLE { પાછા m_name; }
};

વર્ગ V3GraphTestVarVertex final : જાહેર V3GraphTestVertex {
જાહેર:
    V3GraphTestVarVertex(V3Graph* graphp, અચલ string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = મૂળભૂત;
    // ACCESSORS
    string dotColor() અચલ override { પાછા "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

વર્ગ V3GraphTestStrong final : જાહેર V3GraphTest {
જાહેર:
    string name() override { પાછા "strong"; }
    શૂન્ય runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = નવું V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = નવું V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = નવું V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = નવું V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = નવું V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = નવું V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = નવું V3GraphTestVarVertex{gp, "q"};
        નવું V3GraphEdge{gp, i, a, 2, સાચું};
        નવું V3GraphEdge{gp, a, b, 2, સાચું};
        નવું V3GraphEdge{gp, b, g1, 2, સાચું};
        નવું V3GraphEdge{gp, b, g2, 2, સાચું};
        નવું V3GraphEdge{gp, b, g3, 2, સાચું};
        નવું V3GraphEdge{gp, g1, a, 2, સાચું};
        નવું V3GraphEdge{gp, g3, g2, 2, સાચું};
        નવું V3GraphEdge{gp, g2, g3, 2, સાચું};
        નવું V3GraphEdge{gp, g1, q, 2, સાચું};
        નવું V3GraphEdge{gp, g2, q, 2, સાચું};
        નવું V3GraphEdge{gp, g3, q, 2, સાચું};

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

વર્ગ V3GraphTestAcyc final : જાહેર V3GraphTest {
જાહેર:
    string name() override { પાછા "acyc"; }
    શૂન્ય runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = નવું V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = નવું V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = નવું V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = નવું V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = નવું V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = નવું V3GraphTestVarVertex{gp, "g3"};
        નવું V3GraphEdge{gp, i, a, 2, સાચું};
        નવું V3GraphEdge{gp, a, b, 2, સાચું};
        નવું V3GraphEdge{gp, b, g1, 2, સાચું};
        નવું V3GraphEdge{gp, b, g2, 2, સાચું};
        નવું V3GraphEdge{gp, b, g3, 2, સાચું};
        નવું V3GraphEdge{gp, g1, a, 2, સાચું};
        નવું V3GraphEdge{gp, g2, a, 2, સાચું};
        નવું V3GraphEdge{gp, g3, a, 2, સાચું};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

વર્ગ V3GraphTestVars final : જાહેર V3GraphTest {
જાહેર:
    string name() override { પાછા "vars"; }
    શૂન્ય runTest() override {
        V3Graph* અચલ gp = &m_graph;

        V3GraphTestVertex* અચલ clk = નવું V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* અચલ a = નવું V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* અચલ a_dly = નવું V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* અચલ a_dlyblk = નવું V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* અચલ b = નવું V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* અચલ b_dly = નવું V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* અચલ b_dlyblk = નવું V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* અચલ c = નવું V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* અચલ i = નવું V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* અચલ ap = નવું V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* અચલ bp = નવું V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* અચલ cp = નવું V3GraphTestVarVertex{gp, "$c_pre"};

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

        n = નવું V3GraphTestVertex{gp, "*INPUTS*"};
        {
            નવું V3GraphEdge{gp, n, clk, 2};
            નવું V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* અચલ posedge = n = નવું V3GraphTestVertex{gp, "*posedge clk*"};
        { નવું V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = નવું V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            નવું V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            નવું V3GraphEdge{gp, n, a_dly, 2};
            નવું V3GraphEdge{gp, ap, n, 2, સાચું};  // DESIRED delayed ordering (inp is required)
            નવું V3GraphEdge{gp, posedge, n, 2};
        }
        n = નવું V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            નવું V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            નવું V3GraphEdge{gp, n, b_dly, 2};
            નવું V3GraphEdge{gp, bp, n, 2, સાચું};  // DESIRED delayed ordering
            નવું V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = નવું V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            નવું V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            નવું V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            નવું V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            નવું V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            નવું V3GraphEdge{gp, posedge, n, 2};
        }
        n = નવું V3GraphTestVertex{gp, "b_dly<=a"};
        {
            નવું V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            નવું V3GraphEdge{gp, n, b_dly, 2};
            નવું V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            નવું V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = નવું V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            નવું V3GraphEdge{gp, n, a, 3};
            નવું V3GraphEdge{gp, a_dly, n, 3};
            નવું V3GraphEdge{gp, posedge, n, 2};
        }
        n = નવું V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            નવું V3GraphEdge{gp, n, b, 3};
            નવું V3GraphEdge{gp, b_dly, n, 3};
            નવું V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = નવું V3GraphTestVertex{gp, "c=a|b|i"};
            નવું V3GraphEdge{gp, n, c, 1, સાચું};
            નવું V3GraphEdge{gp, a, n, 1, ખોટું};
            નવું V3GraphEdge{gp, b, n, 1, ખોટું};
            નવું V3GraphEdge{gp, i, n, 1, ખોટું};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::cout, *ap);

        dumpSelf();
    }
};

//======================================================================

વર્ગ V3GraphTestImport final : જાહેર V3GraphTest {

#ifdef GRAPH_IMPORT
    શૂન્ય dotImport();
#નહીંતર
    શૂન્ય dotImport() {}
#endif

જાહેર:
    string name() override { પાછા "import"; }
    શૂન્ય runTest() override {
        V3Graph* અચલ gp = &m_graph;
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

શૂન્ય V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
