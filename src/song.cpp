/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
 *   electricityispower@gmail.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <cassert>
#include <cstring>
#include <boost/format.hpp>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <memory>

#include "song.h"
#include "utility/type_conversions.h"
#include "utility/wide_string.h"
#include "window.h"

namespace {

size_t calc_hash(const char *s, size_t seed = 0)
{
	for (; *s != '\0'; ++s)
		boost::hash_combine(seed, *s);
	return seed;
}

}

namespace MPD {

std::string Song::TagsSeparator = " | ";

std::string Song::get(mpd_tag_type type, unsigned idx) const
{
	std::string result;
	const char *tag = mpd_song_get_tag(m_song.get(), type, idx);
	if (tag)
		result = tag;
	if (isStream() && (type==MPD_TAG_ARTIST || type==MPD_TAG_TITLE))
	{
		const char * artist, * title;
		std::string split_me;
		size_t split_at;
		std::string split_on = " - ";
		artist = mpd_song_get_tag(m_song.get(), MPD_TAG_ARTIST, idx);
		title = mpd_song_get_tag(m_song.get(), MPD_TAG_TITLE, idx);
		if (artist && title)
			return result;
		else if (artist && !title)
			split_me=artist;
		else if (title && !artist)
			split_me=title;
		else
			return result;
		split_at=split_me.find(split_on);
		if (split_at==std::string::npos)
			return result;
		if (type==MPD_TAG_ARTIST)
			return split_me.substr(0,split_at);
		else
			return split_me.substr(split_at+split_on.length());
	}
	return result;
}

Song::Song(mpd_song *s)
{
	assert(s);
	m_song = std::shared_ptr<mpd_song>(s, mpd_song_free);
	m_hash = calc_hash(mpd_song_get_uri(s));
}

std::string Song::getURI(unsigned idx) const
{
	assert(m_song);
	if (idx > 0)
		return "";
	else
		return mpd_song_get_uri(m_song.get());
}

std::string Song::getName(unsigned idx) const
{
	assert(m_song);
	mpd_song *s = m_song.get();
	const char *res = mpd_song_get_tag(s, MPD_TAG_NAME, idx);
	if (res)
		return res;
	else if (idx > 0)
		return "";
	const char *uri = mpd_song_get_uri(s);
	const char *name = strrchr(uri, '/');
	if (name)
		return name+1;
	else
		return uri;
}

std::string Song::getDirectory(unsigned idx) const
{
	assert(m_song);
	if (idx > 0 || isStream())
		return "";
	const char *uri = mpd_song_get_uri(m_song.get());
	const char *name = strrchr(uri, '/');
	if (name)
		return std::string(uri, name-uri);
	else
		return "/";
}

std::string Song::getArtist(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_ARTIST, idx);
}

std::string Song::getTitle(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_TITLE, idx);
}

std::string Song::getAlbum(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_ALBUM, idx);
}

std::string Song::getAlbumArtist(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_ALBUM_ARTIST, idx);
}

std::string Song::getTrack(unsigned idx) const
{
	assert(m_song);
	std::string track = get(MPD_TAG_TRACK, idx);
	if ((track.length() == 1 && track[0] != '0')
	||  (track.length() > 3  && track[1] == '/'))
		track = "0"+track;
	return track;
}

std::string Song::getTrackNumber(unsigned idx) const
{
	assert(m_song);
	std::string track = getTrack(idx);
	size_t slash = track.find('/');
	if (slash != std::string::npos)
		track.resize(slash);
	return track;
}

std::string Song::getDate(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_DATE, idx);
}

std::string Song::getGenre(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_GENRE, idx);
}

std::string Song::getComposer(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_COMPOSER, idx);
}

std::string Song::getPerformer(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_PERFORMER, idx);
}

std::string Song::getDisc(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_DISC, idx);
}

std::string Song::getComment(unsigned idx) const
{
	assert(m_song);
	return get(MPD_TAG_COMMENT, idx);
}

std::string Song::getLength(unsigned idx) const
{
	assert(m_song);
	if (idx > 0)
		return "";
	unsigned len = getDuration();
	if (len > 0)
		return ShowTime(len);
	else
		return "-:--";
}

std::string Song::getPriority(unsigned idx) const
{
	assert(m_song);
	if (idx > 0)
		return "";
	return boost::lexical_cast<std::string>(getPrio());
}

std::string MPD::Song::getTags(GetFunction f) const
{
	assert(m_song);
	unsigned idx = 0;
	std::string result;
	for (std::string tag; !(tag = (this->*f)(idx)).empty(); ++idx)
	{
		if (!result.empty())
			result += TagsSeparator;
		result += tag;
	}
	return result;
}

unsigned Song::getDuration() const
{
	assert(m_song);
	return mpd_song_get_duration(m_song.get());
}

unsigned Song::getPosition() const
{
	assert(m_song);
	return mpd_song_get_pos(m_song.get());
}

unsigned Song::getID() const
{
	assert(m_song);
	return mpd_song_get_id(m_song.get());
}

unsigned Song::getPrio() const
{
	assert(m_song);
	return mpd_song_get_prio(m_song.get());
}

time_t Song::getMTime() const
{
	assert(m_song);
	return mpd_song_get_last_modified(m_song.get());
}

bool Song::isFromDatabase() const
{
	assert(m_song);
	const char *uri = mpd_song_get_uri(m_song.get());
	return uri[0] != '/' || !strrchr(uri, '/');
}

bool Song::isStream() const
{
	assert(m_song);
	return !strncmp(mpd_song_get_uri(m_song.get()), "http://", 7);
}

bool Song::empty() const
{
	return m_song.get() == 0;
}

std::string Song::ShowTime(unsigned length)
{
	int hours = length/3600;
	length -= hours*3600;
	int minutes = length/60;
	length -= minutes*60;
	int seconds = length;
	
	std::string result;
	if (hours > 0)
		result = (boost::format("%d:%02d:%02d") % hours % minutes % seconds).str();
	else
		result = (boost::format("%d:%02d") % minutes % seconds).str();
	return result;
}

}
