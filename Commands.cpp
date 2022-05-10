#include "Commands.h"
#define BUILT_IN 1
//**************text parsing**********************
string _ltrim(const std::string &s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args)
{
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;)
    {
        args[i] = (char *)malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

vector<string> _parseCommandLineVector(const char *cmd_line)
{
    FUNC_ENTRY()
    vector<string> args;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;)
    {
        args.push_back(s);
    }
    return args;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line)
{
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos)
    {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&')
    {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}
//**************classes implementation**********************

//**************Command**********************
Command::Command(const char *cmd_line, int type)
{
    char *cmd_line_new = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(cmd_line_new, cmd_line);
    cmd_line_str = string(cmd_line_new);
    if (type == BUILT_IN && _isBackgroundComamnd(cmd_line))
    {
        _removeBackgroundSign(cmd_line_new);
        args = _parseCommandLineVector(cmd_line_new);
        return;
    }
    args = _parseCommandLineVector(cmd_line);
}

string Command::getCmdStr()
{
    /**string str;
    for (auto arg : args)
    {
        str.append(arg);
        str.append(" ");
    }
    str.pop_back();
    return str;*/
    return this->cmd_line_str;
}

char **Command::getArgsArr()
{
    char **argsArr = new char *[4];
    argsArr[0] = (char *)("/bin/bash");
    argsArr[1] = (char *)("-c");
    string str = _rtrim(getCmdStr());
    if (str.back() == '&')
    {
        str.pop_back();
        str = _rtrim(str);
    }
    /**char *cmd[] = {
        (char *)("/bin/bash"),
        (char *)("-c"),
        (char *)str.c_str(),
        nullptr
    };*/
    argsArr[2] = new char[str.length() + 1];
    strcpy(argsArr[2], str.c_str());
    // argsArr[2] = const_cast<char *>(getCmdStr().c_str());
    argsArr[3] = NULL;
    return argsArr;
}

bool Command::isBg()
{
    return (args.back().back() == '&');
}

//**************BuiltInCommand************************
BuiltInCommand::BuiltInCommand(const char *cmd_line)
    : Command(cmd_line, BUILT_IN)
{
}

//**************BlankCommand************************
BlankCommand::BlankCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line) {}

void BlankCommand::execute() {}

//**************chprompt**********************
ChangePromptCommand::ChangePromptCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void ChangePromptCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    string new_dir = (args.size() > 1) ? args[1] : "smash";
    smash.changePrompt(new_dir);
}

//**************showpid**********************
ShowPidCommand::ShowPidCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void ShowPidCommand::execute()
{
    // SmallShell &smash = SmallShell::getInstance();
    pid_t pid = getpid();
    cout << "smash pid is " << pid << endl;
}

//**************cd command**********************
ChangeDirCommand::ChangeDirCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void ChangeDirCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    char *path = new char[COMMAND_ARGS_MAX_LENGTH];
    if (args.size() != 2)
    {
        fprintf(stderr, "smash error: cd: too many arguments\n");
        return;
    }
    if (args[1].compare("-") == 0)
    { // cd to last working cd
        if (smash.isEmpty_dir())
        {
            fprintf(stderr, "smash error: cd: OLDPWD not set\n");
            delete path;
            return;
        }
        getcwd(path, COMMAND_ARGS_MAX_LENGTH);
        if (chdir(smash.top_dir().c_str()) < 0)
        {
            perror("smash error: chdir failed");
            return;
        }
        else
        {
            smash.pop_dir();
            smash.push_dir(path);
        }
    }
    else
    {
        // push current wd to the stack
        if (getcwd(path, COMMAND_ARGS_MAX_LENGTH) < 0)
        {
            perror("smash error: getcwd failed");
            return;
        }
        bool popped;
        string last_path;
        if (!smash.isEmpty_dir())
        {
            last_path = smash.top_dir();
            smash.pop_dir();
            popped = true;
        }
        smash.push_dir(path);
        // cd to whatever path specified
        if (chdir(args[1].c_str()) == -1)
        {
            perror("smash error: chdir failed");
            smash.pop_dir();
            if (popped)
            {
                smash.push_dir(last_path.c_str());
            }
            return;
        }
    }
    delete path;
}

//**************pwd command**********************
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void GetCurrDirCommand::execute()
{
    char curr_dir[COMMAND_ARGS_MAX_LENGTH];
    getcwd(curr_dir, COMMAND_ARGS_MAX_LENGTH);
    std::cout << curr_dir << std::endl;
}

//**************JobEntry**********************
JobsList::JobEntry::JobEntry(Command *cmd, int process_id, bool is_stopped, int job_id, int duration)
    : job_id(job_id), process_id(process_id), is_stopped(is_stopped), duration(duration)
{
    cmd_name = cmd->getCmdStr();
    time(&create_time);
}

