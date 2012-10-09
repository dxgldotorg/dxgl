// DXGL
// Copyright (C) 2012 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

int atoi(const char *str)
{
	int neg = 0;
	int ret = 0;
	const char *ptr = str;
	while((*ptr < '0') && (*ptr > '9') && (*ptr != '-'))
	{
		if(*ptr == 0) return 0;
		ptr++;
	}
	if(*ptr == '-')
	{
		neg = 1;
		ptr++;
		if((*ptr < '0') && (*ptr > '9'))
			return 0;
	}
	while((*ptr >= '0') && (*ptr <= '9'))
	{
		ret *= 10;
		if(neg)
		{
			neg = 0;
			ret -= (*ptr - '0');
		}
		else ret += (*ptr - '0');
	}
	return ret;
}