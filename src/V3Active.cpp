// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity active domains
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
// V3Active's Transformations:
//
// Note this can be called multiple times.
//   Create a IACTIVE(initial), SACTIVE(combo)
//      ALWAYS: Remove any-edges from sense list
//          If no POS/NEG in senselist, Fold into SACTIVE(combo)
//          Else fold into SACTIVE(sequent).
//          OPTIMIZE: When support async clocks, fold into that active if possible
//      INITIAL: Move into IACTIVE
//      WIRE: Move into SACTIVE(combo)
//
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Active.h"

#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

ವರ್ಗ Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ ಶೃಂಗ_Type : uint8_t { VT_ಖಂಡ, VT_BRANCH, VT_OUTPUT };

ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ string m_name;  // Only used for .dot file generation
    ಸ್ಥಿರಾಂಕ ಶೃಂಗ_Type m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ {  //   "
        ಆಯ್ಕೆ (m_type) {
        ಸ್ಥಿತಿ VT_ಖಂಡ: ಹಿಂತಿರುಗಿ "(||)";  // basic block node
        ಸ್ಥಿತಿ VT_BRANCH: ಹಿಂತಿರುಗಿ "(&&)";  // if/else branch mode
        ಸ್ಥಿತಿ VT_OUTPUT: ಹಿಂತಿರುಗಿ "(out)";  // var assignment
        ಪೂರ್ವನಿಯೋಜಿತ: ಹಿಂತಿರುಗಿ "??";  // unknown
        }
    }

ಸಾರ್ವಜನಿಕ:
    Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ string& name, ಶೃಂಗ_Type type = VT_ಖಂಡ)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name + " " + typestr(); }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ user() ? "green" : "black"; }
    ವಾಸ್ತವಿಕ ಪೂರ್ಣಾಂಕ type() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