void JobsList::JobEntry::printJobWithTime()
{
    time_t now;
    time(&now);
    double elapsed = difftime(now, create_time);
    std::cout << cmd_name << " : " << process_id << " " << elapsed << " secs";
    if (is_stopped)
        std::cout << " (stopped)";
    std::cout << std::endl;
}

int JobsList::JobEntry::getJobId()
{
    return this->job_id;
}

void JobsList::JobEntry::setJobId(int id)
{
    this->job_id = id;
}

int JobsList::JobEntry::isTimed()
{
    return duration != -1;
}

int JobsList::JobEntry::getProcessID()
{
    return this->process_id;
}

void JobsList::JobEntry::printAndDie()
{
    std::cout << process_id << ": " << cmd_name << std::endl;
    kill(this->process_id, SIGKILL);
}

void JobsList::JobEntry::stopJob()
{
    is_stopped = true;
}

void JobsList::JobEntry::contJob()
{
    is_stopped = false;
}

void JobsList::JobEntry::printJob()
{
    std::cout << cmd_name << " : " << process_id << std::endl;
}

bool JobsList::JobEntry::isStopped()
{
    return is_stopped;
}

void JobsList::JobEntry::printAlarm()
{
    cout << "smash: " << cmd_name << " timed out!" << endl;
}

bool JobsList::JobEntry::isOver()
{
    time_t stat;
    time(&stat);
    int diff = std::difftime(stat, create_time);
    return (duration > 0) && (duration <= diff);
}

bool JobsList::TimedJob::isOver()
{
    time_t stat;
    time(&stat);
    // int diff = std::difftime(stat, create_time);
    return duration > 0 && time_of_death <= stat;
}

//**************JobList**********************
JobsList::JobsList()
    : max_id(0)
{
}

void JobsList::printJobsList()
{
    for (auto &job_it : jobs)
    {
        std::cout << "[" << job_it.first << "] ";
        job_it.second.printJobWithTime();
    }
}

void JobsList::addJob(Command *cmd, int process_id, bool isForeground, bool is_stopped)
{
    // removeFinishedJobs();
    if (!isForeground)
    {
        JobsList::JobEntry job(cmd, process_id, is_stopped, max_id + 1);
        addJob(job, isForeground);
    }
    else
    {
        JobsList::JobEntry job(cmd, process_id, is_stopped);
        addJob(job, isForeground);
    }
}

void JobsList::addJob(JobEntry &job, bool isForeground)
{
    // removeFinishedJobs();
    if (isForeground)
    {
        foregroundJob.push(job);
    }
    else
    {
        jobs.insert(pair<int, JobEntry>(++max_id, job));
    }
}

// this func is in use iff the job to be added is stopped by ^z
void JobsList::addJobFromZsignal(JobEntry &job)
{
    if (job.getJobId() == 0)
    { // this was never in jobsList
        job.setJobId(++max_id);
        jobs.insert(pair<int, JobEntry>(max_id, job));
    }
    else
    { // this was already in jobsList. no need to give new job id
        jobs.insert(pair<int, JobEntry>(job.getJobId(), job));
    }
}

// this is to add a job by timeout command
void JobsList::addTimedJob(Command *cmd, pid_t pid, int duration, bool isForeground)
{
    if (isForeground)
    {
        JobsList::TimedJob job(duration, cmd, pid, false);
        addJob(job, isForeground);
    }
    else
    {
        JobsList::TimedJob job(duration, cmd, pid, false, ++max_id);
        pq_timed_jobs.push(job);
        jobs.insert(pair<int, JobEntry>(job.getJobId(), job));
    }
}

void JobsList::killAllJobs()
{
    std::cout << "smash: sending SIGKILL signal to " << getCount() << " jobs:" << std::endl;
    for (auto job_pair : jobs)
    {
        job_pair.second.printAndDie();
    }
}

void JobsList::removeFinishedJobs()
{
    /**if (jobs.empty() == 1 && pq_timed_jobs.empty() == 1)
        return; ???? **/
    pid_t pid;
    for (auto job_pair_it = jobs.begin(), next_it = job_pair_it; job_pair_it != jobs.end(); job_pair_it = next_it)
    {
        ++next_it;
        pid = job_pair_it->second.getProcessID();
        int stat;
        pid_t res = waitpid(pid, &stat, WNOHANG);
        if (res < 0)
        {
            perror("wait failed1");
        }
        else
        {
            if (WIFEXITED(stat) && !job_pair_it->second.isTimed()) // child terminated normally
            {
                removeJobById(job_pair_it->first);
            }
            else if (WIFSIGNALED(stat)) // child terminated by a signal
            {
                int sig = WTERMSIG(stat);
                if (sig == SIGKILL) // job was killed
                {
                    removeJobById(job_pair_it->first);
                }
            }
        }
    }
}

