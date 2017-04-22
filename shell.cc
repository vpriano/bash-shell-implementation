// C libraries 
// handles all the immediate Bash commands
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

// C++ libraries
// used more as helper data structures
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <iterator>

using namespace std;

// global constant used to identify which command is being ran
char* jobname;

// Parse the command line into a working vector to
// separate key terms and filenames
void parse (char* line, vector<char*>& v)
{
	// parses for c-strings and pushes them into a vector
	// ignores whitespaces, tabs, and newlines
	while (*line != 0)
	{
		while (*line == ' ' || *line == '\t' || *line == '\n')
			*line++ = '\0';
		v.push_back(line);
		while (*line != ' ' && *line != '\t' && *line != '\0' && *line != '\n')
			line++;
	}
}

// helper function to print out the vector for debuging purposes
void print (const vector<char*> v)
{
	for (unsigned i = 0; i < v.size(); ++i)
		cout << v[i] << " ";
}

// used to debug and print the 'fg' and 'bg' jobs
// this function uses a map container that has all the jobs currently 
// running, job titles, and the corresponding job number
void print_jobs (map<int, char*>& m)
{
	map<int, char*>::iterator it;
	for (it = m.begin (); it != m.end (); ++it)
		cout << it->first << " " << it->second << endl, cout.flush ();
}

// interprets the data which is passes as a string and
// runs the specific functions based on the jobname written by the user
// supports single pipeling, redirection, signal handling
// foreground and background jobs and simple UNIX commands
void execute (vector<char*> v, map<int, char*>& m)
{
	if (v.size () == 0 || strcmp (v[0], "") == 0) return;
	if (v.size () == 1 && strcmp (v[0], "jobs") == 0)
	{
		map<int, char*>::iterator it;
	    for (it = m.begin (); it != m.end (); ++it)
		{
		    cout << it->first << " ";
		    cout << it->second << endl, cout.flush ();
		}
		return;
	}
	if (v.size () == 2 && strcmp (v[0], "kill") == 0)
	{
		bool there = false;
		map<int, char*>::iterator it;
		int i = atoi (v[1]);
		for (it = m.begin (); it != m.end (); ++it)
		   if (it->first == i) there = true;
		if (there) kill (i, SIGINT);
		return;
	}
	pid_t pid = vfork ();
	int file;
	
	if (pid < 0) { cerr << "Forked failed" << endl, exit(1); }
	else if (pid > 0)
	{
		//if ((strcmp (v.back (), "&") != 0)) return ? wait(&status):waitpid (pid, &status, WNOHANG));
		if (strcmp (v.back (), "&") != 0 && wait(0) == -1) exit(1);
		//waitpid (pid, &status, WNOHANG);
	}
	else
	{
		char* argv [v.size () + 1];
		int argc = 0;
		for (unsigned i = 0; i < v.size(); ++i)
		{
			if (strcmp (v[i], "&") == 0) 
			{
				m.insert (pair <int, char*> (getpid (), jobname));
				break;
			}
			else if (strcmp (v[i], "<") == 0)
			{
				file = open (v[++i], O_RDONLY);
				dup2 (file, 0);
				close (file);
			}
			else if (strcmp (v[i], ">") == 0)
			{
				file = open (v[++i], O_RDWR | O_CREAT);
				dup2 (file, 1);
				close (file);
				chmod (v[i], 0644);
			}
			else if (strcmp (v[i], "|") == 0)
			{
				vector<char*> temp;
                for(unsigned j = ++i; j < v.size(); ++j)
                    temp.push_back(v[j]);

                int mypipe[2];
                if(pipe (mypipe)) cerr << "Pipe failed" << endl, exit(1);
                if(fork ())
                {
					close (mypipe[1]);
                    dup2 (mypipe[0], 0);
                    close (mypipe[0]);
                    execute (temp, m);
                    exit(0);
                }
                else
                {
					close (mypipe[0]);
                    dup2 (mypipe[1], 1);
                    close (mypipe[1]);
                    break;
                }
			}
			else
			{
				argv[argc] = new char [strlen (v[i]) + 1];
				strcpy (argv[argc], v[i]);
				argv[++argc] = 0;
			}
		}
		execvp (v[0], argv);
		return;
	}
}


// overwritten the signal handler for 'ctrl C' which
// is the interrupt signal
void handle_it_INT (int signum)
{
	kill (getppid (), SIGINT);
	raise (SIGINT);
	return;
}

// overwritten the signal handler for 'ctrl Z' which
// is the quit signal
void handle_it_QUIT (int signum)
{
	kill (getppid (), SIGQUIT);
	raise (SIGQUIT);
	return;
}

// the main code runs in an infinite loop to emulate the Bash
int main ()
{
	char line[1024];
	vector<char*> argv;
	map<int, char*> job;
	
	while (1)
	{
		argv.clear ();
		cout << "$ ";
		gets (line);
		parse (line, argv);
		jobname = argv[0];
		// 'exit' and 'quit' exits the program
		(strcmp (argv[0], "exit") == 0 || strcmp (argv[0], "quit") == 0) ? exit(0):execute (argv, job);
	}	
	// should never call
	return 0;
}
