/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2016, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "server.h"

// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
  // called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, RTSPClient** rtspclient);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient)
{
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession)
{
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName)
{
  env << "Usage: " << progName << " <start/stop>\n";
  env << "\tstart : daemonize/stop : kill daemon\n";
}

typedef struct
{
	UsageEnvironment* env;
	char prog_name[16];
	char url[50];
} sThread_Arg, *PTHREAD_ARG;

char eventLoopWatchVariable = 0;
pthread_t* player_thread=NULL;
sThread_Arg thread_arg={NULL, {0}, {0}};

extern int g_serverdaemonLoop_running;

RTSPClient* rtspClient[2]={NULL, NULL};

char* alter_url(char* org_url)
{
    char* ret=NULL, *token=NULL;
    const char* alter_port=":8554";
    int alter_port_len=strlen(alter_port), pos=0;
    token=strstr(org_url, "/media");
    if (token)
    {
        pos=token-org_url;
        ret=(char*)calloc(1, strlen(org_url)+alter_port_len+1);
        memcpy(ret, org_url, pos);
        memcpy(&ret[pos], alter_port, alter_port_len);
        memcpy(&ret[pos+alter_port_len], token, strlen(token));
    }
    return ret;
}

void* client_thread(void* arg)
{
	PTHREAD_ARG pArg=(PTHREAD_ARG)arg;
	
	if (arg)
	{
		char* url2=alter_url(pArg->url);
		if (url2)
		{
			printf("url1=%s, url2=%s\r\n", pArg->url, url2);
			openURL(*(pArg->env), pArg->prog_name, pArg->url, &rtspClient[0]);
			openURL(*(pArg->env), pArg->prog_name, url2, &rtspClient[1]);
			free(url2);
			(pArg->env)->taskScheduler().doEventLoop(&eventLoopWatchVariable);
		}
	}

	rtspClient[0]=NULL;	rtspClient[1]=NULL;

	printf("thread terminated normally\r\n");
	return NULL;
}

void init_player(UsageEnvironment* env, char* prog_name)
{
	eventLoopWatchVariable=0;
	// initializing player thread arguments
	sprintf(thread_arg.prog_name, "%s", prog_name);
	thread_arg.env = env;
}

void wait_player(void)
{
	pthread_join(*player_thread, NULL);
	free(player_thread);
	player_thread=NULL;
}

void stop_player(void)
{
	int c=0;
	for (c=0; c<2; c++)
	{
		if (rtspClient[c])
		{
			printf("shutting down stream\r\n");
			shutdownStream(rtspClient[c], 0);
			rtspClient[c]=NULL;
		}
	}
	if (player_thread)
	{
		printf("waiting for player thread closing\r\n");
		wait_player();
	}
}

void start_player(char* url)
{
	// stop previous client first
	stop_player();
	player_thread = (pthread_t*)malloc(sizeof(pthread_t));
	if (player_thread)
	{
		memset(thread_arg.url, 0, sizeof(thread_arg.url));
		sprintf(thread_arg.url, "%s", url);
		eventLoopWatchVariable=0;
		pthread_create(player_thread, NULL, client_thread, (void*)&thread_arg);
	}
}

#define PID_FILE	"/etc/rtspclientd"

static void open_daemon(pid_t pidval)
{
	FILE* pidfile;
	printf("writing daemon info\r\n");
	pidfile=fopen(PID_FILE, "wb");
	if (pidfile==NULL)
	{
		printf("error opening pidfile\r\n");
		exit(EXIT_FAILURE);
	}
	fwrite(&pidval, sizeof(pid_t), 1, pidfile);
	fclose(pidfile);
}

