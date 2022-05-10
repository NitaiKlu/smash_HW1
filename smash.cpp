#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char *argv[])
{
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR)
    {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR)
    {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction sigact;
    sigact.sa_flags = SA_RESTART;
    sigact.sa_handler = alarmHandler; 
    sigaction(SIGALRM , &sigact, NULL);
    /**{
        perror("smash error: failed to set sig-alrm handler");
    }**/
    //signal(SIGALRM, alarmHandler);
    SmallShell &smash = SmallShell::getInstance();
    while (smash.isRunning())
    {
        smash.printPtompt();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}