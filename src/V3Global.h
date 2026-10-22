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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3GLOBAL_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3GLOBAL_H_

// clang-format off
#ಅನ್ತರ್ಭಾವ "config_build.h"
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"
#ಅನ್ತರ್ಭಾವ "V3Options.h"

#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <thread>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

class AstNetlist;
class V3ConstPool;
class V3_Hier_ನಕ್ಷೆ;
class V3LibMap;
class V3ThreadPool;

//======================================================================
// Restorer

// For all flavours, 'var' must be a named scalar variable (simple identifier).

// Copy given variable's value on the stack, copy saved value back at end-of-scope.
// Only usable for trivially copyable types; use VL_RESTORER_COPY/VL_RESTORER_CLEAR for
// non-trivially copyable types, which are more efficient.
#ವ್ಯಾಖ್ಯಾನಿಸು VL_RESTORER(var) \
    ಸ್ಥಿರಾಂಕ VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#ವ್ಯಾಖ್ಯಾನಿಸು VL_RESTORER_COPY(var) \
    ಸ್ಥಿರಾಂಕ VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#ವ್ಯಾಖ್ಯಾನಿಸು VL_RESTORER_CLEAR(var) \
    ಸ್ಥಿರಾಂಕ VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#ವ್ಯಾಖ್ಯಾನಿಸು VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
ಮಾದರಿ <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::ಮೌಲ್ಯ,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ಸ್ಥಿರಾಂಕ T m_saved;  // Value saved, for later restore

ಸಾರ್ವಜನಿಕ:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    ಶೂನ್ಯ* ಕ್ರಿಯಾಕಾರಕ new(ಗಾತ್ರ_t) = delete;
    ಶೂನ್ಯ ಕ್ರಿಯಾಕಾರಕ delete(ಶೂನ್ಯ*) = delete;

    ಸ್ಥಿರಾಂಕ T& saved() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_saved; }
};

// Implementation of VL_RESTORER_COPY
ಮಾದರಿ <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::ಮೌಲ್ಯ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

ಸಾರ್ವಜನಿಕ:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    ಶೂನ್ಯ* ಕ್ರಿಯಾಕಾರಕ new(ಗಾತ್ರ_t) = delete;
    ಶೂನ್ಯ ಕ್ರಿಯಾಕಾರಕ delete(ಶೂನ್ಯ*) = delete;

    ಸ್ಥಿರಾಂಕ T& saved() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
ಮಾದರಿ <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::ಮೌಲ್ಯ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

ಸಾರ್ವಜನಿಕ:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::ಅದಲುಬದಲು(m_ref, m_saved);
    }
    ~VRestorerClear() { std::ಅದಲುಬದಲು(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    ಶೂನ್ಯ* ಕ್ರಿಯಾಕಾರಕ new(ಗಾತ್ರ_t) = delete;
    ಶೂನ್ಯ ಕ್ರಿಯಾಕಾರಕ delete(ಶೂನ್ಯ*) = delete;

    ಸ್ಥಿರಾಂಕ T& saved() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_saved; }
};

//######################################################################

