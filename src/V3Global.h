// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Common headers
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

#اگر_غیر_تعریف_شدہ VERILATOR_V3GLOBAL_H_
#تعریف_کرو VERILATOR_V3GLOBAL_H_

// clang-format off
#شامل "config_build.h"
#اگر_غیر_تعریف_شدہ HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#اگر_ختم
// clang-format on

#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Mutex.h"
#شامل "V3Options.h"

#شامل <string>
#شامل <thread>
#شامل <type_traits>
#شامل <unordered_map>
#شامل <unordered_set>

class AstNetlist;
class V3ConstPool;
class V3_Hier_گراف;
class V3LibMap;
class V3ThreadPool;

//======================================================================
// Restorer

// For all flavours, 'var' must be a named scalar variable (simple identifier).

// Copy given variable's value on the stack, copy saved value back at end-of-scope.
// Only usable for trivially copyable types; use VL_RESTORER_COPY/VL_RESTORER_CLEAR for
// non-trivially copyable types, which are more efficient.
#تعریف_کرو VL_RESTORER(var) \
    ثابت VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#تعریف_کرو VL_RESTORER_COPY(var) \
    ثابت VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#تعریف_کرو VL_RESTORER_CLEAR(var) \
    ثابت VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#تعریف_کرو VL_RESTORER_پچھلا(var) restorer_##var.saved()

// Implementation of VL_RESTORER
سانچہ <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::قدر,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ثابت T m_saved;  // Value saved, for later restore

عوامی:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    خالی* عامل new(سائز_t) = delete;
    خالی عامل delete(خالی*) = delete;

    ثابت T& saved() ثابت { واپس m_saved; }
};

// Implementation of VL_RESTORER_COPY
سانچہ <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::قدر,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

