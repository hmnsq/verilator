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

#شامل "V3PchAstMT.h"

#شامل "V3Control.h"

#شامل "V3InstrCount.h"
#شامل "V3Os.h"
#شامل "V3String.h"

#شامل <memory>
#شامل <set>
#شامل <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
رده V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ثابت std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ثابت std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

عمومی:
    V3ControlWildcardResolver() = پیشفرض;
    ~V3ControlWildcardResolver() = پیشفرض;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    تهی update(ثابت V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        ثابت V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        برای (ثابت auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(ثابت string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        بازگشت m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ثابت string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ثابت V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        ثابت auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        اگر (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ثابت string slashName = V3Os::filenameSlashPath(name);
            برای (ثابت auto& patEnt : m_mapPatterns) {
                اگر (VString::wildmatch(slashName, patEnt.first)) {
                    اگر (!entryr) entryr.reset(جدید T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            اگر (entryr) entryr->updateFinalize();
        }
        بازگشت entryr.get();
    }
};

// List of attributes for variables
رده V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
عمومی:
    // Add new attribugte
    تهی add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    تهی update(ثابت V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.درج(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    تهی updateFinalize() {}
    // Apply all attributes to the variable
    تهی apply(AstVar* varp) ثابت {
        برای (ثابت VAttrType attr : m_attrs) {
            varp->addAttrsp(جدید AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

رده WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ثابت std::string, منطقی> m_mapPatterns;  // Pattern match results
    std::deque<string> m_سطرها;  // Source text lines

    // METHODS
    ایستا WildcardContents& s() {  // Singleton
        ایستا WildcardContents s_s;
        بازگشت s_s;
    }
    تهی clearCacheImp() { m_mapPatterns.clear(); }
    تهی افزودن_Text_Imp(ثابت string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ثابت string leftover = m_سطرها.back() + text;
        m_سطرها.برداشت_back();

        // Insert line-by-line
        string::size_type سطر_start = 0;
        وقتی (درست) {
            ثابت string::size_type سطر_end = leftover.find('\n', سطر_start);
            اگر (سطر_end != string::npos) {
                ثابت string oneline(leftover, سطر_start, سطر_end - سطر_start + 1);
                اگر (oneline.size() > 1) m_سطرها.افزودن_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_سطرها.size() - 1) << "]: " << oneline);
                سطر_start = سطر_end + 1;
            } وگرنه {
                بشکن;
            }
        }
        // Keep leftover for next time
        m_سطرها.emplace_back(string(leftover, سطر_start));  // Might be ""
        clearCacheImp();
    }

    منطقی resolveUncachedImp(ثابت string& contentsRegexp) {
        برای (ثابت string& i : m_سطرها) {
            اگر (VString::wildmatch(i, contentsRegexp)) بازگشت درست;
        }
        بازگشت نادرست;
    }
    منطقی resolveCachedImp(ثابت string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        اگر (contentsRegexp.empty()) بازگشت درست;
        ثابت auto pair = m_mapPatterns.emplace(contentsRegexp, نادرست);
        منطقی& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        اگر (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        بازگشت entryr;
    }

عمومی:
    WildcardContents() {
        m_سطرها.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = پیشفرض;
    // Return true iff contentsRegexp in parsed contents
    ایستا منطقی resolve(ثابت string& contentsRegexp) {
        بازگشت s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ایستا تهی افزودن_Text(ثابت string& text) { s().افزودن_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

رده V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    منطقی m_noinline = نادرست;  // Don't inline function/task
    منطقی m_public = نادرست;  // Public function/task

عمومی:
    V3ControlFTask() = پیشفرض;
    تهی update(ثابت V3ControlFTask& f) {
        // Don't overwrite true with false
        اگر (f.m_noinline) m_noinline = درست;
        اگر (f.m_public) m_public = درست;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    تهی updateFinalize() {}

    V3ControlVarResolver& params() { بازگشت m_params; }
    V3ControlVarResolver& ports() { بازگشت m_ports; }
    V3ControlVarResolver& vars() { بازگشت m_vars; }

    تهی setNoInline(منطقی set) { m_noinline = set; }
    تهی setPublic(منطقی set) { m_public = set; }

    تهی apply(Ast_گره_FTask* ftaskp) ثابت {
        اگر (m_noinline)
            ftaskp->addStmtsp(جدید AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        اگر (m_public)
            ftaskp->addStmtsp(جدید AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

رده V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    منطقی m_hasFsmRegisterWrapper = نادرست;  // Whether descriptor has been set
    منطقی m_inline = نادرست;  // Whether to force the inline
    منطقی m_inline_مقدار = نادرست;  // The inline value (on/off)

عمومی:
    V3ControlModule() = پیشفرض;

    تهی update(ثابت V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        اگر (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = درست;
        }
        برای (ثابت string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.درج(i);
        اگر (!m_inline) {
            m_inline = m.m_inline;
            m_inline_مقدار = m.m_inline_مقدار;
        }
        برای (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.درج(*it);
        }
    }
    تهی updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { بازگشت m_tasks; }
    V3ControlVarResolver& params() { بازگشت m_params; }
    V3ControlVarResolver& ports() { بازگشت m_ports; }
    V3ControlVarResolver& vars() { بازگشت m_vars; }

    تهی addCoverageBlockOff(ثابت string& name) { m_coverageOffBlocks.درج(name); }
    تهی setInline(منطقی set) {
        m_inline = درست;
        m_inline_مقدار = set;
    }
    تهی setFsmRegisterWrapper(File_سطر* fl, ثابت V3Control::FsmRegisterWrapper& desc) {
        اگر (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_گره::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = درست;
    }
    ثابت V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() ثابت {
        بازگشت m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    تهی addModulePragma(VPragmaType pragma) { m_modPragmas.درج(pragma); }

    تهی apply(Ast_گره_Module* modp) {
        اگر (m_inline) {
            ثابت VPragmaType type
                = m_inline_مقدار ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_گره* ثابت nodep = جدید AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        برای (ثابت auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            اگر (itr == VPragmaType::HIER_PARAMS) modp->hierParams(درست);
            Ast_گره* ثابت nodep = جدید AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    تهی applyBlock(AstGenBlock* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        اگر (!nodep->unnamed()) {
            برای (ثابت string& i : m_coverageOffBlocks) {
                اگر (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(جدید AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    تهی applyBlock(Ast_گره_Block* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_سطر* ثابت flp = nodep->fileline();
        اگر (!nodep->unnamed()) {
            برای (ثابت string& i : m_coverageOffBlocks) {
                اگر (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(جدید AstStmtPragma{flp, جدید AstPragma{flp, pragma}});
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
رده V3_Control_Ignores_سطر final {
عمومی:
    ثابت صحیح m_سطر_Min;  // Minimum line number to make change at
    ثابت صحیح m_سطر_Max;  // Maximum line number to make change at (inclusive)
    ثابت V3ErrorCode m_code;  // Error code
    ثابت منطقی m_on;  // True to enable message
    V3_Control_Ignores_سطر(V3ErrorCode code, صحیح linemin, صحیح linemax, منطقی on)
        : m_سطر_Min{linemin}
        , m_سطر_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_سطر() = پیشفرض;
    منطقی every_سطر() ثابت { بازگشت m_سطر_Min == 0 && m_سطر_Max == 0; }
    منطقی inRange(صحیح lineno) ثابت {
        بازگشت (lineno >= m_سطر_Min && lineno <= m_سطر_Max) || every_سطر();
    }
};
std::ostream& operator<<(std::ostream& os, ثابت V3_Control_Ignores_سطر& rhs) {
    بازگشت os << rhs.m_سطر_Min << "-" << rhs.m_سطر_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ایستا std::vector<V3_Control_Ignores_سطر> control_Ign_سطرها;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

رده VInterval_درخت final {
    ساختار Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        صحیح m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(صحیح center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_سطرها;  // All-line disables

خصوصی:
    std::unique_ptr<Entry> build_درخت(ثابت IgnIndices& points) {
        اگر (points.empty()) بازگشت nullptr;

        صحیح minval = std::numeric_limits<صحیح>::max();
        صحیح maxval = std::numeric_limits<صحیح>::min();
        برای (ثابت auto& it : points) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
            اگر (cign.every_سطر()) {
                m_every_سطرها.emplace_back(it);
                ادامه;
            }
            minval = std::min(minval, cign.m_سطر_Min);
            maxval = std::max(maxval, cign.m_سطر_Max);
        }

        ثابت صحیح center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(جدید Entry{center});

        IgnIndices چپ_Points;
        IgnIndices راست_Points;
        برای (ثابت auto& it : points) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
            اگر (cign.every_سطر()) ادامه;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            اگر (cign.m_سطر_Max < center)
                چپ_Points.emplace_back(it);
            وگرنه اگر (cign.m_سطر_Min > center)
                راست_Points.emplace_back(it);
            وگرنه {
                entp->m_byMin.افزودن_back(it);
                entp->m_byMax.افزودن_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            بازگشت control_Ign_سطرها[a].m_سطر_Min < control_Ign_سطرها[b].m_سطر_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            بازگشت control_Ign_سطرها[a].m_سطر_Max > control_Ign_سطرها[b].m_سطر_Max;
        });

        entp->m_leftp = build_درخت(چپ_Points);
        entp->m_rightp = build_درخت(راست_Points);
        بازگشت entp;
    }

    تهی find_درخت(صحیح lineno, Entry* entp, IgnIndices& resultsr, صحیح& nextChanger) ثابت {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        اگر (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            برای (ثابت auto& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
                اگر (cign.m_سطر_Min <= lineno) {
                    resultsr.emplace_back(it);
                } وگرنه {
                    nextChanger = std::min(nextChanger, cign.m_سطر_Min);
                    بشکن;
                }
            }
            اگر (entp->m_leftp) find_درخت(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } وگرنه اگر (lineno > entp->m_center) {
            برای (ثابت auto& it : entp->m_byMax) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
                اگر (cign.m_سطر_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_سطر_Max + 1);
                    resultsr.emplace_back(it);
                } وگرنه {
                    بشکن;
                }
            }
            اگر (entp->m_rightp) find_درخت(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } وگرنه {
            برای (ثابت auto& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
                resultsr.emplace_back(it);
                اگر (cign.m_سطر_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_سطر_Max + 1);
            }
            اگر (entp->m_leftp) find_درخت(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            اگر (entp->m_rightp) find_درخت(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

عمومی:
    // CONSTRUCTORS
    VInterval_درخت() = پیشفرض;
    ~VInterval_درخت() = پیشفرض;
    // METHODS
    تهی clear() { m_rootp = nullptr; }
    تهی build(ثابت IgnIndices& points) {
        clear();
        m_rootp = build_درخت(points);
    }
    تهی find(صحیح lineno, IgnIndices& resultsr, صحیح& nextChanger) ثابت {
        resultsr = m_every_سطرها;
        nextChanger = std::numeric_limits<صحیح>::max();

        اگر (m_rootp) find_درخت(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    ایستا تهی selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<صحیح, صحیح>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        برای (ثابت auto& it : points) {
            control_Ign_سطرها.emplace_back(
                V3_Control_Ignores_سطر{V3ErrorCode::I_LINT, it.first, it.second, درست});
            data.emplace_back(static_cast<uint32_t>(control_Ign_سطرها.size() - 1));
        }
        VInterval_درخت درخت;
        درخت.build(data);

        IgnIndices results;
        صحیح nextChange = 0;
        درخت.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        درخت.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        درخت.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        درخت.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        درخت.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        درخت.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        درخت.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        درخت.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<صحیح>::max());
        //
        points = {{0, 0}};
        برای (ثابت auto& it : points) {
            control_Ign_سطرها.emplace_back(
                V3_Control_Ignores_سطر{V3ErrorCode::I_LINT, it.first, it.second, درست});
            data.emplace_back(static_cast<uint32_t>(control_Ign_سطرها.size() - 1));
        }
        درخت.build(data);
        //
        درخت.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_سطر_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

رده WaiverSetting final {
عمومی:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, ثابت string& contents, ثابت string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = پیشفرض;
    WaiverSetting& operator=(ثابت WaiverSetting& rhs) = پیشفرض;
};

// File entity
رده V3ControlFile final {
    using سطر_Attr_Map = std::map<صحیح, V3_Control_سطر_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    سطر_Attr_Map m_سطر_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_درخت m_interval_درخت;  // Tree of indices to IgnLines

    ساختار {
        صحیح filenameno = -1;  // Last filename
        صحیح lineno = -1;  // Last linenumber
        صحیح nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    منطقی سطر_Match(صحیح lineno, VPragmaType type) {
        اگر (m_سطر_Attrs.find(0) != m_سطر_Attrs.end() && m_سطر_Attrs[0][type]) بازگشت درست;
        اگر (m_سطر_Attrs.find(lineno) == m_سطر_Attrs.end()) بازگشت نادرست;
        بازگشت m_سطر_Attrs[lineno][type];
    }

عمومی:
    V3ControlFile() = پیشفرض;
    تهی update(ثابت V3ControlFile& file) {
        // Copy in all attributes and waivers
        برای (ثابت auto& itr : file.m_سطر_Attrs) m_سطر_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.درج(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.درج(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    تهی updateFinalize() { m_interval_درخت.build(m_ignIndices); }
    تهی add_سطر_Attribute(صحیح lineno, VPragmaType attr) { m_سطر_Attrs[lineno].set(attr); }
    تهی addIgnore(V3ErrorCode code, صحیح min, صحیح max, منطقی on) {
        control_Ign_سطرها.emplace_back(V3_Control_Ignores_سطر{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_سطرها.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    تهی addIgnoreMatch(V3ErrorCode code, ثابت string& contents, ثابت string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        اگر (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    تهی applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        اگر (سطر_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(جدید AstPragma{nodep->fileline(), pragma});
        }
    }
    تهی applyBlock(Ast_گره_Block* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_سطر* ثابت flp = nodep->fileline();
        اگر (سطر_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(جدید AstStmtPragma{flp, جدید AstPragma{flp, pragma}});
        }
    }
    تهی applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ثابت صحیح lineno = nodep->fileline()->lineno();
        اگر (سطر_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(درست);
        اگر (سطر_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(درست);
    }
    تهی applyIgnores(File_سطر* filelinep) {
        // HOT routine, called each parsed token line of this filename
        اگر (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            بازگشت;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        اگر (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            بازگشت;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<صحیح>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_درخت.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        برای (ثابت auto& it : results) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطرها[it];
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
    منطقی waive(V3ErrorCode code, ثابت string& message) {
        اگر (code.hardError()) بازگشت نادرست;
        برای (ثابت auto& itr : m_waivers) {
            اگر ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                بازگشت درست;
            }
        }
        بازگشت نادرست;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

رده V3ControlScopeTraceEntry final {
عمومی:
    ثابت string m_scope;  // Scope or regexp to match
    ثابت منطقی m_on;  // True to enable message
    صحیح m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(ثابت string& scope, منطقی on, صحیح levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = پیشفرض;
    منطقی operator<(ثابت V3ControlScopeTraceEntry& other) ثابت {
        اگر (m_on < other.m_on) بازگشت درست;
        اگر (m_on > other.m_on) بازگشت نادرست;
        اگر (m_levels < other.m_levels) بازگشت درست;
        اگر (m_levels > other.m_levels) بازگشت نادرست;
        بازگشت m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
رده V3ControlScopeTraceEntryMatch final {
عمومی:
    ثابت V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    ثابت string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(ثابت V3ControlScopeTraceEntry* entryp, ثابت string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    منطقی operator<(ثابت V3ControlScopeTraceEntryMatch& other) ثابت {
        اگر (m_entryp < other.m_entryp) بازگشت درست;
        اگر (m_entryp > other.m_entryp) بازگشت نادرست;
        بازگشت m_scopepart < other.m_scopepart;
    }
};

رده V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, منطقی> m_matchCache;  // Matching entries for speed

عمومی:
    تهی addScopeTraceOn(منطقی on, ثابت string& scope, صحیح levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    منطقی getEntryMatch(ثابت V3ControlScopeTraceEntry* entp, ثابت string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ثابت V3ControlScopeTraceEntryMatch کلید{entp, scopepart};
        ثابت auto pair = m_matchCache.emplace(کلید, نادرست);
        اگر (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        بازگشت pair.first->second;
    }

    منطقی getScopeTraceOn(ثابت string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        صحیح maxLevel = 1;
        برای (ثابت auto& ch : scope) {
            اگر (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        منطقی enabled = درست;
        برای (ثابت auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            صحیح partLevel = 1;
            برای (string::size_type partEnd = 0; درست;) {
                partEnd = scope.find('.', partEnd + 1);
                اگر (partEnd == string::npos) partEnd = scope.length();
                ثابت std::string scopepart = scope.substr(0, partEnd);
                اگر (getEntryMatch(&ent, scopepart)) {
                    ثابت منطقی levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    اگر (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    بشکن;
                }
                اگر (partEnd == scope.length()) بشکن;
                ++partLevel;
            }
        }
        بازگشت enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

رده V3ControlResolverHierWorkerEntry final {
    ثابت صحیح m_workers;  // Number of workers requested for the hierarchical block
    File_سطر* ثابت m_flp;  // Location the worker count was specified at

عمومی:
    explicit V3ControlResolverHierWorkerEntry(صحیح workers, File_سطر* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    صحیح workers() ثابت { بازگشت m_workers; }
    File_سطر* flp() ثابت { بازگشت m_flp; }
};

رده V3ControlResolver final {
    شمارش ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_سطر* m_profile_File_سطر = nullptr;  // Location profile_data was read from

    V3ControlResolver() = پیشفرض;
    ~V3ControlResolver() = پیشفرض;

عمومی:
    ایستا V3ControlResolver& s() {
        ایستا V3ControlResolver s_singleton;
        بازگشت s_singleton;
    }
    V3ControlModuleResolver& modules() { بازگشت m_modules; }
    V3ControlFileResolver& files() { بازگشت m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { بازگشت m_scopeTraces; }

    تهی addProfileData(File_سطر* fl, ثابت string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    تهی addProfileData(File_سطر* fl, ثابت string& model, ثابت string& کلید, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        اگر (!m_profile_File_سطر) m_profile_File_سطر = fl;
        اگر (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        اگر (mode == MTASK) {
            m_profileData[model][کلید] += cost;
        } وگرنه اگر (mode == HIER_DPI) {
            m_profileData[model][کلید] = std::max(m_profileData[model][کلید], cost);
        }
        m_mode |= mode;
    }
    منطقی containsMTaskProfileData() ثابت { بازگشت m_mode & MTASK; }
    uint64_t getProfileData(ثابت string& hierDpi) ثابت {
        // Empty key for hierarchical DPI wrapper costs.
        بازگشت getProfileData(hierDpi, "");
    }
    تهی addHierWorkers(File_سطر* flp, ثابت string& model, صحیح workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    صحیح getHierWorkers(ثابت string& model) ثابت {
        ثابت auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        بازگشت mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_سطر* get_Hier_Workers_File_سطر(ثابت string& model) ثابت {
        ثابت auto mit = m_hierWorkers.find(model);
        بازگشت mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(ثابت string& model, ثابت string& کلید) ثابت {
        ثابت auto mit = m_profileData.find(model);
        اگر (mit == m_profileData.cend()) بازگشت 0;
        ثابت auto it = mit->second.find(کلید);
        اگر (it == mit->second.cend()) بازگشت 0;
        بازگشت it->second;
    }
    File_سطر* get_Profile_Data_File_سطر() ثابت { بازگشت m_profile_File_سطر; }  // Maybe null
    ایستا uint64_t getCurrentHierBlockCost() {
        اگر (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            بازگشت cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ثابت netlistp = v3Global.rootp();
        uint64_t cost = 0;
        برای (صحیح i = 0; i < VEval::_ENUM_END; ++i) {
            ثابت VEval eval{i};
            اگر (eval.slow()) ادامه;
            cost += V3_Instr_شمار::شمار(netlistp->evalFuncp(eval), نادرست);
        }
        UINFO(9, "Evaluating cost: " << cost);
        بازگشت cost;
    }
};

//######################################################################
// V3Control

تهی V3Control::addCaseFull(ثابت string& filename, صحیح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_سطر_Attribute(lineno, VPragmaType::FULL_CASE);
}
تهی V3Control::addCaseParallel(ثابت string& filename, صحیح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_سطر_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
تهی V3Control::addCoverageBlockOff(ثابت string& filename, صحیح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_سطر_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
تهی V3Control::addCoverageBlockOff(ثابت string& module, ثابت string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

تهی V3Control::addHierWorkers(File_سطر* fl, ثابت string& model, صحیح workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

تهی V3Control::addFsmRegisterWrapper(File_سطر* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_مقدار) {
    string missing;
    اگر (module.empty()) missing += "-module";
    اگر (d.empty()) missing = VString::dot(missing, ", ", "-d");
    اگر (q.empty()) missing = VString::dot(missing, ", ", "-q");
    اگر (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    اگر (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        بازگشت;
    }

    ثابت FsmRegisterWrapper desc{module, d, q, clock, reset, reset_مقدار};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

تهی V3Control::addIgnore(V3ErrorCode code, منطقی on, ثابت string& filename, صحیح min, صحیح max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    اگر (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_سطر::globalWarnOff(code, !on);
    } وگرنه {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
تهی V3Control::addIgnoreMatch(V3ErrorCode code, ثابت string& filename, ثابت string& contents,
                               ثابت string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

تهی V3Control::addInline(File_سطر* fl, ثابت string& module, ثابت string& ftask, منطقی on) {
    اگر (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } وگرنه {
        اگر (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } وگرنه {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

تهی V3Control::addModulePragma(ثابت string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

تهی V3Control::addProfileData(File_سطر* fl, ثابت string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
تهی V3Control::addProfileData(File_سطر* fl, ثابت string& model, ثابت string& کلید,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, کلید, cost);
}

تهی V3Control::addScopeTraceOn(منطقی on, ثابت string& scope, صحیح levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

تهی V3Control::addVarAttr(File_سطر* fl, ثابت string& module, ثابت string& ftask,
                           VarSpecKind kind, ثابت string& pattern, VAttrType attr,
                           Ast_Sen_درخت* sensep) {
    اگر (sensep) {
        File_سطر* ثابت flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_درخت(), sensep);
        // Used to be only accepted on public_flat_rw
        اگر (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            بازگشت;
        }
    }

    اگر (kind != VarSpecKind::VAR) {
        انتخاب (attr) {
        حالت VAttrType::VAR_PUBLIC_FLAT:
        حالت VAttrType::VAR_PUBLIC_FLAT_RD:
        حالت VAttrType::VAR_PUBLIC_FLAT_RW: بشکن;
        پیشفرض:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            بازگشت;
        }
    }

    // Semantics: Most of the attributes operate on signals
    اگر (pattern.empty()) {
        اگر (attr == VAttrType::VAR_PUBLIC) {
            اگر (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } وگرنه {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(درست);
            }
        } وگرنه {
            fl->v3error("missing -var");
        }
    } وگرنه {
        اگر (attr == VAttrType::VAR_FORCEABLE) {
            اگر (module.empty()) {
                fl->v3error("forceable missing -module");
            } وگرنه اگر (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } وگرنه {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } وگرنه {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                اگر (ftask.empty()) {
                    اگر (kind == VarSpecKind::PARAM) بازگشت mod.params().at(pattern);
                    اگر (kind == VarSpecKind::PORT) بازگشت mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    بازگشت mod.vars().at(pattern);
                }
                اگر (kind == VarSpecKind::PARAM) بازگشت mod.ftasks().at(ftask).params().at(pattern);
                اگر (kind == VarSpecKind::PORT) بازگشت mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                بازگشت mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

تهی V3Control::applyCase(AstCase* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    اگر (filep) filep->applyCase(nodep);
}

تهی V3Control::applyCoverageBlock(Ast_گره_Module* modulep, AstBegin* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    اگر (filep) filep->applyBlock(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    اگر (modp) modp->applyBlock(nodep);
}
تهی V3Control::applyCoverageBlock(Ast_گره_Module* modulep, AstGenBlock* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    اگر (filep) filep->applyBlock(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    اگر (modp) modp->applyBlock(nodep);
}

تهی V3Control::applyIgnores(File_سطر* filelinep) {
    ثابت string& filename = filelinep->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    اگر (filep) filep->applyIgnores(filelinep);
}

تهی V3Control::applyModule(Ast_گره_Module* modulep) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    اگر (modp) modp->apply(modulep);
}

تهی V3Control::applyFTask(Ast_گره_Module* modulep, Ast_گره_FTask* ftaskp) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    اگر (!modp) بازگشت;
    ثابت V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    اگر (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
ایستا تهی resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ثابت std::string name = varp->prettyDehashOrigOrName();
    اگر (ثابت V3ControlVar* ثابت vp = resolver.vars().resolve(name)) vp->apply(varp);
    اگر (varp->isParam()) {
        اگر (ثابت V3ControlVar* ثابت vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    اگر (varp->isIO()) {
        اگر (ثابت V3ControlVar* ثابت vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

تهی V3Control::applyVarAttr(ثابت Ast_گره_Module* modulep, ثابت Ast_گره_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ثابت modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    اگر (!modp) بازگشت;
    اگر (ftaskp) {
        V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        اگر (!ftp) بازگشت;
        resolveThenApply(*ftp, varp);
    } وگرنه {
        resolveThenApply(*modp, varp);
    }
}

صحیح V3Control::getHierWorkers(ثابت string& model) {
    بازگشت V3ControlResolver::s().getHierWorkers(model);
}
File_سطر* V3Control::get_Hier_Workers_File_سطر(ثابت string& model) {
    بازگشت V3ControlResolver::s().get_Hier_Workers_File_سطر(model);
}
ثابت V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(ثابت string& module) {
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(module);
    بازگشت modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(ثابت string& hierDpi) {
    بازگشت V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(ثابت string& model, ثابت string& کلید) {
    بازگشت V3ControlResolver::s().getProfileData(model, کلید);
}
File_سطر* V3Control::get_Profile_Data_File_سطر() {
    بازگشت V3ControlResolver::s().get_Profile_Data_File_سطر();
}
منطقی V3Control::getScopeTraceOn(ثابت string& scope) {
    بازگشت V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

تهی V3Control::contents_افزودن_Text(ثابت string& text) { WildcardContents::افزودن_Text(text); }

منطقی V3Control::containsMTaskProfileData() {
    بازگشت V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    بازگشت V3ControlResolver::s().getCurrentHierBlockCost();
}

منطقی V3Control::waive(ثابت File_سطر* filelinep, V3ErrorCode code, ثابت string& message) {
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    اگر (!filep) بازگشت نادرست;
    بازگشت filep->waive(code, message);
}

تهی V3Control::selfTest() { VInterval_درخت::selfTest(); }
