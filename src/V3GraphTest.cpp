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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Global.h"
#অন্তর্ভাব "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

বর্গ V3GraphTest VL_NOT_FINAL {
রক্ষিত:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    আভাস শূন্য runTest() = 0;  // Run the test
    আভাস string name() = 0;  // Name of the test

    // Utilities
    শূন্য dumpSelf() {
        যদি (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

সার্বজনিক:
    V3GraphTest() = স্বভাব;
    আভাস ~V3GraphTest() = স্বভাব;

    শূন্য run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

বর্গ V3GraphTestVertex VL_NOT_FINAL : সার্বজনিক V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    ধ্রুব string m_name;

সার্বজনিক:
    V3GraphTestVertex(V3Graph* graphp, ধ্রুব string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = স্বভাব;
    // ACCESSORS
    string name() ধ্রুব override VL_MT_STABLE { প্রত্যাগম m_name; }
};

বর্গ V3GraphTestVarVertex final : সার্বজনিক V3GraphTestVertex {
সার্বজনিক:
    V3GraphTestVarVertex(V3Graph* graphp, ধ্রুব string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = স্বভাব;
    // ACCESSORS
    string dotColor() ধ্রুব override { প্রত্যাগম "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

বর্গ V3GraphTestStrong final : সার্বজনিক V3GraphTest {
সার্বজনিক:
    string name() override { প্রত্যাগম "strong"; }
    শূন্য runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = নূতন V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = নূতন V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = নূতন V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = নূতন V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = নূতন V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = নূতন V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = নূতন V3GraphTestVarVertex{gp, "q"};
        নূতন V3GraphEdge{gp, i, a, 2, সত্য};
        নূতন V3GraphEdge{gp, a, b, 2, সত্য};
        নূতন V3GraphEdge{gp, b, g1, 2, সত্য};
        নূতন V3GraphEdge{gp, b, g2, 2, সত্য};
        নূতন V3GraphEdge{gp, b, g3, 2, সত্য};
        নূতন V3GraphEdge{gp, g1, a, 2, সত্য};
        নূতন V3GraphEdge{gp, g3, g2, 2, সত্য};
        নূতন V3GraphEdge{gp, g2, g3, 2, সত্য};
        নূতন V3GraphEdge{gp, g1, q, 2, সত্য};
        নূতন V3GraphEdge{gp, g2, q, 2, সত্য};
        নূতন V3GraphEdge{gp, g3, q, 2, সত্য};

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

বর্গ V3GraphTestAcyc final : সার্বজনিক V3GraphTest {
সার্বজনিক:
    string name() override { প্রত্যাগম "acyc"; }
    শূন্য runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = নূতন V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = নূতন V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = নূতন V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = নূতন V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = নূতন V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = নূতন V3GraphTestVarVertex{gp, "g3"};
        নূতন V3GraphEdge{gp, i, a, 2, সত্য};
        নূতন V3GraphEdge{gp, a, b, 2, সত্য};
        নূতন V3GraphEdge{gp, b, g1, 2, সত্য};
        নূতন V3GraphEdge{gp, b, g2, 2, সত্য};
        নূতন V3GraphEdge{gp, b, g3, 2, সত্য};
        নূতন V3GraphEdge{gp, g1, a, 2, সত্য};
        নূতন V3GraphEdge{gp, g2, a, 2, সত্য};
        নূতন V3GraphEdge{gp, g3, a, 2, সত্য};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

বর্গ V3GraphTestVars final : সার্বজনিক V3GraphTest {
সার্বজনিক:
    string name() override { প্রত্যাগম "vars"; }
    শূন্য runTest() override {
        V3Graph* ধ্রুব gp = &m_graph;

        V3GraphTestVertex* ধ্রুব clk = নূতন V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* ধ্রুব a = নূতন V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* ধ্রুব a_dly = নূতন V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* ধ্রুব a_dlyblk = নূতন V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* ধ্রুব b = নূতন V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* ধ্রুব b_dly = নূতন V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* ধ্রুব b_dlyblk = নূতন V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* ধ্রুব c = নূতন V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* ধ্রুব i = নূতন V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* ধ্রুব ap = নূতন V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* ধ্রুব bp = নূতন V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* ধ্রুব cp = নূতন V3GraphTestVarVertex{gp, "$c_pre"};

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

        n = নূতন V3GraphTestVertex{gp, "*INPUTS*"};
        {
            নূতন V3GraphEdge{gp, n, clk, 2};
            নূতন V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* ধ্রুব posedge = n = নূতন V3GraphTestVertex{gp, "*posedge clk*"};
        { নূতন V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = নূতন V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            নূতন V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            নূতন V3GraphEdge{gp, n, a_dly, 2};
            নূতন V3GraphEdge{gp, ap, n, 2, সত্য};  // DESIRED delayed ordering (inp is required)
            নূতন V3GraphEdge{gp, posedge, n, 2};
        }
        n = নূতন V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            নূতন V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            নূতন V3GraphEdge{gp, n, b_dly, 2};
            নূতন V3GraphEdge{gp, bp, n, 2, সত্য};  // DESIRED delayed ordering
            নূতন V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = নূতন V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            নূতন V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            নূতন V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            নূতন V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            নূতন V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            নূতন V3GraphEdge{gp, posedge, n, 2};
        }
        n = নূতন V3GraphTestVertex{gp, "b_dly<=a"};
        {
            নূতন V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            নূতন V3GraphEdge{gp, n, b_dly, 2};
            নূতন V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            নূতন V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = নূতন V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            নূতন V3GraphEdge{gp, n, a, 3};
            নূতন V3GraphEdge{gp, a_dly, n, 3};
            নূতন V3GraphEdge{gp, posedge, n, 2};
        }
        n = নূতন V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            নূতন V3GraphEdge{gp, n, b, 3};
            নূতন V3GraphEdge{gp, b_dly, n, 3};
            নূতন V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = নূতন V3GraphTestVertex{gp, "c=a|b|i"};
            নূতন V3GraphEdge{gp, n, c, 1, সত্য};
            নূতন V3GraphEdge{gp, a, n, 1, অসত্য};
            নূতন V3GraphEdge{gp, b, n, 1, অসত্য};
            নূতন V3GraphEdge{gp, i, n, 1, অসত্য};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::cout, *ap);

        dumpSelf();
    }
};

//======================================================================

বর্গ V3GraphTestImport final : সার্বজনিক V3GraphTest {

#ifdef GRAPH_IMPORT
    শূন্য dotImport();
#অন্যথা
    শূন্য dotImport() {}
#endif

সার্বজনিক:
    string name() override { প্রত্যাগম "import"; }
    শূন্য runTest() override {
        V3Graph* ধ্রুব gp = &m_graph;
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

শূন্য V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
