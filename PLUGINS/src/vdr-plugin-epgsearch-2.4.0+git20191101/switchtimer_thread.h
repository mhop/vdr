/*                                                                  -*- c++ -*-
Copyright (C) 2004-2013 Christian Wieninger

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

The author can be reached at cwieninger@gmx.de

The project's page is at http://winni.vdr-developer.org/epgsearch
*/

#ifndef VDR_SWITCHTIMER_THREAD_H
#define VDR_SWITCHTIMER_THREAD_H

#include "switchtimer.h"
#include <vdr/thread.h>

class cSwitchTimerThread: public cThread
{
private:
    bool m_Active;
    time_t m_lastUpdate;
    cCondWait Wait;
protected:
    virtual void Action(void);
    void Stop(void);
public:
    static cSwitchTimerThread *m_Instance;

    cSwitchTimerThread(void);
    virtual ~cSwitchTimerThread();
    static void Init(void);
    static void Exit(void);
};

#endif
