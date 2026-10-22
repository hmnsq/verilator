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
#अन्तर्भाव "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Mutex.h"
#अन्तर्भाव "V3Options.h"

#अन्तर्भाव <string>
#अन्तर्भाव <thread>
#अन्तर्भाव <type_traits>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>

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
    ध्रुव VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#define VL_RESTORER_COPY(var) \
    ध्रुव VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#define VL_RESTORER_CLEAR(var) \
    ध्रुव VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#define VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
template <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::मानम्,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ध्रुव T m_saved;  // Value saved, for later restore

सार्वजनिक:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    शून्य* operator new(size_t) = delete;
    शून्य operator delete(शून्य*) = delete;

    ध्रुव T& saved() ध्रुव { प्रत्यागम m_saved; }
};

// Implementation of VL_RESTORER_COPY
template <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::मानम्,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

सार्वजनिक:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    शून्य* operator new(size_t) = delete;
    शून्य operator delete(शून्य*) = delete;

    ध्रुव T& saved() ध्रुव { प्रत्यागम m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
template <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::मानम्,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

सार्वजनिक:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    शून्य* operator new(size_t) = delete;
    शून्य operator delete(शून्य*) = delete;

    ध्रुव T& saved() ध्रुव { प्रत्यागम m_saved; }
};

//######################################################################

class VWidthMinUsage final {
सार्वजनिक:
    गणना en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    गणना en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(ध्रुव VWidthMinUsage& _e) VL_PURE = स्वभाव;
    explicit VWidthMinUsage(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ध्रुव { प्रत्यागम m_e; }
    constexpr VWidthMinUsage& operator=(ध्रुव VWidthMinUsage& _e) VL_PURE = स्वभाव;
};
constexpr bool operator==(ध्रुव VWidthMinUsage& lhs, ध्रुव VWidthMinUsage& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ध्रुव VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool operator==(VWidthMinUsage::en lhs, ध्रुव VWidthMinUsage& rhs) {
    प्रत्यागम lhs == rhs.m_e;
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

    std::atomic_int m_debug_File_सङ्ख्या{0};  // Number to append to debug files created
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
    std::unordered_map<ध्रुव शून्य*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    ध्रुव std::thread::id m_mainThreadId = std::this_thread::get_id();

सार्वजनिक:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = स्वभाव;
    शून्य boot();
    शून्य shutdown();  // Release allocated resources

    शून्य vlExit(पूर्णाङ्क status);

    // ACCESSORS (general)
    AstNetlist* rootp() ध्रुव VL_MT_SAFE { प्रत्यागम m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ध्रुव VL_PURE { प्रत्यागम m_constPoolp; }
    V3LibMap* libMapp() ध्रुव VL_PURE { प्रत्यागम m_libMapp; }
    V3ThreadPool* threadPoolp() ध्रुव VL_PURE { प्रत्यागम m_threadPoolp; }
    शून्य threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() ध्रुव VL_PURE { प्रत्यागम m_widthMinUsage; }
    bool assertDTypesResolved() ध्रुव { प्रत्यागम m_assertDTypesResolved; }
    bool assertScoped() ध्रुव { प्रत्यागम m_assertScoped; }
    bool staticsLifted() ध्रुव { प्रत्यागम m_staticsLifted; }

    // METHODS
    शून्य readFiles() VL_MT_DISABLED;
    शून्य removeStd() VL_MT_DISABLED;
    शून्य check_वृक्षः() ध्रुव;
    स्थिर शून्य dump_Check_Global_वृक्षः(ध्रुव string& stagename, पूर्णाङ्क new_सङ्ख्या = 0, bool doDump = true,
                                    bool doCheck = true);
    शून्य assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    शून्य assertScoped(bool flag) { m_assertScoped = flag; }
    शून्य staticsLifted(bool flag) { m_staticsLifted = flag; }
    शून्य widthMinUsage(ध्रुव VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() ध्रुव { प्रत्यागम m_constRemoveXs; }
    शून्य constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ध्रुव string& nameComment, पूर्णाङ्क new_सङ्ख्या = 0);
    स्थिर string digitsFilename(पूर्णाङ्क सङ्ख्या);
    bool needTraceDumper() ध्रुव { प्रत्यागम m_needTraceDumper; }
    शून्य needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ध्रुव VL_MT_SAFE { प्रत्यागम m_dpi; }
    शून्य dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ध्रुव { प्रत्यागम m_assignsEvents; }
    शून्य setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() ध्रुव { प्रत्यागम m_hasEvents; }
    शून्य setHasEvents() { m_hasEvents = true; }
    bool hasClasses() ध्रुव { प्रत्यागम m_hasClasses; }
    शून्य setHasClasses() { m_hasClasses = true; }
    bool hasSampled() ध्रुव { प्रत्यागम m_hasSampled; }
    शून्य setHasSampled() { m_hasSampled = true; }
    bool hasTable() ध्रुव { प्रत्यागम m_hasTable; }
    शून्य setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() ध्रुव { प्रत्यागम m_hasVirtIfaces; }
    शून्य setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() ध्रुव { प्रत्यागम m_usesProbDist; }
    शून्य setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() ध्रुव { प्रत्यागम m_usesStdPackage; }
    शून्य setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() ध्रुव { प्रत्यागम m_usesTiming; }
    शून्य setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() ध्रुव { प्रत्यागम m_usesZeroDelay; }
    शून्य setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() ध्रुव { प्रत्यागम m_hasForceableSignals; }
    शून्य setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() ध्रुव { प्रत्यागम m_hasAssignDeassign; }
    शून्य setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() ध्रुव { प्रत्यागम m_usesForce; }
    शून्य setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() ध्रुव { प्रत्यागम m_hierGraphp; }
    शून्य hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ध्रुव { प्रत्यागम m_useParallelBuild; }
    शून्य useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ध्रुव { प्रत्यागम m_useRandSequence; }
    शून्य useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ध्रुव { प्रत्यागम m_useCovergroup; }
    शून्य useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ध्रुव { प्रत्यागम m_useRandomizeMethods; }
    शून्य useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ध्रुव { प्रत्यागम m_hasPrintedObjects; }
    शून्य hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    शून्य saveJsonPtrFieldName(ध्रुव std::string& fieldName);
    शून्य ptrNamesDumpJson(std::ostream& os);
    शून्य idPtrMapDumpJson(std::ostream& os);
    ध्रुव std::string& ptrToId(ध्रुव शून्य* p);
    std::thread::id mainThreadId() ध्रुव { प्रत्यागम m_mainThreadId; }
    स्थिर std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() ध्रुव { प्रत्यागम m_currentHierBlockCost; }
    शून्य currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

बाह्य V3Global v3Global;

//######################################################################

#endif  // guard
