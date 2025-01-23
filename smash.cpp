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
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    SmallShell &smash = SmallShell::getInstance();

    while (true) {
        cout << smash.getSmashName() << "> ";
        string cmd_line;
        getline(std::cin, cmd_line);
        if(!cmd_line.empty()){
            smash.executeCommand(cmd_line);
        }
    }
    return 0;
}