void JobsList::removeJobById(int jobId)
{
    jobs.erase(jobId);
    if (jobId == max_id)
    {
        max_id = (jobs.empty()) ? 0 : jobs.rbegin()->first;
    }
}

JobsList::JobEntry &JobsList::getFgJob()
{
    return this->foregroundJob.top();
}

bool JobsList::isExist(int job_id)
{
    return jobs.find(job_id) != jobs.end();
}

JobsList::JobEntry *JobsList::getJobById(int jobId)
{
    if (jobs.find(jobId) == jobs.end())
    {
        return nullptr;
    }
    return &(jobs.find(jobId)->second);
}

void JobsList::AlarmCheck()
{
    if (pq_timed_jobs.empty() && foregroundJob.empty())
        return;
    if (!pq_timed_jobs.empty())
    {
        TimedJob first(pq_timed_jobs.top());
        if (first.isOver())
        {
            first.printAlarm();
            kill(first.getProcessID(), SIGKILL);
            pq_timed_jobs.pop();
            alarm(pq_timed_jobs.top().getDeathTime());
        }
    }
    JobEntry *fgJob = &(foregroundJob.top());
    if (!foregroundJob.empty() && fgJob->isTimed() && fgJob->isOver()) // if this is a timed job
    {
        fgJob->printAlarm();
        kill(fgJob->getProcessID(), SIGKILL);
        foregroundJob.pop();
    }
}

int JobsList::getPID(int jobId)
{
    if (jobs.find(jobId) == jobs.end()) // doesn't exist
    {
        return -1;
    }
    auto job = jobs.find(jobId)->second;
    return job.getProcessID();
}

int JobsList::getCount()
{
    return jobs.size();
}

void JobsList::killForegroundJob()
{
    foregroundJob.pop();
}

bool JobsList::isForeground()
{
    return !foregroundJob.empty();
}

pid_t JobsList::getForegroundPid()
{
    return foregroundJob.top().getProcessID();
}

void JobsList::stopForeground()
{
    JobsList::JobEntry &job = foregroundJob.top();
    foregroundJob.pop();
    job.stopJob();
    addJobFromZsignal(job);
}

pid_t JobsList::lastToFront()
{
    if (jobs.empty())
    {
        fprintf(stderr, "smash error: fg: jobs list is empty\n");
        return -1;
    }
    JobsList::JobEntry job = jobs.rbegin()->second;
    jobs.erase(std::prev(jobs.end()));
    job.printJob();
    job.contJob();
    addJob(job, true);
    return job.getProcessID();
}

pid_t JobsList::jobIdToFront(int JobId)
{
    if (jobs.empty() || jobs.find(JobId) == jobs.end())
    {
        fprintf(stderr, "smash error: fg: job-id %d does not exist\n", JobId);
        return -1;
    }
    JobsList::JobEntry job = jobs.find(JobId)->second;
    jobs.erase(jobs.find(JobId));
    job.printJob();
    job.contJob();
    addJob(job, true);
    return job.getProcessID();
}

pid_t JobsList::lastToBack()
{
    map<int, JobEntry>::reverse_iterator rit;
    for (rit = jobs.rbegin(); rit != jobs.rend(); ++rit)
    {
        if (rit->second.isStopped())
        {
            break;
        }
    }
    if (rit == jobs.rend())
    {
        fprintf(stderr, "smash error: bg: there is no stopped jobs to resume\n");
        return -1;
    }
    rit->second.printJob();
    rit->second.contJob();
    return rit->second.getProcessID();
}

pid_t JobsList::jobIdToBack(int JobId)
{
    map<int, JobEntry>::iterator it = jobs.find(JobId);
    if (jobs.empty() || it == jobs.end())
    {
        fprintf(stderr, "smash error: bg: job-id %d does not exist\n", JobId);
        return -1;
    }
    if (!it->second.isStopped())
    {
        fprintf(stderr, "smash error: bg: job-id %d is already running in the background\n", JobId);
        return -1;
    }
    it->second.printJob();
    it->second.contJob();
    return it->second.getProcessID();
}

bool JobsList::anyTimedJobs()
{
    return pq_timed_jobs.empty();
}

time_t JobsList::closestAlarm()
{
    return pq_timed_jobs.top().getDeathTime();
}

//**************JobsCommand**********************
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), job_ptr(jobs)
{
}

