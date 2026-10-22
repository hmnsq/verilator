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
سانچہ <قسم_نام T>
جماعت V3ControlWildcardResolver final {
    تغیرپذیر V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ثابت std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ثابت std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

عوامی:
    V3ControlWildcardResolver() = طےشدہ;
    ~V3ControlWildcardResolver() = طےشدہ;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    خالی update(ثابت V3ControlWildcardResolver& other) VL_MT_محفوظ_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        ثابت V3_تالا_Guard other_تالا{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        برائے (ثابت خودکار& itr : other.m_mapPatterns) m_mapPatterns[itr.پہلا].update(itr.دوسرا);
    }

    // Access and create a pattern entry
    T& at(ثابت string& name) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        واپس m_mapPatterns[V3Os::filename_Slash_راستہ(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ثابت string& name) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_تالا_Guard تالا_2{m_mutex};
        // Lookup if it was resolved before, typically not
        ثابت خودکار pair = m_mapResolved.emplace(name, خالی_اشاریہ);
        std::unique_ptr<T>& entryr = pair.پہلا->دوسرا;
        // Resolve entry when first requested, cache the result
        اگر (pair.دوسرا) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ثابت string slashName = V3Os::filename_Slash_راستہ(name);
            برائے (ثابت خودکار& patEnt : m_mapPatterns) {
                اگر (VString::wildmatch(slashName, patEnt.پہلا)) {
                    اگر (!entryr) entryr.reset(نیا T{});
                    entryr->update(patEnt.دوسرا);
                }
            }
            // Perform final actions that needed all updates completed
            اگر (entryr) entryr->updateFinalize();
        }
        واپس entryr.get();
    }
};

// List of attributes for variables
جماعت V3ControlVar final {
    std::سمتیہ<VAttrType> m_attrs;  // The list of attributes
عوامی:
    // Add new attribugte
    خالی جوڑو(VAttrType attr) { m_attrs.emplace_عقب(attr); }
    // Update from other by copying all attributes
    خالی update(ثابت V3ControlVar& other) {
        m_attrs.reserve(m_attrs.سائز() + other.m_attrs.سائز());
        m_attrs.داخل_کرو(m_attrs.انتہا(), other.m_attrs.begin(), other.m_attrs.انتہا());
    }
    خالی updateFinalize() {}
    // Apply all attributes to the variable
    خالی apply(AstVar* varp) ثابت {
        برائے (ثابت VAttrType attr : m_attrs) {
            varp->جوڑو_Attrsp(نیا Ast_Attr_کا{varp->fileline(), attr});
        }
    }
};

استعمال V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

جماعت WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ثابت std::string, منطقی> m_mapPatterns;  // Pattern match results
    std::deque<string> m_سطریں;  // Source text lines

    // METHODS
    ساکن WildcardContents& s() {  // Singleton
        ساکن WildcardContents s_s;
        واپس s_s;
    }
    خالی clearCacheImp() { m_mapPatterns.clear(); }
    خالی ڈالو_متن_Imp(ثابت string& متن) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ثابت string leftover = m_سطریں.عقب() + متن;
        m_سطریں.نکالو_عقب();

        // Insert line-by-line
        string::سائز_type سطر_آغاز = 0;
        جب (سچ) {
            ثابت string::سائز_type سطر_انتہا = leftover.ڈھونڈو('\n', سطر_آغاز);
            اگر (سطر_انتہا != string::npos) {
                ثابت string oneline(leftover, سطر_آغاز, سطر_انتہا - سطر_آغاز + 1);
                اگر (oneline.سائز() > 1) m_سطریں.ڈالو_عقب(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_سطریں.سائز() - 1) << "]: " << oneline);
                سطر_آغاز = سطر_انتہا + 1;
            } ورنہ {
                توڑو;
            }
        }
        // Keep leftover for next time
        m_سطریں.emplace_عقب(string(leftover, سطر_آغاز));  // Might be ""
        clearCacheImp();
    }

    منطقی resolveUncachedImp(ثابت string& contentsRegexp) {
        برائے (ثابت string& i : m_سطریں) {
            اگر (VString::wildmatch(i, contentsRegexp)) واپس سچ;
        }
        واپس جھوٹ;
    }
    منطقی resolveCachedImp(ثابت string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        اگر (contentsRegexp.خالی_خانے()) واپس سچ;
        ثابت خودکار pair = m_mapPatterns.emplace(contentsRegexp, جھوٹ);
        منطقی& entryr = pair.پہلا->دوسرا;
        // Resolve entry when first requested, cache the result
        اگر (pair.دوسرا) entryr = resolveUncachedImp(contentsRegexp);
        واپس entryr;
    }

عوامی:
    WildcardContents() {
        m_سطریں.emplace_عقب("");  // start with no leftover
    }
    ~WildcardContents() = طےشدہ;
    // Return true iff contentsRegexp in parsed contents
    ساکن منطقی resolve(ثابت string& contentsRegexp) {
        واپس s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ساکن خالی ڈالو_متن(ثابت string& متن) { s().ڈالو_متن_Imp(متن); }
};

//######################################################################
// Function or task: Have variables and properties

جماعت V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    منطقی m_noinline = جھوٹ;  // Don't inline function/task
    منطقی m_عوامی = جھوٹ;  // Public function/task

