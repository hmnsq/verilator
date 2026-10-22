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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

ଶ୍ରେଣୀ V3GraphTest VL_NOT_FINAL {
ସୁରକ୍ଷିତ:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    ଆଭାସୀ ଶୂନ୍ୟ runTest() = 0;  // Run the test
    ଆଭାସୀ string name() = 0;  // Name of the test

    // Utilities
    ଶୂନ୍ୟ dumpSelf() {
        ଯଦି (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

ସର୍ବସାଧାରଣ:
    V3GraphTest() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଆଭାସୀ ~V3GraphTest() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଶୂନ୍ୟ run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

ଶ୍ରେଣୀ V3GraphTestVertex VL_NOT_FINAL : ସର୍ବସାଧାରଣ V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    ଅଚଳ string m_name;

ସର୍ବସାଧାରଣ:
    V3GraphTestVertex(V3Graph* graphp, ଅଚଳ string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
};

ଶ୍ରେଣୀ V3GraphTestVarVertex final : ସର୍ବସାଧାରଣ V3GraphTestVertex {
ସର୍ବସାଧାରଣ:
    V3GraphTestVarVertex(V3Graph* graphp, ଅଚଳ string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // ACCESSORS
    string dotColor() ଅଚଳ override { ଫେରନ୍ତୁ "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

ଶ୍ରେଣୀ V3GraphTestStrong final : ସର୍ବସାଧାରଣ V3GraphTest {
ସର୍ବସାଧାରଣ:
    string name() override { ଫେରନ୍ତୁ "strong"; }
    ଶୂନ୍ୟ runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = ନୂତନ V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = ନୂତନ V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = ନୂତନ V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = ନୂତନ V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = ନୂତନ V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = ନୂତନ V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = ନୂତନ V3GraphTestVarVertex{gp, "q"};
        ନୂତନ V3GraphEdge{gp, i, a, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, a, b, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, b, g1, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, b, g2, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, b, g3, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g1, a, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g3, g2, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g2, g3, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g1, q, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g2, q, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g3, q, 2, ସତ୍ୟ};

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

ଶ୍ରେଣୀ V3GraphTestAcyc final : ସର୍ବସାଧାରଣ V3GraphTest {
ସର୍ବସାଧାରଣ:
    string name() override { ଫେରନ୍ତୁ "acyc"; }
    ଶୂନ୍ୟ runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = ନୂତନ V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = ନୂତନ V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = ନୂତନ V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = ନୂତନ V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = ନୂତନ V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = ନୂତନ V3GraphTestVarVertex{gp, "g3"};
        ନୂତନ V3GraphEdge{gp, i, a, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, a, b, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, b, g1, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, b, g2, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, b, g3, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g1, a, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g2, a, 2, ସତ୍ୟ};
        ନୂତନ V3GraphEdge{gp, g3, a, 2, ସତ୍ୟ};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

ଶ୍ରେଣୀ V3GraphTestVars final : ସର୍ବସାଧାରଣ V3GraphTest {
ସର୍ବସାଧାରଣ:
    string name() override { ଫେରନ୍ତୁ "vars"; }
    ଶୂନ୍ୟ runTest() override {
        V3Graph* ଅଚଳ gp = &m_graph;

        V3GraphTestVertex* ଅଚଳ clk = ନୂତନ V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* ଅଚଳ a = ନୂତନ V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* ଅଚଳ a_dly = ନୂତନ V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* ଅଚଳ a_dlyblk = ନୂତନ V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* ଅଚଳ b = ନୂତନ V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* ଅଚଳ b_dly = ନୂତନ V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* ଅଚଳ b_dlyblk = ନୂତନ V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* ଅଚଳ c = ନୂତନ V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* ଅଚଳ i = ନୂତନ V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* ଅଚଳ ap = ନୂତନ V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* ଅଚଳ bp = ନୂତନ V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* ଅଚଳ cp = ନୂତନ V3GraphTestVarVertex{gp, "$c_pre"};

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

        n = ନୂତନ V3GraphTestVertex{gp, "*INPUTS*"};
        {
            ନୂତନ V3GraphEdge{gp, n, clk, 2};
            ନୂତନ V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* ଅଚଳ posedge = n = ନୂତନ V3GraphTestVertex{gp, "*posedge clk*"};
        { ନୂତନ V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = ନୂତନ V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            ନୂତନ V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            ନୂତନ V3GraphEdge{gp, n, a_dly, 2};
            ନୂତନ V3GraphEdge{gp, ap, n, 2, ସତ୍ୟ};  // DESIRED delayed ordering (inp is required)
            ନୂତନ V3GraphEdge{gp, posedge, n, 2};
        }
        n = ନୂତନ V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            ନୂତନ V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            ନୂତନ V3GraphEdge{gp, n, b_dly, 2};
            ନୂତନ V3GraphEdge{gp, bp, n, 2, ସତ୍ୟ};  // DESIRED delayed ordering
            ନୂତନ V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = ନୂତନ V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            ନୂତନ V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            ନୂତନ V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            ନୂତନ V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            ନୂତନ V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            ନୂତନ V3GraphEdge{gp, posedge, n, 2};
        }
        n = ନୂତନ V3GraphTestVertex{gp, "b_dly<=a"};
        {
            ନୂତନ V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            ନୂତନ V3GraphEdge{gp, n, b_dly, 2};
            ନୂତନ V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            ନୂତନ V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = ନୂତନ V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            ନୂତନ V3GraphEdge{gp, n, a, 3};
            ନୂତନ V3GraphEdge{gp, a_dly, n, 3};
            ନୂତନ V3GraphEdge{gp, posedge, n, 2};
        }
        n = ନୂତନ V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            ନୂତନ V3GraphEdge{gp, n, b, 3};
            ନୂତନ V3GraphEdge{gp, b_dly, n, 3};
            ନୂତନ V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = ନୂତନ V3GraphTestVertex{gp, "c=a|b|i"};
            ନୂତନ V3GraphEdge{gp, n, c, 1, ସତ୍ୟ};
            ନୂତନ V3GraphEdge{gp, a, n, 1, ମିଥ୍ୟା};
            ନୂତନ V3GraphEdge{gp, b, n, 1, ମିଥ୍ୟା};
            ନୂତନ V3GraphEdge{gp, i, n, 1, ମିଥ୍ୟା};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::cout, *ap);

        dumpSelf();
    }
};

//======================================================================

ଶ୍ରେଣୀ V3GraphTestImport final : ସର୍ବସାଧାରଣ V3GraphTest {

#ifdef GRAPH_IMPORT
    ଶୂନ୍ୟ dotImport();
#ଅନ୍ୟଥା
    ଶୂନ୍ୟ dotImport() {}
#endif

ସର୍ବସାଧାରଣ:
    string name() override { ଫେରନ୍ତୁ "import"; }
    ଶୂନ୍ୟ runTest() override {
        V3Graph* ଅଚଳ gp = &m_graph;
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

ଶୂନ୍ୟ V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