void JobsCommand::execute()
{
    // job_ptr->removeFinishedJobs();
    job_ptr->printJobsList();
}

//**************Kill Command**********************
bool is_number(const std::string &s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it))
        ++it;
    return !s.empty() && it == s.end();
}

int GetSignal(string flag)
{ // turns -<signal> to <signal> and returns it as an integer
    if (flag.find_first_not_of('-') != 1)
    {
        return -1;
    }
    string newFlag = flag.substr(flag.find_first_not_of('-'));
    if (is_number(newFlag))
    {
        return std::stoi(newFlag);
    }
    return -1;
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), job_ptr(jobs) {}

void KillCommand::execute()
{
    if (args.size() != 3)
    {
        fprintf(stderr, "smash error: kill: invalid arguments\n");
        return;
    }
    int signal = GetSignal(args[1]);
    // invalid command check:
    if (signal == -1 || signal > 64 || !(is_number(args[2]) || is_number(args[2].substr(1))))
    {
        fprintf(stderr, "smash error: kill: invalid arguments\n");
        return;
    }
    // checking for the job in the jobs list:
    int jid = std::stoi(args[2]);
    int pid = job_ptr->getPID(jid); // returns -1 if doesn't exist in the jobs list
    if (pid == -1)
    { // job doesn't exist in the Jobs List
        fprintf(stderr, "smash error: kill: job-id %d does not exist\n", jid);
        return;
    }
    if (kill(pid, signal) == -1)
    {
        perror("smash error: kill failed");
        return;
    }
    else
    {
        if (signal == SIGSTOP)
        {
            job_ptr->getJobById(jid)->stopJob();
        }
        else if (signal == SIGCONT)
        {
            job_ptr->getJobById(jid)->contJob();
        }
        cout << "signal number " << signal << " was sent to pid " << pid << endl;
    }
}

//**************ForegroundCommand**********************
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobs_ptr(jobs) {}

void ForegroundCommand::execute()
{
    bool jobSpecified = args.size() > 1;
    if (jobSpecified && (args.size() > 2 || !(is_number(args[1]) || is_number(args[1].substr(1)))))
    {
        std::cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    /*if (jobSpecified)
    {
      int job_id = stoi(args[1]);
      if (!jobs_ptr->isExist(job_id))
      {
        std::cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
      }
    }*/

    pid_t pid = jobSpecified ? jobs_ptr->jobIdToFront(stoi(args[1])) : jobs_ptr->lastToFront();
    SmallShell &smash = SmallShell::getInstance();
    if (pid == -1)
    {
        return;
    }
    kill(pid, SIGCONT);
    smash.runAtFront(pid);
}

//**************BackgroundCommand**********************
BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobs_ptr(jobs) {}

void BackgroundCommand::execute()
{
    bool jobSpecified = args.size() > 1;
    if (args.size() > 2 || (args.size() > 1 && !(is_number(args[1]) || is_number(args[1].substr(1)))))
    {
        std::cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    /*
    if (jobSpecified)
    {
      int job_id = stoi(args[1]);
      if (!jobs_ptr->isExist(job_id))
      {
        std::cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
      }
    }*/

    pid_t pid = jobSpecified ? jobs_ptr->jobIdToBack(stoi(args[1])) : jobs_ptr->lastToBack();
    // SmallShell &smash = SmallShell::getInstance();
    kill(pid, SIGCONT);
    // smash.runAtFront(pid);
}

//**************QuitCommand**********************
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line), jobs_ptr(jobs)
{
}

void QuitCommand::execute()
{
    if (args.size() > 1 && args[1] == "kill")
    {
        jobs_ptr->killAllJobs();
    }
    exit(0);
}

//**************ExternalCommand***********************
ExternalCommand::ExternalCommand(const char *cmd_line)
    : Command(cmd_line, 0)
{
}

void ExternalCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    smash.executeExternalCommand(this);
}

//****************************************************
//**************Special Commands**********************
//****************************************************

//**************IOCommand**********************
IOCommand::IOCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line)
{
}

void IOCommand::execute() {}

//**************RedirectFileCommand**********************
RedirectFileCommand::RedirectFileCommand(const char *cmd_line)
    : IOCommand(cmd_line)
{
    // string cmd = "", target = "";
    source = cmd_line_str.substr(0, cmd_line_str.find_first_of(">"));
    source = _trim(string(source));
    destination = cmd_line_str.substr(cmd_line_str.find_first_of(">") + 1);
    destination = _trim(string(destination));
    /**bool isTarget = false;
    int size = args.size();
    for (int i = 0; i < size; i++)
    {
        if (args[i].find(">") != std::string::npos)
        {
            isTarget = true;
            > is not the first char ==> there is a command before
            cmd.append(args[i].substr(0,args[i].find_first_of(">")));
            //> is the first char ==> there is a stream after
            target.append(args[i].substr(args[i].find_first_of(">") + 1));

            continue;
        }
        if (!isTarget) // still writing the command ______ >
        {
            cmd.append(args[i]);
            cmd.append(" ");
        }
        else // we now take the target  >  _______
        {
            target.append(args[i]);
            target.append(" ");
        }
    }
    destination = target;
    source = cmd;**/
}

