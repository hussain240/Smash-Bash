#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;


void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    SmallShell &smash = SmallShell::getInstance();
    int pid = smash.getFgPid();

    if(pid != -1){
        if (kill(pid, SIGKILL) == -1) {
            perror("smash error: kill failed");
        }
        
        cout << "smash: process " << pid << " was killed" << endl;
        return;
    }
    return;
}
