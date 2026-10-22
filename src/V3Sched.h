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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"

#अन्तर्भाव <array>
#अन्तर्भाव <functional>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

class Sen_अभिव्यक्ती_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using VarScopeSet = std::unordered_set<ध्रुव AstVarScope*>;

अन्तःस्थ bool isVlForceVec(ध्रुव AstVarScope* vscp) {
    ध्रुव AstCDType* ध्रुव dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    प्रत्यागम dtypep && dtypep->name() == "VlForceVec";
}

अन्तःस्थ bool isForceReadMethod(VCMethod method) {
    प्रत्यागम method == VCMethod::FORCE_READ || method == VCMethod::FORCE_READ_INDEX;
}

अन्तःस्थ शून्य collectForceReadEdgeIgnores(Ast_गाठ* nodep, VarScopeSet& out) {
    VarScopeSet writtenForceVecs;
    VarScopeSet writtenVars;

    nodep->foreach([&](ध्रुव AstVarRef* refp) {
        AstVarScope* ध्रुव vscp = refp->varScopep();
        यदि (!refp->access().isWriteOrRW()) प्रत्यागम;
        यदि (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        यदि (!isVlForceVec(vscp)) प्रत्यागम;
        writtenForceVecs.emplace(vscp);
        यदि (!refp->varp()->ignoreSchedWrite()) out.emplace(vscp);
    });

    यदि (writtenForceVecs.empty() || writtenVars.empty()) प्रत्यागम;

    nodep->foreach([&](ध्रुव AstCMethodHard* callp) {
        यदि (!isForceReadMethod(callp->method())) प्रत्यागम;

        ध्रुव AstVarRef* ध्रुव fromRefp = VN_CAST(callp->fromp(), VarRef);
        यदि (!fromRefp || !writtenForceVecs.मोजणी(fromRefp->varScopep())) प्रत्यागम;

        Ast_गाठ_अभिव्यक्ती* ध्रुव origp = callp->pinsp();
        यदि (!origp) प्रत्यागम;
        origp->foreach([&](ध्रुव AstVarRef* refp) {
            AstVarScope* ध्रुव vscp = refp->varScopep();
            यदि (refp->access().isReadOrRW() && writtenVars.मोजणी(vscp)) out.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
सार्वजनिक:
    // TYPES
    // An AstActive and the AstScope it is under
    संरचना Entry final {
        AstScope* m_scopep;  // The scope the AstActive is under
        AstActive* m_activep;  // The AstActive holding the logic
        std::set<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::vector<Entry>::iterator;
    using const_iterator = std::vector<Entry>::const_iterator;

गुप्त:
    // STATE
    std::vector<Entry> m_entries;  // The AstActives held, with their scopes

सार्वजनिक:
    // METHODS

    // Iteration and access
    iterator begin() { प्रत्यागम m_entries.begin(); }
    iterator end() { प्रत्यागम m_entries.end(); }
    const_iterator begin() ध्रुव { प्रत्यागम m_entries.begin(); }
    const_iterator end() ध्रुव { प्रत्यागम m_entries.end(); }
    bool empty() ध्रुव { प्रत्यागम m_entries.empty(); }
    size_t size() ध्रुव { प्रत्यागम m_entries.size(); }
    ध्रुव Entry& operator[](size_t i) ध्रुव { प्रत्यागम m_entries[i]; }

    // Add an AstActive under the given scope
    शून्य add(AstScope* scopep, AstActive* activep) { m_entries.ढकला_back({scopep, activep, {}}); }

    // Add logic
    शून्य add(AstScope* scopep, Ast_Sen_वृक्ष* senTreep, Ast_गाठ* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        यदि (m_entries.empty() || m_entries.back().m_scopep != scopep
            || m_entries.back().m_activep->sentreep() != senTreep) {
            add(scopep, new AstActive{logicp->fileline(), "", senTreep});
        }
        m_entries.back().m_activep->addStmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() ध्रुव {
        LogicByScope result;
        प्रति (ध्रुव Entry& entry : m_entries) {
            result.add(entry.m_scopep, entry.m_activep->clone_वृक्ष(false));
        }
        प्रत्यागम result;
    }

    // Delete actives (they should all be empty)
    शून्य deleteActives() {
        प्रति (ध्रुव Entry& entry : m_entries) {
            AstActive* ध्रुव activep = entry.m_activep;
            यदि (v3Global.opt.debugCheck()) {
                प्रति (Ast_गाठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_गाठ_Procedure* ध्रुव procp = VN_CAST(nodep, गाठ_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            यदि (activep->backp()) activep->unlinkFrBack();
            VL_DO_DANGLING(activep->delete_वृक्ष(), activep);
        }
        m_entries.clear();
    };

    शून्य foreachLogic(ध्रुव std::function<शून्य(Ast_गाठ*)>& f) ध्रुव {
        प्रति (ध्रुव Entry& entry : m_entries) {
            ध्रुव AstActive* ध्रुव activep = entry.m_activep;
            प्रति (Ast_गाठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
संरचना LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = स्वभाव;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = स्वभाव;
    LogicClasses& operator=(LogicClasses&&) = स्वभाव;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
संरचना LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = स्वभाव;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = स्वभाव;
    LogicRegions& operator=(LogicRegions&&) = स्वभाव;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
संरचना LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = स्वभाव;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = स्वभाव;
    LogicReplicas& operator=(LogicReplicas&&) = स्वभाव;
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
सार्वजनिक:
    // Triggers are storead as an UnpackedArray with a fixed word size
    स्थिर constexpr uint32_t WORD_SIZE_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    स्थिर constexpr uint32_t WORD_SIZE = 1 << WORD_SIZE_LOG2;

गुप्त:
    ध्रुव std::string m_name;  // TriggerKit name
    ध्रुव bool m_slow;  // TriggerKit is for schedulign 'slow' code
    ध्रुव uint32_t m_n_Sense_शब्द;  // Number of words for Sense triggers
    ध्रुव uint32_t m_n_Extra_शब्द;  // Number of words for Extra triggers
    ध्रुव uint32_t m_n_Pre_शब्द;  // Number of words for 'pre' part
    ध्रुव uint32_t m_n_Vec_शब्द = m_n_Sense_शब्द + m_n_Extra_शब्द;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<ध्रुव AstSenItem>, size_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::vector<ध्रुव AstSenItem*> m_trigIdx2SenItem;
    // Data type of a single trigger word
    Ast_गाठ_DType* m_wordDTypep = nullptr;
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
    AstCFunc* createDumpExtFunc() ध्रुव;
    AstCFunc* createAnySetFunc(AstUnpackArrayDType* ध्रुव dtypep) ध्रुव;
    AstCFunc* createClearFunc() ध्रुव;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* ध्रुव oDtypep,
                               AstUnpackArrayDType* ध्रुव iDtypep) ध्रुव;

    TriggerKit(ध्रुव std::string& name, bool slow, uint32_t n_Sense_शब्द, uint32_t n_Extra_शब्द,
               uint32_t n_Pre_शब्द,
               std::unordered_map<VNRef<ध्रुव AstSenItem>, size_t> senItem2TrigIdx,
               ध्रुव std::vector<ध्रुव AstSenItem*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

सार्वजनिक:
    // Move constructible
    TriggerKit(TriggerKit&&) = स्वभाव;
    ~TriggerKit() = स्वभाव;
    TriggerKit& operator=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        friend class TriggerKit;
        std::vector<string> m_descriptions;  // Human readable description of extra triggers

    सार्वजनिक:
        ExtraTriggers() = स्वभाव;
        ~ExtraTriggers() = स्वभाव;

        uint32_t allocate(ध्रुव string& description) {
            m_descriptions.ढकला_back(description);
            प्रत्यागम m_descriptions.size() - 1;
        }
        uint32_t size() ध्रुव { प्रत्यागम m_descriptions.size(); }
    };
    // Generates list of assignments that fills
    स्थिर AstAssign* createSenTrigVecAssignment(AstVarScope* ध्रुव target,
                                                 std::vector<Ast_गाठ_अभिव्यक्ती*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    स्थिर TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* ध्रुव initFuncp,  //
                             Sen_अभिव्यक्ती_Builder& sen_अभिव्यक्ती_Builder,  //
                             ध्रुव std::vector<ध्रुव Ast_Sen_वृक्ष*>& preTreeps,  //
                             ध्रुव std::vector<ध्रुव Ast_Sen_वृक्ष*>& senTreeps,  //
                             ध्रुव string& name,  //
                             ध्रुव ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() ध्रुव { प्रत्यागम m_vscp; }
    AstVarScope* vscAccp() ध्रुव { प्रत्यागम m_vscAccp; }
    size_t senItem2TrigIdx(ध्रुव AstSenItem* senItemp) ध्रुव {
        प्रत्यागम m_senItem2TrigIdx.at(*senItemp);
    }
    ध्रुव std::vector<ध्रुव AstSenItem*>& trigIdx2SenItem() ध्रुव { प्रत्यागम m_trigIdx2SenItem; }
    AstCFunc* compBasep() ध्रुव { प्रत्यागम m_compVecp; }
    ध्रुव std::string& name() ध्रुव { प्रत्यागम m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_गाठ_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) ध्रुव;
    Ast_गाठ_अभिव्यक्ती* newAnySetCall(AstVarScope* vscp) ध्रुव;
    Ast_गाठ_Stmt* newClearCall(AstVarScope* vscp) ध्रुव;
    Ast_गाठ_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) ध्रुव;
    // Helpers for code generation
    Ast_गाठ_Stmt* newCompBaseCall() ध्रुव;
    Ast_गाठ_Stmt* newCompExtCall(AstVarScope* vscp) ध्रुव;
    Ast_गाठ_Stmt* newDumpCall(AstVarScope* vscp, ध्रुव std::string& tag, bool debugOnly) ध्रुव;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(ध्रुव std::string& name) ध्रुव;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extraTriggerIndex(uint32_t index) ध्रुव;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::set<uint32_t> triggerIndices(ध्रुव Ast_Sen_वृक्ष* senTreep, bool pre) ध्रुव;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_गाठ_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ध्रुव;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    शून्य addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ध्रुव;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    शून्य add_मूल्य_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t index) ध्रुव;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<ध्रुव AstVarScope*, std::set<Ast_Sen_वृक्ष*>> m_externalDomains;

सार्वजनिक:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_गाठ_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<ध्रुव AstVarScope*, std::vector<uint32_t>>
    externalTriggerIndices(ध्रुव TriggerKit& trigKit) ध्रुव VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* ध्रुव netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = स्वभाव;
    TimingKit(LogicByScope&& lbs, Ast_गाठ_Stmt* postUpdates,
              std::map<ध्रुव AstVarScope*, std::set<Ast_Sen_वृक्ष*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = स्वभाव;
    TimingKit& operator=(TimingKit&&) = स्वभाव;
};

class VirtIfaceTriggers final {
सार्वजनिक:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    संरचना TriggerEntry final {
        ध्रुव AstIface* m_ifacep;  // Interface type
        ध्रुव AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::vector<TriggerEntry> m_triggers;

    शून्य addTrigger(ध्रुव AstIface* ifacep, ध्रुव AstVar* memberp, AstVarScope* vscp) {
        m_triggers.ढकला_back(TriggerEntry{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<ध्रुव AstVarScope*, uint32_t>;
    VscpTrigMap makeVscpToTrigMap(ध्रुव TriggerKit& trigKit, uint32_t firstIndex) ध्रुव;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = स्वभाव;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = स्वभाव;
    VirtIfaceTriggers& operator=(VirtIfaceTriggers&&) = स्वभाव;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
सार्वजनिक:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::vector<AstVarScope*>;

गुप्त:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<ध्रुव AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<ध्रुव AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_अंक_Exact_Calls = 0;
    mutable uint32_t m_अंक_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    स्थिर ध्रुव Bindings s_none;

सार्वजनिक:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    शून्य addConstruction(ध्रुव AstClass* classp, ध्रुव AstVarScope* instp,
                         ध्रुव Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    शून्य dropInstance(ध्रुव AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    ध्रुव Bindings& forSample(ध्रुव AstVarScope* instp, ध्रुव AstClass* classp) ध्रुव;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t अंक_Exact_Calls() ध्रुव { प्रत्यागम m_अंक_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t अंक_Union_Calls() ध्रुव { प्रत्यागम m_अंक_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = स्वभाव;
    CovergroupRefBindings(CovergroupRefBindings&&) = स्वभाव;
    CovergroupRefBindings& operator=(CovergroupRefBindings&&) = स्वभाव;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
ध्रुव CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* ध्रुव netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
शून्य transformForks(AstNetlist* ध्रुव netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
शून्य schedule(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope breakCycles(AstNetlist* netlistp,
                         ध्रुव LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, ध्रुव string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* newArgument(AstCFunc* funcp, Ast_गाठ_DType* dtypep, ध्रुव string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_गाठ_Stmt* setVar(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_गाठ_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_गाठ_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
शून्य splitCheck(AstCFunc* ofuncp);
}  // namespace util

शून्य beforeTrigVisitor(AstNetlist* netlistp, Sen_अभिव्यक्ती_Builder& sen_अभिव्यक्ती_Builder,
                       ध्रुव TriggerKit& trigKit);

}  // namespace V3Sched

#endif  // Guard
