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

#تعریف_کرو VL_MT_DISABLED_رمز_UNIT 1

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Global.h"
#شامل "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

جماعت V3_گراف_Test VL_NOT_FINAL {
محفوظ:
    // MEMBERS
    V3_گراف m_گراف;

    // METHODS - for children
    مجازی خالی تسلسل_Test() = 0;  // Run the test
    مجازی string name() = 0;  // Name of the test

    // Utilities
    خالی dumpSelf() {
        اگر (dump_گراف_Level() >= 9) m_گراف.dumpDotFilePrefixed("v3graphtest_" + name());
    }

عوامی:
    V3_گراف_Test() = طےشدہ;
    مجازی ~V3_گراف_Test() = طےشدہ;

    خالی تسلسل() { تسلسل_Test(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

جماعت V3_گراف_Test_راس VL_NOT_FINAL : عوامی V3_گراف_راس {
    VL_RTTI_IMPL(V3_گراف_Test_راس, V3_گراف_راس)
    ثابت string m_name;

عوامی:
    V3_گراف_Test_راس(V3_گراف* graphp, ثابت string& name)
        : V3_گراف_راس{graphp}
        , m_name{name} {}
    ~V3_گراف_Test_راس() override = طےشدہ;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
};

جماعت V3_گراف_Test_Var_راس final : عوامی V3_گراف_Test_راس {
عوامی:
    V3_گراف_Test_Var_راس(V3_گراف* graphp, ثابت string& name)
        : V3_گراف_Test_راس{graphp, name} {}
    ~V3_گراف_Test_Var_راس() override = طےشدہ;
    // ACCESSORS
    string dotColor() ثابت override { واپس "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

جماعت V3_گراف_Test_Strong final : عوامی V3_گراف_Test {
عوامی:
    string name() override { واپس "strong"; }
    خالی تسلسل_Test() override {
        V3_گراف* gp = &m_گراف;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_گراف_Test_راس* i = نیا V3_گراف_Test_Var_راس{gp, "*INPUTS*"};
        V3_گراف_Test_راس* a = نیا V3_گراف_Test_Var_راس{gp, "a"};
        V3_گراف_Test_راس* b = نیا V3_گراف_Test_Var_راس{gp, "b"};
        V3_گراف_Test_راس* g1 = نیا V3_گراف_Test_Var_راس{gp, "g1"};
        V3_گراف_Test_راس* g2 = نیا V3_گراف_Test_Var_راس{gp, "g2"};
        V3_گراف_Test_راس* g3 = نیا V3_گراف_Test_Var_راس{gp, "g3"};
        V3_گراف_Test_راس* q = نیا V3_گراف_Test_Var_راس{gp, "q"};
        نیا V3_گراف_کنارہ{gp, i, a, 2, سچ};
        نیا V3_گراف_کنارہ{gp, a, b, 2, سچ};
        نیا V3_گراف_کنارہ{gp, b, g1, 2, سچ};
        نیا V3_گراف_کنارہ{gp, b, g2, 2, سچ};
        نیا V3_گراف_کنارہ{gp, b, g3, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g1, a, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g3, g2, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g2, g3, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g1, q, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g2, q, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g3, q, 2, سچ};

        gp->stronglyConnected(&V3_گراف_کنارہ::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

جماعت V3_گراف_Test_Acyc final : عوامی V3_گراف_Test {
عوامی:
    string name() override { واپس "acyc"; }
    خالی تسلسل_Test() override {
        V3_گراف* gp = &m_گراف;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_گراف_Test_راس* i = نیا V3_گراف_Test_Var_راس{gp, "*INPUTS*"};
        V3_گراف_Test_راس* a = نیا V3_گراف_Test_Var_راس{gp, "a"};
        V3_گراف_Test_راس* b = نیا V3_گراف_Test_Var_راس{gp, "b"};
        V3_گراف_Test_راس* g1 = نیا V3_گراف_Test_Var_راس{gp, "g1"};
        V3_گراف_Test_راس* g2 = نیا V3_گراف_Test_Var_راس{gp, "g2"};
        V3_گراف_Test_راس* g3 = نیا V3_گراف_Test_Var_راس{gp, "g3"};
        نیا V3_گراف_کنارہ{gp, i, a, 2, سچ};
        نیا V3_گراف_کنارہ{gp, a, b, 2, سچ};
        نیا V3_گراف_کنارہ{gp, b, g1, 2, سچ};
        نیا V3_گراف_کنارہ{gp, b, g2, 2, سچ};
        نیا V3_گراف_کنارہ{gp, b, g3, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g1, a, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g2, a, 2, سچ};
        نیا V3_گراف_کنارہ{gp, g3, a, 2, سچ};

        gp->acyclic(&V3_گراف_کنارہ::followAlwaysTrue);
        gp->ترتیب();
        dumpSelf();
    }
};

جماعت V3_گراف_Test_Vars final : عوامی V3_گراف_Test {
عوامی:
    string name() override { واپس "vars"; }
    خالی تسلسل_Test() override {
        V3_گراف* ثابت gp = &m_گراف;

        V3_گراف_Test_راس* ثابت clk = نیا V3_گراف_Test_Var_راس{gp, "$clk"};

        V3_گراف_Test_راس* ثابت a = نیا V3_گراف_Test_Var_راس{gp, "$a"};
        V3_گراف_Test_راس* ثابت a_dly = نیا V3_گراف_Test_Var_راس{gp, "$a_dly"};
        V3_گراف_Test_راس* ثابت a_dlyblk = نیا V3_گراف_Test_Var_راس{gp, "$a_dlyblk"};
        V3_گراف_Test_راس* ثابت b = نیا V3_گراف_Test_Var_راس{gp, "$b"};
        V3_گراف_Test_راس* ثابت b_dly = نیا V3_گراف_Test_Var_راس{gp, "$b_dly"};
        V3_گراف_Test_راس* ثابت b_dlyblk = نیا V3_گراف_Test_Var_راس{gp, "$b_dlyblk"};
        V3_گراف_Test_راس* ثابت c = نیا V3_گراف_Test_Var_راس{gp, "$c"};
        V3_گراف_Test_راس* ثابت i = نیا V3_گراف_Test_Var_راس{gp, "$i"};

        V3_گراف_Test_راس* ثابت ap = نیا V3_گراف_Test_Var_راس{gp, "$a_pre"};
        V3_گراف_Test_راس* ثابت bp = نیا V3_گراف_Test_Var_راس{gp, "$b_pre"};
        V3_گراف_Test_راس* ثابت cp = نیا V3_گراف_Test_Var_راس{gp, "$c_pre"};

        V3_گراف_Test_راس* n;

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

        n = نیا V3_گراف_Test_راس{gp, "*INPUTS*"};
        {
            نیا V3_گراف_کنارہ{gp, n, clk, 2};
            نیا V3_گراف_کنارہ{gp, n, i, 2};
        }

        V3_گراف_Test_راس* ثابت posedge = n = نیا V3_گراف_Test_راس{gp, "*posedge clk*"};
        { نیا V3_گراف_کنارہ{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = نیا V3_گراف_Test_راس{gp, "a_dly<PRE=a"};
        {
            نیا V3_گراف_کنارہ{gp, n, a_dlyblk, 2};  // Block ordering
            نیا V3_گراف_کنارہ{gp, n, a_dly, 2};
            نیا V3_گراف_کنارہ{gp, ap, n, 2, سچ};  // DESIRED delayed ordering (inp is required)
            نیا V3_گراف_کنارہ{gp, posedge, n, 2};
        }
        n = نیا V3_گراف_Test_راس{gp, "b_dly<PRE=b"};
        {
            نیا V3_گراف_کنارہ{gp, n, b_dlyblk, 2};  // Block ordering
            نیا V3_گراف_کنارہ{gp, n, b_dly, 2};
            نیا V3_گراف_کنارہ{gp, bp, n, 2, سچ};  // DESIRED delayed ordering
            نیا V3_گراف_کنارہ{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = نیا V3_گراف_Test_راس{gp, "a_dly<=b|c"};
        {
            نیا V3_گراف_کنارہ{gp, a_dlyblk, n, 2};  // Block ordering in
            نیا V3_گراف_کنارہ{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            نیا V3_گراف_کنارہ{gp, n, bp, 2};  // DESIRED delayed usage
            نیا V3_گراف_کنارہ{gp, n, cp, 2};  // DESIRED delayed usage
            نیا V3_گراف_کنارہ{gp, posedge, n, 2};
        }
        n = نیا V3_گراف_Test_راس{gp, "b_dly<=a"};
        {
            نیا V3_گراف_کنارہ{gp, b_dlyblk, n, 2};  // Block ordering in
            نیا V3_گراف_کنارہ{gp, n, b_dly, 2};
            نیا V3_گراف_کنارہ{gp, n, ap, 2};  // DESIRED delayed usage
            نیا V3_گراف_کنارہ{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = نیا V3_گراف_Test_راس{gp, "a=POST=a_dly"};
        {
            نیا V3_گراف_کنارہ{gp, n, a, 3};
            نیا V3_گراف_کنارہ{gp, a_dly, n, 3};
            نیا V3_گراف_کنارہ{gp, posedge, n, 2};
        }
        n = نیا V3_گراف_Test_راس{gp, "b=POST=b_dly"};
        {
            نیا V3_گراف_کنارہ{gp, n, b, 3};
            نیا V3_گراف_کنارہ{gp, b_dly, n, 3};
            نیا V3_گراف_کنارہ{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = نیا V3_گراف_Test_راس{gp, "c=a|b|i"};
            نیا V3_گراف_کنارہ{gp, n, c, 1, سچ};
            نیا V3_گراف_کنارہ{gp, a, n, 1, جھوٹ};
            نیا V3_گراف_کنارہ{gp, b, n, 1, جھوٹ};
            نیا V3_گراف_کنارہ{gp, i, n, 1, جھوٹ};
        }

        gp->acyclic(&V3_گراف_کنارہ::followAlwaysTrue);
        gp->ترتیب();

        // Test debug function
        gp->dump_کنارے(std::خارج, *ap);

        dumpSelf();
    }
};

//======================================================================

جماعت V3_گراف_Test_Import final : عوامی V3_گراف_Test {

#اگر_تعریف_شدہ گراف_IMPORT
    خالی dotImport();
#ورنہ
    خالی dotImport() {}
#اگر_ختم

عوامی:
    string name() override { واپس "import"; }
    خالی تسلسل_Test() override {
        V3_گراف* ثابت gp = &m_گراف;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3_گراف_کنارہ::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3_گراف_کنارہ::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#اگر_تعریف_شدہ گراف_IMPORT
# include "graph_export.cpp"
#اگر_ختم
// clang-format on

//======================================================================

خالی V3_گراف::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3_گراف_Test_Strong{}.تسلسل(); }
    { V3_گراف_Test_Acyc{}.تسلسل(); }
    { V3_گراف_Test_Vars{}.تسلسل(); }
    { V3_گراف_Test_Import{}.تسلسل(); }
}
