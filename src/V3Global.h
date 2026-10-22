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

#جے_بنا_تعریف VERILATOR_V3GLOBAL_H_
#تعریف_کرو VERILATOR_V3GLOBAL_H_

// clang-format off
#ضمن "config_build.h"
#جے_بنا_تعریف HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#جے_انت
// clang-format on

#ضمن "verilatedos.h"

#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Mutex.h"
#ضمن "V3Options.h"

#ضمن <string>
#ضمن <thread>
#ضمن <type_traits>
#ضمن <unordered_map>
#ضمن <unordered_set>

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
#تعریف_کرو VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
سانچا <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::قدر,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ثابت T m_saved;  // Value saved, for later restore

عام:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    فراغ* عامل new(ناپ_t) = delete;
    فراغ عامل delete(فراغ*) = delete;

    ثابت T& saved() ثابت { أرجع m_saved; }
};

// Implementation of VL_RESTORER_COPY
سانچا <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::قدر,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

عام:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    فراغ* عامل new(ناپ_t) = delete;
    فراغ عامل delete(فراغ*) = delete;

    ثابت T& saved() ثابت { أرجع m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
سانچا <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::قدر,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

عام:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::وٹاندرا(m_ref, m_saved);
    }
    ~VRestorerClear() { std::وٹاندرا(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    فراغ* عامل new(ناپ_t) = delete;
    فراغ عامل delete(فراغ*) = delete;

    ثابت T& saved() ثابت { أرجع m_saved; }
};

//######################################################################

class VWidthMinUsage final {
عام:
    تعداد en : uint8_t { LINT_چوڑائی, MATCHES_چوڑائی, VERILOG_چوڑائی };
    تعداد en m_e;
    VWidthMinUsage()
        : m_e{LINT_چوڑائی} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_خالص : m_e{_e} {}
    constexpr VWidthMinUsage(ثابت VWidthMinUsage& _e) VL_خالص = افتراضي;
    explicit VWidthMinUsage(صحيح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr عامل en() ثابت { أرجع m_e; }
    constexpr VWidthMinUsage& عامل=(ثابت VWidthMinUsage& _e) VL_خالص = افتراضي;
};
constexpr bool عامل==(ثابت VWidthMinUsage& lhs, ثابت VWidthMinUsage& rhs) {
    أرجع lhs.m_e == rhs.m_e;
}
constexpr bool عامل==(ثابت VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_خالص {
    أرجع lhs.m_e == rhs;
}
constexpr bool عامل==(VWidthMinUsage::en lhs, ثابت VWidthMinUsage& rhs) {
    أرجع lhs == rhs.m_e;
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
    bool m_ورتدا_Prob_Dist = false;  // Uses $dist_*
    bool m_ورتدا_Std_Package = false;  // Design uses the std package
    bool m_ورتدا_Timing = false;  // Design uses timing constructs
    bool m_ورتدا_Force = false;  // Design uses force/release statements
    bool m_ورتدا_Zero_Delay = false;  // Design uses #0 delay (or non-constant delay)
    bool m_has_Forceable_اشارے = false;  // Need to apply V3Force pass
    bool m_has_سونپو_Deassign = false;  // Need to apply V3Force pass for assign/deassign statements
    bool m_useParallelBuild = false;  // Use parallel build for model
    bool m_useRandSequence = false;  // Has `randsequence`
    bool m_useCovergroup = false;  // Has covergroup declarations
    bool m_useRandomizeMethods = false;  // Need to define randomize() class methods
    bool m_hasPrintedObjects = false;  // Design has format args printed with to_string()
    uint64_t m_current_Hier_بلاک_لاگت = 0;  // Total cost of this hier block, used for scheduling

    // Memory address to short string mapping (for debug)
    std::unordered_map<ثابت فراغ*, std::string>
        m_ptr_منزل_Id;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_سیٹ<std::string> m_json_Ptr_ناں;

    // Id of the main thread
    ثابت std::thread::id m_mainThreadId = std::this_thread::get_id();

عام:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = افتراضي;
    فراغ boot();
    فراغ shutdown();  // Release allocated resources

    فراغ vlExit(صحيح حالت);

    // ACCESSORS (general)
    AstNetlist* rootp() ثابت VL_MT_محفوظ { أرجع m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ثابت VL_خالص { أرجع m_constPoolp; }
    V3LibMap* libMapp() ثابت VL_خالص { أرجع m_libMapp; }
    V3ThreadPool* threadPoolp() ثابت VL_خالص { أرجع m_threadPoolp; }
    فراغ threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage چوڑائی_Min_Usage() ثابت VL_خالص { أرجع m_چوڑائی_Min_Usage; }
    bool assertDTypesResolved() ثابت { أرجع m_assertDTypesResolved; }
    bool assertScoped() ثابت { أرجع m_assertScoped; }
    bool staticsLifted() ثابت { أرجع m_staticsLifted; }

    // METHODS
    فراغ readFiles() VL_MT_DISABLED;
    فراغ removeStd() VL_MT_DISABLED;
    فراغ check_درخت() ثابت;
    ساكن فراغ dump_Check_Global_درخت(ثابت string& stagename, صحيح new_نمبر = 0, bool doDump = true,
                                    bool doCheck = true);
    فراغ assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    فراغ assertScoped(bool flag) { m_assertScoped = flag; }
    فراغ staticsLifted(bool flag) { m_staticsLifted = flag; }
    فراغ چوڑائی_Min_Usage(ثابت VWidthMinUsage& flag) { m_چوڑائی_Min_Usage = flag; }
    bool constRemoveXs() ثابت { أرجع m_constRemoveXs; }
    فراغ constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ثابت string& nameComment, صحيح new_نمبر = 0);
    ساكن string digitsFilename(صحيح نمبر);
    bool needTraceDumper() ثابت { أرجع m_needTraceDumper; }
    فراغ needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ثابت VL_MT_محفوظ { أرجع m_dpi; }
    فراغ dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ثابت { أرجع m_assignsEvents; }
    فراغ سیٹ_Assigns_Events() { m_assignsEvents = true; }
    bool hasEvents() ثابت { أرجع m_hasEvents; }
    فراغ سیٹ_Has_Events() { m_hasEvents = true; }
    bool hasClasses() ثابت { أرجع m_hasClasses; }
    فراغ سیٹ_Has_Classes() { m_hasClasses = true; }
    bool hasSampled() ثابت { أرجع m_hasSampled; }
    فراغ سیٹ_Has_Sampled() { m_hasSampled = true; }
    bool has_جدول() ثابت { أرجع m_has_جدول; }
    فراغ سیٹ_Has_جدول() { m_has_جدول = true; }
    bool hasVirtIfaces() ثابت { أرجع m_hasVirtIfaces; }
    فراغ سیٹ_Has_Virt_Ifaces() { m_hasVirtIfaces = true; }
    bool ورتدا_Prob_Dist() ثابت { أرجع m_ورتدا_Prob_Dist; }
    فراغ سیٹ_ورتدا_Prob_Dist() { m_ورتدا_Prob_Dist = true; }
    bool ورتدا_Std_Package() ثابت { أرجع m_ورتدا_Std_Package; }
    فراغ سیٹ_ورتدا_Std_Package() { m_ورتدا_Std_Package = true; }
    bool ورتدا_Timing() ثابت { أرجع m_ورتدا_Timing; }
    فراغ سیٹ_ورتدا_Timing() { m_ورتدا_Timing = true; }
    bool ورتدا_Zero_Delay() ثابت { أرجع m_ورتدا_Zero_Delay; }
    فراغ سیٹ_ورتدا_Zero_Delay() { m_ورتدا_Zero_Delay = true; }
    bool has_Forceable_اشارے() ثابت { أرجع m_has_Forceable_اشارے; }
    فراغ سیٹ_Has_Forceable_اشارے() { m_has_Forceable_اشارے = true; }
    bool has_سونپو_Deassign() ثابت { أرجع m_has_سونپو_Deassign; }
    فراغ سیٹ_Has_سونپو_Deassign() { m_has_سونپو_Deassign = true; }
    bool ورتدا_Force() ثابت { أرجع m_ورتدا_Force; }
    فراغ سیٹ_ورتدا_Force() { m_ورتدا_Force = true; }
    V3_Hier_گراف* hierGraphp() ثابت { أرجع m_hierGraphp; }
    فراغ hierGraphp(V3_Hier_گراف* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ثابت { أرجع m_useParallelBuild; }
    فراغ useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ثابت { أرجع m_useRandSequence; }
    فراغ useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ثابت { أرجع m_useCovergroup; }
    فراغ useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ثابت { أرجع m_useRandomizeMethods; }
    فراغ useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ثابت { أرجع m_hasPrintedObjects; }
    فراغ hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    فراغ save_Json_Ptr_میدان_Name(ثابت std::string& میدان_Name);
    فراغ ptr_ناں_Dump_Json(std::ostream& os);
    فراغ idPtrMapDumpJson(std::ostream& os);
    ثابت std::string& ptr_منزل_Id(ثابت فراغ* p);
    std::thread::id mainThreadId() ثابت { أرجع m_mainThreadId; }
    ساكن std::سمتیہ<std::string> verilatedCppFiles();
    uint64_t current_Hier_بلاک_لاگت() ثابت { أرجع m_current_Hier_بلاک_لاگت; }
    فراغ current_Hier_بلاک_لاگت(uint64_t لاگت) { m_current_Hier_بلاک_لاگت = لاگت; }
};

خارجي V3Global v3Global;

//######################################################################

#جے_انت  // guard
