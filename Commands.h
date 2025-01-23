#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <limits.h>
#include <iostream>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <vector>
#include <regex>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdlib>
#include <sys/stat.h>
#include <sstream>

using namespace std;
class JobsList;
#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


class Command {
public:
    string cmd_line;

    Command(const string cmd_lineP);

    virtual ~Command(){}

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const string cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ExternalCommand : public Command {
public:
    bool isBackground;
    ExternalCommand(const string cmd_line,bool is);

    virtual ~ExternalCommand() {
    }

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const string cmd_lineP);

    virtual ~RedirectionCommand() {
    }

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const string cmd_lineP);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:

    ChangeDirCommand(string cmdLine); /// Constructor

    virtual ~ChangeDirCommand() {}

    void execute() override;
}; /// "cd []/-"

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const string cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
}; /// done "pwd"

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const string cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
}; /// done "showpid"


class JobsList;

class QuitCommand : public BuiltInCommand {
    // TODO: Add your data members public:
public:
    QuitCommand(const string cmd_line);

    virtual ~QuitCommand() {
    }

    void execute() override;
}; /// done "quit kill"


class JobsList {
public:
    class JobEntry {
    public:
        int jobId;
        string jobName;
        int jobPid;
    };
    vector<JobEntry> jobs; /// the vector of all jobs
    int nextJobId;

    JobsList();

    ~JobsList() {}

    void addJob(string cmd_line, int pid, bool isStopped = false);

    void printJobsList();
    void printJobsListKill();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

};

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(const string cmd_lineP);

    virtual ~JobsCommand(){}

    void execute() override;
};


class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const string cmd_lineP);

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
public:
    ForegroundCommand(const string cmd_lineP);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

class aliasCommand : public BuiltInCommand {
public:
    aliasCommand(const string cmd_line);

    virtual ~aliasCommand() {
    }

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const string cmd_lineP);

    virtual ~unaliasCommand() {
    }

    void execute() override;
};

class ListDirCommand : public Command {
public:
    ListDirCommand(const string cmd_line);

    virtual ~ListDirCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const string cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class NetInfo : public Command {
    // TODO: Add your data members
public:
    NetInfo(const string cmd_lineP);

    virtual ~NetInfo() {
    }

    void execute() override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///Main class (SmallShell):

class SmallShell {
private:
    string smashName;
    string current_command;
    JobsList shellJobs; /// for background
    vector<pair<string,string>> aliasCommands; /// new defined commands (allies)
    string lastDirectory;
    int fgPid;
    SmallShell();

public:
    Command *CreateCommand(const string cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    string& getSmashName();
    string& getCurrent_command();
    vector<pair<string,string>>& getAliasCommands();
    string& getLastDirectory();
    JobsList& getShellJobs();
    int& getFgPid();
    void setFgPid(int x);

    ~SmallShell();

    void executeCommand(const string cmd_line);

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
