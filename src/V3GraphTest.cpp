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
रक्षित:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    आभास शून्य runTest() = 0;  // Run the test
    आभास string name() = 0;  // Name of the test

    // Utilities
    शून्य dumpSelf() {
        यदि (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

सार्वजनिक:
    V3GraphTest() = स्वभाव;
    आभास ~V3GraphTest() = स्वभाव;

    शून्य run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

वर्ग V3GraphTestVertex VL_NOT_FINAL : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    अचर string m_name;

सार्वजनिक:
    V3GraphTestVertex(V3Graph* graphp, अचर string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = स्वभाव;
    // ACCESSORS
    string name() अचर override VL_MT_STABLE { फर्काऊ m_name; }
};

वर्ग V3GraphTestVarVertex final : सार्वजनिक V3GraphTestVertex {
सार्वजनिक:
    V3GraphTestVarVertex(V3Graph* graphp, अचर string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = स्वभाव;
    // ACCESSORS
    string dotColor() अचर override { फर्काऊ "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

वर्ग V3GraphTestStrong final : सार्वजनिक V3GraphTest {
सार्वजनिक:
    string name() override { फर्काऊ "strong"; }
    शून्य runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = नूतन V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = नूतन V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = नूतन V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = नूतन V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = नूतन V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = नूतन V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = नूतन V3GraphTestVarVertex{gp, "q"};
        नूतन V3GraphEdge{gp, i, a, 2, सत्य};
        नूतन V3GraphEdge{gp, a, b, 2, सत्य};
        नूतन V3GraphEdge{gp, b, g1, 2, सत्य};
        नूतन V3GraphEdge{gp, b, g2, 2, सत्य};
        नूतन V3GraphEdge{gp, b, g3, 2, सत्य};
        नूतन V3GraphEdge{gp, g1, a, 2, सत्य};
        नूतन V3GraphEdge{gp, g3, g2, 2, सत्य};
        नूतन V3GraphEdge{gp, g2, g3, 2, सत्य};
        नूतन V3GraphEdge{gp, g1, q, 2, सत्य};
        नूतन V3GraphEdge{gp, g2, q, 2, सत्य};
        नूतन V3GraphEdge{gp, g3, q, 2, सत्य};

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
    string name() override { फर्काऊ "acyc"; }
    शून्य runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = नूतन V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = नूतन V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = नूतन V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = नूतन V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = नूतन V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = नूतन V3GraphTestVarVertex{gp, "g3"};
        नूतन V3GraphEdge{gp, i, a, 2, सत्य};
        नूतन V3GraphEdge{gp, a, b, 2, सत्य};
        नूतन V3GraphEdge{gp, b, g1, 2, सत्य};
        नूतन V3GraphEdge{gp, b, g2, 2, सत्य};
        नूतन V3GraphEdge{gp, b, g3, 2, सत्य};
        नूतन V3GraphEdge{gp, g1, a, 2, सत्य};
        नूतन V3GraphEdge{gp, g2, a, 2, सत्य};
        नूतन V3GraphEdge{gp, g3, a, 2, सत्य};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

वर्ग V3GraphTestVars final : सार्वजनिक V3GraphTest {
सार्वजनिक:
    string name() override { फर्काऊ "vars"; }
    शून्य runTest() override {
        V3Graph* अचर gp = &m_graph;

        V3GraphTestVertex* अचर clk = नूतन V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* अचर a = नूतन V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* अचर a_dly = नूतन V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* अचर a_dlyblk = नूतन V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* अचर b = नूतन V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* अचर b_dly = नूतन V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* अचर b_dlyblk = नूतन V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* अचर c = नूतन V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* अचर i = नूतन V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* अचर ap = नूतन V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* अचर bp = नूतन V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* अचर cp = नूतन V3GraphTestVarVertex{gp, "$c_pre"};

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

        n = नूतन V3GraphTestVertex{gp, "*INPUTS*"};
        {
            नूतन V3GraphEdge{gp, n, clk, 2};
            नूतन V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* अचर posedge = n = नूतन V3GraphTestVertex{gp, "*posedge clk*"};
        { नूतन V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = नूतन V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            नूतन V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            नूतन V3GraphEdge{gp, n, a_dly, 2};
            नूतन V3GraphEdge{gp, ap, n, 2, सत्य};  // DESIRED delayed ordering (inp is required)
            नूतन V3GraphEdge{gp, posedge, n, 2};
        }
        n = नूतन V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            नूतन V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            नूतन V3GraphEdge{gp, n, b_dly, 2};
            नूतन V3GraphEdge{gp, bp, n, 2, सत्य};  // DESIRED delayed ordering
            नूतन V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = नूतन V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            नूतन V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            नूतन V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            नूतन V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            नूतन V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            नूतन V3GraphEdge{gp, posedge, n, 2};
        }
        n = नूतन V3GraphTestVertex{gp, "b_dly<=a"};
        {
            नूतन V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            नूतन V3GraphEdge{gp, n, b_dly, 2};
            नूतन V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            नूतन V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = नूतन V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            नूतन V3GraphEdge{gp, n, a, 3};
            नूतन V3GraphEdge{gp, a_dly, n, 3};
            नूतन V3GraphEdge{gp, posedge, n, 2};
        }
        n = नूतन V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            नूतन V3GraphEdge{gp, n, b, 3};
            नूतन V3GraphEdge{gp, b_dly, n, 3};
            नूतन V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = नूतन V3GraphTestVertex{gp, "c=a|b|i"};
            नूतन V3GraphEdge{gp, n, c, 1, सत्य};
            नूतन V3GraphEdge{gp, a, n, 1, असत्य};
            नूतन V3GraphEdge{gp, b, n, 1, असत्य};
            नूतन V3GraphEdge{gp, i, n, 1, असत्य};
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
    string name() override { फर्काऊ "import"; }
    शून्य runTest() override {
        V3Graph* अचर gp = &m_graph;
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
