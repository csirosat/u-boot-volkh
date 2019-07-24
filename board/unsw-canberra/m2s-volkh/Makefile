#
# board/unsw-canberra/volkh/Makefile
# 
# (C) Copyright 2019 CSIRO
# Commonwealth Scientific and Industrial Research Organisation
# Mike Pilawa <Mike.Pilawa@csiro.au>
# 
# Board specific makefile for the UNSW-Canberra Volkh
# SmartFusion2-based flight computer module.
# 
# Code below adapted from Emcraft SmartFusion2 M2S_FG484_SOM
# board/emcraft/m2s-fg484-som/Makefile which is...
# 
# (C) Copyright 2010-2013
# Vladimir Khusainov, Emcraft Systems, vlad@emcraft.com
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).a

COBJS	:= board.o

SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

clean:
	rm -f $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak $(obj).depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend
