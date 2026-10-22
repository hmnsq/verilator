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

#ifndef VERILATOR_V3GLOBAL_H_
#define VERILATOR_V3GLOBAL_H_

// clang-format off
#അന്തര്ഭാവ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3Mutex.h"
#അന്തര്ഭാവ "V3Options.h"

#അന്തര്ഭാവ <string>
#അന്തര്ഭാവ <thread>
#അന്തര്ഭാവ <type_traits>
#അന്തര്ഭാവ <unordered_map>
#അന്തര്ഭാവ <unordered_set>

class AstNetlist;
class V3ConstPool;
class V3HierGraph;
class V3LibMap;
class V3ThreadPool;

//======================================================================
// Restorer

// For all flavours, 'var' must be a named scalar variable (simple identifier).

// Copy given variable's value on the stack, copy saved value back at end-of-scope.
// Only usable for trivially copyable types; use VL_RESTORER_COPY/VL_RESTORER_CLEAR for
// non-trivially copyable types, which are more efficient.
#define VL_RESTORER(var) \
    സ്ഥിരം VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#define VL_RESTORER_COPY(var) \
    സ്ഥിരം VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#define VL_RESTORER_CLEAR(var) \
    സ്ഥിരം VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#define VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
template <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::മൂല്യം,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    സ്ഥിരം T m_saved;  // Value saved, for later restore

പൊതു:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    ശൂന്യം* operator new(size_t) = delete;
    ശൂന്യം operator delete(ശൂന്യം*) = delete;

    സ്ഥിരം T& saved() സ്ഥിരം { തിരികെ m_saved; }
};

// Implementation of VL_RESTORER_COPY
template <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::മൂല്യം,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

പൊതു:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    ശൂന്യം* operator new(size_t) = delete;
    ശൂന്യം operator delete(ശൂന്യം*) = delete;

    സ്ഥിരം T& saved() സ്ഥിരം { തിരികെ m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
template <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::മൂല്യം,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

പൊതു:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    ശൂന്യം* operator new(size_t) = delete;
    ശൂന്യം operator delete(ശൂന്യം*) = delete;

    സ്ഥിരം T& saved() സ്ഥിരം { തിരികെ m_saved; }
};

//######################################################################

class VWidthMinUsage final {
പൊതു:
    എണ്ണൽ en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    എണ്ണൽ en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(സ്ഥിരം VWidthMinUsage& _e) VL_PURE = സ്ഥിരസ്ഥിതി;
    explicit VWidthMinUsage(പൂർണ്ണസംഖ്യ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() സ്ഥിരം { തിരികെ m_e; }
    constexpr VWidthMinUsage& operator=(സ്ഥിരം VWidthMinUsage& _e) VL_PURE = സ്ഥിരസ്ഥിതി;
};
constexpr bool operator==(സ്ഥിരം VWidthMinUsage& lhs, സ്ഥിരം VWidthMinUsage& rhs) {
    തിരികെ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(സ്ഥിരം VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    തിരികെ lhs.m_e == rhs;
}
constexpr bool operator==(VWidthMinUsage::en lhs, സ്ഥിരം VWidthMinUsage& rhs) {
    തിരികെ lhs == rhs.m_e;
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
    V3HierGraph* m_hierGraphp = nullptr;
    // Thread Pool, nullptr unless 'verilatedJobs' is known, set via threadPoolp(V3ThreadPool*)
    V3ThreadPool* m_threadPoolp = nullptr;
    // Library Mapping, nullptr unless --libmap is used
    V3LibMap* m_libMapp = nullptr;
    VWidthMinUsage m_widthMinUsage
        = VWidthMinUsage::LINT_WIDTH;  // What AstNode::widthMin() is used for

    std::atomic_int m_debug_File_സംഖ്യ{0};  // Number to append to debug files created
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
    bool m_hasTable = false;  // Desgin has the UDP Table.
    bool m_hasVirtIfaces = false;  // Design uses virtual interfaces
    bool m_usesProbDist = false;  // Uses $dist_*
    bool m_usesStdPackage = false;  // Design uses the std package
    bool m_usesTiming = false;  // Design uses timing constructs
    bool m_usesForce = false;  // Design uses force/release statements
    bool m_usesZeroDelay = false;  // Design uses #0 delay (or non-constant delay)
    bool m_hasForceableSignals = false;  // Need to apply V3Force pass
    bool m_hasAssignDeassign = false;  // Need to apply V3Force pass for assign/deassign statements
    bool m_useParallelBuild = false;  // Use parallel build for model
    bool m_useRandSequence = false;  // Has `randsequence`
    bool m_useCovergroup = false;  // Has covergroup declarations
    bool m_useRandomizeMethods = false;  // Need to define randomize() class methods
    bool m_hasPrintedObjects = false;  // Design has format args printed with to_string()
    uint64_t m_currentHierBlockCost = 0;  // Total cost of this hier block, used for scheduling

    // Memory address to short string mapping (for debug)
    std::unordered_map<സ്ഥിരം ശൂന്യം*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    സ്ഥിരം std::thread::id m_mainThreadId = std::this_thread::get_id();

പൊതു:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = സ്ഥിരസ്ഥിതി;
    ശൂന്യം boot();
    ശൂന്യം shutdown();  // Release allocated resources

    ശൂന്യം vlExit(പൂർണ്ണസംഖ്യ status);

    // ACCESSORS (general)
    AstNetlist* rootp() സ്ഥിരം VL_MT_SAFE { തിരികെ m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() സ്ഥിരം VL_PURE { തിരികെ m_constPoolp; }
    V3LibMap* libMapp() സ്ഥിരം VL_PURE { തിരികെ m_libMapp; }
    V3ThreadPool* threadPoolp() സ്ഥിരം VL_PURE { തിരികെ m_threadPoolp; }
    ശൂന്യം threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() സ്ഥിരം VL_PURE { തിരികെ m_widthMinUsage; }
    bool assertDTypesResolved() സ്ഥിരം { തിരികെ m_assertDTypesResolved; }
    bool assertScoped() സ്ഥിരം { തിരികെ m_assertScoped; }
    bool staticsLifted() സ്ഥിരം { തിരികെ m_staticsLifted; }

    // METHODS
    ശൂന്യം readFiles() VL_MT_DISABLED;
    ശൂന്യം removeStd() VL_MT_DISABLED;
    ശൂന്യം check_വൃക്ഷം() സ്ഥിരം;
    സ്ഥിതം ശൂന്യം dump_Check_Global_വൃക്ഷം(സ്ഥിരം string& stagename, പൂർണ്ണസംഖ്യ new_സംഖ്യ = 0, bool doDump = true,
                                    bool doCheck = true);
    ശൂന്യം assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    ശൂന്യം assertScoped(bool flag) { m_assertScoped = flag; }
    ശൂന്യം staticsLifted(bool flag) { m_staticsLifted = flag; }
    ശൂന്യം widthMinUsage(സ്ഥിരം VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() സ്ഥിരം { തിരികെ m_constRemoveXs; }
    ശൂന്യം constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(സ്ഥിരം string& nameComment, പൂർണ്ണസംഖ്യ new_സംഖ്യ = 0);
    സ്ഥിതം string digitsFilename(പൂർണ്ണസംഖ്യ സംഖ്യ);
    bool needTraceDumper() സ്ഥിരം { തിരികെ m_needTraceDumper; }
    ശൂന്യം needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() സ്ഥിരം VL_MT_SAFE { തിരികെ m_dpi; }
    ശൂന്യം dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() സ്ഥിരം { തിരികെ m_assignsEvents; }
    ശൂന്യം setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() സ്ഥിരം { തിരികെ m_hasEvents; }
    ശൂന്യം setHasEvents() { m_hasEvents = true; }
    bool hasClasses() സ്ഥിരം { തിരികെ m_hasClasses; }
    ശൂന്യം setHasClasses() { m_hasClasses = true; }
    bool hasSampled() സ്ഥിരം { തിരികെ m_hasSampled; }
    ശൂന്യം setHasSampled() { m_hasSampled = true; }
    bool hasTable() സ്ഥിരം { തിരികെ m_hasTable; }
    ശൂന്യം setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() സ്ഥിരം { തിരികെ m_hasVirtIfaces; }
    ശൂന്യം setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() സ്ഥിരം { തിരികെ m_usesProbDist; }
    ശൂന്യം setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() സ്ഥിരം { തിരികെ m_usesStdPackage; }
    ശൂന്യം setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() സ്ഥിരം { തിരികെ m_usesTiming; }
    ശൂന്യം setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() സ്ഥിരം { തിരികെ m_usesZeroDelay; }
    ശൂന്യം setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() സ്ഥിരം { തിരികെ m_hasForceableSignals; }
    ശൂന്യം setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() സ്ഥിരം { തിരികെ m_hasAssignDeassign; }
    ശൂന്യം setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() സ്ഥിരം { തിരികെ m_usesForce; }
    ശൂന്യം setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() സ്ഥിരം { തിരികെ m_hierGraphp; }
    ശൂന്യം hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() സ്ഥിരം { തിരികെ m_useParallelBuild; }
    ശൂന്യം useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() സ്ഥിരം { തിരികെ m_useRandSequence; }
    ശൂന്യം useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() സ്ഥിരം { തിരികെ m_useCovergroup; }
    ശൂന്യം useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() സ്ഥിരം { തിരികെ m_useRandomizeMethods; }
    ശൂന്യം useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() സ്ഥിരം { തിരികെ m_hasPrintedObjects; }
    ശൂന്യം hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    ശൂന്യം saveJsonPtrFieldName(സ്ഥിരം std::string& fieldName);
    ശൂന്യം ptrNamesDumpJson(std::ostream& os);
    ശൂന്യം idPtrMapDumpJson(std::ostream& os);
    സ്ഥിരം std::string& ptrToId(സ്ഥിരം ശൂന്യം* p);
    std::thread::id mainThreadId() സ്ഥിരം { തിരികെ m_mainThreadId; }
    സ്ഥിതം std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() സ്ഥിരം { തിരികെ m_currentHierBlockCost; }
    ശൂന്യം currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

ബാഹ്യ V3Global v3Global;

//######################################################################

#endif  // guard
