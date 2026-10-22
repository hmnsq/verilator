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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

വർഗ്ഗം V3GraphTest VL_NOT_FINAL {
സംരക്ഷിത:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    മായാ ശൂന്യം runTest() = 0;  // Run the test
    മായാ string name() = 0;  // Name of the test

    // Utilities
    ശൂന്യം dumpSelf() {
        എങ്കിൽ (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

പൊതു:
    V3GraphTest() = സ്ഥിരസ്ഥിതി;
    മായാ ~V3GraphTest() = സ്ഥിരസ്ഥിതി;

    ശൂന്യം run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

വർഗ്ഗം V3GraphTestVertex VL_NOT_FINAL : പൊതു V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    സ്ഥിരം string m_name;

പൊതു:
    V3GraphTestVertex(V3Graph* graphp, സ്ഥിരം string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    string name() സ്ഥിരം override VL_MT_STABLE { തിരികെ m_name; }
};

വർഗ്ഗം V3GraphTestVarVertex final : പൊതു V3GraphTestVertex {
പൊതു:
    V3GraphTestVarVertex(V3Graph* graphp, സ്ഥിരം string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = സ്ഥിരസ്ഥിതി;
    // ACCESSORS
    string dotColor() സ്ഥിരം override { തിരികെ "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

വർഗ്ഗം V3GraphTestStrong final : പൊതു V3GraphTest {
പൊതു:
    string name() override { തിരികെ "strong"; }
    ശൂന്യം runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = പുതിയ V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = പുതിയ V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = പുതിയ V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = പുതിയ V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = പുതിയ V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = പുതിയ V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = പുതിയ V3GraphTestVarVertex{gp, "q"};
        പുതിയ V3GraphEdge{gp, i, a, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, a, b, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, b, g1, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, b, g2, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, b, g3, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g1, a, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g3, g2, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g2, g3, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g1, q, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g2, q, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g3, q, 2, സത്യം};

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

വർഗ്ഗം V3GraphTestAcyc final : പൊതു V3GraphTest {
പൊതു:
    string name() override { തിരികെ "acyc"; }
    ശൂന്യം runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = പുതിയ V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = പുതിയ V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = പുതിയ V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = പുതിയ V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = പുതിയ V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = പുതിയ V3GraphTestVarVertex{gp, "g3"};
        പുതിയ V3GraphEdge{gp, i, a, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, a, b, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, b, g1, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, b, g2, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, b, g3, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g1, a, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g2, a, 2, സത്യം};
        പുതിയ V3GraphEdge{gp, g3, a, 2, സത്യം};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

വർഗ്ഗം V3GraphTestVars final : പൊതു V3GraphTest {
പൊതു:
    string name() override { തിരികെ "vars"; }
    ശൂന്യം runTest() override {
        V3Graph* സ്ഥിരം gp = &m_graph;

        V3GraphTestVertex* സ്ഥിരം clk = പുതിയ V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* സ്ഥിരം a = പുതിയ V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* സ്ഥിരം a_dly = പുതിയ V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* സ്ഥിരം a_dlyblk = പുതിയ V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* സ്ഥിരം b = പുതിയ V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* സ്ഥിരം b_dly = പുതിയ V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* സ്ഥിരം b_dlyblk = പുതിയ V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* സ്ഥിരം c = പുതിയ V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* സ്ഥിരം i = പുതിയ V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* സ്ഥിരം ap = പുതിയ V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* സ്ഥിരം bp = പുതിയ V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* സ്ഥിരം cp = പുതിയ V3GraphTestVarVertex{gp, "$c_pre"};

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

        n = പുതിയ V3GraphTestVertex{gp, "*INPUTS*"};
        {
            പുതിയ V3GraphEdge{gp, n, clk, 2};
            പുതിയ V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* സ്ഥിരം posedge = n = പുതിയ V3GraphTestVertex{gp, "*posedge clk*"};
        { പുതിയ V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = പുതിയ V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            പുതിയ V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            പുതിയ V3GraphEdge{gp, n, a_dly, 2};
            പുതിയ V3GraphEdge{gp, ap, n, 2, സത്യം};  // DESIRED delayed ordering (inp is required)
            പുതിയ V3GraphEdge{gp, posedge, n, 2};
        }
        n = പുതിയ V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            പുതിയ V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            പുതിയ V3GraphEdge{gp, n, b_dly, 2};
            പുതിയ V3GraphEdge{gp, bp, n, 2, സത്യം};  // DESIRED delayed ordering
            പുതിയ V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = പുതിയ V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            പുതിയ V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            പുതിയ V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            പുതിയ V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            പുതിയ V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            പുതിയ V3GraphEdge{gp, posedge, n, 2};
        }
        n = പുതിയ V3GraphTestVertex{gp, "b_dly<=a"};
        {
            പുതിയ V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            പുതിയ V3GraphEdge{gp, n, b_dly, 2};
            പുതിയ V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            പുതിയ V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = പുതിയ V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            പുതിയ V3GraphEdge{gp, n, a, 3};
            പുതിയ V3GraphEdge{gp, a_dly, n, 3};
            പുതിയ V3GraphEdge{gp, posedge, n, 2};
        }
        n = പുതിയ V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            പുതിയ V3GraphEdge{gp, n, b, 3};
            പുതിയ V3GraphEdge{gp, b_dly, n, 3};
            പുതിയ V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = പുതിയ V3GraphTestVertex{gp, "c=a|b|i"};
            പുതിയ V3GraphEdge{gp, n, c, 1, സത്യം};
            പുതിയ V3GraphEdge{gp, a, n, 1, അസത്യം};
            പുതിയ V3GraphEdge{gp, b, n, 1, അസത്യം};
            പുതിയ V3GraphEdge{gp, i, n, 1, അസത്യം};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::cout, *ap);

        dumpSelf();
    }
};

//======================================================================

വർഗ്ഗം V3GraphTestImport final : പൊതു V3GraphTest {

#ifdef GRAPH_IMPORT
    ശൂന്യം dotImport();
#അല്ലെങ്കിൽ
    ശൂന്യം dotImport() {}
#endif

പൊതു:
    string name() override { തിരികെ "import"; }
    ശൂന്യം runTest() override {
        V3Graph* സ്ഥിരം gp = &m_graph;
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

ശൂന്യം V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
