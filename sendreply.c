/*
    Copyright (C) 2020 Martin Sandsmark

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <systemd/sd-bus.h>
#include <systemd/sd-journal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        sd_journal_print(LOG_ERR, "Invalid args passed, need cookie and authed ID");
        return 1;
    }

    if (getuid()) {
        sd_journal_print(LOG_ERR, "warning: This needs to run as root");
        return 1;
    }

    sd_bus *bus = NULL;
    int ret = sd_bus_open_system(&bus);
    if (ret < 0) {
        sd_journal_print(LOG_ERR, "Failed to connect to system bus: %s", strerror(-ret));
        return 1;
    }

    sd_journal_print(LOG_NOTICE, "Responding, cookie: %s, authed uid: %d\n", argv[1], atoi(argv[2]));

    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *dbusRet = NULL;
    ret = sd_bus_call_method(bus,
                    "org.freedesktop.PolicyKit1",           /* service to contact */
                    "/org/freedesktop/PolicyKit1/Authority",          /* object path */
                    "org.freedesktop.PolicyKit1.Authority",   /* interface name */
                    "AuthenticationAgentResponse2",                         /* method name */
                    &error,                             /* object to return error in */
                    &dbusRet,                           /* return message on success */
                    "us(sa{sv})",/* input signature */

                    // arguments
                    getuid(),
                    argv[1], // cookie
                    "unix-user",
                    1, // number of elements in a(rray)
                    "uid",
                    "u", // v(ariant)'s real type
                    atoi(argv[2]) // authed UID
                    );

    if (ret < 0) {
        sd_journal_print(LOG_ERR, "Failed to issue method call: %s (%s)\n", error.message, strerror(errno));
    }

    sd_bus_error_free(&error);
    sd_bus_message_unref(dbusRet);
    sd_bus_unref(bus);

    return ret;
}

