// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Verilator Control Files (.vlt) handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2010-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ضمن "V3PchAstMT.h"

#ضمن "V3Control.h"

#ضمن "V3InstrCount.h"
#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <memory>
#ضمن <set>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
صنف V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ثابت std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ثابت std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

عام:
    V3ControlWildcardResolver() = افتراضي;
    ~V3ControlWildcardResolver() = افتراضي;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    فراغ update(ثابت V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        ثابت V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        لكل (ثابت auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(ثابت string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        أرجع m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ثابت string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        ثابت auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        إذا (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ثابت string slashName = V3Os::filenameSlashPath(name);
            لكل (ثابت auto& patEnt : m_mapPatterns) {
                إذا (VString::wildmatch(slashName, patEnt.first)) {
                    إذا (!entryr) entryr.reset(جديد T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            إذا (entryr) entryr->updateFinalize();
        }
        أرجع entryr.get();
    }
};

// List of attributes for variables
صنف V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
عام:
    // Add new attribugte
    فراغ add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    فراغ update(ثابت V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.سك(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    فراغ updateFinalize() {}
    // Apply all attributes to the variable
    فراغ apply(AstVar* varp) ثابت {
        لكل (ثابت VAttrType attr : m_attrs) {
            varp->addAttrsp(جديد AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

صنف WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ثابت std::string, منطقي> m_mapPatterns;  // Pattern match results
    std::deque<string> m_ليوك;  // Source text lines

    // METHODS
    ساكن WildcardContents& s() {  // Singleton
        ساكن WildcardContents s_s;
        أرجع s_s;
    }
    فراغ clearCacheImp() { m_mapPatterns.clear(); }
    فراغ تور_Text_Imp(ثابت string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ثابت string leftover = m_ليوك.back() + text;
        m_ليوك.چرو_back();

        // Insert line-by-line
        string::size_type لي_start = 0;
        طالما (true) {
            ثابت string::size_type لي_end = leftover.find('\n', لي_start);
            إذا (لي_end != string::npos) {
                ثابت string oneline(leftover, لي_start, لي_end - لي_start + 1);
                إذا (oneline.size() > 1) m_ليوك.تور_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_ليوك.size() - 1) << "]: " << oneline);
                لي_start = لي_end + 1;
            } وإلا {
                اكسر;
            }
        }
        // Keep leftover for next time
        m_ليوك.emplace_back(string(leftover, لي_start));  // Might be ""
        clearCacheImp();
    }

    منطقي resolveUncachedImp(ثابت string& contentsRegexp) {
        لكل (ثابت string& i : m_ليوك) {
            إذا (VString::wildmatch(i, contentsRegexp)) أرجع true;
        }
        أرجع خطأ;
    }
    منطقي resolveCachedImp(ثابت string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        إذا (contentsRegexp.empty()) أرجع true;
        ثابت auto pair = m_mapPatterns.emplace(contentsRegexp, خطأ);
        منطقي& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        إذا (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        أرجع entryr;
    }

عام:
    WildcardContents() {
        m_ليوك.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = افتراضي;
    // Return true iff contentsRegexp in parsed contents
    ساكن منطقي resolve(ثابت string& contentsRegexp) {
        أرجع s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ساكن فراغ تور_Text(ثابت string& text) { s().تور_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

صنف V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    منطقي m_noinline = خطأ;  // Don't inline function/task
    منطقي m_public = خطأ;  // Public function/task

عام:
    V3ControlFTask() = افتراضي;
    فراغ update(ثابت V3ControlFTask& f) {
        // Don't overwrite true with false
        إذا (f.m_noinline) m_noinline = true;
        إذا (f.m_public) m_public = true;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    فراغ updateFinalize() {}

    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ setNoInline(منطقي set) { m_noinline = set; }
    فراغ setPublic(منطقي set) { m_public = set; }

    فراغ apply(Ast_كلي_FTask* ftaskp) ثابت {
        إذا (m_noinline)
            ftaskp->addStmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        إذا (m_public)
            ftaskp->addStmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

صنف V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    منطقي m_hasFsmRegisterWrapper = خطأ;  // Whether descriptor has been set
    منطقي m_inline = خطأ;  // Whether to force the inline
    منطقي m_inline_درج = خطأ;  // The inline value (on/off)

عام:
    V3ControlModule() = افتراضي;

    فراغ update(ثابت V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        إذا (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = true;
        }
        لكل (ثابت string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.سك(i);
        إذا (!m_inline) {
            m_inline = m.m_inline;
            m_inline_درج = m.m_inline_درج;
        }
        لكل (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.سك(*it);
        }
    }
    فراغ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { أرجع m_tasks; }
    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ addCoverageBlockOff(ثابت string& name) { m_coverageOffBlocks.سك(name); }
    فراغ setInline(منطقي set) {
        m_inline = true;
        m_inline_درج = set;
    }
    فراغ setFsmRegisterWrapper(File_لي* fl, ثابت V3Control::FsmRegisterWrapper& desc) {
        إذا (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_كلي::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = true;
    }
    ثابت V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() ثابت {
        أرجع m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    فراغ addModulePragma(VPragmaType pragma) { m_modPragmas.سك(pragma); }

    فراغ apply(Ast_كلي_Module* modp) {
        إذا (m_inline) {
            ثابت VPragmaType type
                = m_inline_درج ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_كلي* ثابت nodep = جديد AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        لكل (ثابت auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            إذا (itr == VPragmaType::HIER_PARAMS) modp->hierParams(true);
            Ast_كلي* ثابت nodep = جديد AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    فراغ applyBlock(AstGenBlock* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverageOffBlocks) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(جديد AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    فراغ applyBlock(Ast_كلي_Block* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_لي* ثابت flp = nodep->fileline();
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverageOffBlocks) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
                }
            }
        }
    }
};

using V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
صنف V3_Control_Ignores_لي final {
عام:
    ثابت صحيح m_لي_Min;  // Minimum line number to make change at
    ثابت صحيح m_لي_Max;  // Maximum line number to make change at (inclusive)
    ثابت V3ErrorCode m_code;  // Error code
    ثابت منطقي m_on;  // True to enable message
    V3_Control_Ignores_لي(V3ErrorCode code, صحيح linemin, صحيح linemax, منطقي on)
        : m_لي_Min{linemin}
        , m_لي_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_لي() = افتراضي;
    منطقي every_لي() ثابت { أرجع m_لي_Min == 0 && m_لي_Max == 0; }
    منطقي inRange(صحيح lineno) ثابت {
        أرجع (lineno >= m_لي_Min && lineno <= m_لي_Max) || every_لي();
    }
};
std::ostream& operator<<(std::ostream& os, ثابت V3_Control_Ignores_لي& rhs) {
    أرجع os << rhs.m_لي_Min << "-" << rhs.m_لي_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ساكن std::vector<V3_Control_Ignores_لي> control_Ign_ليوك;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

صنف VInterval_بشيا final {
    بنية Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        صحيح m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(صحيح center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_ليوك;  // All-line disables

خاص:
    std::unique_ptr<Entry> build_بشيا(ثابت IgnIndices& points) {
        إذا (points.empty()) أرجع nullptr;

        صحيح minval = std::numeric_limits<صحيح>::max();
        صحيح maxval = std::numeric_limits<صحيح>::min();
        لكل (ثابت auto& it : points) {
            ثابت V3_Control_Ignores_لي& cign = control_Ign_ليوك[it];
            إذا (cign.every_لي()) {
                m_every_ليوك.emplace_back(it);
                استمر;
            }
            minval = std::min(minval, cign.m_لي_Min);
            maxval = std::max(maxval, cign.m_لي_Max);
        }

        ثابت صحيح center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(جديد Entry{center});

        IgnIndices هغو_Points;
        IgnIndices دام_Points;
        لكل (ثابت auto& it : points) {
            ثابت V3_Control_Ignores_لي& cign = control_Ign_ليوك[it];
            إذا (cign.every_لي()) استمر;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            إذا (cign.m_لي_Max < center)
                هغو_Points.emplace_back(it);
            وإلا إذا (cign.m_لي_Min > center)
                دام_Points.emplace_back(it);
            وإلا {
                entp->m_byMin.تور_back(it);
                entp->m_byMax.تور_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_ليوك[a].m_لي_Min < control_Ign_ليوك[b].m_لي_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_ليوك[a].m_لي_Max > control_Ign_ليوك[b].m_لي_Max;
        });

        entp->m_leftp = build_بشيا(هغو_Points);
        entp->m_rightp = build_بشيا(دام_Points);
        أرجع entp;
    }

    فراغ find_بشيا(صحيح lineno, Entry* entp, IgnIndices& resultsr, صحيح& nextChanger) ثابت {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        إذا (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            لكل (ثابت auto& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_لي& cign = control_Ign_ليوك[it];
                إذا (cign.m_لي_Min <= lineno) {
                    resultsr.emplace_back(it);
                } وإلا {
                    nextChanger = std::min(nextChanger, cign.m_لي_Min);
                    اكسر;
                }
            }
            إذا (entp->m_leftp) find_بشيا(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } وإلا إذا (lineno > entp->m_center) {
            لكل (ثابت auto& it : entp->m_byMax) {
                ثابت V3_Control_Ignores_لي& cign = control_Ign_ليوك[it];
                إذا (cign.m_لي_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_لي_Max + 1);
                    resultsr.emplace_back(it);
                } وإلا {
                    اكسر;
                }
            }
            إذا (entp->m_rightp) find_بشيا(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } وإلا {
            لكل (ثابت auto& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_لي& cign = control_Ign_ليوك[it];
                resultsr.emplace_back(it);
                إذا (cign.m_لي_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_لي_Max + 1);
            }
            إذا (entp->m_leftp) find_بشيا(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            إذا (entp->m_rightp) find_بشيا(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

عام:
    // CONSTRUCTORS
    VInterval_بشيا() = افتراضي;
    ~VInterval_بشيا() = افتراضي;
    // METHODS
    فراغ clear() { m_rootp = nullptr; }
    فراغ build(ثابت IgnIndices& points) {
        clear();
        m_rootp = build_بشيا(points);
    }
    فراغ find(صحيح lineno, IgnIndices& resultsr, صحيح& nextChanger) ثابت {
        resultsr = m_every_ليوك;
        nextChanger = std::numeric_limits<صحيح>::max();

        إذا (m_rootp) find_بشيا(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    ساكن فراغ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<صحيح, صحيح>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        لكل (ثابت auto& it : points) {
            control_Ign_ليوك.emplace_back(
                V3_Control_Ignores_لي{V3ErrorCode::I_LINT, it.first, it.second, true});
            data.emplace_back(static_cast<uint32_t>(control_Ign_ليوك.size() - 1));
        }
        VInterval_بشيا بشيا;
        بشيا.build(data);

        IgnIndices results;
        صحيح nextChange = 0;
        بشيا.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        بشيا.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        بشيا.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        بشيا.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        بشيا.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        بشيا.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        بشيا.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        بشيا.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<صحيح>::max());
        //
        points = {{0, 0}};
        لكل (ثابت auto& it : points) {
            control_Ign_ليوك.emplace_back(
                V3_Control_Ignores_لي{V3ErrorCode::I_LINT, it.first, it.second, true});
            data.emplace_back(static_cast<uint32_t>(control_Ign_ليوك.size() - 1));
        }
        بشيا.build(data);
        //
        بشيا.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_لي_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

صنف WaiverSetting final {
عام:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, ثابت string& contents, ثابت string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = افتراضي;
    WaiverSetting& operator=(ثابت WaiverSetting& rhs) = افتراضي;
};

// File entity
صنف V3ControlFile final {
    using لي_Attr_Map = std::map<صحيح, V3_Control_لي_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    لي_Attr_Map m_لي_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_بشيا m_interval_بشيا;  // Tree of indices to IgnLines

    بنية {
        صحيح filenameno = -1;  // Last filename
        صحيح lineno = -1;  // Last linenumber
        صحيح nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    منطقي لي_Match(صحيح lineno, VPragmaType type) {
        إذا (m_لي_Attrs.find(0) != m_لي_Attrs.end() && m_لي_Attrs[0][type]) أرجع true;
        إذا (m_لي_Attrs.find(lineno) == m_لي_Attrs.end()) أرجع خطأ;
        أرجع m_لي_Attrs[lineno][type];
    }

عام:
    V3ControlFile() = افتراضي;
    فراغ update(ثابت V3ControlFile& file) {
        // Copy in all attributes and waivers
        لكل (ثابت auto& itr : file.m_لي_Attrs) m_لي_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.سك(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.سك(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    فراغ updateFinalize() { m_interval_بشيا.build(m_ignIndices); }
    فراغ add_لي_Attribute(صحيح lineno, VPragmaType attr) { m_لي_Attrs[lineno].set(attr); }
    فراغ addIgnore(V3ErrorCode code, صحيح min, صحيح max, منطقي on) {
        control_Ign_ليوك.emplace_back(V3_Control_Ignores_لي{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_ليوك.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    فراغ addIgnoreMatch(V3ErrorCode code, ثابت string& contents, ثابت string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        إذا (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    فراغ applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        إذا (لي_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(جديد AstPragma{nodep->fileline(), pragma});
        }
    }
    فراغ applyBlock(Ast_كلي_Block* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_لي* ثابت flp = nodep->fileline();
        إذا (لي_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
        }
    }
    فراغ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ثابت صحيح lineno = nodep->fileline()->lineno();
        إذا (لي_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(true);
        إذا (لي_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(true);
    }
    فراغ applyIgnores(File_لي* filelinep) {
        // HOT routine, called each parsed token line of this filename
        إذا (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            أرجع;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        إذا (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            أرجع;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<صحيح>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_بشيا.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        لكل (ثابت auto& it : results) {
            ثابت V3_Control_Ignores_لي& cign = control_Ign_ليوك[it];
            UASSERT(cign.inRange(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_code.forDelegateCodes(
                [&](V3ErrorCode subcode) { bitset.set(subcode, cign.m_on); });
        }

        filelinep->warnSetCtrlBitSet(bitset);
        m_lastIgnore.nextChange = std::max(m_lastIgnore.nextChange, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_lastIgnore.nextChange
                                       << " Messages: " << bitset.ascii());
    }
    منطقي waive(V3ErrorCode code, ثابت string& message) {
        إذا (code.hardError()) أرجع خطأ;
        لكل (ثابت auto& itr : m_waivers) {
            إذا ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                أرجع true;
            }
        }
        أرجع خطأ;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

صنف V3ControlScopeTraceEntry final {
عام:
    ثابت string m_scope;  // Scope or regexp to match
    ثابت منطقي m_on;  // True to enable message
    صحيح m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(ثابت string& scope, منطقي on, صحيح levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = افتراضي;
    منطقي operator<(ثابت V3ControlScopeTraceEntry& other) ثابت {
        إذا (m_on < other.m_on) أرجع true;
        إذا (m_on > other.m_on) أرجع خطأ;
        إذا (m_levels < other.m_levels) أرجع true;
        إذا (m_levels > other.m_levels) أرجع خطأ;
        أرجع m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
صنف V3ControlScopeTraceEntryMatch final {
عام:
    ثابت V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    ثابت string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(ثابت V3ControlScopeTraceEntry* entryp, ثابت string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    منطقي operator<(ثابت V3ControlScopeTraceEntryMatch& other) ثابت {
        إذا (m_entryp < other.m_entryp) أرجع true;
        إذا (m_entryp > other.m_entryp) أرجع خطأ;
        أرجع m_scopepart < other.m_scopepart;
    }
};

صنف V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, منطقي> m_matchCache;  // Matching entries for speed

عام:
    فراغ addScopeTraceOn(منطقي on, ثابت string& scope, صحيح levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    منطقي getEntryMatch(ثابت V3ControlScopeTraceEntry* entp, ثابت string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ثابت V3ControlScopeTraceEntryMatch مبودي{entp, scopepart};
        ثابت auto pair = m_matchCache.emplace(مبودي, خطأ);
        إذا (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        أرجع pair.first->second;
    }

    منطقي getScopeTraceOn(ثابت string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        صحيح maxLevel = 1;
        لكل (ثابت auto& ch : scope) {
            إذا (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        منطقي enabled = true;
        لكل (ثابت auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            صحيح partLevel = 1;
            لكل (string::size_type partEnd = 0; true;) {
                partEnd = scope.find('.', partEnd + 1);
                إذا (partEnd == string::npos) partEnd = scope.length();
                ثابت std::string scopepart = scope.substr(0, partEnd);
                إذا (getEntryMatch(&ent, scopepart)) {
                    ثابت منطقي levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    إذا (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    اكسر;
                }
                إذا (partEnd == scope.length()) اكسر;
                ++partLevel;
            }
        }
        أرجع enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

صنف V3ControlResolverHierWorkerEntry final {
    ثابت صحيح m_workers;  // Number of workers requested for the hierarchical block
    File_لي* ثابت m_flp;  // Location the worker count was specified at

عام:
    explicit V3ControlResolverHierWorkerEntry(صحيح workers, File_لي* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    صحيح workers() ثابت { أرجع m_workers; }
    File_لي* flp() ثابت { أرجع m_flp; }
};

صنف V3ControlResolver final {
    تعداد ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_لي* m_profile_File_لي = nullptr;  // Location profile_data was read from

    V3ControlResolver() = افتراضي;
    ~V3ControlResolver() = افتراضي;

عام:
    ساكن V3ControlResolver& s() {
        ساكن V3ControlResolver s_singleton;
        أرجع s_singleton;
    }
    V3ControlModuleResolver& modules() { أرجع m_modules; }
    V3ControlFileResolver& files() { أرجع m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { أرجع m_scopeTraces; }

    فراغ addProfileData(File_لي* fl, ثابت string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    فراغ addProfileData(File_لي* fl, ثابت string& model, ثابت string& مبودي, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        إذا (!m_profile_File_لي) m_profile_File_لي = fl;
        إذا (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        إذا (mode == MTASK) {
            m_profileData[model][مبودي] += cost;
        } وإلا إذا (mode == HIER_DPI) {
            m_profileData[model][مبودي] = std::max(m_profileData[model][مبودي], cost);
        }
        m_mode |= mode;
    }
    منطقي containsMTaskProfileData() ثابت { أرجع m_mode & MTASK; }
    uint64_t getProfileData(ثابت string& hierDpi) ثابت {
        // Empty key for hierarchical DPI wrapper costs.
        أرجع getProfileData(hierDpi, "");
    }
    فراغ addHierWorkers(File_لي* flp, ثابت string& model, صحيح workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    صحيح getHierWorkers(ثابت string& model) ثابت {
        ثابت auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        أرجع mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_لي* get_Hier_Workers_File_لي(ثابت string& model) ثابت {
        ثابت auto mit = m_hierWorkers.find(model);
        أرجع mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(ثابت string& model, ثابت string& مبودي) ثابت {
        ثابت auto mit = m_profileData.find(model);
        إذا (mit == m_profileData.cend()) أرجع 0;
        ثابت auto it = mit->second.find(مبودي);
        إذا (it == mit->second.cend()) أرجع 0;
        أرجع it->second;
    }
    File_لي* get_Profile_Data_File_لي() ثابت { أرجع m_profile_File_لي; }  // Maybe null
    ساكن uint64_t getCurrentHierBlockCost() {
        إذا (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            أرجع cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ثابت netlistp = v3Global.rootp();
        uint64_t cost = 0;
        لكل (صحيح i = 0; i < VEval::_ENUM_END; ++i) {
            ثابت VEval eval{i};
            إذا (eval.slow()) استمر;
            cost += V3_Instr_كدي::كدي(netlistp->evalFuncp(eval), خطأ);
        }
        UINFO(9, "Evaluating cost: " << cost);
        أرجع cost;
    }
};

//######################################################################
// V3Control

فراغ V3Control::addCaseFull(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_لي_Attribute(lineno, VPragmaType::FULL_CASE);
}
فراغ V3Control::addCaseParallel(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_لي_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
فراغ V3Control::addCoverageBlockOff(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_لي_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
فراغ V3Control::addCoverageBlockOff(ثابت string& module, ثابت string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

فراغ V3Control::addHierWorkers(File_لي* fl, ثابت string& model, صحيح workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

فراغ V3Control::addFsmRegisterWrapper(File_لي* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_درج) {
    string missing;
    إذا (module.empty()) missing += "-module";
    إذا (d.empty()) missing = VString::dot(missing, ", ", "-d");
    إذا (q.empty()) missing = VString::dot(missing, ", ", "-q");
    إذا (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    إذا (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        أرجع;
    }

    ثابت FsmRegisterWrapper desc{module, d, q, clock, reset, reset_درج};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

فراغ V3Control::addIgnore(V3ErrorCode code, منطقي on, ثابت string& filename, صحيح min, صحيح max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    إذا (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_لي::globalWarnOff(code, !on);
    } وإلا {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
فراغ V3Control::addIgnoreMatch(V3ErrorCode code, ثابت string& filename, ثابت string& contents,
                               ثابت string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

فراغ V3Control::addInline(File_لي* fl, ثابت string& module, ثابت string& ftask, منطقي on) {
    إذا (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } وإلا {
        إذا (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } وإلا {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

فراغ V3Control::addModulePragma(ثابت string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

فراغ V3Control::addProfileData(File_لي* fl, ثابت string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
فراغ V3Control::addProfileData(File_لي* fl, ثابت string& model, ثابت string& مبودي,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, مبودي, cost);
}

فراغ V3Control::addScopeTraceOn(منطقي on, ثابت string& scope, صحيح levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

فراغ V3Control::addVarAttr(File_لي* fl, ثابت string& module, ثابت string& ftask,
                           VarSpecKind kind, ثابت string& pattern, VAttrType attr,
                           Ast_Sen_بشيا* sensep) {
    إذا (sensep) {
        File_لي* ثابت flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_بشيا(), sensep);
        // Used to be only accepted on public_flat_rw
        إذا (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            أرجع;
        }
    }

    إذا (kind != VarSpecKind::VAR) {
        بدل (attr) {
        حالة VAttrType::VAR_PUBLIC_FLAT:
        حالة VAttrType::VAR_PUBLIC_FLAT_RD:
        حالة VAttrType::VAR_PUBLIC_FLAT_RW: اكسر;
        افتراضي:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            أرجع;
        }
    }

    // Semantics: Most of the attributes operate on signals
    إذا (pattern.empty()) {
        إذا (attr == VAttrType::VAR_PUBLIC) {
            إذا (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } وإلا {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(true);
            }
        } وإلا {
            fl->v3error("missing -var");
        }
    } وإلا {
        إذا (attr == VAttrType::VAR_FORCEABLE) {
            إذا (module.empty()) {
                fl->v3error("forceable missing -module");
            } وإلا إذا (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } وإلا {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } وإلا {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                إذا (ftask.empty()) {
                    إذا (kind == VarSpecKind::PARAM) أرجع mod.params().at(pattern);
                    إذا (kind == VarSpecKind::PORT) أرجع mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    أرجع mod.vars().at(pattern);
                }
                إذا (kind == VarSpecKind::PARAM) أرجع mod.ftasks().at(ftask).params().at(pattern);
                إذا (kind == VarSpecKind::PORT) أرجع mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                أرجع mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

فراغ V3Control::applyCase(AstCase* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyCase(nodep);
}

فراغ V3Control::applyCoverageBlock(Ast_كلي_Module* modulep, AstBegin* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyBlock(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->applyBlock(nodep);
}
فراغ V3Control::applyCoverageBlock(Ast_كلي_Module* modulep, AstGenBlock* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyBlock(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->applyBlock(nodep);
}

فراغ V3Control::applyIgnores(File_لي* filelinep) {
    ثابت string& filename = filelinep->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyIgnores(filelinep);
}

فراغ V3Control::applyModule(Ast_كلي_Module* modulep) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply(modulep);
}

فراغ V3Control::applyFTask(Ast_كلي_Module* modulep, Ast_كلي_FTask* ftaskp) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (!modp) أرجع;
    ثابت V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    إذا (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
ساكن فراغ resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ثابت std::string name = varp->prettyDehashOrigOrName();
    إذا (ثابت V3ControlVar* ثابت vp = resolver.vars().resolve(name)) vp->apply(varp);
    إذا (varp->isParam()) {
        إذا (ثابت V3ControlVar* ثابت vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    إذا (varp->isIO()) {
        إذا (ثابت V3ControlVar* ثابت vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

فراغ V3Control::applyVarAttr(ثابت Ast_كلي_Module* modulep, ثابت Ast_كلي_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ثابت modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    إذا (!modp) أرجع;
    إذا (ftaskp) {
        V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        إذا (!ftp) أرجع;
        resolveThenApply(*ftp, varp);
    } وإلا {
        resolveThenApply(*modp, varp);
    }
}

صحيح V3Control::getHierWorkers(ثابت string& model) {
    أرجع V3ControlResolver::s().getHierWorkers(model);
}
File_لي* V3Control::get_Hier_Workers_File_لي(ثابت string& model) {
    أرجع V3ControlResolver::s().get_Hier_Workers_File_لي(model);
}
ثابت V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(ثابت string& module) {
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(module);
    أرجع modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(ثابت string& hierDpi) {
    أرجع V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(ثابت string& model, ثابت string& مبودي) {
    أرجع V3ControlResolver::s().getProfileData(model, مبودي);
}
File_لي* V3Control::get_Profile_Data_File_لي() {
    أرجع V3ControlResolver::s().get_Profile_Data_File_لي();
}
منطقي V3Control::getScopeTraceOn(ثابت string& scope) {
    أرجع V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

فراغ V3Control::contents_تور_Text(ثابت string& text) { WildcardContents::تور_Text(text); }

منطقي V3Control::containsMTaskProfileData() {
    أرجع V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    أرجع V3ControlResolver::s().getCurrentHierBlockCost();
}

منطقي V3Control::waive(ثابت File_لي* filelinep, V3ErrorCode code, ثابت string& message) {
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    إذا (!filep) أرجع خطأ;
    أرجع filep->waive(code, message);
}

فراغ V3Control::selfTest() { VInterval_بشيا::selfTest(); }
