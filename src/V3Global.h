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

#यदि_अपरिभाषितम् VERILATOR_V3GLOBAL_H_
#परिभाषय VERILATOR_V3GLOBAL_H_

// clang-format off
#अन्तर्भाव "config_build.h"
#यदि_अपरिभाषितम् HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#यदि_अन्तः
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
#परिभाषय VL_RESTORER(var) \
    ध्रुव VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#परिभाषय VL_RESTORER_COPY(var) \
    ध्रुव VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#परिभाषय VL_RESTORER_CLEAR(var) \
    ध्रुव VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#परिभाषय VL_RESTORER_पूर्व(var) restorer_##var.saved()

// Implementation of VL_RESTORER
प्रतिरूपम् <typename T>
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
    शून्य* संक्रियकः new(आकार_t) = delete;
    शून्य संक्रियकः delete(शून्य*) = delete;

    ध्रुव T& saved() ध्रुव { प्रत्यागम m_saved; }
};

// Implementation of VL_RESTORER_COPY
प्रतिरूपम् <typename T>
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
    शून्य* संक्रियकः new(आकार_t) = delete;
    शून्य संक्रियकः delete(शून्य*) = delete;

    ध्रुव T& saved() ध्रुव { प्रत्यागम m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
प्रतिरूपम् <typename T>
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
    शून्य* संक्रियकः new(आकार_t) = delete;
    शून्य संक्रियकः delete(शून्य*) = delete;

    ध्रुव T& saved() ध्रुव { प्रत्यागम m_saved; }
};

//######################################################################

