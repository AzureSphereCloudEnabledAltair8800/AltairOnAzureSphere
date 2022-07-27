#pragma once

#include <applibs/application.h>
#include <applibs/log.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <tlsutils/deviceauth.h>
#include <wolfssl/ssl.h>

int GetDeviceID(char *deviceId, size_t deviceIdLength);