static void close_daemon(void)
{
	FILE* pidfile;
	pid_t daemon_pid;
	pidfile=fopen(PID_FILE, "rb");
	fread(&daemon_pid, sizeof(pid_t), 1, pidfile);
	if (kill(daemon_pid, SIGTERM)==0)
	{
		printf("daemon is unloaded\r\n");
	} else
	{
		printf("cannot unload daemon\r\n");
	}
	fclose(pidfile);
	if (unlink(PID_FILE)==0)
	{
		printf("daemon removed\r\n");
	} else
	{
		printf("daemon remove fail\r\n");
	}
}

void terminate(int sig)
{
	if (sig==SIGTERM)
	{
		stop_player();
		printf("terminate program\r\n");
		g_serverdaemonLoop_running=0;
		exit(0);
	} else
	if (sig==SIGSEGV)
	{
		close_daemon();
	}
}


int main(int argc, char** argv)
{
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler=NULL;
	UsageEnvironment* env=NULL;

	scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

	// We need at least one "rtsp://" URL argument:

	init_player(env, argv[0]);
	
	if (argc < 2)
	{
		usage(*env, argv[0]);
		return 1;
	}

	if (strcasecmp(argv[1], "start")==0)
	{
		// check if previous daemon is running
		pid_t fork_result;

		if (access(PID_FILE, F_OK)==0)
		{
			printf("daemon is already running\r\n");
			exit(EXIT_SUCCESS);
		}
		
		fork_result=fork();
		if (fork_result==-1)
		{
			printf("Cannot daemonize %s\n", argv[0]);
			exit(EXIT_FAILURE);
		} else
		if (fork_result!=0) // parent process
		{
			printf("daemon loaded successfully\r\n");
//			exit(EXIT_SUCCESS);
		} else
		{
			fork_result=setsid();
			open_daemon(fork_result);
			printf("setting signal handler\r\n");
			signal(SIGTERM, terminate);
			signal(SIGSEGV, terminate);

			server_start();
		}
	} else
	if (strcasecmp(argv[1], "stop")==0)
	{
		if (access(PID_FILE, F_OK)==0)
		{
			close_daemon();
		} else
		{
			printf("no daemon is running..\r\n");
		}
		exit(0);
	} else
	{
		usage(*env, argv[0]);
		return 1;
	}
#if 0
	for (int i=1; i<argc; i++)
	{
		start_player(argv[i]);
		
		printf("client state=%d\r\n", eventLoopWatchVariable);
	}
#endif
  // All subsequent activity takes place within the event loop:
    // This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

	env->reclaim(); env = NULL;
	delete scheduler; scheduler = NULL;
	return 0;

  // If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
  // and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
  // then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
  /*
    env->reclaim(); env = NULL;
    delete scheduler; scheduler = NULL;
  */
}

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState
{
	public:
	  StreamClientState();
	  virtual ~StreamClientState();
	
	public:
	  MediaSubsessionIterator* iter;
	  MediaSession* session;
	  MediaSubsession* subsession;
	  TaskToken streamTimerTask;
	  double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient
{
	public:
	  static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
					  int verbosityLevel = 0,
					  char const* applicationName = NULL,
					  portNumBits tunnelOverHTTPPortNum = 0);
	
	protected:
	  ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	    // called only by createNew();
	  virtual ~ourRTSPClient();
	
	public:
	  StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class AlsaSink: public MediaSink
{
	public:
		static AlsaSink* createNew(UsageEnvironment& env,
				      MediaSubsession& subsession, // identifies the kind of data that's being received
				      char const* streamId = NULL); // identifies the stream itself (optional)
	
	private:
		AlsaSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
	    // called only by "createNew()"
		virtual ~AlsaSink();
	
		static void afterGettingFrame(void* clientData, unsigned frameSize,
	                                unsigned numTruncatedBytes,
					struct timeval presentationTime,
	                                unsigned durationInMicroseconds);
		void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				 struct timeval presentationTime, unsigned durationInMicroseconds);
	
	private:
	  // redefined virtual functions:
		virtual Boolean continuePlaying();
		int	init_alsa(void);
	
	private:
		snd_pcm_t* PCM;
		snd_pcm_hw_params_t* params;

		u_int8_t* snd_buf;
		int		cnt;
		u_int8_t* fReceiveBuffer;
		MediaSubsession& fSubsession;
	  char* fStreamId;
};

