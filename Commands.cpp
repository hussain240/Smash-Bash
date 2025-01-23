#include <unistd.h>
#include <string.h>
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

/// unsplitLineNoFirstSpace func:
string unsplitLineNoFirstSpace(const vector<string>& words) {
    // Create an output string stream
    ostringstream oss;

    // Iterate through the words in the vector
    for (size_t i = 0; i < words.size(); ++i) {
        // If it's not the first word, add a space before the word
        if (i != 0) {
            oss << " ";
        }
        // Add the current word
        oss << words[i];
    }

    // Convert stream to string and return
    return oss.str();
}

/// stringToInt func:
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

/// checkIfLegalSIGNAL func:
int checkIfLegalSIGNAL(const std::string& str) {
    // Check if string is at least 2 characters long
    if (str.length() < 2) {
        return -1;
    }

    // Extract numeric part of the string
    string numberPart = str.substr(1);

    // Try to convert the number part to an integer
    try {
        int num = stoi(numberPart);
        return num;
    } catch (...) {
        return -1; // Return -1 if conversion fails
    }
}

/// isReservedAlias func:
bool isReservedAlias(const string& name, const vector<pair<string,string>>& commands) {
    int size = commands.size();
    for(int i=0; i < size; i++)
    if (commands[i].first == name) {
        return true;
    }
    return false;
}

/// getValueByKey func:
string getValueByKey(const string& name, const vector<pair<string,string>>& commands) {
    int size = commands.size();
    for(int i=0; i < size; i++)
        if (commands[i].first == name) {
            return commands[i].second;
        }
    return "";
}

/// isReservedKeyword func:
bool isReservedKeyword(const string& name, const vector<pair<string,string>>& commands) {
    if(name == "chprompt" || name == "pwd" || name == "showpid" || name == "cd" || name == "jobs"
       || name == "quit" || name == "fg" || name == "kill" || name == "alias" || name == "unalias"){
        return true;
    }
    return isReservedAlias(name,commands);
}

/// isValidAliasName func:
bool isValidAliasName(const string& name) {
    // Ensure the name contains only valid characters
    return regex_match(name, regex("^[a-zA-Z0-9_]+$"));
}

/// extractAlias func:
bool extractAlias(const string& input, string& name, string& command) {
    string pattern = R"(^alias [a-zA-Z0-9_]+='[^']*'$)";
    regex alias_regex(pattern);

    if(!regex_match(input, alias_regex)){
        return false;
    }

    // Check if the input starts with "alias "
    if (input.find("alias ") != 0) {
        return false; // Must start with "alias "
    }

    string cleanLine = input;
    while (cleanLine.back() == ' '){
        cleanLine.pop_back();
    }
    if(cleanLine.back() != '\''){
        return false;
    }

    // Remove "alias " prefix
    string aliasPart = input.substr(6);

    // Find position of '=' and quotes
    size_t equalPos = aliasPart.find('=');
    size_t startQuote = aliasPart.find('\'', equalPos);
    size_t endQuote = aliasPart.rfind('\'');

    // Validate the positions and ensure they form a valid format
    if (equalPos == string::npos || startQuote == string::npos || endQuote == string::npos || endQuote <= startQuote) {
        return false; // Invalid format
    }

    // Ensure there are no spaces around the '='
    if ((equalPos > 0 && aliasPart[equalPos - 1] == ' ') ||
        (equalPos + 1 < aliasPart.size() && aliasPart[equalPos + 1] == ' ')) {
        return false; // Spaces around '=' are not allowed
    }

    // Extract the name part before '='
    name = aliasPart.substr(0, equalPos);

    // Trim spaces from the name
    name.erase(0, name.find_first_not_of(" \t"));
    name.erase(name.find_last_not_of(" \t") + 1);

    // Ensure the name is not empty and contains no spaces
    if (name.empty() || name.find(' ') != string::npos) {
        return false; // Name cannot be empty or contain spaces
    }

    // Extract the command part between the single quotes
    command = aliasPart.substr(startQuote + 1, endQuote - startQuote - 1);

    return isValidAliasName(name);
}

