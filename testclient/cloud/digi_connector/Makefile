# ***************************************************************************
# Copyright (c) 2014 Digi International Inc.,
# All rights not expressly granted are reserved.
# 
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.
# 
# Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
#
# ***************************************************************************
# Use GNU C Compiler
CC = gcc
# Target Platform
PLATFORM = linux

# Location of Private Connector Code.
CONNECTOR_DIR=private
# Locate of Public Include Header Files.
PUBLIC_HEADER_DIR=include
# Location of Platform Src Code.
PLATFORM_DIR=platforms/$(PLATFORM)

# Resolves where to find Source files.
vpath $(CONNECTOR_DIR)/%.c
vpath $(PLATFORM_DIR)/%.c

# CFLAG Definition
CFLAGS += $(DFLAGS)
# Enable Compiler Warnings
CFLAGS += -Winit-self -Wbad-function-cast -Wpointer-arith
CFLAGS += -Wmissing-parameter-type -Wstrict-prototypes -Wformat-security 
CFLAGS += -Wformat-y2k -Wold-style-definition -Wcast-align -Wformat-nonliteral 
CFLAGS += -Wpadded -Wredundant-decls -Wvariadic-macros
CFLAGS += -Wall -Werror -Wextra -pedantic
CFLAGS += -Wno-error=padded -Wno-error=format-nonliteral -Wno-unused-function -Wno-missing-field-initializers 
# Use ANSIC 99
CFLAGS +=-std=c99 
# Include POSIX and GNU features.
CFLAGS += -D_POSIX_C_SOURCE=200112L -D_GNU_SOURCE 
# Include Public Header Files.
CFLAGS += -I. -I$(PUBLIC_HEADER_DIR)
# Include Platform Header Files.
CFLAGS += -I$(PLATFORM_DIR)


ifeq ($(DEBUG),true)
CFLAGS += -g -O0 -DDEBUG
else
# Optimize for Size
CFLAGS += -Os
endif

# Target output to generate.
APP_SRCS = status.c remote_config/device_state.c remote_config/ethernet.c remote_config/devicesecurity.c remote_config/serial.c remote_config/remote_config_cb.c remote_config/system.c remote_config/device_time.c remote_config/device_info.c remote_config/firmware.c remote_config/gps_stats.c cloud_data_device/device_request.c application.c cloud_data_device/process_cloud_request.c cloud_data_device/error_handling.c
PRIVATE_SRCS = $(CONNECTOR_DIR)/connector_api.c
PLATFORM_SRCS = $(PLATFORM_DIR)/os.c $(PLATFORM_DIR)/config.c $(PLATFORM_DIR)/debug.c $(PLATFORM_DIR)/main.c $(PLATFORM_DIR)/network_dns.c $(PLATFORM_DIR)/network_tcp.c  $(PLATFORM_DIR)/file_system.c
SRCS = $(APP_SRCS) $(PLATFORM_SRCS) $(PRIVATE_SRCS)

# Libraries to Link
LIBS = -lc -lz -lpthread

# Generated Sample Executable Name.
SAMPLE = connector

# since each of the samples shares private and platform files, do a clean each time we make
.PHONY:all
all: clean $(SAMPLE)

# Linking Flags.
LDFLAGS += $(DFLAGS) -Wl,-Map,$(SAMPLE).map,--sort-common

OBJS = $(SRCS:.c=.o)

$(SAMPLE): $(OBJS)
	#$(CC) $(LDFLAGS) $^ $(LIBS) -o $@
	$(CC) -shared -Wl,-soname,libdigicloud.so.1 -o libdigicloud.so.1.0 status.o remote_config/device_state.o remote_config/ethernet.o remote_config/devicesecurity.o remote_config/serial.o remote_config/remote_config_cb.o remote_config/system.o remote_config/device_time.o remote_config/device_info.o remote_config/firmware.o remote_config/gps_stats.o cloud_data_device/device_request.o application.o cloud_data_device/process_cloud_request.o cloud_data_device/error_handling.o platforms/linux/os.o platforms/linux/config.o platforms/linux/debug.o platforms/linux/main.o platforms/linux/network_dns.o platforms/linux/network_tcp.o platforms/linux/file_system.o private/connector_api.o
	mv libdigicloud.so.1.0 ../../../lib/.
	ln -sf ../../../lib/libdigicloud.so.1.0 ../../../lib/libdigicloud.so.1
	ln -sf ../../../lib/libdigicloud.so.1.0 ../../../lib/libdigicloud.so
	
	ar -cvq libdigicloud.a status.o remote_config/device_state.o remote_config/ethernet.o remote_config/devicesecurity.o remote_config/serial.o remote_config/remote_config_cb.o remote_config/system.o remote_config/device_time.o remote_config/device_info.o remote_config/firmware.o remote_config/gps_stats.o cloud_data_device/device_request.o application.o cloud_data_device/process_cloud_request.o cloud_data_device/error_handling.o platforms/linux/os.o platforms/linux/config.o platforms/linux/debug.o platforms/linux/main.o platforms/linux/network_dns.o platforms/linux/network_tcp.o platforms/linux/file_system.o private/connector_api.o
	mv libdigicloud.a ../../../lib/.
.PHONY: clean
clean:
	-rm -f $(SAMPLE) $(OBJS) $(SAMPLE).map
	-rm -f ../../../lib/libdigicloud.so.1.0 ../../../lib/libdigicloud.so.1 ../../../lib/libdigicloud.so ../../../lib/libdigicloud.a 
