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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3Control.h"

#அந்தர்பாவ "V3InstrCount.h"
#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3String.h"

#அந்தர்பாவ <memory>
#அந்தர்பாவ <set>
#அந்தர்பாவ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
வகுப்பு V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<மாறிலி std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<மாறிலி std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

பொது:
    V3ControlWildcardResolver() = இயல்பு;
    ~V3ControlWildcardResolver() = இயல்பு;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    வெறுமை update(மாறிலி V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        மாறிலி V3LockGuard lock{m_mutex};
        மாறிலி V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        க்கு (மாறிலி auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(மாறிலி string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        மாறிலி V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        திரும்பு m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(மாறிலி string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        மாறிலி V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        மாறிலி auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        எனில் (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            மாறிலி string slashName = V3Os::filenameSlashPath(name);
            க்கு (மாறிலி auto& patEnt : m_mapPatterns) {
                எனில் (VString::wildmatch(slashName, patEnt.first)) {
                    எனில் (!entryr) entryr.reset(புதிய T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            எனில் (entryr) entryr->updateFinalize();
        }
        திரும்பு entryr.get();
    }
};

// List of attributes for variables
வகுப்பு V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
பொது:
    // Add new attribugte
    வெறுமை add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    வெறுமை update(மாறிலி V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.செருகு(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    வெறுமை updateFinalize() {}
    // Apply all attributes to the variable
    வெறுமை apply(AstVar* varp) மாறிலி {
        க்கு (மாறிலி VAttrType attr : m_attrs) {
            varp->addAttrsp(புதிய AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

வகுப்பு WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<மாறிலி std::string, தருக்க> m_mapPatterns;  // Pattern match results
    std::deque<string> m_வரிகள்;  // Source text lines

    // METHODS
    நிலையான WildcardContents& s() {  // Singleton
        நிலையான WildcardContents s_s;
        திரும்பு s_s;
    }
    வெறுமை clearCacheImp() { m_mapPatterns.clear(); }
    வெறுமை தள்ளு_Text_Imp(மாறிலி string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        மாறிலி string leftover = m_வரிகள்.back() + text;
        m_வரிகள்.எடு_back();

        // Insert line-by-line
        string::size_type வரி_start = 0;
        போது (உண்மை) {
            மாறிலி string::size_type வரி_end = leftover.find('\n', வரி_start);
            எனில் (வரி_end != string::npos) {
                மாறிலி string oneline(leftover, வரி_start, வரி_end - வரி_start + 1);
                எனில் (oneline.size() > 1) m_வரிகள்.தள்ளு_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_வரிகள்.size() - 1) << "]: " << oneline);
                வரி_start = வரி_end + 1;
            } இல்லையெனில் {
                நிறுத்து;
            }
        }
        // Keep leftover for next time
        m_வரிகள்.emplace_back(string(leftover, வரி_start));  // Might be ""
        clearCacheImp();
    }

    தருக்க resolveUncachedImp(மாறிலி string& contentsRegexp) {
        க்கு (மாறிலி string& i : m_வரிகள்) {
            எனில் (VString::wildmatch(i, contentsRegexp)) திரும்பு உண்மை;
        }
        திரும்பு பொய்;
    }
    தருக்க resolveCachedImp(மாறிலி string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        எனில் (contentsRegexp.empty()) திரும்பு உண்மை;
        மாறிலி auto pair = m_mapPatterns.emplace(contentsRegexp, பொய்);
        தருக்க& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        எனில் (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        திரும்பு entryr;
    }

பொது:
    WildcardContents() {
        m_வரிகள்.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = இயல்பு;
    // Return true iff contentsRegexp in parsed contents
    நிலையான தருக்க resolve(மாறிலி string& contentsRegexp) {
        திரும்பு s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    நிலையான வெறுமை தள்ளு_Text(மாறிலி string& text) { s().தள்ளு_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

வகுப்பு V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    தருக்க m_noinline = பொய்;  // Don't inline function/task
    தருக்க m_public = பொய்;  // Public function/task

பொது:
    V3ControlFTask() = இயல்பு;
    வெறுமை update(மாறிலி V3ControlFTask& f) {
        // Don't overwrite true with false
        எனில் (f.m_noinline) m_noinline = உண்மை;
        எனில் (f.m_public) m_public = உண்மை;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    வெறுமை updateFinalize() {}

    V3ControlVarResolver& params() { திரும்பு m_params; }
    V3ControlVarResolver& ports() { திரும்பு m_ports; }
    V3ControlVarResolver& vars() { திரும்பு m_vars; }

    வெறுமை setNoInline(தருக்க set) { m_noinline = set; }
    வெறுமை setPublic(தருக்க set) { m_public = set; }

    வெறுமை apply(Ast_முனை_FTask* ftaskp) மாறிலி {
        எனில் (m_noinline)
            ftaskp->addStmtsp(புதிய AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        எனில் (m_public)
            ftaskp->addStmtsp(புதிய AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

வகுப்பு V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    தருக்க m_hasFsmRegisterWrapper = பொய்;  // Whether descriptor has been set
    தருக்க m_inline = பொய்;  // Whether to force the inline
    தருக்க m_inline_மதிப்பு = பொய்;  // The inline value (on/off)

பொது:
    V3ControlModule() = இயல்பு;

    வெறுமை update(மாறிலி V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        எனில் (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = உண்மை;
        }
        க்கு (மாறிலி string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.செருகு(i);
        எனில் (!m_inline) {
            m_inline = m.m_inline;
            m_inline_மதிப்பு = m.m_inline_மதிப்பு;
        }
        க்கு (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.செருகு(*it);
        }
    }
    வெறுமை updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { திரும்பு m_tasks; }
    V3ControlVarResolver& params() { திரும்பு m_params; }
    V3ControlVarResolver& ports() { திரும்பு m_ports; }
    V3ControlVarResolver& vars() { திரும்பு m_vars; }

    வெறுமை addCoverageBlockOff(மாறிலி string& name) { m_coverageOffBlocks.செருகு(name); }
    வெறுமை setInline(தருக்க set) {
        m_inline = உண்மை;
        m_inline_மதிப்பு = set;
    }
    வெறுமை setFsmRegisterWrapper(File_வரி* fl, மாறிலி V3Control::FsmRegisterWrapper& desc) {
        எனில் (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_முனை::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = உண்மை;
    }
    மாறிலி V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() மாறிலி {
        திரும்பு m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    வெறுமை addModulePragma(VPragmaType pragma) { m_modPragmas.செருகு(pragma); }

    வெறுமை apply(Ast_முனை_Module* modp) {
        எனில் (m_inline) {
            மாறிலி VPragmaType type
                = m_inline_மதிப்பு ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_முனை* மாறிலி nodep = புதிய AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        க்கு (மாறிலி auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            எனில் (itr == VPragmaType::HIER_PARAMS) modp->hierParams(உண்மை);
            Ast_முனை* மாறிலி nodep = புதிய AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    வெறுமை applyBlock(AstGenBlock* nodep) {
        மாறிலி VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        எனில் (!nodep->unnamed()) {
            க்கு (மாறிலி string& i : m_coverageOffBlocks) {
                எனில் (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(புதிய AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    வெறுமை applyBlock(Ast_முனை_Block* nodep) {
        மாறிலி VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_வரி* மாறிலி flp = nodep->fileline();
        எனில் (!nodep->unnamed()) {
            க்கு (மாறிலி string& i : m_coverageOffBlocks) {
                எனில் (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(புதிய AstStmtPragma{flp, புதிய AstPragma{flp, pragma}});
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
வகுப்பு V3_Control_Ignores_வரி final {
பொது:
    மாறிலி முழு m_வரி_Min;  // Minimum line number to make change at
    மாறிலி முழு m_வரி_Max;  // Maximum line number to make change at (inclusive)
    மாறிலி V3ErrorCode m_code;  // Error code
    மாறிலி தருக்க m_on;  // True to enable message
    V3_Control_Ignores_வரி(V3ErrorCode code, முழு linemin, முழு linemax, தருக்க on)
        : m_வரி_Min{linemin}
        , m_வரி_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_வரி() = இயல்பு;
    தருக்க every_வரி() மாறிலி { திரும்பு m_வரி_Min == 0 && m_வரி_Max == 0; }
    தருக்க inRange(முழு lineno) மாறிலி {
        திரும்பு (lineno >= m_வரி_Min && lineno <= m_வரி_Max) || every_வரி();
    }
};
std::ostream& operator<<(std::ostream& os, மாறிலி V3_Control_Ignores_வரி& rhs) {
    திரும்பு os << rhs.m_வரி_Min << "-" << rhs.m_வரி_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
நிலையான std::vector<V3_Control_Ignores_வரி> control_Ign_வரிகள்;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

வகுப்பு VInterval_மரம் final {
    கட்டமைப்பு Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        முழு m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(முழு center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_வரிகள்;  // All-line disables

தனிப்பட்ட:
    std::unique_ptr<Entry> build_மரம்(மாறிலி IgnIndices& points) {
        எனில் (points.empty()) திரும்பு nullptr;

        முழு minval = std::numeric_limits<முழு>::max();
        முழு maxval = std::numeric_limits<முழு>::min();
        க்கு (மாறிலி auto& it : points) {
            மாறிலி V3_Control_Ignores_வரி& cign = control_Ign_வரிகள்[it];
            எனில் (cign.every_வரி()) {
                m_every_வரிகள்.emplace_back(it);
                தொடர்;
            }
            minval = std::min(minval, cign.m_வரி_Min);
            maxval = std::max(maxval, cign.m_வரி_Max);
        }

        மாறிலி முழு center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(புதிய Entry{center});

        IgnIndices இடது_Points;
        IgnIndices வலது_Points;
        க்கு (மாறிலி auto& it : points) {
            மாறிலி V3_Control_Ignores_வரி& cign = control_Ign_வரிகள்[it];
            எனில் (cign.every_வரி()) தொடர்;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            எனில் (cign.m_வரி_Max < center)
                இடது_Points.emplace_back(it);
            இல்லையெனில் எனில் (cign.m_வரி_Min > center)
                வலது_Points.emplace_back(it);
            இல்லையெனில் {
                entp->m_byMin.தள்ளு_back(it);
                entp->m_byMax.தள்ளு_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            திரும்பு control_Ign_வரிகள்[a].m_வரி_Min < control_Ign_வரிகள்[b].m_வரி_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            திரும்பு control_Ign_வரிகள்[a].m_வரி_Max > control_Ign_வரிகள்[b].m_வரி_Max;
        });

        entp->m_leftp = build_மரம்(இடது_Points);
        entp->m_rightp = build_மரம்(வலது_Points);
        திரும்பு entp;
    }

    வெறுமை find_மரம்(முழு lineno, Entry* entp, IgnIndices& resultsr, முழு& nextChanger) மாறிலி {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        எனில் (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            க்கு (மாறிலி auto& it : entp->m_byMin) {
                மாறிலி V3_Control_Ignores_வரி& cign = control_Ign_வரிகள்[it];
                எனில் (cign.m_வரி_Min <= lineno) {
                    resultsr.emplace_back(it);
                } இல்லையெனில் {
                    nextChanger = std::min(nextChanger, cign.m_வரி_Min);
                    நிறுத்து;
                }
            }
            எனில் (entp->m_leftp) find_மரம்(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } இல்லையெனில் எனில் (lineno > entp->m_center) {
            க்கு (மாறிலி auto& it : entp->m_byMax) {
                மாறிலி V3_Control_Ignores_வரி& cign = control_Ign_வரிகள்[it];
                எனில் (cign.m_வரி_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_வரி_Max + 1);
                    resultsr.emplace_back(it);
                } இல்லையெனில் {
                    நிறுத்து;
                }
            }
            எனில் (entp->m_rightp) find_மரம்(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } இல்லையெனில் {
            க்கு (மாறிலி auto& it : entp->m_byMin) {
                மாறிலி V3_Control_Ignores_வரி& cign = control_Ign_வரிகள்[it];
                resultsr.emplace_back(it);
                எனில் (cign.m_வரி_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_வரி_Max + 1);
            }
            எனில் (entp->m_leftp) find_மரம்(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            எனில் (entp->m_rightp) find_மரம்(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

பொது:
    // CONSTRUCTORS
    VInterval_மரம்() = இயல்பு;
    ~VInterval_மரம்() = இயல்பு;
    // METHODS
    வெறுமை clear() { m_rootp = nullptr; }
    வெறுமை build(மாறிலி IgnIndices& points) {
        clear();
        m_rootp = build_மரம்(points);
    }
    வெறுமை find(முழு lineno, IgnIndices& resultsr, முழு& nextChanger) மாறிலி {
        resultsr = m_every_வரிகள்;
        nextChanger = std::numeric_limits<முழு>::max();

        எனில் (m_rootp) find_மரம்(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    நிலையான வெறுமை selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<முழு, முழு>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        க்கு (மாறிலி auto& it : points) {
            control_Ign_வரிகள்.emplace_back(
                V3_Control_Ignores_வரி{V3ErrorCode::I_LINT, it.first, it.second, உண்மை});
            data.emplace_back(static_cast<uint32_t>(control_Ign_வரிகள்.size() - 1));
        }
        VInterval_மரம் மரம்;
        மரம்.build(data);

        IgnIndices results;
        முழு nextChange = 0;
        மரம்.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        மரம்.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        மரம்.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        மரம்.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        மரம்.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        மரம்.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        மரம்.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        மரம்.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<முழு>::max());
        //
        points = {{0, 0}};
        க்கு (மாறிலி auto& it : points) {
            control_Ign_வரிகள்.emplace_back(
                V3_Control_Ignores_வரி{V3ErrorCode::I_LINT, it.first, it.second, உண்மை});
            data.emplace_back(static_cast<uint32_t>(control_Ign_வரிகள்.size() - 1));
        }
        மரம்.build(data);
        //
        மரம்.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_வரி_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

வகுப்பு WaiverSetting final {
பொது:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, மாறிலி string& contents, மாறிலி string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = இயல்பு;
    WaiverSetting& operator=(மாறிலி WaiverSetting& rhs) = இயல்பு;
};

// File entity
வகுப்பு V3ControlFile final {
    using வரி_Attr_Map = std::map<முழு, V3_Control_வரி_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    வரி_Attr_Map m_வரி_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_மரம் m_interval_மரம்;  // Tree of indices to IgnLines

    கட்டமைப்பு {
        முழு filenameno = -1;  // Last filename
        முழு lineno = -1;  // Last linenumber
        முழு nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    தருக்க வரி_Match(முழு lineno, VPragmaType type) {
        எனில் (m_வரி_Attrs.find(0) != m_வரி_Attrs.end() && m_வரி_Attrs[0][type]) திரும்பு உண்மை;
        எனில் (m_வரி_Attrs.find(lineno) == m_வரி_Attrs.end()) திரும்பு பொய்;
        திரும்பு m_வரி_Attrs[lineno][type];
    }

பொது:
    V3ControlFile() = இயல்பு;
    வெறுமை update(மாறிலி V3ControlFile& file) {
        // Copy in all attributes and waivers
        க்கு (மாறிலி auto& itr : file.m_வரி_Attrs) m_வரி_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.செருகு(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.செருகு(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    வெறுமை updateFinalize() { m_interval_மரம்.build(m_ignIndices); }
    வெறுமை add_வரி_Attribute(முழு lineno, VPragmaType attr) { m_வரி_Attrs[lineno].set(attr); }
    வெறுமை addIgnore(V3ErrorCode code, முழு min, முழு max, தருக்க on) {
        control_Ign_வரிகள்.emplace_back(V3_Control_Ignores_வரி{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_வரிகள்.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    வெறுமை addIgnoreMatch(V3ErrorCode code, மாறிலி string& contents, மாறிலி string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        எனில் (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    வெறுமை applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        மாறிலி VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        எனில் (வரி_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(புதிய AstPragma{nodep->fileline(), pragma});
        }
    }
    வெறுமை applyBlock(Ast_முனை_Block* nodep) {
        // Apply to block at this line
        மாறிலி VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_வரி* மாறிலி flp = nodep->fileline();
        எனில் (வரி_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(புதிய AstStmtPragma{flp, புதிய AstPragma{flp, pragma}});
        }
    }
    வெறுமை applyCase(AstCase* nodep) {
        // Apply to this case at this line
        மாறிலி முழு lineno = nodep->fileline()->lineno();
        எனில் (வரி_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(உண்மை);
        எனில் (வரி_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(உண்மை);
    }
    வெறுமை applyIgnores(File_வரி* filelinep) {
        // HOT routine, called each parsed token line of this filename
        எனில் (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            திரும்பு;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        எனில் (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            திரும்பு;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<முழு>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_மரம்.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        க்கு (மாறிலி auto& it : results) {
            மாறிலி V3_Control_Ignores_வரி& cign = control_Ign_வரிகள்[it];
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
    தருக்க waive(V3ErrorCode code, மாறிலி string& message) {
        எனில் (code.hardError()) திரும்பு பொய்;
        க்கு (மாறிலி auto& itr : m_waivers) {
            எனில் ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                திரும்பு உண்மை;
            }
        }
        திரும்பு பொய்;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

வகுப்பு V3ControlScopeTraceEntry final {
பொது:
    மாறிலி string m_scope;  // Scope or regexp to match
    மாறிலி தருக்க m_on;  // True to enable message
    முழு m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(மாறிலி string& scope, தருக்க on, முழு levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = இயல்பு;
    தருக்க operator<(மாறிலி V3ControlScopeTraceEntry& other) மாறிலி {
        எனில் (m_on < other.m_on) திரும்பு உண்மை;
        எனில் (m_on > other.m_on) திரும்பு பொய்;
        எனில் (m_levels < other.m_levels) திரும்பு உண்மை;
        எனில் (m_levels > other.m_levels) திரும்பு பொய்;
        திரும்பு m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
வகுப்பு V3ControlScopeTraceEntryMatch final {
பொது:
    மாறிலி V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    மாறிலி string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(மாறிலி V3ControlScopeTraceEntry* entryp, மாறிலி string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    தருக்க operator<(மாறிலி V3ControlScopeTraceEntryMatch& other) மாறிலி {
        எனில் (m_entryp < other.m_entryp) திரும்பு உண்மை;
        எனில் (m_entryp > other.m_entryp) திரும்பு பொய்;
        திரும்பு m_scopepart < other.m_scopepart;
    }
};

வகுப்பு V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, தருக்க> m_matchCache;  // Matching entries for speed

பொது:
    வெறுமை addScopeTraceOn(தருக்க on, மாறிலி string& scope, முழு levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    தருக்க getEntryMatch(மாறிலி V3ControlScopeTraceEntry* entp, மாறிலி string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        மாறிலி V3ControlScopeTraceEntryMatch சாவி{entp, scopepart};
        மாறிலி auto pair = m_matchCache.emplace(சாவி, பொய்);
        எனில் (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        திரும்பு pair.first->second;
    }

    தருக்க getScopeTraceOn(மாறிலி string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        முழு maxLevel = 1;
        க்கு (மாறிலி auto& ch : scope) {
            எனில் (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        தருக்க enabled = உண்மை;
        க்கு (மாறிலி auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            முழு partLevel = 1;
            க்கு (string::size_type partEnd = 0; உண்மை;) {
                partEnd = scope.find('.', partEnd + 1);
                எனில் (partEnd == string::npos) partEnd = scope.length();
                மாறிலி std::string scopepart = scope.substr(0, partEnd);
                எனில் (getEntryMatch(&ent, scopepart)) {
                    மாறிலி தருக்க levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    எனில் (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    நிறுத்து;
                }
                எனில் (partEnd == scope.length()) நிறுத்து;
                ++partLevel;
            }
        }
        திரும்பு enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

வகுப்பு V3ControlResolverHierWorkerEntry final {
    மாறிலி முழு m_workers;  // Number of workers requested for the hierarchical block
    File_வரி* மாறிலி m_flp;  // Location the worker count was specified at

பொது:
    explicit V3ControlResolverHierWorkerEntry(முழு workers, File_வரி* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    முழு workers() மாறிலி { திரும்பு m_workers; }
    File_வரி* flp() மாறிலி { திரும்பு m_flp; }
};

வகுப்பு V3ControlResolver final {
    எண்ணிக்கை ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_வரி* m_profile_File_வரி = nullptr;  // Location profile_data was read from

    V3ControlResolver() = இயல்பு;
    ~V3ControlResolver() = இயல்பு;

பொது:
    நிலையான V3ControlResolver& s() {
        நிலையான V3ControlResolver s_singleton;
        திரும்பு s_singleton;
    }
    V3ControlModuleResolver& modules() { திரும்பு m_modules; }
    V3ControlFileResolver& files() { திரும்பு m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { திரும்பு m_scopeTraces; }

    வெறுமை addProfileData(File_வரி* fl, மாறிலி string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    வெறுமை addProfileData(File_வரி* fl, மாறிலி string& model, மாறிலி string& சாவி, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        எனில் (!m_profile_File_வரி) m_profile_File_வரி = fl;
        எனில் (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        எனில் (mode == MTASK) {
            m_profileData[model][சாவி] += cost;
        } இல்லையெனில் எனில் (mode == HIER_DPI) {
            m_profileData[model][சாவி] = std::max(m_profileData[model][சாவி], cost);
        }
        m_mode |= mode;
    }
    தருக்க containsMTaskProfileData() மாறிலி { திரும்பு m_mode & MTASK; }
    uint64_t getProfileData(மாறிலி string& hierDpi) மாறிலி {
        // Empty key for hierarchical DPI wrapper costs.
        திரும்பு getProfileData(hierDpi, "");
    }
    வெறுமை addHierWorkers(File_வரி* flp, மாறிலி string& model, முழு workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    முழு getHierWorkers(மாறிலி string& model) மாறிலி {
        மாறிலி auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        திரும்பு mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_வரி* get_Hier_Workers_File_வரி(மாறிலி string& model) மாறிலி {
        மாறிலி auto mit = m_hierWorkers.find(model);
        திரும்பு mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(மாறிலி string& model, மாறிலி string& சாவி) மாறிலி {
        மாறிலி auto mit = m_profileData.find(model);
        எனில் (mit == m_profileData.cend()) திரும்பு 0;
        மாறிலி auto it = mit->second.find(சாவி);
        எனில் (it == mit->second.cend()) திரும்பு 0;
        திரும்பு it->second;
    }
    File_வரி* get_Profile_Data_File_வரி() மாறிலி { திரும்பு m_profile_File_வரி; }  // Maybe null
    நிலையான uint64_t getCurrentHierBlockCost() {
        எனில் (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            திரும்பு cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* மாறிலி netlistp = v3Global.rootp();
        uint64_t cost = 0;
        க்கு (முழு i = 0; i < VEval::_ENUM_END; ++i) {
            மாறிலி VEval eval{i};
            எனில் (eval.slow()) தொடர்;
            cost += V3_Instr_மொத்தம்::மொத்தம்(netlistp->evalFuncp(eval), பொய்);
        }
        UINFO(9, "Evaluating cost: " << cost);
        திரும்பு cost;
    }
};

//######################################################################
// V3Control

வெறுமை V3Control::addCaseFull(மாறிலி string& filename, முழு lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_வரி_Attribute(lineno, VPragmaType::FULL_CASE);
}
வெறுமை V3Control::addCaseParallel(மாறிலி string& filename, முழு lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_வரி_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
வெறுமை V3Control::addCoverageBlockOff(மாறிலி string& filename, முழு lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_வரி_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
வெறுமை V3Control::addCoverageBlockOff(மாறிலி string& module, மாறிலி string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

வெறுமை V3Control::addHierWorkers(File_வரி* fl, மாறிலி string& model, முழு workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

வெறுமை V3Control::addFsmRegisterWrapper(File_வரி* fl, மாறிலி string& module, மாறிலி string& d,
                                      மாறிலி string& q, மாறிலி string& clock, மாறிலி string& reset,
                                      மாறிலி string& reset_மதிப்பு) {
    string missing;
    எனில் (module.empty()) missing += "-module";
    எனில் (d.empty()) missing = VString::dot(missing, ", ", "-d");
    எனில் (q.empty()) missing = VString::dot(missing, ", ", "-q");
    எனில் (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    எனில் (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        திரும்பு;
    }

    மாறிலி FsmRegisterWrapper desc{module, d, q, clock, reset, reset_மதிப்பு};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

வெறுமை V3Control::addIgnore(V3ErrorCode code, தருக்க on, மாறிலி string& filename, முழு min, முழு max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    எனில் (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_வரி::globalWarnOff(code, !on);
    } இல்லையெனில் {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
வெறுமை V3Control::addIgnoreMatch(V3ErrorCode code, மாறிலி string& filename, மாறிலி string& contents,
                               மாறிலி string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

வெறுமை V3Control::addInline(File_வரி* fl, மாறிலி string& module, மாறிலி string& ftask, தருக்க on) {
    எனில் (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } இல்லையெனில் {
        எனில் (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } இல்லையெனில் {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

வெறுமை V3Control::addModulePragma(மாறிலி string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

வெறுமை V3Control::addProfileData(File_வரி* fl, மாறிலி string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
வெறுமை V3Control::addProfileData(File_வரி* fl, மாறிலி string& model, மாறிலி string& சாவி,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, சாவி, cost);
}

வெறுமை V3Control::addScopeTraceOn(தருக்க on, மாறிலி string& scope, முழு levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

வெறுமை V3Control::addVarAttr(File_வரி* fl, மாறிலி string& module, மாறிலி string& ftask,
                           VarSpecKind kind, மாறிலி string& pattern, VAttrType attr,
                           Ast_Sen_மரம்* sensep) {
    எனில் (sensep) {
        File_வரி* மாறிலி flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_மரம்(), sensep);
        // Used to be only accepted on public_flat_rw
        எனில் (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            திரும்பு;
        }
    }

    எனில் (kind != VarSpecKind::VAR) {
        தேர்வு (attr) {
        நிலை VAttrType::VAR_PUBLIC_FLAT:
        நிலை VAttrType::VAR_PUBLIC_FLAT_RD:
        நிலை VAttrType::VAR_PUBLIC_FLAT_RW: நிறுத்து;
        இயல்பு:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            திரும்பு;
        }
    }

    // Semantics: Most of the attributes operate on signals
    எனில் (pattern.empty()) {
        எனில் (attr == VAttrType::VAR_PUBLIC) {
            எனில் (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } இல்லையெனில் {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(உண்மை);
            }
        } இல்லையெனில் {
            fl->v3error("missing -var");
        }
    } இல்லையெனில் {
        எனில் (attr == VAttrType::VAR_FORCEABLE) {
            எனில் (module.empty()) {
                fl->v3error("forceable missing -module");
            } இல்லையெனில் எனில் (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } இல்லையெனில் {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } இல்லையெனில் {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                எனில் (ftask.empty()) {
                    எனில் (kind == VarSpecKind::PARAM) திரும்பு mod.params().at(pattern);
                    எனில் (kind == VarSpecKind::PORT) திரும்பு mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    திரும்பு mod.vars().at(pattern);
                }
                எனில் (kind == VarSpecKind::PARAM) திரும்பு mod.ftasks().at(ftask).params().at(pattern);
                எனில் (kind == VarSpecKind::PORT) திரும்பு mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                திரும்பு mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

வெறுமை V3Control::applyCase(AstCase* nodep) {
    மாறிலி string& filename = nodep->fileline()->filename();
    V3ControlFile* மாறிலி filep = V3ControlResolver::s().files().resolve(filename);
    எனில் (filep) filep->applyCase(nodep);
}

வெறுமை V3Control::applyCoverageBlock(Ast_முனை_Module* modulep, AstBegin* nodep) {
    மாறிலி string& filename = nodep->fileline()->filename();
    V3ControlFile* மாறிலி filep = V3ControlResolver::s().files().resolve(filename);
    எனில் (filep) filep->applyBlock(nodep);
    மாறிலி string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* மாறிலி modp = V3ControlResolver::s().modules().resolve(modname);
    எனில் (modp) modp->applyBlock(nodep);
}
வெறுமை V3Control::applyCoverageBlock(Ast_முனை_Module* modulep, AstGenBlock* nodep) {
    மாறிலி string& filename = nodep->fileline()->filename();
    V3ControlFile* மாறிலி filep = V3ControlResolver::s().files().resolve(filename);
    எனில் (filep) filep->applyBlock(nodep);
    மாறிலி string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* மாறிலி modp = V3ControlResolver::s().modules().resolve(modname);
    எனில் (modp) modp->applyBlock(nodep);
}

வெறுமை V3Control::applyIgnores(File_வரி* filelinep) {
    மாறிலி string& filename = filelinep->filename();
    V3ControlFile* மாறிலி filep = V3ControlResolver::s().files().resolve(filename);
    எனில் (filep) filep->applyIgnores(filelinep);
}

வெறுமை V3Control::applyModule(Ast_முனை_Module* modulep) {
    மாறிலி string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* மாறிலி modp = V3ControlResolver::s().modules().resolve(modname);
    எனில் (modp) modp->apply(modulep);
}

வெறுமை V3Control::applyFTask(Ast_முனை_Module* modulep, Ast_முனை_FTask* ftaskp) {
    மாறிலி string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* மாறிலி modp = V3ControlResolver::s().modules().resolve(modname);
    எனில் (!modp) திரும்பு;
    மாறிலி V3ControlFTask* மாறிலி ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    எனில் (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
நிலையான வெறுமை resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    மாறிலி std::string name = varp->prettyDehashOrigOrName();
    எனில் (மாறிலி V3ControlVar* மாறிலி vp = resolver.vars().resolve(name)) vp->apply(varp);
    எனில் (varp->isParam()) {
        எனில் (மாறிலி V3ControlVar* மாறிலி vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    எனில் (varp->isIO()) {
        எனில் (மாறிலி V3ControlVar* மாறிலி vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

வெறுமை V3Control::applyVarAttr(மாறிலி Ast_முனை_Module* modulep, மாறிலி Ast_முனை_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* மாறிலி modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    எனில் (!modp) திரும்பு;
    எனில் (ftaskp) {
        V3ControlFTask* மாறிலி ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        எனில் (!ftp) திரும்பு;
        resolveThenApply(*ftp, varp);
    } இல்லையெனில் {
        resolveThenApply(*modp, varp);
    }
}

முழு V3Control::getHierWorkers(மாறிலி string& model) {
    திரும்பு V3ControlResolver::s().getHierWorkers(model);
}
File_வரி* V3Control::get_Hier_Workers_File_வரி(மாறிலி string& model) {
    திரும்பு V3ControlResolver::s().get_Hier_Workers_File_வரி(model);
}
மாறிலி V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(மாறிலி string& module) {
    V3ControlModule* மாறிலி modp = V3ControlResolver::s().modules().resolve(module);
    திரும்பு modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(மாறிலி string& hierDpi) {
    திரும்பு V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(மாறிலி string& model, மாறிலி string& சாவி) {
    திரும்பு V3ControlResolver::s().getProfileData(model, சாவி);
}
File_வரி* V3Control::get_Profile_Data_File_வரி() {
    திரும்பு V3ControlResolver::s().get_Profile_Data_File_வரி();
}
தருக்க V3Control::getScopeTraceOn(மாறிலி string& scope) {
    திரும்பு V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

வெறுமை V3Control::contents_தள்ளு_Text(மாறிலி string& text) { WildcardContents::தள்ளு_Text(text); }

தருக்க V3Control::containsMTaskProfileData() {
    திரும்பு V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    திரும்பு V3ControlResolver::s().getCurrentHierBlockCost();
}

தருக்க V3Control::waive(மாறிலி File_வரி* filelinep, V3ErrorCode code, மாறிலி string& message) {
    V3ControlFile* மாறிலி filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    எனில் (!filep) திரும்பு பொய்;
    திரும்பு filep->waive(code, message);
}

வெறுமை V3Control::selfTest() { VInterval_மரம்::selfTest(); }