/// splitRedirection func:
bool splitRedirection(const string& line, string& before, string& after) {
    // Find the position of '>>' first (to prioritize double redirection)
    int posDouble = line.find(">>");
    int posSingle = line.find('>');

    // Determine the redirection position
    int pos = -1;  // Default to not found
    if (posDouble != -1) {
        pos = posDouble;  // '>>' found
    } else if (posSingle != -1) {
        pos = posSingle;  // '>' found
    }

    if (pos != -1) {
        // Everything before the redirection operator
        before = line.substr(0, pos);

        // Everything after the redirection operator (skip over '>' or '>>')
        after = line.substr(pos + (posDouble != -1 ? 2 : 1));
    } else {
        // No redirection found, the whole line is before and empty after
        before = line;
        after = "";
    }

    bool isDouble = false;
    if(posDouble != -1){
        isDouble = true;
    }
    return isDouble;
}

/// pipeCommands func:
bool pipeCommands(const string& line, string& cmd1, string& cmd2) {
    size_t pipePos = line.find('|');
    bool redirectStderr = false;
    if (pipePos != string::npos && line[pipePos + 1] == '&') {
        redirectStderr = true;
        pipePos++;
    }

    if (pipePos != string::npos) {
        cmd1 = line.substr(0, pipePos);
        cmd2 = line.substr(pipePos + 1);
    }

    return redirectStderr;
}

/// checkIO func:
bool checkIO(string line){
    int x = line.find('>');
    if(x == -1){
        return false;
    }
    return true;
}

bool checkPipe(string line){
    int x = line.find('|');
    if(x != -1){
        return true;
    }
    return false;
}

/// printMap func:
void printMap(const vector<pair<string,string>>& commands) {
    int size = commands.size();
    if (size == 0) {
        return;
    }

    for (int i=0; i<size; i++) {
        cout << commands[i].first << "='" << commands[i].second << "'" << endl;
    }
}

/// printNetworkInfo func:
void printNetworkInfo(const string interface) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "smash error: netinfo: unable to create socket" << std::endl;
        return;
    }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    // Get IP Address
    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
        std::cerr << "smash error: netinfo: interface " << interface << " does not exist" << std::endl;
        close(sock);
        return;
    }
    struct sockaddr_in *ipaddr = (struct sockaddr_in *)&ifr.ifr_addr;
    std::string ipAddress = inet_ntoa(ipaddr->sin_addr);

    // Get Subnet Mask
    if (ioctl(sock, SIOCGIFNETMASK, &ifr) < 0) {
        std::cerr << "smash error: netinfo: could not retrieve subnet mask" << std::endl;
        close(sock);
        return;
    }
    struct sockaddr_in *netmask = (struct sockaddr_in *)&ifr.ifr_netmask;
    std::string subnetMask = inet_ntoa(netmask->sin_addr);

    close(sock);

    // Get Default Gateway by reading /proc/net/route using system calls
    std::string gateway = "N/A";
    int routeFile = open("/proc/net/route", O_RDONLY);
    if (routeFile != -1) {
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(routeFile, buffer, sizeof(buffer))) > 0) {
            std::string line(buffer, bytesRead);
            size_t pos = line.find(interface);
            if (pos != std::string::npos) {
                // Find gateway address in the same line
                size_t gatewayPos = line.find_first_of("\t", line.find("00000000"));
                if (gatewayPos != std::string::npos) {
                    std::string gatewayHex = line.substr(gatewayPos + 1, 8);
                    unsigned long gatewayDec = std::stoul(gatewayHex, nullptr, 16);
                    struct in_addr gwAddr;
                    gwAddr.s_addr = gatewayDec;
                    gateway = inet_ntoa(gwAddr);
                }
                break;
            }
        }
        close(routeFile);
    }

    // Get DNS Servers by reading /etc/resolv.conf using system calls
    std::string dnsServers = "N/A";
    int resolvFile = open("/etc/resolv.conf", O_RDONLY);
    if (resolvFile != -1) {
        char buffer[1024];
        ssize_t bytesRead;
        std::string dnsList;
        while ((bytesRead = read(resolvFile, buffer, sizeof(buffer))) > 0) {
            std::string line(buffer, bytesRead);
            if (line.find("nameserver") == 0) {
                dnsList += line.substr(11); // Remove "nameserver "
            }
        }
        close(resolvFile);
        if (!dnsList.empty()) {
            dnsServers = dnsList;
        }
    }

    // Print the network details
    std::cout << "IP Address: " << ipAddress << std::endl;
    std::cout << "Subnet Mask: " << subnetMask << std::endl;
    std::cout << "Default Gateway: " << gateway << std::endl;
    std::cout << "DNS Servers: " << dnsServers << std::endl;
}

