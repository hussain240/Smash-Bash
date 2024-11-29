#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

/// splitLine func:
vector<string> splitLine(const string& line) {
    vector<string> words;
    istringstream stream(line);  // Create a stream from the input line
    string word;

    while (stream >> word) {  // Extract words separated by whitespace
        words.push_back(word);
    }
    return words;
}

/// unsplitLine func:
string unsplitLine(const vector<string>& words) {
    // Create an output string stream
    ostringstream oss;

    // Join the words with a space as the delimiter
    for (size_t i = 0; i < words.size(); ++i) {
        oss << words[i];
        if (i != words.size() - 1) {  // Add space after every word except the last
            oss << " ";
        }
    }

    // Convert stream to string and return
    return oss.str();
}



/*string _ltrim(const string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}
*/


SmallShell::SmallShell() : shellJobsPtr(make_shared<JobsList>()),
                            lastDirectory(make_shared<string>("error: cd: OLDPWD not set")) {}
SmallShell::~SmallShell() {
}


Command::Command(const string cmd_lineP) : cmd_line(cmd_lineP){}
BuiltInCommand::BuiltInCommand(const string cmd_lineP) : Command(cmd_lineP){}
ExternalCommand::ExternalCommand(const string cmd_lineP,shared_ptr<JobsList> shellJobsPtrP)
: Command(cmd_lineP), jobsPtr(shellJobsPtrP){}

ShowPidCommand::ShowPidCommand(string cmd_lineP): BuiltInCommand(cmd_lineP){}
GetCurrDirCommand::GetCurrDirCommand(string cmd_lineP): BuiltInCommand(cmd_lineP){}

ChangeDirCommand::ChangeDirCommand(string cmd_lineP, shared_ptr<string> pLastPwd)
        : BuiltInCommand(cmd_lineP), lastPathPtr(pLastPwd){}
JobsCommand::JobsCommand(string cmd_lineP, shared_ptr<JobsList> shellJobsPtrP)
        : BuiltInCommand(cmd_lineP), jobsPtr(shellJobsPtrP){}
KillCommand::KillCommand(const std::string cmd_lineP, shared_ptr<JobsList> shellJobsPtrP)
        : BuiltInCommand(cmd_lineP), jobsPtr(shellJobsPtrP){}
QuitCommand::QuitCommand(const std::string cmd_line, shared_ptr <JobsList> shellJobsPtrP)
: BuiltInCommand(cmd_line), jobsPtr(shellJobsPtrP){}