عوامی:
    V3ControlFTask() = طےشدہ;
    خالی update(ثابت V3ControlFTask& f) {
        // Don't overwrite true with false
        اگر (f.m_noinline) m_noinline = سچ;
        اگر (f.m_عوامی) m_عوامی = سچ;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    خالی updateFinalize() {}

    V3ControlVarResolver& params() { واپس m_params; }
    V3ControlVarResolver& ports() { واپس m_ports; }
    V3ControlVarResolver& vars() { واپس m_vars; }

    خالی سیٹ_No_Inline(منطقی سیٹ) { m_noinline = سیٹ; }
    خالی سیٹ_عوامی(منطقی سیٹ) { m_عوامی = سیٹ; }

    خالی apply(Ast_گرہ_FTask* ftaskp) ثابت {
        اگر (m_noinline)
            ftaskp->جوڑو_Stmtsp(نیا AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        اگر (m_عوامی)
            ftaskp->جوڑو_Stmtsp(نیا AstPragma{ftaskp->fileline(), VPragmaType::عوامی_TASK});
    }
};

استعمال V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

جماعت V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_رجسٹر_Wrapper m_fsm_رجسٹر_Wrapper;  // FSM wrapper descriptor
    std::unordered_سیٹ<std::string> m_coverage_Off_بلاکیں;  // List of block names for coverage_off
    std::سیٹ<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    منطقی m_has_Fsm_رجسٹر_Wrapper = جھوٹ;  // Whether descriptor has been set
    منطقی m_inline = جھوٹ;  // Whether to force the inline
    منطقی m_inline_قدر = جھوٹ;  // The inline value (on/off)

عوامی:
    V3ControlModule() = طےشدہ;

    خالی update(ثابت V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        اگر (m.m_has_Fsm_رجسٹر_Wrapper) {
            m_fsm_رجسٹر_Wrapper = m.m_fsm_رجسٹر_Wrapper;
            m_has_Fsm_رجسٹر_Wrapper = سچ;
        }
        برائے (ثابت string& i : m.m_coverage_Off_بلاکیں) m_coverage_Off_بلاکیں.داخل_کرو(i);
        اگر (!m_inline) {
            m_inline = m.m_inline;
            m_inline_قدر = m.m_inline_قدر;
        }
        برائے (خودکار it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.داخل_کرو(*it);
        }
    }
    خالی updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { واپس m_tasks; }
    V3ControlVarResolver& params() { واپس m_params; }
    V3ControlVarResolver& ports() { واپس m_ports; }
    V3ControlVarResolver& vars() { واپس m_vars; }

    خالی جوڑو_Coverage_بلاک_Off(ثابت string& name) { m_coverage_Off_بلاکیں.داخل_کرو(name); }
    خالی سیٹ_Inline(منطقی سیٹ) {
        m_inline = سچ;
        m_inline_قدر = سیٹ;
    }
    خالی سیٹ_Fsm_رجسٹر_Wrapper(File_سطر* fl, ثابت V3Control::Fsm_رجسٹر_Wrapper& desc) {
        اگر (m_has_Fsm_رجسٹر_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_گرہ::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_رجسٹر_Wrapper = desc;
        m_has_Fsm_رجسٹر_Wrapper = سچ;
    }
    ثابت V3Control::Fsm_رجسٹر_Wrapper* fsm_رجسٹر_Wrapperp() ثابت {
        واپس m_has_Fsm_رجسٹر_Wrapper ? &m_fsm_رجسٹر_Wrapper : خالی_اشاریہ;
    }
    خالی جوڑو_Module_Pragma(VPragmaType pragma) { m_modPragmas.داخل_کرو(pragma); }

    خالی apply(Ast_گرہ_Module* modp) {
        اگر (m_inline) {
            ثابت VPragmaType type
                = m_inline_قدر ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_گرہ* ثابت nodep = نیا AstPragma{modp->fileline(), type};
            modp->جوڑو_Stmtsp(nodep);
        }
        برائے (ثابت خودکار& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            اگر (itr == VPragmaType::HIER_PARAMS) modp->hierParams(سچ);
            Ast_گرہ* ثابت nodep = نیا AstPragma{modp->fileline(), itr};
            modp->جوڑو_Stmtsp(nodep);
        }
    }

    خالی apply_بلاک(Ast_Gen_بلاک* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        اگر (!nodep->unnamed()) {
            برائے (ثابت string& i : m_coverage_Off_بلاکیں) {
                اگر (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جوڑو_Itemsp(نیا AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    خالی apply_بلاک(Ast_گرہ_بلاک* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        File_سطر* ثابت flp = nodep->fileline();
        اگر (!nodep->unnamed()) {
            برائے (ثابت string& i : m_coverage_Off_بلاکیں) {
                اگر (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جوڑو_Stmtsp(نیا AstStmtPragma{flp, نیا AstPragma{flp, pragma}});
                }
            }
        }
    }
};

استعمال V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
جماعت V3_Control_Ignores_سطر final {
عوامی:
    ثابت صحیح m_سطر_Min;  // Minimum line number to make change at
    ثابت صحیح m_سطر_Max;  // Maximum line number to make change at (inclusive)
    ثابت V3_Error_رمز m_رمز;  // Error code
    ثابت منطقی m_on;  // True to enable message
    V3_Control_Ignores_سطر(V3_Error_رمز رمز, صحیح linemin, صحیح linemax, منطقی on)
        : m_سطر_Min{linemin}
        , m_سطر_Max{linemax}
        , m_رمز{رمز}
        , m_on{on} {}
    ~V3_Control_Ignores_سطر() = طےشدہ;
    منطقی every_سطر() ثابت { واپس m_سطر_Min == 0 && m_سطر_Max == 0; }
    منطقی اندر_Range(صحیح lineno) ثابت {
        واپس (lineno >= m_سطر_Min && lineno <= m_سطر_Max) || every_سطر();
    }
};
std::ostream& عامل<<(std::ostream& os, ثابت V3_Control_Ignores_سطر& rhs) {
    واپس os << rhs.m_سطر_Min << "-" << rhs.m_سطر_Max << ", " << rhs.m_رمز << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ساکن std::سمتیہ<V3_Control_Ignores_سطر> control_Ign_سطریں;

استعمال IgnIndices = std::سمتیہ<uint32_t>;  // List of {s_ignLines indces}

جماعت VInterval_درخت final {
    ساخت اندراج_3 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        صحیح m_center;  // Line number of central point
        std::unique_ptr<اندراج_3> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<اندراج_3> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        صریح اندراج_3(صحیح center)
            : m_center{center} {}
    };

    std::unique_ptr<اندراج_3> m_rootp;  // Root of interval tree
    IgnIndices m_every_سطریں;  // All-line disables

نجی:
    std::unique_ptr<اندراج_3> build_درخت(ثابت IgnIndices& points) {
        اگر (points.خالی_خانے()) واپس خالی_اشاریہ;

        صحیح minval = std::numeric_limits<صحیح>::max();
        صحیح maxval = std::numeric_limits<صحیح>::min();
        برائے (ثابت خودکار& it : points) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
            اگر (cign.every_سطر()) {
                m_every_سطریں.emplace_عقب(it);
                جاری;
            }
            minval = std::min(minval, cign.m_سطر_Min);
            maxval = std::max(maxval, cign.m_سطر_Max);
        }

        ثابت صحیح center = minval + (maxval - minval) / 2;
        خودکار entp = std::unique_ptr<اندراج_3>(نیا اندراج_3{center});

        IgnIndices بایاں_Points;
        IgnIndices دایاں_Points;
        برائے (ثابت خودکار& it : points) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
            اگر (cign.every_سطر()) جاری;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            اگر (cign.m_سطر_Max < center)
                بایاں_Points.emplace_عقب(it);
            ورنہ اگر (cign.m_سطر_Min > center)
                دایاں_Points.emplace_عقب(it);
            ورنہ {
                entp->m_byMin.ڈالو_عقب(it);
                entp->m_byMax.ڈالو_عقب(it);
            }
        }

        std::ترتیب_دو(entp->m_byMin.begin(), entp->m_byMin.انتہا(), [](uint32_t a, uint32_t b) {
            واپس control_Ign_سطریں[a].m_سطر_Min < control_Ign_سطریں[b].m_سطر_Min;
        });
        std::ترتیب_دو(entp->m_byMax.begin(), entp->m_byMax.انتہا(), [](uint32_t a, uint32_t b) {
            واپس control_Ign_سطریں[a].m_سطر_Max > control_Ign_سطریں[b].m_سطر_Max;
        });

        entp->m_leftp = build_درخت(بایاں_Points);
        entp->m_rightp = build_درخت(دایاں_Points);
        واپس entp;
    }

    خالی ڈھونڈو_درخت(صحیح lineno, اندراج_3* entp, IgnIndices& resultsr, صحیح& اگلا_Changer) ثابت {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        اگر (lineno < entp->m_center) {
            اگلا_Changer = std::min(اگلا_Changer, entp->m_center);
            برائے (ثابت خودکار& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
                اگر (cign.m_سطر_Min <= lineno) {
                    resultsr.emplace_عقب(it);
                } ورنہ {
                    اگلا_Changer = std::min(اگلا_Changer, cign.m_سطر_Min);
                    توڑو;
                }
            }
            اگر (entp->m_leftp) ڈھونڈو_درخت(lineno, entp->m_leftp.get(), resultsr, اگلا_Changer);
        } ورنہ اگر (lineno > entp->m_center) {
            برائے (ثابت خودکار& it : entp->m_byMax) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
                اگر (cign.m_سطر_Max >= lineno) {
                    اگلا_Changer = std::min(اگلا_Changer, cign.m_سطر_Max + 1);
                    resultsr.emplace_عقب(it);
                } ورنہ {
                    توڑو;
                }
            }
            اگر (entp->m_rightp) ڈھونڈو_درخت(lineno, entp->m_rightp.get(), resultsr, اگلا_Changer);
        } ورنہ {
            برائے (ثابت خودکار& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
                resultsr.emplace_عقب(it);
                اگر (cign.m_سطر_Max >= lineno)
                    اگلا_Changer = std::min(اگلا_Changer, cign.m_سطر_Max + 1);
            }
            اگر (entp->m_leftp) ڈھونڈو_درخت(lineno, entp->m_leftp.get(), resultsr, اگلا_Changer);
            اگر (entp->m_rightp) ڈھونڈو_درخت(lineno, entp->m_rightp.get(), resultsr, اگلا_Changer);
        }
    }

عوامی:
    // CONSTRUCTORS
    VInterval_درخت() = طےشدہ;
    ~VInterval_درخت() = طےشدہ;
    // METHODS
    خالی clear() { m_rootp = خالی_اشاریہ; }
    خالی build(ثابت IgnIndices& points) {
        clear();
        m_rootp = build_درخت(points);
    }
    خالی ڈھونڈو(صحیح lineno, IgnIndices& resultsr, صحیح& اگلا_Changer) ثابت {
        resultsr = m_every_سطریں;
        اگلا_Changer = std::numeric_limits<صحیح>::max();

        اگر (m_rootp) ڈھونڈو_درخت(lineno, m_rootp.get(), resultsr, اگلا_Changer);
        // Sort indices, so can process in parse order
        std::ترتیب_دو(resultsr.begin(), resultsr.انتہا());
    }

    ساکن خالی selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices ڈیٹا;
        std::سمتیہ<std::pair<صحیح, صحیح>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        برائے (ثابت خودکار& it : points) {
            control_Ign_سطریں.emplace_عقب(
                V3_Control_Ignores_سطر{V3_Error_رمز::I_LINT, it.پہلا, it.دوسرا, سچ});
            ڈیٹا.emplace_عقب(static_cast<uint32_t>(control_Ign_سطریں.سائز() - 1));
        }
        VInterval_درخت درخت;
        درخت.build(ڈیٹا);

        IgnIndices results;
        صحیح اگلا_Change = 0;
        درخت.ڈھونڈو(0, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 0);
        UASSERT_SELFTEST(اگلا_Change, 10);
        درخت.ڈھونڈو(10, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(اگلا_Change, 11);
        درخت.ڈھونڈو(11, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(اگلا_Change, 15);  // Center, or would be 20
        درخت.ڈھونڈو(20, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(اگلا_Change, 21);
        درخت.ڈھونڈو(21, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(اگلا_Change, 25);  // Center, or would be 30
        درخت.ڈھونڈو(30, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(اگلا_Change, 31);
        درخت.ڈھونڈو(40, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(اگلا_Change, 41);
        درخت.ڈھونڈو(41, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 0);
        UASSERT_SELFTEST(اگلا_Change, std::numeric_limits<صحیح>::max());
        //
        points = {{0, 0}};
        برائے (ثابت خودکار& it : points) {
            control_Ign_سطریں.emplace_عقب(
                V3_Control_Ignores_سطر{V3_Error_رمز::I_LINT, it.پہلا, it.دوسرا, سچ});
            ڈیٹا.emplace_عقب(static_cast<uint32_t>(control_Ign_سطریں.سائز() - 1));
        }
        درخت.build(ڈیٹا);
        //
        درخت.ڈھونڈو(50, results, اگلا_Change);
        UASSERT_SELFTEST(results.سائز(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
استعمال V3_Control_سطر_Attribute = std::bitset<VPragmaType::ENUM_سائز>;

جماعت WaiverSetting final {
عوامی:
    V3_Error_رمز m_رمز;  // Error code
    string m_contents;  // --contents regexp
    string m_مطابقت;  // --match regexp
    WaiverSetting(V3_Error_رمز رمز, ثابت string& contents, ثابت string& مطابقت)
        : m_رمز{رمز}
        , m_contents{contents}
        , m_مطابقت{مطابقت} {}
    ~WaiverSetting() = طےشدہ;
    WaiverSetting& عامل=(ثابت WaiverSetting& rhs) = طےشدہ;
};

// File entity
جماعت V3ControlFile final {
    استعمال سطر_Attr_Map = std::map<صحیح, V3_Control_سطر_Attribute>;  // Map line->bitset of attributes
    استعمال Waivers = std::سمتیہ<WaiverSetting>;  // List of {code,wildcard string}

    سطر_Attr_Map m_سطر_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_درخت m_interval_درخت;  // Tree of indices to IgnLines

    ساخت {
        صحیح filenameno = -1;  // Last filename
        صحیح lineno = -1;  // Last linenumber
        صحیح اگلا_Change = -1;  // Line number of next change
    } m_آخری_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    منطقی سطر_مطابقت(صحیح lineno, VPragmaType type) {
        اگر (m_سطر_Attrs.ڈھونڈو(0) != m_سطر_Attrs.انتہا() && m_سطر_Attrs[0][type]) واپس سچ;
        اگر (m_سطر_Attrs.ڈھونڈو(lineno) == m_سطر_Attrs.انتہا()) واپس جھوٹ;
        واپس m_سطر_Attrs[lineno][type];
    }

عوامی:
    V3ControlFile() = طےشدہ;
    خالی update(ثابت V3ControlFile& file) {
        // Copy in all attributes and waivers
        برائے (ثابت خودکار& itr : file.m_سطر_Attrs) m_سطر_Attrs[itr.پہلا] |= itr.دوسرا;
        m_waivers.reserve(m_waivers.سائز() + file.m_waivers.سائز());
        m_waivers.داخل_کرو(m_waivers.انتہا(), file.m_waivers.begin(), file.m_waivers.انتہا());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.سائز() + file.m_ignIndices.سائز());
        m_ignIndices.داخل_کرو(m_ignIndices.انتہا(), file.m_ignIndices.begin(),
                            file.m_ignIndices.انتہا());
        // updateFinalize() will soon build tree
    }
    خالی updateFinalize() { m_interval_درخت.build(m_ignIndices); }
    خالی جوڑو_سطر_Attribute(صحیح lineno, VPragmaType attr) { m_سطر_Attrs[lineno].سیٹ(attr); }
    خالی جوڑو_Ignore(V3_Error_رمز رمز, صحیح min, صحیح max, منطقی on) {
        control_Ign_سطریں.emplace_عقب(V3_Control_Ignores_سطر{رمز, min, max, on});
        m_ignIndices.emplace_عقب(static_cast<uint32_t>(control_Ign_سطریں.سائز() - 1));
        m_آخری_Ignore.اگلا_Change = -1;
    }
    خالی جوڑو_Ignore_مطابقت(V3_Error_رمز رمز, ثابت string& contents, ثابت string& مطابقت) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_مطابقت = مطابقت;
        اگر (new_مطابقت.خالی_خانے() || new_مطابقت.عقب() != '*') new_مطابقت += '*';
        m_waivers.emplace_عقب(رمز, contents, new_مطابقت);
    }

    خالی apply_بلاک(Ast_Gen_بلاک* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        اگر (سطر_مطابقت(nodep->fileline()->lineno(), pragma)) {
            nodep->جوڑو_Itemsp(نیا AstPragma{nodep->fileline(), pragma});
        }
    }
    خالی apply_بلاک(Ast_گرہ_بلاک* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        File_سطر* ثابت flp = nodep->fileline();
        اگر (سطر_مطابقت(flp->lineno(), pragma)) {
            nodep->جوڑو_Stmtsp(نیا AstStmtPragma{flp, نیا AstPragma{flp, pragma}});
        }
    }
    خالی applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ثابت صحیح lineno = nodep->fileline()->lineno();
        اگر (سطر_مطابقت(lineno, VPragmaType::بھرا_CASE)) nodep->بھرا_Pragma(سچ);
        اگر (سطر_مطابقت(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(سچ);
    }
    خالی applyIgnores(File_سطر* filelinep) {
        // HOT routine, called each parsed token line of this filename
        اگر (filelinep->filenameno() == m_آخری_Ignore.filenameno
            && filelinep->lineno() == m_آخری_Ignore.lineno)
            واپس;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        اگر (filelinep->filenameno() == m_آخری_Ignore.filenameno
            && filelinep->lineno() < m_آخری_Ignore.اگلا_Change) {
            m_آخری_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            واپس;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_آخری_Ignore.filenameno = filelinep->filenameno();
        m_آخری_Ignore.lineno = filelinep->lineno();
        m_آخری_Ignore.اگلا_Change = std::numeric_limits<صحیح>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_درخت.ڈھونڈو(filelinep->lineno(), results /*ref*/, m_آخری_Ignore.اگلا_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_ثنائی_ہندسہ_سیٹ bitset{VError_ثنائی_ہندسہ_سیٹ::AllOnes{}};
        برائے (ثابت خودکار& it : results) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
            UASSERT(cign.اندر_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_رمز.forDelegateCodes(
                [&](V3_Error_رمز subcode) { bitset.سیٹ(subcode, cign.m_on); });
        }

        filelinep->warn_سیٹ_Ctrl_ثنائی_ہندسہ_سیٹ(bitset);
        m_آخری_Ignore.اگلا_Change = std::max(m_آخری_Ignore.اگلا_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_آخری_Ignore.اگلا_Change
                                       << " Messages: " << bitset.ascii());
    }
    منطقی waive(V3_Error_رمز رمز, ثابت string& پیغام) {
        اگر (رمز.hardError()) واپس جھوٹ;
        برائے (ثابت خودکار& itr : m_waivers) {
            اگر ((رمز.isUnder(itr.m_رمز) || (itr.m_رمز == V3_Error_رمز::I_LINT))
                && VString::wildmatch(پیغام, itr.m_مطابقت)
                && WildcardContents::resolve(itr.m_contents)) {
                واپس سچ;
            }
        }
        واپس جھوٹ;
    }
};

استعمال V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

جماعت V3_Control_Scope_Trace_اندراج final {
عوامی:
    ثابت string m_scope;  // Scope or regexp to match
    ثابت منطقی m_on;  // True to enable message
    صحیح m_سطحیں = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_اندراج(ثابت string& scope, منطقی on, صحیح سطحیں)
        : m_scope{scope}
        , m_on{on}
        , m_سطحیں{سطحیں} {}
    ~V3_Control_Scope_Trace_اندراج() = طےشدہ;
    منطقی عامل<(ثابت V3_Control_Scope_Trace_اندراج& other) ثابت {
        اگر (m_on < other.m_on) واپس سچ;
        اگر (m_on > other.m_on) واپس جھوٹ;
        اگر (m_سطحیں < other.m_سطحیں) واپس سچ;
        اگر (m_سطحیں > other.m_سطحیں) واپس جھوٹ;
        واپس m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
جماعت V3_Control_Scope_Trace_اندراج_مطابقت final {
عوامی:
    ثابت V3_Control_Scope_Trace_اندراج* m_entryp;  // Entry the match refers to
    ثابت string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_اندراج_مطابقت(ثابت V3_Control_Scope_Trace_اندراج* entryp, ثابت string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    منطقی عامل<(ثابت V3_Control_Scope_Trace_اندراج_مطابقت& other) ثابت {
        اگر (m_entryp < other.m_entryp) واپس سچ;
        اگر (m_entryp > other.m_entryp) واپس جھوٹ;
        واپس m_scopepart < other.m_scopepart;
    }
};

جماعت V3ControlScopeTraceResolver final {
    std::سمتیہ<V3_Control_Scope_Trace_اندراج> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_اندراج_مطابقت, منطقی> m_مطابقت_Cache;  // Matching entries for speed

عوامی:
    خالی جوڑو_Scope_Trace_On(منطقی on, ثابت string& scope, صحیح سطحیں) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << سطحیں);
        m_entries.emplace_عقب(V3_Control_Scope_Trace_اندراج{scope, on, سطحیں});
        m_مطابقت_Cache.clear();
    }

    منطقی get_اندراج_مطابقت(ثابت V3_Control_Scope_Trace_اندراج* entp, ثابت string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ثابت V3_Control_Scope_Trace_اندراج_مطابقت کلید{entp, scopepart};
        ثابت خودکار pair = m_مطابقت_Cache.emplace(کلید, جھوٹ);
        اگر (pair.دوسرا) pair.پہلا->دوسرا = VString::wildmatch(scopepart, entp->m_scope);
        واپس pair.پہلا->دوسرا;
    }

    منطقی getScopeTraceOn(ثابت string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        صحیح maxLevel = 1;
        برائے (ثابت خودکار& ch : scope) {
            اگر (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        منطقی enabled = سچ;
        برائے (ثابت خودکار& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            صحیح partLevel = 1;
            برائے (string::سائز_type part_انتہا = 0; سچ;) {
                part_انتہا = scope.ڈھونڈو('.', part_انتہا + 1);
                اگر (part_انتہا == string::npos) part_انتہا = scope.لمبائی_2();
                ثابت std::string scopepart = scope.substr(0, part_انتہا);
                اگر (get_اندراج_مطابقت(&ent, scopepart)) {
                    ثابت منطقی level_مطابقت
                        = !ent.m_سطحیں || (ent.m_سطحیں >= maxLevel - partLevel);
                    اگر (level_مطابقت) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_مطابقت ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    توڑو;
                }
                اگر (part_انتہا == scope.لمبائی_2()) توڑو;
                ++partLevel;
            }
        }
        واپس enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

جماعت V3_Control_Resolver_Hier_کارکن_اندراج final {
    ثابت صحیح m_workers;  // Number of workers requested for the hierarchical block
    File_سطر* ثابت m_flp;  // Location the worker count was specified at

عوامی:
    صریح V3_Control_Resolver_Hier_کارکن_اندراج(صحیح workers, File_سطر* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    صحیح workers() ثابت { واپس m_workers; }
    File_سطر* flp() ثابت { واپس m_flp; }
};

جماعت V3ControlResolver final {
    شمار Profile_ڈیٹا_منوال : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_ڈیٹا;  // Access to profile_data records
    uint8_t m_منوال = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_کارکن_اندراج> m_hierWorkers;
    File_سطر* m_profile_File_سطر = خالی_اشاریہ;  // Location profile_data was read from

    V3ControlResolver() = طےشدہ;
    ~V3ControlResolver() = طےشدہ;

عوامی:
    ساکن V3ControlResolver& s() {
        ساکن V3ControlResolver s_singleton;
        واپس s_singleton;
    }
    V3ControlModuleResolver& modules() { واپس m_modules; }
    V3ControlFileResolver& files() { واپس m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { واپس m_scopeTraces; }

    خالی جوڑو_Profile_ڈیٹا(File_سطر* fl, ثابت string& hierDpi, uint64_t لاگت) {
        // Empty key for hierarchical DPI wrapper costs.
        جوڑو_Profile_ڈیٹا(fl, hierDpi, "", لاگت, HIER_DPI);
    }
    خالی جوڑو_Profile_ڈیٹا(File_سطر* fl, ثابت string& model, ثابت string& کلید, uint64_t لاگت,
                        Profile_ڈیٹا_منوال منوال = MTASK) {
        اگر (!m_profile_File_سطر) m_profile_File_سطر = fl;
        اگر (لاگت == 0) لاگت = 1;  // Cost 0 means delete (or no data)
        اگر (منوال == MTASK) {
            m_profile_ڈیٹا[model][کلید] += لاگت;
        } ورنہ اگر (منوال == HIER_DPI) {
            m_profile_ڈیٹا[model][کلید] = std::max(m_profile_ڈیٹا[model][کلید], لاگت);
        }
        m_منوال |= منوال;
    }
    منطقی شامل_ہے_MTask_Profile_ڈیٹا() ثابت { واپس m_منوال & MTASK; }
    uint64_t get_Profile_ڈیٹا(ثابت string& hierDpi) ثابت {
        // Empty key for hierarchical DPI wrapper costs.
        واپس get_Profile_ڈیٹا(hierDpi, "");
    }
    خالی جوڑو_Hier_Workers(File_سطر* flp, ثابت string& model, صحیح workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    صحیح getHierWorkers(ثابت string& model) ثابت {
        ثابت خودکار mit = m_hierWorkers.ڈھونڈو(model);
        // Assign a single worker if no specified.
        واپس mit != m_hierWorkers.cend() ? mit->دوسرا.workers() : 0;
    }
    File_سطر* get_Hier_Workers_File_سطر(ثابت string& model) ثابت {
        ثابت خودکار mit = m_hierWorkers.ڈھونڈو(model);
        واپس mit != m_hierWorkers.cend() ? mit->دوسرا.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_ڈیٹا(ثابت string& model, ثابت string& کلید) ثابت {
        ثابت خودکار mit = m_profile_ڈیٹا.ڈھونڈو(model);
        اگر (mit == m_profile_ڈیٹا.cend()) واپس 0;
        ثابت خودکار it = mit->دوسرا.ڈھونڈو(کلید);
        اگر (it == mit->دوسرا.cend()) واپس 0;
        واپس it->دوسرا;
    }
    File_سطر* get_Profile_ڈیٹا_File_سطر() ثابت { واپس m_profile_File_سطر; }  // Maybe null
    ساکن uint64_t get_Current_Hier_بلاک_لاگت() {
        اگر (uint64_t لاگت = V3Control::get_Profile_ڈیٹا(v3Global.opt.سابقہ())) {
            UINFO(9, "Fetching cost from profile info: " << لاگت);
            واپس لاگت;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ثابت netlistp = v3Global.rootp();
        uint64_t لاگت = 0;
        برائے (صحیح i = 0; i < VEval::ENUM_انتہا; ++i) {
            ثابت VEval eval{i};
            اگر (eval.slow()) جاری;
            لاگت += V3_Instr_گنتی::گنتی(netlistp->evalFuncp(eval), جھوٹ);
        }
        UINFO(9, "Evaluating cost: " << لاگت);
        واپس لاگت;
    }
};

//######################################################################
// V3Control

خالی V3Control::جوڑو_Case_بھرا(ثابت string& filename, صحیح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڑو_سطر_Attribute(lineno, VPragmaType::بھرا_CASE);
}
خالی V3Control::جوڑو_Case_Parallel(ثابت string& filename, صحیح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڑو_سطر_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
خالی V3Control::جوڑو_Coverage_بلاک_Off(ثابت string& filename, صحیح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڑو_سطر_Attribute(lineno, VPragmaType::COVERAGE_بلاک_OFF);
}
خالی V3Control::جوڑو_Coverage_بلاک_Off(ثابت string& module, ثابت string& blockname) {
    V3ControlResolver::s().modules().at(module).جوڑو_Coverage_بلاک_Off(blockname);
}

خالی V3Control::جوڑو_Hier_Workers(File_سطر* fl, ثابت string& model, صحیح workers) {
    V3ControlResolver::s().جوڑو_Hier_Workers(fl, model, workers);
}

خالی V3Control::جوڑو_Fsm_رجسٹر_Wrapper(File_سطر* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_قدر) {
    string missing;
    اگر (module.خالی_خانے()) missing += "-module";
    اگر (d.خالی_خانے()) missing = VString::dot(missing, ", ", "-d");
    اگر (q.خالی_خانے()) missing = VString::dot(missing, ", ", "-q");
    اگر (clock.خالی_خانے()) missing = VString::dot(missing, ", ", "-clock");
    اگر (!missing.خالی_خانے()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        واپس;
    }

    ثابت Fsm_رجسٹر_Wrapper desc{module, d, q, clock, reset, reset_قدر};
    V3ControlResolver::s().modules().at(module).سیٹ_Fsm_رجسٹر_Wrapper(fl, desc);
}

خالی V3Control::جوڑو_Ignore(V3_Error_رمز رمز, منطقی on, ثابت string& filename, صحیح min, صحیح max) {
    UINFO(9, "addIgnore " << رمز << " " << min << "-" << max << " fn=" << filename);
    اگر (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_سطر::globalWarnOff(رمز, !on);
    } ورنہ {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).جوڑو_Ignore(رمز, min, max, on);
    }
}
خالی V3Control::جوڑو_Ignore_مطابقت(V3_Error_رمز رمز, ثابت string& filename, ثابت string& contents,
                               ثابت string& مطابقت) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).جوڑو_Ignore_مطابقت(رمز, contents, مطابقت);
}

خالی V3Control::جوڑو_Inline(File_سطر* fl, ثابت string& module, ثابت string& ftask, منطقی on) {
    اگر (ftask.خالی_خانے()) {
        V3ControlResolver::s().modules().at(module).سیٹ_Inline(on);
    } ورنہ {
        اگر (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } ورنہ {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).سیٹ_No_Inline(on);
        }
    }
}

خالی V3Control::جوڑو_Module_Pragma(ثابت string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).جوڑو_Module_Pragma(pragma);
}

خالی V3Control::جوڑو_Profile_ڈیٹا(File_سطر* fl, ثابت string& hierDpi, uint64_t لاگت) {
    V3ControlResolver::s().جوڑو_Profile_ڈیٹا(fl, hierDpi, لاگت);
}
خالی V3Control::جوڑو_Profile_ڈیٹا(File_سطر* fl, ثابت string& model, ثابت string& کلید,
                               uint64_t لاگت) {
    V3ControlResolver::s().جوڑو_Profile_ڈیٹا(fl, model, کلید, لاگت);
}

خالی V3Control::جوڑو_Scope_Trace_On(منطقی on, ثابت string& scope, صحیح سطحیں) {
    V3ControlResolver::s().scopeTraces().جوڑو_Scope_Trace_On(on, scope, سطحیں);
}

خالی V3Control::جوڑو_Var_Attr(File_سطر* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_قسم قسم, ثابت string& نمونہ_2, VAttrType attr,
                           Ast_Sen_درخت* sensep) {
    اگر (sensep) {
        File_سطر* ثابت flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_درخت(), sensep);
        // Used to be only accepted on public_flat_rw
        اگر (attr != VAttrType::VAR_عوامی_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            واپس;
        }
    }

    اگر (قسم != Var_Spec_قسم::VAR) {
        انتخاب (attr) {
        حالت VAttrType::VAR_عوامی_FLAT:
        حالت VAttrType::VAR_عوامی_FLAT_RD:
        حالت VAttrType::VAR_عوامی_FLAT_RW: توڑو;
        طےشدہ:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            واپس;
        }
    }

    // Semantics: Most of the attributes operate on signals
    اگر (نمونہ_2.خالی_خانے()) {
        اگر (attr == VAttrType::VAR_عوامی) {
            اگر (ftask.خالی_خانے()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).جوڑو_Module_Pragma(
                    VPragmaType::عوامی_MODULE);
            } ورنہ {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).سیٹ_عوامی(سچ);
            }
        } ورنہ {
            fl->v3error("missing -var");
        }
    } ورنہ {
        اگر (attr == VAttrType::VAR_FORCEABLE) {
            اگر (module.خالی_خانے()) {
                fl->v3error("forceable missing -module");
            } ورنہ اگر (!ftask.خالی_خانے()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } ورنہ {
                V3ControlResolver::s().modules().at(module).vars().at(نمونہ_2).جوڑو(attr);
            }
        } ورنہ {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                اگر (ftask.خالی_خانے()) {
                    اگر (قسم == Var_Spec_قسم::PARAM) واپس mod.params().at(نمونہ_2);
                    اگر (قسم == Var_Spec_قسم::پورٹ) واپس mod.ports().at(نمونہ_2);
                    UASSERT_OBJ(قسم == Var_Spec_قسم::VAR, fl, "Unexpected VarSpecKind");
                    واپس mod.vars().at(نمونہ_2);
                }
                اگر (قسم == Var_Spec_قسم::PARAM) واپس mod.ftasks().at(ftask).params().at(نمونہ_2);
                اگر (قسم == Var_Spec_قسم::پورٹ) واپس mod.ftasks().at(ftask).ports().at(نمونہ_2);
                UASSERT_OBJ(قسم == Var_Spec_قسم::VAR, fl, "Unexpected VarSpecKind");
                واپس mod.ftasks().at(ftask).vars().at(نمونہ_2);
            }();
            controlVar.جوڑو(attr);
        }
    }
}

خالی V3Control::applyCase(AstCase* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    اگر (filep) filep->applyCase(nodep);
}

خالی V3Control::apply_Coverage_بلاک(Ast_گرہ_Module* modulep, AstBegin* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    اگر (filep) filep->apply_بلاک(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    اگر (modp) modp->apply_بلاک(nodep);
}
خالی V3Control::apply_Coverage_بلاک(Ast_گرہ_Module* modulep, Ast_Gen_بلاک* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    اگر (filep) filep->apply_بلاک(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    اگر (modp) modp->apply_بلاک(nodep);
}

خالی V3Control::applyIgnores(File_سطر* filelinep) {
    ثابت string& filename = filelinep->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    اگر (filep) filep->applyIgnores(filelinep);
}

خالی V3Control::applyModule(Ast_گرہ_Module* modulep) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    اگر (modp) modp->apply(modulep);
}

خالی V3Control::applyFTask(Ast_گرہ_Module* modulep, Ast_گرہ_FTask* ftaskp) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    اگر (!modp) واپس;
    ثابت V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    اگر (ftp) ftp->apply(ftaskp);
}

سانچہ <قسم_نام T_Resolver>
ساکن خالی resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ثابت std::string name = varp->prettyDehashOrigOrName();
    اگر (ثابت V3ControlVar* ثابت vp = resolver.vars().resolve(name)) vp->apply(varp);
    اگر (varp->isParam()) {
        اگر (ثابت V3ControlVar* ثابت vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    اگر (varp->isIO()) {
        اگر (ثابت V3ControlVar* ثابت vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

خالی V3Control::applyVarAttr(ثابت Ast_گرہ_Module* modulep, ثابت Ast_گرہ_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ثابت modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    اگر (!modp) واپس;
    اگر (ftaskp) {
        V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        اگر (!ftp) واپس;
        resolveThenApply(*ftp, varp);
    } ورنہ {
        resolveThenApply(*modp, varp);
    }
}

صحیح V3Control::getHierWorkers(ثابت string& model) {
    واپس V3ControlResolver::s().getHierWorkers(model);
}
File_سطر* V3Control::get_Hier_Workers_File_سطر(ثابت string& model) {
    واپس V3ControlResolver::s().get_Hier_Workers_File_سطر(model);
}
ثابت V3Control::Fsm_رجسٹر_Wrapper* V3Control::get_Fsm_رجسٹر_Wrapper(ثابت string& module) {
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(module);
    واپس modp ? modp->fsm_رجسٹر_Wrapperp() : خالی_اشاریہ;
}
uint64_t V3Control::get_Profile_ڈیٹا(ثابت string& hierDpi) {
    واپس V3ControlResolver::s().get_Profile_ڈیٹا(hierDpi);
}
uint64_t V3Control::get_Profile_ڈیٹا(ثابت string& model, ثابت string& کلید) {
    واپس V3ControlResolver::s().get_Profile_ڈیٹا(model, کلید);
}
File_سطر* V3Control::get_Profile_ڈیٹا_File_سطر() {
    واپس V3ControlResolver::s().get_Profile_ڈیٹا_File_سطر();
}
منطقی V3Control::getScopeTraceOn(ثابت string& scope) {
    واپس V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

خالی V3Control::contents_ڈالو_متن(ثابت string& متن) { WildcardContents::ڈالو_متن(متن); }

منطقی V3Control::شامل_ہے_MTask_Profile_ڈیٹا() {
    واپس V3ControlResolver::s().شامل_ہے_MTask_Profile_ڈیٹا();
}
uint64_t V3Control::get_Current_Hier_بلاک_لاگت() {
    واپس V3ControlResolver::s().get_Current_Hier_بلاک_لاگت();
}

منطقی V3Control::waive(ثابت File_سطر* filelinep, V3_Error_رمز رمز, ثابت string& پیغام) {
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    اگر (!filep) واپس جھوٹ;
    واپس filep->waive(رمز, پیغام);
}

خالی V3Control::selfTest() { VInterval_درخت::selfTest(); }