#define RTSP_CLIENT_VERBOSITY_LEVEL 0 // by default, print verbose output from each "RTSPClient"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, RTSPClient** rtspclient)
{
  // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
  // to receive (even if more than stream uses the same "rtsp://" URL).
//  RTSPClient* rtspClient = ourRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
	RTSPClient* tmpClient=NULL;
	tmpClient = ourRTSPClient::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
	if (tmpClient == NULL)
	{
		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
		*rtspclient=NULL;
	} else
	{
		++rtspClientCount;

		// Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
		// Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
		// Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
		tmpClient->sendDescribeCommand(continueAfterDESCRIBE); 
	}
	*rtspclient=tmpClient;
}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString)
{
  do
  {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }
    char* const sdpDescription = resultString;
	if (RTSP_CLIENT_VERBOSITY_LEVEL)
	{
	    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";
	}

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL)
    {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } else
    if (!scs.session->hasSubsessions())
    {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient* rtspClient)
{
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL)
  {
    if (!scs.subsession->initiate())
    {
		if (RTSP_CLIENT_VERBOSITY_LEVEL)
			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
		setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    } else
    {
      if (RTSP_CLIENT_VERBOSITY_LEVEL)
      {
		  env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
	      if (scs.subsession->rtcpIsMuxed())
	      {
					env << "client port " << scs.subsession->clientPortNum();
	      } else
	      {
					env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
	      }
	      env << ")\n";
      }

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL)
  {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else
  {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString)
{
  do
  {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

	if (RTSP_CLIENT_VERBOSITY_LEVEL)
	{
	    env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
	    if (scs.subsession->rtcpIsMuxed())
	    {
	      env << "client port " << scs.subsession->clientPortNum();
	    } else
	    {
	      env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
	    }
	   	env << ")\n";
	}

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

    scs.subsession->sink = AlsaSink::createNew(env, *scs.subsession, rtspClient->url());
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL)
    {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
	  		  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

	if (RTSP_CLIENT_VERBOSITY_LEVEL)
	{
	    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
	}
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL)
    {
      scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString)
{
  Boolean success = False;

  do
  {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0)
    {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0)
    {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

	if (RTSP_CLIENT_VERBOSITY_LEVEL)
	{
	    env << *rtspClient << "Started playing session";
	    if (scs.duration > 0)
	    {
	      env << " (for up to " << scs.duration << " seconds)";
	    }
	    env << "...\n";
	}

    success = True;
  } while (0);
  delete[] resultString;

  if (!success)
  {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData)
{
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL)
  {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData)
{
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData)
{
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode)
{
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL)
  { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL)
    {
      if (subsession->sink != NULL)
      {
      	Medium::close(subsession->sink);
      	subsession->sink = NULL;
      	if (subsession->rtcpInstance() != NULL)
      	{
      		subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
      	}
      	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive)
	{
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

#if 0
  	if (--rtspClientCount == 0)
  	{
	    // The final stream has ended, so exit the application now.
	    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
	    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
	    	exit(exitCode);
 	}
#else
	if (--rtspClientCount==0)
		eventLoopWatchVariable=1;
	printf("rtspClientCount=%d\r\n", rtspClientCount);
#endif
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
{
  return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1)
{
}

ourRTSPClient::~ourRTSPClient()
{
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0)
{
}

StreamClientState::~StreamClientState()
{
  delete iter;
  if (session != NULL)
  {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}


// Implementation of "AlsaSink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define ALSA_SINK_RECEIVE_BUFFER_SIZE 100000

AlsaSink* AlsaSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
{
  return new AlsaSink(env, subsession, streamId);
}

int AlsaSink::init_alsa(void)
{
	int err=0;
	unsigned int rate = 8000;

//	if ((err=snd_pcm_open(&PCM, "default", /*plughw:0,0",*/ SND_PCM_STREAM_PLAYBACK, 0))<0)
	if ((err=snd_pcm_open(&PCM, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0 /*SND_PCM_NONBLOCK*/))<0)
	{
		fprintf(stderr, "error snd_pcm_open\r\n");
		PCM=NULL;
		return err;
	} else
	{
		fprintf(stderr, "snd_pcm_open success\r\n");
	}
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(PCM, params);
	snd_pcm_hw_params_set_access(PCM, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(PCM, params, SND_PCM_FORMAT_S16_BE);
	snd_pcm_hw_params_set_channels(PCM, params, 1);
	snd_pcm_hw_params_set_rate_near(PCM, params, &rate, NULL);
	if ((err=snd_pcm_hw_params(PCM, params))<0)
	{
		fprintf(stderr, "error snd_pcm_hw_params\r\n");
		snd_pcm_hw_params_free(params);
		snd_pcm_close(PCM);
		PCM=NULL;
		return err;
	} else
	{
		fprintf(stderr, "snd_pcm_hw_params success\r\n");
	}
	return err;
}

#ifdef SAVE_TO_FILE
FILE* fp=NULL;
#endif// SAVE_TO_FILE

AlsaSink::AlsaSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env),
    fSubsession(subsession)
{
	PCM=NULL;
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[25600];
	snd_buf = new u_int8_t[ALSA_SINK_RECEIVE_BUFFER_SIZE];
	cnt=0;
	init_alsa();
#ifdef SAVE_TO_FILE
	fp=fopen("./data.bin", "wb");
#endif // SAVE_TO_FILE
}

AlsaSink::~AlsaSink()
{
	if (PCM)
	{
		printf("closing Alsa Device\r\n");
		snd_pcm_drain(PCM);
		snd_pcm_close(PCM); PCM=NULL;
		snd_config_update_free_global();
	}
#ifdef SAVE_TO_FILE
	fclose(fp);
#endif // SAVE_TO_FILE
  delete[] fReceiveBuffer;
  delete[] fStreamId;
  delete[] snd_buf;
}

void AlsaSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds)
{
  AlsaSink* sink = (AlsaSink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

void AlsaSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds)
{
  // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef SAVE_TO_FILE
	fwrite(fReceiveBuffer, frameSize, 1, fp);
#else
	ssize_t r=0;

	cnt+=frameSize;

//	fprintf(stderr, "num = %d\r\n", numTruncatedBytes);
#if 1 // 1 for endian change
	for (int i=0; i<(int)frameSize; i+=2)
	{
		u_int8_t tmp;
		tmp=fReceiveBuffer[i];
		fReceiveBuffer[i]=fReceiveBuffer[i+1];
		fReceiveBuffer[i+1]=tmp;
	}
#endif
	if (PCM)
	{
		do 
		{
			r=snd_pcm_writei(PCM, fReceiveBuffer, frameSize>>1);
			if (r==-EBADFD)
			{
				envir() << "-EBADFD\n";
				break;
			} else
			if (r==-EAGAIN)
			{
				envir() << "-EAGAIN\n";
				snd_pcm_wait(PCM, 1000);
			} else
			if (r==-EPIPE)
			{
				envir() << "-EPIPE\n";
				snd_pcm_prepare(PCM);
			}
		}  while (r<0);
		
	} else
	{
		init_alsa();
	}
//	printf("cnt=%d\r\n", cnt);
#endif
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  envir() << ".\tduration : " << durationInMicroseconds;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP())
  {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif //DEBUG_PRINT_NPT
  envir() << "\n";
#endif // DEBUG_PRINT_EACH_RECEIVED_FRAME
  // Then continue, to request the next frame of data:
  continuePlaying();
}

Boolean AlsaSink::continuePlaying()
{
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, ALSA_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}
