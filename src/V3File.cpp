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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3File.h"

#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <cerrno>
#अन्तर्भाव <cstdarg>
#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>

#अन्तर्भाव <sys/stat.h>
#अन्तर्भाव <sys/types.h>

// clang-format off
#यदि defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#यदि_अन्तः

#यदि_परिभाषितम् HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#अन्यथा
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#यदि_अन्तः

#यदि_परिभाषितम् INFILTER_PIPE
# include <sys/wait.h>
#यदि_अन्तः

#यदि defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#यदि_अन्तः
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr पूर्णाङ्क INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr पूर्णाङ्क INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_आकार_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

वर्ग V3FileDependImp final {
    // TYPES
    वर्ग DependFile final {
        // A single file
        ध्रुव तार्किक m_target;  // True if write, else read
        तार्किक m_exists = सत्य;  // File exists
        ध्रुव string m_filename;  // Filename
        संरचना stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    सार्वजनिक:
        DependFile(ध्रुव string& filename, तार्किक target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = स्वभाव;
        ध्रुव string& filename() ध्रुव { प्रत्यागम m_filename; }
        तार्किक target() ध्रुव { प्रत्यागम m_target; }
        तार्किक exists() ध्रुव { प्रत्यागम m_exists; }
        off_t आकार() ध्रुव { प्रत्यागम m_stat.st_आकार; }
        ino_t ino() ध्रुव { प्रत्यागम m_stat.st_ino; }
        time_t cstime() ध्रुव { प्रत्यागम m_stat.st_ctime; }  // Seconds
        time_t cnstime() ध्रुव { प्रत्यागम VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ध्रुव { प्रत्यागम m_stat.st_mtime; }  // Seconds
        time_t mnstime() ध्रुव { प्रत्यागम VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_चिह्नम्() {
            स्थिर VHashSha512 s_रिक्तानि_Hash;
            प्रत्यागम m_hash.digestSymbol24() != s_रिक्तानि_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        शून्य आरोपय_Stats() {
            यदि (!m_stat.st_mtime) {
                ध्रुव string fn = filename();
                ध्रुव पूर्णाङ्क err = stat(fn.c_str(), &m_stat);
                यदि (err != 0) {
                    std::स्मृतिं_पूरय(&m_stat, 0, परिमाण(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = असत्य;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } अन्यथा {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    यदि (!target() && आकार() <= FILE_HASH_आकार_MAX) m_hash.निवेशय_File(filename());
                }
            }
        }
        तार्किक संक्रियकः<(ध्रुव DependFile& rhs) ध्रुव { प्रत्यागम filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::समुच्चयः<string> m_filename_समुच्चयः VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::समुच्चयः<DependFile> m_filenameList;  // Files sourced/generated

    स्थिर string stripQuotes(ध्रुव string& अन्तर्) {
        string pretty = अन्तर्;
        string::आकार_type pos;
        यावत् ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        यावत् ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        प्रत्यागम pretty;
    }

सार्वजनिक:
    // ACCESSOR METHODS
    शून्य सङ्कलय_Src_Depend(ध्रुव string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        ध्रुव स्वचालित it_प्राप्तम्_Pair = m_filename_समुच्चयः.निवेशय(filename);
        यदि (it_प्राप्तम्_Pair.second) {
            DependFile df{filename, असत्य};
            df.आरोपय_Stats();  // Get size now, in case changes during the run
            m_filenameList.निवेशय(df);
        }
    }
    शून्य सङ्कलय_Tgt_Depend(ध्रुव string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        ध्रुव स्वचालित it_प्राप्तम्_Pair = m_filename_समुच्चयः.निवेशय(filename);
        यदि (it_प्राप्तम्_Pair.second) m_filenameList.निवेशय(DependFile{filename, सत्य});
    }
    शून्य writeDepend(ध्रुव string& filename);
    std::सदिशम्<string> getAllDeps() ध्रुव;
    शून्य writeTimes(ध्रुव string& filename, ध्रुव string& cmdline_अन्तर्);
    तार्किक checkTimes(ध्रुव string& filename, ध्रुव string& cmdline_अन्तर्);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

शून्य V3FileDependImp::writeDepend(ध्रुव string& filename) {
    ध्रुव std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    यदि (ofp->fail()) v3fatal("Can't write file: " << filename);

    प्रति (ध्रुव DependFile& i : m_filenameList) {
        यदि (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    प्रति (ध्रुव DependFile& i : m_filenameList) {
        यदि (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    यदि (v3Global.opt.makePhony()) {
        *ofp << '\n';
        प्रति (ध्रुव DependFile& i : m_filenameList) {
            यदि (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::सदिशम्<string> V3FileDependImp::getAllDeps() ध्रुव {
    std::सदिशम्<string> r;
    प्रति (ध्रुव स्वचालित& itr : m_filenameList) {
        यदि (!itr.target() && itr.exists()) r.प्रक्षिप_पृष्ठभागः(itr.filename());
    }
    प्रत्यागम r;
}

शून्य V3FileDependImp::writeTimes(ध्रुव string& filename, ध्रुव string& cmdline_अन्तर्) {
    ध्रुव std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    यदि (ofp->fail()) v3fatal("Can't write file: " << filename);

    ध्रुव string cmdline = stripQuotes(cmdline_अन्तर्);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    प्रति (स्वचालित& it : m_filenameList) {
        DependFile* ध्रुव depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->आरोपय_Stats();
        off_t show_आकार = depfilep->आकार();
        ino_t showIno = depfilep->ino();
        यदि (depfilep->filename() == filename) {
            show_आकार = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->target() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_आकार;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_चिह्नम्() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

तार्किक V3FileDependImp::checkTimes(ध्रुव string& filename, ध्रुव string& cmdline_अन्तर्) {
    ध्रुव std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    यदि (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        प्रत्यागम असत्य;
    }
    {
        ध्रुव string ignore = V3Os::getline(*ifp);
        यदि (ignore.रिक्तानि()) { /*प्रयुक्तम्*/
        }
    }
    {
        वर्ण chkDir;
        *ifp >> chkDir;
        वर्ण quote;
        *ifp >> quote;
        ध्रुव string chkCmdline = V3Os::getline(*ifp, '"');
        ध्रुव string cmdline = stripQuotes(cmdline_अन्तर्);
        यदि (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            प्रत्यागम असत्य;
        }
    }

    ध्रुव तार्किक skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").रिक्तानि();

    यावत् (!ifp->eof()) {
        वर्ण chkDir;
        *ifp >> chkDir;
        off_t chk_आकार;
        *ifp >> chk_आकार;
        ino_t chkIno;
        *ifp >> chkIno;
        यदि (ifp->eof()) भङ्ग;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        वर्ण quote;
        *ifp >> quote;
        ध्रुव string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ध्रुव string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        संरचना stat curStat;
        ध्रुव पूर्णाङ्क err = stat(chkFilename.c_str(), &curStat);
        यदि (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            प्रत्यागम असत्य;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        यदि (filename == chkFilename) अनुवर्त;

        UINFO(9, " got d=" << chkDir << " s=" << chk_आकार << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_आकार << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        यदि (!(curStat.st_आकार >= chk_आकार && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_आकार << "=?" << chk_आकार << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            यदि (skipHashing) प्रत्यागम असत्य;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            यदि (chkHash == "unhashed") प्रत्यागम असत्य;

            VHashSha512 curHash;
            curHash.निवेशय_File(chkFilename);
            यदि (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                प्रत्यागम असत्य;
            }
        }
    }
    प्रत्यागम सत्य;
}

//######################################################################
// V3File

शून्य V3File::सङ्कलय_Src_Depend(ध्रुव string& filename) VL_MT_SAFE { dependImp.सङ्कलय_Src_Depend(filename); }
शून्य V3File::सङ्कलय_Tgt_Depend(ध्रुव string& filename) VL_MT_SAFE { dependImp.सङ्कलय_Tgt_Depend(filename); }
शून्य V3File::writeDepend(ध्रुव string& filename) { dependImp.writeDepend(filename); }
std::सदिशम्<string> V3File::getAllDeps() { प्रत्यागम dependImp.getAllDeps(); }
शून्य V3File::writeTimes(ध्रुव string& filename, ध्रुव string& cmdline_अन्तर्) {
    dependImp.writeTimes(filename, cmdline_अन्तर्);
}
तार्किक V3File::checkTimes(ध्रुव string& filename, ध्रुव string& cmdline_अन्तर्) {
    प्रत्यागम dependImp.checkTimes(filename, cmdline_अन्तर्);
}
शून्य V3File::createMakeDirFor(ध्रुव string& filename) {
    यदि (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().दीर्घता_2() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
शून्य V3File::createMakeDir() {
    स्थिर तार्किक s_created = असत्य;
    यदि (!s_created) {
        s_created = सत्य;
        V3Os::createDir(v3Global.opt.makeDir());
        यदि (v3Global.opt.hier_शिखरम्()) V3Os::createDir(v3Global.opt.hier_शिखरम्_दत्त_Dir());
    }
}

//######################################################################
// VInFilterImp

वर्ग VInFilterImp final {
    उपयुज्य StrList = VInFilter::StrList;

    std::map<ध्रुव std::string, std::string> m_contentsMap;  // Cache of file contents
    तार्किक m_readEof = असत्य;  // Received EOF on read
#यदि_परिभाषितम् INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#अन्यथा
    पूर्णाङ्क m_pid = 0;  // fork() process id - always zero as disabled
#यदि_अन्तः
    तार्किक m_pidExited = असत्य;  //  waitpid() reaped the filter process
    पूर्णाङ्क m_pid_स्थितिः = 0;  // Exit status of the filter process
    पूर्णाङ्क m_writeFd = 0;  // File descriptor TO filter
    पूर्णाङ्क m_readFd = 0;  // File descriptor FROM filter

गुप्त:
    // METHODS

    तार्किक readContents(ध्रुव string& filename, StrList& outl) {
        यदि (m_pid) {
            प्रत्यागम readContentsFilter(filename, outl);
        } अन्यथा {
            प्रत्यागम readContentsFile(filename, outl);
        }
    }
    तार्किक readContentsFile(ध्रुव string& filename, StrList& outl) {
        ध्रुव पूर्णाङ्क fd = उद्घाटय(filename.c_str(), O_RDONLY);
        यदि (fd < 0) प्रत्यागम असत्य;
        m_readEof = असत्य;
        read_खण्डानि(fd, -1, outl);
        close(fd);
        प्रत्यागम सत्य;
    }
    // cppcheck-suppress constParameterReference
    तार्किक readContentsFilter(ध्रुव string& filename, StrList& outl) {
        (शून्य)filename;  // Prevent unused variable warning
        (शून्य)outl;  // Prevent unused variable warning
#यदि_परिभाषितम् INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ध्रुव string पङ्क्तिः = read_Filter_पङ्क्तिः();
        यदि (पङ्क्तिः.find("Content-Length") != string::npos) {
            पूर्णाङ्क len = 0;
            वाक्यात्_पठ(पङ्क्तिः.c_str(), "Content-Length: %d\n", &len);
            read_खण्डानि(m_readFd, len, outl);
            प्रत्यागम सत्य;
        } अन्यथा {
            यदि (पङ्क्तिः != "") v3error("--pipe-filter protocol error, unexpected: " << पङ्क्तिः);
            प्रत्यागम असत्य;
        }
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
        प्रत्यागम असत्य;
#यदि_अन्तः
    }

    // cppcheck-suppress functionConst
    शून्य checkFilter(तार्किक hang) {
#यदि_परिभाषितम् INFILTER_PIPE
        यदि (!m_pidExited && waitpid(m_pid, &m_pid_स्थितिः, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_स्थितिः << " exit=" << WEXITSTATUS(m_pid_स्थितिः) << " err"
                         << std::strerror(errno));
            m_readEof = सत्य;
            m_pidExited = सत्य;
        }
#यदि_अन्तः
    }

    शून्य read_खण्डानि(पूर्णाङ्क fd, पूर्णाङ्क आकार, StrList& outl) {
        वर्ण buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        यावत् (!m_readEof && (आकार < 0 || आकार > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            यदि (आकार > 0 && आकार < todo) todo = आकार;
            errno = 0;
            ध्रुव ssize_t got = पठ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            यदि (got > 0) {
                outl.emplace_पृष्ठभागः(string(buf, got));
                sizegot += got;
            } अन्यथा यदि (errno == EINTR || errno == EAGAIN
#यदि_परिभाषितम् EWOULDBLOCK
                       || errno == EWOULDBLOCK
#यदि_अन्तः
            ) {
                checkFilter(असत्य);
                V3Os::u_sleep(1000);
                अनुवर्त;
            } अन्यथा {
                m_readEof = सत्य;
                भङ्ग;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_पङ्क्तिः() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string पङ्क्तिः;
        यावत् (!m_readEof) {
            StrList outl;
            read_खण्डानि(m_readFd, 1, outl);
            ध्रुव string onechar = listString(outl);
            पङ्क्तिः += onechar;
            यदि (onechar == "\n") {
                यदि (पङ्क्तिः == "\n") {
                    पङ्क्तिः = "";
                    अनुवर्त;
                } अन्यथा {
                    भङ्ग;
                }
            }
        }
        UINFO(6, "filter-line-in: " << पङ्क्तिः);
        प्रत्यागम पङ्क्तिः;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    शून्य writeFilter(ध्रुव string& बहिर्गमनम्) {
        यदि (debug() >= 6) {
            UINFO(6, "filter-out: " << बहिर्गमनम्);
            यदि (बहिर्गमनम्[बहिर्गमनम्.दीर्घता_2() - 1] != '\n') निर्गमः << '\n';
        }
        यदि (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = सत्य;
            stop();
        }
        अचिह्नित विस्थापनम् = 0;
        यावत् (!m_readEof && बहिर्गमनम्.दीर्घता_2() > विस्थापनम्) {
            errno = 0;
            ध्रुव पूर्णाङ्क got = लिख(m_writeFd, (बहिर्गमनम्.c_str()) + विस्थापनम्, बहिर्गमनम्.दीर्घता_2() - विस्थापनम्);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            यदि (got > 0) {
                विस्थापनम् += got;
            } अन्यथा यदि (errno == EINTR || errno == EAGAIN
#यदि_परिभाषितम् EWOULDBLOCK
                       || errno == EWOULDBLOCK
#यदि_अन्तः
            ) {
                checkFilter(असत्य);
                V3Os::u_sleep(1000);
                अनुवर्त;
            } अन्यथा {
                भङ्ग;
            }
        }
    }

    // Start the filter
    शून्य start(ध्रुव string& command) {
        यदि (command == "") {
            m_pid = 0;  // Disabled
        } अन्यथा {
            startFilter(command);
        }
    }
    शून्य startFilter(ध्रुव string& command) {
        (शून्य)command;  // Prevent Unused variable warning
#यदि_परिभाषितम् INFILTER_PIPE
        पूर्णाङ्क fd_stdin[2];  // Can't use std::array
        पूर्णाङ्क fd_stdout[2];  // Can't use std::array
        constexpr पूर्णाङ्क P_RD = 0;
        constexpr पूर्णाङ्क P_WR = 1;

        यदि (VL_UNCOVERABLE(प्रणाल(fd_stdin) != 0 || प्रणाल(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        यदि (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ध्रुव pid_t pid = fork();
        यदि (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        यदि (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<वर्ण*>(शून्यसूचकः));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } अन्यथा {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = असत्य;
            m_pid_स्थितिः = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = असत्य;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            पूर्णाङ्क flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#यदि_अन्तः
    }

    शून्य stop() {
        यदि (m_pid) stopFilter();
    }
    शून्य stopFilter() {
        UINFO(6, "Stopping filter process");
#यदि_परिभाषितम् INFILTER_PIPE
        close(m_writeFd);
        checkFilter(सत्य);
        यदि (!WIFEXITED(m_pid_स्थितिः) || WEXITSTATUS(m_pid_स्थितिः) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#यदि_अन्तः
    }

रक्षित:
    मित्रम् वर्ग VInFilter;
    // Read file contents and return it
    तार्किक readWholefile(ध्रुव string& filename, StrList& outl) {
        ध्रुव स्वचालित it = m_contentsMap.find(filename);
        यदि (it != m_contentsMap.अन्तः_2()) {
            outl.प्रक्षिप_पृष्ठभागः(it->second);
            प्रत्यागम सत्य;
        }
        यदि (!readContents(filename, outl)) प्रत्यागम असत्य;
        यदि (list_आकार(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        प्रत्यागम सत्य;
    }
    स्थिर आकार_t list_आकार(ध्रुव StrList& sl) {
        आकार_t परिणाम = 0;
        प्रति (ध्रुव string& i : sl) परिणाम += i.दीर्घता_2();
        प्रत्यागम परिणाम;
    }
    स्थिर string listString(ध्रुव StrList& sl) {
        string परिणाम;
        प्रति (ध्रुव string& i : sl) परिणाम += i;
        प्रत्यागम परिणाम;
    }
    // CONSTRUCTORS
    स्पष्ट VInFilterImp(ध्रुव string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ध्रुव string& command) { m_impp = नूतन VInFilterImp{command}; }
VInFilter::~VInFilter() {
    यदि (m_impp) VL_DO_CLEAR(विलोपय m_impp, m_impp = शून्यसूचकः);
}

तार्किक VInFilter::readWholefile(ध्रुव string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    प्रत्यागम m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_बहिर्गमनम्_Formatter::V3_बहिर्गमनम्_Formatter(V3_बहिर्गमनम्_Formatter::Language lang)
    : m_lang{lang} {
    m_खण्डम्_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_विस्तारः = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

स्थिर constexpr पूर्णाङ्क MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
स्थिर ध्रुव std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> सारणी;
    प्रति (पूर्णाङ्क i = 0; i <= MAXSPACE; ++i) सारणी[i] = std::string(static_cast<आकार_t>(i), ' ');
    प्रत्यागम सारणी;
}();

ध्रुव std::string& V3_बहिर्गमनम्_Formatter::indentSpaces(पूर्णाङ्क अङ्कः) {
    // Indent the specified number of spaces.
    प्रत्यागम s_indentSpaces[std::max(0, std::min(अङ्कः, MAXSPACE))];
}

तार्किक V3_बहिर्गमनम्_Formatter::token_मेलनम्(ध्रुव वर्ण* cp, ध्रुव वर्ण* cmp) {
    यावत् (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    यदि (*cmp) प्रत्यागम असत्य;
    यदि (*cp && !std::रिक्तम्_वा(*cp)) प्रत्यागम असत्य;
    प्रत्यागम सत्य;
}

तार्किक V3_बहिर्गमनम्_Formatter::tokenStart(ध्रुव वर्ण* cp) {
    प्रत्यागम (token_मेलनम्(cp, "begin") || token_मेलनम्(cp, "case") || token_मेलनम्(cp, "casex")
            || token_मेलनम्(cp, "casez") || token_मेलनम्(cp, "class") || token_मेलनम्(cp, "function")
            || token_मेलनम्(cp, "interface") || token_मेलनम्(cp, "module") || token_मेलनम्(cp, "package")
            || token_मेलनम्(cp, "task"));
}

तार्किक V3_बहिर्गमनम्_Formatter::token_अन्तः(ध्रुव वर्ण* cp) {
    प्रत्यागम (token_मेलनम्(cp, "end") || token_मेलनम्(cp, "endcase") || token_मेलनम्(cp, "endclass")
            || token_मेलनम्(cp, "endfunction") || token_मेलनम्(cp, "endinterface")
            || token_मेलनम्(cp, "endmodule") || token_मेलनम्(cp, "endpackage")
            || token_मेलनम्(cp, "endtask"));
}

तार्किक V3_बहिर्गमनम्_Formatter::tokenNotStart(ध्रुव वर्ण* cp) {
    प्रत्यागम (token_मेलनम्(cp, "export") || token_मेलनम्(cp, "import"));
}

पूर्णाङ्क V3_बहिर्गमनम्_Formatter::अन्तः_स्तराः(ध्रुव वर्ण* strg) {
    पूर्णाङ्क स्तराः = m_indentLevel;
    {
        ध्रुव वर्ण* cp = strg;
        यावत् (std::रिक्तम्_वा(*cp)) ++cp;
        विकल्प (*cp) {
        पक्ष '\n':  // Newlines.. No need for whitespace before it
            प्रत्यागम 0;
        पक्ष '#':  // Preproc directive
            यदि (m_lang == LA_C) प्रत्यागम 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ध्रुव वर्ण* mp = cp;
            प्रति (; std::अक्षरम्_अङ्कः_वा(*mp); ++mp) {}
            यदि (mp[0] == ':' && mp[1] != ':') प्रत्यागम (स्तराः - m_खण्डम्_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    प्रति (ध्रुव वर्ण* cp = strg; *cp; ++cp) {
        विकल्प (*cp) {
        पक्ष '}':
        पक्ष ')': स्तराः -= m_खण्डम्_Indent; भङ्ग;
        पक्ष '<':
            यदि (m_lang == LA_XML) {
                यदि (cp[1] == '/') स्तराः -= m_खण्डम्_Indent;
            }
            भङ्ग;
        पक्ष 'e':
            यदि (m_lang == LA_VERILOG && token_अन्तः(cp)) स्तराः -= m_खण्डम्_Indent;
            भङ्ग;
        पक्ष '\t':
        पक्ष ' ': भङ्ग;  // Continue
        स्वभाव: प्रत्यागम स्तराः;  // Letter
        }
    }
    प्रत्यागम स्तराः;
}

शून्य V3_बहिर्गमनम्_Formatter::putns(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* strg) {
    यदि (!v3Global.opt.decoration()) {
        putsOutput(strg);
        प्रत्यागम;
    }

    ध्रुव तार्किक put_ग्रन्थिः_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_source_चरमः_Filenameno != nodep->fileline()->filenameno()  //
            || m_source_चरमः_Lineno != nodep->fileline()->firstLineno())  //
        && File_पङ्क्तिः::built_अन्तर्_Filename() != nodep->fileline()->filename();

    यदि (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_ग्रन्थिः_Decoration)) {
        putsNoTracking(indentSpaces(अन्तः_स्तराः(strg)));
        m_prependIndent = असत्य;
    }

    यदि (put_ग्रन्थिः_Decoration) {
        ध्रुव File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        m_source_चरमः_Lineno = flp->firstLineno();
        m_source_चरमः_Filenameno = flp->filenameno();
        ध्रुव std::string lineno = std::गन्तव्यम्_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_गन्तव्यम्_Hex(nodep) + "*/");
    }

    तार्किक notstart = असत्य;
    तार्किक wordstart = सत्य;
    तार्किक equalsForBracket = असत्य;  // Looking for "= {"
    प्रति (ध्रुव वर्ण* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        यदि (std::अक्षरम्_वा(*cp)) {
            यदि (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = सत्य;
            यदि (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            यदि (wordstart && m_lang == LA_VERILOG && token_अन्तः(cp)) indentDec();
        }
        ध्रुव तार्किक was_अन्तर्_Backslash = m_अन्तर्_Backslash;
        m_अन्तर्_Backslash = असत्य;
        विकल्प (*cp) {
        पक्ष '\n':
            ++m_lineno;
            wordstart = सत्य;
            यदि (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = सत्य;
            } अन्यथा {
                m_prependIndent = असत्य;
                putsNoTracking(indentSpaces(अन्तः_स्तराः(cp + 1)));
            }
            भङ्ग;
        पक्ष ' ': wordstart = सत्य; भङ्ग;
        पक्ष '\t': wordstart = सत्य; भङ्ग;
        पक्ष '"':
            wordstart = असत्य;
            यदि (!was_अन्तर्_Backslash) m_अन्तर्_String_Literal = !m_अन्तर्_String_Literal;
            भङ्ग;
        पक्ष '\\':
            wordstart = असत्य;
            यदि (!was_अन्तर्_Backslash) m_अन्तर्_Backslash = सत्य;
            भङ्ग;
        पक्ष '/':
            यदि (m_lang == LA_C || m_lang == LA_VERILOG) {
                यदि (cp > strg && cp[-1] == '/' && !m_अन्तर्_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    यावत् (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    यदि (*cp) putcNoTracking(*cp);
                }
            }
            भङ्ग;
        पक्ष '{':
            यदि (!m_अन्तर्_String_Literal) {
                यदि (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.प्रक्षिप(m_indentLevel
                                    * m_खण्डम्_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            भङ्ग;
        पक्ष '}':
            यदि (!m_अन्तर्_String_Literal) {
                यदि (m_bracketLevel > 0) {
                    m_parenVec.निष्कासय();
                    --m_bracketLevel;
                }
                indentDec();
            }
            भङ्ग;
        पक्ष '(':
            यदि (!m_अन्तर्_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.प्रक्षिप(m_column);
            }
            भङ्ग;
        पक्ष ')':
            यदि (!m_अन्तर्_String_Literal) {
                यदि (!m_parenVec.रिक्तानि()) m_parenVec.निष्कासय();
                indentDec();
            }
            भङ्ग;
        पक्ष '<':
            यदि (m_lang == LA_XML) {
                यदि (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } अन्यथा यदि (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } अन्यथा {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            भङ्ग;
        पक्ष '>':
            यदि (m_lang == LA_XML) {
                indentDec();
                यदि (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            भङ्ग;
        स्वभाव: wordstart = असत्य; भङ्ग;
        }

        विकल्प (*cp) {
        पक्ष '=': equalsForBracket = सत्य; भङ्ग;
        पक्ष ' ': भङ्ग;
        स्वभाव: equalsForBracket = असत्य; भङ्ग;
        }
    }
}

शून्य V3_बहिर्गमनम्_Formatter::put_Break_व्यञ्जकम्() {
    यदि (!m_parenVec.रिक्तानि()) putBreak();
}

// Add a line break if too wide
शून्य V3_बहिर्गमनम्_Formatter::putBreak() {
    यदि (!v3Global.opt.decoration()) प्रत्यागम;
    यदि (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        यदि (exceeded_विस्तारः()) {
            putcNoTracking('\n');
            यदि (!m_parenVec.रिक्तानि()) putsNoTracking(indentSpaces(m_parenVec.शिखरम्()));
        }
    }
}

शून्य V3_बहिर्गमनम्_Formatter::putsQuoted(ध्रुव string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ध्रुव string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    प्रति (ध्रुव वर्ण c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    यदि (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
शून्य V3_बहिर्गमनम्_Formatter::putsNoTracking(ध्रुव string& strg) {
    यदि (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        प्रत्यागम;
    }
    // Don't track {}'s, probably because it's a $display format string
    प्रति (ध्रुव वर्ण c : strg) putcNoTracking(c);
}

शून्य V3_बहिर्गमनम्_Formatter::putcNoTracking(वर्ण chr) {
    यदि (!v3Global.opt.decoration()) {
        putcOutput(chr);
        प्रत्यागम;
    }
    विकल्प (chr) {
    पक्ष '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = सत्य;
        भङ्ग;
    पक्ष '\t': m_column = ((m_column + 9) / 8) * 8; भङ्ग;
    पक्ष ' ':
    पक्ष '(':
    पक्ष '|':
    पक्ष '&': ++m_column; भङ्ग;
    स्वभाव:
        ++m_column;
        m_nobreak = असत्य;
        भङ्ग;
    }
    putcOutput(chr);
}

string V3_बहिर्गमनम्_Formatter::quoteNameControls(ध्रुव string& namein,
                                         V3_बहिर्गमनम्_Formatter::Language lang) VL_विशुद्धम् {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string बहिर्गमनम्;
    यदि (lang == LA_XML) {
        // Encode chars into XML string
        प्रति (ध्रुव वर्ण c : namein) {
            यदि (c == '"') {
                बहिर्गमनम् += "&quot;"s;
            } अन्यथा यदि (c == '\'') {
                बहिर्गमनम् += "&apos;"s;
            } अन्यथा यदि (c == '<') {
                बहिर्गमनम् += "&lt;"s;
            } अन्यथा यदि (c == '>') {
                बहिर्गमनम् += "&gt;"s;
            } अन्यथा यदि (c == '&') {
                बहिर्गमनम् += "&amp;"s;
            } अन्यथा यदि (std::isprint(c)) {
                बहिर्गमनम् += c;
            } अन्यथा {
                बहिर्गमनम् += "&#"s + cvt_गन्तव्यम्_Str(static_cast<अचिह्नित पूर्णाङ्क>(c & 0xff)) + ";";
            }
        }
    } अन्यथा {
        // Encode control chars into C style escapes
        प्रति (ध्रुव वर्ण c : namein) {
            यदि (c == '\\' || c == '"') {
                बहिर्गमनम् += "\\"s + c;
            } अन्यथा यदि (c == '\n') {
                बहिर्गमनम् += "\\n";
            } अन्यथा यदि (c == '\r') {
                बहिर्गमनम् += "\\r";
            } अन्यथा यदि (c == '\t') {
                बहिर्गमनम् += "\\t";
            } अन्यथा यदि (std::isprint(c)) {
                बहिर्गमनम् += c;
            } अन्यथा {
                // This will also cover \a etc
                ध्रुव string octal
                    = "\\"s + cvt_गन्तव्यम्_Str((c >> 6) & 3) + cvt_गन्तव्यम्_Str((c >> 3) & 7) + cvt_गन्तव्यम्_Str(c & 7);
                बहिर्गमनम् += octal;
            }
        }
    }
    प्रत्यागम बहिर्गमनम्;
}

//----------------------------------------------------------------------
// Simple wrappers

शून्य V3_बहिर्गमनम्_Formatter::प्रारूप_लिख(ध्रुव वर्ण* fmt...) {
    constexpr आकार_t bufsize = 5000;
    वर्ण sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_अन्तः(ap);
    एतत्->पङ्क्तिं_लिख(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_बहिर्गमनम्_File::V3_बहिर्गमनम्_File(ध्रुव string& filename, V3_बहिर्गमनम्_Formatter::Language lang)
    : V3_बहिर्गमनम्_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{नूतन std::array<वर्ण, WRITE_बफर_आकार_अष्टकानि>{}} {
    यदि ((m_fp = V3File::new_fopen_w(filename)) == शून्यसूचकः) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_बहिर्गमनम्_File::~V3_बहिर्गमनम्_File() {
    write_खण्डम्();

    यदि (m_fp) fclose(m_fp);
    m_fp = शून्यसूचकः;
}

शून्य V3_बहिर्गमनम्_File::putsForceIncs() {
    ध्रुव VStringList& forceIncs = v3Global.opt.forceIncs();
    प्रति (ध्रुव string& i : forceIncs) पङ्क्तिं_लिख("#include \"" + i + "\"\n");
}

शून्य V3_बहिर्गमनम्_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = सत्य;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    प्रति (वर्ण& c : var) {
        यदि (!std::अक्षरम्_अङ्कः_वा(c)) c = '_';
    }
    पङ्क्तिं_लिख("\n#ifndef " + var + "\n");
    पङ्क्तिं_लिख("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_बहिर्गमनम्_प्रवाहः::V3_बहिर्गमनम्_प्रवाहः(std::ostream& ostream, V3_बहिर्गमनम्_Formatter::Language lang)
    : V3_बहिर्गमनम्_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

वर्ग VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ध्रुव std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_समुच्चयः<std::string> m_new_Id_समुच्चयः VL_GUARDED_BY(m_mutex);  // Which new names exist
रक्षित:
    // CONSTRUCTORS
    मित्रम् वर्ग VIdProtect;
    स्थिर VIdProtectImp& singleton() VL_MT_SAFE {
        स्थिर VIdProtectImp s;
        प्रत्यागम s;
    }

सार्वजनिक:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = स्वभाव;
    // METHODS
    string passthru(ध्रुव string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        यदि (!v3Global.opt.protectIds()) प्रत्यागम old;
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        ध्रुव स्वचालित it = m_nameMap.find(old);
        यदि (it != m_nameMap.अन्तः_2()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } अन्यथा {
            m_nameMap.emplace(old, old);
            m_new_Id_समुच्चयः.निवेशय(old);
        }
        प्रत्यागम old;
    }
    string protectIf(ध्रुव string& old, तार्किक doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        यदि (!v3Global.opt.protectIds() || old.रिक्तानि() || !doIt) प्रत्यागम old;
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        ध्रुव स्वचालित pair = m_nameMap.emplace(old, "");
        यदि (pair.second) {
            string बहिर्गमनम्;
            यदि (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                बहिर्गमनम् = "PS" + old;
            } अन्यथा {
                VHashSha512 digest{v3Global.opt.protect_कुञ्चिका_Defaulted()};
                digest.निवेशय(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                बहिर्गमनम् = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                प्रति (आकार_t len = 6; len < बहिर्गमनम्.आकार() - 3; len += 3) {
                    ध्रुव string tryout = बहिर्गमनम्.substr(0, len);
                    यदि (m_new_Id_समुच्चयः.निवेशय(tryout).second) {
                        बहिर्गमनम् = tryout;
                        भङ्ग;
                    }
                }
            }
            pair.first->second = बहिर्गमनम्;
        }
        प्रत्यागम pair.first->second;
    }
    string protect_शब्दाः_If(ध्रुव string& old, तार्किक doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        यदि (!(doIt && v3Global.opt.protectIds())) प्रत्यागम old;
        string बहिर्गमनम्;
        string::आकार_type start = 0;
        // space, ., ->
        यावत् (सत्य) {
            // When C++11, use find_if and lambda
            string::आकार_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            यदि (pos == string::npos) भङ्ग;
            बहिर्गमनम् += protectIf(old.substr(start, pos - start), सत्य) + separator;
            start = pos + separator.दीर्घता_2();
        }
        बहिर्गमनम् += protectIf(old.substr(start), सत्य);
        प्रत्यागम बहिर्गमनम्;
    }
    शून्य writeMapFile(ध्रुव string& filename) ध्रुव {
        V3_बहिर्गमनम्_Xml_File सम्बन्धि{filename};
        सम्बन्धि.puts_शीर्षकम्();
        सम्बन्धि.पङ्क्तिं_लिख("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        सम्बन्धि.पङ्क्तिं_लिख("<verilator_id_map>\n");
        {
            प्रति (ध्रुव स्वचालित& itr : m_nameMap) {
                सम्बन्धि.पङ्क्तिं_लिख("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        सम्बन्धि.पङ्क्तिं_लिख("</verilator_id_map>\n");
    }

गुप्त:
    शून्य trySep(ध्रुव string& old, string::आकार_type start, ध्रुव string& trySep,
                string::आकार_type& posr, string& separatorr) VL_विशुद्धम् {
        ध्रुव string::आकार_type trypos = old.find(trySep, start);
        यदि (trypos != string::npos) {
            यदि (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ध्रुव string& अन्तर्) VL_MT_SAFE {
    प्रत्यागम v3Global.opt.protectIds() ? "" : अन्तर्;
}
string VIdProtect::protectIf(ध्रुव string& old, तार्किक doIt) VL_MT_SAFE {
    प्रत्यागम VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_शब्दाः_If(ध्रुव string& old, तार्किक doIt) VL_MT_SAFE {
    प्रत्यागम VIdProtectImp::singleton().protect_शब्दाः_If(old, doIt);
}
शून्य VIdProtect::writeMapFile(ध्रुव string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
