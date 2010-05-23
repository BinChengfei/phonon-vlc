/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
 * Copyright (C) 2009-2010 vlc-phonon AUTHORS                                *
 * Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>                     *
 *                                                                           *
 * This program is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Lesser General Public                *
 * License as published by the Free Software Foundation; either              *
 * version 2.1 of the License, or (at your option) any later version.        *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Lesser General Public License for more details.                           *
 *                                                                           *
 * You should have received a copy of the GNU Lesser General Public          *
 * License along with this package; if not, write to the Free Software       *
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA *
 *****************************************************************************/

#include "vlcmediaobject.h"

#include "videowidget.h"
#include "sinknode.h"

#include "vlcloader.h"

#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QtCore/QtDebug>

namespace Phonon
{
namespace VLC {

VLCMediaObject::VLCMediaObject(QObject * parent)
        : MediaObject(parent), VLCMediaController()
{
    // Create an empty Media Player object
    p_vlc_media_player = libvlc_media_player_new(vlc_instance);
    if(!p_vlc_media_player)
        qDebug() << "libvlc exception:" << libvlc_errmsg();
    p_vlc_media_player_event_manager = 0;
    connectToPlayerVLCEvents();

    // Media
    p_vlc_media = 0;
    p_vlc_media_event_manager = 0;

    // Media Discoverer
    p_vlc_media_discoverer = 0;
    p_vlc_media_discoverer_event_manager = 0;

    i_total_time = 0;
    b_has_video = false;
    b_seekable = false;
    p_seek_point = 0;

    connect(this, SIGNAL(metaDataNeedsRefresh()), this, SLOT(updateMetaData()));
    connect(this, SIGNAL(durationChanged(qint64)), this, SLOT(updateDuration(qint64)));
}

VLCMediaObject::~VLCMediaObject()
{
    unloadMedia();

    libvlc_media_player_stop(p_vlc_media_player); // ensure that we are stopped
    libvlc_media_player_release(p_vlc_media_player);
}

void VLCMediaObject::unloadMedia()
{
//    if( p_vlc_media_player ) {
//        libvlc_media_player_release(p_vlc_media_player);
//        p_vlc_media_player = 0;
//    }

    if (p_vlc_media) {
        libvlc_media_release(p_vlc_media);
        p_vlc_media = 0;
    }
}

void VLCMediaObject::loadMediaInternal(const QString & filename)
{
    qDebug() << __FUNCTION__ << filename;

    p_current_file = QUrl::toPercentEncoding(filename, ":/?=&");

    // Why is this needed???
    emit stateChanged(Phonon::StoppedState);
}

void VLCMediaObject::setVLCWidgetId()
{
    // Get our media player to use our window
#if defined(Q_OS_UNIX)
    libvlc_media_player_set_xwindow(p_vlc_media_player, i_video_widget_id);
#elif defined(Q_OS_WIN)
    libvlc_media_player_set_hwnd(p_vlc_media_player, i_video_widget_id);
#elif defined(Q_OS_MAC)
    libvlc_media_player_set_agl(p_vlc_media_player, i_video_widget_id);
#endif
}

void VLCMediaObject::playInternal()
{
    if( p_vlc_media ) { // We are changing media, discard the old one
        libvlc_media_release(p_vlc_media);
        p_vlc_media = 0;
    }

    // Create a media with the given MRL
    p_vlc_media = libvlc_media_new_location(vlc_instance, p_current_file);
    if(!p_vlc_media)
        qDebug() << "libvlc exception:" << libvlc_errmsg();

    foreach( SinkNode * sink, m_sinks ) {
        sink->addToMedia( p_vlc_media );
    }

    // Set the media that will be used by the media player
    libvlc_media_player_set_media(p_vlc_media_player, p_vlc_media);

    // connectToMediaVLCEvents() at the end since it needs to be done for each new libvlc_media_t instance
    connectToMediaVLCEvents();

    // Get meta data (artist, title, etc...)
    updateMetaData();

    // Update available audio channels/subtitles/angles/chapters/etc...
    // i.e everything from MediaController
    // There is no audio channel/subtitle/angle/chapter events inside libvlc
    // so let's send our own events...
    // This will reset the GUI
    clearMediaController();

    setVLCWidgetId();

    // Play
    if(libvlc_media_player_play(p_vlc_media_player))
        qDebug() << "libvlc exception:" << libvlc_errmsg();

    if( p_seek_point != 0 ) { // Workaround that seeking needs to work before the file is being played...
        seekInternal( p_seek_point );
        p_seek_point = 0;
    }
}

void VLCMediaObject::pause()
{
    libvlc_media_player_pause(p_vlc_media_player);
}

void VLCMediaObject::stop()
{
    p_next_source = MediaSource(QUrl());
    libvlc_media_player_stop(p_vlc_media_player);
//    unloadMedia();
}

void VLCMediaObject::seekInternal(qint64 milliseconds)
{
    if( state() != Phonon::PlayingState ) { // Is we aren't playing, seeking is invalid...
        p_seek_point = milliseconds;
    }

    qDebug() << __FUNCTION__ << milliseconds;
    libvlc_media_player_set_time(p_vlc_media_player, milliseconds);
}

QString VLCMediaObject::errorString() const
{
    return libvlc_errmsg();
}

bool VLCMediaObject::hasVideo() const
{
    return b_has_video;
}

bool VLCMediaObject::isSeekable() const
{
    return b_seekable;
}

void VLCMediaObject::connectToPlayerVLCEvents()
{
    // Get the event manager from which the media player send event
    p_vlc_media_player_event_manager = libvlc_media_player_event_manager(p_vlc_media_player);
    libvlc_event_type_t eventsMediaPlayer[] = {
        libvlc_MediaPlayerPlaying,
        libvlc_MediaPlayerPaused,
        libvlc_MediaPlayerEndReached,
        libvlc_MediaPlayerStopped,
        libvlc_MediaPlayerEncounteredError,
        libvlc_MediaPlayerTimeChanged,
        libvlc_MediaPlayerTitleChanged,
        //libvlc_MediaPlayerPositionChanged, // What does this event do???
        libvlc_MediaPlayerSeekableChanged,
        //libvlc_MediaPlayerPausableChanged, // Phonon has no use for this
    };
    int i_nbEvents = sizeof(eventsMediaPlayer) / sizeof(*eventsMediaPlayer);
    for (int i = 0; i < i_nbEvents; i++) {
        libvlc_event_attach(p_vlc_media_player_event_manager, eventsMediaPlayer[i],
                            libvlc_callback, this);
    }
}

void VLCMediaObject::connectToMediaVLCEvents()
{
    // Get event manager from media descriptor object
    p_vlc_media_event_manager = libvlc_media_event_manager(p_vlc_media);
    libvlc_event_type_t eventsMedia[] = {
        libvlc_MediaMetaChanged,
        //libvlc_MediaSubItemAdded, // Could this be used for Audio Channels / Subtitles / Chapter info??
        libvlc_MediaDurationChanged,
        //libvlc_MediaFreed, // Not needed is this?
        //libvlc_MediaStateChanged, // We don't use this? Could we??
    };
    int i_nbEvents = sizeof(eventsMedia) / sizeof(*eventsMedia);
    for (int i = 0; i < i_nbEvents; i++) {
        libvlc_event_attach(p_vlc_media_event_manager, eventsMedia[i], libvlc_callback, this);
    }

    // Get event manager from media service discoverer object
    // FIXME why libvlc_media_discoverer_event_manager() does not take a libvlc_exception_t ?
//    p_vlc_media_discoverer_event_manager = libvlc_media_discoverer_event_manager(p_vlc_media_discoverer);
//    libvlc_event_type_t eventsMediaDiscoverer[] = {
//        libvlc_MediaDiscovererStarted,
//        libvlc_MediaDiscovererEnded
//    };
//    nbEvents = sizeof(eventsMediaDiscoverer) / sizeof(*eventsMediaDiscoverer);
//    for (int i = 0; i < nbEvents; i++) {
//        libvlc_event_attach(p_vlc_media_discoverer_event_manager, eventsMediaDiscoverer[i], libvlc_callback, this, vlc_exception);
//    }
}

void VLCMediaObject::libvlc_callback(const libvlc_event_t *p_event, void *p_user_data)
{
    static int i_first_time_media_player_time_changed = 0;
    static bool b_media_player_title_changed = false;

    VLCMediaObject * const p_vlc_mediaObject = (VLCMediaObject *) p_user_data;

//    qDebug() << (int)p_vlc_mediaObject << "event:" << libvlc_event_type_name(p_event->type);

    // Media player events
    if (p_event->type == libvlc_MediaPlayerSeekableChanged) {
        const bool b_seekable = libvlc_media_player_is_seekable(p_vlc_mediaObject->p_vlc_media_player);
        if (b_seekable != p_vlc_mediaObject->b_seekable) {
            p_vlc_mediaObject->b_seekable = b_seekable;
            emit p_vlc_mediaObject->seekableChanged(p_vlc_mediaObject->b_seekable);
        }
    }
    if (p_event->type == libvlc_MediaPlayerTimeChanged) {

        i_first_time_media_player_time_changed++;

        // FIXME It is ugly. It should be solved by some events in libvlc
        if (i_first_time_media_player_time_changed == 15) {
            // Update metadata
            p_vlc_mediaObject->updateMetaData();

            // Get current video width
            const int i_width = libvlc_video_get_width(p_vlc_mediaObject->p_vlc_media_player);

            // Get current video height
            const int i_height = libvlc_video_get_height(p_vlc_mediaObject->p_vlc_media_player);
            emit p_vlc_mediaObject->videoWidgetSizeChanged(i_width, i_height);

            // Does this media player have a video output
            const bool b_has_video = libvlc_media_player_has_vout(p_vlc_mediaObject->p_vlc_media_player);
            if (b_has_video != p_vlc_mediaObject->b_has_video) {
                p_vlc_mediaObject->b_has_video = b_has_video;
                emit p_vlc_mediaObject->hasVideoChanged(p_vlc_mediaObject->b_has_video);
            }

            if (b_has_video) {
                // Give info about audio tracks
                p_vlc_mediaObject->refreshAudioChannels();
                // Give info about subtitle tracks
                p_vlc_mediaObject->refreshSubtitles();

                // Get movie chapter count
                // It is not a title/chapter media if there is no chapter
                if (libvlc_media_player_get_chapter_count(
                            p_vlc_mediaObject->p_vlc_media_player) > 0) {
                    // Give info about title
                    // only first time, no when title changed
                    if (!b_media_player_title_changed) {
                        libvlc_track_description_t *p_info = libvlc_video_get_title_description(
                                                                 p_vlc_mediaObject->p_vlc_media_player);
                        while (p_info) {
                            p_vlc_mediaObject->titleAdded(p_info->i_id, p_info->psz_name);
                            p_info = p_info->p_next;
                        }
                        libvlc_track_description_release(p_info);
                    }

                    // Give info about chapters for actual title 0
                    if (b_media_player_title_changed)
                        p_vlc_mediaObject->refreshChapters(libvlc_media_player_get_title(
                                                               p_vlc_mediaObject->p_vlc_media_player));
                    else
                        p_vlc_mediaObject->refreshChapters(0);
                }
                if (b_media_player_title_changed)
                    b_media_player_title_changed = false;
            }

            // Bugfix with Qt mediaplayer example
            // Now we are in playing state
            emit p_vlc_mediaObject->stateChanged(Phonon::PlayingState);
        }

        emit p_vlc_mediaObject->tickInternal(p_vlc_mediaObject->currentTime());
    }

    if (p_event->type == libvlc_MediaPlayerPlaying) {
        if (p_vlc_mediaObject->state() != Phonon::BufferingState) {
            // Bugfix with Qt mediaplayer example
            emit p_vlc_mediaObject->stateChanged(Phonon::PlayingState);
        }
    }

    if (p_event->type == libvlc_MediaPlayerPaused) {
        emit p_vlc_mediaObject->stateChanged(Phonon::PausedState);
    }

    if (p_event->type == libvlc_MediaPlayerEndReached && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->clearMediaController();
        emit p_vlc_mediaObject->stateChanged(Phonon::StoppedState);
        emit p_vlc_mediaObject->finished();
    } else if(p_event->type == libvlc_MediaPlayerEndReached) {
        emit p_vlc_mediaObject->moveToNext();
    }

    if(p_event->type == libvlc_MediaPlayerEncounteredError && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->clearMediaController();
        emit p_vlc_mediaObject->stateChanged(Phonon::ErrorState);
        emit p_vlc_mediaObject->finished();
    } else if(p_event->type == libvlc_MediaPlayerEncounteredError) {
        emit p_vlc_mediaObject->moveToNext();
    }

    if (p_event->type == libvlc_MediaPlayerStopped && !p_vlc_mediaObject->checkGaplessWaiting()) {
        i_first_time_media_player_time_changed = 0;
        p_vlc_mediaObject->clearMediaController();
        emit p_vlc_mediaObject->stateChanged(Phonon::StoppedState);
    }

    if (p_event->type == libvlc_MediaPlayerTitleChanged) {
        i_first_time_media_player_time_changed = 0;
        b_media_player_title_changed = true;
    }

    // Media events

    if (p_event->type == libvlc_MediaDurationChanged) {
        emit p_vlc_mediaObject->durationChanged(p_event->u.media_duration_changed.new_duration);
    }

    if (p_event->type == libvlc_MediaMetaChanged) {
        emit p_vlc_mediaObject->metaDataNeedsRefresh();
    }
}

void VLCMediaObject::updateMetaData()
{
    QMultiMap<QString, QString> metaDataMap;

    metaDataMap.insert(QLatin1String("ARTIST"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Artist)));
    metaDataMap.insert(QLatin1String("ALBUM"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Album)));
    metaDataMap.insert(QLatin1String("TITLE"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Title)));
    metaDataMap.insert(QLatin1String("DATE"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Date)));
    metaDataMap.insert(QLatin1String("GENRE"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Genre)));
    metaDataMap.insert(QLatin1String("TRACKNUMBER"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_TrackNumber)));
    metaDataMap.insert(QLatin1String("DESCRIPTION"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Description)));
    metaDataMap.insert(QLatin1String("COPYRIGHT"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_Copyright)));
    metaDataMap.insert(QLatin1String("URL"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_URL)));
    metaDataMap.insert(QLatin1String("ENCODEDBY"),
                       QString::fromUtf8(libvlc_media_get_meta(p_vlc_media, libvlc_meta_EncodedBy)));

    if( metaDataMap == p_vlc_meta_data ) {
        // No need to issue any change, the data is the same
        return;
    }

    p_vlc_meta_data = metaDataMap;

    qDebug() << "updateMetaData(): artist:"
    << libvlc_media_get_meta(p_vlc_media, libvlc_meta_Artist);
    qDebug() << "updateMetaData(): title:"
    << libvlc_media_get_meta(p_vlc_media, libvlc_meta_Title);

    emit metaDataChanged(metaDataMap);
}

qint64 VLCMediaObject::totalTime() const
{
    return i_total_time;
}

qint64 VLCMediaObject::currentTimeInternal() const
{
    return libvlc_media_player_get_time(p_vlc_media_player);
}

void VLCMediaObject::updateDuration(qint64 newDuration)
{
    // If its within 5ms of the current total time, don't bother....
    if( newDuration - 5 > i_total_time || newDuration + 5 < i_total_time ) {
        qDebug() << __FUNCTION__ << "Length changing from " << i_total_time
        << " to " << newDuration;
        i_total_time = newDuration;
        emit totalTimeChanged(i_total_time);
    }
}

void VLCMediaObject::addSink( SinkNode * node )
{
    if( m_sinks.contains( node ) ) {
        // This shouldn't happen....
        return;
    }
    m_sinks.append( node );
}

void VLCMediaObject::removeSink( SinkNode * node )
{
    m_sinks.removeAll( node );
}

void VLCMediaObject::setOption(QString opt)
{
    libvlc_media_add_option_flag(p_vlc_media, opt.toLocal8Bit(), libvlc_media_option_trusted);
}

}
} // Namespace Phonon::VLC
