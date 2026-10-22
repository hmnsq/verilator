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

#ifndef VERILATOR_V3SCHED_H_
#define VERILATOR_V3SCHED_H_

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
using VarScopeSet = std::unordered_set<ಧ್ರುವ AstVarScope*>;

ಅನ್ತಃಸ್ಥ bool isVlForceVec(ಧ್ರುವ AstVarScope* vscp) {
    ಧ್ರುವ AstCDType* ಧ್ರುವ dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    ಪ್ರತ್ಯಾಗಮ dtypep && dtypep->name() == "VlForceVec";
}

ಅನ್ತಃಸ್ಥ bool isForceReadMethod(VCMethod method) {
    ಪ್ರತ್ಯಾಗಮ method == VCMethod::FORCE_READ || method == VCMethod::FORCE_READ_INDEX;
}

ಅನ್ತಃಸ್ಥ ಶೂನ್ಯ collectForceReadEdgeIgnores(Ast_ಗಂಟು* nodep, VarScopeSet& out) {
    VarScopeSet writtenForceVecs;
    VarScopeSet writtenVars;

    nodep->foreach([&](ಧ್ರುವ AstVarRef* refp) {
        AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
        ಯದಿ (!refp->access().isWriteOrRW()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        ಯದಿ (!isVlForceVec(vscp)) ಪ್ರತ್ಯಾಗಮ;
        writtenForceVecs.emplace(vscp);
        ಯದಿ (!refp->varp()->ignoreSchedWrite()) out.emplace(vscp);
    });

    ಯದಿ (writtenForceVecs.empty() || writtenVars.empty()) ಪ್ರತ್ಯಾಗಮ;

    nodep->foreach([&](ಧ್ರುವ AstCMethodHard* callp) {
        ಯದಿ (!isForceReadMethod(callp->method())) ಪ್ರತ್ಯಾಗಮ;

        ಧ್ರುವ AstVarRef* ಧ್ರುವ fromRefp = VN_CAST(callp->fromp(), VarRef);
        ಯದಿ (!fromRefp || !writtenForceVecs.ಎಣಿಕೆ(fromRefp->varScopep())) ಪ್ರತ್ಯಾಗಮ;

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ origp = callp->pinsp();
        ಯದಿ (!origp) ಪ್ರತ್ಯಾಗಮ;
        origp->foreach([&](ಧ್ರುವ AstVarRef* refp) {
            AstVarScope* ಧ್ರುವ vscp = refp->varScopep();
            ಯದಿ (refp->access().isReadOrRW() && writtenVars.ಎಣಿಕೆ(vscp)) out.emplace(vscp);
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
    ಸಂರಚನಾ Entry final {
        AstScope* m_scopep;  // The scope the AstActive is under
        AstActive* m_activep;  // The AstActive holding the logic
        std::set<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::vector<Entry>::iterator;
    using const_iterator = std::vector<Entry>::const_iterator;

ಗುಪ್ತ:
    // STATE
    std::vector<Entry> m_entries;  // The AstActives held, with their scopes

ಸಾರ್ವಜನಿಕ:
    // METHODS

    // Iteration and access
    iterator begin() { ಪ್ರತ್ಯಾಗಮ m_entries.begin(); }
    iterator end() { ಪ್ರತ್ಯಾಗಮ m_entries.end(); }
    const_iterator begin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_entries.begin(); }
    const_iterator end() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_entries.end(); }
    bool empty() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_entries.empty(); }
    size_t size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_entries.size(); }
    ಧ್ರುವ Entry& operator[](size_t i) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_entries[i]; }

    // Add an AstActive under the given scope
    ಶೂನ್ಯ add(AstScope* scopep, AstActive* activep) { m_entries.ತಳ್ಳು_back({scopep, activep, {}}); }

    // Add logic
    ಶೂನ್ಯ add(AstScope* scopep, Ast_Sen_ವೃಕ್ಷ* senTreep, Ast_ಗಂಟು* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        ಯದಿ (m_entries.empty() || m_entries.back().m_scopep != scopep
            || m_entries.back().m_activep->sentreep() != senTreep) {
            add(scopep, new AstActive{logicp->fileline(), "", senTreep});
        }
        m_entries.back().m_activep->addStmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() ಧ್ರುವ {
        LogicByScope result;
        ಪ್ರತಿ (ಧ್ರುವ Entry& entry : m_entries) {
            result.add(entry.m_scopep, entry.m_activep->clone_ವೃಕ್ಷ(false));
        }
        ಪ್ರತ್ಯಾಗಮ result;
    }

    // Delete actives (they should all be empty)
    ಶೂನ್ಯ deleteActives() {
        ಪ್ರತಿ (ಧ್ರುವ Entry& entry : m_entries) {
            AstActive* ಧ್ರುವ activep = entry.m_activep;
            ಯದಿ (v3Global.opt.debugCheck()) {
                ಪ್ರತಿ (Ast_ಗಂಟು* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_ಗಂಟು_Procedure* ಧ್ರುವ procp = VN_CAST(nodep, ಗಂಟು_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            ಯದಿ (activep->backp()) activep->unlinkFrBack();
            VL_DO_DANGLING(activep->delete_ವೃಕ್ಷ(), activep);
        }
        m_entries.clear();
    };

    ಶೂನ್ಯ foreachLogic(ಧ್ರುವ std::function<ಶೂನ್ಯ(Ast_ಗಂಟು*)>& f) ಧ್ರುವ {
        ಪ್ರತಿ (ಧ್ರುವ Entry& entry : m_entries) {
            ಧ್ರುವ AstActive* ಧ್ರುವ activep = entry.m_activep;
            ಪ್ರತಿ (Ast_ಗಂಟು* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
ಸಂರಚನಾ LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = ಸ್ವಭಾವ;
    LogicClasses& operator=(LogicClasses&&) = ಸ್ವಭಾವ;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
ಸಂರಚನಾ LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = ಸ್ವಭಾವ;
    LogicRegions& operator=(LogicRegions&&) = ಸ್ವಭಾವ;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
ಸಂರಚನಾ LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = ಸ್ವಭಾವ;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = ಸ್ವಭಾವ;
    LogicReplicas& operator=(LogicReplicas&&) = ಸ್ವಭಾವ;
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
    ಸ್ಥಿರ constexpr uint32_t WORD_SIZE_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    ಸ್ಥಿರ constexpr uint32_t WORD_SIZE = 1 << WORD_SIZE_LOG2;

ಗುಪ್ತ:
    ಧ್ರುವ std::string m_name;  // TriggerKit name
    ಧ್ರುವ bool m_slow;  // TriggerKit is for schedulign 'slow' code
    ಧ್ರುವ uint32_t m_n_Sense_ಪದಗಳು;  // Number of words for Sense triggers
    ಧ್ರುವ uint32_t m_n_Extra_ಪದಗಳು;  // Number of words for Extra triggers
    ಧ್ರುವ uint32_t m_n_Pre_ಪದಗಳು;  // Number of words for 'pre' part
    ಧ್ರುವ uint32_t m_n_Vec_ಪದಗಳು = m_n_Sense_ಪದಗಳು + m_n_Extra_ಪದಗಳು;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<ಧ್ರುವ AstSenItem>, size_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::vector<ಧ್ರುವ AstSenItem*> m_trigIdx2SenItem;
    // Data type of a single trigger word
    Ast_ಗಂಟು_DType* m_wordDTypep = nullptr;
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
    mutable AstCFunc* m_anySetVecp = nullptr;
    mutable AstCFunc* m_anySetExtp = nullptr;
    // The AstCFunc setting bits in a trigger vector that are set in another - create lazily
    mutable std::array<AstCFunc*, 4> m_orIntoVecps = {nullptr};
    // The AstCFunc setting a trigger vector to all zeroes - create lazily
    mutable AstCFunc* m_clearp = nullptr;

    // Methods to lazy construct functions processing trigger vectors
    AstCFunc* createDumpExtFunc() ಧ್ರುವ;
    AstCFunc* createAnySetFunc(AstUnpackArrayDType* ಧ್ರುವ dtypep) ಧ್ರುವ;
    AstCFunc* createClearFunc() ಧ್ರುವ;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* ಧ್ರುವ oDtypep,
                               AstUnpackArrayDType* ಧ್ರುವ iDtypep) ಧ್ರುವ;

    TriggerKit(ಧ್ರುವ std::string& name, bool slow, uint32_t n_Sense_ಪದಗಳು, uint32_t n_Extra_ಪದಗಳು,
               uint32_t n_Pre_ಪದಗಳು,
               std::unordered_map<VNRef<ಧ್ರುವ AstSenItem>, size_t> senItem2TrigIdx,
               ಧ್ರುವ std::vector<ಧ್ರುವ AstSenItem*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

ಸಾರ್ವಜನಿಕ:
    // Move constructible
    TriggerKit(TriggerKit&&) = ಸ್ವಭಾವ;
    ~TriggerKit() = ಸ್ವಭಾವ;
    TriggerKit& operator=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        friend class TriggerKit;
        std::vector<string> m_descriptions;  // Human readable description of extra triggers

    ಸಾರ್ವಜನಿಕ:
        ExtraTriggers() = ಸ್ವಭಾವ;
        ~ExtraTriggers() = ಸ್ವಭಾವ;

        uint32_t allocate(ಧ್ರುವ string& description) {
            m_descriptions.ತಳ್ಳು_back(description);
            ಪ್ರತ್ಯಾಗಮ m_descriptions.size() - 1;
        }
        uint32_t size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_descriptions.size(); }
    };
    // Generates list of assignments that fills
    ಸ್ಥಿರ AstAssign* createSenTrigVecAssignment(AstVarScope* ಧ್ರುವ target,
                                                 std::vector<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    ಸ್ಥಿರ TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* ಧ್ರುವ initFuncp,  //
                             Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,  //
                             ಧ್ರುವ std::vector<ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ*>& preTreeps,  //
                             ಧ್ರುವ std::vector<ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ*>& senTreeps,  //
                             ಧ್ರುವ string& name,  //
                             ಧ್ರುವ ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscp; }
    AstVarScope* vscAccp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vscAccp; }
    size_t senItem2TrigIdx(ಧ್ರುವ AstSenItem* senItemp) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_senItem2TrigIdx.at(*senItemp);
    }
    ಧ್ರುವ std::vector<ಧ್ರುವ AstSenItem*>& trigIdx2SenItem() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_trigIdx2SenItem; }
    AstCFunc* compBasep() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_compVecp; }
    ಧ್ರುವ std::string& name() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_ಗಂಟು_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) ಧ್ರುವ;
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newAnySetCall(AstVarScope* vscp) ಧ್ರುವ;
    Ast_ಗಂಟು_Stmt* newClearCall(AstVarScope* vscp) ಧ್ರುವ;
    Ast_ಗಂಟು_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) ಧ್ರುವ;
    // Helpers for code generation
    Ast_ಗಂಟು_Stmt* newCompBaseCall() ಧ್ರುವ;
    Ast_ಗಂಟು_Stmt* newCompExtCall(AstVarScope* vscp) ಧ್ರುವ;
    Ast_ಗಂಟು_Stmt* newDumpCall(AstVarScope* vscp, ಧ್ರುವ std::string& tag, bool debugOnly) ಧ್ರುವ;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(ಧ್ರುವ std::string& name) ಧ್ರುವ;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extraTriggerIndex(uint32_t index) ಧ್ರುವ;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::set<uint32_t> triggerIndices(ಧ್ರುವ Ast_Sen_ವೃಕ್ಷ* senTreep, bool pre) ಧ್ರುವ;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_ಗಂಟು_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ಧ್ರುವ;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    ಶೂನ್ಯ addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ಧ್ರುವ;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    ಶೂನ್ಯ add_ಮೌಲ್ಯ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t index) ಧ್ರುವ;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<ಧ್ರುವ AstVarScope*, std::set<Ast_Sen_ವೃಕ್ಷ*>> m_externalDomains;

