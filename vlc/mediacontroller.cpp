/*****************************************************************************
 * libVLC backend for the Phonon library                                     *
 *                                                                           *
 * Copyright (C) 2007-2008 Tanguy Krotoff <tkrotoff@gmail.com>               *
 * Copyright (C) 2008 Lukas Durfina <lukas.durfina@gmail.com>                *
 * Copyright (C) 2009 Fathi Boudra <fabo@kde.org>                            *
 * Copyright (C) 2009-2010 vlc-phonon AUTHORS                                *
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

#include "mediacontroller.h"

#include <vlc/vlc.h>

#include "debug.h"
#include "globaldescriptioncontainer.h"
#include "libvlc.h"

namespace Phonon
{
namespace VLC
{

MediaController::MediaController()
    : m_player(0)
{
    GlobalSubtitles::instance()->register_(this);
    resetMembers();
}

MediaController::~MediaController()
{
    GlobalSubtitles::instance()->unregister_(this);
}

bool MediaController::hasInterface(Interface iface) const
{
    switch (iface) {
    case AddonInterface::NavigationInterface:
        return true;
        break;
    case AddonInterface::ChapterInterface:
        return true;
        break;
    case AddonInterface::AngleInterface:
        return true;
        break;
    case AddonInterface::TitleInterface:
        return true;
        break;
    case AddonInterface::SubtitleInterface:
        return true;
        break;
    case AddonInterface::AudioChannelInterface:
        return true;
        break;
    default:
        error() << Q_FUNC_INFO << "unsupported AddonInterface::Interface" << iface;
    }

    return false;
}

QVariant MediaController::interfaceCall(Interface iface, int i_command, const QList<QVariant> & arguments)
{
    DEBUG_BLOCK;
    switch (iface) {
    case AddonInterface::ChapterInterface:
        switch (static_cast<AddonInterface::ChapterCommand>(i_command)) {
//        case AddonInterface::availableChapters:
//            return QVariant::fromValue(availableChapters());
        case AddonInterface::availableChapters:
            return availableChapters();
//        case AddonInterface::currentChapter:
//            return QVariant::fromValue(currentChapter());
        case AddonInterface::chapter:
            return currentChapter();
//        case AddonInterface::setCurrentChapter:
//            if( arguments.isEmpty() || !arguments.first().canConvert<ChapterDescription>()) {
//                    error() << Q_FUNC_INFO << "arguments invalid";
//                    return false;
//                }
//            setCurrentChapter(arguments.first().value<ChapterDescription>());
//            return true;
        case AddonInterface::setChapter:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Int)) {
                error() << Q_FUNC_INFO << "arguments invalid";
                return false;
            }
            setCurrentChapter(arguments.first().toInt());
            return true;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::ChapterInterface command:" << i_command;
        }
        break;
    case AddonInterface::TitleInterface:
        switch (static_cast<AddonInterface::TitleCommand>(i_command)) {
//        case AddonInterface::availableTitles:
//            return QVariant::fromValue(availableTitles());
        case AddonInterface::availableTitles:
            return availableTitles();
//        case AddonInterface::currentTitle:
//            return QVariant::fromValue(currentTitle());
        case AddonInterface::title:
            return currentTitle();
//        case AddonInterface::setCurrentTitle:
//            if( arguments.isEmpty() || !arguments.first().canConvert<TitleDescription>()) {
//                    error() << Q_FUNC_INFO << " arguments invalid";
//                    return false;
//            }
//            setCurrentTitle(arguments.first().value<TitleDescription>());
//            return true;
        case AddonInterface::setTitle:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Int)) {
                error() << Q_FUNC_INFO << "arguments invalid";
                return false;
            }
            setCurrentTitle(arguments.first().toInt());
            return true;
        case AddonInterface::autoplayTitles:
            return autoplayTitles();
        case AddonInterface::setAutoplayTitles:
            if (arguments.isEmpty() || !arguments.first().canConvert(QVariant::Bool)) {
                error() << Q_FUNC_INFO << " arguments invalid";
                return false;
            }
            setAutoplayTitles(arguments.first().toBool());
            return true;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::TitleInterface command:" << i_command;
        }
        break;
    case AddonInterface::AngleInterface:
        switch (static_cast<AddonInterface::AngleCommand>(i_command)) {
        case AddonInterface::availableAngles:
        case AddonInterface::angle:
        case AddonInterface::setAngle:
            break;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::AngleInterface command:" << i_command;
        }
        break;
    case AddonInterface::SubtitleInterface:
        switch (static_cast<AddonInterface::SubtitleCommand>(i_command)) {
        case AddonInterface::availableSubtitles:
            return QVariant::fromValue(availableSubtitles());
        case AddonInterface::currentSubtitle:
            return QVariant::fromValue(currentSubtitle());
        case AddonInterface::setCurrentSubtitle:
            if (arguments.isEmpty() || !arguments.first().canConvert<SubtitleDescription>()) {
                error() << Q_FUNC_INFO << "arguments invalid";
                return false;
            }
            setCurrentSubtitle(arguments.first().value<SubtitleDescription>());
            return true;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::SubtitleInterface command:" << i_command;
        }
        break;
    case AddonInterface::AudioChannelInterface:
        switch (static_cast<AddonInterface::AudioChannelCommand>(i_command)) {
        case AddonInterface::availableAudioChannels:
            return QVariant::fromValue(availableAudioChannels());
        case AddonInterface::currentAudioChannel:
            return QVariant::fromValue(currentAudioChannel());
        case AddonInterface::setCurrentAudioChannel:
            if (arguments.isEmpty() || !arguments.first().canConvert<AudioChannelDescription>()) {
                error() << Q_FUNC_INFO << "arguments invalid";
                return false;
            }
            setCurrentAudioChannel(arguments.first().value<AudioChannelDescription>());
            return true;
        default:
            error() << Q_FUNC_INFO << "unsupported AddonInterface::AudioChannelInterface command:" << i_command;
        }
        break;
    default:
        error() << Q_FUNC_INFO << "unsupported AddonInterface::Interface:" << iface;
    }

    return QVariant();
}

void MediaController::resetMediaController()
{
    resetMembers();
    emit availableAudioChannelsChanged();
    emit availableSubtitlesChanged();
    emit availableTitlesChanged(0);
    emit availableChaptersChanged(0);
}

void MediaController::resetMembers()
{
    m_currentAudioChannel = Phonon::AudioChannelDescription();
    m_availableAudioChannels.clear();

    m_currentSubtitle = Phonon::SubtitleDescription();
    GlobalSubtitles::instance()->clearListFor(this);

    m_currentAngle = 0;
    m_availableAngles = 0;

//    m_currentChapter = Phonon::ChapterDescription();
//    m_availableChapters.clear();
    m_currentChapter = 0;
    m_availableChapters = 0;

//    current_title = Phonon::TitleDescription();
//    m_availableTitles.clear();
    m_currentTitle = 0;
    m_availableTitles = 0;

    m_autoPlayTitles = false;
}

// Add audio channel -> in libvlc it is track, it means audio in another language
void MediaController::audioChannelAdded(int id, const QString &lang)
{
    QHash<QByteArray, QVariant> properties;
    properties.insert("name", lang);
    properties.insert("description", "");

    m_availableAudioChannels << Phonon::AudioChannelDescription(id, properties);
    emit availableAudioChannelsChanged();
}

// Add title
void MediaController::titleAdded(int id, const QString &name)
{
//    QHash<QByteArray, QVariant> properties;
//    properties.insert("name", name);
//    properties.insert("description", "");

//    m_availableTitles << Phonon::TitleDescription(id, properties);
    ++m_availableTitles;
    emit availableTitlesChanged(m_availableTitles);
}

// Add chapter
void MediaController::chapterAdded(int titleId, const QString &name)
{
//    QHash<QByteArray, QVariant> properties;
//    properties.insert("name", name);
//    properties.insert("description", "");

//    m_availableChapters << Phonon::ChapterDescription(titleId, properties);
    ++m_availableChapters;
    emit availableChaptersChanged(m_availableChapters);
}

// Audio channel

void MediaController::setCurrentAudioChannel(const Phonon::AudioChannelDescription &audioChannel)
{
    m_currentAudioChannel = audioChannel;
    if (libvlc_audio_set_track(m_player, audioChannel.index())) {
        error() << "libVLC:" << LibVLC::errorMessage();
    }
}

QList<Phonon::AudioChannelDescription> MediaController::availableAudioChannels() const
{
    return m_availableAudioChannels;
}

Phonon::AudioChannelDescription MediaController::currentAudioChannel() const
{
    return m_currentAudioChannel;
}

void MediaController::refreshAudioChannels()
{
    m_currentAudioChannel = Phonon::AudioChannelDescription();
    m_availableAudioChannels.clear();

    libvlc_track_description_t *p_info = libvlc_audio_get_track_description(m_player);
    while (p_info) {
        audioChannelAdded(p_info->i_id, p_info->psz_name);
        p_info = p_info->p_next;
    }
    libvlc_track_description_release(p_info);
}

// Subtitle

void MediaController::setCurrentSubtitle(const Phonon::SubtitleDescription &subtitle)
{
    DEBUG_BLOCK;
    m_currentSubtitle = subtitle;
//    int id = current_subtitle.index();
    QString type = m_currentSubtitle.property("type").toString();

#warning file stuff is untested and probably causes problems for globalsubtitles
    if (type == "file") {
        debug() << "file";
        QString filename = m_currentSubtitle.property("name").toString();
        if (!filename.isEmpty()) {
            if (!libvlc_video_set_subtitle_file(m_player,
                                                filename.toAscii().data())) {
                error() << "libVLC:" << LibVLC::errorMessage();
            }

            // There is no subtitle event inside libvlc so let's send our own event...
            GlobalSubtitles::instance()->add(this, m_currentSubtitle);
            emit availableSubtitlesChanged();
        }
    } else {
        debug() << "no file";
        int localIndex = GlobalSubtitles::instance()->localIdFor(this, subtitle.index());
        debug() << localIndex;
        if (libvlc_video_set_spu(m_player, localIndex)) {
            error() << "libVLC:" << LibVLC::errorMessage();
        }
    }
}

QList<Phonon::SubtitleDescription> MediaController::availableSubtitles() const
{
    return GlobalSubtitles::instance()->listFor(this);
}

Phonon::SubtitleDescription MediaController::currentSubtitle() const
{
    return m_currentSubtitle;
}

void MediaController::refreshSubtitles()
{
    DEBUG_BLOCK;
    m_currentSubtitle = Phonon::SubtitleDescription();
    GlobalSubtitles::instance()->clearListFor(this);

    int idOffset = 0;
    bool idSet = false;
    libvlc_track_description_t *p_info = libvlc_video_get_spu_description(m_player);
    while (p_info) {
#ifdef __GNUC__
#warning In the name of Kent Beck! libvlc is the broken...
#endif
        int id = -1;
        if (p_info->i_id == -1)
            id = 0;

        if (p_info->i_id > 0 && !idSet) {
            idSet = true;
            idOffset = p_info->i_id - 1;
            debug() << idOffset;
        }

        if (id == -1)
            id = p_info->i_id - idOffset;

        GlobalSubtitles::instance()->add(this, id, p_info->psz_name, "");
        p_info = p_info->p_next;
    }
    libvlc_track_description_release(p_info);
    emit availableSubtitlesChanged();
}

// Title

//void MediaController::setCurrentTitle( const Phonon::TitleDescription & title )
void MediaController::setCurrentTitle(int title)
{
    m_currentTitle = title;

//    libvlc_media_player_set_title(m_player, title.index(), vlc_exception);
    libvlc_media_player_set_title(m_player, title);
}

//QList<Phonon::TitleDescription> MediaController::availableTitles() const
int MediaController::availableTitles() const
{
    return m_availableTitles;
}

//Phonon::TitleDescription MediaController::currentTitle() const
int MediaController::currentTitle() const
{
    return m_currentTitle;
}

void MediaController::setAutoplayTitles(bool autoplay)
{
    m_autoPlayTitles = autoplay;
}

bool MediaController::autoplayTitles() const
{
    return m_autoPlayTitles;
}

// Chapter

//void MediaController::setCurrentChapter(const Phonon::ChapterDescription &chapter)
void MediaController::setCurrentChapter(int chapter)
{
    m_currentChapter = chapter;
//    libvlc_media_player_set_chapter(m_player, chapter.index(), vlc_exception);
    libvlc_media_player_set_chapter(m_player, chapter);
}

//QList<Phonon::ChapterDescription> MediaController::availableChapters() const
int MediaController::availableChapters() const
{
    return m_availableChapters;
}

//Phonon::ChapterDescription MediaController::currentChapter() const
int MediaController::currentChapter() const
{
    return m_currentChapter;
}

// We need to rebuild available chapters when title is changed
void MediaController::refreshChapters(int title)
{
//    m_currentChapter = Phonon::ChapterDescription();
//    m_availableChapters.clear();
    m_currentChapter = 0;
    m_availableChapters = 0;

    // Get the description of available chapters for specific title
    libvlc_track_description_t *p_info = libvlc_video_get_chapter_description(m_player, title);
    while (p_info) {
        chapterAdded(p_info->i_id, p_info->psz_name);
        p_info = p_info->p_next;
    }
    libvlc_track_description_release(p_info);
}

// Angle

void MediaController::setCurrentAngle(int angleNumber)
{
    m_currentAngle = angleNumber;
}

int MediaController::availableAngles() const
{
    return m_availableAngles;
}

int MediaController::currentAngle() const
{
    return m_currentAngle;
}

}
} // Namespace Phonon::VLC