ವರ್ಗ Latch_Detect_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
ಸಂರಕ್ಷಿತ:
    Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* m_curVertexp = ಶೂನ್ಯಸೂಚಕ;  // Current latch detection graph vertex
    std::ಸದಿಶ<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ಸ್ಥಿರ Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* castVertexp(ಶೂನ್ಯ* vertexp) {
        ಹಿಂತಿರುಗಿ reinterpret_cast<Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    ತಾರ್ಕಿಕ latch_Check_ಆಂತರಿಕ(Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* vertexp) {
        ತಾರ್ಕಿಕ ಫಲಿತಾಂಶ = ಸುಳ್ಳು;
        ಆಯ್ಕೆ (vertexp->type()) {
        ಸ್ಥಿತಿ Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ::VT_OUTPUT:  // Base case
            ಫಲಿತಾಂಶ = vertexp->user();
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ::VT_ಖಂಡ:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            ಗಾಗಿ (V3_ನಕ್ಷೆ_ಅಂಚು& ಅಂಚು : vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು()) {
                ಆದರೆ (latch_Check_ಆಂತರಿಕ(castVertexp(ಅಂಚು.ತುದಿ()))) {
                    ಫಲಿತಾಂಶ = ನಿಜ;
                    ನಿಲ್ಲಿಸು;
                }
            }
            ನಿಲ್ಲಿಸು;
        ಸ್ಥಿತಿ Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ifp = castVertexp(vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().frontp()->ತುದಿ());
            Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ elsp = castVertexp(vertexp->ಹೊರಹೋಗು_ಅಂಚುಗಳು().backp()->ತುದಿ());
            ಫಲಿತಾಂಶ = latch_Check_ಆಂತರಿಕ(ifp) && latch_Check_ಆಂತರಿಕ(elsp);
            ನಿಲ್ಲಿಸು;
        }
        vertexp->user(ಫಲಿತಾಂಶ);
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

ಸಾರ್ವಜನಿಕ:
    Latch_Detect_ನಕ್ಷೆ() { clear(); }
    ~Latch_Detect_ನಕ್ಷೆ() override { clear(); }
    // ACCESSORS
    Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* currentp() { ಹಿಂತಿರುಗಿ m_curVertexp; }
    ಶೂನ್ಯ currentp(Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಶೃಂಗ) { m_curVertexp = ಶೃಂಗ; }
    // METHODS
    ಶೂನ್ಯ begin() {
        // Start a new if/else tracking graph
        m_curVertexp = ಹೊಸ Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ{ಇದು, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    ಶೂನ್ಯ clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3_ನಕ್ಷೆ::clear();
        m_curVertexp = ಶೂನ್ಯಸೂಚಕ;
    }
    // Add a new control path and connect it to its parent
    Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಕೂಡಿಸು_ಮಾರ್ಗ_ಶೃಂಗ(Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಪೋಷಕ, ಸ್ಥಿರಾಂಕ string& name,
                                          ತಾರ್ಕಿಕ branch = ಸುಳ್ಳು) {
        m_curVertexp = ಹೊಸ Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ{ಇದು, name,
                                                  branch ? Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ::VT_BRANCH
                                                         : Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ::VT_ಖಂಡ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{ಇದು, ಪೋಷಕ, m_curVertexp, 1};
        ಹಿಂತಿರುಗಿ m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಕೂಡಿಸು_Output_ಶೃಂಗ(AstVarRef* nodep) {
        Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ಹೊರಹೋಗು_Vertexp
            = ಹೊಸ Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ{ಇದು, nodep->name(), Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ::VT_OUTPUT};
        nodep->varp()->user1p(ಹೊರಹೋಗು_Vertexp);
        m_outputs.ತಳ್ಳು_ಹಿಂಭಾಗ(nodep);
        ಹಿಂತಿರುಗಿ ಹೊರಹೋಗು_Vertexp;
    }
    // Connect an output assignment to its parent control block
    ಶೂನ್ಯ ಕೂಡಿಸು_Assignment(AstVarRef* nodep) {
        Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಹೊರಹೋಗು_Vertexp;
        ಆದರೆ (!nodep->varp()->user1p()) {  // Not seen this output before
            ಹೊರಹೋಗು_Vertexp = ಕೂಡಿಸು_Output_ಶೃಂಗ(nodep);
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಹೊರಹೋಗು_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{ಇದು, m_curVertexp, ಹೊರಹೋಗು_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    ಶೂನ್ಯ latchCheck(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ latch_expected) {
        ತಾರ್ಕಿಕ latch_detected = ಸುಳ್ಳು;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ vrp : m_outputs) {
            Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(ನಿಜ);  // Identify the output vertex we are checking paths _to_
            ಆದರೆ (!latch_Check_ಆಂತರಿಕ(castVertexp(vertices().frontp()))) latch_detected = ನಿಜ;
            ಆದರೆ (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_ಇನ್ನಷ್ಟು()
                        << "... Suggest use of always_latch for intentional latches");
                ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(ಸುಳ್ಳು);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        ಆದರೆ (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

ವರ್ಗ ಸಕ್ರಿಯ_Namer final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // STATE
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope to add statement to
    Ast_ಸಕ್ರಿಯ* m_sActivep = ಶೂನ್ಯಸೂಚಕ;  // For current scope, the Static active we're building
    Ast_ಸಕ್ರಿಯ* m_iActivep = ಶೂನ್ಯಸೂಚಕ;  // For current scope, the Initial active we're building
    Ast_ಸಕ್ರಿಯ* m_fActivep = ಶೂನ್ಯಸೂಚಕ;  // For current scope, the Final active we're building
    Ast_ಸಕ್ರಿಯ* m_cActivep = ಶೂನ್ಯಸೂಚಕ;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_ವೃಕ್ಷ>, Ast_ಸಕ್ರಿಯ*> m_ಸಕ್ರಿಯ_Map;

    // METHODS
    ಶೂನ್ಯ ಕೂಡಿಸು_ಸಕ್ರಿಯ(Ast_ಸಕ್ರಿಯ* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->ಕೂಡಿಸು_Blocksp(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = ಶೂನ್ಯಸೂಚಕ;
        m_iActivep = ಶೂನ್ಯಸೂಚಕ;
        m_fActivep = ಶೂನ್ಯಸೂಚಕ;
        m_cActivep = ಶೂನ್ಯಸೂಚಕ;
        m_ಸಕ್ರಿಯ_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ವೃಕ್ಷ* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Stmt*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Sen_ಅಂಶ_ಬಗೆ>
    Ast_ಸಕ್ರಿಯ*& get_Special_ಸಕ್ರಿಯ();

ಸಾರ್ವಜನಿಕ:
    // METHODS
    AstScope* scopep() { ಹಿಂತಿರುಗಿ m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_ಸಕ್ರಿಯ* make_ಸಕ್ರಿಯ(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl, Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep) {
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{fl, "", senTreep};
        activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
        ಕೂಡಿಸು_ಸಕ್ರಿಯ(activep);
        ಹಿಂತಿರುಗಿ activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Sen_ಅಂಶ_ಬಗೆ>
    Ast_ಸಕ್ರಿಯ* make_Special_ಸಕ್ರಿಯ(File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl) {
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ senTreep = ಹೊಸ Ast_Sen_ವೃಕ್ಷ{fl, ಹೊಸ Ast_Sen_ಅಂಶ{fl, T_Sen_ಅಂಶ_ಬಗೆ{}}};
        ಹಿಂತಿರುಗಿ make_ಸಕ್ರಿಯ(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T_Sen_ಅಂಶ_ಬಗೆ>
    Ast_ಸಕ್ರಿಯ* get_Special_ಸಕ್ರಿಯ(File_ಸಾಲು* fl) {
        Ast_ಸಕ್ರಿಯ*& cachep = get_Special_ಸಕ್ರಿಯ<T_Sen_ಅಂಶ_ಬಗೆ>();
        ಆದರೆ (!cachep) cachep = make_Special_ಸಕ್ರಿಯ<T_Sen_ಅಂಶ_ಬಗೆ>(fl);
        ಹಿಂತಿರುಗಿ cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_ಸಕ್ರಿಯ* get_ಸಕ್ರಿಯ(File_ಸಾಲು* fl, Ast_Sen_ವೃಕ್ಷ* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        ಸ್ವಯಂಚಾಲಿತ it = m_ಸಕ್ರಿಯ_Map.ಕಂಡುಹಿಡಿ(*sentreep);
        // If found matching AstActive, return it
        ಆದರೆ (it != m_ಸಕ್ರಿಯ_Map.ಕೊನೆ()) ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ newsenp = sentreep->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ಹೊಸ Ast_ಸಕ್ರಿಯ{fl, "sequent", newsenp};
        activep->sen_ವೃಕ್ಷ_Storep(activep->sentreep());
        ಕೂಡಿಸು_ಸಕ್ರಿಯ(activep);
        m_ಸಕ್ರಿಯ_Map.emplace(*newsenp, activep);
        ಹಿಂತಿರುಗಿ activep;
    }

    // CONSTRUCTORS
    ಸಕ್ರಿಯ_Namer() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~ಸಕ್ರಿಯ_Namer() override = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ ಮುಖ್ಯ(AstScope* nodep) { iterate(nodep); }
};

ಮಾದರಿ <>
Ast_ಸಕ್ರಿಯ*& ಸಕ್ರಿಯ_Namer::get_Special_ಸಕ್ರಿಯ<Ast_Sen_ಅಂಶ::Static>() {
    ಹಿಂತಿರುಗಿ m_sActivep;
}
ಮಾದರಿ <>
Ast_ಸಕ್ರಿಯ*& ಸಕ್ರಿಯ_Namer::get_Special_ಸಕ್ರಿಯ<Ast_Sen_ಅಂಶ::Initial>() {
    ಹಿಂತಿರುಗಿ m_iActivep;
}
ಮಾದರಿ <>
Ast_ಸಕ್ರಿಯ*& ಸಕ್ರಿಯ_Namer::get_Special_ಸಕ್ರಿಯ<Ast_Sen_ಅಂಶ::Final>() {
    ಹಿಂತಿರುಗಿ m_fActivep;
}
ಮಾದರಿ <>
Ast_ಸಕ್ರಿಯ*& ಸಕ್ರಿಯ_Namer::get_Special_ಸಕ್ರಿಯ<Ast_Sen_ಅಂಶ::Combo>() {
    ಹಿಂತಿರುಗಿ m_cActivep;
}

//######################################################################
// Latch checking visitor

ವರ್ಗ ಸಕ್ರಿಯ_Latch_Check_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use m_inuser1;
    // STATE
    Latch_Detect_ನಕ್ಷೆ m_ನಕ್ಷೆ;  // Graph used to detect latches in combo always
    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        ಆದರೆ (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_ಬಳಸಿದ_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_ನಕ್ಷೆ.ಕೂಡಿಸು_Assignment(nodep);
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_If* nodep) override {
        ಆದರೆ (!nodep->isBoundsCheck()) {
            Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ parentp = m_ನಕ್ಷೆ.currentp();
            Latch_Detect_ನಕ್ಷೆ_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ branchp = m_ನಕ್ಷೆ.ಕೂಡಿಸು_ಮಾರ್ಗ_ಶೃಂಗ(parentp, "BRANCH", ನಿಜ);
            m_ನಕ್ಷೆ.ಕೂಡಿಸು_ಮಾರ್ಗ_ಶೃಂಗ(branchp, "IF");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->thensp());
            m_ನಕ್ಷೆ.ಕೂಡಿಸು_ಮಾರ್ಗ_ಶೃಂಗ(branchp, "ELSE");
            iterate_And_ಮುಂದಿನ_Const_Null(nodep->elsesp());
            m_ನಕ್ಷೆ.currentp(parentp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸಕ್ರಿಯ_Latch_Check_Visitor(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ expectLatch) {
        m_ನಕ್ಷೆ.begin();
        iterateConst(nodep);
        m_ನಕ್ಷೆ.latchCheck(nodep, expectLatch);
    }
    ~ಸಕ್ರಿಯ_Latch_Check_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

ವರ್ಗ ಸಕ್ರಿಯ_Dly_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ CheckType : uint8_t { CT_COMB, CT_FINAL };

ಖಾಸಗಿ:
    // MEMBERS
    ಸ್ಥಿರಾಂಕ CheckType m_check;  // Process type we are checking

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        // Issue appropriate warning
        ಆದರೆ (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_ಇನ್ನಷ್ಟು()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(ಹೊಸ Ast_ನಿಯೋಜಿಸು{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(),  //
            nodep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_ಹಿಂಭಾಗ() : ಶೂನ್ಯಸೂಚಕ});
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸಕ್ರಿಯ_Dly_Visitor(Ast_ಗಂಟು* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ಸಕ್ರಿಯ_Dly_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Active class functions

ವರ್ಗ ಸಕ್ರಿಯ_Visitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ಸಕ್ರಿಯ_Namer m_namer;  // Tracking of active names
    ತಾರ್ಕಿಕ m_clockedProcess = ಸುಳ್ಳು;  // Whether current process is a clocked process
    ತಾರ್ಕಿಕ m_all_ಬದಲಾಯಿತು = ಸುಳ್ಳು;  // Whether all SenItem in the SenTree are ET_CHANGED
    ತಾರ್ಕಿಕ m_walkingBody = ಸುಳ್ಳು;  // Walking body of a process
    ತಾರ್ಕಿಕ m_canBeComb = ಸುಳ್ಳು;  // Whether current clocked process can be turned into a comb process

    // METHODS
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು T>
    ಶೂನ್ಯ moveUnderSpecial(Ast_ಗಂಟು* nodep) {
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ wantactivep = m_namer.get_Special_ಸಕ್ರಿಯ<T>(nodep->fileline());
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        wantactivep->ಕೂಡಿಸು_Stmtsp(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ_Always(Ast_ಗಂಟು* nodep, Ast_Sen_ವೃಕ್ಷ* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        ಆದರೆ (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }

        {
            ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;

            // Walk sensitivity list
            m_clockedProcess = ಸುಳ್ಳು;
            m_all_ಬದಲಾಯಿತು = ನಿಜ;
            ಆದರೆ (oldsentreep) {
                oldsentreep->unlink_Fr_ಹಿಂಭಾಗ();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            ಆದರೆ (m_all_ಬದಲಾಯಿತು) {
                ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
                m_walkingBody = ನಿಜ;
                m_canBeComb = ನಿಜ;
                iterateChildrenConst(nodep);
                m_walkingBody = ಸುಳ್ಳು;
                ಆದರೆ (m_canBeComb) m_clockedProcess = ಸುಳ್ಳು;
            }
        }

        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ wantactivep
            = !m_clockedProcess ? m_namer.get_Special_ಸಕ್ರಿಯ<Ast_Sen_ಅಂಶ::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_ಸಕ್ರಿಯ(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_ಸಕ್ರಿಯ<Ast_Sen_ಅಂಶ::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        wantactivep->ಕೂಡಿಸು_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        ಆದರೆ (!m_clockedProcess) ಸಕ್ರಿಯ_Dly_Visitor{nodep, ಸಕ್ರಿಯ_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        ಆದರೆ (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_ವೃಕ್ಷ(), oldsentreep);

        // check combinational processes for latches
        ಆದರೆ (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ಸ್ಥಿರಾಂಕ ಸಕ್ರಿಯ_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ_Sen_ವಸ್ತುಗಳು(Ast_ಗಂಟು* nodep) {
        ಆದರೆ (v3Global.opt.timing().is_ಗಣ_True()) {
            nodep->foreach([ಇದು](Ast_Sen_ಅಂಶ* senItemp) { ಭೇಟಿ(senItemp); });
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ mark_Event_ಅಂಚುಗಳು(Ast_Sen_ವೃಕ್ಷ* sentreep) {
        ಗಾಗಿ (Ast_Sen_ಅಂಶ* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), Sen_ಅಂಶ)) {
            ಆದರೆ (!senip->sensp()) ಮುಂದುವರಿ;
            ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = senip->sensp()->dtypep()) {
                ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp()) {
                    ಆದರೆ (basicp->isEvent()) senip->ಅಂಚು_Type(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        m_namer.ಮುಖ್ಯ(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstInitialStatic* nodep) override { moveUnderSpecial<Ast_Sen_ಅಂಶ::Static>(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstInitial* nodep) override {
        ಭೇಟಿ_Sen_ವಸ್ತುಗಳು(nodep);
        moveUnderSpecial<Ast_Sen_ಅಂಶ::Initial>(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFinal* nodep) override {
        ಸ್ಥಿರಾಂಕ ಸಕ್ರಿಯ_Dly_Visitor dlyvisitor{nodep, ಸಕ್ರಿಯ_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<Ast_Sen_ಅಂಶ::Final>(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCoverToggle* nodep) override { moveUnderSpecial<Ast_Sen_ಅಂಶ::Combo>(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        ಆದರೆ (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_ಹಿಂಭಾಗ()->delete_ವೃಕ್ಷ(), nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಆದರೆ (nodep->keyword() == VAlwaysKwd::CONT_ನಿಯೋಜಿಸು) {
            moveUnderSpecial<Ast_Sen_ಅಂಶ::Combo>(nodep);
            ಹಿಂತಿರುಗಿ;
        }
        ಭೇಟಿ_Sen_ವಸ್ತುಗಳು(nodep);
        ಭೇಟಿ_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = m_namer.make_Special_ಸಕ್ರಿಯ<Ast_Sen_ಅಂಶ::Combo>(nodep->fileline());
        activep->ಕೂಡಿಸು_Stmtsp(nodep->unlink_Fr_ಹಿಂಭಾಗ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_ಅಂಚುಗಳು(nodep->sentreep());
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = nodep->sentreep();
        sentreep->unlink_Fr_ಹಿಂಭಾಗ();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = m_namer.make_ಸಕ್ರಿಯ(nodep->fileline(), sentreep);
        activep->ಕೂಡಿಸು_Stmtsp(nodep->unlink_Fr_ಹಿಂಭಾಗ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_ಅಂಚುಗಳು(nodep->sentreep());
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ sentreep = nodep->sentreep();
        sentreep->unlink_Fr_ಹಿಂಭಾಗ();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = m_namer.make_ಸಕ್ರಿಯ(nodep->fileline(), sentreep);
        activep->ಕೂಡಿಸು_Stmtsp(nodep->unlink_Fr_ಹಿಂಭಾಗ());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override { ಭೇಟಿ_Sen_ವಸ್ತುಗಳು(nodep); }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_Sen_ಅಂಶ* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        ಆದರೆ (!nodep->sensp()) ಹಿಂತಿರುಗಿ;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = ನಿಜ;
        ಆದರೆ (nodep->ಅಂಚು_Type() != VEdgeType::ET_ಬದಲಾಯಿತು) m_all_ಬದಲಾಯಿತು = ಸುಳ್ಳು;

        ಆದರೆ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = nodep->sensp()->dtypep()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp()) {
                ಆದರೆ (basicp->isEvent()) nodep->ಅಂಚು_Type(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ಸ್ಥಿರಾಂಕ AstVarRef* refp) { refp->varScopep()->user1(ನಿಜ); });
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = nodep->varScopep();
        ಆದರೆ (nodep->access().isWriteOnly()) {
            vscp->user2(ನಿಜ);
        } ಇಲ್ಲದಿದ್ದರೆ {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = vscp->varp();
            ಆದರೆ (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = ಸುಳ್ಳು;
            }
        }
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Dly* nodep) override {
        m_canBeComb = ಸುಳ್ಳು;
        ಆದರೆ (nodep->isTimingControl()) m_clockedProcess = ನಿಜ;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFireEvent* nodep) override {
        m_canBeComb = ಸುಳ್ಳು;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ನಿಯೋಜಿಸು_Force* nodep) override {
        m_canBeComb = ಸುಳ್ಳು;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstRelease* nodep) override {
        m_canBeComb = ಸುಳ್ಳು;
        iterateChildrenConst(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstFork* nodep) override {
        ಆದರೆ (nodep->isTimingControl()) {
            m_canBeComb = ಸುಳ್ಳು;
            m_clockedProcess = ನಿಜ;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    ಶೂನ್ಯ ಭೇಟಿ(AstVar*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope*) override {}  // Accelerate
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override {
        ಆದರೆ (!v3Global.opt.timing().is_ಗಣ_True() && m_walkingBody && !m_canBeComb) {
            ಹಿಂತಿರುಗಿ;  // Accelerate
        }
        ಆದರೆ (!nodep->is_ಶುದ್ಧ()) m_canBeComb = ಸುಳ್ಳು;
        ಆದರೆ (nodep->isTimingControl()) {
            m_canBeComb = ಸುಳ್ಳು;
            m_clockedProcess = ನಿಜ;
            ಹಿಂತಿರುಗಿ;
        }
        iterateChildren(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ ಸಕ್ರಿಯ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ಸಕ್ರಿಯ_Visitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

ವರ್ಗ CovergroupCollectVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = ಶೂನ್ಯಸೂಚಕ;  // Current covergroup class context, or nullptr

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        ಆದರೆ (!nodep->isCovergroup()) ಹಿಂತಿರುಗಿ;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override { iterateChildren(nodep); }

    ಶೂನ್ಯ ಭೇಟಿ(AstCFunc* nodep) override {
        ಆದರೆ (!m_classp) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_ಹಿಂಭಾಗ());
        nodep->unlink_Fr_ಹಿಂಭಾಗ();
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

ವರ್ಗ CovergroupEventBindVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    AstVarScope* ಸ್ಥಿರಾಂಕ m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVar*> m_memberps;  // Non-static variables in the covergroup class

    ಶೂನ್ಯ ಭೇಟಿ(AstVarRef* nodep) override {
        ಆದರೆ (!m_memberps.ಎಣಿಕೆ_2(nodep->varp())) ಹಿಂತಿರುಗಿ;
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        AstMemberSel* ಸ್ಥಿರಾಂಕ selp
            = ಹೊಸ AstMemberSel{fl, ಹೊಸ AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    CovergroupEventBindVisitor(Ast_Sen_ವೃಕ್ಷ* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ಸ್ಥಿರಾಂಕ, AstVar* ಸ್ಥಿರಾಂಕ varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

ವರ್ಗ CovergroupInjectVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ಸಕ್ರಿಯ_Namer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        m_namer.ಮುಖ್ಯ(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ಸ್ಥಿರಾಂಕ varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ಸ್ಥಿರಾಂಕ AstClassRefDType* ಸ್ಥಿರಾಂಕ classRefp = VN_CAST(dtypep, ClassRefDType);
        ಆದರೆ (!classRefp) ಹಿಂತಿರುಗಿ;

        AstClass* ಸ್ಥಿರಾಂಕ classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ eventp = VN_CAST(classp->user2p(), Sen_ವೃಕ್ಷ);
        ಆದರೆ (!eventp) ಹಿಂತಿರುಗಿ;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ಸ್ಥಿರಾಂಕ sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        AstVarRef* ಸ್ಥಿರಾಂಕ varrefp = ಹೊಸ AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ಸ್ಥಿರಾಂಕ cmethodCallp
            = ಹೊಸ AstCMethodCall{fl, varrefp, sampleCFuncp, ಶೂನ್ಯಸೂಚಕ};

        cmethodCallp->dtype_ಗಣ_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_ವೃಕ್ಷ* senTreep = eventp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = m_namer.get_ಸಕ್ರಿಯ(fl, senTreep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->ಕೂಡಿಸು_Stmtsp(
            ಹೊಸ AstAlways{fl, VAlwaysKwd::ALWAYS_FF, ಶೂನ್ಯಸೂಚಕ, cmethodCallp->makeStmt()});
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        ಆದರೆ (Ast_Sen_ವೃಕ್ಷ* ಸ್ಥಿರಾಂಕ eventp = VN_CAST(nodep->user2p(), Sen_ವೃಕ್ಷ)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(eventp), eventp);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಸಕ್ರಿಯ*) override {}  // Don't iterate into actives
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;
};

//######################################################################
// Active class functions

ಶೂನ್ಯ V3_ಸಕ್ರಿಯ::ಸಕ್ರಿಯ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ಸಕ್ರಿಯ_Visitor{nodep}; }  // Destruct before checking
    ಆದರೆ (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
        ಸ್ಥಿರಾಂಕ VNUser2_ಒಳ_Use user2_ಒಳ_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_ವೃಕ್ಷ("active", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