ಸಾರ್ವಜನಿಕ:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_ಗಂಟು_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<ಧ್ರುವ AstVarScope*, std::vector<uint32_t>>
    externalTriggerIndices(ಧ್ರುವ TriggerKit& trigKit) ಧ್ರುವ VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* ಧ್ರುವ netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = ಸ್ವಭಾವ;
    TimingKit(LogicByScope&& lbs, Ast_ಗಂಟು_Stmt* postUpdates,
              std::map<ಧ್ರುವ AstVarScope*, std::set<Ast_Sen_ವೃಕ್ಷ*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = ಸ್ವಭಾವ;
    TimingKit& operator=(TimingKit&&) = ಸ್ವಭಾವ;
};

class VirtIfaceTriggers final {
ಸಾರ್ವಜನಿಕ:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    ಸಂರಚನಾ TriggerEntry final {
        ಧ್ರುವ AstIface* m_ifacep;  // Interface type
        ಧ್ರುವ AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::vector<TriggerEntry> m_triggers;

    ಶೂನ್ಯ addTrigger(ಧ್ರುವ AstIface* ifacep, ಧ್ರುವ AstVar* memberp, AstVarScope* vscp) {
        m_triggers.ತಳ್ಳು_back(TriggerEntry{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<ಧ್ರುವ AstVarScope*, uint32_t>;
    VscpTrigMap makeVscpToTrigMap(ಧ್ರುವ TriggerKit& trigKit, uint32_t firstIndex) ಧ್ರುವ;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = ಸ್ವಭಾವ;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = ಸ್ವಭಾವ;
    VirtIfaceTriggers& operator=(VirtIfaceTriggers&&) = ಸ್ವಭಾವ;
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
    using Bindings = std::vector<AstVarScope*>;

ಗುಪ್ತ:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<ಧ್ರುವ AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<ಧ್ರುವ AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_ಅಂಕೆ_Exact_Calls = 0;
    mutable uint32_t m_ಅಂಕೆ_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    ಸ್ಥಿರ ಧ್ರುವ Bindings s_none;

ಸಾರ್ವಜನಿಕ:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    ಶೂನ್ಯ addConstruction(ಧ್ರುವ AstClass* classp, ಧ್ರುವ AstVarScope* instp,
                         ಧ್ರುವ Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    ಶೂನ್ಯ dropInstance(ಧ್ರುವ AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    ಧ್ರುವ Bindings& forSample(ಧ್ರುವ AstVarScope* instp, ಧ್ರುವ AstClass* classp) ಧ್ರುವ;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t ಅಂಕೆ_Exact_Calls() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t ಅಂಕೆ_Union_Calls() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಅಂಕೆ_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = ಸ್ವಭಾವ;
    CovergroupRefBindings(CovergroupRefBindings&&) = ಸ್ವಭಾವ;
    CovergroupRefBindings& operator=(CovergroupRefBindings&&) = ಸ್ವಭಾವ;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
ಧ್ರುವ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* ಧ್ರುವ netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
ಶೂನ್ಯ transformForks(AstNetlist* ಧ್ರುವ netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
ಶೂನ್ಯ schedule(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope breakCycles(AstNetlist* netlistp,
                         ಧ್ರುವ LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, ಧ್ರುವ string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* newArgument(AstCFunc* funcp, Ast_ಗಂಟು_DType* dtypep, ಧ್ರುವ string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_ಗಂಟು_Stmt* setVar(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_ಗಂಟು_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_ಗಂಟು_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
ಶೂನ್ಯ splitCheck(AstCFunc* ofuncp);
}  // namespace util

ಶೂನ್ಯ beforeTrigVisitor(AstNetlist* netlistp, Sen_ಅಭಿವ್ಯಕ್ತಿ_Builder& sen_ಅಭಿವ್ಯಕ್ತಿ_Builder,
                       ಧ್ರುವ TriggerKit& trigKit);

}  // namespace V3Sched

#endif  // Guard