/// readDirectory func:
void readDirectory(const std::string& directoryPath, std::vector<std::string>& files) {
    int pipefd[2];


    if (pipe(pipefd) == -1) {
        perror("smash error: Pipe failed");
        return;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (pid == 0) {

        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);

        char* const args[] = {
                (char*)"ls",
                (char*)"-1",
                (char*)directoryPath.c_str(),
                nullptr
        };

        execvp(args[0], args);

        perror("smash error: execute failed");
        exit(1);
    } else {

        close(pipefd[1]);


        char buffer[1024];
        ssize_t bytesRead;

        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';


            char* line = strtok(buffer, "\n");
            while (line != nullptr) {
                std::string filename(line);


                if (!filename.empty()) {
                    files.push_back(filename);
                }

                line = strtok(nullptr, "\n");
            }
        }


        close(pipefd[0]);


        waitpid(pid, nullptr, 0);
    }
}

/// listDirectory func:
void listDirectory(const std::string& dirPath, int depth) {
    std::vector<std::string> entries;
    readDirectory(dirPath, entries);


    std::sort(entries.begin(), entries.end());


    std::vector<std::string> directories;
    std::vector<std::string> files;
    for (const auto& entry : entries) {
        if (entry == "." || entry == "..") continue;
        struct stat entryStat;
        std::string fullPath = dirPath + "/" + entry;
        if (stat(fullPath.c_str(), &entryStat) == 0) {
            if (S_ISDIR(entryStat.st_mode)) {
                directories.push_back(entry);
            } else {
                files.push_back(entry);
            }
        }
    }


    for (const auto& dir : directories) {
        for (int i = 0; i < depth; ++i) std::cout << '\t';
        std::cout << dir << std::endl;
        listDirectory(dirPath + "/" + dir, depth + 1);
    }


    for (const auto& file : files) {
        for (int i = 0; i < depth; ++i) std::cout << '\t';
        std::cout << file << std::endl;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////


SmallShell::SmallShell() : smashName("smash"),
                           current_command(""),
                           shellJobs(),
                           aliasCommands(),
                           lastDirectory("smash error: cd: OLDPWD not set"),
                           fgPid(-1){}
SmallShell::~SmallShell() {}


Command::Command(const string cmd_lineP) : cmd_line(cmd_lineP){}
BuiltInCommand::BuiltInCommand(const string cmd_lineP) : Command(cmd_lineP){}
RedirectionCommand::RedirectionCommand(const string cmd_lineP) : Command(cmd_lineP){}
PipeCommand::PipeCommand(const string cmd_lineP) : Command(cmd_lineP){}
NetInfo::NetInfo(const string cmd_lineP) : Command(cmd_lineP) {}
WhoAmICommand::WhoAmICommand(const string cmd_line):Command(cmd_line){}
ListDirCommand:: ListDirCommand(const string cmd_line):Command(cmd_line){}

ExternalCommand::ExternalCommand(const string cmd_lineP,bool is)
: Command(cmd_lineP),isBackground(is){}

ShowPidCommand::ShowPidCommand(string cmd_lineP): BuiltInCommand(cmd_lineP){}
GetCurrDirCommand::GetCurrDirCommand(string cmd_lineP): BuiltInCommand(cmd_lineP){}

ChangeDirCommand::ChangeDirCommand(string cmd_lineP): BuiltInCommand(cmd_lineP){}

JobsCommand::JobsCommand(string cmd_lineP): BuiltInCommand(cmd_lineP){}

KillCommand::KillCommand(const string cmd_lineP): BuiltInCommand(cmd_lineP){}

QuitCommand::QuitCommand(const string cmd_line): BuiltInCommand(cmd_line){}

ForegroundCommand::ForegroundCommand(const string cmd_lineP):BuiltInCommand(cmd_lineP) {}

aliasCommand::aliasCommand(const string cmd_lineP):BuiltInCommand(cmd_lineP) {}

unaliasCommand::unaliasCommand(const string cmd_lineP):BuiltInCommand(cmd_lineP){}


/// Creates and returns a pointer to Command class which matches the given command line (cmd_line)
Command *SmallShell::CreateCommand(const string cmd_line) {
    string cleanLine = cmd_line;
    bool is = false;
    while (!cleanLine.empty() && cleanLine.back() == ' '){
        cleanLine.pop_back();
    }
    if(!cleanLine.empty() && cleanLine.back() == '&'){
        cleanLine.pop_back();
        is = true;
    }

    vector<string> words = splitLine(cleanLine); /// split line
    if(words[0].compare("alias")==0){
        return new aliasCommand(cmd_line);
    }else if(checkIO(cleanLine)){
        return new RedirectionCommand(cleanLine);
    } else if(checkPipe(cmd_line)){
        return new PipeCommand(cleanLine);
    } else if (words[0].compare("chprompt") == 0) {
        /// if we removed the & from the second we should return it
        if(words.size() == 2 && is){
            words[2] += "&";
        }
        /// for chprompt:
        string theWord="smash";
        if(words.size() > 1) /// change smash(even if already changed)
        {
            theWord = words[1];
        }
        smashName = theWord;
        return nullptr;
    }else if (words[0].compare("pwd") == 0 || words[0].compare("pwd&") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }else if (words[0].compare("showpid") == 0 || words[0].compare("showpid&") == 0) {
        return new ShowPidCommand(cmd_line);
    }else if (words[0].compare("cd") == 0){
        return new ChangeDirCommand(cmd_line);
    }else if (words[0].compare("jobs") == 0 || words[0].compare("jobs&") == 0){
        return new JobsCommand(cmd_line);
    }else if (words[0].compare("quit") == 0){
        return new QuitCommand(cmd_line);
    }else if(words[0].compare("fg")==0){
        return new ForegroundCommand(cmd_line);
    }else if(words[0].compare("kill")==0){
        return new KillCommand(cmd_line);
    }else if(words[0].compare("unalias")==0){
        return new unaliasCommand(cleanLine);
    }else if(words[0].compare("netinfo")==0){
        return new NetInfo(cleanLine);
    }else if(words[0].compare("listdir")==0){
        return new ListDirCommand(cleanLine);
    }else if(words[0].compare("whoami")==0){
        return new WhoAmICommand(cleanLine);
    }else if(isReservedAlias(words[0], aliasCommands)){
        words[0] = getValueByKey(words[0], aliasCommands);

        string new_line = unsplitLineNoFirstSpace(words);
        if(is){
            new_line.push_back('&');
        }
        Command* cmd = CreateCommand(new_line);
        return cmd;
    }
    return new ExternalCommand(cmd_line,is);
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
       perror("smash error: getcwd failed");
    }
}
/// cd:
void ChangeDirCommand::execute(){
    SmallShell &smash = SmallShell::getInstance();
    string cleanLine = cmd_line;
    while (!cleanLine.empty() && cleanLine.back() == ' '){
        cleanLine.pop_back();
    }
    if(!cleanLine.empty() && cleanLine.back() == '&'){
        cleanLine.pop_back();
    }
    
    vector<string> words = splitLine(cleanLine); /// split line
    if(words.size() == 1){
        return;
    }
    if(words.size() > 2){
        cerr << "smash error: cd: too many arguments" << endl;
    } else{
        /// get current path:
        char cwd[PATH_MAX];
        string currentPath = "Error";
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            currentPath = cwd;
        } else{
            perror("smash error: getcwd failed");
            return;
        }

        string lastDir = smash.getLastDirectory();
        if(words[1] == "-"){
            /// check if we can go back
            if(lastDir == "smash error: cd: OLDPWD not set"){
                cerr << lastDir << endl;
            }else{
                /// go to folder:
                if (chdir(lastDir.c_str()) != 0) {
                    perror("smash error: chdir failed");
                }
                smash.getLastDirectory() = currentPath; /// modify last path ptr
            }
        } else{ /// its a path
            /// go to folder:
            if (chdir(words[1].c_str()) != 0) {
                perror("smash error: chdir failed");
            }else if(smash.getLastDirectory() != currentPath){
				smash.getLastDirectory() = currentPath; /// modify last path ptr
			}
        }
    }
}
/// jobs:
void JobsCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    smash.getShellJobs().printJobsList();
}
/// quit kill:
void QuitCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    string cleanLine = cmd_line;
    if(cmd_line.back() == '&'){
        cleanLine.pop_back();
    }

    vector<string> words = splitLine(cleanLine); /// split line

    if(words.size() > 1 && (words[1].compare("kill") == 0)){
        smash.getShellJobs().killAllJobs();
    }
    exit(0);
}
/// kill:
void KillCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    string cleanLine = cmd_line;
    if(cmd_line.back() == '&'){
        cleanLine.pop_back();
    }

    vector<string> words = splitLine(cleanLine);
    if(words.size()!= 3)
    {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int num=checkIfLegalSIGNAL(words[1]);
    if(num==-1 || (words[1][0] != '-'))
    {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int id= stringToInt(words[2]);
    if(id==-1) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if(smash.getShellJobs().jobs.size() == 0) {
        cerr << "smash error: kill: job-id " << id << " does not exist" << endl;
        return;
    }
    for(auto job : smash.getShellJobs().jobs) {
        if(job.jobId==id) {
			cout << "signal number " << num << " was sent to pid " << job.jobPid << endl;
            if (kill(job.jobPid, num) == -1) {
                perror("smash error: kill failed");
                return;
            }
            return;
        }
    }
    cerr << "smash error: kill: job-id " << id << " does not exist" << endl;
}
/// fg:
void ForegroundCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    string cleanLine = cmd_line;
    if(cmd_line.back() == '&'){
        cleanLine.pop_back();
    }
    vector<string> words = splitLine(cleanLine);

    if(words.size() == 1){ /// we go for the max id (the last in list)
        if(smash.getShellJobs().jobs.size()==0){
            cerr <<"smash error: fg: jobs list is empty" << endl;
            return;
        }

        cout << smash.getShellJobs().jobs.back().jobName << " " << smash.getShellJobs().jobs.back().jobPid << endl;
        smash.setFgPid(smash.getShellJobs().jobs.back().jobPid);
        if (waitpid(smash.getShellJobs().jobs.back().jobPid, nullptr, 0) == -1) {
            // I don't know if we supposed to print here //
            perror("smash error: waitpid failed");
        }
        smash.getShellJobs().removeJobById(smash.getShellJobs().jobs.back().jobId);
        smash.setFgPid(-1);
        return;
    }

    if(words.size() == 2) {

        int id = stringToInt(words[1]);

        if (id < 0) {
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }

        int size = smash.getShellJobs().jobs.size();
        for (int i = 0; i < size; i++) {
            if (smash.getShellJobs().jobs[i].jobId == id) {
                smash.setFgPid(smash.getShellJobs().jobs[i].jobPid);
                cout << smash.getShellJobs().jobs[i].jobName << " " << smash.getShellJobs().jobs[i].jobPid << endl;
                if (waitpid(smash.getShellJobs().jobs[i].jobPid, nullptr, 0) == -1) {
                    perror("smash error: waitpid failed");
                }
                smash.setFgPid(-1);
                smash.getShellJobs().removeJobById(smash.getShellJobs().jobs[i].jobId);
                return;
            }
        }
        cerr << "smash error: fg: job-id " << id << " does not exist" << endl;
        return;
    }

    cerr << "smash error: fg: invalid arguments" << endl;
}
/// alias:
void aliasCommand::execute() {
    vector<string> words = splitLine(cmd_line); /// split line
    SmallShell &smash = SmallShell::getInstance();
    if(words.size() == 1){
        printMap(smash.getAliasCommands());
        return;
    }

    string name,command;
	string cleanLine = cmd_line;
    while (!cleanLine.empty() && cleanLine.back() == ' '){
        cleanLine.pop_back();
    }
    if(!cleanLine.empty() && cleanLine.back() == '&'){
        cleanLine.pop_back();
    }
    while (!cleanLine.empty() && cleanLine.back() == ' '){
        cleanLine.pop_back();
    }
    
    if(!extractAlias(cleanLine, name, command)){
        cerr << "smash error: alias: invalid alias format" << endl;
        return;
    }

    if(isReservedKeyword(name,smash.getAliasCommands())){
        cerr << "smash error: alias: " << name << " already exists or is a reserved command" << endl;
        return;
    }

    smash.getAliasCommands().push_back({name,command});
}
/// unalias:
void unaliasCommand::execute() {
    vector<string> words = splitLine(cmd_line);
    SmallShell &smash = SmallShell::getInstance();
    if(words.size()<2)
    {
        cerr << "smash error: unalias: not enough arguments" << endl;
    }
    int wsize = words.size();
    for(int i=1;i<wsize;i++)
    {
        bool isCommand=false;
        int size = smash.getAliasCommands().size();
        for(int j=0;j<size;j++)
        {
            if(words[i] == smash.getAliasCommands()[j].first)
            {
                isCommand = true;
                smash.getAliasCommands().erase(smash.getAliasCommands().begin()+j);
                break;
            }
        }

        if(!isCommand)
        {
            std::cerr<< "smash error: unalias: "<< words[i] <<" alias does not exist" <<std::endl;
            return;
        }
    }
    return;
}

