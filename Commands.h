#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <limits.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

using namespace std;
class JobsList;
#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


vector<string> splitLine(const string& line);
string unsplitLine(const vector<string>& words);
int checkIfLegalSIGNAL(const std::string& str);

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
    shared_ptr<JobsList> jobsPtr;
    ExternalCommand(const string cmd_line, shared_ptr<JobsList> jobsPtr);

    virtual ~ExternalCommand() {
    }

    void execute() override;
};

/*
class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {
    }

    void execute() override;
};
*/

class ChangeDirCommand : public BuiltInCommand {
public:
    shared_ptr<string> lastPathPtr; /// the last so we can go if "cd -"

    ChangeDirCommand(string cmdLine, shared_ptr<string> pLastPwd); /// Constructor

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
    shared_ptr<JobsList>jobsPtr;
    QuitCommand(const string cmd_line,shared_ptr<JobsList> shellJobsPtrP);

    virtual ~QuitCommand() {
    }

    void execute() override;
};


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

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

};

class JobsCommand : public BuiltInCommand {
public:
    shared_ptr<JobsList> jobsPtr;
    JobsCommand(const string cmd_lineP, shared_ptr<JobsList> shellJobsPtrP);

    virtual ~JobsCommand(){}

    void execute() override;
};


class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    shared_ptr<JobsList> jobsPtr;
    KillCommand(const string cmd_lineP, shared_ptr<JobsList> shellJobsPtrP);

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    //fg command
public:
    shared_ptr<JobsList>jobsPtr;
    ForegroundCommand(const string cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};
/*
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
    NetInfo(const string cmd_line);

    virtual ~NetInfo() {
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
    unaliasCommand(const string cmd_line);

    virtual ~unaliasCommand() {
    }

    void execute() override;
};
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///Main class (SmallShell):

class SmallShell {
private:
    shared_ptr<JobsList> shellJobsPtr; /// for background
    shared_ptr<unordered_map<string, string>> alliesCommandsPtr; /// new defined commands (allies)
    shared_ptr<string> lastDirectory;
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

    ~SmallShell();

    void executeCommand(const string cmd_line);

    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
