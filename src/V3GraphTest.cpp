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

#تعریف_کن VL_MT_DISABLED_کد_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Global.h"
#ضمن "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

صنف V3_گراف_Test VL_NOT_FINAL {
محمي:
    // MEMBERS
    V3_گراف m_گراف;

    // METHODS - for children
    مجازی فراغ پیاپی_Test() = 0;  // Run the test
    مجازی string name() = 0;  // Name of the test

    // Utilities
    فراغ dumpSelf() {
        إذا (dump_گراف_Level() >= 9) m_گراف.dumpDotFilePrefixed("v3graphtest_" + name());
    }

عام:
    V3_گراف_Test() = افتراضي;
    مجازی ~V3_گراف_Test() = افتراضي;

    فراغ پیاپی() { پیاپی_Test(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

صنف V3_گراف_Test_رأس VL_NOT_FINAL : عام V3_گراف_رأس {
    VL_RTTI_IMPL(V3_گراف_Test_رأس, V3_گراف_رأس)
    ثابت string m_name;

عام:
    V3_گراف_Test_رأس(V3_گراف* graphp, ثابت string& name)
        : V3_گراف_رأس{graphp}
        , m_name{name} {}
    ~V3_گراف_Test_رأس() override = افتراضي;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};

صنف V3_گراف_Test_Var_رأس final : عام V3_گراف_Test_رأس {
عام:
    V3_گراف_Test_Var_رأس(V3_گراف* graphp, ثابت string& name)
        : V3_گراف_Test_رأس{graphp, name} {}
    ~V3_گراف_Test_Var_رأس() override = افتراضي;
    // ACCESSORS
    string dotColor() ثابت override { أرجع "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

صنف V3_گراف_Test_Strong final : عام V3_گراف_Test {
عام:
    string name() override { أرجع "strong"; }
    فراغ پیاپی_Test() override {
        V3_گراف* gp = &m_گراف;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_گراف_Test_رأس* i = جديد V3_گراف_Test_Var_رأس{gp, "*INPUTS*"};
        V3_گراف_Test_رأس* a = جديد V3_گراف_Test_Var_رأس{gp, "a"};
        V3_گراف_Test_رأس* b = جديد V3_گراف_Test_Var_رأس{gp, "b"};
        V3_گراف_Test_رأس* g1 = جديد V3_گراف_Test_Var_رأس{gp, "g1"};
        V3_گراف_Test_رأس* g2 = جديد V3_گراف_Test_Var_رأس{gp, "g2"};
        V3_گراف_Test_رأس* g3 = جديد V3_گراف_Test_Var_رأس{gp, "g3"};
        V3_گراف_Test_رأس* q = جديد V3_گراف_Test_Var_رأس{gp, "q"};
        جديد V3_گراف_یال{gp, i, a, 2, درست};
        جديد V3_گراف_یال{gp, a, b, 2, درست};
        جديد V3_گراف_یال{gp, b, g1, 2, درست};
        جديد V3_گراف_یال{gp, b, g2, 2, درست};
        جديد V3_گراف_یال{gp, b, g3, 2, درست};
        جديد V3_گراف_یال{gp, g1, a, 2, درست};
        جديد V3_گراف_یال{gp, g3, g2, 2, درست};
        جديد V3_گراف_یال{gp, g2, g3, 2, درست};
        جديد V3_گراف_یال{gp, g1, q, 2, درست};
        جديد V3_گراف_یال{gp, g2, q, 2, درست};
        جديد V3_گراف_یال{gp, g3, q, 2, درست};

        gp->stronglyConnected(&V3_گراف_یال::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

صنف V3_گراف_Test_Acyc final : عام V3_گراف_Test {
عام:
    string name() override { أرجع "acyc"; }
    فراغ پیاپی_Test() override {
        V3_گراف* gp = &m_گراف;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_گراف_Test_رأس* i = جديد V3_گراف_Test_Var_رأس{gp, "*INPUTS*"};
        V3_گراف_Test_رأس* a = جديد V3_گراف_Test_Var_رأس{gp, "a"};
        V3_گراف_Test_رأس* b = جديد V3_گراف_Test_Var_رأس{gp, "b"};
        V3_گراف_Test_رأس* g1 = جديد V3_گراف_Test_Var_رأس{gp, "g1"};
        V3_گراف_Test_رأس* g2 = جديد V3_گراف_Test_Var_رأس{gp, "g2"};
        V3_گراف_Test_رأس* g3 = جديد V3_گراف_Test_Var_رأس{gp, "g3"};
        جديد V3_گراف_یال{gp, i, a, 2, درست};
        جديد V3_گراف_یال{gp, a, b, 2, درست};
        جديد V3_گراف_یال{gp, b, g1, 2, درست};
        جديد V3_گراف_یال{gp, b, g2, 2, درست};
        جديد V3_گراف_یال{gp, b, g3, 2, درست};
        جديد V3_گراف_یال{gp, g1, a, 2, درست};
        جديد V3_گراف_یال{gp, g2, a, 2, درست};
        جديد V3_گراف_یال{gp, g3, a, 2, درست};

        gp->acyclic(&V3_گراف_یال::followAlwaysTrue);
        gp->ترتیب();
        dumpSelf();
    }
};

صنف V3_گراف_Test_Vars final : عام V3_گراف_Test {
عام:
    string name() override { أرجع "vars"; }
    فراغ پیاپی_Test() override {
        V3_گراف* ثابت gp = &m_گراف;

        V3_گراف_Test_رأس* ثابت clk = جديد V3_گراف_Test_Var_رأس{gp, "$clk"};

        V3_گراف_Test_رأس* ثابت a = جديد V3_گراف_Test_Var_رأس{gp, "$a"};
        V3_گراف_Test_رأس* ثابت a_dly = جديد V3_گراف_Test_Var_رأس{gp, "$a_dly"};
        V3_گراف_Test_رأس* ثابت a_dlyblk = جديد V3_گراف_Test_Var_رأس{gp, "$a_dlyblk"};
        V3_گراف_Test_رأس* ثابت b = جديد V3_گراف_Test_Var_رأس{gp, "$b"};
        V3_گراف_Test_رأس* ثابت b_dly = جديد V3_گراف_Test_Var_رأس{gp, "$b_dly"};
        V3_گراف_Test_رأس* ثابت b_dlyblk = جديد V3_گراف_Test_Var_رأس{gp, "$b_dlyblk"};
        V3_گراف_Test_رأس* ثابت c = جديد V3_گراف_Test_Var_رأس{gp, "$c"};
        V3_گراف_Test_رأس* ثابت i = جديد V3_گراف_Test_Var_رأس{gp, "$i"};

        V3_گراف_Test_رأس* ثابت ap = جديد V3_گراف_Test_Var_رأس{gp, "$a_pre"};
        V3_گراف_Test_رأس* ثابت bp = جديد V3_گراف_Test_Var_رأس{gp, "$b_pre"};
        V3_گراف_Test_رأس* ثابت cp = جديد V3_گراف_Test_Var_رأس{gp, "$c_pre"};

        V3_گراف_Test_رأس* n;

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

        n = جديد V3_گراف_Test_رأس{gp, "*INPUTS*"};
        {
            جديد V3_گراف_یال{gp, n, clk, 2};
            جديد V3_گراف_یال{gp, n, i, 2};
        }

        V3_گراف_Test_رأس* ثابت posedge = n = جديد V3_گراف_Test_رأس{gp, "*posedge clk*"};
        { جديد V3_گراف_یال{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = جديد V3_گراف_Test_رأس{gp, "a_dly<PRE=a"};
        {
            جديد V3_گراف_یال{gp, n, a_dlyblk, 2};  // Block ordering
            جديد V3_گراف_یال{gp, n, a_dly, 2};
            جديد V3_گراف_یال{gp, ap, n, 2, درست};  // DESIRED delayed ordering (inp is required)
            جديد V3_گراف_یال{gp, posedge, n, 2};
        }
        n = جديد V3_گراف_Test_رأس{gp, "b_dly<PRE=b"};
        {
            جديد V3_گراف_یال{gp, n, b_dlyblk, 2};  // Block ordering
            جديد V3_گراف_یال{gp, n, b_dly, 2};
            جديد V3_گراف_یال{gp, bp, n, 2, درست};  // DESIRED delayed ordering
            جديد V3_گراف_یال{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = جديد V3_گراف_Test_رأس{gp, "a_dly<=b|c"};
        {
            جديد V3_گراف_یال{gp, a_dlyblk, n, 2};  // Block ordering in
            جديد V3_گراف_یال{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            جديد V3_گراف_یال{gp, n, bp, 2};  // DESIRED delayed usage
            جديد V3_گراف_یال{gp, n, cp, 2};  // DESIRED delayed usage
            جديد V3_گراف_یال{gp, posedge, n, 2};
        }
        n = جديد V3_گراف_Test_رأس{gp, "b_dly<=a"};
        {
            جديد V3_گراف_یال{gp, b_dlyblk, n, 2};  // Block ordering in
            جديد V3_گراف_یال{gp, n, b_dly, 2};
            جديد V3_گراف_یال{gp, n, ap, 2};  // DESIRED delayed usage
            جديد V3_گراف_یال{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = جديد V3_گراف_Test_رأس{gp, "a=POST=a_dly"};
        {
            جديد V3_گراف_یال{gp, n, a, 3};
            جديد V3_گراف_یال{gp, a_dly, n, 3};
            جديد V3_گراف_یال{gp, posedge, n, 2};
        }
        n = جديد V3_گراف_Test_رأس{gp, "b=POST=b_dly"};
        {
            جديد V3_گراف_یال{gp, n, b, 3};
            جديد V3_گراف_یال{gp, b_dly, n, 3};
            جديد V3_گراف_یال{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = جديد V3_گراف_Test_رأس{gp, "c=a|b|i"};
            جديد V3_گراف_یال{gp, n, c, 1, درست};
            جديد V3_گراف_یال{gp, a, n, 1, خطأ};
            جديد V3_گراف_یال{gp, b, n, 1, خطأ};
            جديد V3_گراف_یال{gp, i, n, 1, خطأ};
        }

        gp->acyclic(&V3_گراف_یال::followAlwaysTrue);
        gp->ترتیب();

        // Test debug function
        gp->dump_یالها(std::خروجی, *ap);

        dumpSelf();
    }
};

//======================================================================

صنف V3_گراف_Test_Import final : عام V3_گراف_Test {

#اگر_تعریف_شده گراف_IMPORT
    فراغ dotImport();
#وإلا
    فراغ dotImport() {}
#پایان_اگر

عام:
    string name() override { أرجع "import"; }
    فراغ پیاپی_Test() override {
        V3_گراف* ثابت gp = &m_گراف;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3_گراف_یال::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3_گراف_یال::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#اگر_تعریف_شده گراف_IMPORT
# include "graph_export.cpp"
#پایان_اگر
// clang-format on

//======================================================================

فراغ V3_گراف::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3_گراف_Test_Strong{}.پیاپی(); }
    { V3_گراف_Test_Acyc{}.پیاپی(); }
    { V3_گراف_Test_Vars{}.پیاپی(); }
    { V3_گراف_Test_Import{}.پیاپی(); }
}
