/*
   This code was written with reference to the following source.
   source: https://github.com/chrisrouck/tutorial-cpp-audio-capture
*/

#include <stdio.h>
#include <stdlib.h>

#include "portaudio.h"
#include "Constant/VideoConstants.h"

class WinAudioCapture
{
public:
    WinAudioCapture();

private:
    PaStreamParameters input_param;

    static int PaCaptureCallback(
        const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
        void* userData
    );

    void checkErr(PaError err) {
        if (err != paNoError) {
            printf("PortAudio error: %s\n", Pa_GetErrorText(err));
            exit(EXIT_FAILURE);
        }
    }
};


