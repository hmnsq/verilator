// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
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

#ضمن "V3PchAstMT.h"

#ضمن "V3File.h"

#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <cerrno>
#ضمن <cstdarg>
#ضمن <fcntl.h>
#ضمن <iomanip>
#ضمن <map>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

// clang-format off
#إذا defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#جے_انت

#جے_تعریف_شدہ HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#وإلا
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#جے_انت

#جے_تعریف_شدہ INFILTER_PIPE
# include <sys/wait.h>
#جے_انت

#إذا defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#جے_انت
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr صحيح INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr صحيح INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_ناپ_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

صنف V3FileDependImp final {
    // TYPES
    صنف DependFile final {
        // A single file
        ثابت منطقي m_نشانہ;  // True if write, else read
        منطقي m_exists = سچ;  // File exists
        ثابت string m_filename;  // Filename
        بنية stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    عام:
        DependFile(ثابت string& filename, منطقي نشانہ)
            : m_نشانہ{نشانہ}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = افتراضي;
        ثابت string& filename() ثابت { أرجع m_filename; }
        منطقي نشانہ() ثابت { أرجع m_نشانہ; }
        منطقي exists() ثابت { أرجع m_exists; }
        off_t ناپ() ثابت { أرجع m_stat.st_ناپ; }
        ino_t ino() ثابت { أرجع m_stat.st_ino; }
        ویلا_t cstime() ثابت { أرجع m_stat.st_ctime; }  // Seconds
        ویلا_t cnstime() ثابت { أرجع VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        ویلا_t mstime() ثابت { أرجع m_stat.st_mtime; }  // Seconds
        ویلا_t mnstime() ثابت { أرجع VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_علامت() {
            ساكن VHashSha512 s_خالی_تھاواں_Hash;
            أرجع m_hash.digestSymbol24() != s_خالی_تھاواں_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        فراغ لدو_Stats() {
            إذا (!m_stat.st_mtime) {
                ثابت string fn = filename();
                ثابت صحيح err = stat(fn.c_str(), &m_stat);
                إذا (err != 0) {
                    std::memset(&m_stat, 0, حجم(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = خطأ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } وإلا {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    إذا (!نشانہ() && ناپ() <= FILE_HASH_ناپ_MAX) m_hash.داخل_کرو_File(filename());
                }
            }
        }
        منطقي عامل<(ثابت DependFile& rhs) ثابت { أرجع filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::سیٹ<string> m_filename_سیٹ VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::سیٹ<DependFile> m_filenameList;  // Files sourced/generated

    ساكن string stripQuotes(ثابت string& وچ) {
        string pretty = وچ;
        string::ناپ_type pos;
        طالما ((pos = pretty.لبھو('\"')) != string::npos) pretty.replace(pos, 1, "_");
        طالما ((pos = pretty.لبھو('\n')) != string::npos) pretty.replace(pos, 1, "_");
        أرجع pretty;
    }

عام:
    // ACCESSOR METHODS
    فراغ جوڑو_Src_Depend(ثابت string& filename) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        ثابت خودکار it_لبھیا_Pair = m_filename_سیٹ.داخل_کرو(filename);
        إذا (it_لبھیا_Pair.دوجا) {
            DependFile df{filename, خطأ};
            df.لدو_Stats();  // Get size now, in case changes during the run
            m_filenameList.داخل_کرو(df);
        }
    }
    فراغ جوڑو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        ثابت خودکار it_لبھیا_Pair = m_filename_سیٹ.داخل_کرو(filename);
        إذا (it_لبھیا_Pair.دوجا) m_filenameList.داخل_کرو(DependFile{filename, سچ});
    }
    فراغ writeDepend(ثابت string& filename);
    std::سمتیہ<string> getAllDeps() ثابت;
    فراغ writeTimes(ثابت string& filename, ثابت string& cmdline_وچ);
    منطقي checkTimes(ثابت string& filename, ثابت string& cmdline_وچ);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

فراغ V3FileDependImp::writeDepend(ثابت string& filename) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    لكل (ثابت DependFile& i : m_filenameList) {
        إذا (i.نشانہ()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    لكل (ثابت DependFile& i : m_filenameList) {
        إذا (!i.نشانہ()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    إذا (v3Global.opt.makePhony()) {
        *ofp << '\n';
        لكل (ثابت DependFile& i : m_filenameList) {
            إذا (!i.نشانہ()) *ofp << i.filename() << ":\n";
        }
    }
}

std::سمتیہ<string> V3FileDependImp::getAllDeps() ثابت {
    std::سمتیہ<string> r;
    لكل (ثابت خودکار& itr : m_filenameList) {
        إذا (!itr.نشانہ() && itr.exists()) r.ڈالو_پٹھ(itr.filename());
    }
    أرجع r;
}

فراغ V3FileDependImp::writeTimes(ثابت string& filename, ثابت string& cmdline_وچ) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    ثابت string cmdline = stripQuotes(cmdline_وچ);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    لكل (خودکار& it : m_filenameList) {
        DependFile* ثابت depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->لدو_Stats();
        off_t show_ناپ = depfilep->ناپ();
        ino_t showIno = depfilep->ino();
        إذا (depfilep->filename() == filename) {
            show_ناپ = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->نشانہ() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_ناپ;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_علامت() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

منطقي V3FileDependImp::checkTimes(ثابت string& filename, ثابت string& cmdline_وچ) {
    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    إذا (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        أرجع خطأ;
    }
    {
        ثابت string ignore = V3Os::getline(*ifp);
        إذا (ignore.خالی_تھاواں()) { /*ورتیا*/
        }
    }
    {
        حرف chkDir;
        *ifp >> chkDir;
        حرف quote;
        *ifp >> quote;
        ثابت string chkCmdline = V3Os::getline(*ifp, '"');
        ثابت string cmdline = stripQuotes(cmdline_وچ);
        إذا (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            أرجع خطأ;
        }
    }

    ثابت منطقي skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").خالی_تھاواں();

    طالما (!ifp->eof()) {
        حرف chkDir;
        *ifp >> chkDir;
        off_t chk_ناپ;
        *ifp >> chk_ناپ;
        ino_t chkIno;
        *ifp >> chkIno;
        إذا (ifp->eof()) اكسر;  // Needed to read final whitespace before found eof
        ویلا_t chkCstime;
        *ifp >> chkCstime;
        ویلا_t chkCnstime;
        *ifp >> chkCnstime;
        ویلا_t chkMstime;
        *ifp >> chkMstime;
        ویلا_t chkMnstime;
        *ifp >> chkMnstime;
        حرف quote;
        *ifp >> quote;
        ثابت string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ثابت string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        بنية stat curStat;
        ثابت صحيح err = stat(chkFilename.c_str(), &curStat);
        إذا (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            أرجع خطأ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        إذا (filename == chkFilename) استمر;

        UINFO(9, " got d=" << chkDir << " s=" << chk_ناپ << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_ناپ << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        إذا (!(curStat.st_ناپ >= chk_ناپ && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_ناپ << "=?" << chk_ناپ << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            إذا (skipHashing) أرجع خطأ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            إذا (chkHash == "unhashed") أرجع خطأ;

            VHashSha512 curHash;
            curHash.داخل_کرو_File(chkFilename);
            إذا (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                أرجع خطأ;
            }
        }
    }
    أرجع سچ;
}

//######################################################################
// V3File

فراغ V3File::جوڑو_Src_Depend(ثابت string& filename) VL_MT_محفوظ { dependImp.جوڑو_Src_Depend(filename); }
فراغ V3File::جوڑو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ { dependImp.جوڑو_Tgt_Depend(filename); }
فراغ V3File::writeDepend(ثابت string& filename) { dependImp.writeDepend(filename); }
std::سمتیہ<string> V3File::getAllDeps() { أرجع dependImp.getAllDeps(); }
فراغ V3File::writeTimes(ثابت string& filename, ثابت string& cmdline_وچ) {
    dependImp.writeTimes(filename, cmdline_وچ);
}
منطقي V3File::checkTimes(ثابت string& filename, ثابت string& cmdline_وچ) {
    أرجع dependImp.checkTimes(filename, cmdline_وچ);
}
فراغ V3File::createMakeDirFor(ثابت string& filename) {
    إذا (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().لمائی() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
فراغ V3File::createMakeDir() {
    ساكن منطقي s_created = خطأ;
    إذا (!s_created) {
        s_created = سچ;
        V3Os::createDir(v3Global.opt.makeDir());
        إذا (v3Global.opt.hier_چوٹی()) V3Os::createDir(v3Global.opt.hier_چوٹی_انکڑے_Dir());
    }
}

//######################################################################
// VInFilterImp

صنف VInFilterImp final {
    ورتوں StrList = VInFilter::StrList;

    std::map<ثابت std::string, std::string> m_contentsMap;  // Cache of file contents
    منطقي m_readEof = خطأ;  // Received EOF on read
#جے_تعریف_شدہ INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#وإلا
    صحيح m_pid = 0;  // fork() process id - always zero as disabled
#جے_انت
    منطقي m_pidExited = خطأ;  //  waitpid() reaped the filter process
    صحيح m_pid_حالت = 0;  // Exit status of the filter process
    صحيح m_writeFd = 0;  // File descriptor TO filter
    صحيح m_readFd = 0;  // File descriptor FROM filter

خاص:
    // METHODS

    منطقي readContents(ثابت string& filename, StrList& outl) {
        إذا (m_pid) {
            أرجع readContentsFilter(filename, outl);
        } وإلا {
            أرجع readContentsFile(filename, outl);
        }
    }
    منطقي readContentsFile(ثابت string& filename, StrList& outl) {
        ثابت صحيح fd = open(filename.c_str(), O_RDONLY);
        إذا (fd < 0) أرجع خطأ;
        m_readEof = خطأ;
        read_بلاکاں(fd, -1, outl);
        close(fd);
        أرجع سچ;
    }
    // cppcheck-suppress constParameterReference
    منطقي readContentsFilter(ثابت string& filename, StrList& outl) {
        (فراغ)filename;  // Prevent unused variable warning
        (فراغ)outl;  // Prevent unused variable warning
#جے_تعریف_شدہ INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ثابت string سطر = read_Filter_سطر();
        إذا (سطر.لبھو("Content-Length") != string::npos) {
            صحيح len = 0;
            لڑی_توں_پڑھو(سطر.c_str(), "Content-Length: %d\n", &len);
            read_بلاکاں(m_readFd, len, outl);
            أرجع سچ;
        } وإلا {
            إذا (سطر != "") v3error("--pipe-filter protocol error, unexpected: " << سطر);
            أرجع خطأ;
        }
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
        أرجع خطأ;
#جے_انت
    }

    // cppcheck-suppress functionConst
    فراغ checkFilter(منطقي hang) {
#جے_تعریف_شدہ INFILTER_PIPE
        إذا (!m_pidExited && waitpid(m_pid, &m_pid_حالت, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_حالت << " exit=" << WEXITSTATUS(m_pid_حالت) << " err"
                         << std::strerror(errno));
            m_readEof = سچ;
            m_pidExited = سچ;
        }
#جے_انت
    }

    فراغ read_بلاکاں(صحيح fd, صحيح ناپ, StrList& outl) {
        حرف buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        طالما (!m_readEof && (ناپ < 0 || ناپ > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            إذا (ناپ > 0 && ناپ < todo) todo = ناپ;
            errno = 0;
            ثابت ssize_t got = اقرأ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            إذا (got > 0) {
                outl.emplace_پٹھ(string(buf, got));
                sizegot += got;
            } وإلا إذا (errno == EINTR || errno == EAGAIN
#جے_تعریف_شدہ EWOULDBLOCK
                       || errno == EWOULDBLOCK
#جے_انت
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                m_readEof = سچ;
                اكسر;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_سطر() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string سطر;
        طالما (!m_readEof) {
            StrList outl;
            read_بلاکاں(m_readFd, 1, outl);
            ثابت string onechar = listString(outl);
            سطر += onechar;
            إذا (onechar == "\n") {
                إذا (سطر == "\n") {
                    سطر = "";
                    استمر;
                } وإلا {
                    اكسر;
                }
            }
        }
        UINFO(6, "filter-line-in: " << سطر);
        أرجع سطر;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    فراغ writeFilter(ثابت string& باہرلا) {
        إذا (debug() >= 6) {
            UINFO(6, "filter-out: " << باہرلا);
            إذا (باہرلا[باہرلا.لمائی() - 1] != '\n') باہر << '\n';
        }
        إذا (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = سچ;
            stop();
        }
        بنانشانی وتھ = 0;
        طالما (!m_readEof && باہرلا.لمائی() > وتھ) {
            errno = 0;
            ثابت صحيح got = اكتب(m_writeFd, (باہرلا.c_str()) + وتھ, باہرلا.لمائی() - وتھ);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            إذا (got > 0) {
                وتھ += got;
            } وإلا إذا (errno == EINTR || errno == EAGAIN
#جے_تعریف_شدہ EWOULDBLOCK
                       || errno == EWOULDBLOCK
#جے_انت
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                اكسر;
            }
        }
    }

    // Start the filter
    فراغ شروع(ثابت string& command) {
        إذا (command == "") {
            m_pid = 0;  // Disabled
        } وإلا {
            شروع_Filter(command);
        }
    }
    فراغ شروع_Filter(ثابت string& command) {
        (فراغ)command;  // Prevent Unused variable warning
#جے_تعریف_شدہ INFILTER_PIPE
        صحيح fd_stdin[2];  // Can't use std::array
        صحيح fd_stdout[2];  // Can't use std::array
        constexpr صحيح P_RD = 0;
        constexpr صحيح P_WR = 1;

        إذا (VL_UNCOVERABLE(أنبوب(fd_stdin) != 0 || أنبوب(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        إذا (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ثابت pid_t pid = fork();
        إذا (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        إذا (pid == 0) {  //بچہ
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<حرف*>(خالی_سوچک));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } وإلا {  //ماپا
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = خطأ;
            m_pid_حالت = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = خطأ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            صحيح flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#جے_انت
    }

    فراغ stop() {
        إذا (m_pid) stopFilter();
    }
    فراغ stopFilter() {
        UINFO(6, "Stopping filter process");
#جے_تعریف_شدہ INFILTER_PIPE
        close(m_writeFd);
        checkFilter(سچ);
        إذا (!WIFEXITED(m_pid_حالت) || WEXITSTATUS(m_pid_حالت) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#جے_انت
    }

محمي:
    یار صنف VInFilter;
    // Read file contents and return it
    منطقي readWholefile(ثابت string& filename, StrList& outl) {
        ثابت خودکار it = m_contentsMap.لبھو(filename);
        إذا (it != m_contentsMap.سرا()) {
            outl.ڈالو_پٹھ(it->دوجا);
            أرجع سچ;
        }
        إذا (!readContents(filename, outl)) أرجع خطأ;
        إذا (list_ناپ(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        أرجع سچ;
    }
    ساكن ناپ_t list_ناپ(ثابت StrList& sl) {
        ناپ_t نتیجہ = 0;
        لكل (ثابت string& i : sl) نتیجہ += i.لمائی();
        أرجع نتیجہ;
    }
    ساكن string listString(ثابت StrList& sl) {
        string نتیجہ;
        لكل (ثابت string& i : sl) نتیجہ += i;
        أرجع نتیجہ;
    }
    // CONSTRUCTORS
    واضح VInFilterImp(ثابت string& command) { شروع(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ثابت string& command) { m_impp = جديد VInFilterImp{command}; }
VInFilter::~VInFilter() {
    إذا (m_impp) VL_DO_CLEAR(مٹاؤ m_impp, m_impp = خالی_سوچک);
}

منطقي VInFilter::readWholefile(ثابت string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    أرجع m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_باہرلا_Formatter::V3_باہرلا_Formatter(V3_باہرلا_Formatter::Language lang)
    : m_lang{lang} {
    m_بلاک_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_چوڑائی = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ساكن constexpr صحيح MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ساكن ثابت std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> جدول;
    لكل (صحيح i = 0; i <= MAXSPACE; ++i) جدول[i] = std::string(static_cast<ناپ_t>(i), ' ');
    أرجع جدول;
}();

ثابت std::string& V3_باہرلا_Formatter::indentSpaces(صحيح عدد) {
    // Indent the specified number of spaces.
    أرجع s_indentSpaces[std::max(0, std::min(عدد, MAXSPACE))];
}

منطقي V3_باہرلا_Formatter::token_میل(ثابت حرف* cp, ثابت حرف* cmp) {
    طالما (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    إذا (*cmp) أرجع خطأ;
    إذا (*cp && !std::خلا_اے(*cp)) أرجع خطأ;
    أرجع سچ;
}

منطقي V3_باہرلا_Formatter::token_شروع(ثابت حرف* cp) {
    أرجع (token_میل(cp, "begin") || token_میل(cp, "case") || token_میل(cp, "casex")
            || token_میل(cp, "casez") || token_میل(cp, "class") || token_میل(cp, "function")
            || token_میل(cp, "interface") || token_میل(cp, "module") || token_میل(cp, "package")
            || token_میل(cp, "task"));
}

منطقي V3_باہرلا_Formatter::token_سرا(ثابت حرف* cp) {
    أرجع (token_میل(cp, "end") || token_میل(cp, "endcase") || token_میل(cp, "endclass")
            || token_میل(cp, "endfunction") || token_میل(cp, "endinterface")
            || token_میل(cp, "endmodule") || token_میل(cp, "endpackage")
            || token_میل(cp, "endtask"));
}

منطقي V3_باہرلا_Formatter::token_Not_شروع(ثابت حرف* cp) {
    أرجع (token_میل(cp, "export") || token_میل(cp, "import"));
}

صحيح V3_باہرلا_Formatter::سرا_پدھراں(ثابت حرف* strg) {
    صحيح پدھراں = m_indentLevel;
    {
        ثابت حرف* cp = strg;
        طالما (std::خلا_اے(*cp)) ++cp;
        بدل (*cp) {
        حالة '\n':  // Newlines.. No need for whitespace before it
            أرجع 0;
        حالة '#':  // Preproc directive
            إذا (m_lang == LA_C) أرجع 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ثابت حرف* mp = cp;
            لكل (; std::حرف_یا_ہندسہ(*mp); ++mp) {}
            إذا (mp[0] == ':' && mp[1] != ':') أرجع (پدھراں - m_بلاک_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        بدل (*cp) {
        حالة '}':
        حالة ')': پدھراں -= m_بلاک_Indent; اكسر;
        حالة '<':
            إذا (m_lang == LA_XML) {
                إذا (cp[1] == '/') پدھراں -= m_بلاک_Indent;
            }
            اكسر;
        حالة 'e':
            إذا (m_lang == LA_VERILOG && token_سرا(cp)) پدھراں -= m_بلاک_Indent;
            اكسر;
        حالة '\t':
        حالة ' ': اكسر;  // Continue
        افتراضي: أرجع پدھراں;  // Letter
        }
    }
    أرجع پدھراں;
}

فراغ V3_باہرلا_Formatter::putns(ثابت Ast_گرہ* nodep, ثابت حرف* strg) {
    إذا (!v3Global.opt.decoration()) {
        putsOutput(strg);
        أرجع;
    }

    ثابت منطقي put_گرہ_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_سروت_آخری_Filenameno != nodep->fileline()->filenameno()  //
            || m_سروت_آخری_Lineno != nodep->fileline()->پہلا_Lineno())  //
        && File_سطر::built_وچ_Filename() != nodep->fileline()->filename();

    إذا (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_گرہ_Decoration)) {
        putsNoTracking(indentSpaces(سرا_پدھراں(strg)));
        m_prependIndent = خطأ;
    }

    إذا (put_گرہ_Decoration) {
        ثابت File_سطر* ثابت flp = nodep->fileline();
        m_سروت_آخری_Lineno = flp->پہلا_Lineno();
        m_سروت_آخری_Filenameno = flp->filenameno();
        ثابت std::string lineno = std::منزل_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_منزل_Hex(nodep) + "*/");
    }

    منطقي notstart = خطأ;
    منطقي wordstart = سچ;
    منطقي equalsForBracket = خطأ;  // Looking for "= {"
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        إذا (std::حرف_اے(*cp)) {
            إذا (wordstart && m_lang == LA_VERILOG && token_Not_شروع(cp)) notstart = سچ;
            إذا (wordstart && m_lang == LA_VERILOG && !notstart && token_شروع(cp)) indentInc();
            إذا (wordstart && m_lang == LA_VERILOG && token_سرا(cp)) indentDec();
        }
        ثابت منطقي was_وچ_Backslash = m_وچ_Backslash;
        m_وچ_Backslash = خطأ;
        بدل (*cp) {
        حالة '\n':
            ++m_lineno;
            wordstart = سچ;
            إذا (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = سچ;
            } وإلا {
                m_prependIndent = خطأ;
                putsNoTracking(indentSpaces(سرا_پدھراں(cp + 1)));
            }
            اكسر;
        حالة ' ': wordstart = سچ; اكسر;
        حالة '\t': wordstart = سچ; اكسر;
        حالة '"':
            wordstart = خطأ;
            إذا (!was_وچ_Backslash) m_وچ_String_Literal = !m_وچ_String_Literal;
            اكسر;
        حالة '\\':
            wordstart = خطأ;
            إذا (!was_وچ_Backslash) m_وچ_Backslash = سچ;
            اكسر;
        حالة '/':
            إذا (m_lang == LA_C || m_lang == LA_VERILOG) {
                إذا (cp > strg && cp[-1] == '/' && !m_وچ_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    طالما (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    إذا (*cp) putcNoTracking(*cp);
                }
            }
            اكسر;
        حالة '{':
            إذا (!m_وچ_String_Literal) {
                إذا (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ڈالو(m_indentLevel
                                    * m_بلاک_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            اكسر;
        حالة '}':
            إذا (!m_وچ_String_Literal) {
                إذا (m_bracketLevel > 0) {
                    m_parenVec.نکالو();
                    --m_bracketLevel;
                }
                indentDec();
            }
            اكسر;
        حالة '(':
            إذا (!m_وچ_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ڈالو(m_کالم);
            }
            اكسر;
        حالة ')':
            إذا (!m_وچ_String_Literal) {
                إذا (!m_parenVec.خالی_تھاواں()) m_parenVec.نکالو();
                indentDec();
            }
            اكسر;
        حالة '<':
            إذا (m_lang == LA_XML) {
                إذا (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } وإلا إذا (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } وإلا {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            اكسر;
        حالة '>':
            إذا (m_lang == LA_XML) {
                indentDec();
                إذا (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            اكسر;
        افتراضي: wordstart = خطأ; اكسر;
        }

        بدل (*cp) {
        حالة '=': equalsForBracket = سچ; اكسر;
        حالة ' ': اكسر;
        افتراضي: equalsForBracket = خطأ; اكسر;
        }
    }
}

فراغ V3_باہرلا_Formatter::put_Break_اظہاریہ() {
    إذا (!m_parenVec.خالی_تھاواں()) putBreak();
}

// Add a line break if too wide
فراغ V3_باہرلا_Formatter::putBreak() {
    إذا (!v3Global.opt.decoration()) أرجع;
    إذا (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        إذا (exceeded_چوڑائی()) {
            putcNoTracking('\n');
            إذا (!m_parenVec.خالی_تھاواں()) putsNoTracking(indentSpaces(m_parenVec.چوٹی()));
        }
    }
}

فراغ V3_باہرلا_Formatter::putsQuoted(ثابت string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ثابت string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    لكل (ثابت حرف c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    إذا (strg.لبھو('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
فراغ V3_باہرلا_Formatter::putsNoTracking(ثابت string& strg) {
    إذا (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        أرجع;
    }
    // Don't track {}'s, probably because it's a $display format string
    لكل (ثابت حرف c : strg) putcNoTracking(c);
}

فراغ V3_باہرلا_Formatter::putcNoTracking(حرف chr) {
    إذا (!v3Global.opt.decoration()) {
        putcOutput(chr);
        أرجع;
    }
    بدل (chr) {
    حالة '\n':
        ++m_lineno;
        m_کالم = 0;
        m_nobreak = سچ;
        اكسر;
    حالة '\t': m_کالم = ((m_کالم + 9) / 8) * 8; اكسر;
    حالة ' ':
    حالة '(':
    حالة '|':
    حالة '&': ++m_کالم; اكسر;
    افتراضي:
        ++m_کالم;
        m_nobreak = خطأ;
        اكسر;
    }
    putcOutput(chr);
}

string V3_باہرلا_Formatter::quoteNameControls(ثابت string& namein,
                                         V3_باہرلا_Formatter::Language lang) VL_خالص {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string باہرلا;
    إذا (lang == LA_XML) {
        // Encode chars into XML string
        لكل (ثابت حرف c : namein) {
            إذا (c == '"') {
                باہرلا += "&quot;"s;
            } وإلا إذا (c == '\'') {
                باہرلا += "&apos;"s;
            } وإلا إذا (c == '<') {
                باہرلا += "&lt;"s;
            } وإلا إذا (c == '>') {
                باہرلا += "&gt;"s;
            } وإلا إذا (c == '&') {
                باہرلا += "&amp;"s;
            } وإلا إذا (std::isprint(c)) {
                باہرلا += c;
            } وإلا {
                باہرلا += "&#"s + cvt_منزل_Str(static_cast<بنانشانی صحيح>(c & 0xff)) + ";";
            }
        }
    } وإلا {
        // Encode control chars into C style escapes
        لكل (ثابت حرف c : namein) {
            إذا (c == '\\' || c == '"') {
                باہرلا += "\\"s + c;
            } وإلا إذا (c == '\n') {
                باہرلا += "\\n";
            } وإلا إذا (c == '\r') {
                باہرلا += "\\r";
            } وإلا إذا (c == '\t') {
                باہرلا += "\\t";
            } وإلا إذا (std::isprint(c)) {
                باہرلا += c;
            } وإلا {
                // This will also cover \a etc
                ثابت string octal
                    = "\\"s + cvt_منزل_Str((c >> 6) & 3) + cvt_منزل_Str((c >> 3) & 7) + cvt_منزل_Str(c & 7);
                باہرلا += octal;
            }
        }
    }
    أرجع باہرلا;
}

//----------------------------------------------------------------------
// Simple wrappers

فراغ V3_باہرلا_Formatter::لکھو(ثابت حرف* fmt...) {
    constexpr ناپ_t bufsize = 5000;
    حرف sbuff[bufsize];
    va_list ap;
    va_شروع(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_سرا(ap);
    هذا->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_باہرلا_File::V3_باہرلا_File(ثابت string& filename, V3_باہرلا_Formatter::Language lang)
    : V3_باہرلا_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{جديد std::array<حرف, WRITE_بفر_ناپ_بائٹاں>{}} {
    إذا ((m_fp = V3File::new_fopen_w(filename)) == خالی_سوچک) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_باہرلا_File::~V3_باہرلا_File() {
    write_بلاک();

    إذا (m_fp) fclose(m_fp);
    m_fp = خالی_سوچک;
}

فراغ V3_باہرلا_File::putsForceIncs() {
    ثابت VStringList& forceIncs = v3Global.opt.forceIncs();
    لكل (ثابت string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

فراغ V3_باہرلا_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = سچ;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    لكل (حرف& c : var) {
        إذا (!std::حرف_یا_ہندسہ(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_باہرلا_دھارا::V3_باہرلا_دھارا(std::ostream& ostream, V3_باہرلا_Formatter::Language lang)
    : V3_باہرلا_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

صنف VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ثابت std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_سیٹ<std::string> m_new_Id_سیٹ VL_GUARDED_BY(m_mutex);  // Which new names exist
محمي:
    // CONSTRUCTORS
    یار صنف VIdProtect;
    ساكن VIdProtectImp& singleton() VL_MT_محفوظ {
        ساكن VIdProtectImp s;
        أرجع s;
    }

عام:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = افتراضي;
    // METHODS
    string passthru(ثابت string& old) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        إذا (!v3Global.opt.protectIds()) أرجع old;
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        ثابت خودکار it = m_nameMap.لبھو(old);
        إذا (it != m_nameMap.سرا()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->دوجا,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } وإلا {
            m_nameMap.emplace(old, old);
            m_new_Id_سیٹ.داخل_کرو(old);
        }
        أرجع old;
    }
    string protectIf(ثابت string& old, منطقي doIt) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        إذا (!v3Global.opt.protectIds() || old.خالی_تھاواں() || !doIt) أرجع old;
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        ثابت خودکار pair = m_nameMap.emplace(old, "");
        إذا (pair.دوجا) {
            string باہرلا;
            إذا (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                باہرلا = "PS" + old;
            } وإلا {
                VHashSha512 digest{v3Global.opt.protect_کلید_Defaulted()};
                digest.داخل_کرو(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                باہرلا = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                لكل (ناپ_t len = 6; len < باہرلا.ناپ() - 3; len += 3) {
                    ثابت string tryout = باہرلا.substr(0, len);
                    إذا (m_new_Id_سیٹ.داخل_کرو(tryout).دوجا) {
                        باہرلا = tryout;
                        اكسر;
                    }
                }
            }
            pair.پہلا->دوجا = باہرلا;
        }
        أرجع pair.پہلا->دوجا;
    }
    string protect_الفاظ_If(ثابت string& old, منطقي doIt) VL_MT_محفوظ {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        إذا (!(doIt && v3Global.opt.protectIds())) أرجع old;
        string باہرلا;
        string::ناپ_type شروع = 0;
        // space, ., ->
        طالما (سچ) {
            // When C++11, use find_if and lambda
            string::ناپ_type pos = string::npos;
            string separator;
            trySep(old, شروع, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, شروع, ")", pos /*ref*/, separator /*ref*/);
            إذا (pos == string::npos) اكسر;
            باہرلا += protectIf(old.substr(شروع, pos - شروع), سچ) + separator;
            شروع = pos + separator.لمائی();
        }
        باہرلا += protectIf(old.substr(شروع), سچ);
        أرجع باہرلا;
    }
    فراغ writeMapFile(ثابت string& filename) ثابت {
        V3_باہرلا_Xml_File دا{filename};
        دا.puts_سرخی();
        دا.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        دا.puts("<verilator_id_map>\n");
        {
            لكل (ثابت خودکار& itr : m_nameMap) {
                دا.puts("<map from=\"" + itr.دوجا + "\" to=\"" + itr.پہلا + "\"/>\n");
            }
        }
        دا.puts("</verilator_id_map>\n");
    }

خاص:
    فراغ trySep(ثابت string& old, string::ناپ_type شروع, ثابت string& trySep,
                string::ناپ_type& posr, string& separatorr) VL_خالص {
        ثابت string::ناپ_type trypos = old.لبھو(trySep, شروع);
        إذا (trypos != string::npos) {
            إذا (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ثابت string& وچ) VL_MT_محفوظ {
    أرجع v3Global.opt.protectIds() ? "" : وچ;
}
string VIdProtect::protectIf(ثابت string& old, منطقي doIt) VL_MT_محفوظ {
    أرجع VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_الفاظ_If(ثابت string& old, منطقي doIt) VL_MT_محفوظ {
    أرجع VIdProtectImp::singleton().protect_الفاظ_If(old, doIt);
}
فراغ VIdProtect::writeMapFile(ثابت string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
