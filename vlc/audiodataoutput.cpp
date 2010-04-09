/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>
    Copyright (C) 2009 Martin Sandsmark <sandsmark@samfundet.no>
    Copyright (C) 2010 Ben Cooksley <sourtooth@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) version 3, or any
    later version accepted by the membership of KDE e.V. (or its
    successor approved by the membership of KDE e.V.), Nokia Corporation
    (or its successors, if any) and the KDE Free Qt Foundation, which shall
    act as a proxy defined in Section 6 of version 3 of the license.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "audiodataoutput.h"
#include "medianode.h"
#include "mediaobject.h"
#include <QtCore/QVector>
#include <QtCore/QMap>
#include <phonon/audiooutput.h>

namespace Phonon
{
namespace VLC
{
AudioDataOutput::AudioDataOutput(Backend *backend, QObject *parent)
    : SinkNode(parent)
{
}

AudioDataOutput::~AudioDataOutput()
{
}

int AudioDataOutput::dataSize() const
{
    return m_dataSize;
}

int AudioDataOutput::sampleRate() const
{
    return 44100;
}

void AudioDataOutput::setDataSize(int size)
{
    m_dataSize = size;
}

void AudioDataOutput::addToMedia( libvlc_media_t * media )
{
    // WARNING: DO NOT CHANGE ANYTHING HERE FOR CODE CLEANING PURPOSES!
    // WARNING: REQUIRED FOR COMPATIBILITY WITH LIBVLC!
    char param[64];

    // Output to stream renderer
    libvlc_media_add_option_flag( media, "sout=:smem", libvlc_media_option_trusted );

    // Add audio lock callback
    void * lock_call = reinterpret_cast<void*>( &AudioDataOutput::lock );
    sprintf( param, ":sout-smem-audio-prerender-callback=%"PRId64, (qint64)(intptr_t)lock_call );
    libvlc_media_add_option_flag( media, param, libvlc_media_option_trusted );

    // Add audio unlock callback
    void * unlock_call = reinterpret_cast<void*>( &AudioDataOutput::unlock );
    sprintf( param, ":sout-smem-audio-postrender-callback=%"PRId64, (qint64)(intptr_t)unlock_call );
    libvlc_media_add_option_flag( media, param, libvlc_media_option_trusted );

    // Add pointer to ourselves...
    sprintf( param, ":sout-smem-audio-data=%"PRId64, (qint64)(intptr_t)this );
    libvlc_media_add_option_flag( media, param, libvlc_media_option_trusted );
}

typedef QMap<Phonon::AudioDataOutput::Channel, QVector<float> > FloatMap;
typedef QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > IntMap;

inline void AudioDataOutput::convertAndEmit(const QVector<qint16> &leftBuffer, const QVector<qint16> &rightBuffer)
{
    //TODO: Floats
    IntMap map;
    map.insert(Phonon::AudioDataOutput::LeftChannel, leftBuffer);
    map.insert(Phonon::AudioDataOutput::RightChannel, rightBuffer);
    emit dataReady(map);
}

void AudioDataOutput::lock( AudioDataOutput *cw, quint8 **pcm_buffer , quint32 size )
{
}

void AudioDataOutput::unlock( AudioDataOutput *cw, quint8 *pcm_buffer,
                              quint32 channels, quint32 rate,
                              quint32 nb_samples, quint32 bits_per_sample,
                              quint32 size, qint64 pts )
{
    Q_UNUSED( pcm_buffer );
    Q_UNUSED( rate );
    Q_UNUSED( bits_per_sample );
    Q_UNUSED( size );
}

}} //namespace Phonon::VLC

#include "moc_audiodataoutput.cpp"
// vim: sw=4 ts=4