void RedirectFileCommand::execute()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0)
    { // son
        SmallShell &smash = SmallShell::getInstance();
        Command *cmd = smash.CreateCommand(source.c_str());
        if (close(1) < 0)
        {
            perror("smash error: close failed");
            exit(1);
        }                                                                       // close the standard output
        int fp = open(destination.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0655); // create a file if needed | delete its content first | write only
        if (fp < 0)
        {
            perror("smash error: open failed");
            exit(1);
        }
        setpgrp();
        smash.stopRunning();
        cmd->execute();
    }
    else
    { // parent
        int stat;
        if (waitpid(pid, &stat, WUNTRACED) < 0)
        {
            perror("wait failed2");
            return;
        }
        /**if (close(fp) < 0)
        {
            perror("smash error: close failed");
        }*/
    }
}

//**************RedirectFileCommand**********************
AppendFileCommand::AppendFileCommand(const char *cmd_line)
    : IOCommand(cmd_line)
{
    source = cmd_line_str.substr(0, cmd_line_str.find_first_of(">>"));
    destination = cmd_line_str.substr(cmd_line_str.find_first_of(">>") + 2);
    destination = _trim(string(destination));
    source = _trim(string(source));
}

void AppendFileCommand::execute()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0)
    { // son
        SmallShell &smash = SmallShell::getInstance();
        Command *cmd = smash.CreateCommand(source.c_str());
        if (close(1) < 0)
        {
            perror("smash error: close failed");
            exit(1);
        }                                                              // close the standard output
        int fp = open(destination.c_str(), O_WRONLY | O_APPEND, 0655); // write only | append mode
        if (fp < 0)                                                    // there is no such file
        {
            fp = open(destination.c_str(), O_CREAT | O_WRONLY, 0655); // delete its content first | write only | append mode
            if (fp < 0)
            {
                perror("smash error: open failed");
                exit(1);
            }
        }
        setpgrp();
        smash.stopRunning();
        cmd->execute();
    }
    else
    { // parent
        int stat;
        if (waitpid(pid, &stat, WUNTRACED) < 0)
        {
            perror("wait failed3");
            return;
        }
    }
}

//**************PipeCommand**********************
PipeCommand::PipeCommand(const char *cmd_line)
    : IOCommand(cmd_line)
{
    source = cmd_line_str.substr(0, cmd_line_str.find_first_of("|"));
    destination = cmd_line_str.substr(cmd_line_str.find_first_of("|") + 1);
    destination = _trim(string(destination));
    source = _trim(string(source));
}

void PipeCommand::execute()
{
    /**
     * my_pipe[0] = FD to read from pipe
     * my_pipe[1] = FD to write to pipe
     * */
    pipe(my_pipe);
    SmallShell &smash = SmallShell::getInstance();

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0)
    { // son
        Command *cmd = smash.CreateCommand(source.c_str());
        close(my_pipe[0]); // son can't read from pipe
        // close(1);
        dup2(my_pipe[1], 1); // now entry 1 in FDT is my_pipe[1]
        setpgrp();
        close(my_pipe[1]);
        smash.stopRunning();
        cmd->execute();
    }
    else
    { // parent
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("smash error: fork failed");
            return;
        }
        if (pid == 0) // child
        {
            setpgrp();
            dup2(my_pipe[0], STDIN_FILENO); // now entry 0 in FDT is my_pipe[0]
            close(my_pipe[0]);
            close(my_pipe[1]);
            Command *after = smash.CreateCommand(destination.c_str());
            smash.stopRunning();
            after->execute();
        }
        else
        { // parent closing things up
            close(my_pipe[0]);
            close(my_pipe[1]); // parent can't write to pipe
            if (waitpid(-1, 0, WUNTRACED) < 0)
            {
                perror("smash error: wait failed4");
                return;
            }
            if (waitpid(-1, 0, WUNTRACED) < 0)
            {
                perror("smash error: wait failed5");
                return;
            }
        }
    }
}

//**************PipeErrorCommand**********************
PipeErrorCommand::PipeErrorCommand(const char *cmd_line)
    : IOCommand(cmd_line)
{
    source = cmd_line_str.substr(0, cmd_line_str.find_first_of("|&"));
    destination = cmd_line_str.substr(cmd_line_str.find_first_of("|&") + 2);
    destination = _trim(string(destination));
    source = _trim(string(source));
}

