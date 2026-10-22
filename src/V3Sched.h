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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"

#অন্তর্ভাব <array>
#অন্তর্ভাব <functional>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>
#অন্তর্ভাব <utility>
#অন্তর্ভাব <vector>

class Sen_রাশিমালা_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using VarScopeSet = std::unordered_set<ধ্রুবক AstVarScope*>;

অন্তঃরেখ bool isVlForceVec(ধ্রুবক AstVarScope* vscp) {
    ধ্রুবক AstCDType* ধ্রুবক dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    ফেরত dtypep && dtypep->name() == "VlForceVec";
}

অন্তঃরেখ bool isForceReadMethod(VCMethod method) {
    ফেরত method == VCMethod::FORCE_READ || method == VCMethod::FORCE_READ_সূচক;
}

অন্তঃরেখ শূন্য collectForceReadEdgeIgnores(Ast_গ্রন্থি* nodep, VarScopeSet& out) {
    VarScopeSet writtenForceVecs;
    VarScopeSet writtenVars;

    nodep->foreach([&](ধ্রুবক AstVarRef* refp) {
        AstVarScope* ধ্রুবক vscp = refp->varScopep();
        যদি (!refp->access().isWriteOrRW()) ফেরত;
        যদি (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        যদি (!isVlForceVec(vscp)) ফেরত;
        writtenForceVecs.emplace(vscp);
        যদি (!refp->varp()->ignoreSchedWrite()) out.emplace(vscp);
    });

    যদি (writtenForceVecs.empty() || writtenVars.empty()) ফেরত;

    nodep->foreach([&](ধ্রুবক AstCMethodHard* callp) {
        যদি (!isForceReadMethod(callp->method())) ফেরত;

        ধ্রুবক AstVarRef* ধ্রুবক fromRefp = VN_CAST(callp->fromp(), VarRef);
        যদি (!fromRefp || !writtenForceVecs.গুনতি(fromRefp->varScopep())) ফেরত;

        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক origp = callp->pinsp();
        যদি (!origp) ফেরত;
        origp->foreach([&](ধ্রুবক AstVarRef* refp) {
            AstVarScope* ধ্রুবক vscp = refp->varScopep();
            যদি (refp->access().isReadOrRW() && writtenVars.গুনতি(vscp)) out.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
সর্বজনীন:
    // TYPES
    // An AstActive and the AstScope it is under
    কাঠামো Entry final {
        AstScope* m_scopep;  // The scope the AstActive is under
        AstActive* m_activep;  // The AstActive holding the logic
        std::set<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::vector<Entry>::iterator;
    using const_iterator = std::vector<Entry>::const_iterator;

ব্যক্তিগত:
    // STATE
    std::vector<Entry> m_entries;  // The AstActives held, with their scopes

সর্বজনীন:
    // METHODS

    // Iteration and access
    iterator begin() { ফেরত m_entries.begin(); }
    iterator end() { ফেরত m_entries.end(); }
    const_iterator begin() ধ্রুবক { ফেরত m_entries.begin(); }
    const_iterator end() ধ্রুবক { ফেরত m_entries.end(); }
    bool empty() ধ্রুবক { ফেরত m_entries.empty(); }
    আকার_t আকার_2() ধ্রুবক { ফেরত m_entries.আকার_2(); }
    ধ্রুবক Entry& operator[](আকার_t i) ধ্রুবক { ফেরত m_entries[i]; }

    // Add an AstActive under the given scope
    শূন্য add(AstScope* scopep, AstActive* activep) { m_entries.ঠেলো_back({scopep, activep, {}}); }

    // Add logic
    শূন্য add(AstScope* scopep, Ast_Sen_বৃক্ষ* senTreep, Ast_গ্রন্থি* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        যদি (m_entries.empty() || m_entries.back().m_scopep != scopep
            || m_entries.back().m_activep->sentreep() != senTreep) {
            add(scopep, new AstActive{logicp->fileline(), "", senTreep});
        }
        m_entries.back().m_activep->addStmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() ধ্রুবক {
        LogicByScope ফলাফল;
        জন্য (ধ্রুবক Entry& entry : m_entries) {
            ফলাফল.add(entry.m_scopep, entry.m_activep->clone_বৃক্ষ(false));
        }
        ফেরত ফলাফল;
    }

    // Delete actives (they should all be empty)
    শূন্য deleteActives() {
        জন্য (ধ্রুবক Entry& entry : m_entries) {
            AstActive* ধ্রুবক activep = entry.m_activep;
            যদি (v3Global.opt.debugCheck()) {
                জন্য (Ast_গ্রন্থি* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_গ্রন্থি_Procedure* ধ্রুবক procp = VN_CAST(nodep, গ্রন্থি_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            যদি (activep->backp()) activep->unlinkFrBack();
            VL_DO_DANGLING(activep->delete_বৃক্ষ(), activep);
        }
        m_entries.clear();
    };

    শূন্য foreachLogic(ধ্রুবক std::function<শূন্য(Ast_গ্রন্থি*)>& f) ধ্রুবক {
        জন্য (ধ্রুবক Entry& entry : m_entries) {
            ধ্রুবক AstActive* ধ্রুবক activep = entry.m_activep;
            জন্য (Ast_গ্রন্থি* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
কাঠামো LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = পূর্বনির্ধারিত;
    LogicClasses& operator=(LogicClasses&&) = পূর্বনির্ধারিত;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
কাঠামো LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = পূর্বনির্ধারিত;
    LogicRegions& operator=(LogicRegions&&) = পূর্বনির্ধারিত;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
কাঠামো LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = পূর্বনির্ধারিত;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = পূর্বনির্ধারিত;
    LogicReplicas& operator=(LogicReplicas&&) = পূর্বনির্ধারিত;
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
সর্বজনীন:
    // Triggers are storead as an UnpackedArray with a fixed word size
    স্থির constexpr uint32_t WORD_আকার_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    স্থির constexpr uint32_t WORD_আকার = 1 << WORD_আকার_LOG2;

ব্যক্তিগত:
    ধ্রুবক std::string m_name;  // TriggerKit name
    ধ্রুবক bool m_slow;  // TriggerKit is for schedulign 'slow' code
    ধ্রুবক uint32_t m_n_Sense_শব্দগুলি;  // Number of words for Sense triggers
    ধ্রুবক uint32_t m_n_Extra_শব্দগুলি;  // Number of words for Extra triggers
    ধ্রুবক uint32_t m_n_Pre_শব্দগুলি;  // Number of words for 'pre' part
    ধ্রুবক uint32_t m_n_Vec_শব্দগুলি = m_n_Sense_শব্দগুলি + m_n_Extra_শব্দগুলি;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<ধ্রুবক AstSenItem>, আকার_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::vector<ধ্রুবক AstSenItem*> m_trigIdx2SenItem;
    // Data type of a single trigger word
    Ast_গ্রন্থি_DType* m_wordDTypep = nullptr;
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
    AstCFunc* createDumpExtFunc() ধ্রুবক;
    AstCFunc* createAnySetFunc(AstUnpackArrayDType* ধ্রুবক dtypep) ধ্রুবক;
    AstCFunc* createClearFunc() ধ্রুবক;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* ধ্রুবক oDtypep,
                               AstUnpackArrayDType* ধ্রুবক iDtypep) ধ্রুবক;

    TriggerKit(ধ্রুবক std::string& name, bool slow, uint32_t n_Sense_শব্দগুলি, uint32_t n_Extra_শব্দগুলি,
               uint32_t n_Pre_শব্দগুলি,
               std::unordered_map<VNRef<ধ্রুবক AstSenItem>, আকার_t> senItem2TrigIdx,
               ধ্রুবক std::vector<ধ্রুবক AstSenItem*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

সর্বজনীন:
    // Move constructible
    TriggerKit(TriggerKit&&) = পূর্বনির্ধারিত;
    ~TriggerKit() = পূর্বনির্ধারিত;
    TriggerKit& operator=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        friend class TriggerKit;
        std::vector<string> m_descriptions;  // Human readable description of extra triggers

    সর্বজনীন:
        ExtraTriggers() = পূর্বনির্ধারিত;
        ~ExtraTriggers() = পূর্বনির্ধারিত;

        uint32_t allocate(ধ্রুবক string& description) {
            m_descriptions.ঠেলো_back(description);
            ফেরত m_descriptions.আকার_2() - 1;
        }
        uint32_t আকার_2() ধ্রুবক { ফেরত m_descriptions.আকার_2(); }
    };
    // Generates list of assignments that fills
    স্থির AstAssign* createSenTrigVecAssignment(AstVarScope* ধ্রুবক target,
                                                 std::vector<Ast_গ্রন্থি_রাশিমালা*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    স্থির TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* ধ্রুবক initFuncp,  //
                             Sen_রাশিমালা_Builder& sen_রাশিমালা_Builder,  //
                             ধ্রুবক std::vector<ধ্রুবক Ast_Sen_বৃক্ষ*>& preTreeps,  //
                             ধ্রুবক std::vector<ধ্রুবক Ast_Sen_বৃক্ষ*>& senTreeps,  //
                             ধ্রুবক string& name,  //
                             ধ্রুবক ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() ধ্রুবক { ফেরত m_vscp; }
    AstVarScope* vscAccp() ধ্রুবক { ফেরত m_vscAccp; }
    আকার_t senItem2TrigIdx(ধ্রুবক AstSenItem* senItemp) ধ্রুবক {
        ফেরত m_senItem2TrigIdx.at(*senItemp);
    }
    ধ্রুবক std::vector<ধ্রুবক AstSenItem*>& trigIdx2SenItem() ধ্রুবক { ফেরত m_trigIdx2SenItem; }
    AstCFunc* compBasep() ধ্রুবক { ফেরত m_compVecp; }
    ধ্রুবক std::string& name() ধ্রুবক { ফেরত m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_গ্রন্থি_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) ধ্রুবক;
    Ast_গ্রন্থি_রাশিমালা* newAnySetCall(AstVarScope* vscp) ধ্রুবক;
    Ast_গ্রন্থি_Stmt* newClearCall(AstVarScope* vscp) ধ্রুবক;
    Ast_গ্রন্থি_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) ধ্রুবক;
    // Helpers for code generation
    Ast_গ্রন্থি_Stmt* newCompBaseCall() ধ্রুবক;
    Ast_গ্রন্থি_Stmt* newCompExtCall(AstVarScope* vscp) ধ্রুবক;
    Ast_গ্রন্থি_Stmt* newDumpCall(AstVarScope* vscp, ধ্রুবক std::string& tag, bool debugOnly) ধ্রুবক;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(ধ্রুবক std::string& name) ধ্রুবক;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extra_Trigger_সূচক(uint32_t সূচক) ধ্রুবক;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::set<uint32_t> triggerIndices(ধ্রুবক Ast_Sen_বৃক্ষ* senTreep, bool pre) ধ্রুবক;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_গ্রন্থি_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t সূচক) ধ্রুবক;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    শূন্য addExtraTriggerAssignment(AstVarScope* vscp, uint32_t সূচক) ধ্রুবক;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    শূন্য add_মান_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t সূচক) ধ্রুবক;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<ধ্রুবক AstVarScope*, std::set<Ast_Sen_বৃক্ষ*>> m_externalDomains;

সর্বজনীন:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_গ্রন্থি_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<ধ্রুবক AstVarScope*, std::vector<uint32_t>>
    externalTriggerIndices(ধ্রুবক TriggerKit& trigKit) ধ্রুবক VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* ধ্রুবক netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = পূর্বনির্ধারিত;
    TimingKit(LogicByScope&& lbs, Ast_গ্রন্থি_Stmt* postUpdates,
              std::map<ধ্রুবক AstVarScope*, std::set<Ast_Sen_বৃক্ষ*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = পূর্বনির্ধারিত;
    TimingKit& operator=(TimingKit&&) = পূর্বনির্ধারিত;
};

class VirtIfaceTriggers final {
সর্বজনীন:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    কাঠামো TriggerEntry final {
        ধ্রুবক AstIface* m_ifacep;  // Interface type
        ধ্রুবক AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::vector<TriggerEntry> m_triggers;

    শূন্য addTrigger(ধ্রুবক AstIface* ifacep, ধ্রুবক AstVar* memberp, AstVarScope* vscp) {
        m_triggers.ঠেলো_back(TriggerEntry{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<ধ্রুবক AstVarScope*, uint32_t>;
    VscpTrigMap makeVscpToTrigMap(ধ্রুবক TriggerKit& trigKit, uint32_t first_সূচক) ধ্রুবক;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = পূর্বনির্ধারিত;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = পূর্বনির্ধারিত;
    VirtIfaceTriggers& operator=(VirtIfaceTriggers&&) = পূর্বনির্ধারিত;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
সর্বজনীন:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::vector<AstVarScope*>;

ব্যক্তিগত:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<ধ্রুবক AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<ধ্রুবক AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_অঙ্ক_Exact_Calls = 0;
    mutable uint32_t m_অঙ্ক_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    স্থির ধ্রুবক Bindings s_none;

সর্বজনীন:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    শূন্য addConstruction(ধ্রুবক AstClass* classp, ধ্রুবক AstVarScope* instp,
                         ধ্রুবক Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    শূন্য dropInstance(ধ্রুবক AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    ধ্রুবক Bindings& forSample(ধ্রুবক AstVarScope* instp, ধ্রুবক AstClass* classp) ধ্রুবক;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t অঙ্ক_Exact_Calls() ধ্রুবক { ফেরত m_অঙ্ক_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t অঙ্ক_Union_Calls() ধ্রুবক { ফেরত m_অঙ্ক_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = পূর্বনির্ধারিত;
    CovergroupRefBindings(CovergroupRefBindings&&) = পূর্বনির্ধারিত;
    CovergroupRefBindings& operator=(CovergroupRefBindings&&) = পূর্বনির্ধারিত;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
ধ্রুবক CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* ধ্রুবক netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
শূন্য transformForks(AstNetlist* ধ্রুবক netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
শূন্য schedule(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope breakCycles(AstNetlist* netlistp,
                         ধ্রুবক LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, ধ্রুবক string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* newArgument(AstCFunc* funcp, Ast_গ্রন্থি_DType* dtypep, ধ্রুবক string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_গ্রন্থি_Stmt* setVar(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_গ্রন্থি_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_গ্রন্থি_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
শূন্য splitCheck(AstCFunc* ofuncp);
}  // namespace util

শূন্য beforeTrigVisitor(AstNetlist* netlistp, Sen_রাশিমালা_Builder& sen_রাশিমালা_Builder,
                       ধ্রুবক TriggerKit& trigKit);

}  // namespace V3Sched

#endif  // Guard
