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

#اگر_تعریف_نشده VERILATOR_V3SCHED_H_
#تعریف_کن VERILATOR_V3SCHED_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"

#ضمن <array>
#ضمن <functional>
#ضمن <unordered_map>
#ضمن <unordered_set>
#ضمن <utility>
#ضمن <vector>

class Sen_عبارت_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using Var_Scope_مجموعه = std::unordered_مجموعه<ثابت AstVarScope*>;

مضمن bool isVlForceVec(ثابت AstVarScope* vscp) {
    ثابت AstCDType* ثابت dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    أرجع dtypep && dtypep->name() == "VlForceVec";
}

مضمن bool is_Force_Read_روش(VCMethod روش) {
    أرجع روش == VCMethod::FORCE_READ || روش == VCMethod::FORCE_READ_شاخص;
}

مضمن فراغ collect_Force_Read_یال_Ignores(Ast_گره* nodep, Var_Scope_مجموعه& بیرون) {
    Var_Scope_مجموعه writtenForceVecs;
    Var_Scope_مجموعه writtenVars;

    nodep->foreach([&](ثابت AstVarRef* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        إذا (!refp->access().isWriteOrRW()) أرجع;
        إذا (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        إذا (!isVlForceVec(vscp)) أرجع;
        writtenForceVecs.emplace(vscp);
        إذا (!refp->varp()->ignoreSchedWrite()) بیرون.emplace(vscp);
    });

    إذا (writtenForceVecs.خالی() || writtenVars.خالی()) أرجع;

    nodep->foreach([&](ثابت AstCMethodHard* callp) {
        إذا (!is_Force_Read_روش(callp->روش())) أرجع;

        ثابت AstVarRef* ثابت fromRefp = VN_CAST(callp->fromp(), VarRef);
        إذا (!fromRefp || !writtenForceVecs.شمار(fromRefp->varScopep())) أرجع;

        Ast_گره_عبارت* ثابت origp = callp->pinsp();
        إذا (!origp) أرجع;
        origp->foreach([&](ثابت AstVarRef* refp) {
            AstVarScope* ثابت vscp = refp->varScopep();
            إذا (refp->access().isReadOrRW() && writtenVars.شمار(vscp)) بیرون.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
عام:
    // TYPES
    // An AstActive and the AstScope it is under
    بنية مدخل_2 final {
        AstScope* m_scopep;  // The scope the AstActive is under
        Ast_فعال* m_activep;  // The AstActive holding the logic
        std::مجموعه<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::وکتور<مدخل_2>::iterator;
    using const_iterator = std::وکتور<مدخل_2>::const_iterator;

خاص:
    // STATE
    std::وکتور<مدخل_2> m_entries;  // The AstActives held, with their scopes

عام:
    // METHODS

    // Iteration and access
    iterator begin() { أرجع m_entries.begin(); }
    iterator انتها() { أرجع m_entries.انتها(); }
    const_iterator begin() ثابت { أرجع m_entries.begin(); }
    const_iterator انتها() ثابت { أرجع m_entries.انتها(); }
    bool خالی() ثابت { أرجع m_entries.خالی(); }
    اندازه_t اندازه() ثابت { أرجع m_entries.اندازه(); }
    ثابت مدخل_2& عملگر[](اندازه_t i) ثابت { أرجع m_entries[i]; }

    // Add an AstActive under the given scope
    فراغ جمع_کن(AstScope* scopep, Ast_فعال* activep) { m_entries.افزودن_پشت({scopep, activep, {}}); }

    // Add logic
    فراغ جمع_کن(AstScope* scopep, Ast_Sen_درخت* senTreep, Ast_گره* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        إذا (m_entries.خالی() || m_entries.پشت().m_scopep != scopep
            || m_entries.پشت().m_activep->sentreep() != senTreep) {
            جمع_کن(scopep, new Ast_فعال{logicp->fileline(), "", senTreep});
        }
        m_entries.پشت().m_activep->جمع_کن_Stmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() ثابت {
        LogicByScope نتیجه;
        لكل (ثابت مدخل_2& مدخل : m_entries) {
            نتیجه.جمع_کن(مدخل.m_scopep, مدخل.m_activep->clone_درخت(false));
        }
        أرجع نتیجه;
    }

    // Delete actives (they should all be empty)
    فراغ deleteActives() {
        لكل (ثابت مدخل_2& مدخل : m_entries) {
            Ast_فعال* ثابت activep = مدخل.m_activep;
            إذا (v3Global.opt.debugCheck()) {
                لكل (Ast_گره* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_گره_Procedure* ثابت procp = VN_CAST(nodep, گره_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            إذا (activep->backp()) activep->unlink_Fr_پشت();
            VL_DO_DANGLING(activep->delete_درخت(), activep);
        }
        m_entries.clear();
    };

    فراغ foreachLogic(ثابت std::function<فراغ(Ast_گره*)>& f) ثابت {
        لكل (ثابت مدخل_2& مدخل : m_entries) {
            ثابت Ast_فعال* ثابت activep = مدخل.m_activep;
            لكل (Ast_گره* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
بنية LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = افتراضي;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = افتراضي;
    LogicClasses& عملگر=(LogicClasses&&) = افتراضي;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
بنية LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = افتراضي;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = افتراضي;
    LogicRegions& عملگر=(LogicRegions&&) = افتراضي;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
بنية LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = افتراضي;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = افتراضي;
    LogicReplicas& عملگر=(LogicReplicas&&) = افتراضي;
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
عام:
    // Triggers are storead as an UnpackedArray with a fixed word size
    ساكن constexpr uint32_t کلمه_اندازه_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    ساكن constexpr uint32_t کلمه_اندازه = 1 << کلمه_اندازه_LOG2;

خاص:
    ثابت std::string m_name;  // TriggerKit name
    ثابت bool m_slow;  // TriggerKit is for schedulign 'slow' code
    ثابت uint32_t m_n_Sense_واژگان;  // Number of words for Sense triggers
    ثابت uint32_t m_n_Extra_واژگان;  // Number of words for Extra triggers
    ثابت uint32_t m_n_Pre_واژگان;  // Number of words for 'pre' part
    ثابت uint32_t m_n_Vec_واژگان = m_n_Sense_واژگان + m_n_Extra_واژگان;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<ثابت Ast_Sen_قلم>, اندازه_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::وکتور<ثابت Ast_Sen_قلم*> m_trig_Idx2_Sen_قلم;
    // Data type of a single trigger word
    Ast_گره_DType* m_کلمه_DTypep = nullptr;
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
    mutable AstCFunc* m_any_مجموعه_Vecp = nullptr;
    mutable AstCFunc* m_any_مجموعه_Extp = nullptr;
    // The AstCFunc setting bits in a trigger vector that are set in another - create lazily
    mutable std::array<AstCFunc*, 4> m_orIntoVecps = {nullptr};
    // The AstCFunc setting a trigger vector to all zeroes - create lazily
    mutable AstCFunc* m_clearp = nullptr;

    // Methods to lazy construct functions processing trigger vectors
    AstCFunc* createDumpExtFunc() ثابت;
    AstCFunc* create_Any_مجموعه_Func(AstUnpackArrayDType* ثابت dtypep) ثابت;
    AstCFunc* createClearFunc() ثابت;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* ثابت oDtypep,
                               AstUnpackArrayDType* ثابت iDtypep) ثابت;

    TriggerKit(ثابت std::string& name, bool slow, uint32_t n_Sense_واژگان, uint32_t n_Extra_واژگان,
               uint32_t n_Pre_واژگان,
               std::unordered_map<VNRef<ثابت Ast_Sen_قلم>, اندازه_t> senItem2TrigIdx,
               ثابت std::وکتور<ثابت Ast_Sen_قلم*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

عام:
    // Move constructible
    TriggerKit(TriggerKit&&) = افتراضي;
    ~TriggerKit() = افتراضي;
    TriggerKit& عملگر=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        دوست class TriggerKit;
        std::وکتور<string> m_descriptions;  // Human readable description of extra triggers

    عام:
        ExtraTriggers() = افتراضي;
        ~ExtraTriggers() = افتراضي;

        uint32_t allocate(ثابت string& description) {
            m_descriptions.افزودن_پشت(description);
            أرجع m_descriptions.اندازه() - 1;
        }
        uint32_t اندازه() ثابت { أرجع m_descriptions.اندازه(); }
    };
    // Generates list of assignments that fills
    ساكن Ast_تخصیص_بده* createSenTrigVecAssignment(AstVarScope* ثابت هدف,
                                                 std::وکتور<Ast_گره_عبارت*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    ساكن TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* ثابت initFuncp,  //
                             Sen_عبارت_Builder& sen_عبارت_Builder,  //
                             ثابت std::وکتور<ثابت Ast_Sen_درخت*>& preTreeps,  //
                             ثابت std::وکتور<ثابت Ast_Sen_درخت*>& senTreeps,  //
                             ثابت string& name,  //
                             ثابت ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() ثابت { أرجع m_vscp; }
    AstVarScope* vscAccp() ثابت { أرجع m_vscAccp; }
    اندازه_t senItem2TrigIdx(ثابت Ast_Sen_قلم* senItemp) ثابت {
        أرجع m_senItem2TrigIdx.at(*senItemp);
    }
    ثابت std::وکتور<ثابت Ast_Sen_قلم*>& trig_Idx2_Sen_قلم() ثابت { أرجع m_trig_Idx2_Sen_قلم; }
    AstCFunc* compBasep() ثابت { أرجع m_compVecp; }
    ثابت std::string& name() ثابت { أرجع m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_گره_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) ثابت;
    Ast_گره_عبارت* new_Any_مجموعه_Call(AstVarScope* vscp) ثابت;
    Ast_گره_Stmt* newClearCall(AstVarScope* vscp) ثابت;
    Ast_گره_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) ثابت;
    // Helpers for code generation
    Ast_گره_Stmt* new_Comp_پایه_Call() ثابت;
    Ast_گره_Stmt* newCompExtCall(AstVarScope* vscp) ثابت;
    Ast_گره_Stmt* newDumpCall(AstVarScope* vscp, ثابت std::string& برچسب, bool debugOnly) ثابت;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(ثابت std::string& name) ثابت;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extra_Trigger_شاخص(uint32_t شاخص) ثابت;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::مجموعه<uint32_t> triggerIndices(ثابت Ast_Sen_درخت* senTreep, bool pre) ثابت;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_گره_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t شاخص) ثابت;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    فراغ جمع_کن_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t شاخص) ثابت;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    فراغ جمع_کن_مقدار_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t شاخص) ثابت;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<ثابت AstVarScope*, std::مجموعه<Ast_Sen_درخت*>> m_externalDomains;

عام:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_گره_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<ثابت AstVarScope*, std::وکتور<uint32_t>>
    externalTriggerIndices(ثابت TriggerKit& trigKit) ثابت VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* ثابت netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = افتراضي;
    TimingKit(LogicByScope&& lbs, Ast_گره_Stmt* postUpdates,
              std::map<ثابت AstVarScope*, std::مجموعه<Ast_Sen_درخت*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = افتراضي;
    TimingKit& عملگر=(TimingKit&&) = افتراضي;
};

class VirtIfaceTriggers final {
عام:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    بنية Trigger_مدخل final {
        ثابت AstIface* m_ifacep;  // Interface type
        ثابت AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::وکتور<Trigger_مدخل> m_triggers;

    فراغ جمع_کن_Trigger(ثابت AstIface* ifacep, ثابت AstVar* memberp, AstVarScope* vscp) {
        m_triggers.افزودن_پشت(Trigger_مدخل{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<ثابت AstVarScope*, uint32_t>;
    VscpTrigMap make_Vscp_مقصد_Trig_Map(ثابت TriggerKit& trigKit, uint32_t اولی_شاخص) ثابت;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = افتراضي;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = افتراضي;
    VirtIfaceTriggers& عملگر=(VirtIfaceTriggers&&) = افتراضي;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
عام:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::وکتور<AstVarScope*>;

خاص:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<ثابت AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<ثابت AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_رقم_Exact_Calls = 0;
    mutable uint32_t m_رقم_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    ساكن ثابت Bindings s_none;

عام:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    فراغ جمع_کن_Construction(ثابت AstClass* classp, ثابت AstVarScope* instp,
                         ثابت Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    فراغ dropInstance(ثابت AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    ثابت Bindings& forSample(ثابت AstVarScope* instp, ثابت AstClass* classp) ثابت;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t رقم_Exact_Calls() ثابت { أرجع m_رقم_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t رقم_Union_Calls() ثابت { أرجع m_رقم_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = افتراضي;
    CovergroupRefBindings(CovergroupRefBindings&&) = افتراضي;
    CovergroupRefBindings& عملگر=(CovergroupRefBindings&&) = افتراضي;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
ثابت CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* ثابت netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
فراغ transformForks(AstNetlist* ثابت netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
فراغ زمانبندی(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope break_چرخهها(AstNetlist* netlistp,
                         ثابت LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions افراز(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, ثابت string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* new_آرگومان(AstCFunc* funcp, Ast_گره_DType* dtypep, ثابت string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_گره_Stmt* مجموعه_Var(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_گره_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_گره_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
فراغ تقسیم_Check(AstCFunc* ofuncp);
}  // namespace util

فراغ beforeTrigVisitor(AstNetlist* netlistp, Sen_عبارت_Builder& sen_عبارت_Builder,
                       ثابت TriggerKit& trigKit);

}  // namespace V3Sched

#پایان_اگر  // Guard
