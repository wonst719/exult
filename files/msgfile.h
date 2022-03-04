/**
 ** Msgfile.h - Read in text message file.
 **
 ** Written: 6/25/03
 **/

/*
 *  Copyright (C) 2002-2022  The Exult Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef INCL_MSGFILE_H
#define INCL_MSGFILE_H 1

#include <iosfwd>
#include <string>
#include <vector>

class IDataSource;

int Read_text_msg_file(
    IDataSource *in,
    std::vector<std::string> &strings,    // Strings returned here, each
    //   allocated on heap.
    const char *section = nullptr
);
int Read_text_msg_file(
    std::istream &in,
    std::vector<std::string> &strings,    // Strings returned here, each
    //   allocated on heap.
    const char *section = nullptr
);
bool Search_text_msg_section(
    IDataSource *in,
    const char *section = nullptr
);
int Read_text_msg_file_sections(
    IDataSource *in,
    std::vector<std::vector<std::string> > &strings,   // Strings returned here
    const char *sections[],         // Section names
    int numsections
);
int Read_text_msg_file_sections(
    std::istream &in,
    std::vector<std::vector<std::string> > &strings,   // Strings returned here
    const char *sections[],         // Section names
    int numsections
);
void Write_msg_file_section(
    std::ostream &out,
    const char *section,
    std::vector<std::string> &items
);

#endif