void PipeErrorCommand::execute()
{
    /**
     * my_pipe[0] = FD to read from pipe
     * my_pipe[1] = FD to write to pipe
     * */
    pipe(my_pipe);
    SmallShell &smash = SmallShell::getInstance();
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0)
    {
        Command *cmd = smash.CreateCommand(source.c_str());
        close(my_pipe[0]);               // son can't read from pipe
        dup2(my_pipe[1], STDERR_FILENO); // now entry 2 in FDT is my_pipe[1]
        setpgrp();
        close(my_pipe[1]);
        smash.stopRunning();
        cmd->execute();
    }
    else
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("smash error: fork failed");
            return;
        }
        if (pid == 0) // child
        {
            close(my_pipe[1]);
            dup2(my_pipe[0], STDIN_FILENO); // now entry 0 in FDT is my_pipe[0]
            close(my_pipe[0]);
            setpgrp();
            Command *after = smash.CreateCommand(destination.c_str());
            smash.stopRunning();
            after->execute();
        }
        else
        { // parent closing things up
            close(my_pipe[0]);
            close(my_pipe[1]); // parent can't write to pipe
            if (waitpid(-1, 0, WUNTRACED) < 0)
            {
                perror("smash error: wait failed4");
                return;
            }
            if (waitpid(-1, 0, WUNTRACED) < 0)
            {
                perror("smash error: wait failed5");
                return;
            }
        }
    }
}

//**************TimeOutCommand**********************
TimeOutCommand::TimeOutCommand(const char *cmd_line)
    : Command(cmd_line, 0)
{
    if (args.size() < 3)
    {
        fprintf(stderr, "smash error: timeout: invalid arguments\n");
        return;
    }
    if (!is_number(args[1]))
    {
        fprintf(stderr, "smash error: timeout: invalid arguments\n");
        return;
    }
    duration = std::stoi(args[1]);
    // int size = args.size();
    cmd = cmd_line_str.substr(cmd_line_str.find_first_of(args[1]) + args[1].length());
    cmd = _trim(cmd.c_str());
}

void TimeOutCommand::execute()
{
    SmallShell &smash = SmallShell::getInstance();
    Command *command = smash.CreateCommand(cmd.c_str());
    if (duration == -1)
    {
        return;
    }
    pid_t pid = fork();
    if (pid < 0) // fail
    {
        perror("smash error: fork failed");
        return;
    }
    else if (pid == 0) // child
    {
        SmallShell &smash = SmallShell::getInstance();
        smash.stopRunning();
        char **argsArr = command->getArgsArr();
        execv(argsArr[0], argsArr); // supports only external commands
        perror("execv failed");
    }
    else // parent
    {
        // setting up the needed alarm:
        if (smash.isEmpty_pq()) // no alarms set
        {
            if (alarm(this->duration) < 0)
            {
                perror("smash error: alarm failed");
                return;
            }
        }
        else
        {
            // there's an alarm already, now we check which is sooner:
            time_t timer;
            time(&timer);
            if (timer + duration < smash.closestAlarmFromNow()) // if the new alarm is sooner
            {
                if (alarm(this->duration) < 0)
                {
                    perror("smash error: alarm failed");
                    return;
                }
            }
        }
        if (_isBackgroundComamnd(cmd.c_str()))
        {
            smash.addTimedJob(this, pid, duration, false);
        }
        else
        {
            smash.addTimedJob(this, pid, duration, true);
            smash.runAtFront(pid, command);
        }
    }
}

//**************TailCommand**********************
TailCommand::TailCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line), n(10) {}

