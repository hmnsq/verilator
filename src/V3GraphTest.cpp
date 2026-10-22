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

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_MT_DISABLED_ਕੋਡ_UNIT 1

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

ਵਰਗ V3_ਗ੍ਰਾਫ_Test VL_NOT_FINAL {
ਸੁਰੱਖਿਅਤ:
    // MEMBERS
    V3_ਗ੍ਰਾਫ m_ਗ੍ਰਾਫ;

    // METHODS - for children
    ਆਭਾਸੀ ਖਾਲੀ ਲੜੀਵਾਰ_Test() = 0;  // Run the test
    ਆਭਾਸੀ string name() = 0;  // Name of the test

    // Utilities
    ਖਾਲੀ dumpSelf() {
        ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) m_ਗ੍ਰਾਫ.dumpDotFilePrefixed("v3graphtest_" + name());
    }

ਜਨਤਕ:
    V3_ਗ੍ਰਾਫ_Test() = ਮੂਲ;
    ਆਭਾਸੀ ~V3_ਗ੍ਰਾਫ_Test() = ਮੂਲ;

    ਖਾਲੀ ਲੜੀਵਾਰ() { ਲੜੀਵਾਰ_Test(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

ਵਰਗ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    ਅਚਲ string m_name;

ਜਨਤਕ:
    V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, ਅਚਲ string& name)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_name{name} {}
    ~V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    // ACCESSORS
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
};

ਵਰਗ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼ {
ਜਨਤਕ:
    V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, ਅਚਲ string& name)
        : V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{graphp, name} {}
    ~V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼() override = ਮੂਲ;
    // ACCESSORS
    string dotColor() ਅਚਲ override { ਵਾਪਸ "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

ਵਰਗ V3_ਗ੍ਰਾਫ_Test_Strong final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_Test {
ਜਨਤਕ:
    string name() override { ਵਾਪਸ "strong"; }
    ਖਾਲੀ ਲੜੀਵਾਰ_Test() override {
        V3_ਗ੍ਰਾਫ* gp = &m_ਗ੍ਰਾਫ;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* i = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "*INPUTS*"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* a = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "a"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* b = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "b"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* g1 = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "g1"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* g2 = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "g2"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* g3 = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "g3"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* q = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "q"};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, i, a, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, a, b, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b, g1, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b, g2, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b, g3, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g1, a, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g3, g2, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g2, g3, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g1, q, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g2, q, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g3, q, 2, ਸੱਚ};

        gp->stronglyConnected(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

ਵਰਗ V3_ਗ੍ਰਾਫ_Test_Acyc final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_Test {
ਜਨਤਕ:
    string name() override { ਵਾਪਸ "acyc"; }
    ਖਾਲੀ ਲੜੀਵਾਰ_Test() override {
        V3_ਗ੍ਰਾਫ* gp = &m_ਗ੍ਰਾਫ;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* i = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "*INPUTS*"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* a = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "a"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* b = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "b"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* g1 = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "g1"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* g2 = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "g2"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* g3 = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "g3"};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, i, a, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, a, b, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b, g1, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b, g2, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b, g3, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g1, a, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g2, a, 2, ਸੱਚ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, g3, a, 2, ਸੱਚ};

        gp->acyclic(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        gp->ਕ੍ਰਮ();
        dumpSelf();
    }
};

ਵਰਗ V3_ਗ੍ਰਾਫ_Test_Vars final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_Test {
ਜਨਤਕ:
    string name() override { ਵਾਪਸ "vars"; }
    ਖਾਲੀ ਲੜੀਵਾਰ_Test() override {
        V3_ਗ੍ਰਾਫ* ਅਚਲ gp = &m_ਗ੍ਰਾਫ;

        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ clk = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$clk"};

        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ a = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$a"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ a_dly = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$a_dly"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ a_dlyblk = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$a_dlyblk"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ b = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$b"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ b_dly = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$b_dly"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ b_dlyblk = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$b_dlyblk"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ c = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$c"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ i = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$i"};

        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ ap = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$a_pre"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ bp = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$b_pre"};
        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ cp = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_Var_ਸ਼ੀਰਸ਼{gp, "$c_pre"};

        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* n;

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

        n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "*INPUTS*"};
        {
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, clk, 2};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, i, 2};
        }

        V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼* ਅਚਲ posedge = n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "*posedge clk*"};
        { ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "a_dly<PRE=a"};
        {
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, a_dlyblk, 2};  // Block ordering
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, a_dly, 2};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, ap, n, 2, ਸੱਚ};  // DESIRED delayed ordering (inp is required)
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, posedge, n, 2};
        }
        n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "b_dly<PRE=b"};
        {
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, b_dlyblk, 2};  // Block ordering
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, b_dly, 2};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, bp, n, 2, ਸੱਚ};  // DESIRED delayed ordering
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "a_dly<=b|c"};
        {
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, a_dlyblk, n, 2};  // Block ordering in
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, bp, 2};  // DESIRED delayed usage
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, cp, 2};  // DESIRED delayed usage
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, posedge, n, 2};
        }
        n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "b_dly<=a"};
        {
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b_dlyblk, n, 2};  // Block ordering in
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, b_dly, 2};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, ap, 2};  // DESIRED delayed usage
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "a=POST=a_dly"};
        {
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, a, 3};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, a_dly, n, 3};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, posedge, n, 2};
        }
        n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "b=POST=b_dly"};
        {
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, b, 3};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b_dly, n, 3};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = ਨਵਾਂ V3_ਗ੍ਰਾਫ_Test_ਸ਼ੀਰਸ਼{gp, "c=a|b|i"};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, n, c, 1, ਸੱਚ};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, a, n, 1, ਝੂਠ};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, b, n, 1, ਝੂਠ};
            ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{gp, i, n, 1, ਝੂਠ};
        }

        gp->acyclic(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        gp->ਕ੍ਰਮ();

        // Test debug function
        gp->dump_ਕਿਨਾਰੇ(std::ਬਾਹਰ, *ap);

        dumpSelf();
    }
};

//======================================================================

ਵਰਗ V3_ਗ੍ਰਾਫ_Test_Import final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_Test {

#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ ਗ੍ਰਾਫ_IMPORT
    ਖਾਲੀ dotImport();
#ਨਹੀਂਤਾਂ
    ਖਾਲੀ dotImport() {}
#ਜੇ_ਅੰਤ

ਜਨਤਕ:
    string name() override { ਵਾਪਸ "import"; }
    ਖਾਲੀ ਲੜੀਵਾਰ_Test() override {
        V3_ਗ੍ਰਾਫ* ਅਚਲ gp = &m_ਗ੍ਰਾਫ;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#ਜੇ_ਪਰਿਭਾਸ਼ਿਤ ਗ੍ਰਾਫ_IMPORT
# include "graph_export.cpp"
#ਜੇ_ਅੰਤ
// clang-format on

//======================================================================

ਖਾਲੀ V3_ਗ੍ਰਾਫ::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3_ਗ੍ਰਾਫ_Test_Strong{}.ਲੜੀਵਾਰ(); }
    { V3_ਗ੍ਰਾਫ_Test_Acyc{}.ਲੜੀਵਾਰ(); }
    { V3_ਗ੍ਰਾਫ_Test_Vars{}.ਲੜੀਵਾਰ(); }
    { V3_ਗ੍ਰਾਫ_Test_Import{}.ਲੜੀਵਾਰ(); }
}
