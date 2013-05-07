//
// You received this file as part of RRLib
// Robotics Research Library
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//----------------------------------------------------------------------
/*!\file    rrlib/crash_handler/crash_handler.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-06-20
 *
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sstream>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/crash_handler/crash_handler.h"
#include "rrlib/thread/tThread.h"

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace rrlib
{
namespace crash_handler
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------
#ifndef PR_SET_PTRACER
#define PR_SET_PTRACER 0x59616d61
#endif
//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// SomeFunction
//----------------------------------------------------------------------
static void HandleCrash(int signal)
{
  // We do not want to execute this function more than once in application life-time
  static bool called = false;
  if (called)
  {
    return;
  }
  called = true;

  rrlib::thread::tThread& t = rrlib::thread::tThread::CurrentThread();
  const char* signal_string = (signal == SIGSEGV) ? "SIGSEGV" : "SIGABRT";
  std::cout << std::endl << "\033[;1;31m************************************************************************" << std::endl;
  if (&t) // Should always be the case. However, you never know if application has crashed.
  {
    std::cout << "Thread '" << t.GetName() << "' (" << (void*)t.GetNativeHandle() << ") just did something bad (" << signal_string << ") and crashed your program." << std::endl;
  }
  else
  {
    std::cout << "Something bad happened and crashed your program (" << signal_string << ")." << std::endl;
  }
  std::cout << "Would you like to start gdb to inquire what happened (y/n)?" << std::endl << std::endl;
  std::cout << "Important gdb commands:" << std::endl;
  std::cout << "  'info threads'      shows all running threads" << std::endl;
  std::cout << "  'thread <threadno>' selects thread" << std::endl;
  std::cout << "  'bt [full]'         prints stack trace" << std::endl << std::endl;
  std::cout << "If you believe the error is not in your own code, we are always happy to receive bug reports. Please submit a full backtrace of the thread that crashed. Thanks!" << std::endl;
  std::cout << "************************************************************************\033[;0m" << std::endl;
  char answer;
  std::cin >> answer;
  if (answer == 'y' || answer == 'Y')
  {
    pid_t pid = getpid();
    prctl(PR_SET_PTRACER, pid, 0, 0, 0); // Allows a gdb child process to attach
    std::ostringstream pidstr;
    pidstr << pid;
    char binary_name[1024];
    int chars = readlink("/proc/self/exe", binary_name, 1013);
    binary_name[chars] = 0;
    pid_t child = fork();
    if (!child)
    {
      execlp("gdb", "gdb", binary_name, pidstr.str().c_str(), NULL);
      abort(); // If we could not start gdb
    }
    else
    {
      waitpid(child, NULL, 0);
    }
  }
}

bool InstallCrashHandler()
{
  //rrlib::thread::tThread::CurrentThread().SetName("Main"); // Names process "Main", too

  struct sigaction signal_action;
  signal_action.sa_handler = HandleCrash;
  sigemptyset(&signal_action.sa_mask);
  signal_action.sa_flags = 0;

  if (sigaction(SIGABRT, &signal_action, NULL) != 0)
  {
    perror("Could not install signal handler for SIGABRT");
    return false;
  }
  if (sigaction(SIGSEGV, &signal_action, NULL) != 0)
  {
    perror("Could not install signal handler for SIGSEGV");
    return false;
  }

  return true;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
