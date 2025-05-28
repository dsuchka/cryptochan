AM_CFLAGS = -pedantic -Wall -Werror @DEPS_CFLAGS@
AM_LDFLAGS = @DEPS_LDFLAGS@

bin_PROGRAMS = cryptochan test_cyclic_buffer
cryptochan_SOURCES = cryptochan.c common.c random.c ec_helper.c cryptochan_config.c \
    client.c server.c dispatcher.c session.c cyclic_buffer.c

test_cyclic_buffer_SOURCES = test_cyclic_buffer.c common.c random.c cyclic_buffer.c

