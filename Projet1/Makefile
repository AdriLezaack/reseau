# See gcc/clang manual to understand all flags
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

#Librairies
PATH1 = /usr/include
CFLAGS += -I$(PATH1)

all: clean receiver sender

receiver: receiver.o packet_interface.o create_socket.o real_address.o wait_for_client.o -lz

sender: sender.o packet_interface.o create_socket.o real_address.o wait_for_client.o -lz

clean:
	@rm -f receiver receiver.o sender sender.o packet_interface.o create_socket.o real_address.o wait_for_client.o
	
