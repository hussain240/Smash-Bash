#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include "Commands.h"
#include "signals.h"
#include <string>
using namespace std;

int main(int argc, char *argv[]) {
    /*
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    */

    /// for chprompt:
    std::string theWord="smash> ";
    std::string prefix="chprompt";

    SmallShell &smash = SmallShell::getInstance();

    while (true) {
        cout << theWord;
        string cmd_line;
        getline(std::cin, cmd_line);

        vector<string> words = splitLine(cmd_line); /// split line
        if(!words.empty()){
            if(words[0] == prefix) /// its chprompt command
            {
                if(words.size() == 1) /// reset smash
                {
                    theWord="smash> ";
                }
                else if(words.size() > 1) /// change smash(even if already changed)
                {
                    theWord = words[1];
                    theWord += "> ";
                }
            }
            else
            {
                smash.executeCommand(cmd_line);
            }
        }
    }
    return 0;
}
