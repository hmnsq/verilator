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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"

#അന്തര്ഭാവ <array>
#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>
#അന്തര്ഭാവ <utility>
#അന്തര്ഭാവ <vector>

class Sen_വ്യഞ്ജകം_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using VarScopeSet = std::unordered_set<സ്ഥിരം AstVarScope*>;

വരിയിൽ bool isVlForceVec(സ്ഥിരം AstVarScope* vscp) {
    സ്ഥിരം AstCDType* സ്ഥിരം dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    തിരികെ dtypep && dtypep->name() == "VlForceVec";
}

വരിയിൽ bool isForceReadMethod(VCMethod method) {
    തിരികെ method == VCMethod::FORCE_READ || method == VCMethod::FORCE_READ_INDEX;
}

വരിയിൽ ശൂന്യം collectForceReadEdgeIgnores(Ast_കണ്ണി* nodep, VarScopeSet& out) {
    VarScopeSet writtenForceVecs;
    VarScopeSet writtenVars;

    nodep->foreach([&](സ്ഥിരം AstVarRef* refp) {
        AstVarScope* സ്ഥിരം vscp = refp->varScopep();
        എങ്കിൽ (!refp->access().isWriteOrRW()) തിരികെ;
        എങ്കിൽ (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        എങ്കിൽ (!isVlForceVec(vscp)) തിരികെ;
        writtenForceVecs.emplace(vscp);
        എങ്കിൽ (!refp->varp()->ignoreSchedWrite()) out.emplace(vscp);
    });

    എങ്കിൽ (writtenForceVecs.empty() || writtenVars.empty()) തിരികെ;

    nodep->foreach([&](സ്ഥിരം AstCMethodHard* callp) {
        എങ്കിൽ (!isForceReadMethod(callp->method())) തിരികെ;

        സ്ഥിരം AstVarRef* സ്ഥിരം fromRefp = VN_CAST(callp->fromp(), VarRef);
        എങ്കിൽ (!fromRefp || !writtenForceVecs.എണ്ണം(fromRefp->varScopep())) തിരികെ;

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം origp = callp->pinsp();
        എങ്കിൽ (!origp) തിരികെ;
        origp->foreach([&](സ്ഥിരം AstVarRef* refp) {
            AstVarScope* സ്ഥിരം vscp = refp->varScopep();
            എങ്കിൽ (refp->access().isReadOrRW() && writtenVars.എണ്ണം(vscp)) out.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
പൊതു:
    // TYPES
    // An AstActive and the AstScope it is under
    ഘടന Entry final {
        AstScope* m_scopep;  // The scope the AstActive is under
        AstActive* m_activep;  // The AstActive holding the logic
        std::set<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::vector<Entry>::iterator;
    using const_iterator = std::vector<Entry>::const_iterator;

സ്വകാര്യ:
    // STATE
    std::vector<Entry> m_entries;  // The AstActives held, with their scopes

പൊതു:
    // METHODS

    // Iteration and access
    iterator begin() { തിരികെ m_entries.begin(); }
    iterator end() { തിരികെ m_entries.end(); }
    const_iterator begin() സ്ഥിരം { തിരികെ m_entries.begin(); }
    const_iterator end() സ്ഥിരം { തിരികെ m_entries.end(); }
    bool empty() സ്ഥിരം { തിരികെ m_entries.empty(); }
    size_t size() സ്ഥിരം { തിരികെ m_entries.size(); }
    സ്ഥിരം Entry& operator[](size_t i) സ്ഥിരം { തിരികെ m_entries[i]; }

    // Add an AstActive under the given scope
    ശൂന്യം add(AstScope* scopep, AstActive* activep) { m_entries.തള്ളുക_back({scopep, activep, {}}); }

    // Add logic
    ശൂന്യം add(AstScope* scopep, Ast_Sen_വൃക്ഷം* senTreep, Ast_കണ്ണി* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        എങ്കിൽ (m_entries.empty() || m_entries.back().m_scopep != scopep
            || m_entries.back().m_activep->sentreep() != senTreep) {
            add(scopep, new AstActive{logicp->fileline(), "", senTreep});
        }
        m_entries.back().m_activep->addStmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() സ്ഥിരം {
        LogicByScope result;
        വേണ്ടി (സ്ഥിരം Entry& entry : m_entries) {
            result.add(entry.m_scopep, entry.m_activep->clone_വൃക്ഷം(false));
        }
        തിരികെ result;
    }

    // Delete actives (they should all be empty)
    ശൂന്യം deleteActives() {
        വേണ്ടി (സ്ഥിരം Entry& entry : m_entries) {
            AstActive* സ്ഥിരം activep = entry.m_activep;
            എങ്കിൽ (v3Global.opt.debugCheck()) {
                വേണ്ടി (Ast_കണ്ണി* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_കണ്ണി_Procedure* സ്ഥിരം procp = VN_CAST(nodep, കണ്ണി_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            എങ്കിൽ (activep->backp()) activep->unlinkFrBack();
            VL_DO_DANGLING(activep->delete_വൃക്ഷം(), activep);
        }
        m_entries.clear();
    };

    ശൂന്യം foreachLogic(സ്ഥിരം std::function<ശൂന്യം(Ast_കണ്ണി*)>& f) സ്ഥിരം {
        വേണ്ടി (സ്ഥിരം Entry& entry : m_entries) {
            സ്ഥിരം AstActive* സ്ഥിരം activep = entry.m_activep;
            വേണ്ടി (Ast_കണ്ണി* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
ഘടന LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = സ്ഥിരസ്ഥിതി;
    LogicClasses& operator=(LogicClasses&&) = സ്ഥിരസ്ഥിതി;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
ഘടന LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = സ്ഥിരസ്ഥിതി;
    LogicRegions& operator=(LogicRegions&&) = സ്ഥിരസ്ഥിതി;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
ഘടന LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = സ്ഥിരസ്ഥിതി;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = സ്ഥിരസ്ഥിതി;
    LogicReplicas& operator=(LogicReplicas&&) = സ്ഥിരസ്ഥിതി;
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
പൊതു:
    // Triggers are storead as an UnpackedArray with a fixed word size
    സ്ഥിതം constexpr uint32_t WORD_SIZE_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    സ്ഥിതം constexpr uint32_t WORD_SIZE = 1 << WORD_SIZE_LOG2;

സ്വകാര്യ:
    സ്ഥിരം std::string m_name;  // TriggerKit name
    സ്ഥിരം bool m_slow;  // TriggerKit is for schedulign 'slow' code
    സ്ഥിരം uint32_t m_n_Sense_വാക്കുകൾ;  // Number of words for Sense triggers
    സ്ഥിരം uint32_t m_n_Extra_വാക്കുകൾ;  // Number of words for Extra triggers
    സ്ഥിരം uint32_t m_n_Pre_വാക്കുകൾ;  // Number of words for 'pre' part
    സ്ഥിരം uint32_t m_n_Vec_വാക്കുകൾ = m_n_Sense_വാക്കുകൾ + m_n_Extra_വാക്കുകൾ;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<സ്ഥിരം AstSenItem>, size_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::vector<സ്ഥിരം AstSenItem*> m_trigIdx2SenItem;
    // Data type of a single trigger word
    Ast_കണ്ണി_DType* m_wordDTypep = nullptr;
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
    AstCFunc* createDumpExtFunc() സ്ഥിരം;
    AstCFunc* createAnySetFunc(AstUnpackArrayDType* സ്ഥിരം dtypep) സ്ഥിരം;
    AstCFunc* createClearFunc() സ്ഥിരം;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* സ്ഥിരം oDtypep,
                               AstUnpackArrayDType* സ്ഥിരം iDtypep) സ്ഥിരം;

    TriggerKit(സ്ഥിരം std::string& name, bool slow, uint32_t n_Sense_വാക്കുകൾ, uint32_t n_Extra_വാക്കുകൾ,
               uint32_t n_Pre_വാക്കുകൾ,
               std::unordered_map<VNRef<സ്ഥിരം AstSenItem>, size_t> senItem2TrigIdx,
               സ്ഥിരം std::vector<സ്ഥിരം AstSenItem*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

പൊതു:
    // Move constructible
    TriggerKit(TriggerKit&&) = സ്ഥിരസ്ഥിതി;
    ~TriggerKit() = സ്ഥിരസ്ഥിതി;
    TriggerKit& operator=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        friend class TriggerKit;
        std::vector<string> m_descriptions;  // Human readable description of extra triggers

    പൊതു:
        ExtraTriggers() = സ്ഥിരസ്ഥിതി;
        ~ExtraTriggers() = സ്ഥിരസ്ഥിതി;

        uint32_t allocate(സ്ഥിരം string& description) {
            m_descriptions.തള്ളുക_back(description);
            തിരികെ m_descriptions.size() - 1;
        }
        uint32_t size() സ്ഥിരം { തിരികെ m_descriptions.size(); }
    };
    // Generates list of assignments that fills
    സ്ഥിതം AstAssign* createSenTrigVecAssignment(AstVarScope* സ്ഥിരം target,
                                                 std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    സ്ഥിതം TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* സ്ഥിരം initFuncp,  //
                             Sen_വ്യഞ്ജകം_Builder& sen_വ്യഞ്ജകം_Builder,  //
                             സ്ഥിരം std::vector<സ്ഥിരം Ast_Sen_വൃക്ഷം*>& preTreeps,  //
                             സ്ഥിരം std::vector<സ്ഥിരം Ast_Sen_വൃക്ഷം*>& senTreeps,  //
                             സ്ഥിരം string& name,  //
                             സ്ഥിരം ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() സ്ഥിരം { തിരികെ m_vscp; }
    AstVarScope* vscAccp() സ്ഥിരം { തിരികെ m_vscAccp; }
    size_t senItem2TrigIdx(സ്ഥിരം AstSenItem* senItemp) സ്ഥിരം {
        തിരികെ m_senItem2TrigIdx.at(*senItemp);
    }
    സ്ഥിരം std::vector<സ്ഥിരം AstSenItem*>& trigIdx2SenItem() സ്ഥിരം { തിരികെ m_trigIdx2SenItem; }
    AstCFunc* compBasep() സ്ഥിരം { തിരികെ m_compVecp; }
    സ്ഥിരം std::string& name() സ്ഥിരം { തിരികെ m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_കണ്ണി_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) സ്ഥിരം;
    Ast_കണ്ണി_വ്യഞ്ജകം* newAnySetCall(AstVarScope* vscp) സ്ഥിരം;
    Ast_കണ്ണി_Stmt* newClearCall(AstVarScope* vscp) സ്ഥിരം;
    Ast_കണ്ണി_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) സ്ഥിരം;
    // Helpers for code generation
    Ast_കണ്ണി_Stmt* newCompBaseCall() സ്ഥിരം;
    Ast_കണ്ണി_Stmt* newCompExtCall(AstVarScope* vscp) സ്ഥിരം;
    Ast_കണ്ണി_Stmt* newDumpCall(AstVarScope* vscp, സ്ഥിരം std::string& tag, bool debugOnly) സ്ഥിരം;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(സ്ഥിരം std::string& name) സ്ഥിരം;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extraTriggerIndex(uint32_t index) സ്ഥിരം;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::set<uint32_t> triggerIndices(സ്ഥിരം Ast_Sen_വൃക്ഷം* senTreep, bool pre) സ്ഥിരം;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_കണ്ണി_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) സ്ഥിരം;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    ശൂന്യം addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) സ്ഥിരം;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    ശൂന്യം add_മൂല്യം_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t index) സ്ഥിരം;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<സ്ഥിരം AstVarScope*, std::set<Ast_Sen_വൃക്ഷം*>> m_externalDomains;

പൊതു:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_കണ്ണി_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<സ്ഥിരം AstVarScope*, std::vector<uint32_t>>
    externalTriggerIndices(സ്ഥിരം TriggerKit& trigKit) സ്ഥിരം VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* സ്ഥിരം netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = സ്ഥിരസ്ഥിതി;
    TimingKit(LogicByScope&& lbs, Ast_കണ്ണി_Stmt* postUpdates,
              std::map<സ്ഥിരം AstVarScope*, std::set<Ast_Sen_വൃക്ഷം*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = സ്ഥിരസ്ഥിതി;
    TimingKit& operator=(TimingKit&&) = സ്ഥിരസ്ഥിതി;
};

class VirtIfaceTriggers final {
പൊതു:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    ഘടന TriggerEntry final {
        സ്ഥിരം AstIface* m_ifacep;  // Interface type
        സ്ഥിരം AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::vector<TriggerEntry> m_triggers;

    ശൂന്യം addTrigger(സ്ഥിരം AstIface* ifacep, സ്ഥിരം AstVar* memberp, AstVarScope* vscp) {
        m_triggers.തള്ളുക_back(TriggerEntry{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<സ്ഥിരം AstVarScope*, uint32_t>;
    VscpTrigMap makeVscpToTrigMap(സ്ഥിരം TriggerKit& trigKit, uint32_t firstIndex) സ്ഥിരം;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = സ്ഥിരസ്ഥിതി;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = സ്ഥിരസ്ഥിതി;
    VirtIfaceTriggers& operator=(VirtIfaceTriggers&&) = സ്ഥിരസ്ഥിതി;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
പൊതു:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::vector<AstVarScope*>;

സ്വകാര്യ:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<സ്ഥിരം AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<സ്ഥിരം AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_അക്കം_Exact_Calls = 0;
    mutable uint32_t m_അക്കം_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    സ്ഥിതം സ്ഥിരം Bindings s_none;

പൊതു:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    ശൂന്യം addConstruction(സ്ഥിരം AstClass* classp, സ്ഥിരം AstVarScope* instp,
                         സ്ഥിരം Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    ശൂന്യം dropInstance(സ്ഥിരം AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    സ്ഥിരം Bindings& forSample(സ്ഥിരം AstVarScope* instp, സ്ഥിരം AstClass* classp) സ്ഥിരം;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t അക്കം_Exact_Calls() സ്ഥിരം { തിരികെ m_അക്കം_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t അക്കം_Union_Calls() സ്ഥിരം { തിരികെ m_അക്കം_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = സ്ഥിരസ്ഥിതി;
    CovergroupRefBindings(CovergroupRefBindings&&) = സ്ഥിരസ്ഥിതി;
    CovergroupRefBindings& operator=(CovergroupRefBindings&&) = സ്ഥിരസ്ഥിതി;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
സ്ഥിരം CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* സ്ഥിരം netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
ശൂന്യം transformForks(AstNetlist* സ്ഥിരം netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
ശൂന്യം schedule(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope breakCycles(AstNetlist* netlistp,
                         സ്ഥിരം LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, സ്ഥിരം string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* newArgument(AstCFunc* funcp, Ast_കണ്ണി_DType* dtypep, സ്ഥിരം string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_കണ്ണി_Stmt* setVar(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_കണ്ണി_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_കണ്ണി_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
ശൂന്യം splitCheck(AstCFunc* ofuncp);
}  // namespace util

ശൂന്യം beforeTrigVisitor(AstNetlist* netlistp, Sen_വ്യഞ്ജകം_Builder& sen_വ്യഞ്ജകം_Builder,
                       സ്ഥിരം TriggerKit& trigKit);

}  // namespace V3Sched

#endif  // Guard