class VWidthMinUsage final {
ಸಾರ್ವಜನಿಕ:
    ಎಣಿಕೆ en : uint8_t { LINT_ಅಗಲ, MATCHES_ಅಗಲ, VERILOG_ಅಗಲ };
    ಎಣಿಕೆ en m_e;
    VWidthMinUsage()
        : m_e{LINT_ಅಗಲ} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_ಶುದ್ಧ : m_e{_e} {}
    constexpr VWidthMinUsage(ಸ್ಥಿರಾಂಕ VWidthMinUsage& _e) VL_ಶುದ್ಧ = ಪೂರ್ವನಿಯೋಜಿತ;
    explicit VWidthMinUsage(ಪೂರ್ಣಾಂಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr ಕ್ರಿಯಾಕಾರಕ en() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_e; }
    constexpr VWidthMinUsage& ಕ್ರಿಯಾಕಾರಕ=(ಸ್ಥಿರಾಂಕ VWidthMinUsage& _e) VL_ಶುದ್ಧ = ಪೂರ್ವನಿಯೋಜಿತ;
};
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VWidthMinUsage& lhs, ಸ್ಥಿರಾಂಕ VWidthMinUsage& rhs) {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs.m_e;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(ಸ್ಥಿರಾಂಕ VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_ಶುದ್ಧ {
    ಹಿಂತಿರುಗಿ lhs.m_e == rhs;
}
constexpr bool ಕ್ರಿಯಾಕಾರಕ==(VWidthMinUsage::en lhs, ಸ್ಥಿರಾಂಕ VWidthMinUsage& rhs) {
    ಹಿಂತಿರುಗಿ lhs == rhs.m_e;
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
    V3_Hier_ನಕ್ಷೆ* m_hierGraphp = nullptr;
    // Thread Pool, nullptr unless 'verilatedJobs' is known, set via threadPoolp(V3ThreadPool*)
    V3ThreadPool* m_threadPoolp = nullptr;
    // Library Mapping, nullptr unless --libmap is used
    V3LibMap* m_libMapp = nullptr;
    VWidthMinUsage m_ಅಗಲ_Min_Usage
        = VWidthMinUsage::LINT_ಅಗಲ;  // What AstNode::widthMin() is used for

    std::atomic_int m_debug_File_ಸಂಖ್ಯೆ{0};  // Number to append to debug files created
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
    bool m_has_ಕೋಷ್ಟಕ = false;  // Desgin has the UDP Table.
    bool m_hasVirtIfaces = false;  // Design uses virtual interfaces
    bool m_ಬಳಸುತ್ತದೆ_Prob_Dist = false;  // Uses $dist_*
    bool m_ಬಳಸುತ್ತದೆ_Std_Package = false;  // Design uses the std package
    bool m_ಬಳಸುತ್ತದೆ_Timing = false;  // Design uses timing constructs
    bool m_ಬಳಸುತ್ತದೆ_Force = false;  // Design uses force/release statements
    bool m_ಬಳಸುತ್ತದೆ_Zero_Delay = false;  // Design uses #0 delay (or non-constant delay)
    bool m_has_Forceable_ಸಂಕೇತಗಳು = false;  // Need to apply V3Force pass
    bool m_has_ನಿಯೋಜಿಸು_Deassign = false;  // Need to apply V3Force pass for assign/deassign statements
    bool m_useParallelBuild = false;  // Use parallel build for model
    bool m_useRandSequence = false;  // Has `randsequence`
    bool m_useCovergroup = false;  // Has covergroup declarations
    bool m_useRandomizeMethods = false;  // Need to define randomize() class methods
    bool m_hasPrintedObjects = false;  // Design has format args printed with to_string()
    uint64_t m_current_Hier_ಖಂಡ_ವೆಚ್ಚ = 0;  // Total cost of this hier block, used for scheduling

    // Memory address to short string mapping (for debug)
    std::unordered_map<ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ*, std::string>
        m_ptr_ಗಮ್ಯ_Id;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_ಗಣ<std::string> m_json_Ptr_ಹೆಸರುಗಳು;

    // Id of the main thread
    ಸ್ಥಿರಾಂಕ std::thread::id m_mainThreadId = std::this_thread::get_id();

ಸಾರ್ವಜನಿಕ:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = ಪೂರ್ವನಿಯೋಜಿತ;
    ಶೂನ್ಯ boot();
    ಶೂನ್ಯ shutdown();  // Release allocated resources

    ಶೂನ್ಯ vlExit(ಪೂರ್ಣಾಂಕ ಸ್ಥಿತಿ_2);

    // ACCESSORS (general)
    AstNetlist* rootp() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ m_constPoolp; }
    V3LibMap* libMapp() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ m_libMapp; }
    V3ThreadPool* threadPoolp() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ m_threadPoolp; }
    ಶೂನ್ಯ threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage ಅಗಲ_Min_Usage() ಸ್ಥಿರಾಂಕ VL_ಶುದ್ಧ { ಹಿಂತಿರುಗಿ m_ಅಗಲ_Min_Usage; }
    bool assertDTypesResolved() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_assertDTypesResolved; }
    bool assertScoped() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_assertScoped; }
    bool staticsLifted() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_staticsLifted; }

    // METHODS
    ಶೂನ್ಯ readFiles() VL_MT_DISABLED;
    ಶೂನ್ಯ removeStd() VL_MT_DISABLED;
    ಶೂನ್ಯ check_ವೃಕ್ಷ() ಸ್ಥಿರಾಂಕ;
    ಸ್ಥಿರ ಶೂನ್ಯ dump_Check_Global_ವೃಕ್ಷ(ಸ್ಥಿರಾಂಕ string& stagename, ಪೂರ್ಣಾಂಕ new_ಸಂಖ್ಯೆ = 0, bool doDump = true,
                                    bool doCheck = true);
    ಶೂನ್ಯ assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    ಶೂನ್ಯ assertScoped(bool flag) { m_assertScoped = flag; }
    ಶೂನ್ಯ staticsLifted(bool flag) { m_staticsLifted = flag; }
    ಶೂನ್ಯ ಅಗಲ_Min_Usage(ಸ್ಥಿರಾಂಕ VWidthMinUsage& flag) { m_ಅಗಲ_Min_Usage = flag; }
    bool constRemoveXs() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_constRemoveXs; }
    ಶೂನ್ಯ constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ಸ್ಥಿರಾಂಕ string& nameComment, ಪೂರ್ಣಾಂಕ new_ಸಂಖ್ಯೆ = 0);
    ಸ್ಥಿರ string digitsFilename(ಪೂರ್ಣಾಂಕ ಸಂಖ್ಯೆ);
    bool needTraceDumper() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_needTraceDumper; }
    ಶೂನ್ಯ needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_dpi; }
    ಶೂನ್ಯ dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_assignsEvents; }
    ಶೂನ್ಯ ಗಣ_Assigns_Events() { m_assignsEvents = true; }
    bool hasEvents() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasEvents; }
    ಶೂನ್ಯ ಗಣ_Has_Events() { m_hasEvents = true; }
    bool hasClasses() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasClasses; }
    ಶೂನ್ಯ ಗಣ_Has_Classes() { m_hasClasses = true; }
    bool hasSampled() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasSampled; }
    ಶೂನ್ಯ ಗಣ_Has_Sampled() { m_hasSampled = true; }
    bool has_ಕೋಷ್ಟಕ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_has_ಕೋಷ್ಟಕ; }
    ಶೂನ್ಯ ಗಣ_Has_ಕೋಷ್ಟಕ() { m_has_ಕೋಷ್ಟಕ = true; }
    bool hasVirtIfaces() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasVirtIfaces; }
    ಶೂನ್ಯ ಗಣ_Has_Virt_Ifaces() { m_hasVirtIfaces = true; }
    bool ಬಳಸುತ್ತದೆ_Prob_Dist() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸುತ್ತದೆ_Prob_Dist; }
    ಶೂನ್ಯ ಗಣ_ಬಳಸುತ್ತದೆ_Prob_Dist() { m_ಬಳಸುತ್ತದೆ_Prob_Dist = true; }
    bool ಬಳಸುತ್ತದೆ_Std_Package() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸುತ್ತದೆ_Std_Package; }
    ಶೂನ್ಯ ಗಣ_ಬಳಸುತ್ತದೆ_Std_Package() { m_ಬಳಸುತ್ತದೆ_Std_Package = true; }
    bool ಬಳಸುತ್ತದೆ_Timing() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸುತ್ತದೆ_Timing; }
    ಶೂನ್ಯ ಗಣ_ಬಳಸುತ್ತದೆ_Timing() { m_ಬಳಸುತ್ತದೆ_Timing = true; }
    bool ಬಳಸುತ್ತದೆ_Zero_Delay() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸುತ್ತದೆ_Zero_Delay; }
    ಶೂನ್ಯ ಗಣ_ಬಳಸುತ್ತದೆ_Zero_Delay() { m_ಬಳಸುತ್ತದೆ_Zero_Delay = true; }
    bool has_Forceable_ಸಂಕೇತಗಳು() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_has_Forceable_ಸಂಕೇತಗಳು; }
    ಶೂನ್ಯ ಗಣ_Has_Forceable_ಸಂಕೇತಗಳು() { m_has_Forceable_ಸಂಕೇತಗಳು = true; }
    bool has_ನಿಯೋಜಿಸು_Deassign() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_has_ನಿಯೋಜಿಸು_Deassign; }
    ಶೂನ್ಯ ಗಣ_Has_ನಿಯೋಜಿಸು_Deassign() { m_has_ನಿಯೋಜಿಸು_Deassign = true; }
    bool ಬಳಸುತ್ತದೆ_Force() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ಬಳಸುತ್ತದೆ_Force; }
    ಶೂನ್ಯ ಗಣ_ಬಳಸುತ್ತದೆ_Force() { m_ಬಳಸುತ್ತದೆ_Force = true; }
    V3_Hier_ನಕ್ಷೆ* hierGraphp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hierGraphp; }
    ಶೂನ್ಯ hierGraphp(V3_Hier_ನಕ್ಷೆ* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_useParallelBuild; }
    ಶೂನ್ಯ useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_useRandSequence; }
    ಶೂನ್ಯ useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_useCovergroup; }
    ಶೂನ್ಯ useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_useRandomizeMethods; }
    ಶೂನ್ಯ useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hasPrintedObjects; }
    ಶೂನ್ಯ hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    ಶೂನ್ಯ save_Json_Ptr_ಕ್ಷೇತ್ರ_Name(ಸ್ಥಿರಾಂಕ std::string& ಕ್ಷೇತ್ರ_Name);
    ಶೂನ್ಯ ptr_ಹೆಸರುಗಳು_Dump_Json(std::ostream& os);
    ಶೂನ್ಯ idPtrMapDumpJson(std::ostream& os);
    ಸ್ಥಿರಾಂಕ std::string& ptr_ಗಮ್ಯ_Id(ಸ್ಥಿರಾಂಕ ಶೂನ್ಯ* p);
    std::thread::id mainThreadId() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_mainThreadId; }
    ಸ್ಥಿರ std::ಸದಿಶ<std::string> verilatedCppFiles();
    uint64_t current_Hier_ಖಂಡ_ವೆಚ್ಚ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_current_Hier_ಖಂಡ_ವೆಚ್ಚ; }
    ಶೂನ್ಯ current_Hier_ಖಂಡ_ವೆಚ್ಚ(uint64_t ವೆಚ್ಚ) { m_current_Hier_ಖಂಡ_ವೆಚ್ಚ = ವೆಚ್ಚ; }
};

ಬಾಹ್ಯ V3Global v3Global;

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
