 
/* Copyright (c) 2010 Katharine Berry All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Katharine Berry nor the names of any contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KATHARINE BERRY AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KATHARINE BERRY OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "llviewerprecompiledheaders.h"
//#include "message.h"
#include "llchat.h"
//#include "llagent.h"
#include "llfloaterchat.h"
//#include "llnotificationmanager.h"
#include "llaudioengine.h"
#include "llstreamingaudio.h"
#include "llviewercontrol.h"
//#include "lltrans.h"
#include "streamtitledisplay.h"

StreamTitleDisplay::StreamTitleDisplay():LLEventTimer(2){ };
BOOL StreamTitleDisplay::tick()
{
	checkMetadata();
	return false;
}
void StreamTitleDisplay::checkMetadata()
{
	if(!gAudiop)
		return;
	if(gAudiop->getStreamingAudioImpl()->hasNewMetadata()&&gSavedSettings.getBOOL("PhoenixShowStreamMetadata"))
	{
		LLChat chat;
		std::string title = gAudiop->getStreamingAudioImpl()->getCurrentTitle();
		std::string artist = gAudiop->getStreamingAudioImpl()->getCurrentArtist();
		// Sometimes we get blanks...
		if(title.length() > 0)
		{
			if(artist.length() > 0)
			{
				chat.mText = "Now playing: " + artist + " - " + title;
			}
			else
			{
				chat.mText = "Now playing: " + title;
			}
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			LLFloaterChat::addChat(chat, FALSE, FALSE);
		}
	}
}