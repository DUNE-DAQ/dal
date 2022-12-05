#ifndef _daq_core_exit_status_H_
#define _daq_core_exit_status_H_

 /// used for get and set config versions exit status

enum __ExitStatus__
{
  __SuccessExitStatus__ = 0,
  __CommandLineErrorExitStatus__ = 1,
  __RepositoryNotFoundExitStatus__ = 2,
  __InfoNotFoundExitStatus__ = 3,
  __FailureExitStatus__ = 4
};

#endif