عوامی:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    خالی* عامل new(سائز_t) = delete;
    خالی عامل delete(خالی*) = delete;

    ثابت T& saved() ثابت { واپس m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
سانچہ <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::قدر,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

عوامی:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::تبادلہ(m_ref, m_saved);
    }
    ~VRestorerClear() { std::تبادلہ(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    خالی* عامل new(سائز_t) = delete;
    خالی عامل delete(خالی*) = delete;

    ثابت T& saved() ثابت { واپس m_saved; }
};

//######################################################################

class VWidthMinUsage final {
عوامی:
    شمار en : uint8_t { LINT_چوڑائی, MATCHES_چوڑائی, VERILOG_چوڑائی };
    شمار en m_e;
    VWidthMinUsage()
        : m_e{LINT_چوڑائی} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_خالص : m_e{_e} {}
    constexpr VWidthMinUsage(ثابت VWidthMinUsage& _e) VL_خالص = طےشدہ;
    explicit VWidthMinUsage(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { واپس m_e; }
    constexpr VWidthMinUsage& عامل=(ثابت VWidthMinUsage& _e) VL_خالص = طےشدہ;
};
constexpr bool عامل==(ثابت VWidthMinUsage& lhs, ثابت VWidthMinUsage& rhs) {
    واپس lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_خالص {
    واپس lhs.m_e == rhs;
}
constexpr bool عامل==(VWidthMinUsage::en lhs, ثابت VWidthMinUsage& rhs) {
    واپس lhs == rhs.m_e;
}

//######################################################################
// V3Global - The top level class for the entire program

class V3Global final {
    // Globals
    // Root of entire netlist, created by makeInitNetlist(} so static constructors run first
    AstNetlist* m_rootp = nullptr;
    // Constant pool lookup, see V3ConstPool.h
    V3ConstPool* m_constPoolp = nullptr;
    // Hierarchical block graph (plan) iff hierarchical verilation is performed
    V3_Hier_گراف* m_hierGraphp = nullptr;
    // Thread Pool, nullptr unless 'verilatedJobs' is known, set via threadPoolp(V3ThreadPool*)
    V3ThreadPool* m_threadPoolp = nullptr;
    // Library Mapping, nullptr unless --libmap is used
    V3LibMap* m_libMapp = nullptr;
    VWidthMinUsage m_چوڑائی_Min_Usage
        = VWidthMinUsage::LINT_چوڑائی;  // What AstNode::widthMin() is used for

    std::atomic_int m_debug_File_نمبر{0};  // Number to append to debug files created
    bool m_assertDTypesResolved = false;  // Tree should have dtypep()'s
    bool m_assertScoped = false;  // Tree is scoped
    bool m_staticsLifted = false;  // Static variables have been lifted to module scope
    bool m_assignsEvents = false;  // Design uses assignments on SystemVerilog Events
    bool m_constRemoveXs = false;  // Const needs to strip any Xs
    // Experimenting with always requiring heavy, see issue #2701
    bool m_needTraceDumper = false;  // Need __Vm_dumperp in symbols
    bool m_dpi = false;  // Need __Dpi include files
    bool m_hasEvents = false;  // Design uses SystemVerilog named events
    bool m_hasClasses = false;  // Design uses SystemVerilog classes
    bool m_hasSampled = false;  // Design uses SAMPLED expresions
    bool m_has_جدول = false;  // Desgin has the UDP Table.
    bool m_hasVirtIfaces = false;  // Design uses virtual interfaces
    bool m_استعمال_کرتا_Prob_Dist = false;  // Uses $dist_*
    bool m_استعمال_کرتا_Std_Package = false;  // Design uses the std package
    bool m_استعمال_کرتا_Timing = false;  // Design uses timing constructs
    bool m_استعمال_کرتا_Force = false;  // Design uses force/release statements
    bool m_استعمال_کرتا_Zero_Delay = false;  // Design uses #0 delay (or non-constant delay)
    bool m_has_Forceable_اشارات = false;  // Need to apply V3Force pass
    bool m_has_تفویض_Deassign = false;  // Need to apply V3Force pass for assign/deassign statements
    bool m_useParallelBuild = false;  // Use parallel build for model
    bool m_useRandSequence = false;  // Has `randsequence`
    bool m_useCovergroup = false;  // Has covergroup declarations
    bool m_useRandomizeMethods = false;  // Need to define randomize() class methods
    bool m_hasPrintedObjects = false;  // Design has format args printed with to_string()
    uint64_t m_current_Hier_بلاک_لاگت = 0;  // Total cost of this hier block, used for scheduling

    // Memory address to short string mapping (for debug)
    std::unordered_map<ثابت خالی*, std::string>
        m_ptr_مقصد_Id;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_سیٹ<std::string> m_json_Ptr_نام;

    // Id of the main thread
    ثابت std::thread::id m_mainThreadId = std::this_thread::get_id();

عوامی:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = طےشدہ;
    خالی boot();
    خالی shutdown();  // Release allocated resources

    خالی vlExit(صحیح حالت_2);

    // ACCESSORS (general)
    AstNetlist* rootp() ثابت VL_MT_محفوظ { واپس m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ثابت VL_خالص { واپس m_constPoolp; }
    V3LibMap* libMapp() ثابت VL_خالص { واپس m_libMapp; }
    V3ThreadPool* threadPoolp() ثابت VL_خالص { واپس m_threadPoolp; }
    خالی threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage چوڑائی_Min_Usage() ثابت VL_خالص { واپس m_چوڑائی_Min_Usage; }
    bool assertDTypesResolved() ثابت { واپس m_assertDTypesResolved; }
    bool assertScoped() ثابت { واپس m_assertScoped; }
    bool staticsLifted() ثابت { واپس m_staticsLifted; }

    // METHODS
    خالی readFiles() VL_MT_DISABLED;
    خالی removeStd() VL_MT_DISABLED;
    خالی check_درخت() ثابت;
    ساکن خالی dump_Check_Global_درخت(ثابت string& stagename, صحیح new_نمبر = 0, bool doDump = true,
                                    bool doCheck = true);
    خالی assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    خالی assertScoped(bool flag) { m_assertScoped = flag; }
    خالی staticsLifted(bool flag) { m_staticsLifted = flag; }
    خالی چوڑائی_Min_Usage(ثابت VWidthMinUsage& flag) { m_چوڑائی_Min_Usage = flag; }
    bool constRemoveXs() ثابت { واپس m_constRemoveXs; }
    خالی constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ثابت string& nameComment, صحیح new_نمبر = 0);
    ساکن string digitsFilename(صحیح نمبر);
    bool needTraceDumper() ثابت { واپس m_needTraceDumper; }
    خالی needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ثابت VL_MT_محفوظ { واپس m_dpi; }
    خالی dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ثابت { واپس m_assignsEvents; }
    خالی سیٹ_Assigns_Events() { m_assignsEvents = true; }
    bool hasEvents() ثابت { واپس m_hasEvents; }
    خالی سیٹ_Has_Events() { m_hasEvents = true; }
    bool hasClasses() ثابت { واپس m_hasClasses; }
    خالی سیٹ_Has_Classes() { m_hasClasses = true; }
    bool hasSampled() ثابت { واپس m_hasSampled; }
    خالی سیٹ_Has_Sampled() { m_hasSampled = true; }
    bool has_جدول() ثابت { واپس m_has_جدول; }
    خالی سیٹ_Has_جدول() { m_has_جدول = true; }
    bool hasVirtIfaces() ثابت { واپس m_hasVirtIfaces; }
    خالی سیٹ_Has_Virt_Ifaces() { m_hasVirtIfaces = true; }
    bool استعمال_کرتا_Prob_Dist() ثابت { واپس m_استعمال_کرتا_Prob_Dist; }
    خالی سیٹ_استعمال_کرتا_Prob_Dist() { m_استعمال_کرتا_Prob_Dist = true; }
    bool استعمال_کرتا_Std_Package() ثابت { واپس m_استعمال_کرتا_Std_Package; }
    خالی سیٹ_استعمال_کرتا_Std_Package() { m_استعمال_کرتا_Std_Package = true; }
    bool استعمال_کرتا_Timing() ثابت { واپس m_استعمال_کرتا_Timing; }
    خالی سیٹ_استعمال_کرتا_Timing() { m_استعمال_کرتا_Timing = true; }
    bool استعمال_کرتا_Zero_Delay() ثابت { واپس m_استعمال_کرتا_Zero_Delay; }
    خالی سیٹ_استعمال_کرتا_Zero_Delay() { m_استعمال_کرتا_Zero_Delay = true; }
    bool has_Forceable_اشارات() ثابت { واپس m_has_Forceable_اشارات; }
    خالی سیٹ_Has_Forceable_اشارات() { m_has_Forceable_اشارات = true; }
    bool has_تفویض_Deassign() ثابت { واپس m_has_تفویض_Deassign; }
    خالی سیٹ_Has_تفویض_Deassign() { m_has_تفویض_Deassign = true; }
    bool استعمال_کرتا_Force() ثابت { واپس m_استعمال_کرتا_Force; }
    خالی سیٹ_استعمال_کرتا_Force() { m_استعمال_کرتا_Force = true; }
    V3_Hier_گراف* hierGraphp() ثابت { واپس m_hierGraphp; }
    خالی hierGraphp(V3_Hier_گراف* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ثابت { واپس m_useParallelBuild; }
    خالی useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ثابت { واپس m_useRandSequence; }
    خالی useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ثابت { واپس m_useCovergroup; }
    خالی useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ثابت { واپس m_useRandomizeMethods; }
    خالی useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ثابت { واپس m_hasPrintedObjects; }
    خالی hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    خالی save_Json_Ptr_میدان_Name(ثابت std::string& میدان_Name);
    خالی ptr_نام_Dump_Json(std::ostream& os);
    خالی idPtrMapDumpJson(std::ostream& os);
    ثابت std::string& ptr_مقصد_Id(ثابت خالی* p);
    std::thread::id mainThreadId() ثابت { واپس m_mainThreadId; }
    ساکن std::سمتیہ<std::string> verilatedCppFiles();
    uint64_t current_Hier_بلاک_لاگت() ثابت { واپس m_current_Hier_بلاک_لاگت; }
    خالی current_Hier_بلاک_لاگت(uint64_t لاگت) { m_current_Hier_بلاک_لاگت = لاگت; }
};

خارجی V3Global v3Global;

//######################################################################

#اگر_ختم  // guard