void TailCommand::execute()
{
    if (args.size() > 3)
    {
        fprintf(stderr, "smash error: tail: invalid arguments\n");
        return;
    }
    else if (args.size() <= 1)
    {
        fprintf(stderr, "smash error: tail: invalid arguments\n");
        return;
    }
    else if (args.size() == 3) // if there are 3 parameters- the second is N
    {
        if (args[1].at(0) != '-' || !is_number(args[1].substr(1)))
        {
            fprintf(stderr, "smash error: tail: invalid arguments\n");
            return;
        }
        n = std::stoi(args[1].substr(1));
        if (n < 0)
        {
            fprintf(stderr, "smash error: tail: invalid arguments\n");
            return;
        }
        else if (n == 0)
        {
            return;
        }
    }
    // the last parameter is the path
    const char *path = args.back().c_str();

    char buf;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("smash error: open failed");
        return;
    }
    lseek(fd, -1, SEEK_END); // seek pointer at the last char
    int i = 0;               // number of "\n" read from EOF
    bool last_char = true;   // we are not counting new_line if it's the last char
    int read_success = 0;

    /* Reading one char at a time from the end of the file,
    until n 'new-line's were read (going BACKWARDS).
    By doing so- the seek pointer is set. */
    while ((read_success = read(fd, &buf, 1)) == 1)
    {
        if (buf == '\n' && !last_char)
        {
            i++;
            if (i == n)
            {
                break;
            }
        }
        if (lseek(fd, -2, SEEK_CUR) < 0) // handeling reading the first char of the file
        {
            lseek(fd, 0, SEEK_SET);
            if (read(fd, &buf, 1) < 0)
            {
                perror("smash error: read failed");
                close(fd);
                return;
            }
            lseek(fd, 0, SEEK_SET);
            break;
        }
        last_char = false;
    }
    if (read_success < 0)
    {
        perror("smash error: read failed");
        close(fd);
        return;
    }

    int write_success = 0;
    while ((read_success = read(fd, &buf, 1)) == 1) // writing to stdout
    {
        if ((write_success = write(1, &buf, 1)) < 0)
        {
            perror("smash error: write failed");
            close(fd);
            return;
        }
        else if (write_success == 0)
        {
            break;
        }
    }
    if (read_success < 0)
    {
        perror("smash error: read failed");
    }
    close(fd);
}

//**************TouchCommand**********************
TouchCommand::TouchCommand(const char *cmd_line)
    : BuiltInCommand(cmd_line) {}

void TouchCommand::execute()
{
    if (args.size() != 3)
    {
        fprintf(stderr, "smash error: touch: invalid arguments\n");
        return;
    }
    struct tm tm;
    strptime(args[2].c_str(), "%S:%M:%H:%d:%m:%Y", &tm);
    time_t t = mktime(&tm);
    if (t == -1)
    {
        perror("smash error: mktime failed");
        return;
    }
    struct utimbuf new_time;
    new_time.actime = t;
    new_time.modtime = t;
    if (utime(args[1].c_str(), &new_time) == -1)
    {
        perror("smash error: utime failed");
        return;
    }
}

//**************SmallShell**********************
SmallShell::SmallShell()
    : prompt("smash")
{
    // TODO: add your implementation
}

SmallShell::~SmallShell()
{
    // TODO: add your implementation
}

bool isBuiltIn(string cmd, const string built_in)
{
    string bg_built_in = built_in + "&";
    if (cmd.compare(built_in) == 0 || cmd.compare(bg_built_in) == 0)
    {
        return true;
    }
    return false;
}

bool isRedirect(string cmd)
{
    if (cmd.find(">") == std::string::npos)
    { // no '>' found
        return false;
    }
    return true;
}

bool isPipe(string cmd)
{
    if (cmd.find("|") == std::string::npos)
    { // no '|' found
        return false;
    }
    return true;
}

bool isPipeError(string cmd)
{
    if (cmd.find("|&") == std::string::npos)
    { // no '|&' found
        return false;
    }
    return true;
}

bool isRedirectAppend(string cmd)
{
    if (cmd.find(">>") == std::string::npos)
    { // no '>>' found
        return false;
    }
    return true;
}

Command *SmallShell::CreateCommand(const char *cmd_line)
{
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (isPipeError(cmd_s))
    {
        return new PipeErrorCommand(cmd_line);
    }
    else if (isPipe(cmd_s))
    {
        return new PipeCommand(cmd_line);
    }
    else if (isRedirectAppend(cmd_s))
    {
        return new AppendFileCommand(cmd_line);
    }
    else if (isRedirect(cmd_s))
    {
        return new RedirectFileCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, ""))
    {
        return new BlankCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, "timeout"))
    {
        return new TimeOutCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, "chprompt"))
    {
        return new ChangePromptCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, "pwd"))
    {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, "showpid"))
    {
        return new ShowPidCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, "cd"))
    {
        return new ChangeDirCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, "jobs"))
    {
        return new JobsCommand(cmd_line, &jobs);
    }
    else if (isBuiltIn(firstWord, "kill"))
    {
        return new KillCommand(cmd_line, &jobs);
    }
    else if (isBuiltIn(firstWord, "quit"))
    {
        return new QuitCommand(cmd_line, &jobs);
    }
    else if (isBuiltIn(firstWord, "fg"))
    {
        return new ForegroundCommand(cmd_line, &jobs);
    }
    else if (isBuiltIn(firstWord, "bg"))
    {
        return new BackgroundCommand(cmd_line, &jobs);
    }
    else if (isRedirect(cmd_s))
    {
        return new RedirectFileCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, "tail"))
    {
        return new TailCommand(cmd_line);
    }
    else if (isBuiltIn(firstWord, "touch"))
    {
        return new TouchCommand(cmd_line);
    }
    else
    {
        return new ExternalCommand(cmd_line);
    }
}

