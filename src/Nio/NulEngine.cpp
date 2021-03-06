/*
  ZynAddSubFX - a software synthesizer

  OSSaudiooutput.C - Audio output for Open Sound System
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "NulEngine.h"
#include "../globals.h"
#include "../Misc/Util.h"

#include <iostream>
using namespace std;

namespace zyn {

NulEngine::NulEngine(const SYNTH_T &synth_)
    :AudioOut(synth_), pThread(NULL)
{
    name = "NULL";
    playing_until.tv_sec  = 0;
    playing_until.tv_usec = 0;
}

void *NulEngine::_AudioThread(void *arg)
{
    return (static_cast<NulEngine *>(arg))->AudioThread();
}

void *NulEngine::AudioThread()
{
    while(pThread) {
        getNext();

        struct timeval now;
        int remaining = 0;
        gettimeofday(&now, NULL);
        if((playing_until.tv_usec == 0) && (playing_until.tv_sec == 0)) {
            playing_until.tv_usec = now.tv_usec;
            playing_until.tv_sec  = now.tv_sec;
        }
        else {
            remaining = (playing_until.tv_usec - now.tv_usec)
                        + (playing_until.tv_sec - now.tv_sec) * 1000000;
            if(remaining > 10000) //Don't sleep() less than 10ms.
                //This will add latency...
                os_usleep(remaining  - 10000);
            if(remaining < 0)
                cerr << "WARNING - too late" << endl;
        }
        playing_until.tv_usec += synth.buffersize * 1000000
                                 / synth.samplerate;
        if(remaining < 0)
            playing_until.tv_usec -= remaining;
        playing_until.tv_sec  += playing_until.tv_usec / 1000000;
        playing_until.tv_usec %= 1000000;
    }
    return NULL;
}

NulEngine::~NulEngine()
{}

bool NulEngine::Start()
{
    setAudioEn(true);
    return getAudioEn();
}

void NulEngine::Stop()
{
    setAudioEn(false);
}

void NulEngine::setAudioEn(bool nval)
{
    if(nval) {
        if(!getAudioEn()) {
            pthread_t     *thread = new pthread_t;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            pThread = thread;
            pthread_create(pThread, &attr, _AudioThread, this);
        }
    }
    else
    if(getAudioEn()) {
        pthread_t *thread = pThread;
        pThread = NULL;
        pthread_join(*thread, NULL);
        delete thread;
    }
}

bool NulEngine::getAudioEn() const
{
    return pThread;
}

}
