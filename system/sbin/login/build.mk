LOGIN_DIR = sbin/login
LOGIN_PROGRAM = build/sbin/login 
PROGRAM += $(LOGIN_PROGRAM)

EXTRA_CLEAN += $(LOGIN_PROGRAM) $(LOGIN_DIR)/*.o

$(LOGIN_PROGRAM): $(LOGIN_DIR)/*.c $(COMMON_OBJ) lib/libewoklibc.a
	mkdir -p build/sbin
	$(CC) $(CFLAGS) -c -o $(LOGIN_DIR)/login.o $(LOGIN_DIR)/login.c
	$(LD) -Ttext=100 $(LOGIN_DIR)/*.o lib/libewoklibc.a $(COMMON_OBJ) -o $(LOGIN_PROGRAM)