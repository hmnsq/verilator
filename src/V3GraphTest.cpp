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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

ವರ್ಗ V3GraphTest VL_NOT_FINAL {
ರಕ್ಷಿತ:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    ಆಭಾಸ ಶೂನ್ಯ runTest() = 0;  // Run the test
    ಆಭಾಸ string name() = 0;  // Name of the test

    // Utilities
    ಶೂನ್ಯ dumpSelf() {
        ಯದಿ (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

ಸಾರ್ವಜನಿಕ:
    V3GraphTest() = ಸ್ವಭಾವ;
    ಆಭಾಸ ~V3GraphTest() = ಸ್ವಭಾವ;

    ಶೂನ್ಯ run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

ವರ್ಗ V3GraphTestVertex VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    ಧ್ರುವ string m_name;

ಸಾರ್ವಜನಿಕ:
    V3GraphTestVertex(V3Graph* graphp, ಧ್ರುವ string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = ಸ್ವಭಾವ;
    // ACCESSORS
    string name() ಧ್ರುವ override VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ m_name; }
};

ವರ್ಗ V3GraphTestVarVertex final : ಸಾರ್ವಜನಿಕ V3GraphTestVertex {
ಸಾರ್ವಜನಿಕ:
    V3GraphTestVarVertex(V3Graph* graphp, ಧ್ರುವ string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = ಸ್ವಭಾವ;
    // ACCESSORS
    string dotColor() ಧ್ರುವ override { ಪ್ರತ್ಯಾಗಮ "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

ವರ್ಗ V3GraphTestStrong final : ಸಾರ್ವಜನಿಕ V3GraphTest {
ಸಾರ್ವಜನಿಕ:
    string name() override { ಪ್ರತ್ಯಾಗಮ "strong"; }
    ಶೂನ್ಯ runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = ನೂತನ V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = ನೂತನ V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = ನೂತನ V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = ನೂತನ V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = ನೂತನ V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = ನೂತನ V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = ನೂತನ V3GraphTestVarVertex{gp, "q"};
        ನೂತನ V3GraphEdge{gp, i, a, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, a, b, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, b, g1, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, b, g2, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, b, g3, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g1, a, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g3, g2, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g2, g3, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g1, q, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g2, q, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g3, q, 2, ಸತ್ಯ};

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

ವರ್ಗ V3GraphTestAcyc final : ಸಾರ್ವಜನಿಕ V3GraphTest {
ಸಾರ್ವಜನಿಕ:
    string name() override { ಪ್ರತ್ಯಾಗಮ "acyc"; }
    ಶೂನ್ಯ runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = ನೂತನ V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = ನೂತನ V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = ನೂತನ V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = ನೂತನ V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = ನೂತನ V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = ನೂತನ V3GraphTestVarVertex{gp, "g3"};
        ನೂತನ V3GraphEdge{gp, i, a, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, a, b, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, b, g1, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, b, g2, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, b, g3, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g1, a, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g2, a, 2, ಸತ್ಯ};
        ನೂತನ V3GraphEdge{gp, g3, a, 2, ಸತ್ಯ};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

ವರ್ಗ V3GraphTestVars final : ಸಾರ್ವಜನಿಕ V3GraphTest {
ಸಾರ್ವಜನಿಕ:
    string name() override { ಪ್ರತ್ಯಾಗಮ "vars"; }
    ಶೂನ್ಯ runTest() override {
        V3Graph* ಧ್ರುವ gp = &m_graph;

        V3GraphTestVertex* ಧ್ರುವ clk = ನೂತನ V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* ಧ್ರುವ a = ನೂತನ V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* ಧ್ರುವ a_dly = ನೂತನ V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* ಧ್ರುವ a_dlyblk = ನೂತನ V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* ಧ್ರುವ b = ನೂತನ V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* ಧ್ರುವ b_dly = ನೂತನ V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* ಧ್ರುವ b_dlyblk = ನೂತನ V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* ಧ್ರುವ c = ನೂತನ V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* ಧ್ರುವ i = ನೂತನ V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* ಧ್ರುವ ap = ನೂತನ V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* ಧ್ರುವ bp = ನೂತನ V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* ಧ್ರುವ cp = ನೂತನ V3GraphTestVarVertex{gp, "$c_pre"};

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

        n = ನೂತನ V3GraphTestVertex{gp, "*INPUTS*"};
        {
            ನೂತನ V3GraphEdge{gp, n, clk, 2};
            ನೂತನ V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* ಧ್ರುವ posedge = n = ನೂತನ V3GraphTestVertex{gp, "*posedge clk*"};
        { ನೂತನ V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = ನೂತನ V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            ನೂತನ V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            ನೂತನ V3GraphEdge{gp, n, a_dly, 2};
            ನೂತನ V3GraphEdge{gp, ap, n, 2, ಸತ್ಯ};  // DESIRED delayed ordering (inp is required)
            ನೂತನ V3GraphEdge{gp, posedge, n, 2};
        }
        n = ನೂತನ V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            ನೂತನ V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            ನೂತನ V3GraphEdge{gp, n, b_dly, 2};
            ನೂತನ V3GraphEdge{gp, bp, n, 2, ಸತ್ಯ};  // DESIRED delayed ordering
            ನೂತನ V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = ನೂತನ V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            ನೂತನ V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            ನೂತನ V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            ನೂತನ V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            ನೂತನ V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            ನೂತನ V3GraphEdge{gp, posedge, n, 2};
        }
        n = ನೂತನ V3GraphTestVertex{gp, "b_dly<=a"};
        {
            ನೂತನ V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            ನೂತನ V3GraphEdge{gp, n, b_dly, 2};
            ನೂತನ V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            ನೂತನ V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = ನೂತನ V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            ನೂತನ V3GraphEdge{gp, n, a, 3};
            ನೂತನ V3GraphEdge{gp, a_dly, n, 3};
            ನೂತನ V3GraphEdge{gp, posedge, n, 2};
        }
        n = ನೂತನ V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            ನೂತನ V3GraphEdge{gp, n, b, 3};
            ನೂತನ V3GraphEdge{gp, b_dly, n, 3};
            ನೂತನ V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = ನೂತನ V3GraphTestVertex{gp, "c=a|b|i"};
            ನೂತನ V3GraphEdge{gp, n, c, 1, ಸತ್ಯ};
            ನೂತನ V3GraphEdge{gp, a, n, 1, ಅಸತ್ಯ};
            ನೂತನ V3GraphEdge{gp, b, n, 1, ಅಸತ್ಯ};
            ನೂತನ V3GraphEdge{gp, i, n, 1, ಅಸತ್ಯ};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::cout, *ap);

        dumpSelf();
    }
};

//======================================================================

ವರ್ಗ V3GraphTestImport final : ಸಾರ್ವಜನಿಕ V3GraphTest {

#ifdef GRAPH_IMPORT
    ಶೂನ್ಯ dotImport();
#ಅನ್ಯಥಾ
    ಶೂನ್ಯ dotImport() {}
#endif

ಸಾರ್ವಜನಿಕ:
    string name() override { ಪ್ರತ್ಯಾಗಮ "import"; }
    ಶೂನ್ಯ runTest() override {
        V3Graph* ಧ್ರುವ gp = &m_graph;
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

ಶೂನ್ಯ V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
