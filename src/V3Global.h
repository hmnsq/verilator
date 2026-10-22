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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3GLOBAL_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3GLOBAL_H_

// clang-format off
#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#ਜੇ_ਅੰਤ
// clang-format on

#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Mutex.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Options.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <string>
#ਅਨ੍ਤਰ੍ਭਾਵ <thread>
#ਅਨ੍ਤਰ੍ਭਾਵ <type_traits>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

class AstNetlist;
class V3ConstPool;
class V3_Hier_ਗ੍ਰਾਫ;
class V3LibMap;
class V3ThreadPool;

//======================================================================
// Restorer

// For all flavours, 'var' must be a named scalar variable (simple identifier).

// Copy given variable's value on the stack, copy saved value back at end-of-scope.
// Only usable for trivially copyable types; use VL_RESTORER_COPY/VL_RESTORER_CLEAR for
// non-trivially copyable types, which are more efficient.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_RESTORER(var) \
    ਅਚਲ VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_RESTORER_COPY(var) \
    ਅਚਲ VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_RESTORER_CLEAR(var) \
    ਅਚਲ VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
ਸਾਂਚਾ <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::ਮੁੱਲ,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ਅਚਲ T m_saved;  // Value saved, for later restore

ਜਨਤਕ:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    ਖਾਲੀ* ਸੰਚਾਲਕ new(ਮਾਪ_t) = delete;
    ਖਾਲੀ ਸੰਚਾਲਕ delete(ਖਾਲੀ*) = delete;

    ਅਚਲ T& saved() ਅਚਲ { ਵਾਪਸ m_saved; }
};

// Implementation of VL_RESTORER_COPY
ਸਾਂਚਾ <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::ਮੁੱਲ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