/// Creates and returns a pointer to Command class which matches the given command line (cmd_line)
Command *SmallShell::CreateCommand(const string cmd_line) {
    vector<string> words = splitLine(cmd_line); /// split line
    if (words[0].compare("pwd") == 0 || words[0].compare("pwd&") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }else if (words[0].compare("showpid") == 0 || words[0].compare("showpid&") == 0) {
        return new ShowPidCommand(cmd_line);
    }else if (words[0].compare("cd") == 0 && words.size() != 1){
        return new ChangeDirCommand(cmd_line, lastDirectory);
    }else if (words[0].compare("jobs") == 0 || words[0].compare("jobs&") == 0){
        return new JobsCommand(cmd_line,shellJobsPtr);
    }else if (words[0].compare("quit") == 0){
        return new QuitCommand(cmd_line,shellJobsPtr);
    }else if(words[0].compare("fg")==0){
        return new ForegroundCommand(cmd_line,shellJobsPtr);
    }else if(words[0].compare("kill")==0){
        return new KillCommand(cmd_line,shellJobsPtr);
    }
    else {
        cout << "\033[35m" << "It's External Command.." << "\033[0m" << endl;
        return new ExternalCommand(cmd_line,shellJobsPtr);
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// Executions:
/// showpid:
void ShowPidCommand::execute(){
    pid_t smash_pid = getpid(); // Get the process ID of the current process
    std::cout << "smash pid is " << smash_pid << std::endl;
}
/// pwd:
void GetCurrDirCommand::execute(){
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << cwd << std::endl; // Print the current working directory
    } else {
        std::cout << "nothing" << std::endl;
    }
}
/// cd:
void ChangeDirCommand::execute(){
    vector<string> words = splitLine(cmd_line); /// split line
    if(words.size() > 2){
        cout << "smash error: cd: too many arguments" << endl;
    } else{
        /// get current path:
        char cwd[PATH_MAX];
        string currentPath = "Error";
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            currentPath = cwd;
        } else{
            cout << "Nothing" << endl;
            return;
        }

        if(words[1] == "-"){
            /// check if we can go back
            if(*lastPathPtr == "error: cd: OLDPWD not set"){
                cout << *lastPathPtr << endl;
            }else{
                string lastPath = lastPathPtr->data();
                /// go to folder:
                if (chdir(lastPath.c_str()) != 0) {
                    perror("smash error: cd failed");
                }

                *lastPathPtr = currentPath; /// modify last path ptr
            }
        } else{ /// its a path
            /// go to folder:
            if (chdir(words[1].c_str()) != 0) {
                perror("smash error: cd failed");
            } else{
                *lastPathPtr = currentPath; /// modify last path ptr
            }
        }
    }
}
/// jobs:
void JobsCommand::execute() {
    jobsPtr->removeFinishedJobs();
    jobsPtr->printJobsList();
}
/// quit kill:
void QuitCommand::execute() {
    jobsPtr->killAllJobs();
    vector<string> words = splitLine(cmd_line); /// split line
    if(words.size() > 1 && (words[1].compare("kill") == 0 || words[1].compare("kill&") == 0)){
        if (kill(getpid(), SIGKILL) == -1) {
            perror("smash error: kill failed");
        }
    }
}
int checkIfLegalSIGNAL(const std::string& str) {
    if (str.length() < 2) {
        return -1;
    }
    std::string numberPart = str.substr(1);
    if (!std::all_of(numberPart.begin(), numberPart.end(), ::isdigit)) {
        return -1;
    }
    int num = std::stoi(numberPart);
    return num;
}
void KillCommand::execute() {
    vector<string> words = splitLine(cmd_line);
    if(words.size()!= 3)
    {
        perror("smash error: kill: invalid arguments");
        return;
    }
    int num=checkIfLegalSIGNAL(words[1]);
    if(num==-1 || words[1][0]!="-")
    {
        perror("smash error: kill: invalid arguments");
        return;
    }
    int id= stringToInt(words[2]);
    if(id==-1)
    {
        perror("smash error: kill: invalid arguments");
        return;
    }
    if(jobsPtr->jobs.size()==0)
    {
        std::cerr << "smash error: kill: job-id " << id << " does not exist" << std::endl;
        return;
    }
    for(auto job : jobsPtr->jobs)
    {
        if(job.jobId==id)
        {
            if (kill(job.jobId, num) == -1) {
                // I don't know if we supposed to print here //
                perror("smash error: kill failed");
            }

            std::cout<<"signal number "<<num<<" was sent to pid "<<job.jobPid<<std::endl;
            return;
        }
    }
    std::cerr << "smash error: kill: job-id " << id << " does not exist" << std::endl;


}

