// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Scheduling
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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3SCHED_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3SCHED_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"

#ಅನ್ತರ್ಭಾವ <array>
#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>
#ಅನ್ತರ್ಭಾವ <utility>
#ಅನ್ತರ್ಭಾವ <vector>

class Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using Var_Scope_ಗಣ = std::unordered_ಗಣ<ಸ್ಥಿರಾಂಕ AstVarScope*>;

ಸಾಲೊಳಗೆ bool isVlForceVec(ಸ್ಥಿರಾಂಕ AstVarScope* vscp) {
    ಸ್ಥಿರಾಂಕ AstCDType* ಸ್ಥಿರಾಂಕ dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    ಹಿಂತಿರುಗಿ dtypep && dtypep->name() == "VlForceVec";
}

ಸಾಲೊಳಗೆ bool is_Force_Read_ವಿಧಾನ(VCMethod ವಿಧಾನ) {
    ಹಿಂತಿರುಗಿ ವಿಧಾನ == VCMethod::FORCE_READ || ವಿಧಾನ == VCMethod::FORCE_READ_ಸೂಚ್ಯಂಕ;
}

ಸಾಲೊಳಗೆ ಶೂನ್ಯ collect_Force_Read_ಅಂಚು_Ignores(Ast_ಗಂಟು* nodep, Var_Scope_ಗಣ& ಹೊರಹೋಗು) {
    Var_Scope_ಗಣ writtenForceVecs;
    Var_Scope_ಗಣ writtenVars;

    nodep->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
        AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
        ಆದರೆ (!refp->access().isWriteOrRW()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        ಆದರೆ (!isVlForceVec(vscp)) ಹಿಂತಿರುಗಿ;
        writtenForceVecs.emplace(vscp);
        ಆದರೆ (!refp->varp()->ignoreSchedWrite()) ಹೊರಹೋಗು.emplace(vscp);
    });

    ಆದರೆ (writtenForceVecs.ಖಾಲಿಗಳು() || writtenVars.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

    nodep->foreach([&](ಸ್ಥಿರಾಂಕ AstCMethodHard* callp) {
        ಆದರೆ (!is_Force_Read_ವಿಧಾನ(callp->ವಿಧಾನ())) ಹಿಂತಿರುಗಿ;

        ಸ್ಥಿರಾಂಕ AstVarRef* ಸ್ಥಿರಾಂಕ fromRefp = VN_CAST(callp->fromp(), VarRef);
        ಆದರೆ (!fromRefp || !writtenForceVecs.ಎಣಿಕೆ_2(fromRefp->varScopep())) ಹಿಂತಿರುಗಿ;

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ origp = callp->pinsp();
        ಆದರೆ (!origp) ಹಿಂತಿರುಗಿ;
        origp->foreach([&](ಸ್ಥಿರಾಂಕ AstVarRef* refp) {
            AstVarScope* ಸ್ಥಿರಾಂಕ vscp = refp->varScopep();
            ಆದರೆ (refp->access().isReadOrRW() && writtenVars.ಎಣಿಕೆ_2(vscp)) ಹೊರಹೋಗು.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    // An AstActive and the AstScope it is under
    ರಚನೆ ನಮೂದು_2 final {
        AstScope* m_scopep;  // The scope the AstActive is under
        Ast_ಸಕ್ರಿಯ* m_activep;  // The AstActive holding the logic
        std::ಗಣ<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::ಸದಿಶ<ನಮೂದು_2>::iterator;
    using const_iterator = std::ಸದಿಶ<ನಮೂದು_2>::const_iterator;

ಖಾಸಗಿ:
    // STATE
    std::ಸದಿಶ<ನಮೂದು_2> m_entries;  // The AstActives held, with their scopes

ಸಾರ್ವಜನಿಕ:
    // METHODS

    // Iteration and access
    iterator begin() { ಹಿಂತಿರುಗಿ m_entries.begin(); }
    iterator ಕೊನೆ() { ಹಿಂತಿರುಗಿ m_entries.ಕೊನೆ(); }
    const_iterator begin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_entries.begin(); }
    const_iterator ಕೊನೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_entries.ಕೊನೆ(); }
    bool ಖಾಲಿಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_entries.ಖಾಲಿಗಳು(); }
    ಗಾತ್ರ_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_entries.ಗಾತ್ರ_2(); }
    ಸ್ಥಿರಾಂಕ ನಮೂದು_2& ಕ್ರಿಯಾಕಾರಕ[](ಗಾತ್ರ_t i) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_entries[i]; }

    // Add an AstActive under the given scope
    ಶೂನ್ಯ ಕೂಡಿಸು(AstScope* scopep, Ast_ಸಕ್ರಿಯ* activep) { m_entries.ತಳ್ಳು_ಹಿಂಭಾಗ({scopep, activep, {}}); }

    // Add logic
    ಶೂನ್ಯ ಕೂಡಿಸು(AstScope* scopep, Ast_Sen_ವೃಕ್ಷ* senTreep, Ast_ಗಂಟು* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        ಆದರೆ (m_entries.ಖಾಲಿಗಳು() || m_entries.ಹಿಂಭಾಗ().m_scopep != scopep
            || m_entries.ಹಿಂಭಾಗ().m_activep->sentreep() != senTreep) {
            ಕೂಡಿಸು(scopep, new Ast_ಸಕ್ರಿಯ{logicp->fileline(), "", senTreep});
        }
        m_entries.ಹಿಂಭಾಗ().m_activep->ಕೂಡಿಸು_Stmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() ಸ್ಥಿರಾಂಕ {
        LogicByScope ಫಲಿತಾಂಶ;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ನಮೂದು_2& ನಮೂದು : m_entries) {
            ಫಲಿತಾಂಶ.ಕೂಡಿಸು(ನಮೂದು.m_scopep, ನಮೂದು.m_activep->clone_ವೃಕ್ಷ(false));
        }
        ಹಿಂತಿರುಗಿ ಫಲಿತಾಂಶ;
    }

    // Delete actives (they should all be empty)
    ಶೂನ್ಯ deleteActives() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ನಮೂದು_2& ನಮೂದು : m_entries) {
            Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
            ಆದರೆ (v3Global.opt.debugCheck()) {
                ಗಾಗಿ (Ast_ಗಂಟು* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_ಗಂಟು_Procedure* ಸ್ಥಿರಾಂಕ procp = VN_CAST(nodep, ಗಂಟು_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            ಆದರೆ (activep->backp()) activep->unlink_Fr_ಹಿಂಭಾಗ();
            VL_DO_DANGLING(activep->delete_ವೃಕ್ಷ(), activep);
        }
        m_entries.clear();
    };

    ಶೂನ್ಯ foreachLogic(ಸ್ಥಿರಾಂಕ std::function<ಶೂನ್ಯ(Ast_ಗಂಟು*)>& f) ಸ್ಥಿರಾಂಕ {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ನಮೂದು_2& ನಮೂದು : m_entries) {
            ಸ್ಥಿರಾಂಕ Ast_ಸಕ್ರಿಯ* ಸ್ಥಿರಾಂಕ activep = ನಮೂದು.m_activep;
            ಗಾಗಿ (Ast_ಗಂಟು* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
ರಚನೆ LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    LogicClasses& ಕ್ರಿಯಾಕಾರಕ=(LogicClasses&&) = ಪೂರ್ವನಿಯೋಜಿತ;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
ರಚನೆ LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    LogicRegions& ಕ್ರಿಯಾಕಾರಕ=(LogicRegions&&) = ಪೂರ್ವನಿಯೋಜಿತ;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
ರಚನೆ LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = ಪೂರ್ವನಿಯೋಜಿತ;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    LogicReplicas& ಕ್ರಿಯಾಕಾರಕ=(LogicReplicas&&) = ಪೂರ್ವನಿಯೋಜಿತ;
};

// A TriggerKit holds all the components related to a scheduling region triggers
//
// Each piece of code is executed when some trigger bits are set. There is no
// code after scheduling which is not conditional on at least one trigger bit.
//
// There are 3 different kinds of triggers
// 1. "Sense" triggers, which correspond to a unique SenItem in the design
// 2. "Extra" triggers, which represent non-SenItem based conditions
// 3. "Pre" triggers, which are only used in the 'act' region. These are a copy
//    of some of the "Sense" triggers but only ever fire during one evaluation
//    of the 'act' loop. They are used for executing AlwaysPre block that
//    reside in the 'act' region. (AlwaysPre in 'nba' use the "Sense" triggers.)
//
// All trigger bits are stored in an unpacked array of fixed sized words, which
// is informally referred to in various places as the "trigger vector".
// There is only one trigger vector for each eval loop (~scheduling region).
//
// The organization of the trigger vector (array) is shown here, LSB on the
// right, MSB on the left. There are 4 main sections:
//
// | <- bit N-1                                                                         bit 0 -> |
// +--------------------+----------------+----------------------------------+--------------------+
// | Pre triggers       | Extra triggers | Sense triggers                   | Pre Sense triggers |
// +--------------------+----------------+----------------------------------+--------------------+
// |        'pre'       |                                 'vec'                                  |
//
// The section labelled "Pre Sense triggers" contains regular "Sense" triggers
// that are also duplicated in the "Pre triggers" section the first time they
// fire.
//
// The "Sense triggers" section contains the rest of the "Sense" triggers that
// do not need a copy in "Pre triggers".
//
// The "Sense triggers" and "Pre Sense triggers" together contain all "Sense"
// triggers described in point 1 above. "Extra triggers" contains the
// non-SenItem based additional conditions described in point 2, and the
// "Per triggers" section contains the "Pre" triggers described in point 3.
//
// All 4 sections in the trigger vector are padded to contain a whole word
// worth of bits (padding bits are always zero). Any one of the 4 sections
// can be empty, but "Pre triggers" and "Pre Sense triggers" are always the
// same size.
//
// The portion holding the Sense triggers and Extra triggers is referred to
// in various places as the 'vec' part, and is also informally referred to as
// the trigger vector.
//
// The portion holding the Pre trigger is named 'pre'
//
// The combination of 'pre' + 'vec' is the "extended trigger vector" referred
// to as 'ext' in various places.
//
// In realistic designs there are often no "Pre" triggers, and only a few
// "Extra" triggers, with "Sense" triggers taking up the bulk of the bits.
//
class TriggerKit final {
ಸಾರ್ವಜನಿಕ:
    // Triggers are storead as an UnpackedArray with a fixed word size
    ಸ್ಥಿರ constexpr uint32_t ಶಬ್ದ_ಗಾತ್ರ_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    ಸ್ಥಿರ constexpr uint32_t ಶಬ್ದ_ಗಾತ್ರ = 1 << ಶಬ್ದ_ಗಾತ್ರ_LOG2;

ಖಾಸಗಿ:
    ಸ್ಥಿರಾಂಕ std::string m_name;  // TriggerKit name
    ಸ್ಥಿರಾಂಕ bool m_slow;  // TriggerKit is for schedulign 'slow' code
    ಸ್ಥಿರಾಂಕ uint32_t m_n_Sense_ಪದಗಳು;  // Number of words for Sense triggers
    ಸ್ಥಿರಾಂಕ uint32_t m_n_Extra_ಪದಗಳು;  // Number of words for Extra triggers
    ಸ್ಥಿರಾಂಕ uint32_t m_n_Pre_ಪದಗಳು;  // Number of words for 'pre' part
    ಸ್ಥಿರಾಂಕ uint32_t m_n_Vec_ಪದಗಳು = m_n_Sense_ಪದಗಳು + m_n_Extra_ಪದಗಳು;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ>, ಗಾತ್ರ_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ*> m_trig_Idx2_Sen_ಅಂಶ;
    // Data type of a single trigger word
    Ast_ಗಂಟು_DType* m_ಶಬ್ದ_DTypep = nullptr;
    // Data type of a trigger vector holding one copy of all triggers
    AstUnpackArrayDType* m_trigVecDTypep = nullptr;
    // Data type of an extended trigger vector holding one copy of all triggers
    // + additional copy of 'pre' triggers
    AstUnpackArrayDType* m_trigExtDTypep = nullptr;
    // The AstVarScope representing the extended trigger vector
    AstVarScope* m_vscp = nullptr;
    // The AstVarScope representing the trigger accumulator vector
    // It is used to accumulate triggers that were found fired and cleared in beforeTrigger's
    // in current 'act' region iteration
    AstVarScope* m_vscAccp = nullptr;
    // The AstCFunc that computes the current active base triggers
    AstCFunc* m_compVecp = nullptr;
    // The AstCFunc that computes the current active extended triggers
    AstCFunc* m_compExtp = nullptr;
    // The AstCFunc that dumps a trigger vector
    AstCFunc* m_dumpp = nullptr;
    // The AstCFunc that dumps an exended trigger vector - create lazily
    mutable AstCFunc* m_dumpExtp = nullptr;
    // The AstCFunc testing if a trigger vector has any bits set - create lazily
    mutable AstCFunc* m_any_ಗಣ_Vecp = nullptr;
    mutable AstCFunc* m_any_ಗಣ_Extp = nullptr;
    // The AstCFunc setting bits in a trigger vector that are set in another - create lazily
    mutable std::array<AstCFunc*, 4> m_orIntoVecps = {nullptr};
    // The AstCFunc setting a trigger vector to all zeroes - create lazily
    mutable AstCFunc* m_clearp = nullptr;

    // Methods to lazy construct functions processing trigger vectors
    AstCFunc* createDumpExtFunc() ಸ್ಥಿರಾಂಕ;
    AstCFunc* create_Any_ಗಣ_Func(AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ dtypep) ಸ್ಥಿರಾಂಕ;
    AstCFunc* createClearFunc() ಸ್ಥಿರಾಂಕ;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ oDtypep,
                               AstUnpackArrayDType* ಸ್ಥಿರಾಂಕ iDtypep) ಸ್ಥಿರಾಂಕ;

    TriggerKit(ಸ್ಥಿರಾಂಕ std::string& name, bool slow, uint32_t n_Sense_ಪದಗಳು, uint32_t n_Extra_ಪದಗಳು,
               uint32_t n_Pre_ಪದಗಳು,
               std::unordered_map<VNRef<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ>, ಗಾತ್ರ_t> senItem2TrigIdx,
               ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

ಸಾರ್ವಜನಿಕ:
    // Move constructible
    TriggerKit(TriggerKit&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    ~TriggerKit() = ಪೂರ್ವನಿಯೋಜಿತ;
    TriggerKit& ಕ್ರಿಯಾಕಾರಕ=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        ಮಿತ್ರ class TriggerKit;
        std::ಸದಿಶ<string> m_descriptions;  // Human readable description of extra triggers

    ಸಾರ್ವಜನಿಕ:
        ExtraTriggers() = ಪೂರ್ವನಿಯೋಜಿತ;
        ~ExtraTriggers() = ಪೂರ್ವನಿಯೋಜಿತ;

        uint32_t allocate(ಸ್ಥಿರಾಂಕ string& description) {
            m_descriptions.ತಳ್ಳು_ಹಿಂಭಾಗ(description);
            ಹಿಂತಿರುಗಿ m_descriptions.ಗಾತ್ರ_2() - 1;
        }
        uint32_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_descriptions.ಗಾತ್ರ_2(); }
    };
    // Generates list of assignments that fills
    ಸ್ಥಿರ Ast_ನಿಯೋಜಿಸು* createSenTrigVecAssignment(AstVarScope* ಸ್ಥಿರಾಂಕ ಗುರಿ,
                                                 std::ಸದಿಶ<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    ಸ್ಥಿರ TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* ಸ್ಥಿರಾಂಕ initFuncp,  //
                             Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,  //
                             ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ*>& preTreeps,  //
                             ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ*>& senTreeps,  //
                             ಸ್ಥಿರಾಂಕ string& name,  //
                             ಸ್ಥಿರಾಂಕ ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp; }
    AstVarScope* vscAccp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscAccp; }
    ಗಾತ್ರ_t senItem2TrigIdx(ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* senItemp) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_senItem2TrigIdx.at(*senItemp);
    }
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ*>& trig_Idx2_Sen_ಅಂಶ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_trig_Idx2_Sen_ಅಂಶ; }
    AstCFunc* compBasep() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_compVecp; }
    ಸ್ಥಿರಾಂಕ std::string& name() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_ಗಂಟು_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Any_ಗಣ_Call(AstVarScope* vscp) ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_Stmt* newClearCall(AstVarScope* vscp) ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) ಸ್ಥಿರಾಂಕ;
    // Helpers for code generation
    Ast_ಗಂಟು_Stmt* new_Comp_ತಳ_Call() ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_Stmt* newCompExtCall(AstVarScope* vscp) ಸ್ಥಿರಾಂಕ;
    Ast_ಗಂಟು_Stmt* newDumpCall(AstVarScope* vscp, ಸ್ಥಿರಾಂಕ std::string& ಟ್ಯಾಗ್, bool debugOnly) ಸ್ಥಿರಾಂಕ;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(ಸ್ಥಿರಾಂಕ std::string& name) ಸ್ಥಿರಾಂಕ;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extra_Trigger_ಸೂಚ್ಯಂಕ(uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::ಗಣ<uint32_t> triggerIndices(ಸ್ಥಿರಾಂಕ Ast_Sen_ವೃಕ್ಷ* senTreep, bool pre) ಸ್ಥಿರಾಂಕ;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_ಗಂಟು_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    ಶೂನ್ಯ ಕೂಡಿಸು_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    ಶೂನ್ಯ ಕೂಡಿಸು_ಮೌಲ್ಯ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಗಣ<Ast_Sen_ವೃಕ್ಷ*>> m_externalDomains;

ಸಾರ್ವಜನಿಕ:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_ಗಂಟು_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಸದಿಶ<uint32_t>>
    externalTriggerIndices(ಸ್ಥಿರಾಂಕ TriggerKit& trigKit) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* ಸ್ಥಿರಾಂಕ netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = ಪೂರ್ವನಿಯೋಜಿತ;
    TimingKit(LogicByScope&& lbs, Ast_ಗಂಟು_Stmt* postUpdates,
              std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, std::ಗಣ<Ast_Sen_ವೃಕ್ಷ*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    TimingKit& ಕ್ರಿಯಾಕಾರಕ=(TimingKit&&) = ಪೂರ್ವನಿಯೋಜಿತ;
};

class VirtIfaceTriggers final {
ಸಾರ್ವಜನಿಕ:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    ರಚನೆ Trigger_ನಮೂದು final {
        ಸ್ಥಿರಾಂಕ AstIface* m_ifacep;  // Interface type
        ಸ್ಥಿರಾಂಕ AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::ಸದಿಶ<Trigger_ನಮೂದು> m_triggers;

    ಶೂನ್ಯ ಕೂಡಿಸು_Trigger(ಸ್ಥಿರಾಂಕ AstIface* ifacep, ಸ್ಥಿರಾಂಕ AstVar* memberp, AstVarScope* vscp) {
        m_triggers.ತಳ್ಳು_ಹಿಂಭಾಗ(Trigger_ನಮೂದು{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<ಸ್ಥಿರಾಂಕ AstVarScope*, uint32_t>;
    VscpTrigMap make_Vscp_ಗಮ್ಯ_Trig_Map(ಸ್ಥಿರಾಂಕ TriggerKit& trigKit, uint32_t ಮೊದಲ_ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = ಪೂರ್ವನಿಯೋಜಿತ;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    VirtIfaceTriggers& ಕ್ರಿಯಾಕಾರಕ=(VirtIfaceTriggers&&) = ಪೂರ್ವನಿಯೋಜಿತ;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
ಸಾರ್ವಜನಿಕ:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::ಸದಿಶ<AstVarScope*>;

ಖಾಸಗಿ:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<ಸ್ಥಿರಾಂಕ AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_ಅಂಕೆ_Exact_Calls = 0;
    mutable uint32_t m_ಅಂಕೆ_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Bindings s_none;

ಸಾರ್ವಜನಿಕ:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    ಶೂನ್ಯ ಕೂಡಿಸು_Construction(ಸ್ಥಿರಾಂಕ AstClass* classp, ಸ್ಥಿರಾಂಕ AstVarScope* instp,
                         ಸ್ಥಿರಾಂಕ Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    ಶೂನ್ಯ dropInstance(ಸ್ಥಿರಾಂಕ AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    ಸ್ಥಿರಾಂಕ Bindings& forSample(ಸ್ಥಿರಾಂಕ AstVarScope* instp, ಸ್ಥಿರಾಂಕ AstClass* classp) ಸ್ಥಿರಾಂಕ;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t ಅಂಕೆ_Exact_Calls() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಕೆ_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t ಅಂಕೆ_Union_Calls() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಅಂಕೆ_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = ಪೂರ್ವನಿಯೋಜಿತ;
    CovergroupRefBindings(CovergroupRefBindings&&) = ಪೂರ್ವನಿಯೋಜಿತ;
    CovergroupRefBindings& ಕ್ರಿಯಾಕಾರಕ=(CovergroupRefBindings&&) = ಪೂರ್ವನಿಯೋಜಿತ;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
ಸ್ಥಿರಾಂಕ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* ಸ್ಥಿರಾಂಕ netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
ಶೂನ್ಯ transformForks(AstNetlist* ಸ್ಥಿರಾಂಕ netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
ಶೂನ್ಯ ವೇಳಾಪಟ್ಟಿ(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope break_ಚಕ್ರಗಳು(AstNetlist* netlistp,
                         ಸ್ಥಿರಾಂಕ LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions ವಿಭಜನೆ(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, ಸ್ಥಿರಾಂಕ string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* new_ವಾದ(AstCFunc* funcp, Ast_ಗಂಟು_DType* dtypep, ಸ್ಥಿರಾಂಕ string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_ಗಂಟು_Stmt* ಗಣ_Var(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_ಗಂಟು_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_ಗಂಟು_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
ಶೂನ್ಯ ವಿಭಜಿಸು_Check(AstCFunc* ofuncp);
}  // namespace util

ಶೂನ್ಯ beforeTrigVisitor(AstNetlist* netlistp, Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,
                       ಸ್ಥಿರಾಂಕ TriggerKit& trigKit);

}  // namespace V3Sched

#ಆದರೆ_ಅಂತ್ಯ  // Guard