ਜਨਤਕ:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    ਖਾਲੀ* ਸੰਚਾਲਕ new(ਮਾਪ_t) = delete;
    ਖਾਲੀ ਸੰਚਾਲਕ delete(ਖਾਲੀ*) = delete;

    ਅਚਲ T& saved() ਅਚਲ { ਵਾਪਸ m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
ਸਾਂਚਾ <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::ਮੁੱਲ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

ਜਨਤਕ:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::ਅਦਲਾ_ਬਦਲੀ(m_ref, m_saved);
    }
    ~VRestorerClear() { std::ਅਦਲਾ_ਬਦਲੀ(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    ਖਾਲੀ* ਸੰਚਾਲਕ new(ਮਾਪ_t) = delete;
    ਖਾਲੀ ਸੰਚਾਲਕ delete(ਖਾਲੀ*) = delete;

    ਅਚਲ T& saved() ਅਚਲ { ਵਾਪਸ m_saved; }
};

//######################################################################

class VWidthMinUsage final {
ਜਨਤਕ:
    ਗਿਣਤੀ en : uint8_t { LINT_ਚੌੜਾਈ, MATCHES_ਚੌੜਾਈ, VERILOG_ਚੌੜਾਈ };
    ਗਿਣਤੀ en m_e;
    VWidthMinUsage()
        : m_e{LINT_ਚੌੜਾਈ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_ਸ਼ੁੱਧ : m_e{_e} {}
    constexpr VWidthMinUsage(ਅਚਲ VWidthMinUsage& _e) VL_ਸ਼ੁੱਧ = ਮੂਲ;
    explicit VWidthMinUsage(ਪੂਰਨ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ਸੰਚਾਲਕ en() ਅਚਲ { ਵਾਪਸ m_e; }
    constexpr VWidthMinUsage& ਸੰਚਾਲਕ=(ਅਚਲ VWidthMinUsage& _e) VL_ਸ਼ੁੱਧ = ਮੂਲ;
};
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VWidthMinUsage& lhs, ਅਚਲ VWidthMinUsage& rhs) {
    ਵਾਪਸ lhs.m_e == rhs.m_e;
}
constexpr bool ਸੰਚਾਲਕ==(ਅਚਲ VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_ਸ਼ੁੱਧ {
    ਵਾਪਸ lhs.m_e == rhs;
}
constexpr bool ਸੰਚਾਲਕ==(VWidthMinUsage::en lhs, ਅਚਲ VWidthMinUsage& rhs) {
    ਵਾਪਸ lhs == rhs.m_e;
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
    V3_Hier_ਗ੍ਰਾਫ* m_hierGraphp = nullptr;
    // Thread Pool, nullptr unless 'verilatedJobs' is known, set via threadPoolp(V3ThreadPool*)
    V3ThreadPool* m_threadPoolp = nullptr;
    // Library Mapping, nullptr unless --libmap is used
    V3LibMap* m_libMapp = nullptr;
    VWidthMinUsage m_ਚੌੜਾਈ_Min_Usage
        = VWidthMinUsage::LINT_ਚੌੜਾਈ;  // What AstNode::widthMin() is used for

    std::atomic_int m_debug_File_ਸੰਖਿਆ{0};  // Number to append to debug files created
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
    bool m_has_ਸਾਰਣੀ = false;  // Desgin has the UDP Table.
    bool m_hasVirtIfaces = false;  // Design uses virtual interfaces
    bool m_ਵਰਤਦਾ_Prob_Dist = false;  // Uses $dist_*
    bool m_ਵਰਤਦਾ_Std_Package = false;  // Design uses the std package
    bool m_ਵਰਤਦਾ_Timing = false;  // Design uses timing constructs
    bool m_ਵਰਤਦਾ_Force = false;  // Design uses force/release statements
    bool m_ਵਰਤਦਾ_Zero_Delay = false;  // Design uses #0 delay (or non-constant delay)
    bool m_has_Forceable_ਸੰਕੇਤ = false;  // Need to apply V3Force pass
    bool m_has_ਸੌਂਪੋ_Deassign = false;  // Need to apply V3Force pass for assign/deassign statements
    bool m_useParallelBuild = false;  // Use parallel build for model
    bool m_useRandSequence = false;  // Has `randsequence`
    bool m_useCovergroup = false;  // Has covergroup declarations
    bool m_useRandomizeMethods = false;  // Need to define randomize() class methods
    bool m_hasPrintedObjects = false;  // Design has format args printed with to_string()
    uint64_t m_current_Hier_ਖੰਡ_ਲਾਗਤ = 0;  // Total cost of this hier block, used for scheduling

    // Memory address to short string mapping (for debug)
    std::unordered_map<ਅਚਲ ਖਾਲੀ*, std::string>
        m_ptr_ਮੰਜ਼ਿਲ_Id;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_ਸੈੱਟ<std::string> m_json_Ptr_ਨਾਂ;

    // Id of the main thread
    ਅਚਲ std::thread::id m_mainThreadId = std::this_thread::get_id();

ਜਨਤਕ:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = ਮੂਲ;
    ਖਾਲੀ boot();
    ਖਾਲੀ shutdown();  // Release allocated resources

    ਖਾਲੀ vlExit(ਪੂਰਨ ਸਥਿਤੀ);

    // ACCESSORS (general)
    AstNetlist* rootp() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ਅਚਲ VL_ਸ਼ੁੱਧ { ਵਾਪਸ m_constPoolp; }
    V3LibMap* libMapp() ਅਚਲ VL_ਸ਼ੁੱਧ { ਵਾਪਸ m_libMapp; }
    V3ThreadPool* threadPoolp() ਅਚਲ VL_ਸ਼ੁੱਧ { ਵਾਪਸ m_threadPoolp; }
    ਖਾਲੀ threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage ਚੌੜਾਈ_Min_Usage() ਅਚਲ VL_ਸ਼ੁੱਧ { ਵਾਪਸ m_ਚੌੜਾਈ_Min_Usage; }
    bool assertDTypesResolved() ਅਚਲ { ਵਾਪਸ m_assertDTypesResolved; }
    bool assertScoped() ਅਚਲ { ਵਾਪਸ m_assertScoped; }
    bool staticsLifted() ਅਚਲ { ਵਾਪਸ m_staticsLifted; }

    // METHODS
    ਖਾਲੀ readFiles() VL_MT_DISABLED;
    ਖਾਲੀ removeStd() VL_MT_DISABLED;
    ਖਾਲੀ check_ਰੁੱਖ() ਅਚਲ;
    ਸਥਿਰ ਖਾਲੀ dump_Check_Global_ਰੁੱਖ(ਅਚਲ string& stagename, ਪੂਰਨ new_ਸੰਖਿਆ = 0, bool doDump = true,
                                    bool doCheck = true);
    ਖਾਲੀ assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    ਖਾਲੀ assertScoped(bool flag) { m_assertScoped = flag; }
    ਖਾਲੀ staticsLifted(bool flag) { m_staticsLifted = flag; }
    ਖਾਲੀ ਚੌੜਾਈ_Min_Usage(ਅਚਲ VWidthMinUsage& flag) { m_ਚੌੜਾਈ_Min_Usage = flag; }
    bool constRemoveXs() ਅਚਲ { ਵਾਪਸ m_constRemoveXs; }
    ਖਾਲੀ constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ਅਚਲ string& nameComment, ਪੂਰਨ new_ਸੰਖਿਆ = 0);
    ਸਥਿਰ string digitsFilename(ਪੂਰਨ ਸੰਖਿਆ);
    bool needTraceDumper() ਅਚਲ { ਵਾਪਸ m_needTraceDumper; }
    ਖਾਲੀ needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_dpi; }
    ਖਾਲੀ dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ਅਚਲ { ਵਾਪਸ m_assignsEvents; }
    ਖਾਲੀ ਸੈੱਟ_Assigns_Events() { m_assignsEvents = true; }
    bool hasEvents() ਅਚਲ { ਵਾਪਸ m_hasEvents; }
    ਖਾਲੀ ਸੈੱਟ_Has_Events() { m_hasEvents = true; }
    bool hasClasses() ਅਚਲ { ਵਾਪਸ m_hasClasses; }
    ਖਾਲੀ ਸੈੱਟ_Has_Classes() { m_hasClasses = true; }
    bool hasSampled() ਅਚਲ { ਵਾਪਸ m_hasSampled; }
    ਖਾਲੀ ਸੈੱਟ_Has_Sampled() { m_hasSampled = true; }
    bool has_ਸਾਰਣੀ() ਅਚਲ { ਵਾਪਸ m_has_ਸਾਰਣੀ; }
    ਖਾਲੀ ਸੈੱਟ_Has_ਸਾਰਣੀ() { m_has_ਸਾਰਣੀ = true; }
    bool hasVirtIfaces() ਅਚਲ { ਵਾਪਸ m_hasVirtIfaces; }
    ਖਾਲੀ ਸੈੱਟ_Has_Virt_Ifaces() { m_hasVirtIfaces = true; }
    bool ਵਰਤਦਾ_Prob_Dist() ਅਚਲ { ਵਾਪਸ m_ਵਰਤਦਾ_Prob_Dist; }
    ਖਾਲੀ ਸੈੱਟ_ਵਰਤਦਾ_Prob_Dist() { m_ਵਰਤਦਾ_Prob_Dist = true; }
    bool ਵਰਤਦਾ_Std_Package() ਅਚਲ { ਵਾਪਸ m_ਵਰਤਦਾ_Std_Package; }
    ਖਾਲੀ ਸੈੱਟ_ਵਰਤਦਾ_Std_Package() { m_ਵਰਤਦਾ_Std_Package = true; }
    bool ਵਰਤਦਾ_Timing() ਅਚਲ { ਵਾਪਸ m_ਵਰਤਦਾ_Timing; }
    ਖਾਲੀ ਸੈੱਟ_ਵਰਤਦਾ_Timing() { m_ਵਰਤਦਾ_Timing = true; }
    bool ਵਰਤਦਾ_Zero_Delay() ਅਚਲ { ਵਾਪਸ m_ਵਰਤਦਾ_Zero_Delay; }
    ਖਾਲੀ ਸੈੱਟ_ਵਰਤਦਾ_Zero_Delay() { m_ਵਰਤਦਾ_Zero_Delay = true; }
    bool has_Forceable_ਸੰਕੇਤ() ਅਚਲ { ਵਾਪਸ m_has_Forceable_ਸੰਕੇਤ; }
    ਖਾਲੀ ਸੈੱਟ_Has_Forceable_ਸੰਕੇਤ() { m_has_Forceable_ਸੰਕੇਤ = true; }
    bool has_ਸੌਂਪੋ_Deassign() ਅਚਲ { ਵਾਪਸ m_has_ਸੌਂਪੋ_Deassign; }
    ਖਾਲੀ ਸੈੱਟ_Has_ਸੌਂਪੋ_Deassign() { m_has_ਸੌਂਪੋ_Deassign = true; }
    bool ਵਰਤਦਾ_Force() ਅਚਲ { ਵਾਪਸ m_ਵਰਤਦਾ_Force; }
    ਖਾਲੀ ਸੈੱਟ_ਵਰਤਦਾ_Force() { m_ਵਰਤਦਾ_Force = true; }
    V3_Hier_ਗ੍ਰਾਫ* hierGraphp() ਅਚਲ { ਵਾਪਸ m_hierGraphp; }
    ਖਾਲੀ hierGraphp(V3_Hier_ਗ੍ਰਾਫ* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ਅਚਲ { ਵਾਪਸ m_useParallelBuild; }
    ਖਾਲੀ useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ਅਚਲ { ਵਾਪਸ m_useRandSequence; }
    ਖਾਲੀ useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ਅਚਲ { ਵਾਪਸ m_useCovergroup; }
    ਖਾਲੀ useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ਅਚਲ { ਵਾਪਸ m_useRandomizeMethods; }
    ਖਾਲੀ useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ਅਚਲ { ਵਾਪਸ m_hasPrintedObjects; }
    ਖਾਲੀ hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    ਖਾਲੀ save_Json_Ptr_ਖੇਤਰ_Name(ਅਚਲ std::string& ਖੇਤਰ_Name);
    ਖਾਲੀ ptr_ਨਾਂ_Dump_Json(std::ostream& os);
    ਖਾਲੀ idPtrMapDumpJson(std::ostream& os);
    ਅਚਲ std::string& ptr_ਮੰਜ਼ਿਲ_Id(ਅਚਲ ਖਾਲੀ* p);
    std::thread::id mainThreadId() ਅਚਲ { ਵਾਪਸ m_mainThreadId; }
    ਸਥਿਰ std::ਸਦਿਸ਼<std::string> verilatedCppFiles();
    uint64_t current_Hier_ਖੰਡ_ਲਾਗਤ() ਅਚਲ { ਵਾਪਸ m_current_Hier_ਖੰਡ_ਲਾਗਤ; }
    ਖਾਲੀ current_Hier_ਖੰਡ_ਲਾਗਤ(uint64_t ਲਾਗਤ) { m_current_Hier_ਖੰਡ_ਲਾਗਤ = ਲਾਗਤ; }
};

ਬਾਹਰੀ V3Global v3Global;

//######################################################################

#ਜੇ_ਅੰਤ  // guard