/// ExternalCommand:
void ExternalCommand::execute() {
    string command = cmd_line; /// not modify
    vector<string> words = splitLine(cmd_line); /// split line
    bool isBackground = false;
    // Handle background execution
    if (words[words.size()-1].back() == '&') {
        words[words.size()-1].pop_back();
        command = unsplitLine(words);
        isBackground = true;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (pid == 0) {
        /// Child process
        if (command.find('*') != string::npos || command.find('?') != string::npos) {
            /// Complex command: run using bash -c
            execl("/bin/bash", "bash", "-c", command.c_str(), nullptr);
        } else {
            /// Simple command: split into executable and arguments
            vector<char*> c_args;
            for (const string& arg : words) {
                c_args.push_back(const_cast<char*>(arg.c_str()));
            }
            c_args.push_back(nullptr); // Null-terminate the arguments list

            execvp(c_args[0], c_args.data());
        }
        perror("smash error: exec failed");
    } else {
        /// Parent process
        if (isBackground) {
            jobsPtr->addJob(cmd_line, pid, false);
        } else {
            // Wait for child process to finish
            if (waitpid(pid, nullptr, 0) == -1) {
                perror("smash error: waitpid failed");
            }
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Jobs Funcs:
JobsList::JobsList() : nextJobId(1){}

void JobsList::addJob(string cmd_line,int pidP, bool isStopped){
    if(!isStopped){
        JobEntry newJob;
        newJob.jobId = nextJobId;
        newJob.jobName = cmd_line;
        newJob.jobPid = pidP;

        jobs.push_back(newJob);
        nextJobId++;
    }
}

void JobsList::printJobsList(){
    for (size_t i = 0; i < jobs.size(); ++i) {
        cout << "[" << jobs[i].jobId << "] " << jobs[i].jobName << endl;
    }
}

void JobsList::killAllJobs()
{
    removeFinishedJobs();
    cout << "sending SIGKILL signal to " << jobs.size() << " jobs:" << endl;
    printJobsList();

    for (int i = 0; i < jobs.size(); i++) {
        if (kill(jobs[i].jobPid, SIGKILL) == -1) {
            perror("smash error: kill failed");
        }
    }
    while (!jobs.empty()){
        jobs.pop_back();
    }
}

void JobsList::removeFinishedJobs(){
    for (auto it = jobs.begin(); it != jobs.end(); ) {
        int status;
        pid_t result = waitpid(it->jobPid, &status, WNOHANG);

        if (result == 0) {
            /// Process is still running, move to the next PID
            ++it;
        } else if (result == -1) {
            // Error occurred (e.g., process does not exist), remove from vector
            perror("smash error: waitpid failed");
            it = jobs.erase(it);
        } else{
            it = jobs.erase(it);
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId){
    for (int i = 0; i < jobs.size(); i++) {
        if (jobs[i].jobId == jobId) {
            return &(jobs[i]);
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId){
    int pid=0;
    for(int i=0; i<jobs.size();i++){
        if(jobs[i].jobId == jobId){
            pid = jobs[i].jobPid;
            jobs.erase(jobs.begin() + i);
        }
    }

    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);

    if (result == 0) {
        /// Process is still running
        if (kill(pid, SIGKILL) == -1) {
            perror("smash error: kill failed");
        }
    } else if (result == -1) {
        perror("smash error: waitpid failed");
    }
}

JobsList::JobEntry *JobsList::getLastJob(int *lastJobId){
    if(jobs.empty()){
        return nullptr;
    }
    *lastJobId = jobs.back().jobId;
    return &(jobs.back());
}

JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId){
    for (int i = jobs.size() - 1; i >= 0; --i) {
        int status;
        pid_t result = waitpid(jobs[i].jobPid, &status, WNOHANG);  // Non-blocking check

        if (result == -1) {
            perror("smash error: waitpid failed");
        } else if (WIFSTOPPED(status)) {
            *jobId = jobs[i].jobId;  // Set the jobId to the stopped job's ID
            return &jobs[i];  // Return a pointer to the stopped job entry
        }
    }

    return nullptr;  // Return nullptr if no stopped job was found
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SmallShell::executeCommand(const string cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    if(cmd != nullptr){
        cmd->execute();
    }
    /// Please note that you must fork smash process for some commands (e.g., external commands....)
    /// done in the execute
}
ForegroundCommand::ForegroundCommand(const std::string cmd_line, JobsList *jobs):BuiltInCommand(cmd_lineP),jobsPtr(jobs) {
}
int stringToInt(const std::string& str) {
    try {
        size_t idx;
        int result = std::stoi(str, &idx);
        if (idx != str.length()) {
            throw std::invalid_argument("Invalid input");
        }

        return result;
    } catch (...) {
        return -1;
    }
}
void ForegroundCommand::execute() {
    vector<string> words = splitLine(cmd_line);
    if(words.size()!=2)
    {
        perror("smash error: fg: invalid arguments");
        return;
    }
    if(jobsPtr->jobs.size()==0)
    {
        perror("smash error: fg: jobs list is empty");
        return;
    }
    int num= stringToInt(words[1]);
    if(num==-1)
    {
        perror("smash error: fg: invalid arguments");
        return;
    }
    for(auto job : jobsPtr->jobs)
    {
        if(job.jobId==num)
        {
            std::cout<<job.jobName<<" "<<num<<std::endl;
            if (waitpid(job.jobId, nullptr, 0) == -1) {
                // I don't know if we supposed to print here //
                perror("smash error: waitpid failed");
            }
            jobsPtr->removeJobById(job.jobId);
            return;
        }
    }
    std::cerr << "smash error: fg: job-id " << num << " does not exist" << std::endl;

}