class VWidthMinUsage final {
सार्वजनिक:
    गणना en : uint8_t { LINT_विस्तारः, MATCHES_विस्तारः, VERILOG_विस्तारः };
    गणना en m_e;
    VWidthMinUsage()
        : m_e{LINT_विस्तारः} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_विशुद्धम् : m_e{_e} {}
    constexpr VWidthMinUsage(ध्रुव VWidthMinUsage& _e) VL_विशुद्धम् = स्वभाव;
    explicit VWidthMinUsage(पूर्णाङ्क _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr संक्रियकः en() ध्रुव { प्रत्यागम m_e; }
    constexpr VWidthMinUsage& संक्रियकः=(ध्रुव VWidthMinUsage& _e) VL_विशुद्धम् = स्वभाव;
};
constexpr bool संक्रियकः==(ध्रुव VWidthMinUsage& lhs, ध्रुव VWidthMinUsage& rhs) {
    प्रत्यागम lhs.m_e == rhs.m_e;
}
constexpr bool संक्रियकः==(ध्रुव VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_विशुद्धम् {
    प्रत्यागम lhs.m_e == rhs;
}
constexpr bool संक्रियकः==(VWidthMinUsage::en lhs, ध्रुव VWidthMinUsage& rhs) {
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
    VWidthMinUsage m_विस्तारः_Min_Usage
        = VWidthMinUsage::LINT_विस्तारः;  // What AstNode::widthMin() is used for

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
    bool m_has_सारणी = false;  // Desgin has the UDP Table.
    bool m_hasVirtIfaces = false;  // Design uses virtual interfaces
    bool m_उपयुङ्क्ते_Prob_Dist = false;  // Uses $dist_*
    bool m_उपयुङ्क्ते_Std_Package = false;  // Design uses the std package
    bool m_उपयुङ्क्ते_Timing = false;  // Design uses timing constructs
    bool m_उपयुङ्क्ते_Force = false;  // Design uses force/release statements
    bool m_उपयुङ्क्ते_Zero_Delay = false;  // Design uses #0 delay (or non-constant delay)
    bool m_has_Forceable_सङ्केताः = false;  // Need to apply V3Force pass
    bool m_has_नियोजय_Deassign = false;  // Need to apply V3Force pass for assign/deassign statements
    bool m_useParallelBuild = false;  // Use parallel build for model
    bool m_useRandSequence = false;  // Has `randsequence`
    bool m_useCovergroup = false;  // Has covergroup declarations
    bool m_useRandomizeMethods = false;  // Need to define randomize() class methods
    bool m_hasPrintedObjects = false;  // Design has format args printed with to_string()
    uint64_t m_current_Hier_खण्डम्_Cost = 0;  // Total cost of this hier block, used for scheduling

    // Memory address to short string mapping (for debug)
    std::unordered_map<ध्रुव शून्य*, std::string>
        m_ptr_गन्तव्यम्_Id;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_समुच्चयः<std::string> m_json_Ptr_नामानि;

    // Id of the main thread
    ध्रुव std::thread::id m_mainThreadId = std::this_thread::get_id();

सार्वजनिक:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = स्वभाव;
    शून्य boot();
    शून्य shutdown();  // Release allocated resources

    शून्य vlExit(पूर्णाङ्क स्थितिः);

    // ACCESSORS (general)
    AstNetlist* rootp() ध्रुव VL_MT_SAFE { प्रत्यागम m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ध्रुव VL_विशुद्धम् { प्रत्यागम m_constPoolp; }
    V3LibMap* libMapp() ध्रुव VL_विशुद्धम् { प्रत्यागम m_libMapp; }
    V3ThreadPool* threadPoolp() ध्रुव VL_विशुद्धम् { प्रत्यागम m_threadPoolp; }
    शून्य threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage विस्तारः_Min_Usage() ध्रुव VL_विशुद्धम् { प्रत्यागम m_विस्तारः_Min_Usage; }
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
    शून्य विस्तारः_Min_Usage(ध्रुव VWidthMinUsage& flag) { m_विस्तारः_Min_Usage = flag; }
    bool constRemoveXs() ध्रुव { प्रत्यागम m_constRemoveXs; }
    शून्य constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ध्रुव string& nameComment, पूर्णाङ्क new_सङ्ख्या = 0);
    स्थिर string digitsFilename(पूर्णाङ्क सङ्ख्या);
    bool needTraceDumper() ध्रुव { प्रत्यागम m_needTraceDumper; }
    शून्य needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ध्रुव VL_MT_SAFE { प्रत्यागम m_dpi; }
    शून्य dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ध्रुव { प्रत्यागम m_assignsEvents; }
    शून्य समुच्चयः_Assigns_Events() { m_assignsEvents = true; }
    bool hasEvents() ध्रुव { प्रत्यागम m_hasEvents; }
    शून्य समुच्चयः_Has_Events() { m_hasEvents = true; }
    bool hasClasses() ध्रुव { प्रत्यागम m_hasClasses; }
    शून्य समुच्चयः_Has_Classes() { m_hasClasses = true; }
    bool hasSampled() ध्रुव { प्रत्यागम m_hasSampled; }
    शून्य समुच्चयः_Has_Sampled() { m_hasSampled = true; }
    bool has_सारणी() ध्रुव { प्रत्यागम m_has_सारणी; }
    शून्य समुच्चयः_Has_सारणी() { m_has_सारणी = true; }
    bool hasVirtIfaces() ध्रुव { प्रत्यागम m_hasVirtIfaces; }
    शून्य समुच्चयः_Has_Virt_Ifaces() { m_hasVirtIfaces = true; }
    bool उपयुङ्क्ते_Prob_Dist() ध्रुव { प्रत्यागम m_उपयुङ्क्ते_Prob_Dist; }
    शून्य समुच्चयः_उपयुङ्क्ते_Prob_Dist() { m_उपयुङ्क्ते_Prob_Dist = true; }
    bool उपयुङ्क्ते_Std_Package() ध्रुव { प्रत्यागम m_उपयुङ्क्ते_Std_Package; }
    शून्य समुच्चयः_उपयुङ्क्ते_Std_Package() { m_उपयुङ्क्ते_Std_Package = true; }
    bool उपयुङ्क्ते_Timing() ध्रुव { प्रत्यागम m_उपयुङ्क्ते_Timing; }
    शून्य समुच्चयः_उपयुङ्क्ते_Timing() { m_उपयुङ्क्ते_Timing = true; }
    bool उपयुङ्क्ते_Zero_Delay() ध्रुव { प्रत्यागम m_उपयुङ्क्ते_Zero_Delay; }
    शून्य समुच्चयः_उपयुङ्क्ते_Zero_Delay() { m_उपयुङ्क्ते_Zero_Delay = true; }
    bool has_Forceable_सङ्केताः() ध्रुव { प्रत्यागम m_has_Forceable_सङ्केताः; }
    शून्य समुच्चयः_Has_Forceable_सङ्केताः() { m_has_Forceable_सङ्केताः = true; }
    bool has_नियोजय_Deassign() ध्रुव { प्रत्यागम m_has_नियोजय_Deassign; }
    शून्य समुच्चयः_Has_नियोजय_Deassign() { m_has_नियोजय_Deassign = true; }
    bool उपयुङ्क्ते_Force() ध्रुव { प्रत्यागम m_उपयुङ्क्ते_Force; }
    शून्य समुच्चयः_उपयुङ्क्ते_Force() { m_उपयुङ्क्ते_Force = true; }
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
    शून्य save_Json_Ptr_क्षेत्रम्_Name(ध्रुव std::string& क्षेत्रम्_Name);
    शून्य ptr_नामानि_Dump_Json(std::ostream& os);
    शून्य idPtrMapDumpJson(std::ostream& os);
    ध्रुव std::string& ptr_गन्तव्यम्_Id(ध्रुव शून्य* p);
    std::thread::id mainThreadId() ध्रुव { प्रत्यागम m_mainThreadId; }
    स्थिर std::सदिशम्<std::string> verilatedCppFiles();
    uint64_t current_Hier_खण्डम्_Cost() ध्रुव { प्रत्यागम m_current_Hier_खण्डम्_Cost; }
    शून्य current_Hier_खण्डम्_Cost(uint64_t cost) { m_current_Hier_खण्डम्_Cost = cost; }
};

बाह्य V3Global v3Global;

//######################################################################

#यदि_अन्तः  // guard