/// Special:
/// xxx >>> yyy
void RedirectionCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();

    // Split the line into before and after redirection
    string before, after;
    bool isDou = splitRedirection(cmd_line, before, after);

    vector<string> afterWords = splitLine(after); // Split the after part
	
	if(afterWords.empty()){
		afterWords.push_back("");
		}
    // Open the file for redirection
    int file;
    if (isDou) {
        file = open(afterWords[0].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (file < 0) {
            perror("smash error: open failed");
            return;
        }
    } else {
        file = open(afterWords[0].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (file < 0) {
            perror("smash error: open failed");
            return;
        }
    }



    // Save the original stdout
    int saved_stdout = dup(1);
    if (saved_stdout == -1) {
        perror("smash error: dup failed");
        close(file);
        return;
    }

    // Redirect stdout to the file
    if (dup2(file, 1) == -1) {
        perror("smash error: dup2 failed");
        close(file);
        close(saved_stdout);
        return;
    }

    close(file); // Close the original file descriptor, not needed anymore

    // Execute the Command
    Command *cmd = smash.CreateCommand(before);
    cmd->execute();

    // Restore the original stdout
    if (dup2(saved_stdout, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
    }
    close(saved_stdout);
}
/// xxx |& yyy
void PipeCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    string cmd1,cmd2;
    bool redirectStderr;
    redirectStderr = pipeCommands(cmd_line,cmd1,cmd2);

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (pid1 == 0) { // Child process for command1
        setpgrp(); /// change its group.
        if (redirectStderr) {
            if(dup2(pipefd[1], STDERR_FILENO) == -1){
                perror("smash error: dup2 failed");
            }
        } else {
            if(dup2(pipefd[1], STDOUT_FILENO) == -1){
                perror("smash error: dup2 failed");
            }
        }
        close(pipefd[0]);
        close(pipefd[1]);

        /// exe here
        Command *cmd = smash.CreateCommand(cmd1);
        if(cmd != nullptr){
            cmd->execute();
        }
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (pid2 == 0) { // Child process for command2
        setpgrp(); /// change its group.
        dup2(pipefd[0], STDIN_FILENO); // Redirect pipe to stdin
        close(pipefd[0]);
        close(pipefd[1]);

        /// exe here
        Command *cmd = smash.CreateCommand(cmd2);
        if(cmd != nullptr){
            cmd->execute();
        }
        exit(0);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, nullptr, 0); // Wait for command1
    waitpid(pid2, nullptr, 0); // Wait for command2
}
/// netinfo:
void NetInfo::execute() {
    vector<string> words = splitLine(cmd_line); /// split line
    if (words.size() == 1) {
        std::cerr << "smash error: netinfo: interface not specified" << std::endl;
        return;
    }
    printNetworkInfo(words[1]);
}
/// whoami:
void WhoAmICommand::execute() {
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    char buffer[1024];
    int bytesRead;
    string line;

    while ((bytesRead = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        for (int i = 0; i < bytesRead; ++i) {
            if (buffer[i] == '\n') {
                stringstream ss(line);
                string username, password, uid, gid, fullname, home_dir, shell;

                getline(ss, username, ':');
                getline(ss, password, ':');
                getline(ss, uid, ':');
                getline(ss, gid, ':');
                getline(ss, fullname, ':');
                getline(ss, home_dir, ':');
                getline(ss, shell, ':');

                int x = stoi(uid);
                int y = getuid();
                if (x == y) {
                    cout << username << " " << home_dir << std::endl;
                    close(fd);
                    return;
                }

                line.clear();
            } else {
                line += buffer[i];
            }
        }
    }

    if (bytesRead == -1) {
        perror("smash error: read failed");
    }

    close(fd);
}
/// listdir:
void ListDirCommand::execute() {

    vector<string> words = splitLine(cmd_line);
    if(words.size()!=2)
    {
        std::cerr<<"smash error: listdir: too many arguments"<<std::endl;
    }
    int x=0;
    listDirectory(words[1],x);

}

/// ExternalCommand:
void ExternalCommand::execute() {

    string command = cmd_line; /// not modify

    string cleanLine = cmd_line;
    while (cleanLine.back() == ' '){
        cleanLine.pop_back();
    }
    if(cleanLine.back() == '&'){
        cleanLine.pop_back();
    }
    vector<string> words = splitLine(cleanLine); /// split line

    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }

    SmallShell &smash = SmallShell::getInstance();

    if (pid == 0) {
        /// Child process
        setpgrp(); /// change its group.

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
        perror("smash error: execvp failed");
        exit(0);  // Child process exits here
    } else {
        /// Parent process
        if (isBackground) {
            if(smash.getShellJobs().jobs.size() == 0){
                smash.getShellJobs().nextJobId = 1;
            } else{
                smash.getShellJobs().nextJobId = smash.getShellJobs().jobs.back().jobId + 1;
            }
            smash.getShellJobs().addJob(smash.getCurrent_command(), pid, false);
        } else {
            // Wait for child process to finish
            smash.setFgPid(pid);
            if (waitpid(pid, nullptr, 0) == -1) {
                perror("smash error: waitpid failed");
            }
            smash.setFgPid(-1);
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
void JobsList::printJobsListKill(){
    for (size_t i = 0; i < jobs.size(); ++i) {
        cout << jobs[i].jobPid << ": " << jobs[i].jobName << endl;
    }
}

void JobsList::killAllJobs()
{
    cout << "smash: sending SIGKILL signal to " << jobs.size() << " jobs:" << endl;
    printJobsListKill();

    int size = jobs.size();
    for (int i = 0; i < size; i++) {
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
    int size = jobs.size();
    for (int i = 0; i < size; i++) {
        if (jobs[i].jobId == jobId) {
            return &(jobs[i]);
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId){
    int pid = 0;
    int size = jobs.size();
    for(int i=0; i<size; i++){
        if(jobs[i].jobId == jobId){
            pid = jobs[i].jobPid;
            jobs.erase(jobs.begin() + i);
        }
    }

    pid_t result = waitpid(pid, nullptr, WNOHANG);
    if (result == 0) {
        /// Process is still running
        if (kill(pid, SIGKILL) == -1) {
            perror("smash error: kill failed");
        }
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
    SmallShell &smash = SmallShell::getInstance();
    smash.getCurrent_command() = cmd_line;
    Command *cmd = CreateCommand(cmd_line);
    shellJobs.removeFinishedJobs();
    if(cmd != nullptr){
        cmd->execute();
    }
    smash.getCurrent_command() = "";
}

string& SmallShell::getSmashName(){
    return smashName;
}
string& SmallShell::getCurrent_command(){
    return current_command;
}
JobsList& SmallShell::getShellJobs(){
    return shellJobs;
}
vector<pair<string,string>>& SmallShell::getAliasCommands(){
    return aliasCommands;
}
string& SmallShell::getLastDirectory(){
    return lastDirectory;
}
int& SmallShell::getFgPid(){
    return fgPid;
}
void SmallShell::setFgPid(int x){
    fgPid = x;
}