void SmallShell::executeCommand(const char *cmd_line)
{
    Command *cmd = CreateCommand(cmd_line);
    jobs.removeFinishedJobs();
    cmd->execute();
    /*
    if (dynamic_cast<ExternalCommand *>(cmd) == nullptr) // Built-in Command
    {
      cmd->execute();
    }
    else // External Command
    {
      char **argsArr = cmd->getArgsArr();
      pid_t pid = fork();
      if (pid < 0) // fail
      {
        perror("fork failed");
      }
      else if (pid == 0) // child
      {
        setpgrp();
        execv(argsArr[0], argsArr);
        perror("execv failed");
      }
      else // parent
      {
        if (_isBackgroundComamnd(cmd_line))
        {
          jobs.addJob(cmd, pid);
        }
        else
        {
          jobs.addJob(cmd, pid, true);
          runAtFront(pid, cmd);
        }
      }
    }
    */
}

void SmallShell::executeExternalCommand(Command *cmd)
{
    char **argsArr = cmd->getArgsArr();
    /**string str = cmd->getCmdStr();
    if (str.back() == '&')
    {
        str.pop_back();
        str = _rtrim(str);
    }
    char *line[] = {
        (char *)("/bin/bash"),
        (char *)("-c"),
        (char *)str.c_str(),
        nullptr
    };*/
    pid_t pid = fork();
    if (pid < 0) // fail
    {
        perror("fork failed");
        return;
    }
    else if (pid == 0) // child
    {
        stopRunning();
        setpgrp();
        execv(argsArr[0], argsArr);
        perror("execv failed");
    }
    else // parent
    {
        if (cmd->isBg())
        {
            jobs.addJob(cmd, pid);
        }
        else
        {
            jobs.addJob(cmd, pid, true);
            runAtFront(pid, cmd);
        }
    }
}

void SmallShell::printPtompt()
{
    std::cout << prompt << "> ";
}

void SmallShell::changePrompt(string new_prompt)
{
    prompt = new_prompt;
}

void SmallShell::push_dir(string dir)
{
    this->directories.push(dir);
}

void SmallShell::pop_dir()
{
    this->directories.pop();
}

string SmallShell::top_dir()
{
    return this->directories.top();
}

int SmallShell::getDirSize()
{
    return this->directories.size();
}

bool SmallShell::isEmpty_dir()
{
    return this->directories.empty();
}

void SmallShell::killForegroundJob()
{
    jobs.killForegroundJob();
}

bool SmallShell::isForeground()
{
    return jobs.isForeground();
}

pid_t SmallShell::getForegroundPid()
{
    return jobs.getForegroundPid();
}

void SmallShell::stopForeground()
{
    jobs.stopForeground();
}

void SmallShell::runAtFront(pid_t pid, Command *cmd)
{
    int stat;
    if (waitpid(pid, &stat, WUNTRACED) < 0)
    {
        perror("wait failed7");
        return;
    }
    else
    {
        if (WIFEXITED(stat)) // child terminated normally
        {
            jobs.killForegroundJob();
        }
        else if (WIFSIGNALED(stat)) // child terminated by a signal
        {
            int sig = WTERMSIG(stat);
            if (sig == SIGTSTP)
            {
                jobs.addJob(cmd, pid, false, true);
            }
        }
    }
}

void SmallShell::runAtFront(pid_t pid)
{
    int stat;
    if (waitpid(pid, &stat, WUNTRACED) < 0)
    {
        perror("wait failed8");
        return;
    }
    else
    {
        if (WIFEXITED(stat)) // child terminated normally
        {
            jobs.killForegroundJob();
        }
        else if (WIFSIGNALED(stat)) // child terminated by a signal
        {
            int sig = WTERMSIG(stat);
            if (sig == SIGTSTP)
            {
                jobs.addJobFromZsignal(jobs.getFgJob());
            }
        }
    }
}

void SmallShell::AlarmHandle()
{
    jobs.AlarmCheck();
}

void SmallShell::addTimedJob(Command *cmd, pid_t pid, int duration, bool isForeground)
{
    jobs.addTimedJob(cmd, pid, duration, isForeground);
}
bool SmallShell::isRunning()
{
    return is_running;
}
void SmallShell::stopRunning()
{
    is_running = false;
}

bool SmallShell::isEmpty_pq()
{
    return jobs.anyTimedJobs();
}

time_t SmallShell::closestAlarmFromNow()
{
    return jobs.closestAlarm();
}